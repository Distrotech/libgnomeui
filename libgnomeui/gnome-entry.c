/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

/* GnomeEntry widget - combo box with auto-saved history
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <gtk/gtkentry.h>
#include <gtk/gtklist.h>
#include <gtk/gtklistitem.h>
#include <gtk/gtksignal.h>

#include "gnome-entry.h"

#include <libgnome/gnome-i18n.h>

enum {
	PROP_0,
	PROP_HISTORY_ID,
	PROP_GTK_ENTRY
};

#define DEFAULT_MAX_HISTORY_SAVED 10  /* This seems to make more sense then 60*/

struct _GnomeEntryPrivate {
	gchar     *history_id;

	GList     *items;

	guint16    max_saved;
	guint32    changed : 1;
};


struct item {
	gboolean save;
	gchar *text;
};


static void gnome_entry_class_init (GnomeEntryClass *class);
static void gnome_entry_init       (GnomeEntry      *gentry);
static void gnome_entry_destroy    (GtkObject       *object);
static void gnome_entry_finalize   (GObject         *object);

static void gnome_entry_get_property (GObject        *object,
				      guint           param_id,
				      GValue         *value,
				      GParamSpec     *pspec);
static void gnome_entry_set_property (GObject        *object,
				      guint           param_id,
				      const GValue   *value,
				      GParamSpec     *pspec);

static GtkComboClass *parent_class;

guint
gnome_entry_get_type (void)
{
	static guint entry_type = 0;

	if (!entry_type) {
		GtkTypeInfo entry_info = {
			"GnomeEntry",
			sizeof (GnomeEntry),
			sizeof (GnomeEntryClass),
			(GtkClassInitFunc) gnome_entry_class_init,
			(GtkObjectInitFunc) gnome_entry_init,
			NULL,
			NULL,
			NULL
		};

		entry_type = gtk_type_unique (gtk_combo_get_type (), &entry_info);
	}

	return entry_type;
}

