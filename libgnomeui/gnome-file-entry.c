/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 1998, 1999, 2000 Free Software Foundation
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

/* GnomeFileEntry widget - Combo box with "Browse" button for files
 *
 *
 * Authors: Federico Mena <federico@nuclecu.unam.mx>
 */
#include <config.h>
#include "gnome-macros.h"

/* Must be before all other gnome includes!! */
#include "gnome-i18nP.h"

#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include "libgnome/gnome-util.h"

#include "libgnomeuiP.h"

#include "gnome-file-entry.h"

#include <libgnomevfs/gnome-vfs-mime.h>

struct _GnomeFileEntryPrivate {
	GtkWidget *gentry;

	char *browse_dialog_title;
	char *default_path;

	gboolean is_modal : 1;

	gboolean directory_entry : 1; /*optional flag to only do directories*/

	/* FIXME: Non local files!! */
	/* FIXME: executable_entry as used in gnome_run */
	/* FIXME: multiple_entry for entering multiple filenames */
};



static void gnome_file_entry_class_init (GnomeFileEntryClass *class);
static void gnome_file_entry_init       (GnomeFileEntry      *fentry);
static void gnome_file_entry_destroy    (GtkObject           *object);
static void gnome_file_entry_finalize   (GObject             *object);
static void gnome_file_entry_drag_data_received (GtkWidget        *widget,
						 GdkDragContext   *context,
						 gint              x,
						 gint              y,
						 GtkSelectionData *data,
						 guint             info,
						 guint32           time);
static void browse_clicked(GnomeFileEntry *fentry);
static void fentry_set_arg                (GtkObject *object,
					   GtkArg *arg,
					   guint arg_id);
static void fentry_get_arg                (GtkObject *object,
					   GtkArg *arg,
					   guint arg_id);
enum {
	ARG_0,
	ARG_HISTORY_ID,
	ARG_BROWSE_DIALOG_TITLE,
	ARG_DIRECTORY_ENTRY,
	ARG_MODAL,
	ARG_FILENAME,
	ARG_DEFAULT_PATH,
	ARG_GNOME_ENTRY,
	ARG_GTK_ENTRY
};

GNOME_CLASS_BOILERPLATE (GnomeFileEntry, gnome_file_entry,
			 GtkVBox, gtk_vbox)

enum {
	BROWSE_CLICKED_SIGNAL,
	LAST_SIGNAL
};

static int gnome_file_entry_signals[LAST_SIGNAL] = {0};

static void
gnome_file_entry_class_init (GnomeFileEntryClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;

	gnome_file_entry_signals[BROWSE_CLICKED_SIGNAL] =
		gtk_signal_new("browse_clicked",
			       GTK_RUN_LAST,
			       GTK_CLASS_TYPE (object_class),
			       GTK_SIGNAL_OFFSET(GnomeFileEntryClass,
			       			 browse_clicked),
			       gtk_signal_default_marshaller,
			       GTK_TYPE_NONE,
			       0);

	gtk_object_add_arg_type("GnomeFileEntry::history_id",
				GTK_TYPE_STRING,
				GTK_ARG_WRITABLE,
				ARG_HISTORY_ID);
	gtk_object_add_arg_type("GnomeFileEntry::browse_dialog_title",
				GTK_TYPE_STRING,
				GTK_ARG_WRITABLE,
				ARG_BROWSE_DIALOG_TITLE);
	gtk_object_add_arg_type("GnomeFileEntry::directory_entry",
				GTK_TYPE_BOOL,
				GTK_ARG_READWRITE,
				ARG_DIRECTORY_ENTRY);
	gtk_object_add_arg_type("GnomeFileEntry::modal",
				GTK_TYPE_BOOL,
				GTK_ARG_READWRITE,
				ARG_MODAL);
	gtk_object_add_arg_type("GnomeFileEntry::filename",
				GTK_TYPE_STRING,
				GTK_ARG_READWRITE,
				ARG_FILENAME);
	gtk_object_add_arg_type("GnomeFileEntry::default_path",
				GTK_TYPE_STRING,
				GTK_ARG_WRITABLE,
				ARG_FILENAME);
	gtk_object_add_arg_type("GnomeFileEntry::gnome_entry",
				GTK_TYPE_POINTER,
				GTK_ARG_READABLE,
				ARG_GNOME_ENTRY);
	gtk_object_add_arg_type("GnomeFileEntry::gtk_entry",
				GTK_TYPE_POINTER,
				GTK_ARG_READABLE,
				ARG_GTK_ENTRY);

	object_class->destroy = gnome_file_entry_destroy;
	gobject_class->finalize = gnome_file_entry_finalize;
	object_class->get_arg = fentry_get_arg;
	object_class->set_arg = fentry_set_arg;

	class->browse_clicked = browse_clicked;
}

