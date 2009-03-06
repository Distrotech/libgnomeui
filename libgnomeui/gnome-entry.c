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

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <libgnome/gnome-macros.h>

/* Must be before all other gnome includes!! */
#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#include <gconf/gconf-client.h>
#include <libgnome/gnome-program.h>

#include "gnome-gconf-ui.h"

#include "gnome-entry.h"

enum {
	PROP_0,
	PROP_HISTORY_ID,
	PROP_GTK_ENTRY
};

#define DEFAULT_MAX_HISTORY_SAVED 10  /* This seems to make more sense then 60*/

struct _GnomeEntryPrivate {
	gchar       *history_id;

	GList       *items;

	guint16      max_saved;
	guint        changed : 1;
	guint        saving_history : 1;
	GConfClient *gconf_client;
	guint        gconf_notify_id;
};


struct item {
	gboolean save;
	gchar *text;
};


static void gnome_entry_class_init (GnomeEntryClass *class);
static void gnome_entry_init       (GnomeEntry      *gentry);
static void gnome_entry_finalize   (GObject         *object);
static void gnome_entry_destroy    (GtkObject       *object);

static void gnome_entry_get_property (GObject        *object,
				      guint           param_id,
				      GValue         *value,
				      GParamSpec     *pspec);
static void gnome_entry_set_property (GObject        *object,
				      guint           param_id,
				      const GValue   *value,
				      GParamSpec     *pspec);
static void gnome_entry_editable_init (GtkEditableClass *iface);
static void gnome_entry_load_history (GnomeEntry *gentry);
static void gnome_entry_save_history (GnomeEntry *gentry);

static char *build_gconf_key (GnomeEntry *gentry);

/* Note, can't use boilerplate with interfaces yet,
 * should get sorted out */
