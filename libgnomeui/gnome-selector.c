/* -*- Mode: C; c-set-style: gnu indent-tabs-mode: t; c-basic-offset: 4; tab-width: 8 -*- */
/*
 * Copyright (C) 2000 SuSE GmbH
 * Author: Martin Baulig <baulig@suse.de>
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

/* GnomeSelector widget - pure virtual widget.
 *
 *   Use the Gnome{File,Icon,Pixmap}Selector subclasses.
 *
 * Author: Martin Baulig <baulig@suse.de>
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtksignal.h>
#include "libgnome/libgnomeP.h"
#include "gnome-macros.h"
#include "gnome-selectorP.h"
#include "gnome-uidefs.h"
#include "gnome-gconf.h"


static void gnome_selector_class_init          (GnomeSelectorClass *class);
static void gnome_selector_init                (GnomeSelector      *selector);
static void gnome_selector_destroy             (GtkObject          *object);
static void gnome_selector_finalize            (GObject            *object);

static void gnome_selector_get_param           (GObject            *object,
                                                guint               param_id,
                                                GValue             *value,
                                                GParamSpec         *pspec,
                                                const gchar        *trailer);
static void gnome_selector_set_param           (GObject            *object,
                                                guint               param_id,
                                                GValue             *value,
                                                GParamSpec         *pspec,
                                                const gchar        *trailer);

static void     update_handler                 (GnomeSelector   *selector);
static void     browse_handler                 (GnomeSelector   *selector);
static void     clear_handler                  (GnomeSelector   *selector);
static void     clear_default_handler          (GnomeSelector   *selector);

static gchar   *get_filename_handler           (GnomeSelector   *selector);
static gboolean set_filename_handler           (GnomeSelector   *selector,
                                                const gchar     *filename);
static gboolean check_filename_handler         (GnomeSelector   *selector,
                                                const gchar     *filename);
static void     add_file_handler               (GnomeSelector   *selector,
                                                const gchar     *filename,
                                                gint             position);
static void     add_file_default_handler       (GnomeSelector   *selector,
                                                const gchar     *filename,
                                                gint             position);
static gboolean check_directory_handler        (GnomeSelector   *selector,
                                                const gchar     *filename);
static void     add_directory_handler          (GnomeSelector   *selector,
                                                const gchar     *directory,
                                                gint             position);
static void     add_directory_default_handler  (GnomeSelector   *selector,
                                                const gchar     *directory,
                                                gint             position);
static void     update_file_list_handler       (GnomeSelector   *selector);

static void     free_entry_func                (gpointer         data,
                                                gpointer         user_data);

#define GNOME_SELECTOR_GCONF_DIR "/desktop/standard/gnome-selector"

enum {
    PARAM_0,
};

enum {
    CHANGED_SIGNAL,
    BROWSE_SIGNAL,
    CLEAR_SIGNAL,
    CLEAR_DEFAULT_SIGNAL,
    CHECK_FILENAME_SIGNAL,
    GET_FILENAME_SIGNAL,
    SET_FILENAME_SIGNAL,
    ADD_FILE_SIGNAL,
    ADD_FILE_DEFAULT_SIGNAL,
    CHECK_DIRECTORY_SIGNAL,
    ADD_DIRECTORY_SIGNAL,
    ADD_DIRECTORY_DEFAULT_SIGNAL,
    FREEZE_SIGNAL,
    UPDATE_SIGNAL,
    UPDATE_FILE_LIST_SIGNAL,
    THAW_SIGNAL,
    SET_SELECTION_MODE_SIGNAL,
    GET_SELECTION_SIGNAL,
    SELECTION_CHANGED_SIGNAL,
    SET_ENTRY_TEXT_SIGNAL,
    GET_ENTRY_TEXT_SIGNAL,
    ACTIVATE_ENTRY_SIGNAL,
    HISTORY_CHANGED_SIGNAL,
    DO_ADD_SIGNAL,
    STOP_LOADING_SIGNAL,
    LAST_SIGNAL
};

static int gnome_selector_signals [LAST_SIGNAL] = {0};

/**
 * gnome_selector_get_type
 *
 * Returns the type assigned to the GnomeSelector widget.
 **/
/* The following defines the get_type */
GNOME_CLASS_BOILERPLATE (GnomeSelector, gnome_selector,
			 GtkVBox, gtk_vbox)


typedef gpointer (*GtkSignal_POINTER__NONE) (GtkObject * object,
					     gpointer user_data);
static void
gtk_marshal_POINTER__NONE (GtkObject * object,
			   GtkSignalFunc func, gpointer func_data, GtkArg * args)
{
    GtkSignal_POINTER__NONE rfunc;
    gpointer *return_val;

    return_val = GTK_RETLOC_POINTER (args[0]);
    rfunc = (GtkSignal_POINTER__NONE) func;
    *return_val = (*rfunc) (object, func_data);
}