static void
fentry_set_arg (GtkObject *object,
		GtkArg *arg,
		guint arg_id)
{
	GnomeFileEntry *self;

	self = GNOME_FILE_ENTRY (object);

	switch (arg_id) {
	case ARG_HISTORY_ID: {
		GtkWidget *gentry;
		gentry = gnome_file_entry_gnome_entry(self);
		gnome_entry_set_history_id (GNOME_ENTRY(gentry),
					    GTK_VALUE_POINTER(*arg));
		gnome_entry_load_history (GNOME_ENTRY(gentry));
		break;
	}
	case ARG_DIRECTORY_ENTRY:
		gnome_file_entry_set_directory_entry (self, GTK_VALUE_BOOL(*arg));
		break;
	case ARG_MODAL:
		gnome_file_entry_set_modal (self, GTK_VALUE_BOOL(*arg));
		break;
	case ARG_FILENAME:
		gnome_file_entry_set_filename (self, GTK_VALUE_STRING(*arg));
		break;
	case ARG_DEFAULT_PATH:
		gnome_file_entry_set_default_path (self, GTK_VALUE_STRING(*arg));
		break;
	case ARG_BROWSE_DIALOG_TITLE:
		gnome_file_entry_set_title (self, GTK_VALUE_STRING(*arg));
		break;
	default:
		break;
	}
}

static void
fentry_get_arg (GtkObject *object,
		GtkArg *arg,
		guint arg_id)
{
	GnomeFileEntry *self;

	self = GNOME_FILE_ENTRY (object);

	switch (arg_id) {
	case ARG_GNOME_ENTRY:
		GTK_VALUE_POINTER(*arg) =
			gnome_file_entry_gnome_entry(self);
		break;
	case ARG_DIRECTORY_ENTRY:
		GTK_VALUE_BOOL(*arg) = self->_priv->directory_entry;
		break;
	case ARG_MODAL:
		GTK_VALUE_BOOL(*arg) = self->_priv->is_modal;
		break;
	case ARG_FILENAME:
		GTK_VALUE_STRING(*arg) =
			gnome_file_entry_get_full_path (self, FALSE);
		break;
	case ARG_GTK_ENTRY:
		GTK_VALUE_POINTER(*arg) =
			gnome_file_entry_gtk_entry(self);
		break;
	default:
		break;
	}
}

static void
browse_dialog_ok (GtkWidget *widget, gpointer data)
{
	GtkFileSelection *fs;
	GnomeFileEntry *fentry;
	GtkWidget *entry;

	fs = GTK_FILE_SELECTION (data);
	fentry = GNOME_FILE_ENTRY (gtk_object_get_user_data (GTK_OBJECT (fs)));
	entry = gnome_file_entry_gtk_entry (fentry);

	gtk_entry_set_text (GTK_ENTRY (entry),
			    gtk_file_selection_get_filename (fs));
	/* Is this evil? */
	gtk_signal_emit_by_name (GTK_OBJECT (entry), "activate");
	gtk_widget_destroy (GTK_WIDGET (fs));
}

static void
browse_dialog_kill (GtkWidget *widget, gpointer data)
{
	GnomeFileEntry *fentry;
	fentry = GNOME_FILE_ENTRY (data);

	fentry->fsw = NULL;
}

