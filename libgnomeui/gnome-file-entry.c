/* GnomeFileEntry widget - Combo box with "Browse" button for files
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Authors: Federico Mena <federico@nuclecu.unam.mx>
 */
#include <config.h>
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
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-i18nP.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-mime.h"
#include "libgnomeui/gnome-preferences.h"
#include "libgnomeui/gnome-window-icon.h"
#include "gnome-file-entry.h"


static void gnome_file_entry_class_init (GnomeFileEntryClass *class);
static void gnome_file_entry_init       (GnomeFileEntry      *fentry);
static void gnome_file_entry_finalize   (GtkObject           *object);
static void gnome_file_entry_drag_data_received (GtkWidget        *widget,
						 GdkDragContext   *context,
						 gint              x,
						 gint              y,
						 GtkSelectionData *data,
						 guint             info,
						 guint32           time);
static void browse_clicked(GnomeFileEntry *fentry);
static GtkHBoxClass *parent_class;

guint
gnome_file_entry_get_type (void)
{
	static guint file_entry_type = 0;

	if (!file_entry_type){
		GtkTypeInfo file_entry_info = {
			"GnomeFileEntry",
			sizeof (GnomeFileEntry),
			sizeof (GnomeFileEntryClass),
			(GtkClassInitFunc) gnome_file_entry_class_init,
			(GtkObjectInitFunc) gnome_file_entry_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		file_entry_type = gtk_type_unique (gtk_hbox_get_type (), &file_entry_info);
	}

	return file_entry_type;
}

enum {
	BROWSE_CLICKED_SIGNAL,
	LAST_SIGNAL
};

static int gnome_file_entry_signals[LAST_SIGNAL] = {0};

static void
gnome_file_entry_class_init (GnomeFileEntryClass *class)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) class;
	parent_class = gtk_type_class (gtk_hbox_get_type ());

	gnome_file_entry_signals[BROWSE_CLICKED_SIGNAL] =
		gtk_signal_new("browse_clicked",
			       GTK_RUN_LAST,
			       object_class->type,
			       GTK_SIGNAL_OFFSET(GnomeFileEntryClass,
			       			 browse_clicked),
			       gtk_signal_default_marshaller,
			       GTK_TYPE_NONE,
			       0);
	gtk_object_class_add_signals(object_class,gnome_file_entry_signals,
				     LAST_SIGNAL);

	object_class->finalize = gnome_file_entry_finalize;

	class->browse_clicked = browse_clicked;

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
	char *p;

	/*if it already exists make sure it's shown and raised*/
	if(fentry->fsw) {
		gtk_widget_show(fentry->fsw);
		if(fentry->fsw->window)
			gdk_window_raise(fentry->fsw->window);
		fs = GTK_FILE_SELECTION(fentry->fsw);
		gtk_widget_set_sensitive(fs->file_list,
					 !fentry->directory_entry);
		p = gtk_entry_get_text (GTK_ENTRY (gnome_file_entry_gtk_entry (fentry)));
		if(p && *p!='/' && fentry->default_path) {
			p = g_concat_dir_and_file (fentry->default_path, p);
			gtk_file_selection_set_filename (fs, p);
			g_free(p);
		} else
			gtk_file_selection_set_filename (fs, p);
		return;
	}


	fsw = gtk_file_selection_new (fentry->browse_dialog_title
				      ? fentry->browse_dialog_title
				      : _("Select file"));
	gnome_window_icon_set_from_default (GTK_WINDOW (fsw));
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
				 !fentry->directory_entry);

	p = gtk_entry_get_text (GTK_ENTRY (gnome_file_entry_gtk_entry (fentry)));
	if(p && *p!='/' && fentry->default_path) {
		p = g_concat_dir_and_file (fentry->default_path, p);
		gtk_file_selection_set_filename (fs, p);
		g_free(p);
	} else
		gtk_file_selection_set_filename (fs, p);

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

	if(fentry->is_modal)
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
	GList *files;

	/*here we extract the filenames from the URI-list we recieved*/
	files = gnome_uri_list_extract_filenames(selection_data->data);
	/*if there's isn't a file*/
	if(!files) {
		gtk_drag_finish(context,FALSE,FALSE,time);
		return;
	}

	gtk_entry_set_text (GTK_ENTRY(widget), files->data);

	/*free the list of files we got*/
	gnome_uri_list_free_strings (files);
}