static void
gnome_selector_class_init (GnomeSelectorClass *class)
{
    GtkObjectClass *object_class;
    GObjectClass *gobject_class;

    object_class = (GtkObjectClass *) class;
    gobject_class = (GObjectClass *) class;

    gnome_selector_signals [CHANGED_SIGNAL] =
	gtk_signal_new ("changed",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   changed),
			gtk_signal_default_marshaller,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [BROWSE_SIGNAL] =
	gtk_signal_new ("browse",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   browse),
			gtk_signal_default_marshaller,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [CLEAR_SIGNAL] =
	gtk_signal_new ("clear",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   clear),
			gtk_signal_default_marshaller,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [CLEAR_DEFAULT_SIGNAL] =
	gtk_signal_new ("clear_default",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   clear_default),
			gtk_signal_default_marshaller,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [FREEZE_SIGNAL] =
	gtk_signal_new ("freeze",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   freeze),
			gtk_signal_default_marshaller,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [UPDATE_SIGNAL] =
	gtk_signal_new ("update",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   update),
			gtk_signal_default_marshaller,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [THAW_SIGNAL] =
	gtk_signal_new ("thaw",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   thaw),
			gtk_signal_default_marshaller,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [CHECK_FILENAME_SIGNAL] =
	gtk_signal_new ("check_filename",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   check_filename),
			gtk_marshal_BOOL__POINTER,
			GTK_TYPE_BOOL, 1,
			GTK_TYPE_STRING);
    gnome_selector_signals [GET_FILENAME_SIGNAL] =
	gtk_signal_new ("get_filename",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   get_filename),
			gtk_marshal_POINTER__NONE,
			GTK_TYPE_POINTER,
			0);
    gnome_selector_signals [SET_FILENAME_SIGNAL] =
	gtk_signal_new ("set_filename",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   set_filename),
			gtk_marshal_BOOL__POINTER,
			GTK_TYPE_BOOL, 1,
			GTK_TYPE_STRING);
    gnome_selector_signals [ADD_FILE_SIGNAL] =
	gtk_signal_new ("add_file",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   add_file),
			gtk_marshal_NONE__POINTER_INT,
			GTK_TYPE_NONE, 2,
			GTK_TYPE_STRING, GTK_TYPE_INT);
    gnome_selector_signals [ADD_FILE_DEFAULT_SIGNAL] =
	gtk_signal_new ("add_file_default",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   add_file_default),
			gtk_marshal_NONE__POINTER_INT,
			GTK_TYPE_NONE, 2,
			GTK_TYPE_STRING, GTK_TYPE_INT);
    gnome_selector_signals [CHECK_DIRECTORY_SIGNAL] =
	gtk_signal_new ("check_directory",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   check_directory),
			gtk_marshal_BOOL__POINTER,
			GTK_TYPE_BOOL, 1,
			GTK_TYPE_STRING);
    gnome_selector_signals [ADD_DIRECTORY_SIGNAL] =
	gtk_signal_new ("add_directory",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   add_directory),
			gtk_marshal_NONE__POINTER_INT,
			GTK_TYPE_NONE, 2,
			GTK_TYPE_STRING, GTK_TYPE_INT);
    gnome_selector_signals [ADD_DIRECTORY_DEFAULT_SIGNAL] =
	gtk_signal_new ("add_directory_default",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   add_directory_default),
			gtk_marshal_NONE__POINTER_INT,
			GTK_TYPE_NONE, 2,
			GTK_TYPE_STRING, GTK_TYPE_INT);
    gnome_selector_signals [UPDATE_FILE_LIST_SIGNAL] =
	gtk_signal_new ("update_file_list",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   update_file_list),
			gtk_signal_default_marshaller,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [SET_SELECTION_MODE_SIGNAL] =
	gtk_signal_new ("set_selection_mode",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   set_selection_mode),
			gtk_marshal_NONE__INT,
			GTK_TYPE_NONE,
			1, GTK_TYPE_INT);
    gnome_selector_signals [GET_SELECTION_SIGNAL] =
	gtk_signal_new ("get_selection",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   get_selection),
			gtk_marshal_POINTER__NONE,
			GTK_TYPE_POINTER,
			0);
    gnome_selector_signals [SELECTION_CHANGED_SIGNAL] =
	gtk_signal_new ("selection_changed",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   selection_changed),
			gtk_signal_default_marshaller,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [GET_ENTRY_TEXT_SIGNAL] =
	gtk_signal_new ("get_entry_text",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   get_entry_text),
			gtk_marshal_POINTER__NONE,
			GTK_TYPE_POINTER,
			0);
    gnome_selector_signals [SET_ENTRY_TEXT_SIGNAL] =
	gtk_signal_new ("set_entry_text",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   set_entry_text),
			gtk_marshal_NONE__POINTER,
			GTK_TYPE_NONE,
			1,
			GTK_TYPE_STRING);
    gnome_selector_signals [ACTIVATE_ENTRY_SIGNAL] =
	gtk_signal_new ("activate_entry",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   activate_entry),
			gtk_marshal_NONE__NONE,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [HISTORY_CHANGED_SIGNAL] =
	gtk_signal_new ("history_changed",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   history_changed),
			gtk_marshal_NONE__NONE,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [DO_ADD_SIGNAL] =
	gtk_signal_new ("do_add",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   do_add),
			gtk_marshal_NONE__INT_POINTER,
			GTK_TYPE_NONE,
			2, GTK_TYPE_STRING, GTK_TYPE_INT);
    gnome_selector_signals [STOP_LOADING_SIGNAL] =
	gtk_signal_new ("stop_loading",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   stop_loading),
			gtk_signal_default_marshaller,
			GTK_TYPE_NONE,
			0);
    gtk_object_class_add_signals (object_class,
				  gnome_selector_signals,
				  LAST_SIGNAL);

    object_class->destroy = gnome_selector_destroy;
    gobject_class->finalize = gnome_selector_finalize;
    gobject_class->get_param = gnome_selector_get_param;
    gobject_class->set_param = gnome_selector_set_param;

    class->browse = browse_handler;
    class->clear = clear_handler;
    class->clear_default = clear_default_handler;
    class->update = update_handler;

    class->check_filename = check_filename_handler;
    class->get_filename = get_filename_handler;
    class->set_filename = set_filename_handler;
    class->add_file = add_file_handler;
    class->add_file_default = add_file_default_handler;
    class->check_directory = check_directory_handler;
    class->add_directory = add_directory_handler;
    class->add_directory_default = add_directory_default_handler;
    class->update_file_list = update_file_list_handler;
}