static void
browse_clicked(GnomeFileEntry *fentry)
{
	GtkWidget *fsw;
	GtkFileSelection *fs;
	GtkWidget *parent;
	const char *p;

	/*if it already exists make sure it's shown and raised*/
	if(fentry->fsw) {
		gtk_widget_show(fentry->fsw);
		if(fentry->fsw->window)
			gdk_window_raise(fentry->fsw->window);
		fs = GTK_FILE_SELECTION(fentry->fsw);
		gtk_widget_set_sensitive(fs->file_list,
					 ! fentry->_priv->directory_entry);
		p = gtk_entry_get_text (GTK_ENTRY (gnome_file_entry_gtk_entry (fentry)));
		if(p && *p!=G_DIR_SEPARATOR && fentry->_priv->default_path) {
			char *dp = g_concat_dir_and_file (fentry->_priv->default_path, p);
			gtk_file_selection_set_filename (fs, dp);
			g_free(dp);
		} else {
			gtk_file_selection_set_filename (fs, p);
		}
		return;
	}


	fsw = gtk_file_selection_new (fentry->_priv->browse_dialog_title
				      ? fentry->_priv->browse_dialog_title
				      : _("Select file"));
	/* BEGIN UGLINESS.  This code is stolen from gnome_dialog_set_parent.
	 * We want its functionality, but it takes a GnomeDialog as its argument.
	 * So we copy it )-: */
	parent = gtk_widget_get_toplevel (GTK_WIDGET (fentry));
	gtk_window_set_transient_for (GTK_WINDOW(fsw), GTK_WINDOW (parent));

	if ( gnome_preferences_get_dialog_centered() ) {

		/* User wants us to center over parent */

		gint x, y, w, h, dialog_x, dialog_y;

		if ( ! GTK_WIDGET_VISIBLE(parent)) return; /* Can't get its
							      size/pos */

		/* Throw out other positioning */
		gtk_window_set_position(GTK_WINDOW(fsw),GTK_WIN_POS_NONE);

		gdk_window_get_origin (GTK_WIDGET(parent)->window, &x, &y);
		gdk_window_get_size   (GTK_WIDGET(parent)->window, &w, &h);

		/* The problem here is we don't know how big the dialog is.
		   So "centered" isn't really true. We'll go with
		   "kind of more or less on top" */

		dialog_x = x + w/4;
		dialog_y = y + h/4;

		gtk_widget_set_uposition(GTK_WIDGET(fsw), dialog_x, dialog_y);
	}

	gtk_object_set_user_data (GTK_OBJECT (fsw), fentry);

	fs = GTK_FILE_SELECTION (fsw);
	gtk_widget_set_sensitive(fs->file_list,
				 ! fentry->_priv->directory_entry);

	p = gtk_entry_get_text (GTK_ENTRY (gnome_file_entry_gtk_entry (fentry)));
	if(p && *p!=G_DIR_SEPARATOR && fentry->_priv->default_path) {
		char *dp = g_concat_dir_and_file (fentry->_priv->default_path, p);
		gtk_file_selection_set_filename (fs, dp);
		g_free(dp);
	} else {
		gtk_file_selection_set_filename (fs, p);
	}

	gtk_signal_connect (GTK_OBJECT (fs->ok_button), "clicked",
			    (GtkSignalFunc) browse_dialog_ok,
			    fs);
	gtk_signal_connect_object (GTK_OBJECT (fs->cancel_button), "clicked",
				   GTK_SIGNAL_FUNC(gtk_widget_destroy),
				   GTK_OBJECT(fsw));
	gtk_signal_connect (GTK_OBJECT (fsw), "destroy",
			    GTK_SIGNAL_FUNC(browse_dialog_kill),
			    fentry);

	if (gtk_grab_get_current ())
		gtk_grab_add (fsw);

	gtk_widget_show (fsw);

	if(fentry->_priv->is_modal)
		gtk_window_set_modal (GTK_WINDOW (fsw), TRUE);
	fentry->fsw = fsw;
}

static void
browse_clicked_signal(GtkWidget *widget, gpointer data)
{
	gtk_signal_emit(GTK_OBJECT(data),
			gnome_file_entry_signals[BROWSE_CLICKED_SIGNAL]);
}

static void
gnome_file_entry_drag_data_received (GtkWidget        *widget,
				     GdkDragContext   *context,
				     gint              x,
				     gint              y,
				     GtkSelectionData *selection_data,
				     guint             info,
				     guint32           time)
{
	GList *uris, *li;
	GnomeVFSURI *uri = NULL;

	/*here we extract the filenames from the URI-list we recieved*/
	uris = gnome_vfs_uri_list_parse (selection_data->data);

	/* FIXME: Support multiple files */
	/* FIXME: Support executable entries (smack files after others) */
	for (li = uris; li != NULL; li = li->next) {
		uri = li->data;
		/* FIXME: Support non-local files */
		if (gnome_vfs_uri_is_local (uri)) {
			break;
		}
		uri = NULL;
	}

	/*if there's isn't a file*/
	if (uri == NULL) {
		gtk_drag_finish (context, FALSE, FALSE, time);
		/*free the list of files we got*/
		gnome_vfs_uri_list_free (uris);
		return;
	}

	gtk_entry_set_text (GTK_ENTRY(widget),
			    gnome_vfs_uri_get_path (uri));

	/*free the list of files we got*/
	gnome_vfs_uri_list_free (uris);
}

