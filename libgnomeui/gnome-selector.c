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
#include <gtk/gtkentry.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtklist.h>
#include <gtk/gtklistitem.h>
#include <gtk/gtksignal.h>
#include "libgnome/libgnomeP.h"
#include "gnome-selectorP.h"
#include "gnome-uidefs.h"


static void gnome_selector_class_init    (GnomeSelectorClass *class);
static void gnome_selector_init          (GnomeSelector      *selector);
static void gnome_selector_destroy       (GtkObject          *object);
static void gnome_selector_finalize      (GObject            *object);

static void selector_set_arg             (GtkObject          *object,
				          GtkArg             *arg,
				          guint               arg_id);
static void selector_get_arg             (GtkObject          *object,
				          GtkArg             *arg,
				          guint               arg_id);

static void     update_handler           (GnomeSelector   *selector);
static void     browse_handler           (GnomeSelector   *selector);
static void     clear_handler            (GnomeSelector   *selector);

static gboolean set_filename_handler     (GnomeSelector   *selector,
                                          const gchar     *filename);
static gboolean check_filename_handler   (GnomeSelector   *selector,
                                          const gchar     *filename);
static void     add_file_handler         (GnomeSelector   *selector,
                                          const gchar     *filename);
static gboolean check_directory_handler  (GnomeSelector   *selector,
                                          const gchar     *filename);
static void     add_directory_handler    (GnomeSelector   *selector,
                                          const gchar     *directory);
static void     update_file_list_handler (GnomeSelector   *selector);

static void     free_entry_func          (gpointer         data,
					  gpointer         user_data);


static GtkVBoxClass *parent_class;

enum {
	ARG_0,
};

enum {
	BROWSE_SIGNAL,
	CLEAR_SIGNAL,
	CHECK_FILENAME_SIGNAL,
	SET_FILENAME_SIGNAL,
	ADD_FILE_SIGNAL,
	CHECK_DIRECTORY_SIGNAL,
	ADD_DIRECTORY_SIGNAL,
	FREEZE_SIGNAL,
	UPDATE_SIGNAL,
	UPDATE_FILE_LIST_SIGNAL,
	THAW_SIGNAL,
	SET_SELECTION_MODE_SIGNAL,
	GET_SELECTION_SIGNAL,
	LAST_SIGNAL
};

static int gnome_selector_signals [LAST_SIGNAL] = {0};

guint
gnome_selector_get_type (void)
{
	static guint selector_type = 0;

	if (!selector_type) {
		GtkTypeInfo selector_info = {
			"GnomeSelector",
			sizeof (GnomeSelector),
			sizeof (GnomeSelectorClass),
			(GtkClassInitFunc) gnome_selector_class_init,
			(GtkObjectInitFunc) gnome_selector_init,
			NULL,
			NULL,
			NULL
		};

		selector_type = gtk_type_unique (gtk_vbox_get_type (),
						 &selector_info);
	}

	return selector_type;
}

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

	parent_class = gtk_type_class (gtk_vbox_get_type ());

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
				gtk_marshal_NONE__POINTER,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_STRING);
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
				gtk_marshal_NONE__POINTER,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_STRING);
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
	gtk_object_class_add_signals (object_class,
				      gnome_selector_signals,
				      LAST_SIGNAL);

	object_class->destroy = gnome_selector_destroy;
	gobject_class->finalize = gnome_selector_finalize;
	object_class->get_arg = selector_get_arg;
	object_class->set_arg = selector_set_arg;

	class->browse = browse_handler;
	class->clear = clear_handler;
	class->update = update_handler;

	class->check_filename = check_filename_handler;
	class->set_filename = set_filename_handler;
	class->add_file = add_file_handler;
	class->check_directory = check_directory_handler;
	class->add_directory = add_directory_handler;
	class->update_file_list = update_file_list_handler;
}

static void
selector_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeSelector *self;

	self = GNOME_SELECTOR (object);

	switch (arg_id) {
	default:
		break;
	}
}