static void
gnome_selector_set_param (GObject *object, guint param_id, GValue *value,
			  GParamSpec *pspec, const gchar *trailer)
{
    GnomeSelector *selector;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (object));

    selector = GNOME_SELECTOR (object);

    switch (param_id) {
    default:
	g_assert_not_reached ();
	break;
    }
}

static void
gnome_selector_get_param (GObject *object, guint param_id, GValue *value,
			  GParamSpec *pspec, const gchar *trailer)
{
    GnomeSelector *selector;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (object));

    selector = GNOME_SELECTOR (object);

    switch (param_id) {
    default:
	g_assert_not_reached ();
	break;
    }
}

static void
gnome_selector_init (GnomeSelector *selector)
{
    selector->_priv = g_new0 (GnomeSelectorPrivate, 1);

    selector->_priv->changed = FALSE;

    selector->_priv->selector_widget = NULL;
    selector->_priv->browse_dialog = NULL;
}

/*
 * Default signal handlers.
 */

static void
update_handler (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    if (selector->_priv->need_rebuild)
	gnome_selector_update_file_list (selector);

    if (selector->_priv->history_changed)
	gtk_signal_emit (GTK_OBJECT (selector),
			 gnome_selector_signals [HISTORY_CHANGED_SIGNAL]);

    selector->_priv->history_changed = FALSE;
    selector->_priv->dirty = FALSE;
}

static void
update_file_list_handler (GnomeSelector *selector)
{
    GSList *c;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    g_slist_foreach (selector->_priv->total_list, (GFunc) g_free, NULL);
    g_slist_free (selector->_priv->total_list);
    selector->_priv->total_list = NULL;

    g_slist_foreach (selector->_priv->default_total_list,
		     (GFunc) g_free, NULL);
    g_slist_free (selector->_priv->default_total_list);
    selector->_priv->default_total_list = NULL;

    for (c = selector->_priv->dir_list; c; c = c->next)
	gtk_signal_emit (GTK_OBJECT (selector),
			 gnome_selector_signals [ADD_DIRECTORY_SIGNAL],
			 c->data, 0);

    for (c = selector->_priv->default_dir_list; c; c = c->next)
	gtk_signal_emit (GTK_OBJECT (selector),
			 gnome_selector_signals [ADD_DIRECTORY_DEFAULT_SIGNAL],
			 c->data, 0);

    for (c = selector->_priv->file_list; c; c = c->next)
	gtk_signal_emit (GTK_OBJECT (selector),
			 gnome_selector_signals [ADD_FILE_SIGNAL],
			 c->data, 0);

    for (c = selector->_priv->default_file_list; c; c = c->next)
	gtk_signal_emit (GTK_OBJECT (selector),
			 gnome_selector_signals [ADD_FILE_DEFAULT_SIGNAL],
			 c->data, 0);

    selector->_priv->total_list = g_slist_reverse
	(selector->_priv->total_list);

    selector->_priv->default_total_list = g_slist_reverse
	(selector->_priv->default_total_list);

    selector->_priv->need_rebuild = FALSE;
}

static void
browse_handler (GnomeSelector *selector)
{
    GnomeSelectorPrivate *priv;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    priv = selector->_priv;

    /*if it already exists make sure it's shown and raised*/
    if (priv->browse_dialog) {
	gtk_widget_show (priv->browse_dialog);
	if (priv->browse_dialog->window)
	    gdk_window_raise (priv->browse_dialog->window);
    }
}

static void
free_entry_func (gpointer data, gpointer user_data)
{
    g_free (data);
}

static void
clear_handler (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    g_slist_foreach (selector->_priv->dir_list, free_entry_func,
		     selector);
    g_slist_free (selector->_priv->dir_list);
    selector->_priv->dir_list = NULL;

    g_slist_foreach (selector->_priv->file_list, free_entry_func,
		     selector);
    g_slist_free (selector->_priv->file_list);
    selector->_priv->file_list = NULL;
}

static void
clear_default_handler (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    g_slist_foreach (selector->_priv->default_dir_list, free_entry_func,
		     selector);
    g_slist_free (selector->_priv->default_dir_list);
    selector->_priv->default_dir_list = NULL;

    g_slist_foreach (selector->_priv->default_file_list, free_entry_func,
		     selector);
    g_slist_free (selector->_priv->default_file_list);
    selector->_priv->default_file_list = NULL;
}

static gboolean
check_filename_handler (GnomeSelector *selector, const gchar *filename)
{
    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);
    g_return_val_if_fail (filename != NULL, FALSE);

    g_message (G_STRLOC ": '%s'", filename);

    return TRUE;
}

static gchar *
get_filename_handler (GnomeSelector *selector)
{
    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);

    return gnome_selector_get_entry_text (selector);
}

static gboolean
set_filename_handler (GnomeSelector *selector, const gchar *filename)
{
    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);

    if (!filename) {
	gnome_selector_set_entry_text (selector, NULL);
	gnome_selector_activate_entry (selector);
	return TRUE;
    }

    g_message (G_STRLOC ": '%s'", filename);

    if (!gnome_selector_check_filename (selector, filename))
	return FALSE;

    gnome_selector_set_entry_text (selector, filename);
    gnome_selector_activate_entry (selector);

    return TRUE;
}

static gboolean
check_directory_handler (GnomeSelector *selector, const gchar *directory)
{
    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);
    g_return_val_if_fail (directory != NULL, FALSE);

    g_message (G_STRLOC ": '%s'", directory);

    return TRUE;
}