#define ELEMENTS(x) (sizeof (x) / sizeof (x[0]))

static void
gnome_file_entry_init (GnomeFileEntry *fentry)
{
	GtkWidget *button, *the_gtk_entry;
	static GtkTargetEntry drop_types[] = { { "text/uri-list", 0, 0 } };
	GtkWidget *hbox;

	gtk_box_set_spacing (GTK_BOX (fentry), 4);

	/* Allow for a preview thingie to be smacked on top of
	 * the file entry */
	hbox = gtk_hbox_new (FALSE, 4);
	gtk_widget_show (hbox);
	gtk_box_pack_end (GTK_BOX (fentry), hbox, FALSE, FALSE, 0);

	fentry->_priv = g_new0(GnomeFileEntryPrivate, 1);

	fentry->_priv->browse_dialog_title = NULL;
	fentry->_priv->default_path = NULL;
	fentry->_priv->is_modal = FALSE;
	fentry->_priv->directory_entry = FALSE;

	fentry->_priv->gentry = gnome_entry_new (NULL);
	the_gtk_entry = gnome_file_entry_gtk_entry (fentry);

	gtk_drag_dest_set (GTK_WIDGET (the_gtk_entry),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drop_types, ELEMENTS(drop_types), GDK_ACTION_COPY);

	gtk_signal_connect (GTK_OBJECT (the_gtk_entry), "drag_data_received",
			    GTK_SIGNAL_FUNC (gnome_file_entry_drag_data_received),
			    NULL);

	gtk_box_pack_start (GTK_BOX (hbox), fentry->_priv->gentry, TRUE, TRUE, 0);
	gtk_widget_show (fentry->_priv->gentry);

	button = gtk_button_new_with_label (_("Browse..."));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    (GtkSignalFunc) browse_clicked_signal,
			    fentry);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);
}

static void
gnome_file_entry_destroy (GtkObject *object)
{
	GnomeFileEntry *fentry;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (object));

	fentry = GNOME_FILE_ENTRY (object);

	g_free (fentry->_priv->browse_dialog_title);
	fentry->_priv->browse_dialog_title = NULL;

	g_free (fentry->_priv->default_path);
	fentry->_priv->default_path = NULL;

	if (fentry->fsw != NULL)
		gtk_widget_destroy(fentry->fsw);
	fentry->fsw = NULL;

	GNOME_CALL_PARENT_HANDLER (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gnome_file_entry_finalize (GObject *object)
{
	GnomeFileEntry *fentry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (object));

	fentry = GNOME_FILE_ENTRY (object);

	g_free(fentry->_priv);
	fentry->_priv = NULL;

	GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, finalize, (object));
}

/**
 * gnome_file_entry_construct:
 * @fentry: A #GnomeFileEntry to construct.
 * @history_id: the id given to #gnome_entry_new (see #GnomeEntry).
 * @browse_dialog_title: Title for the file dialog window.
 *
 * Description: Constructs a #GnomeFileEntry
 */
void
gnome_file_entry_construct (GnomeFileEntry *fentry, const char *history_id, const char *browse_dialog_title)
{
	g_return_if_fail (fentry != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (fentry));

	gnome_entry_construct (GNOME_ENTRY (fentry->_priv->gentry),
			       history_id);

	gnome_file_entry_set_title (fentry, browse_dialog_title);
}

/**
 * gnome_file_entry_new:
 * @history_id: the id given to #gnome_entry_new (see #GnomeEntry).
 * @browse_dialog_title: Title for the file dialog window.
 *
 * Description: Creates a new #GnomeFileEntry widget.
 *
 * Returns: A pointer to the widget, NULL if it cannot be created.
 **/
GtkWidget *
gnome_file_entry_new (const char *history_id, const char *browse_dialog_title)
{
	GnomeFileEntry *fentry;

	fentry = gtk_type_new (gnome_file_entry_get_type ());

	gnome_file_entry_construct (fentry, history_id, browse_dialog_title);
	return GTK_WIDGET (fentry);
}

/**
 * gnome_file_entry_gnome_entry:
 * @fentry: The GnomeFileEntry widget to work with.
 *
 * Description: It returns a pointer to the gnome entry widget of the
 * widget (see#GnomeEntry).
 *
 * Returns: A pointer to the component #GnomeEntry widget
 **/
GtkWidget *
gnome_file_entry_gnome_entry (GnomeFileEntry *fentry)
{
	g_return_val_if_fail (fentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_FILE_ENTRY (fentry), NULL);

	return fentry->_priv->gentry;
}

