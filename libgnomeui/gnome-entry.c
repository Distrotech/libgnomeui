/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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
#include <gtk/gtkcombo.h>
#include <gtk/gtklist.h>
#include <gtk/gtklistitem.h>
#include <gtk/gtksignal.h>
#include "libgnome/libgnomeP.h"
#include "gnome-entry.h"

struct _GnomeEntryPrivate {
	gchar     *history_id;

	GList     *items;

	GtkWidget *combo;
	GtkWidget *entry;

	guint32    changed : 1;
};
	

static void   gnome_entry_class_init   (GnomeEntryClass *class);
static void   gnome_entry_init         (GnomeEntry      *gentry);
static void   gnome_entry_destroy      (GtkObject       *object);
static void   gnome_entry_finalize     (GObject         *object);

static gchar *get_entry_text_handler   (GnomeSelector   *selector);
static void   set_entry_text_handler   (GnomeSelector   *selector,
                                        const gchar     *text);
static void   activate_entry_handler   (GnomeSelector   *selector);
static void   history_changed_handler  (GnomeSelector   *selector);
static void   entry_activated_cb       (GtkWidget       *widget,
                                        gpointer         data);


static GnomeSelectorClass *parent_class;

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

		entry_type = gtk_type_unique (gnome_selector_get_type (), &entry_info);
	}

	return entry_type;
}

static void
gnome_entry_class_init (GnomeEntryClass *class)
{
	GnomeSelectorClass *selector_class;
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	selector_class = (GnomeSelectorClass *) class;
	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;

	parent_class = gtk_type_class (gnome_selector_get_type ());

	object_class->destroy = gnome_entry_destroy;
	gobject_class->finalize = gnome_entry_finalize;

	selector_class->get_entry_text = get_entry_text_handler;
	selector_class->set_entry_text = set_entry_text_handler;
	selector_class->activate_entry = activate_entry_handler;
	selector_class->history_changed = history_changed_handler;
}

static void
gnome_entry_init (GnomeEntry *gentry)
{
	gentry->_priv = g_new0(GnomeEntryPrivate, 1);

	gentry->_priv->changed      = FALSE;
	gentry->_priv->items        = NULL;
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
	guint32 flags;

	g_return_if_fail (gentry != NULL);

	gentry->_priv->history_id = g_strdup (history_id);

	flags = GNOME_SELECTOR_DEFAULT_ENTRY_WIDGET;

	gnome_entry_construct_full (gentry, history_id, NULL, NULL,
				    NULL, NULL, flags);
}