static void
add_file_handler (GnomeSelector *selector, const gchar *filename,
		  gint position)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));
    g_return_if_fail (filename != NULL);
    g_return_if_fail (position >= -1);

    if (!gnome_selector_check_filename (selector, filename))
	return;

    if (position == -1)
	selector->_priv->total_list = g_slist_append
	    (selector->_priv->total_list, g_strdup (filename));
    else
	selector->_priv->total_list = g_slist_insert
	    (selector->_priv->total_list, g_strdup (filename),
	     position);
}

static void
add_file_default_handler (GnomeSelector *selector, const gchar *filename,
			  gint position)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));
    g_return_if_fail (filename != NULL);
    g_return_if_fail (position >= -1);
 
    if (!gnome_selector_check_filename (selector, filename))
	return;

    if (position == -1)
	selector->_priv->default_total_list = g_slist_append
	    (selector->_priv->default_total_list, g_strdup (filename));
    else
	selector->_priv->default_total_list = g_slist_insert
	    (selector->_priv->default_total_list, g_strdup (filename),
	     position);
}

static void
add_directory_handler (GnomeSelector *selector, const gchar *directory,
		       gint position)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));
    g_return_if_fail (directory != NULL);
    g_return_if_fail (position >= -1);

    if (!gnome_selector_check_directory (selector, directory))
	return;

    if (position == -1)
	selector->_priv->dir_list = g_slist_append
	    (selector->_priv->dir_list, g_strdup (directory));
    else
	selector->_priv->dir_list = g_slist_insert
	    (selector->_priv->dir_list, g_strdup (directory),
	     position);
}

static void
add_directory_default_handler (GnomeSelector *selector, const gchar *directory,
			       gint position)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));
    g_return_if_fail (directory != NULL);
    g_return_if_fail (position >= -1);

    if (!gnome_selector_check_directory (selector, directory))
	return;

    if (position == -1)
	selector->_priv->default_dir_list = g_slist_append
	    (selector->_priv->default_dir_list, g_strdup (directory));
    else
	selector->_priv->default_dir_list = g_slist_insert
	    (selector->_priv->default_dir_list, g_strdup (directory),
	     position);
}


/*
 * Misc callbacks.
 */

static void
browse_clicked_cb (GtkWidget *widget, gpointer data)
{
    gtk_signal_emit (GTK_OBJECT (data),
		     gnome_selector_signals [BROWSE_SIGNAL]);
}

static void
default_clicked_cb (GtkWidget *widget, gpointer data)
{
    gnome_selector_set_to_defaults (GNOME_SELECTOR (data));
}

static void
clear_clicked_cb (GtkWidget *widget, gpointer data)
{
    gnome_selector_clear (GNOME_SELECTOR (data), FALSE);
}


/**
 * gnome_selector_construct:
 * @selector: Pointer to GnomeSelector object.
 * @history_id: If not %NULL, the text id under which history data is stored
 * @selector_widget: Widget which should be used inside the selector box.
 * @browse_dialog: Widget which should be used as browse dialog.
 *
 * Constructs a #GnomeSelector object, for language bindings or subclassing
 * use #gnome_selector_new from C
 *
 * Returns: 
 */
void
gnome_selector_construct (GnomeSelector *selector, 
			  const gchar *history_id,
			  const gchar *dialog_title,
			  GtkWidget *entry_widget,
			  GtkWidget *selector_widget,
			  GtkWidget *browse_dialog,
			  guint32 flags)
{
    GnomeSelectorPrivate *priv;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    priv = selector->_priv;

    priv->entry_widget = entry_widget;
    if (priv->entry_widget)
	gtk_widget_ref (priv->entry_widget);

    priv->history_id = g_strdup (history_id);
    priv->dialog_title = g_strdup (dialog_title);

    priv->flags = flags;

    priv->client = gnome_get_gconf_client ();
    gtk_object_ref (GTK_OBJECT (priv->client));

    if (priv->history_id) {
	priv->gconf_history_dir = gconf_concat_key_and_dir
	    (GNOME_SELECTOR_GCONF_DIR, priv->history_id);
	priv->gconf_history_key = gconf_concat_key_and_dir
	    (priv->gconf_history_dir, "history");
	priv->gconf_dir_list_key = gconf_concat_key_and_dir
	    (priv->gconf_history_dir, "dir-list");
	priv->gconf_file_list_key = gconf_concat_key_and_dir
	    (priv->gconf_history_dir, "file-list");

	gconf_client_add_dir (priv->client, priv->gconf_history_dir,
			      GCONF_CLIENT_PRELOAD_NONE, NULL);
    }

    priv->selector_widget = selector_widget;
    if (priv->selector_widget)
	gtk_widget_ref (priv->selector_widget);

    priv->browse_dialog = browse_dialog;
    if (priv->browse_dialog)
	gtk_widget_ref (priv->browse_dialog);

    priv->box = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);

    priv->hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);

    if (priv->entry_widget) {
	gtk_box_pack_start (GTK_BOX (priv->hbox), priv->entry_widget,
			    TRUE, TRUE, 0);
    }

    gtk_box_pack_start (GTK_BOX (priv->box), priv->hbox,
			TRUE, FALSE, GNOME_PAD_SMALL);

    if (flags & GNOME_SELECTOR_WANT_BROWSE_BUTTON) {
	if (priv->browse_dialog == NULL) {
	    g_warning (G_STRLOC ": It makes no sense to use "
		       "GNOME_SELECTOR_WANT_BROWSE_BUTTON "
		       "without having a `browse_dialog'.");
	    return;
	}

	priv->browse_button = gtk_button_new_with_label (_("Browse..."));

	gtk_signal_connect (GTK_OBJECT (priv->browse_button),
			    "clicked", browse_clicked_cb, selector);

	gtk_box_pack_start (GTK_BOX (priv->hbox),
			    priv->browse_button, FALSE, FALSE, 0);
    }

    if (flags & GNOME_SELECTOR_WANT_DEFAULT_BUTTON) {
	priv->default_button = gtk_button_new_with_label (_("Default..."));

	gtk_signal_connect (GTK_OBJECT (priv->default_button),
			    "clicked", default_clicked_cb, selector);

	gtk_box_pack_start (GTK_BOX (priv->hbox),
			    priv->default_button, FALSE, FALSE, 0);
    }

    if (flags & GNOME_SELECTOR_WANT_CLEAR_BUTTON) {
	priv->clear_button = gtk_button_new_with_label (_("Clear..."));

	gtk_signal_connect (GTK_OBJECT (priv->clear_button),
			    "clicked", clear_clicked_cb, selector);

	gtk_box_pack_start (GTK_BOX (priv->hbox),
			    priv->clear_button, FALSE, FALSE, 0);
    }

    if (priv->selector_widget) {
	gtk_box_pack_start (GTK_BOX (priv->box),
			    priv->selector_widget, TRUE, TRUE,
			    GNOME_PAD_SMALL);
    }

    gtk_widget_show_all (priv->box);

    gtk_box_pack_start (GTK_BOX (selector), priv->box,
			TRUE, TRUE, GNOME_PAD_SMALL);

    if (priv->flags & GNOME_SELECTOR_AUTO_SAVE_HISTORY)
	gnome_selector_load_history (selector);

    if (priv->flags & GNOME_SELECTOR_AUTO_SAVE_ALL)
	_gnome_selector_load_all (selector);
}