/**
 * gnome_file_entry_gtk_entry:
 * @fentry: The GnomeFileEntry widget to work with.
 *
 * Description: Similar to #gnome_file_entry_gnome_entry but
 * returns the gtk entry instead of the Gnome entry widget.
 *
 * Returns: Returns the GtkEntry widget
 **/
GtkWidget *
gnome_file_entry_gtk_entry (GnomeFileEntry *fentry)
{
	g_return_val_if_fail (fentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_FILE_ENTRY (fentry), NULL);

	return gnome_entry_gtk_entry (GNOME_ENTRY (fentry->_priv->gentry));
}

/**
 * gnome_file_entry_set_title:
 * @fentry: The GnomeFileEntry widget to work with.
 * @browse_dialog_title: The new title for the file browse dialog window.
 *
 * Description: Set the title of the browse dialog to @browse_dialog_title.
 * The new title will go into effect the next time the browse button is pressed.
 *
 * Returns: @fentry (the widget itself)
 **/
void
gnome_file_entry_set_title (GnomeFileEntry *fentry, const char *browse_dialog_title)
{
	g_return_if_fail (fentry != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (fentry));

	if (fentry->_priv->browse_dialog_title)
		g_free (fentry->_priv->browse_dialog_title);

	fentry->_priv->browse_dialog_title = g_strdup (browse_dialog_title); /* handles NULL correctly */
}

/**
 * gnome_file_entry_set_default_path:
 * @fentry: The GnomeFileEntry widget to work with.
 * @path: A path string.
 *
 * Description: Set the default path of browse dialog to @path. The
 * default path is only used if the entry is empty or if the current path
 * of the entry is not an absolute path, in which case the default
 * path is prepended to it before the dialog is started.
 *
 * Returns:
 **/
void
gnome_file_entry_set_default_path(GnomeFileEntry *fentry, const char *path)
{
	char rpath[MAXPATHLEN+1];
	char *p;
	g_return_if_fail (fentry != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (fentry));

	if(path && realpath(path, rpath))
		p = g_strdup(rpath);
	else
		p = NULL;

	/*handles NULL as well*/
	g_free(fentry->_priv->default_path);
	fentry->_priv->default_path = p;
}

/* Does tilde (home directory) expansion on a string */
static char *
tilde_expand (char *str)
{
	struct passwd *passwd;
	char *p;
	char *name;

	g_assert (str != NULL);

	if (*str != '~')
		return g_strdup (str);

	str++;

	p = strchr (str, G_DIR_SEPARATOR);

	/* d = "~" or d = "~/" */
	if (!*str || *str == G_DIR_SEPARATOR) {
		passwd = getpwuid (geteuid ());
		p = (*str == G_DIR_SEPARATOR) ? str + 1 : "";
	} else {
		if (!p) {
			p = "";
			passwd = getpwnam (str);
		} else {
			name = g_strndup (str, p - str);
			passwd = getpwnam (name);
			g_free (name);
		}
	}

	/* If we can't figure out the user name, bail out */
	if (!passwd)
		return NULL;

	return g_strconcat (passwd->pw_dir, G_DIR_SEPARATOR_S, p, NULL);
}

/**
 * gnome_file_entry_get_full_path:
 * @fentry: The GnomeFileEntry widget to work with.
 * @file_must_exist: boolean
 *
 * Description: Gets the full absolute path of the file from the entry.
 * If @file_must_exist is false, nothing is tested and the path is returned.
 * If @file_must_exist is true, then the path is only returned if the path
 * actually exists. In case the entry is a directory entry (see
 * #gnome_file_entry_set_directory_entry), then if the path exists and is a
 * directory then it's returned; if not, it is assumed it was a file so
 * we try to strip it, and try again. It allocates memory for the returned string.
 *
 * Returns: a newly allocated string with the path or NULL if something went
 * wrong
 **/