void
gnome_entry_construct_full (GnomeEntry *gentry,
			    const gchar *history_id,
			    const gchar *dialog_title,
			    GtkWidget *entry_widget,
			    GtkWidget *selector_widget,
			    GtkWidget *browse_dialog,
			    guint32 flags)
{
	guint32 newflags = flags;

	g_return_if_fail (gentry != NULL);

	/* Create the default selector widget if requested. */
	if (flags & GNOME_SELECTOR_DEFAULT_ENTRY_WIDGET) {
		if (entry_widget != NULL) {
			g_warning (G_STRLOC ": It makes no sense to use "
				   "GNOME_SELECTOR_DEFAULT_ENTRY_WIDGET "
				   "and pass a `entry_widget' as well.");
			return;
		}

		entry_widget = gtk_combo_new ();

		gentry->_priv->combo = entry_widget;
		gentry->_priv->entry = GTK_COMBO (entry_widget)->entry;

		gtk_combo_disable_activate (GTK_COMBO (entry_widget));
		gtk_combo_set_case_sensitive (GTK_COMBO (entry_widget), TRUE);

		gtk_signal_connect (GTK_OBJECT (gentry->_priv->entry),
				    "activate", entry_activated_cb, gentry);

		newflags &= ~GNOME_SELECTOR_DEFAULT_ENTRY_WIDGET;
	}

	gnome_selector_construct (GNOME_SELECTOR (gentry),
				  history_id, dialog_title,
				  entry_widget, selector_widget,
				  browse_dialog, newflags);
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
gnome_entry_destroy (GtkObject *object)
{
	GnomeEntry *gentry;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (object));

	gentry = GNOME_ENTRY (object);

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

	if (gentry->_priv) {
		g_free (gentry->_priv->history_id);
	}

	g_free (gentry->_priv);
	gentry->_priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static gchar *
get_entry_text_handler (GnomeSelector *selector)
{
	GnomeEntry *gentry;
	gchar *text;

	g_return_val_if_fail (selector != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ENTRY (selector), NULL);

	gentry = GNOME_ENTRY (selector);

	text = gtk_entry_get_text (GTK_ENTRY (gentry->_priv->entry));
	return g_strdup (text);
}

static void
set_entry_text_handler (GnomeSelector *selector, const gchar *text)
{
	GnomeEntry *gentry;

	g_return_if_fail (selector != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (selector));

	gentry = GNOME_ENTRY (selector);

	if (gentry->_priv->entry)
		gtk_entry_set_text (GTK_ENTRY (gentry->_priv->entry), text);
}

static void
activate_entry_handler (GnomeSelector *selector)
{
	GnomeEntry *gentry;
	gchar *text;

	g_return_if_fail (selector != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (selector));

	gentry = GNOME_ENTRY (selector);

	text = gtk_entry_get_text (GTK_ENTRY (gentry->_priv->entry));
	gnome_selector_prepend_history (selector, TRUE, text);
}

static void
history_changed_handler (GnomeSelector *selector)
{
	GnomeEntry *gentry;
	GtkWidget *list_widget;
	GList *items = NULL;
	GSList *history_list, *c;

	g_return_if_fail (selector != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (selector));

	gentry = GNOME_ENTRY (selector);

	list_widget = GTK_COMBO (gentry->_priv->combo)->list;

	gtk_list_clear_items (GTK_LIST (list_widget), 0, -1);

	history_list = gnome_selector_get_history (GNOME_SELECTOR (gentry));

	for (c = history_list; c; c = c->next) {
		GtkWidget *item;

		item = gtk_list_item_new_with_label (c->data);
		items = g_list_prepend (items, item);
		gtk_widget_show_all (item);
	}

	items = g_list_reverse (items);

	gtk_list_prepend_items (GTK_LIST (list_widget), items);

	g_slist_foreach (history_list, (GFunc) g_free, NULL);
	g_slist_free (history_list);
}

static void
entry_activated_cb (GtkWidget *widget, gpointer data)
{
	g_return_if_fail (data != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (data));

	gnome_selector_activate_entry (GNOME_SELECTOR (data));
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

	g_warning (G_STRLOC ": This function is deprecated.");

	return GTK_COMBO (gentry->_priv->combo)->entry;
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

	g_warning (G_STRLOC ": This function is deprecated and has no "
		   "effect at all.");
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

	g_warning (G_STRLOC ": This function is deprecated.");

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

	g_warning (G_STRLOC ": This function is deprecated, use "
		   "gnome_selector_set_history_length() instead.");

	gnome_selector_set_history_length (GNOME_SELECTOR (gentry), max_saved);
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

	g_warning (G_STRLOC ": This function is deprecated, use "
		   "gnome_selector_get_history_length() instead.");

	return gnome_selector_get_history_length (GNOME_SELECTOR (gentry));
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
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	gnome_selector_prepend_history (GNOME_SELECTOR (gentry), save, text);
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
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	gnome_selector_append_history (GNOME_SELECTOR (gentry), save, text);
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
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	gnome_selector_load_history (GNOME_SELECTOR (gentry));
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

	gnome_selector_clear_history (GNOME_SELECTOR (gentry));
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
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	gnome_selector_save_history (GNOME_SELECTOR (gentry));
}