static void
gnome_selector_destroy (GtkObject *object)
{
    GnomeSelector *selector;

    /* remember, destroy can be run multiple times! */

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (object));

    selector = GNOME_SELECTOR (object);

    if (selector->_priv->client) {
	if (selector->_priv->flags & GNOME_SELECTOR_AUTO_SAVE_ALL)
	    _gnome_selector_save_all (selector);

	if (selector->_priv->gconf_history_dir)
	    gconf_client_remove_dir
		(selector->_priv->client,
		 selector->_priv->gconf_history_dir,
		 NULL);

	gtk_object_unref (GTK_OBJECT (selector->_priv->client));
	selector->_priv->client = NULL;
    }

    if (selector->_priv->selector_widget) {
	gtk_widget_unref (selector->_priv->selector_widget);
	selector->_priv->selector_widget = NULL;
    }

    if (selector->_priv->browse_dialog) {
	gtk_widget_unref (selector->_priv->browse_dialog);
	selector->_priv->browse_dialog = NULL;
    }

    if (selector->_priv->entry_widget) {
	gtk_widget_unref (selector->_priv->entry_widget);
	selector->_priv->entry_widget = NULL;
    }

    GNOME_CALL_PARENT_HANDLER (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gnome_selector_finalize (GObject *object)
{
    GnomeSelector *selector;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (object));

    selector = GNOME_SELECTOR (object);

    if (selector->_priv) {
	g_slist_foreach (selector->_priv->dir_list,
			 free_entry_func, selector);
	g_slist_foreach (selector->_priv->file_list,
			 free_entry_func, selector);
	g_free (selector->_priv->dialog_title);
	g_free (selector->_priv->history_id);
	g_free (selector->_priv->gconf_history_dir);
	g_free (selector->_priv->gconf_history_key);
	g_free (selector->_priv->gconf_dir_list_key);
	g_free (selector->_priv->gconf_file_list_key);
    }

    g_free (selector->_priv);
    selector->_priv = NULL;

    GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, finalize, (object));
}


/**
 * gnome_selector_get_dialog_title
 * @selector: Pointer to GnomeSelector object.
 *
 * Description: Returns the titel of the popup dialog.
 *
 * Returns: Titel of the popup dialog.
 */
const gchar *
gnome_selector_get_dialog_title (GnomeSelector *selector)
{
    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

    return selector->_priv->dialog_title;
}


/**
 * gnome_selector_set_dialog_title
 * @selector: Pointer to GnomeSelector object.
 * @dialog_title: New title for the popup dialog.
 *
 * Description: Sets the titel of the popup dialog.
 *
 * This is only used when the widget uses a popup dialog for
 * the actual selector.
 *
 * Returns:
 */
void
gnome_selector_set_dialog_title (GnomeSelector *selector,
				 const gchar *dialog_title)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    if (selector->_priv->dialog_title) {
	g_free (selector->_priv->dialog_title);
	selector->_priv->dialog_title = NULL;
    }

    /* this is NULL safe. */
    selector->_priv->dialog_title = g_strdup (dialog_title);
}


/**
 * gnome_selector_check_directory
 * @selector: Pointer to GnomeSelector object.
 * @filename: Directory to check.
 *
 * Description: Asks the derived class whether @directory is a
 *    valid directory for this kind of selector.
 *
 * Returns: #TRUE if the directory is ok or #FALSE if not.
 */
gboolean
gnome_selector_check_directory (GnomeSelector *selector,
				const gchar *directory)
{
    gboolean ok;

    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);
    g_return_val_if_fail (directory != NULL, FALSE);

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [CHECK_DIRECTORY_SIGNAL],
		     directory, &ok);

    return ok;
}



/**
 * gnome_selector_prepend_directory
 * @selector: Pointer to GnomeSelector object.
 * @directory: The directory to add.
 *
 * Description: Prepends @directory to the directory list.
 *
 * Returns:
 */
gboolean
gnome_selector_prepend_directory (GnomeSelector *selector,
				  const gchar *directory,
				  gboolean defaultp)
{
    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);
    g_return_val_if_fail (directory != NULL, FALSE);

    return gnome_selector_add_directory (selector, directory, 0,
					 defaultp);
}


