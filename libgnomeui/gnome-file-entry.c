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
#include <sys/param.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <libgnome/gnome-macros.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>

/* Must be before all other gnome includes!! */
#include "gnome-i18nP.h"

#include <libgnome/gnome-util.h>
#include "libgnomeuiP.h"

#include "gnome-file-entry.h"

struct _GnomeFileEntryPrivate {
	GtkWidget *gentry;

	char *browse_dialog_title;

	gboolean is_modal : 1;

	gboolean directory_entry : 1; /*optional flag to only do directories*/

	/* FIXME: Non local files!! */
	/* FIXME: executable_entry as used in gnome_run */
	/* FIXME: multiple_entry for entering multiple filenames */
};

/* Private functions */
/* Expand files, useful here and in GnomeIconEntry */
char * _gnome_file_entry_expand_filename (const char *input,
					  const char *default_dir);


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

static void fentry_set_property (GObject *object, guint param_id,
				 const GValue *value, GParamSpec *pspec);
static void fentry_get_property (GObject *object, guint param_id,
				 GValue *value, GParamSpec *pspec);

static void gnome_file_entry_editable_init (GtkEditableClass *iface);

/* Property IDs */
enum {
	PROP_0,
	PROP_HISTORY_ID,
	PROP_BROWSE_DIALOG_TITLE,
	PROP_DIRECTORY_ENTRY,
	PROP_MODAL,
	PROP_FILENAME,
	PROP_DEFAULT_PATH,
	PROP_GNOME_ENTRY,
	PROP_GTK_ENTRY
};

/* Note, can't use boilerplate with interfaces yet,
 * should get sorted out */
