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

/* GnomeFileSelectorDialog - the new file selector dialog.
 *
 * Author: Martin Baulig <baulig@suse.de>
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkeditable.h>
#include <gtk/gtklist.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtksignal.h>
#include "libgnome/libgnomeP.h"
#include "gnome-file-selector-dialog.h"
#include "gnome-gconf.h"

#include <glade/glade.h>
#include <gconf/gconf-client.h>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define GCONF_PREFIX "/desktop/gnome/gnome-file-selector-dialog"

#define GLADE_FILE_GCONF GCONF_PREFIX "/glade-file"
#define GLADE_WIDGET_NAME_GCONF GCONF_PREFIX "/glade-widget-name"

#define GLADE_FILE_DEFAULT "gnome-file-selector-dialog.glade"
#define GLADE_WIDGET_NAME_DEFAULT "GnomeFileSelectorDialog"

struct _GnomeFileSelectorDialogPrivate {
	GConfClient *client;

	GladeXML *xml;
	GtkWidget *main_widget;

	GtkWidget *home_button;
	GtkWidget *chdir_button;
	GtkWidget *current_dir_widget;
	GtkWidget *dir_list_widget;

	gchar *cur_selection;
	gboolean is_dir_selection;

	gchar *directory;
	gchar *home_directory;

	gboolean auto_dir_select : 1;
};
	

static void gnome_file_selector_dialog_class_init (GnomeFileSelectorDialogClass *class);
static void gnome_file_selector_dialog_init       (GnomeFileSelectorDialog      *fsdialog);
static void gnome_file_selector_dialog_destroy    (GtkObject       *object);
static void gnome_file_selector_dialog_finalize   (GObject         *object);

static void update_handler                        (GnomeFileSelectorDialog      *fsdialog);

static GtkWindowClass *parent_class;

enum {
	UPDATE_SIGNAL,
	LAST_SIGNAL
};

static int gnome_file_selector_dialog_signals [LAST_SIGNAL] = {0};

guint
gnome_file_selector_dialog_get_type (void)
{
	static guint fsdialog_type = 0;

	if (!fsdialog_type) {
		GtkTypeInfo fsdialog_info = {
			"GnomeFileSelectorDialog",
			sizeof (GnomeFileSelectorDialog),
			sizeof (GnomeFileSelectorDialogClass),
			(GtkClassInitFunc) gnome_file_selector_dialog_class_init,
			(GtkObjectInitFunc) gnome_file_selector_dialog_init,
			NULL,
			NULL,
			NULL
		};

		fsdialog_type = gtk_type_unique 
			(gtk_window_get_type (), &fsdialog_info);
	}

	return fsdialog_type;
}

static void
gnome_file_selector_dialog_class_init (GnomeFileSelectorDialogClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;

	parent_class = gtk_type_class (gtk_window_get_type ());

	gnome_file_selector_dialog_signals [UPDATE_SIGNAL] =
		gtk_signal_new ("update",
				GTK_RUN_LAST,
				GTK_CLASS_TYPE (object_class),
				GTK_SIGNAL_OFFSET (GnomeFileSelectorDialogClass,
						   update),
				gtk_signal_default_marshaller,
				GTK_TYPE_NONE,
				0);
	gtk_object_class_add_signals (object_class,
				      gnome_file_selector_dialog_signals,
				      LAST_SIGNAL);

	object_class->destroy = gnome_file_selector_dialog_destroy;
	gobject_class->finalize = gnome_file_selector_dialog_finalize;

	class->update = update_handler;
}

static void
gnome_file_selector_dialog_init (GnomeFileSelectorDialog *fsdialog)
{
	fsdialog->_priv = g_new0 (GnomeFileSelectorDialogPrivate, 1);
}

static void
set_widget_text (GtkWidget *widget, const gchar *text,
		 const gchar *widget_name)
{
	if (GTK_IS_LABEL (widget))
		gtk_label_set_text (GTK_LABEL (widget), text);
	else if (GTK_IS_EDITABLE (widget)) {
		gint position = 0;

		gtk_editable_delete_text (GTK_EDITABLE (widget), 0, -1);
		gtk_editable_insert_text (GTK_EDITABLE (widget), text,
					  strlen (text), &position);
	} else
		g_warning (G_STRLOC ": `%s' is of unsupported type `%s'.",
			   widget_name, G_OBJECT_TYPE_NAME (widget));
}

static gchar *
normalize_dir (const gchar *directory)
{
	gchar *cwd = g_get_current_dir ();
	gchar *retval;

	if (chdir (directory)) {
		chdir (cwd);
		g_free (cwd);
		return NULL;
	}

	retval = g_get_current_dir ();

	chdir (cwd);
	g_free (cwd);

	return retval;
}

static gboolean
chdir_to_current_selection (GnomeFileSelectorDialog *fsdialog)
{
	gchar *new_directory, *normalized;
	struct stat statb;

	g_return_val_if_fail (fsdialog != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_FILE_SELECTOR_DIALOG (fsdialog), FALSE);

	if (!fsdialog->_priv->is_dir_selection ||
	    (fsdialog->_priv->cur_selection == NULL) ||
	    (fsdialog->_priv->directory == NULL))
		return FALSE;

	new_directory = g_concat_dir_and_file (fsdialog->_priv->directory,
					       fsdialog->_priv->cur_selection);

	if (stat (new_directory, &statb)) {
		g_free (new_directory);
		return FALSE;
	}

	if (!S_ISDIR (statb.st_mode) || access (new_directory, X_OK|R_OK)) {
		g_free (new_directory);
		return FALSE;
	}

	normalized = normalize_dir (new_directory);
	g_free (new_directory);

	if (!normalized)
		return FALSE;

	g_free (fsdialog->_priv->directory);
	fsdialog->_priv->directory = normalized;

	return TRUE;
}

static void
directory_list_select_cb (GtkWidget *widget, gpointer data)
{
	GnomeFileSelectorDialog *fsdialog;
	gchar *dir;

	g_return_if_fail (data != NULL);
	g_return_if_fail (GNOME_IS_FILE_SELECTOR_DIALOG (data));
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_ITEM (widget));

	fsdialog = GNOME_FILE_SELECTOR_DIALOG (data);

	dir = GTK_LABEL (GTK_BIN (widget)->child)->label;

	g_print ("SELECT: `%s'", dir);

	if (fsdialog->_priv->cur_selection)
		g_free (fsdialog->_priv->cur_selection);
	fsdialog->_priv->cur_selection = g_filename_from_utf8 (dir);
	fsdialog->_priv->is_dir_selection = TRUE;

	if (fsdialog->_priv->auto_dir_select) {
		if (!chdir_to_current_selection (fsdialog))
			gdk_beep ();
		else
			gnome_file_selector_dialog_update (fsdialog);
	}

}

static void
directory_list_deselect_cb (GtkWidget *widget, gpointer data)
{
	GnomeFileSelectorDialog *fsdialog;
	gchar *dir;

	g_return_if_fail (data != NULL);
	g_return_if_fail (GNOME_IS_FILE_SELECTOR_DIALOG (data));
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_ITEM (widget));

	fsdialog = GNOME_FILE_SELECTOR_DIALOG (data);

	dir = GTK_LABEL (GTK_BIN (widget)->child)->label;

	g_print ("DESELECT: `%s'", dir);

	if (fsdialog->_priv->cur_selection)
		g_free (fsdialog->_priv->cur_selection);
	fsdialog->_priv->cur_selection = NULL;
	fsdialog->_priv->is_dir_selection = FALSE;
}

static void
update_directory_list (GnomeFileSelectorDialog *fsdialog, GtkWidget *widget,
		       const gchar *dir_name, const gchar *widget_name)
{
	DIR *directory;
	struct dirent *dirent_ptr;
	GList *entries = NULL;
	gchar *xdir;

	if (GTK_IS_LIST (widget)) {
		; /* ok */
	} else if (GTK_IS_CLIST (widget)) {
		if (GTK_CLIST (widget)->columns != 1) {
			g_warning (G_STRLOC ": `%s' has unexpected number "
				   "of columns.", widget_name);
			return;
		}
	} else {
		g_warning (G_STRLOC ": `%s' is of unsupported type `%s'.",
			   widget_name, G_OBJECT_TYPE_NAME (widget));
		return;
	}


	xdir = g_filename_from_utf8 (dir_name);
	directory = opendir (xdir);
	g_free (xdir);

	if (!directory) return;

	while ((dirent_ptr = readdir (directory)) != NULL) {
		gchar *filename, *path_name;
		struct stat statb;

		fprintf (stderr, "TEST: |%s|\n", dirent_ptr->d_name);

		path_name = g_concat_dir_and_file (xdir, dirent_ptr->d_name);

		if (stat (path_name, &statb)) {
			g_free (path_name);
			continue;
		}

		if (!S_ISDIR (statb.st_mode) || access (path_name, X_OK|R_OK)) {
			g_free (path_name);
			continue;
		}

		g_free (path_name);

		filename = g_filename_to_utf8 (dirent_ptr->d_name);
		entries = g_list_prepend (entries, filename);
	}

	entries = g_list_sort (entries, (GCompareFunc) strcmp);

	if (GTK_IS_LIST (widget)) {
		GList *c, *items = NULL;

		for (c = entries; c; c = c->next) {
			GtkWidget *item;

			item = gtk_list_item_new_with_label (c->data);
			gtk_signal_connect (GTK_OBJECT (item), "select",
					    directory_list_select_cb,
					    fsdialog);
			gtk_signal_connect (GTK_OBJECT (item), "deselect",
					    directory_list_deselect_cb,
					    fsdialog);
			gtk_widget_show (item);

			items = g_list_prepend (items, item);
		}

		items = g_list_reverse (items);

		gtk_list_clear_items (GTK_LIST (widget), 0, -1);
		gtk_list_prepend_items (GTK_LIST (widget), items);
	} else if (GTK_IS_CLIST (widget)) {
		GList *c;

		gtk_clist_clear (GTK_CLIST (widget));

		for (c = entries; c; c = c->next) {
			gchar *texts[2];

			texts [0] = c->data;
			texts [1] = NULL;

			gtk_clist_append (GTK_CLIST (widget), texts);
		}
	}

	g_list_foreach (entries, (GFunc) g_free, NULL);
	g_list_free (entries);
}