/**
 * gnome_selector_append_directory
 * @selector: Pointer to GnomeSelector object.
 * @directory: The directory to add.
 *
 * Description: Append @directory to the directory list.
 *
 * Returns:
 */
gboolean
gnome_selector_append_directory (GnomeSelector *selector,
				 const gchar *directory,
				 gboolean defaultp)
{
    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);
    g_return_val_if_fail (directory != NULL, FALSE);

    return gnome_selector_add_directory (selector, directory, -1,
					 defaultp);
}


/**
 * gnome_selector_add_directory
 * @selector: Pointer to GnomeSelector object.
 * @directory: The directory to add.
 *
 * Description:
 *
 * Returns:
 */
gboolean
gnome_selector_add_directory (GnomeSelector *selector, const gchar *directory,
			      gint position, gboolean defaultp)
{
    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);
    g_return_val_if_fail (directory != NULL, FALSE);
    g_return_val_if_fail (position >= -1, FALSE);

    if (defaultp)
	gtk_signal_emit (GTK_OBJECT (selector),
			 gnome_selector_signals [ADD_DIRECTORY_DEFAULT_SIGNAL],
			 directory, position);
    else
	gtk_signal_emit (GTK_OBJECT (selector),
			 gnome_selector_signals [ADD_DIRECTORY_SIGNAL],
			 directory, position);

    return TRUE;
}


/**
 * gnome_selector_prepend_file
 * @selector: Pointer to GnomeSelector object.
 * @directory: The file to add.
 *
 * Description: Prepends @filename to the file list.
 *
 * Returns:
 */
gboolean
gnome_selector_prepend_file (GnomeSelector *selector,
			     const gchar *filename,
			     gboolean defaultp)
{
    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);
    g_return_val_if_fail (filename != NULL, FALSE);

    return gnome_selector_add_file (selector, filename, 0, defaultp);
}


/**
 * gnome_selector_append_file
 * @selector: Pointer to GnomeSelector object.
 * @directory: The file to add.
 *
 * Description: Appends @filename to the file list.
 *
 * Returns:
 */
gboolean
gnome_selector_append_file (GnomeSelector *selector,
			    const gchar *filename,
			    gboolean defaultp)
{
    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);
    g_return_val_if_fail (filename != NULL, FALSE);

    return gnome_selector_add_file (selector, filename, -1, defaultp);
}

/**
 * gnome_selector_add_file
 * @selector: Pointer to GnomeSelector object.
 * @filename: The file to add.
 *
 * Description:
 *
 * Returns:
 */
gboolean
gnome_selector_add_file (GnomeSelector *selector, const gchar *filename,
			 gint position, gboolean defaultp)
{
    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);
    g_return_val_if_fail (filename != NULL, FALSE);
    g_return_val_if_fail (position >= -1, FALSE);

    if (defaultp)
	gtk_signal_emit (GTK_OBJECT (selector),
			 gnome_selector_signals [ADD_FILE_DEFAULT_SIGNAL],
			 filename, position);
    else
	gtk_signal_emit (GTK_OBJECT (selector),
			 gnome_selector_signals [ADD_FILE_SIGNAL],
			 filename, position);

    return TRUE;
}

GSList *
gnome_selector_get_file_list (GnomeSelector *selector,
			      gboolean include_dir_list,
			      gboolean defaultp)
{
    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

    if (selector->_priv->need_rebuild)
	gnome_selector_update_file_list (selector);

    if (include_dir_list)
	if (defaultp)
	    return selector->_priv->default_total_list;
	else
	    return selector->_priv->total_list;
    else
	if (defaultp)
	    return selector->_priv->default_file_list;
	else
	    return selector->_priv->file_list;
}


void
gnome_selector_set_selection_mode (GnomeSelector *selector,
				   GtkSelectionMode mode)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [SET_SELECTION_MODE_SIGNAL],
		     (gint) mode);
}


GSList *
gnome_selector_get_selection (GnomeSelector *selector)
{
    GSList *retval = NULL;

    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [GET_SELECTION_SIGNAL],
		     &retval);

    return retval;
}


/**
 * gnome_selector_update
 * @selector: Pointer to GnomeSelector object.
 *
 * Description: Updates the file list.
 *
 * Returns:
 */
void
gnome_selector_update (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [UPDATE_SIGNAL]);
}


/**
 * gnome_selector_clear
 * @selector: Pointer to GnomeSelector object.
 *
 * Description: Removes all entries from the selector.
 *
 * Returns:
 */
void
gnome_selector_clear (GnomeSelector *selector, gboolean defaultp)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    gnome_selector_freeze (selector);
    if (defaultp)
	gtk_signal_emit (GTK_OBJECT (selector),
			 gnome_selector_signals [CLEAR_DEFAULT_SIGNAL]);
    else
	gtk_signal_emit (GTK_OBJECT (selector),
			 gnome_selector_signals [CLEAR_SIGNAL]);
    gnome_selector_thaw (selector);

    selector->_priv->need_rebuild = TRUE;

    if (selector->_priv->frozen)
	selector->_priv->dirty = TRUE;
    else
	gnome_selector_update (selector);
}


/**
 * gnome_selector_check_filename
 * @selector: Pointer to GnomeSelector object.
 * @filename: Filename to check.
 *
 * Description: Asks the derived class whether @filename is a
 *    valid file for this kind of selector.
 *
 * This can be used in derived classes to only allow certain
 * file types, for instance images, in the selector.
 *
 * Returns: #TRUE if the filename is ok or #FALSE if not.
 */
gboolean
gnome_selector_check_filename (GnomeSelector *selector,
			       const gchar *filename)
{
    gboolean ok;

    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);
    g_return_val_if_fail (filename != NULL, FALSE);

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [CHECK_FILENAME_SIGNAL],
		     filename, &ok);

    return ok;
}