static GtkVBoxClass *parent_class = NULL;
GType
gnome_file_entry_get_type (void)
{
	static GType object_type = 0;

	if (object_type == 0) {
		static const GTypeInfo object_info = {
			sizeof (GnomeFileEntryClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gnome_file_entry_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,			/* class_data */
			sizeof (GnomeFileEntry),
			0,			/* n_preallocs */
			(GInstanceInitFunc) gnome_file_entry_init,
			NULL			/* value_table */
		};

		static const GInterfaceInfo editable_info = {
			(GInterfaceInitFunc) gnome_file_entry_editable_init,	 /* interface_init */
			NULL,			                         	 /* interface_finalize */
			NULL			                         	 /* interface_data */
		};

		object_type = g_type_register_static (GTK_TYPE_VBOX,
						      "GnomeFileEntry",
						      &object_info, 0);

		g_type_add_interface_static (object_type,
					     GTK_TYPE_EDITABLE,
					     &editable_info);
	}
	return object_type;
}

enum {
	BROWSE_CLICKED_SIGNAL,
	ACTIVATE_SIGNAL,
	LAST_SIGNAL
};

static int gnome_file_entry_signals[LAST_SIGNAL] = {0};

static void
gnome_file_entry_class_init (GnomeFileEntryClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	parent_class = g_type_class_peek_parent (class);

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;

	object_class->destroy = gnome_file_entry_destroy;

	gobject_class->finalize = gnome_file_entry_finalize;
	gobject_class->set_property = fentry_set_property;
	gobject_class->get_property = fentry_get_property;
	
	gnome_file_entry_signals[BROWSE_CLICKED_SIGNAL] =
		g_signal_new("browse_clicked",
			     G_TYPE_FROM_CLASS (gobject_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GnomeFileEntryClass, browse_clicked),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);
	gnome_file_entry_signals[ACTIVATE_SIGNAL] =
		g_signal_new("activate",
			     G_TYPE_FROM_CLASS (gobject_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GnomeFileEntryClass, activate),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);

	g_object_class_install_property (gobject_class,
					 PROP_HISTORY_ID,
					 g_param_spec_string (
						 "history_id",
						 _("History ID"),
						 _("Unique identifier for the file entry.  "
						   "This will be used to save the history list."),
						 NULL,
						 G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_BROWSE_DIALOG_TITLE,
					 g_param_spec_string (
						 "browse_dialog_title",
						 _("Browse Dialog Title"),
						 _("Title for the Browse file dialog."),
						 NULL,
						 G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_DIRECTORY_ENTRY,
					 g_param_spec_boolean (
						 "directory_entry",
						 _("Directory Entry"),
						 _("Whether the file entry is being used to "
						   "enter directory names or complete filenames."),
						 FALSE,
						 G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_MODAL,
					 g_param_spec_boolean (
						 "modal",
						 _("Modal"),
						 _("Whether the Browse file window should be modal."),
						 FALSE,
						 G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_FILENAME,
					 g_param_spec_string (
						 "filename",
						 _("Filename"),
						 _("Filename that should be displayed in the "
						   "file entry."),
						 NULL,
						 G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_DEFAULT_PATH,
					 g_param_spec_string (
						 "default_path",
						 _("Default Path"),
						 _("Default path for the Browse file window."),
						 NULL,
						 G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_GNOME_ENTRY,
					 g_param_spec_object (
						 "gnome_entry",
						 _("GnomeEntry"),
						 _("GnomeEntry that the file entry uses for "
						   "entering filenames.  You can use this property "
						   "to get the GnomeEntry if you need to modify "
						   "or query any of its parameters."),
						 GNOME_TYPE_ENTRY,
						 G_PARAM_READABLE));
	g_object_class_install_property (gobject_class,
					 PROP_GTK_ENTRY,
					 g_param_spec_object (
						 "gtk_entry",
						 _("GtkEntry"),
						 _("GtkEntry that the file entry uses for "
						   "entering filenames.  You can use this property "
						   "to get the GtkEntry if you need to modify "
						   "or query any of its parameters."),
						 GTK_TYPE_ENTRY,
						 G_PARAM_READABLE));

	class->browse_clicked = browse_clicked;
	class->activate = NULL;
}

/* set_property handler for the file entry */
static void
fentry_set_property (GObject *object, guint param_id,
		     const GValue *value, GParamSpec *pspec)
{
	GnomeFileEntry *fentry;
	GnomeFileEntryPrivate *priv;

	fentry = GNOME_FILE_ENTRY (object);
	priv = fentry->_priv;

	switch (param_id) {
	case PROP_HISTORY_ID:
		g_object_set (priv->gentry, "history_id", g_value_get_string (value),
			      NULL);
		break;

	case PROP_BROWSE_DIALOG_TITLE:
		gnome_file_entry_set_title (fentry, g_value_get_string (value));
		break;

	case PROP_DIRECTORY_ENTRY:
		gnome_file_entry_set_directory_entry (fentry, g_value_get_boolean (value));
		break;

	case PROP_MODAL:
		gnome_file_entry_set_modal (fentry, g_value_get_boolean (value));
		break;

	case PROP_FILENAME:
		gnome_file_entry_set_filename (fentry, g_value_get_string (value));
		break;

	case PROP_DEFAULT_PATH:
		gnome_file_entry_set_default_path (fentry, g_value_get_string (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

/* get_property handler for the file entry */
static void
fentry_get_property (GObject *object, guint param_id,
		     GValue *value, GParamSpec *pspec)
{
	GnomeFileEntry *fentry;
	GnomeFileEntryPrivate *priv;

	fentry = GNOME_FILE_ENTRY (object);
	priv = fentry->_priv;

	switch (param_id) {
	case PROP_HISTORY_ID:
		g_value_set_string (value, gnome_entry_get_history_id (GNOME_ENTRY (priv->gentry)));
		break;

	case PROP_BROWSE_DIALOG_TITLE:
		g_value_set_string (value, priv->browse_dialog_title);
		break;

	case PROP_DIRECTORY_ENTRY:
		g_value_set_boolean (value, priv->directory_entry);
		break;

	case PROP_MODAL:
		g_value_set_boolean (value, priv->is_modal);
		break;

	case PROP_FILENAME: {
		char *filename;

		filename = gnome_file_entry_get_full_path (fentry, FALSE);
		g_value_set_string (value, filename);
		g_free (filename);
		break; }

	case PROP_DEFAULT_PATH:
		g_value_set_string (value, fentry->default_path);
		break;

	case PROP_GNOME_ENTRY:
		g_value_set_object (value, gnome_file_entry_gnome_entry (fentry));
		break;

	case PROP_GTK_ENTRY:
		g_value_set_object (value, gnome_file_entry_gtk_entry (fentry));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
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
	fentry = GNOME_FILE_ENTRY (g_object_get_data (G_OBJECT (fs), "gnome_file_entry"));
	entry = gnome_file_entry_gtk_entry (fentry);

	gtk_entry_set_text (GTK_ENTRY (entry),
			    gtk_file_selection_get_filename (fs));
	/* Is this evil? */
	g_signal_emit_by_name (entry, "activate");
	gtk_widget_destroy (GTK_WIDGET (fs));
}

static void
browse_dialog_kill (GtkWidget *widget, gpointer data)
{
	GnomeFileEntry *fentry;
	fentry = GNOME_FILE_ENTRY (data);

	fentry->fsw = NULL;
}

/* Does tilde (home directory) expansion on a string */
static char *
tilde_expand (const char *str)
{
	struct passwd *passwd;
	const char *p;
	char *name;

	g_assert (str != NULL);

	if (*str != '~')
		return g_strdup (str);

	str++;

	p = (const char *)strchr (str, G_DIR_SEPARATOR);

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

static gchar *
build_filename (GnomeFileEntry *fentry)
{
	const char *text;
	char *file;

	g_return_val_if_fail (fentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_FILE_ENTRY (fentry), NULL);

	text = gtk_entry_get_text
		(GTK_ENTRY (gnome_file_entry_gtk_entry (fentry)));

	if (text == NULL || text[0] == '\0')
		return NULL;

	file = _gnome_file_entry_expand_filename (text, fentry->default_path);
	if (file == NULL)
		return NULL;

	/* Now append an '/' if it doesn't exist and we're in directory only mode */
	if (fentry->_priv->directory_entry && strlen(file) > 0 &&
	    file[strlen (file)] != G_DIR_SEPARATOR ) {
		gchar *tmp;

		tmp = g_strconcat (file, "/", NULL);

		g_free (file);
		return tmp;
	}

	return file;
}

static void
browse_clicked(GnomeFileEntry *fentry)
{
	GtkWidget *fsw;
	GtkFileSelection *fs;
	char *p;

	/*if it already exists make sure it's shown and raised*/
	if(fentry->fsw) {
		gtk_widget_show(fentry->fsw);
		if(fentry->fsw->window)
			gdk_window_raise(fentry->fsw->window);
		fs = GTK_FILE_SELECTION(fentry->fsw);
		gtk_widget_set_sensitive(fs->file_list,
					 ! fentry->_priv->directory_entry);

		p = build_filename (fentry);
		if (p != NULL) {
			gtk_file_selection_set_filename (fs, p);
			g_free (p);
		}
		return;
	}


	fsw = gtk_file_selection_new (fentry->_priv->browse_dialog_title
				      ? fentry->_priv->browse_dialog_title
				      : _("Select file"));

	g_object_set_data (G_OBJECT (fsw), "gnome_file_entry", fentry);

	fs = GTK_FILE_SELECTION (fsw);
	gtk_widget_set_sensitive(fs->file_list,
				 ! fentry->_priv->directory_entry);

	p = build_filename (fentry);
	gtk_file_selection_set_filename (fs, p);
	g_free (p);

	g_signal_connect (fs->ok_button, "clicked",
			  G_CALLBACK (browse_dialog_ok),
			  fs);
	g_signal_connect_swapped (fs->cancel_button, "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  fsw);
	g_signal_connect (fsw, "destroy",
			  G_CALLBACK (browse_dialog_kill),
			  fentry);

	if (gtk_grab_get_current ())
		gtk_grab_add (fsw);

	gtk_widget_show (fsw);

	if(fentry->_priv->is_modal)
		gtk_window_set_modal (GTK_WINDOW (fsw), TRUE);
	fentry->fsw = fsw;
}

static void
browse_clicked_signal (GtkWidget *widget, gpointer data)
{
	g_signal_emit (data, gnome_file_entry_signals[BROWSE_CLICKED_SIGNAL], 0);
}

static void
entry_changed_signal (GtkWidget *widget, gpointer data)
{
	g_signal_emit_by_name (data, "changed");
}

static void
entry_activate_signal (GtkWidget *widget, gpointer data)
{
	g_signal_emit (data, gnome_file_entry_signals[ACTIVATE_SIGNAL], 0);
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
	fentry->default_path = NULL;
	fentry->_priv->is_modal = FALSE;
	fentry->_priv->directory_entry = FALSE;

	fentry->_priv->gentry = gnome_entry_new (NULL);
	the_gtk_entry = gnome_file_entry_gtk_entry (fentry);

	g_signal_connect (the_gtk_entry, "changed",
			  G_CALLBACK (entry_changed_signal),
			  fentry);
	g_signal_connect (the_gtk_entry, "activate",
			  G_CALLBACK (entry_activate_signal),
			  fentry);

	gtk_drag_dest_set (GTK_WIDGET (the_gtk_entry),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drop_types, ELEMENTS(drop_types), GDK_ACTION_COPY);

	g_signal_connect (the_gtk_entry, "drag_data_received",
			  G_CALLBACK (gnome_file_entry_drag_data_received),
			  NULL);

	gtk_box_pack_start (GTK_BOX (hbox), fentry->_priv->gentry, TRUE, TRUE, 0);
	gtk_widget_show (fentry->_priv->gentry);

	button = gtk_button_new_with_mnemonic (_("_Browse..."));
	g_signal_connect (button, "clicked",
			  G_CALLBACK (browse_clicked_signal),
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

	if (fentry->fsw != NULL)
		gtk_widget_destroy (fentry->fsw);
	fentry->fsw = NULL;

	GNOME_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gnome_file_entry_finalize (GObject *object)
{
	GnomeFileEntry *fentry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (object));

	fentry = GNOME_FILE_ENTRY (object);

	g_free (fentry->_priv->browse_dialog_title);
	fentry->_priv->browse_dialog_title = NULL;

	g_free (fentry->default_path);
	fentry->default_path = NULL;

	g_free (fentry->_priv);
	fentry->_priv = NULL;

	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
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

	g_object_set (G_OBJECT (fentry->_priv->gentry),
		      "history_id", history_id,
		      NULL);

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

	fentry = g_object_new (GNOME_TYPE_FILE_ENTRY, NULL);

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
	g_free(fentry->default_path);
	fentry->default_path = p;
}

/* Expand files, useful here and in GnomeIconEntry */
char *
_gnome_file_entry_expand_filename (const char *input, const char *default_dir)
{
	if (input == NULL) {
		return NULL;
	} else if (input[0] == '\0') {
		return g_strdup ("");
	} else if (input[0] == G_DIR_SEPARATOR) {
		return g_strdup (input);
	} else if (input[0] == '~') {
		return tilde_expand (input);
	} else if (default_dir != NULL) {
		char *ret = g_build_filename (default_dir, input, NULL);
		if (ret[0] == '~') {
			char *tmp = tilde_expand (ret);
			g_free (ret);
			return tmp;
		}
		return ret;
	} else {
		char *cwd = g_get_current_dir ();
		char *ret = g_build_filename (cwd, input, NULL);
		g_free (cwd);
		return ret;
	}
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
	const char *text;
	char *file;

	g_return_val_if_fail (fentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_FILE_ENTRY (fentry), NULL);

	text = gtk_entry_get_text
		(GTK_ENTRY (gnome_file_entry_gtk_entry (fentry)));

	if (text == NULL || text[0] == '\0')
		return NULL;

	file = _gnome_file_entry_expand_filename (text, fentry->default_path);
	if (file == NULL)
		return NULL;

	if (file_must_exist) {
		if (fentry->_priv->directory_entry) {
			char *d;

			if (g_file_test (file, G_FILE_TEST_IS_DIR))
				return file;

			d = g_path_get_dirname (file);
			g_free (file);

			if (g_file_test (d, G_FILE_TEST_IS_DIR))
				return d;
			g_free (d);

			return NULL;
		} else if (g_file_test (file, G_FILE_TEST_EXISTS)) {
			return file;
		}

		g_free (file);
		return NULL;
	} else {
		return file;
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

static void
insert_text (GtkEditable    *editable,
	     const gchar    *text,
	     gint            length,
	     gint           *position)
{
	GtkWidget *entry = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (editable));
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
	GtkWidget *entry = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (editable));
	gtk_editable_delete_text (GTK_EDITABLE (entry),
				  start_pos,
				  end_pos);
}

static gchar*
get_chars (GtkEditable    *editable,
	   gint            start_pos,
	   gint            end_pos)
{
	GtkWidget *entry = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (editable));
	return gtk_editable_get_chars (GTK_EDITABLE (entry),
				       start_pos,
				       end_pos);
}

static void
set_selection_bounds (GtkEditable    *editable,
		      gint            start_pos,
		      gint            end_pos)
{
	GtkWidget *entry = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (editable));
	gtk_editable_select_region (GTK_EDITABLE (entry),
				    start_pos,
				    end_pos);
}

static gboolean
get_selection_bounds (GtkEditable    *editable,
		      gint           *start_pos,
		      gint           *end_pos)
{
	GtkWidget *entry = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (editable));
	return gtk_editable_get_selection_bounds (GTK_EDITABLE (entry),
						  start_pos,
						  end_pos);
}

static void
set_position (GtkEditable    *editable,
	      gint            position)
{
	GtkWidget *entry = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (editable));
	gtk_editable_set_position (GTK_EDITABLE (entry),
				   position);
}

static gint
get_position (GtkEditable    *editable)
{
	GtkWidget *entry = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (editable));
	return gtk_editable_get_position (GTK_EDITABLE (entry));
}

static void
gnome_file_entry_editable_init (GtkEditableClass *iface)
{
	/* Just proxy to the GtkEntry */
	iface->insert_text = insert_text;
	iface->delete_text = delete_text;
	iface->get_chars = get_chars;
	iface->set_selection_bounds = set_selection_bounds;
	iface->get_selection_bounds = get_selection_bounds;
	iface->set_position = set_position;
	iface->get_position = get_position;
}