static void
selector_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeSelector *self;

	self = GNOME_SELECTOR (object);

	switch (arg_id) {
	default:
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

	for (c = selector->_priv->dir_list; c; c = c->next)
		gtk_signal_emit (GTK_OBJECT (selector),
				 gnome_selector_signals [ADD_DIRECTORY_SIGNAL],
				 c->data);

	for (c = selector->_priv->file_list; c; c = c->next)
		gtk_signal_emit (GTK_OBJECT (selector),
				 gnome_selector_signals [ADD_FILE_SIGNAL],
				 c->data);

	selector->_priv->total_list = g_slist_reverse
		(selector->_priv->total_list);

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

static gboolean
check_filename_handler (GnomeSelector *selector, const gchar *filename)
{
	g_return_val_if_fail (selector != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);

	g_message (G_STRLOC ": '%s'", filename);

	return g_file_exists (filename);
}

static gboolean
set_filename_handler (GnomeSelector *selector, const gchar *filename)
{
	g_return_val_if_fail (selector != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);

	g_message (G_STRLOC ": '%s'", filename);

	if (!gnome_selector_check_filename (selector, filename))
		return FALSE;

	gtk_entry_set_text (GTK_ENTRY (selector->_priv->entry), filename);
	gtk_signal_emit_by_name (GTK_OBJECT (selector->_priv->entry),
				 "activate");

	return TRUE;
}

static gboolean
check_directory_handler (GnomeSelector *selector, const gchar *directory)
{
	g_return_val_if_fail (selector != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);
	g_return_val_if_fail (directory != NULL, FALSE);

	g_message (G_STRLOC ": '%s'", directory);

	return g_file_test (directory, G_FILE_TEST_ISDIR);
}

void
add_file_handler (GnomeSelector *selector, const gchar *filename)
{
	g_return_if_fail (selector != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (selector));
	g_return_if_fail (filename != NULL);

	if (!g_file_test (filename, G_FILE_TEST_ISFILE)) {
		g_warning ("GnomeSelector: '%s' is not a regular file",
			   filename);
		return;
	}

	selector->_priv->total_list = g_slist_prepend
		(selector->_priv->total_list, g_strdup (filename));
}

void
add_directory_handler (GnomeSelector *selector, const gchar *directory)
{
	struct dirent *de;
	DIR *dp;

	g_return_if_fail (selector != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (selector));
	g_return_if_fail (directory != NULL);

	g_message (G_STRLOC ": '%s'", directory);

	if (!g_file_test (directory, G_FILE_TEST_ISDIR)) {
		g_warning ("GnomeSelector: '%s' is not a directory",
			   directory);
		return;
	}

	dp = opendir (directory);

	if (dp == NULL) {
		g_warning ("GnomeSelector: couldn't open directory '%s'",
			   directory);
		return;
	}

	while ((de = readdir (dp)) != NULL) {
		gchar *full_path = g_concat_dir_and_file
			(directory, de->d_name);

		/* skip dotfiles */
		if (*(de->d_name) == '.') {
		    g_free (full_path);
		    continue;
		}

		if (!g_file_test (full_path, G_FILE_TEST_ISFILE)) {
		    g_free (full_path);
		    continue;
		}

		if (!gnome_selector_check_filename (selector, full_path)) {
		    g_free (full_path);
		    continue;
		}

		gtk_signal_emit (GTK_OBJECT (selector),
				 gnome_selector_signals [ADD_FILE_SIGNAL],
				 full_path);

		g_free (full_path);
	}

	closedir (dp);
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
clear_clicked_cb (GtkWidget *widget, gpointer data)
{
	gnome_selector_clear (GNOME_SELECTOR (data));
}

static void
entry_activated (GtkWidget *widget, gpointer data)
{
        GnomeSelector *selector;
	gchar *text;

        selector = GNOME_SELECTOR (data);
	text = gtk_entry_get_text (GTK_ENTRY (selector->_priv->entry));

	g_message (G_STRLOC ": '%s'", text);

	gnome_selector_freeze (selector);

	if (g_file_test (text, G_FILE_TEST_ISFILE)) {
	    gnome_selector_append_file (selector, text);
	} else if (g_file_test (text, G_FILE_TEST_ISDIR)) {
	    gnome_selector_append_directory (selector, text);
	}

	gnome_selector_thaw (selector);
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
			  GtkWidget *selector_widget,
			  GtkWidget *browse_dialog,
			  guint32 flags)
{
	GnomeSelectorPrivate *priv;

	g_return_if_fail (selector != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (selector));

	priv = selector->_priv;

	priv->gentry = gnome_entry_new (history_id);
	priv->entry = gnome_entry_gtk_entry (GNOME_ENTRY (priv->gentry));

	priv->dialog_title = g_strdup (dialog_title);

	priv->flags = flags;

	priv->selector_widget = selector_widget;
	if (priv->selector_widget)
		gtk_widget_ref (priv->selector_widget);

	priv->browse_dialog = browse_dialog;
	if (priv->browse_dialog)
		gtk_widget_ref (priv->browse_dialog);

	priv->box = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);

	priv->hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);

	gtk_box_pack_start (GTK_BOX (priv->hbox), priv->gentry,
			    TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (priv->box), priv->hbox,
			    TRUE, FALSE, GNOME_PAD_SMALL);

	if (flags & GNOME_SELECTOR_WANT_BROWSE_BUTTON) {
		if (priv->browse_dialog == NULL) {
			g_warning (G_STRLOC ": It makes no sense to use "
				   "GNOME_SELECTOR_WANT_BROWSE_BUTTON "
				   "without having a `browse_dialog'.");
			return;
		}

		priv->browse_button = gtk_button_new_with_label
			(_("Browse..."));

		gtk_signal_connect (GTK_OBJECT (priv->browse_button),
				    "clicked", browse_clicked_cb, selector);

		gtk_box_pack_start (GTK_BOX (priv->hbox),
				    priv->browse_button, FALSE, FALSE, 0);
	}

	if (flags & GNOME_SELECTOR_WANT_CLEAR_BUTTON) {
		priv->clear_button = gtk_button_new_with_label
			(_("Clear..."));

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

	gtk_signal_connect (GTK_OBJECT (priv->entry), "activate",
			    entry_activated, selector);
}


static void
gnome_selector_destroy (GtkObject *object)
{
	GnomeSelector *selector;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (object));

	selector = GNOME_SELECTOR (object);

	if (selector->_priv->selector_widget) {
		gtk_widget_unref (selector->_priv->selector_widget);
		selector->_priv->selector_widget = NULL;
	}

	if (selector->_priv->browse_dialog) {
		gtk_widget_unref (selector->_priv->browse_dialog);
		selector->_priv->browse_dialog = NULL;
	}

	if (selector->_priv->gentry) {
	    gtk_widget_destroy (selector->_priv->gentry);
	    selector->_priv->gentry = NULL;
	    selector->_priv->entry = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
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
	}

	g_free (selector->_priv);
	selector->_priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

/**
 * gnome_selector_get_gnome_entry
 * @selector: Pointer to GnomeSelector object.
 *
 * Description: Obtain pointer to GnomeSelector's GnomeEntry widget
 *
 * Returns: Pointer to GnomeEntry widget.
 */
GtkWidget *
gnome_selector_get_gnome_entry (GnomeSelector *selector)
{
	g_return_val_if_fail (selector != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

	return selector->_priv->gentry;
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
				  const gchar *directory)
{
	g_return_val_if_fail (selector != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);
	g_return_val_if_fail (directory != NULL, FALSE);

	if (!gnome_selector_check_directory (selector, directory))
		return FALSE;

	selector->_priv->dir_list = g_slist_prepend
		(selector->_priv->dir_list, g_strdup (directory));

	selector->_priv->need_rebuild = TRUE;

	if (selector->_priv->frozen)
		selector->_priv->dirty = TRUE;
	else
		gnome_selector_update (selector);

	return TRUE;
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
				 const gchar *directory)
{
	g_return_val_if_fail (selector != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);
	g_return_val_if_fail (directory != NULL, FALSE);

	if (!gnome_selector_check_directory (selector, directory))
		return FALSE;

	selector->_priv->dir_list = g_slist_append
		(selector->_priv->dir_list, g_strdup (directory));

	selector->_priv->need_rebuild = TRUE;

	if (selector->_priv->frozen)
		selector->_priv->dirty = TRUE;
	else
		gnome_selector_update (selector);

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
			     const gchar *filename)
{
	g_return_val_if_fail (selector != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);

	if (!gnome_selector_check_filename (selector, filename))
		return FALSE;

	selector->_priv->file_list = g_slist_prepend
		(selector->_priv->file_list, g_strdup (filename));

	selector->_priv->need_rebuild = TRUE;

	if (selector->_priv->frozen)
		selector->_priv->dirty = TRUE;
	else
		gnome_selector_update (selector);

	return TRUE;
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
			    const gchar *filename)
{
	g_return_val_if_fail (selector != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);

	if (!gnome_selector_check_filename (selector, filename))
		return FALSE;

	selector->_priv->file_list = g_slist_append
		(selector->_priv->file_list, g_strdup (filename));

	selector->_priv->need_rebuild = TRUE;

	if (selector->_priv->frozen)
		selector->_priv->dirty = TRUE;
	else
		gnome_selector_update (selector);

	return TRUE;
}

GSList *
gnome_selector_get_file_list (GnomeSelector *selector,
			      gboolean include_dir_list)
{
	g_return_val_if_fail (selector != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

	if (selector->_priv->need_rebuild)
		gnome_selector_update_file_list (selector);

	if (include_dir_list)
		return selector->_priv->total_list;
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
gnome_selector_clear (GnomeSelector *selector)
{
	g_return_if_fail (selector != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (selector));

	gnome_selector_freeze (selector);
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
	g_return_val_if_fail (filename != NULL, FALSE);

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