/**
 * gnome_selector_get_filename
 * @selector: Pointer to GnomeSelector object.
 *
 * Description:
 *
 * Returns:
 */
gchar *
gnome_selector_get_filename (GnomeSelector *selector)
{
    gchar *retval = NULL;

    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [GET_FILENAME_SIGNAL],
		     &retval);

    return retval;
}


/**
 * gnome_selector_set_filename
 * @selector: Pointer to GnomeSelector object.
 * @filename: Filename to set.
 *
 * Description: Sets @filename as the currently selected filename.
 *
 * This calls gnome_selector_check_filename() to make sure @filename
 * is a valid file for this kind of selector.
 *
 * Returns: #TRUE if @filename was ok or #FALSE if not.
 */
gboolean
gnome_selector_set_filename (GnomeSelector *selector,
			     const gchar *filename)
{
    gboolean ok;

    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);

    gnome_selector_freeze (selector);
    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [SET_FILENAME_SIGNAL],
		     filename, &ok);
    gnome_selector_thaw (selector);

    return ok;
}


/**
 * gnome_selector_update_file_list
 * @selector: Pointer to GnomeSelector object.
 *
 * Description: Updates the internal file list.
 *
 * Returns:
 */
void
gnome_selector_update_file_list (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [UPDATE_FILE_LIST_SIGNAL]);
}


/**
 * gnome_selector_freeze
 * @selector: Pointer to GnomeSelector object.
 *
 * Description:
 *
 * Returns:
 */
void
gnome_selector_freeze (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    selector->_priv->frozen++;

    /* Note that we only emit the signal once. */
    if (selector->_priv->frozen == 1)
	gtk_signal_emit (GTK_OBJECT (selector),
			 gnome_selector_signals [FREEZE_SIGNAL]);
}

/**
 * gnome_selector_is_frozen
 * @selector: Pointer to GnomeSelector object.
 *
 * Description:
 *
 * Returns:
 */
gboolean
gnome_selector_is_frozen (GnomeSelector *selector)
{
    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);

    return selector->_priv->frozen ? TRUE : FALSE;
}


/**
 * gnome_selector_thaw
 * @selector: Pointer to GnomeSelector object.
 *
 * Description:
 *
 * Returns:
 */
void
gnome_selector_thaw (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    g_return_if_fail (selector->_priv->frozen > 0);

    selector->_priv->frozen--;

    /* Note that we only emit the signal once. */
    if (selector->_priv->frozen == 0) {
	gtk_signal_emit (GTK_OBJECT (selector),
			 gnome_selector_signals [THAW_SIGNAL]);

	if (selector->_priv->dirty) {
	    selector->_priv->dirty = FALSE;
	    gnome_selector_update (selector);
	}
    }
}


/**
 * gnome_selector_get_entry_text
 * @selector: Pointer to GnomeSelector object.
 *
 * Description:
 *
 * Returns:
 */
gchar *
gnome_selector_get_entry_text (GnomeSelector *selector)
{
    gchar *retval = NULL;

    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [GET_ENTRY_TEXT_SIGNAL],
		     &retval);

    return retval;
}


/**
 * gnome_selector_set_entry_text
 * @selector: Pointer to GnomeSelector object.
 * @text: The text which should be set.
 *
 * Description:
 *
 * Returns:
 */
void
gnome_selector_set_entry_text (GnomeSelector *selector, const gchar *text)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [SET_ENTRY_TEXT_SIGNAL],
		     text);
}

void
gnome_selector_activate_entry (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [ACTIVATE_ENTRY_SIGNAL]);
}

guint
gnome_selector_get_history_length (GnomeSelector *selector)
{
    g_return_val_if_fail (selector != NULL, 0);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), 0);

    return selector->_priv->max_history_length;
}

static void
free_history_item (GnomeSelectorHistoryItem *item, gpointer data)
{
    if (item != NULL)
	g_free (item->text);
    g_free (item);
}

static gint
compare_history_items (gconstpointer a, gconstpointer b)
{
    GnomeSelectorHistoryItem *ia, *ib;

    ia = (GnomeSelectorHistoryItem *) a;
    ib = (GnomeSelectorHistoryItem *) b;

    return strcmp (ia->text, ib->text);
}

static void
set_history_changed (GnomeSelector *selector)
{
    if (selector->_priv->frozen)
	selector->_priv->history_changed = TRUE;
    else
	gtk_signal_emit (GTK_OBJECT (selector),
			 gnome_selector_signals [HISTORY_CHANGED_SIGNAL]);
}

void
gnome_selector_set_history_length (GnomeSelector *selector,
				   guint history_length)
{
    guint old_length;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    /* truncate history if necessary. */
    old_length = g_slist_length (selector->_priv->history);

    if (old_length < history_length) {
	guint prev_pos = (history_length > 0) ? history_length-1 : 0;
	GSList *c = g_slist_nth (selector->_priv->history, prev_pos);

	if (c) {
	    g_slist_foreach (c->next, (GFunc) free_history_item,
			     selector);
	    g_slist_free (c->next);
	    c->next = NULL;

	    set_history_changed (selector);
	}
    }

    selector->_priv->max_history_length = history_length;
}

static void
_gnome_selector_add_history (GnomeSelector *selector, gboolean save,
			     const gchar *text, gboolean append)
{
    GnomeSelectorHistoryItem *item;
    GSList *node;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));
    g_return_if_fail (text != NULL);

    item = g_new0 (GnomeSelectorHistoryItem, 1);
    item->text = g_strdup (text);
    item->save = save;

    /* if it's already in the history */
    node = g_slist_find_custom (selector->_priv->history, item,
				compare_history_items);
    if (node) {
	free_history_item (node->data, selector);
	selector->_priv->history = g_slist_remove
	    (selector->_priv->history, node->data);
    }

    if (append)
	selector->_priv->history = g_slist_append
	    (selector->_priv->history, item);
    else
	selector->_priv->history = g_slist_prepend
	    (selector->_priv->history, item);

    set_history_changed (selector);
}