static void
gnome_entry_class_init (GnomeEntryClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;

	parent_class = gtk_type_class (gtk_combo_get_type ());
	
	object_class->destroy = gnome_entry_destroy;
		
	gobject_class->finalize = gnome_entry_finalize;
	gobject_class->set_property = gnome_entry_set_property;
	gobject_class->get_property = gnome_entry_get_property;

	g_object_class_install_property (gobject_class,
					 PROP_HISTORY_ID,
					 g_param_spec_string ("history_id",
							      _("History id"),
							      _("History id"),
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_GTK_ENTRY,
					 g_param_spec_object ("gtk_entry",
							      _("GTK entry"),
							      _("The GTK entry"),
							      GTK_TYPE_ENTRY,
							      G_PARAM_READABLE));
}

static void
gnome_entry_get_property (GObject        *object,
			  guint           param_id,
			  GValue         *value,
			  GParamSpec     *pspec)
{
	GnomeEntry *entry = GNOME_ENTRY (object);

	switch (param_id) {
	case PROP_HISTORY_ID:
		g_value_set_string (value, entry->_priv->history_id);
		break;
	case PROP_GTK_ENTRY:
		g_value_set_object (value, gnome_entry_gtk_entry (entry));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
	
}

static void
gnome_entry_set_property (GObject        *object,
			  guint           param_id,
			  const GValue   *value,
			  GParamSpec     *pspec)
{
	GnomeEntry *entry = GNOME_ENTRY (object);

	switch (param_id) {
	case PROP_HISTORY_ID:
		gnome_entry_set_history_id (entry, g_value_get_string (value));
		gnome_entry_load_history (entry);
		break;
		
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
	
}

static void
entry_changed (GtkWidget *widget, gpointer data)
{
	GnomeEntry *gentry;

	gentry = data;
	gentry->_priv->changed = TRUE;
}

static void
entry_activated (GtkWidget *widget, gpointer data)
{
	GnomeEntry *gentry;
	const gchar *text;

	gentry = data;

	text = gtk_entry_get_text (GTK_ENTRY (widget));

	if (!gentry->_priv->changed || (strcmp (text, "") == 0)) {
		gentry->_priv->changed = FALSE;
		return;
	}

	gnome_entry_prepend_history (gentry, TRUE, gtk_entry_get_text (GTK_ENTRY (widget)));
}

static void
gnome_entry_init (GnomeEntry *gentry)
{
	gentry->_priv = g_new0(GnomeEntryPrivate, 1);

	gentry->_priv->changed      = FALSE;
	gentry->_priv->history_id   = NULL;
	gentry->_priv->items        = NULL;
	gentry->_priv->max_saved    = DEFAULT_MAX_HISTORY_SAVED;

	gtk_signal_connect (GTK_OBJECT (gnome_entry_gtk_entry (gentry)), "changed",
			    (GtkSignalFunc) entry_changed,
			    gentry);
	gtk_signal_connect (GTK_OBJECT (gnome_entry_gtk_entry (gentry)), "activate",
			    (GtkSignalFunc) entry_activated,
			    gentry);
	gtk_combo_disable_activate (GTK_COMBO (gentry));
        gtk_combo_set_case_sensitive (GTK_COMBO (gentry), TRUE);
}

/**
 * gnome_entry_construct:
 * @gentry: Pointer to GnomeEntry object.
 * @history_id: If not %NULL, the text id under which history data is stored
 *
 * Constructs a #GnomeEntry object, for language bindings or subclassing
 * use #gnome_entry_new from C
 *
 * Returns: 
 */
void
gnome_entry_construct (GnomeEntry *gentry, 
		       const gchar *history_id)
{
	g_return_if_fail (gentry != NULL);

	gnome_entry_set_history_id (gentry, history_id);
	gnome_entry_load_history (gentry);
}


/**
 * gnome_entry_new
 * @history_id: If not %NULL, the text id under which history data is stored
 *
 * Description: Creates a new GnomeEntry widget.  If  @history_id is
 * not %NULL, then the history list will be saved and restored between
 * uses under the given id.
 *
 * Returns: Newly-created GnomeEntry widget.
 */
GtkWidget *
gnome_entry_new (const gchar *history_id)
{
	GnomeEntry *gentry;

	gentry = gtk_type_new (gnome_entry_get_type ());

	gnome_entry_construct (gentry, history_id);

	return GTK_WIDGET (gentry);
}

static void
free_item (gpointer data, gpointer user_data)
{
	struct item *item;

	item = data;
	if (item->text)
		g_free (item->text);

	g_free (item);
}

static void
free_items (GnomeEntry *gentry)
{
	g_list_foreach (gentry->_priv->items, free_item, NULL);
	g_list_free (gentry->_priv->items);
	gentry->_priv->items = NULL;
}

static void
gnome_entry_destroy (GtkObject *object)
{
	GnomeEntry *gentry;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (object));

	gentry = GNOME_ENTRY (object);

	if(gentry->_priv->history_id) {
		GtkWidget *entry;
		const gchar *text;

		entry = gnome_entry_gtk_entry (gentry);
		text = gtk_entry_get_text (GTK_ENTRY (entry));

		if (gentry->_priv->changed && (strcmp (text, "") != 0)) {
			struct item *item;

			item = g_new (struct item, 1);
			item->save = 1;
			item->text = g_strdup (text);

			gentry->_priv->items = g_list_prepend (gentry->_priv->items, item);
		}

		gnome_entry_save_history (gentry);

		g_free (gentry->_priv->history_id);
		gentry->_priv->history_id = NULL;

		free_items (gentry);
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gnome_entry_finalize (GObject *object)
{
	GnomeEntry *gentry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (object));

	gentry = GNOME_ENTRY (object);

	g_free(gentry->_priv);
	gentry->_priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

/**
 * gnome_entry_gtk_entry
 * @gentry: Pointer to GnomeEntry object.
 *
 * Description: Obtain pointer to GnomeEntry's internal text entry
 *
 * Returns: Pointer to GtkEntry widget.
 */
GtkWidget *
gnome_entry_gtk_entry (GnomeEntry *gentry)
{
	g_return_val_if_fail (gentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ENTRY (gentry), NULL);

	return GTK_COMBO (gentry)->entry;
}

/**
 * gnome_entry_set_history_id
 * @gentry: Pointer to GnomeEntry object.
 * @history_id: If not %NULL, the text id under which history data is stored
 *
 * Description: Set or clear the history id of the GnomeEntry widget.  If
 * @history_id is %NULL, the widget's history id is cleared.  Otherwise,
 * the given id replaces the previous widget history id.
 *
 * Returns:
 */
void
gnome_entry_set_history_id (GnomeEntry *gentry, const gchar *history_id)
{
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	if (gentry->_priv->history_id)
		g_free (gentry->_priv->history_id);

	gentry->_priv->history_id = g_strdup (history_id); /* this handles NULL correctly */
}

/**
 * gnome_entry_get_history_id
 * @gentry: Pointer to GnomeEntry object.
 *
 * Description: Returns the current history id of the GnomeEntry widget.
 *
 * Returns: The current history id.
 */
const gchar *
gnome_entry_get_history_id (GnomeEntry *gentry)
{
	g_return_val_if_fail (gentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ENTRY (gentry), NULL);

	return gentry->_priv->history_id;
}


/**
 * gnome_entry_set_max_saved
 * @gentry: Pointer to GnomeEntry object.
 * @max_saved: Maximum number of history items to save
 *
 * Description: Set internal limit on number of history items saved
 * to the config file, when #gnome_entry_save_history() is called.
 * Zero is an acceptable value for @max_saved, but the same thing is
 * accomplished by setting the history id of @gentry to %NULL.
 *
 * Returns:
 */
void
gnome_entry_set_max_saved (GnomeEntry *gentry, guint max_saved)
{
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	gentry->_priv->max_saved = max_saved;
}

/**
 * gnome_entry_get_max_saved
 * @gentry: Pointer to GnomeEntry object.
 *
 * Description: Get internal limit on number of history items saved
 * to the config file, when #gnome_entry_save_history() is called.
 * See #gnome_entry_set_max_saved().
 *
 * Returns: An unsigned integer
 */
guint
gnome_entry_get_max_saved (GnomeEntry *gentry)
{
	g_return_val_if_fail (gentry != NULL, 0);
	g_return_val_if_fail (GNOME_IS_ENTRY (gentry), 0);

	return gentry->_priv->max_saved;
}

#ifdef FIXME
static char *
build_prefix (GnomeEntry *gentry, gboolean trailing_slash)
{
	return g_strconcat ("/",
			       gnome_program_get_name(gnome_program_get()),
			       "/History: ",
			       gentry->_priv->history_id,
			       trailing_slash ? "/" : "",
			       NULL);
}
#endif

static void
gnome_entry_add_history (GnomeEntry *gentry, gboolean save,
			 const gchar *text, gboolean append)
{
	struct item *item;
	GList *gitem;
	GtkWidget *li;
	GtkWidget *entry;
	gchar *tmp;

	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));
	g_return_if_fail (text != NULL); /* FIXME: should we just return without warning? */

	entry = gnome_entry_gtk_entry (gentry);
	tmp = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
	
	item = g_new (struct item, 1);
	item->save = save;
	item->text = g_strdup (text);

	gentry->_priv->items = g_list_prepend (gentry->_priv->items, item);

	li = gtk_list_item_new_with_label (text);
	gtk_widget_show (li);

	gitem = g_list_append (NULL, li);

	if (append)
	  gtk_list_prepend_items (GTK_LIST (GTK_COMBO (gentry)->list), gitem);
	else
	  gtk_list_append_items (GTK_LIST (GTK_COMBO (gentry)->list), gitem);

	gtk_entry_set_text (GTK_ENTRY (entry), tmp);
	g_free (tmp);

	/* gtk_entry_set_text runs our 'entry_changed' routine, so we have
	   to undo the effect */
	gentry->_priv->changed = FALSE;
}



/**
 * gnome_entry_prepend_history
 * @gentry: Pointer to GnomeEntry object.
 * @save: If %TRUE, history entry will be saved to config file
 * @text: Text to add
 *
 * Description: Adds a history item of the given @text to the head of
 * the history list inside @gentry.  If @save is %TRUE, the history
 * item will be saved in the config file (assuming that @gentry's
 * history id is not %NULL).
 *
 * Returns:
 */
void
gnome_entry_prepend_history (GnomeEntry *gentry, gboolean save,
			     const gchar *text)
{
  gnome_entry_add_history (gentry, save, text, FALSE);
}


/**
 * gnome_entry_append_history
 * @gentry: Pointer to GnomeEntry object.
 * @save: If %TRUE, history entry will be saved to config file
 * @text: Text to add
 *
 * Description: Adds a history item of the given @text to the tail
 * of the history list inside @gentry.  If @save is %TRUE, the
 * history item will be saved in the config file (assuming that
 * @gentry's history id is not %NULL).
 *
 * Returns:
 */
void
gnome_entry_append_history (GnomeEntry *gentry, gboolean save,
			    const gchar *text)
{
	gnome_entry_add_history (gentry, save, text, TRUE);
}



static void
set_combo_items (GnomeEntry *gentry)
{
	GtkList *gtklist;
	GList *items;
	GList *gitems;
	struct item *item;
	GtkWidget *li;
	GtkWidget *entry;
	gchar *tmp;

	gtklist = GTK_LIST (GTK_COMBO (gentry)->list);

	/* We save the contents of the entry because when we insert
	 * items on the combo list, the contents of the entry will get
	 * changed.
	 */

	entry = gnome_entry_gtk_entry (gentry);
	tmp = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
	
	gtk_list_clear_items (gtklist, 0, -1); /* erase everything */

	gitems = NULL;

	for (items = gentry->_priv->items; items; items = items->next) {
		item = items->data;

		li = gtk_list_item_new_with_label (item->text);
		gtk_widget_show (li);
		
		gitems = g_list_append (gitems, li);
	}

	gtk_list_append_items (gtklist, gitems); /* this handles NULL correctly */

	gtk_entry_set_text (GTK_ENTRY (entry), tmp);
	g_free (tmp);

	gentry->_priv->changed = FALSE;
}


/**
 * gnome_entry_load_history
 * @gentry: Pointer to GnomeEntry object.
 *
 * Description: Loads a stored history list from the GNOME config file,
 * if one is available.  If the history id of @gentry is %NULL,
 * nothing occurs.
 *
 * Returns:
 */
void
gnome_entry_load_history (GnomeEntry *gentry)
{
#ifdef FIXME
	gchar *prefix;
	struct item *item;
	gint n;
	gchar key[13];
	gchar *value;

	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	if (!(gnome_program_get_name(gnome_program_get()) && gentry->_priv->history_id))
		return;

	free_items (gentry);

	prefix = build_prefix (gentry, TRUE);
	gnome_config_push_prefix (prefix);
	g_free (prefix);

	for (n = 0; ; n++) {
		g_snprintf (key, sizeof(key), "%d", n);
		value = gnome_config_get_string (key);
		if (!value)
			break;

		item = g_new (struct item, 1);
		item->save = TRUE;
		item->text = value;

		gentry->_priv->items = g_list_append (gentry->_priv->items, item);
	}

	set_combo_items (gentry);

	gnome_config_pop_prefix ();
#endif
}

/**
 * gnome_entry_clear_history
 * @gentry: Pointer to GnomeEntry object.
 *
 * Description:  Clears the history, you should call #gnome_entry_save_history
 * To make the change permanent.
 *
 * Returns:
 */
void
gnome_entry_clear_history (GnomeEntry *gentry)
{
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	free_items (gentry);

	set_combo_items (gentry);
}

static gboolean
check_for_duplicates (struct item **final_items, gint n,
		      const struct item *item)
{
	gint i;
	for (i = 0; i < n; i++) {
		if (final_items[i] &&
		    !strcmp (item->text, final_items[i]->text))
			return FALSE;
	}
	return TRUE;
}


/**
 * gnome_entry_save_history
 * @gentry: Pointer to GnomeEntry object.
 *
 * Description: Force the history items of the widget to be stored
 * in a configuration file.  If the history id of @gentry is %NULL,
 * nothing occurs.
 *
 * Returns:
 */
void
gnome_entry_save_history (GnomeEntry *gentry)
{
#ifdef FIXME
	gchar *prefix;
	GList *items;
	struct item *final_items[DEFAULT_MAX_HISTORY_SAVED];
	struct item *item;
	gint n;
	gchar key[13];

	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	if (!(gnome_program_get_name(gnome_program_get()) && gentry->_priv->history_id))
		return;

	prefix = build_prefix (gentry, TRUE);
	/* a little ugly perhaps, but should speed things up and
	 * prevent us from building this twice */
	prefix[strlen (prefix) - 1] = '\0';
	if (gnome_config_has_section (prefix))
		gnome_config_clean_section (prefix);

	prefix [strlen (prefix)] = '/';
	gnome_config_push_prefix (prefix);
	g_free (prefix);

	for (n = 0, items = gentry->_priv->items; items && n < gentry->_priv->max_saved; items = items->next, n++) {
		item = items->data;

		final_items [n] = NULL;
		if (item->save) {
			if (check_for_duplicates (final_items, n, item)) {
				final_items [n] = item;
				g_snprintf (key, sizeof(key), "%d", n);
				gnome_config_set_string (key, item->text);
			}
		}
	}

	gnome_config_pop_prefix ();
	prefix = g_strconcat ("/",gnome_app_id,NULL);
	gnome_config_sync_file (prefix);
	g_free (prefix);
#endif
}