static GtkComboClass *parent_class = NULL;
GType
gnome_entry_get_type (void)
{
	static GType object_type = 0;

	if (object_type == 0) {
		const GTypeInfo object_info = {
			sizeof (GnomeEntryClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gnome_entry_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,			/* class_data */
			sizeof (GnomeEntry),
			0,			/* n_preallocs */
			(GInstanceInitFunc) gnome_entry_init,
			NULL			/* value_table */
		};

		const GInterfaceInfo editable_info =
		{
			(GInterfaceInitFunc) gnome_entry_editable_init,	 /* interface_init */
			NULL,			                         /* interface_finalize */
			NULL			                         /* interface_data */
		};

		object_type = g_type_register_static (GTK_TYPE_COMBO, "GnomeEntry", &object_info, 0);

		g_type_add_interface_static (object_type,
					     GTK_TYPE_EDITABLE,
					     &editable_info);
	}
	return object_type;
}

enum {
	ACTIVATE_SIGNAL,
	LAST_SIGNAL
};
static int gnome_entry_signals[LAST_SIGNAL] = {0};

static gboolean
gnome_entry_mnemonic_activate (GtkWidget *widget,
			       gboolean   group_cycling)
{
	gboolean handled;
	GnomeEntry *entry;
	
	entry = GNOME_ENTRY (widget);

	group_cycling = group_cycling != FALSE;

	if (!GTK_WIDGET_IS_SENSITIVE (GTK_COMBO (entry)->entry))
		handled = TRUE;
	else
		g_signal_emit_by_name (GTK_COMBO (entry)->entry, "mnemonic_activate", group_cycling, &handled);

	return handled;
}

static void
gnome_entry_class_init (GnomeEntryClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	GObjectClass *gobject_class;

	parent_class = g_type_class_peek_parent (class);

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;
	
	gnome_entry_signals[ACTIVATE_SIGNAL] =
		g_signal_new("activate",
			     G_TYPE_FROM_CLASS (gobject_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GnomeEntryClass, activate),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);

	class->activate = NULL;

	object_class->destroy = gnome_entry_destroy;
	gobject_class->finalize = gnome_entry_finalize;
	gobject_class->set_property = gnome_entry_set_property;
	gobject_class->get_property = gnome_entry_get_property;
	widget_class->mnemonic_activate = gnome_entry_mnemonic_activate;
	
	g_object_class_install_property (gobject_class,
					 PROP_HISTORY_ID,
					 g_param_spec_string ("history_id",
							      _("History ID"),
							      _("History ID"),
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

	g_signal_emit_by_name (gentry, "changed");
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
	} else {
		gnome_entry_prepend_history (gentry, TRUE, gtk_entry_get_text (GTK_ENTRY (widget)));
	}

	g_signal_emit (gentry, gnome_entry_signals[ACTIVATE_SIGNAL], 0);
}

static void
gnome_entry_init (GnomeEntry *gentry)
{
	gentry->_priv = g_new0(GnomeEntryPrivate, 1);

	gentry->_priv->changed      = FALSE;
	gentry->_priv->history_id   = NULL;
	gentry->_priv->items        = NULL;
	gentry->_priv->max_saved    = DEFAULT_MAX_HISTORY_SAVED;

	g_signal_connect (gnome_entry_gtk_entry (gentry), "changed",
			  G_CALLBACK (entry_changed),
			  gentry);
	g_signal_connect (gnome_entry_gtk_entry (gentry), "activate",
			  G_CALLBACK (entry_activated),
			  gentry);
	gtk_combo_disable_activate (GTK_COMBO (gentry));
        gtk_combo_set_case_sensitive (GTK_COMBO (gentry), TRUE);
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

	gentry = g_object_new (GNOME_TYPE_ENTRY,
			       "history_id", history_id,
			       NULL);

	return GTK_WIDGET (gentry);
}

static void
free_item (gpointer data, gpointer user_data)
{
	struct item *item;

	item = data;
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

	/* Note: destroy can run multiple times */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (object));

	gentry = GNOME_ENTRY (object);

	if (gentry->_priv->gconf_client != NULL) {
		gchar *key;

		if (gentry->_priv->gconf_notify_id != 0) {
			gconf_client_notify_remove
				(gentry->_priv->gconf_client,
				 gentry->_priv->gconf_notify_id);
			gentry->_priv->gconf_notify_id = 0;
		}

		key = build_gconf_key (gentry);
		gconf_client_remove_dir (gentry->_priv->gconf_client,
					 key, NULL);
		g_free (key);

		g_object_unref (G_OBJECT (gentry->_priv->gconf_client));

		gentry->_priv->gconf_client = NULL;
	}

	GNOME_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gnome_entry_finalize (GObject *object)
{
	GnomeEntry *gentry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (object));

	gentry = GNOME_ENTRY (object);

	g_free (gentry->_priv->history_id);
	gentry->_priv->history_id = NULL;

	free_items (gentry);

	g_free (gentry->_priv);
	gentry->_priv = NULL;

	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
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

static void
gnome_entry_history_changed (GConfClient* client,
			     guint cnxn_id,
			     GConfEntry *entry,
			     gpointer user_data)
{
	GnomeEntry *gentry;

	GDK_THREADS_ENTER();

	gentry = GNOME_ENTRY (user_data);

	/* If we're getting a notification from saving our own
	 * history, don't reload it.
	 */
	if (gentry->_priv->saving_history) {
		gentry->_priv->saving_history = FALSE;

		goto end;
	}

	gnome_entry_load_history (gentry);

 end:
	GDK_THREADS_LEAVE();
}

/* FIXME: Make this static */

/**
 * gnome_entry_set_history_id
 * @gentry: Pointer to GnomeEntry object.
 * @history_id: the text id under which history data is stored
 *
 * Description: Set the id of the history list. This function cannot be
 * used to change and already existing id.
 */
void
gnome_entry_set_history_id (GnomeEntry *gentry, const gchar *history_id)
{
	gchar *key;

	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	if (gentry->_priv->history_id != NULL) {
		g_warning ("The program is trying to change an existing "
			   "GnomeEntry history id. This operation is "
			   "not allowed.");

		return;
	}
				
	if (history_id == NULL)
		return;

	gentry->_priv->history_id = g_strdup (history_id);

	/* Register with gconf */
	key = build_gconf_key (gentry);

	gnomeui_gconf_lazy_init ();
	gentry->_priv->gconf_client = gconf_client_get_default ();

	gconf_client_add_dir (gentry->_priv->gconf_client,
			      key,
			      GCONF_CLIENT_PRELOAD_NONE,
			      NULL);

	gentry->_priv->gconf_notify_id = gconf_client_notify_add (gentry->_priv->gconf_client,
								  key,
								  gnome_entry_history_changed,
								  gentry,
								  NULL, NULL);

	g_free (key);
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
 * to the config file.
 * Zero is an acceptable value for @max_saved, but the same thing is
 * accomplished by setting the history id of @gentry to %NULL.
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
 * to the config file
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

static char *
build_gconf_key (GnomeEntry *gentry)
{
	gchar *retval;
	gchar *app_id;

	app_id = gconf_escape_key (gnome_program_get_app_id (gnome_program_get()), -1);

	retval = g_strconcat ("/apps/gnome-settings/",
			      gnome_program_get_app_id (gnome_program_get()),
			      "/history-",
			      gentry->_priv->history_id,
			      NULL);

	g_free (app_id);

	return retval;
}

static void
set_combo_items (GnomeEntry *gentry)
{
	GnomeEntryPrivate *priv;
	GList *l;
	char *text;

	priv = gentry->_priv;

	/* Save the contents of the entry so that setting the new list of
	 * strings will not overwrite it.
	 */

	text = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO (gentry)->entry), 0, -1);

	/* Build the list of strings out of our items */

	if (priv->items) {
		GList *strings = NULL;

		for (l = priv->items; l; l = l->next) {
			struct item *item = l->data;

			strings = g_list_prepend (strings, item->text);
		}

		strings = g_list_reverse (strings);
	
		gtk_combo_set_popdown_strings (GTK_COMBO (gentry), strings);

		g_list_free (strings);
	}
	else
	{
		gtk_list_clear_items (GTK_LIST (GTK_COMBO (gentry)->list), 0, -1);
	}

	/* Restore the text in the entry and clear our changed flag. */

	gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (gentry)->entry), text);
	g_free (text);

	priv->changed = FALSE;
}

