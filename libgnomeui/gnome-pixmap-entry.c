/* GnomePixmapEntry widget - Combo box with "Browse" button for files and
 *			     preview space for pixmaps and a file picker in
 *			     electric eyes style
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: George Lebl <jirka@5z.com>
 */
#include <config.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkscrolledwindow.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-i18nP.h"
#include "libgnome/gnome-util.h"
#include "gnome-pixmap.h"
#include "gnome-file-entry.h"
#include "gnome-pixmap-entry.h"


static void gnome_pixmap_entry_class_init (GnomePixmapEntryClass *class);
static void gnome_pixmap_entry_init       (GnomePixmapEntry      *pentry);

static GtkVBoxClass *parent_class;

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

		pixmap_entry_type = gtk_type_unique (gtk_vbox_get_type (),
						     &pixmap_entry_info);
	}

	return pixmap_entry_type;
}

static void
gnome_pixmap_entry_class_init (GnomePixmapEntryClass *class)
{
	parent_class = gtk_type_class (gtk_hbox_get_type ());
}

static void
entry_changed(GtkWidget *w, GnomePixmapEntry *pentry)
{
	char *t = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(pentry->fentry),
						 TRUE);
	if(!t) {
		if(pentry->preview)
			gtk_widget_destroy(pentry->preview);
		pentry->preview = NULL;
		return;
	}
	if(pentry->preview)
		gnome_pixmap_load_file(GNOME_PIXMAP(pentry->preview),t);
	else {
		pentry->preview = gnome_pixmap_new_from_file(t);
		gtk_widget_show(pentry->preview);
		gtk_container_add(GTK_CONTAINER(pentry->preview_sw),
				  pentry->preview);
	}
	g_free(t);
}

static void
gnome_pixmap_entry_init (GnomePixmapEntry *pentry)
{
	GtkWidget *w;
	char *p;

	gtk_box_set_spacing (GTK_BOX (pentry), 4);
	
	pentry->do_preview = TRUE;
	
	pentry->preview_sw = gtk_scrolled_window_new(NULL,NULL);
	gtk_box_pack_start (GTK_BOX (pentry), pentry->preview_sw, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pentry->preview_sw),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_widget_set_usize(pentry->preview_sw,100,100);
	gtk_widget_show (pentry->preview_sw);

	/*this will be added whn there is an existing file to load*/
	pentry->preview = NULL;

	pentry->fentry = gnome_file_entry_new (NULL,NULL);
	gtk_box_pack_start (GTK_BOX (pentry), pentry->fentry, FALSE, FALSE, 0);
	gtk_widget_show (pentry->fentry);
	
	p = gnome_pixmap_file(".");
	gnome_file_entry_set_default_path(GNOME_FILE_ENTRY(pentry->fentry),p);
	g_free(p);
	
	w = gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(pentry->fentry));
	gtk_signal_connect(GTK_OBJECT(w),"changed",
			   GTK_SIGNAL_FUNC(entry_changed),
			   pentry);
}

GtkWidget *
gnome_pixmap_entry_new (char *history_id, char *browse_dialog_title, int do_preview)
{
	GnomePixmapEntry *pentry;
	GtkWidget *gentry;

	pentry = gtk_type_new (gnome_pixmap_entry_get_type ());
	
	gentry = gnome_file_entry_gnome_entry(GNOME_FILE_ENTRY(pentry->fentry));

	gnome_entry_set_history_id (GNOME_ENTRY (gentry), history_id);
	gnome_file_entry_set_title (GNOME_FILE_ENTRY(pentry->fentry),
				    browse_dialog_title);
	
	pentry->do_preview = do_preview;
	if(!do_preview)
		gtk_widget_hide(pentry->preview_sw);

	return GTK_WIDGET (pentry);
}

GtkWidget *
gnome_pixmap_entry_gnome_file_entry (GnomePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry), NULL);

	return pentry->fentry;
}

GtkWidget *
gnome_pixmap_entry_gnome_entry (GnomePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry), NULL);

	return gnome_file_entry_gnome_entry(GNOME_FILE_ENTRY(pentry->fentry));
}

GtkWidget *
gnome_pixmap_entry_gtk_entry (GnomePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry), NULL);

	return gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (pentry->fentry));
}

void
gnome_pixmap_entry_set_pixmap_subdir(GnomePixmapEntry *pentry,
				     const char *subdir)
{
	char *p;
	g_return_if_fail (pentry != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry));
	
	if(!subdir)
		subdir = ".";

	p = gnome_pixmap_file(subdir);
	gnome_file_entry_set_default_path(GNOME_FILE_ENTRY(pentry->fentry),p);
	g_free(p);
}

void
gnome_pixmap_entry_set_preview (GnomePixmapEntry *pentry, int do_preview)
{
	g_return_if_fail (pentry != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry));
	
	pentry->do_preview = do_preview;
	if(do_preview)
		gtk_widget_show(pentry->preview_sw);
	else
		gtk_widget_hide(pentry->preview_sw);
}

void
gnome_pixmap_entry_set_preview_size(GnomePixmapEntry *pentry,
				    int preview_w,
				    int preview_h)
{
	g_return_if_fail (pentry != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry));
	
	gtk_widget_set_usize(pentry->preview_sw,preview_w,preview_h);
}