static void
update_handler (GnomeFileSelectorDialog *fsdialog)
{
	GnomeFileSelectorDialogPrivate *priv;

	g_return_if_fail (fsdialog != NULL);
	g_return_if_fail (GNOME_IS_FILE_SELECTOR_DIALOG (fsdialog));

	priv = fsdialog->_priv;

	if (priv->current_dir_widget)
		set_widget_text (priv->current_dir_widget,
				 priv->directory, "current-dir-widget");

	if (priv->dir_list_widget) {
		update_directory_list (fsdialog, priv->dir_list_widget,
				       priv->directory, "directory-list");
	}
}

static void
home_button_clicked_cb (GtkWidget *widget, gpointer data)
{
	GnomeFileSelectorDialog *fsdialog;

	g_return_if_fail (data != NULL);
	g_return_if_fail (GNOME_IS_FILE_SELECTOR_DIALOG (data));

	fsdialog = GNOME_FILE_SELECTOR_DIALOG (data);

	if (!fsdialog->_priv->home_directory) {
		g_warning (G_STRLOC ": No home directory set.");
		return;
	}

	gnome_file_selector_dialog_set_dir
		(fsdialog, fsdialog->_priv->home_directory);
	gnome_file_selector_dialog_update (fsdialog);
}

static void
chdir_button_clicked_cb (GtkWidget *widget, gpointer data)
{
	GnomeFileSelectorDialog *fsdialog;

	g_return_if_fail (data != NULL);
	g_return_if_fail (GNOME_IS_FILE_SELECTOR_DIALOG (data));

	fsdialog = GNOME_FILE_SELECTOR_DIALOG (data);

	if (!chdir_to_current_selection (fsdialog))
		gdk_beep ();
	else
		gnome_file_selector_dialog_update (fsdialog);
}