static void
gnome_entry_add_history (GnomeEntry *gentry, gboolean save,
			 const gchar *text, gboolean append)
{
	GnomeEntryPrivate *priv;
	struct item *item;
	GList *list;
	gboolean changed;

	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));
	g_return_if_fail (text != NULL); /* FIXME: should we just return without warning? */

	priv = gentry->_priv;

	item = g_new (struct item, 1);
	item->save = save;
	item->text = g_strdup (text);

	/* Remove duplicates */
	changed = FALSE;
	list = priv->items;
	
	while (list != NULL) {
		struct item * data = list->data;
			
		if (strcmp (data->text, text) == 0) {
			free_item (data, NULL);
			priv->items = g_list_delete_link (priv->items, list);
			
			changed = TRUE;
			break;
		}

		list = g_list_next (list);
	}

	/* Really add the history item */
	if (append)
		priv->items = g_list_append (priv->items, item);
	else
		priv->items = g_list_prepend (priv->items, item);

	/* Update combo list */
	set_combo_items (gentry);

	/* Save the history if needed */
	if (changed || save)
		gnome_entry_save_history (gentry);
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
 * Duplicates are automatically removed from the history list.
 * The history list is automatically saved if needed.
 */
void
gnome_entry_prepend_history (GnomeEntry *gentry, gboolean save,
			     const gchar *text)
{
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));
	g_return_if_fail (text != NULL); /* FIXME: should we just return without warning? */
	
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
 * Duplicates are automatically removed from the history list.
 * The history list is automatically saved if needed.
 */
void
gnome_entry_append_history (GnomeEntry *gentry, gboolean save,
			    const gchar *text)
{
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));
	g_return_if_fail (text != NULL); /* FIXME: should we just return without warning? */

	gnome_entry_add_history (gentry, save, text, TRUE);
}


static void
gnome_entry_load_history (GnomeEntry *gentry)
{
	struct item *item;
	gchar *key;
	GSList *gconf_items, *items;

	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	if (gnome_program_get_app_id (gnome_program_get()) == NULL ||
	    gentry->_priv->history_id == NULL)
		return;

	g_return_if_fail (gentry->_priv->gconf_client != NULL);

	free_items (gentry);

	key = build_gconf_key (gentry);

	gconf_items = gconf_client_get_list (gentry->_priv->gconf_client,
					     key, GCONF_VALUE_STRING, NULL);
	g_free (key);

	for (items = gconf_items; items; items = items->next) {

		item = g_new (struct item, 1);
		item->save = TRUE;
		item->text = items->data;

		gentry->_priv->items = g_list_append (gentry->_priv->items, item);
	}

	set_combo_items (gentry);

	g_slist_free (gconf_items);
}

/**
 * gnome_entry_clear_history
 * @gentry: Pointer to GnomeEntry object.
 *
 * Description:  Clears the history.
 */
void
gnome_entry_clear_history (GnomeEntry *gentry)
{
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	free_items (gentry);

	set_combo_items (gentry);

	gnome_entry_save_history (gentry);
}

static gboolean
check_for_duplicates (GSList *gconf_items,
		      const struct item *item)
{

	for (; gconf_items; gconf_items = gconf_items->next) {

		if (strcmp ((gchar *) gconf_items->data, item->text) == 0) {
			return FALSE;
		}
	}

	return TRUE;
}


static void
gnome_entry_save_history (GnomeEntry *gentry)
{
	GList *items;
	GSList *gconf_items;
	struct item *item;
	gchar *key;
	gint n;

	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	if (gnome_program_get_app_id (gnome_program_get ()) == NULL ||
	    gentry->_priv->history_id == NULL)
		return;

	g_return_if_fail (gentry->_priv->gconf_client != NULL);

	key = build_gconf_key (gentry);
	gconf_items = NULL;

	for (n = 0, items = gentry->_priv->items;
	     items && n < gentry->_priv->max_saved;
	     items = items->next, n++) {
		item = items->data;

		if (item->save && check_for_duplicates (gconf_items, item)) {
			gconf_items = g_slist_prepend (gconf_items, item->text);
		}
	}

	gconf_items = g_slist_reverse (gconf_items);

	/* Save the list */
	gentry->_priv->saving_history = TRUE;
	gconf_client_set_list (gentry->_priv->gconf_client, key, GCONF_VALUE_STRING, gconf_items, NULL);

	g_slist_free (gconf_items);
	g_free (key);
}

