/* GnomePixmapEntry widget - Combo box with "Browse" button for files and
 *			     preview space for pixmaps and a file picker in
 *			     electric eyes style
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Authors: George Lebl <jirka@5z.com>
 *	    Federico Mena <federico@nuclecu.unam.mx>
 */
#include <config.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-i18nP.h"
#include "gnome-file-entry.h"


static void gnome_pixmap_entry_class_init (GnomePixmapEntryClass *class);
static void gnome_pixmap_entry_init       (GnomePixmapEntry      *pentry);
static void gnome_pixmap_entry_finalize   (GtkObject           *object);
static void gnome_pixmap_entry_drag_data_received (GtkEntry         *widget,
						 GdkDragContext   *context,
						 gint              x,
						 gint              y,
						 GtkSelectionData *data,
						 guint             info,
						 guint             time);

static GtkHBoxClass *parent_class;

guint
gnome_pixmap_entry_get_type (void)
{
	static guint pixmap_entry_type = 0;

	if (!pixmap_entry_type) {
		GtkTypeInfo pixmap_entry_info = {
			"GnomePixmapEntry",
			sizeof (GnomePixmapEntry),
			sizeof (GnomePixmapEntryClass),
			(GtkClassInitFunc) gnome_pixmap_entry_class_init,
			(GtkObjectInitFunc) gnome_pixmap_entry_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		pixmap_entry_type = gtk_type_unique (gtk_hbox_get_type (), &pixmap_entry_info);
	}

	return pixmap_entry_type;
}

static void
gnome_pixmap_entry_class_init (GnomePixmapEntryClass *class)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) class;
	parent_class = gtk_type_class (gtk_hbox_get_type ());
	object_class->finalize = gnome_pixmap_entry_finalize;

}

static void
browse_dialog_ok (GtkWidget *widget, gpointer data)
{
	GtkFileSelection *fs;
	GnomePixmapEntry *pentry;
	GtkWidget *entry;

	fs = GTK_FILE_SELECTION (data);
	pentry = GNOME_PIXMAP_ENTRY (gtk_object_get_user_data (GTK_OBJECT (fs)));
	entry = gnome_pixmap_entry_gtk_entry (pentry);

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
	GnomePixmapEntry *pentry;
	GtkWidget *fsw;
	GtkFileSelection *fs;

	pentry = GNOME_PIXMAP_ENTRY (data);

	fsw = gtk_file_selection_new (pentry->browse_dialog_title
				      ? pentry->browse_dialog_title
				      : _("Select pixmap"));
	gtk_object_set_user_data (GTK_OBJECT (fsw), pentry);

	fs = GTK_FILE_SELECTION (fsw);

	gtk_file_selection_set_filename (fs, gtk_entry_get_text (GTK_ENTRY (gnome_pixmap_entry_gtk_entry (pentry))));

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
gnome_pixmap_entry_drag_data_received (GtkEntry         *widget,
				     GdkDragContext   *context,
				     gint              x,
				     gint              y,
				     GtkSelectionData *selection_data,
				     guint             info,
				     guint             time)
{
	gtk_entry_set_text (widget, selection_data->data);
}

#define ELEMENTS(x) (sizeof (x) / sizeof (x[0]))

static void
gnome_pixmap_entry_init (GnomePixmapEntry *pentry)
{
	GtkWidget *button, *the_gtk_entry;
	static const GtkTargetEntry drop_types[] = { { "url:ALL", 0, 0 } };

	pentry->browse_dialog_title = NULL;

	gtk_box_set_spacing (GTK_BOX (pentry), 4);

	pentry->gentry = gnome_entry_new (NULL);
	the_gtk_entry = gnome_pixmap_entry_gtk_entry (pentry);

	gtk_drag_dest_set (GTK_WIDGET (the_gtk_entry),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drop_types, ELEMENTS(drop_types), GDK_ACTION_COPY);

	gtk_box_pack_start (GTK_BOX (pentry), pentry->gentry, TRUE, TRUE, 0);
	gtk_widget_show (pentry->gentry);

	button = gtk_button_new_with_label (_("Browse..."));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    (GtkSignalFunc) browse_clicked,
			    pentry);
	gtk_box_pack_start (GTK_BOX (pentry), button, FALSE, FALSE, 0);
	gtk_widget_show (button);
}

static void
gnome_pixmap_entry_finalize (GtkObject *object)
{
	GnomePixmapEntry *pentry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP_ENTRY (object));

	pentry = GNOME_PIXMAP_ENTRY (object);

	if (pentry->browse_dialog_title)
		g_free (pentry->browse_dialog_title);

	(* GTK_OBJECT_CLASS (parent_class)->finalize) (object);
}

GtkWidget *
gnome_pixmap_entry_new (char *history_id, char *browse_dialog_title)
{
	GnomePixmapEntry *pentry;

	pentry = gtk_type_new (gnome_pixmap_entry_get_type ());

	gnome_entry_set_history_id (GNOME_ENTRY (pentry->gentry), history_id);
	gnome_pixmap_entry_set_title (pentry, browse_dialog_title);

	return GTK_WIDGET (pentry);
}

GtkWidget *
gnome_pixmap_entry_gnome_entry (GnomePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry), NULL);

	return pentry->gentry;
}

GtkWidget *
gnome_pixmap_entry_gtk_entry (GnomePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry), NULL);

	return gnome_entry_gtk_entry (GNOME_ENTRY (pentry->gentry));
}

void
gnome_pixmap_entry_set_title (GnomePixmapEntry *pentry, char *browse_dialog_title)
{
	g_return_if_fail (pentry != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry));

	if (pentry->browse_dialog_title)
		g_free (pentry->browse_dialog_title);

	pentry->browse_dialog_title = g_strdup (browse_dialog_title); /* handles NULL correctly */
}