/**
 * gnome_file_selector_dialog_construct:
 * @fsdialog: Pointer to GnomeFileSelectorDialog object.
 *
 * Constructs a #GnomeFileSelectorDialog object, for language bindings or
 # subclassing. Use #gnome_file_selector_dialog_new from C.
 *
 * Returns: 
 */
void
gnome_file_selector_dialog_construct (GnomeFileSelectorDialog *fsdialog,
				      const gchar *dialog_title)
{
	gchar *glade_file, *glade_widget;
	GConfError *error = NULL;

	g_return_if_fail (fsdialog != NULL);
	g_return_if_fail (GNOME_IS_FILE_SELECTOR_DIALOG (fsdialog));

	gtk_window_set_title (GTK_WINDOW (fsdialog), dialog_title);

	fsdialog->_priv->client = gnome_get_gconf_client ();

	glade_file = gconf_client_get_string
		(fsdialog->_priv->client, GLADE_FILE_GCONF, &error);

	if (error) {
		g_warning (G_STRLOC ": %s", error->str);
		gconf_error_destroy (error);
		return;
	}
	if (!glade_file)
		glade_file = g_strdup (GLADE_FILE_DEFAULT);

	glade_widget = gconf_client_get_string
		(fsdialog->_priv->client, GLADE_WIDGET_NAME_GCONF, &error);

	if (error) {
		g_warning (G_STRLOC ": %s", error->str);
		gconf_error_destroy (error);
		return;
	}
	if (!glade_widget)
		glade_widget = g_strdup (GLADE_WIDGET_NAME_DEFAULT);

	fsdialog->_priv->xml = glade_xml_new (glade_file, glade_widget);

	if (!fsdialog->_priv->xml) {
		g_warning ("Failed to load libglade file `%s'.", glade_file);
		return;
	}

	fsdialog->_priv->main_widget = glade_xml_get_widget
		(fsdialog->_priv->xml, glade_widget);

	if (!fsdialog->_priv->main_widget) {
		g_warning ("Failed to create `%s' from libglade file `%s'.",
			   glade_widget, glade_file);
		return;
	}

	gtk_container_add (GTK_CONTAINER (fsdialog),
			   fsdialog->_priv->main_widget);

	fsdialog->_priv->home_button = glade_xml_get_widget
		(fsdialog->_priv->xml, "home-button");

	if (fsdialog->_priv->home_button)
		gtk_signal_connect (GTK_OBJECT (fsdialog->_priv->home_button),
				    "clicked", home_button_clicked_cb,
				    fsdialog);

	fsdialog->_priv->chdir_button = glade_xml_get_widget
		(fsdialog->_priv->xml, "chdir-button");

	if (fsdialog->_priv->chdir_button)
		gtk_signal_connect (GTK_OBJECT (fsdialog->_priv->chdir_button),
				    "clicked", chdir_button_clicked_cb,
				    fsdialog);

	fsdialog->_priv->current_dir_widget = glade_xml_get_widget
		(fsdialog->_priv->xml, "current-directory");

	fsdialog->_priv->dir_list_widget = glade_xml_get_widget
		(fsdialog->_priv->xml, "directory-list");

	fsdialog->_priv->auto_dir_select = TRUE;
}