static void
insert_text (GtkEditable    *editable,
	     const gchar    *text,
	     gint            length,
	     gint           *position)
{
	GtkWidget *entry = gnome_entry_gtk_entry (GNOME_ENTRY (editable));
	gtk_editable_insert_text (GTK_EDITABLE (entry),
				  text,
				  length,
				  position);
}

static void
delete_text (GtkEditable    *editable,
	     gint            start_pos,
	     gint            end_pos)
{
	GtkWidget *entry = gnome_entry_gtk_entry (GNOME_ENTRY (editable));
	gtk_editable_delete_text (GTK_EDITABLE (entry),
				  start_pos,
				  end_pos);
}

static gchar*
get_chars (GtkEditable    *editable,
	   gint            start_pos,
	   gint            end_pos)
{
	GtkWidget *entry = gnome_entry_gtk_entry (GNOME_ENTRY (editable));
	return gtk_editable_get_chars (GTK_EDITABLE (entry),
				       start_pos,
				       end_pos);
}

static void
set_selection_bounds (GtkEditable    *editable,
		      gint            start_pos,
		      gint            end_pos)
{
	GtkWidget *entry = gnome_entry_gtk_entry (GNOME_ENTRY (editable));
	gtk_editable_select_region (GTK_EDITABLE (entry),
				    start_pos,
				    end_pos);
}

static gboolean
get_selection_bounds (GtkEditable    *editable,
		      gint           *start_pos,
		      gint           *end_pos)
{
	GtkWidget *entry = gnome_entry_gtk_entry (GNOME_ENTRY (editable));
	return gtk_editable_get_selection_bounds (GTK_EDITABLE (entry),
						  start_pos,
						  end_pos);
}

static void
set_position (GtkEditable    *editable,
	      gint            position)
{
	GtkWidget *entry = gnome_entry_gtk_entry (GNOME_ENTRY (editable));
	gtk_editable_set_position (GTK_EDITABLE (entry),
				   position);
}

static gint
get_position (GtkEditable    *editable)
{
	GtkWidget *entry = gnome_entry_gtk_entry (GNOME_ENTRY (editable));
	return gtk_editable_get_position (GTK_EDITABLE (entry));
}

/* Copied from gtkentry */
static void
do_insert_text (GtkEditable *editable,
		const gchar *new_text,
		gint         new_text_length,
		gint        *position)
{
	GtkEntry *entry = GTK_ENTRY (gnome_entry_gtk_entry (GNOME_ENTRY (editable)));
	gchar buf[64];
	gchar *text;

	if (*position < 0 || *position > entry->text_length)
		*position = entry->text_length;

	g_object_ref (G_OBJECT (editable));

	if (new_text_length <= 63)
		text = buf;
	else
		text = g_new (gchar, new_text_length + 1);

	text[new_text_length] = '\0';
	strncpy (text, new_text, new_text_length);

	g_signal_emit_by_name (editable, "insert_text", text, new_text_length, position);

	if (new_text_length > 63)
		g_free (text);

	g_object_unref (G_OBJECT (editable));
}

/* Copied from gtkentry */
static void
do_delete_text (GtkEditable *editable,
		gint         start_pos,
		gint         end_pos)
{
	GtkEntry *entry = GTK_ENTRY (gnome_entry_gtk_entry (GNOME_ENTRY (editable)));

	if (end_pos < 0 || end_pos > entry->text_length)
		end_pos = entry->text_length;
	if (start_pos < 0)
		start_pos = 0;
	if (start_pos > end_pos)
		start_pos = end_pos;

	g_object_ref (G_OBJECT (editable));

	g_signal_emit_by_name (editable, "delete_text", start_pos, end_pos);

	g_object_unref (G_OBJECT (editable));
}

static void
gnome_entry_editable_init (GtkEditableClass *iface)
{
	/* Just proxy to the GtkEntry */
	iface->do_insert_text = do_insert_text;
	iface->do_delete_text = do_delete_text;
	iface->insert_text = insert_text;
	iface->delete_text = delete_text;
	iface->get_chars = get_chars;
	iface->set_selection_bounds = set_selection_bounds;
	iface->get_selection_bounds = get_selection_bounds;
	iface->set_position = set_position;
	iface->get_position = get_position;
}