char *
gnome_file_entry_get_full_path(GnomeFileEntry *fentry, gboolean file_must_exist)
{
	char *p;
	char *t;

	g_return_val_if_fail (fentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_FILE_ENTRY (fentry), NULL);

	t = gtk_editable_get_chars (GTK_EDITABLE (gnome_file_entry_gtk_entry (fentry)), 0, -1);

	if (!t)
		return NULL;
	else if (!*t) {
		g_free (t);
		return NULL;
	}

	if (*t == G_DIR_SEPARATOR)
		p = t;
	else if (*t == '~') {
		p = tilde_expand (t);
		g_free (t);
	} else if (fentry->_priv->default_path) {
		p = g_concat_dir_and_file (fentry->_priv->default_path, t);
		g_free (t);
		if (*p == '~') {
			t = p;
			p = tilde_expand (t);
			g_free (t);
		}
	} else {
		gchar *cwd = g_get_current_dir ();

		p = g_concat_dir_and_file (cwd, t);
		g_free (cwd);
		g_free (t);
	}

	if (file_must_exist) {
		if (!p)
			return NULL;

		if (fentry->_priv->directory_entry) {
			char *d;

			if (g_file_test (p, G_FILE_TEST_IS_DIR))
				return p;

			d = g_path_get_dirname (p);
			g_free (p);

			if (g_file_test (d, G_FILE_TEST_IS_DIR))
				return d;

			p = d;
		} else if (g_file_test (p, G_FILE_TEST_EXISTS)) {
			return p;
		}

		g_free (p);
		return NULL;
	} else {
		return p;
	}
}

/**
 * gnome_file_entry_set_filename:
 * @fentry: The GnomeFileEntry widget to work with.
 *
 * Description: Sets the internal entry to this string.
 **/
void
gnome_file_entry_set_filename(GnomeFileEntry *fentry, const char *filename)
{
	GtkWidget *entry;

	g_return_if_fail (fentry != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (fentry));

	entry = gnome_file_entry_gtk_entry (fentry);

	gtk_entry_set_text(GTK_ENTRY(entry), filename);
}

/**
 * gnome_file_entry_set_modal:
 * @fentry: The GnomeFileEntry widget to work with.
 * @is_modal: true if the window is to be modal, false otherwise.
 *
 * Description: Sets the modality of the browse dialog.
 *
 * Returns:
 **/
void
gnome_file_entry_set_modal(GnomeFileEntry *fentry, gboolean is_modal)
{
	g_return_if_fail (fentry != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (fentry));

	fentry->_priv->is_modal = is_modal;
}

/**
 * gnome_file_entry_get_modal:
 * @fentry: The GnomeFileEntry widget to work with.
 *
 * Description:  This function gets the boolean which specifies if
 * the browsing dialog is modal or not
 *
 * Returns:  A boolean.
 **/
gboolean
gnome_file_entry_get_modal(GnomeFileEntry *fentry)
{
	g_return_val_if_fail (fentry != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_FILE_ENTRY (fentry), FALSE);

	return fentry->_priv->is_modal;
}

/**
 * gnome_file_entry_set_directory_entry:
 * @fentry: The GnomeFileEntry widget to work with.
 * @directory_entry: boolean
 *
 * Description: Sets whether this is a directory only entry.  If
 * @directory_entry is true, then #gnome_file_entry_get_full_path will
 * check for the file being a directory, and the browse dialog will have
 * the file list disabled.
 *
 * Returns:
 **/
void
gnome_file_entry_set_directory_entry(GnomeFileEntry *fentry, gboolean directory_entry)
{
	g_return_if_fail (fentry != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (fentry));

	fentry->_priv->directory_entry = directory_entry ? TRUE : FALSE;
}


/**
 * gnome_file_entry_get_directory_entry:
 * @fentry: The GnomeFileEntry widget to work with.
 * @directory_entry: boolean
 *
 * Description: Gets whether this is a directory only entry.
 * See also #gnome_file_entry_set_directory_entry.
 *
 * Returns:  A boolean.
 **/
gboolean
gnome_file_entry_get_directory_entry(GnomeFileEntry *fentry)
{
	g_return_val_if_fail (fentry != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_FILE_ENTRY (fentry), FALSE);

	return fentry->_priv->directory_entry;
}

#ifndef GNOME_DISABLE_DEPRECATED_SOURCE
/**
 * gnome_file_entry_set_directory:
 * @fentry: The GnomeFileEntry widget to work with.
 * @directory_entry: boolean
 *
 * Description:  Deprecated, use #gnome_file_entry_set_directory_entry
 *
 * Returns:
 **/
void
gnome_file_entry_set_directory(GnomeFileEntry *fentry, gboolean directory_entry)
{
	g_warning("gnome_file_entry_set_directory is deprecated, "
		  "please use gnome_file_entry_set_directory_entry");
	gnome_file_entry_set_directory_entry(fentry, directory_entry);
}
#endif /* GNOME_DISABLE_DEPRECATED_SOURCE */