#define ELEMENTS(x) (sizeof (x) / sizeof (x[0]))

static void
gnome_file_entry_init (GnomeFileEntry *fentry)
{
	GtkWidget *button, *the_gtk_entry;
	static GtkTargetEntry drop_types[] = { { "text/uri-list", 0, 0 } };

	fentry->browse_dialog_title = NULL;
	fentry->default_path = NULL;
	fentry->is_modal = FALSE;
	fentry->directory_entry = FALSE;

	gtk_box_set_spacing (GTK_BOX (fentry), 4);

	fentry->gentry = gnome_entry_new (NULL);
	the_gtk_entry = gnome_file_entry_gtk_entry (fentry);

	gtk_drag_dest_set (GTK_WIDGET (the_gtk_entry),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drop_types, ELEMENTS(drop_types), GDK_ACTION_COPY);

	gtk_signal_connect (GTK_OBJECT (the_gtk_entry), "drag_data_received",
			    GTK_SIGNAL_FUNC (gnome_file_entry_drag_data_received),
			    NULL);

	gtk_box_pack_start (GTK_BOX (fentry), fentry->gentry, TRUE, TRUE, 0);
	gtk_widget_show (fentry->gentry);

	button = gtk_button_new_with_label (_("Browse..."));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    (GtkSignalFunc) browse_clicked_signal,
			    fentry);
	gtk_box_pack_start (GTK_BOX (fentry), button, FALSE, FALSE, 0);
	gtk_widget_show (button);
}

static void
gnome_file_entry_finalize (GtkObject *object)
{
	GnomeFileEntry *fentry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (object));

	fentry = GNOME_FILE_ENTRY (object);

	if (fentry->browse_dialog_title)
		g_free (fentry->browse_dialog_title);
	if (fentry->default_path)
		g_free (fentry->default_path);
	if (fentry->fsw)
		gtk_widget_destroy(fentry->fsw);

	(* GTK_OBJECT_CLASS (parent_class)->finalize) (object);
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

        /* Keep in sync with gnome_entry_new() - or better yet,
           add a _construct() method once we are in development
           branch.
        */

	gnome_entry_set_history_id (GNOME_ENTRY (fentry->gentry), history_id);
	gnome_entry_load_history (GNOME_ENTRY(fentry->gentry));
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

	return fentry->gentry;
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

	return gnome_entry_gtk_entry (GNOME_ENTRY (fentry->gentry));
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

	if (fentry->browse_dialog_title)
		g_free (fentry->browse_dialog_title);

	fentry->browse_dialog_title = g_strdup (browse_dialog_title); /* handles NULL correctly */
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

	if(path) {
		if(realpath(path,rpath))
			p = g_strdup(rpath);
		else
			p = NULL;
	} else
		p = NULL;

	if(fentry->default_path)
		g_free(fentry->default_path);

	/*handles NULL as well*/
	fentry->default_path = p;
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
 * #gnome_file_entry_set_directory), then if the path exists and is a
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

	if (*t == '/')
		p = t;
	else if (*t == '~') {
		p = tilde_expand (t);
		g_free (t);
	} else if (fentry->default_path) {
		p = g_concat_dir_and_file (fentry->default_path, t);
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

		if (fentry->directory_entry) {
			char *d;

			if (g_file_test (p, G_FILE_TEST_ISDIR))
				return p;

			d = g_dirname (p);
			g_free (p);

			if (g_file_test (d, G_FILE_TEST_ISDIR))
				return d;

			p = d;
		} else if (g_file_exists (p))
			return p;

		g_free (p);
		return NULL;
	} else
		return p;
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

	fentry->is_modal = is_modal;
}

/**
 * gnome_file_entry_set_directory:
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
gnome_file_entry_set_directory(GnomeFileEntry *fentry, gboolean directory_entry)
{
	g_return_if_fail (fentry != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (fentry));

	fentry->directory_entry = directory_entry;
}
