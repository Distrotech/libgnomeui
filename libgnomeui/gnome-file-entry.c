/* GnomeFileEntry widget - Combo box with "Browse" button for files
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <gtk/gtkbutton.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include "libgnome/libgnome.h"
#include "gnome-file-entry.h"


static void gnome_file_entry_class_init (GnomeFileEntryClass *class);
static void gnome_file_entry_init       (GnomeFileEntry      *fentry);
static void gnome_file_entry_finalize   (GtkObject           *object);


static GtkHBoxClass *parent_class;


guint
gnome_file_entry_get_type (void)
{
	static guint file_entry_type = 0;

	if (!file_entry_type) {
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

static void
gnome_file_entry_class_init (GnomeFileEntryClass *class)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) class;

	parent_class = gtk_type_class (gtk_hbox_get_type ());

	object_class->finalize = gnome_file_entry_finalize;
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

	gtk_entry_set_text (GTK_ENTRY (entry), gtk_file_selection_get_filename (fs));

	gtk_widget_destroy (GTK_WIDGET (fs));
}

static void
browse_dialog_cancel (GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy (GTK_WIDGET (data));
}

static void
browse_clicked (GtkWidget *widget, gpointer data)
{
	GnomeFileEntry *fentry;
	GtkWidget *fsw;
	GtkFileSelection *fs;

	fentry = GNOME_FILE_ENTRY (data);

	fsw = gtk_file_selection_new (fentry->browse_dialog_title
				      ? fentry->browse_dialog_title
				      : _("Select file"));
	gtk_object_set_user_data (GTK_OBJECT (fsw), fentry);

	fs = GTK_FILE_SELECTION (fsw);

	gtk_file_selection_set_filename (fs, gtk_entry_get_text (GTK_ENTRY (gnome_file_entry_gtk_entry (fentry))));

	gtk_signal_connect (GTK_OBJECT (fs->ok_button), "clicked",
			    (GtkSignalFunc) browse_dialog_ok,
			    fs);
	gtk_signal_connect (GTK_OBJECT (fs->cancel_button), "clicked",
			    (GtkSignalFunc) browse_dialog_cancel,
			    fs);

	gtk_widget_show (fsw);
	gtk_grab_add (fsw); /* Yes, it is modal, so sue me */
}

static void
gnome_file_entry_init (GnomeFileEntry *fentry)
{
	GtkWidget *button;

	fentry->browse_dialog_title = NULL;

	gtk_box_set_spacing (GTK_BOX (fentry), 4);

	fentry->gentry = gnome_entry_new (NULL);
	gtk_box_pack_start (GTK_BOX (fentry), fentry->gentry, TRUE, TRUE, 0);
	gtk_widget_show (fentry->gentry);

	button = gtk_button_new_with_label (_("Browse..."));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    (GtkSignalFunc) browse_clicked,
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

	(* GTK_OBJECT_CLASS (parent_class)->finalize) (object);
}

GtkWidget *
gnome_file_entry_new (char *history_id, char *browse_dialog_title)
{
	GnomeFileEntry *fentry;

	fentry = gtk_type_new (gnome_file_entry_get_type ());

	gnome_entry_set_history_id (GNOME_ENTRY (fentry->gentry), history_id);
	gnome_file_entry_set_title (fentry, browse_dialog_title);

	return GTK_WIDGET (fentry);
}

GtkWidget *
gnome_file_entry_gnome_entry (GnomeFileEntry *fentry)
{
	g_return_val_if_fail (fentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_FILE_ENTRY (fentry), NULL);

	return fentry->gentry;
}

GtkWidget *
gnome_file_entry_gtk_entry (GnomeFileEntry *fentry)
{
	g_return_val_if_fail (fentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_FILE_ENTRY (fentry), NULL);

	return gnome_entry_gtk_entry (GNOME_ENTRY (fentry->gentry));
}

void
gnome_file_entry_set_title (GnomeFileEntry *fentry, char *browse_dialog_title)
{
	g_return_if_fail (fentry != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (fentry));

	if (fentry->browse_dialog_title)
		g_free (fentry->browse_dialog_title);

	fentry->browse_dialog_title = g_strdup (browse_dialog_title); /* handles NULL correctly */
}