void
gnome_selector_prepend_history (GnomeSelector *selector, gboolean save,
				const gchar *text)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));
    g_return_if_fail (text != NULL);

    _gnome_selector_add_history (selector, save, text, FALSE);
}

void
gnome_selector_append_history (GnomeSelector *selector, gboolean save,
			       const gchar *text)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));
    g_return_if_fail (text != NULL);

    _gnome_selector_add_history (selector, save, text, TRUE);
}

GSList *
gnome_selector_get_history (GnomeSelector *selector)
{
    GSList *retval = NULL, *c;

    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

    for (c = selector->_priv->history; c; c = c->next) {
	GnomeSelectorHistoryItem *item = c->data;

	retval = g_slist_prepend (retval, g_strdup (item->text));
    }

    return g_slist_reverse (retval);
}

void
gnome_selector_set_history (GnomeSelector *selector, GSList *history)
{
    GSList *c;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    gnome_selector_clear_history (selector);

    for (c = history; c; c = c->next)
	gnome_selector_prepend_history (selector, TRUE, c->data);

    selector->_priv->history = g_slist_reverse (selector->_priv->history);

    set_history_changed (selector);
}

static GSList *
_gnome_selector_history_to_list (GnomeSelector *selector, gboolean only_save)
{
    GSList *thelist = NULL, *c;

    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

    for (c = selector->_priv->history; c; c = c->next) {
	GnomeSelectorHistoryItem *item = c->data;

	if (only_save && !item->save)
	    continue;

	thelist = g_slist_prepend (thelist, item->text);
    }

    thelist = g_slist_reverse (thelist);
    return thelist;
}

void
gnome_selector_load_history (GnomeSelector *selector)
{
    GSList *thelist;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    if (!selector->_priv->gconf_history_key)
	return;

    thelist = gconf_client_get_list (selector->_priv->client,
				     selector->_priv->gconf_history_key,
				     GCONF_VALUE_STRING, NULL);

    gnome_selector_set_history (selector, thelist);

    g_slist_foreach (thelist, (GFunc) g_free, NULL);
    g_slist_free (thelist);
}

void
gnome_selector_save_history (GnomeSelector *selector)
{
    GSList *thelist;
    gboolean result;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    if (!selector->_priv->gconf_history_key)
	return;

    thelist = _gnome_selector_history_to_list (selector, TRUE);

    result = gconf_client_set_list (selector->_priv->client,
				    selector->_priv->gconf_history_key,
				    GCONF_VALUE_STRING, thelist, NULL);

    g_slist_free (thelist);
}

void
gnome_selector_clear_history (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    g_slist_foreach (selector->_priv->history,
		     (GFunc) free_history_item, selector);
    g_slist_free (selector->_priv->history);
    selector->_priv->history = NULL;

    set_history_changed (selector);
}

static GSList *
_gnome_selector_copy_list (GSList *thelist)
{
    GSList *retval = NULL, *c;

    for (c = thelist; c; c = c->next)
	retval = g_slist_prepend (retval, g_strdup (c->data));

    retval = g_slist_reverse (retval);
    return retval;
}

static void
_gnome_selector_free_list (GSList *thelist)
{
    g_slist_foreach (thelist, (GFunc) g_free, NULL);
    g_slist_free (thelist);
}

void
gnome_selector_set_to_defaults (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    _gnome_selector_free_list (selector->_priv->dir_list);
    selector->_priv->dir_list = _gnome_selector_copy_list
	(selector->_priv->default_dir_list);

    _gnome_selector_free_list (selector->_priv->file_list);
    selector->_priv->file_list = _gnome_selector_copy_list
	(selector->_priv->default_file_list);

    _gnome_selector_free_list (selector->_priv->total_list);
    selector->_priv->total_list = _gnome_selector_copy_list
	(selector->_priv->default_total_list);

    selector->_priv->need_rebuild = TRUE;

    if (selector->_priv->frozen)
	selector->_priv->dirty = TRUE;
    else
	gnome_selector_update (selector);
}

void
_gnome_selector_save_all (GnomeSelector *selector)
{
    gboolean result;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    if (!selector->_priv->gconf_dir_list_key ||
	!selector->_priv->gconf_file_list_key)
	return;

    result = gconf_client_set_list (selector->_priv->client,
				    selector->_priv->gconf_dir_list_key,
				    GCONF_VALUE_STRING,
				    selector->_priv->dir_list,
				    NULL);

    result = gconf_client_set_list (selector->_priv->client,
				    selector->_priv->gconf_file_list_key,
				    GCONF_VALUE_STRING,
				    selector->_priv->file_list,
				    NULL);
}

void
_gnome_selector_load_all (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    if (!selector->_priv->gconf_dir_list_key ||
	!selector->_priv->gconf_file_list_key)
	return;

    selector->_priv->dir_list = gconf_client_get_list
	(selector->_priv->client,
	 selector->_priv->gconf_dir_list_key,
	 GCONF_VALUE_STRING, NULL);

    selector->_priv->file_list = gconf_client_get_list
	(selector->_priv->client,
	 selector->_priv->gconf_file_list_key,
	 GCONF_VALUE_STRING, NULL);

    selector->_priv->need_rebuild = TRUE;

    if (selector->_priv->frozen)
	selector->_priv->dirty = TRUE;
    else
	gnome_selector_update (selector);
}