/**
 * gnome_file_selector_dialog_new
 *
 * Description: Creates a new GnomeFileSelectorDialog widget.
 *
 * Returns: Newly-created GnomeFileSelectorDialog widget.
 */
GtkWidget *
gnome_file_selector_dialog_new (const gchar *dialog_title)
{
	GtkWidget *fsdialog;

	fsdialog = gtk_type_new (gnome_file_selector_dialog_get_type ());

	gnome_file_selector_dialog_construct
	    (GNOME_FILE_SELECTOR_DIALOG (fsdialog), dialog_title);

	return GTK_WIDGET (fsdialog);
}

static void
gnome_file_selector_dialog_destroy (GtkObject *object)
{
	GnomeFileSelectorDialog *fsdialog;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_FILE_SELECTOR_DIALOG (object));

	fsdialog = GNOME_FILE_SELECTOR_DIALOG (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gnome_file_selector_dialog_finalize (GObject *object)
{
	GnomeFileSelectorDialog *fsdialog;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_FILE_SELECTOR_DIALOG (object));

	fsdialog = GNOME_FILE_SELECTOR_DIALOG (object);

	g_free (fsdialog->_priv);
	fsdialog->_priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

void
gnome_file_selector_dialog_update (GnomeFileSelectorDialog *fsdialog)
{
	g_return_if_fail (fsdialog != NULL);
	g_return_if_fail (GNOME_IS_FILE_SELECTOR_DIALOG (fsdialog));

	gtk_signal_emit (GTK_OBJECT (fsdialog),
			 gnome_file_selector_dialog_signals [UPDATE_SIGNAL]);
}

void
gnome_file_selector_dialog_set_dir (GnomeFileSelectorDialog *fsdialog,
				    const gchar *directory)
{
	g_return_if_fail (fsdialog != NULL);
	g_return_if_fail (GNOME_IS_FILE_SELECTOR_DIALOG (fsdialog));
	g_return_if_fail (directory != NULL);

	if (fsdialog->_priv->directory)
		g_free (fsdialog->_priv->directory);

	fsdialog->_priv->directory = g_strdup (directory);
}

const gchar *
gnome_file_selector_dialog_get_dir (GnomeFileSelectorDialog *fsdialog)
{
	g_return_val_if_fail (fsdialog != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_FILE_SELECTOR_DIALOG (fsdialog), NULL);

	return fsdialog->_priv->directory;
}

void
gnome_file_selector_dialog_set_home_dir (GnomeFileSelectorDialog *fsdialog,
					 const gchar *home_directory)
{
	g_return_if_fail (fsdialog != NULL);
	g_return_if_fail (GNOME_IS_FILE_SELECTOR_DIALOG (fsdialog));
	g_return_if_fail (home_directory != NULL);

	if (fsdialog->_priv->home_directory)
		g_free (fsdialog->_priv->home_directory);

	fsdialog->_priv->home_directory = g_strdup (home_directory);
}

const gchar *
gnome_file_selector_dialog_get_home_dir (GnomeFileSelectorDialog *fsdialog)
{
	g_return_val_if_fail (fsdialog != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_FILE_SELECTOR_DIALOG (fsdialog), NULL);

	return fsdialog->_priv->home_directory;
}
