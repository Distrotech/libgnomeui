/* GnomePixmapEntry widget - Combo box with "Browse" button for files and
 *			     preview space for pixmaps and a file picker in
 *			     electric eyes style (well kind of)
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: George Lebl <jirka@5z.com>
 */
#include <config.h>
#include <sys/stat.h> /*stat*/
#include <unistd.h> /*stat*/
#include <gdk_imlib.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkscrolledwindow.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-i18nP.h"
#include "libgnome/gnome-util.h"
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

static int
my_g_is_file (const char *filename)
{
	struct stat s;
	
	if(stat (filename, &s) != 0 ||
	   S_ISDIR (s.st_mode))
		return FALSE;
	return TRUE;
}

static void
entry_changed(GtkWidget *w, GnomePixmapEntry *pentry)
{
	char *t = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(pentry->fentry),
						 FALSE);
	GdkImlibImage *im;
	GdkPixmap *pix;
	GdkBitmap *mask;
	
	if(!pentry->preview)
		return;
	if(!t || !my_g_is_file(t) || !(im = gdk_imlib_load_image (t))) {
		if(GTK_IS_PIXMAP(pentry->preview)) {
			gtk_widget_destroy(pentry->preview);
			pentry->preview = gtk_label_new(_("No Image"));
			gtk_widget_show(pentry->preview);
			gtk_container_add(GTK_CONTAINER(pentry->preview_sw),
					  pentry->preview);
		}
		return;
	}
	gdk_imlib_render (im, im->rgb_width,im->rgb_height);
	pix = gdk_imlib_move_image (im);
	mask = gdk_imlib_move_mask (im);

	if(GTK_IS_PIXMAP(pentry->preview))
		gtk_pixmap_set(GTK_PIXMAP(pentry->preview),pix,mask);
	else {
		gtk_widget_destroy(pentry->preview);
		pentry->preview = gtk_pixmap_new(pix,mask);
		gtk_widget_show(pentry->preview);
		gtk_container_add(GTK_CONTAINER(pentry->preview_sw),
				  pentry->preview);
	}
	g_free(t);
	gdk_imlib_destroy_image(im);
}

static void
setup_preview(GtkWidget *widget)
{
	char *p;
	GList *l;
	GtkWidget *pp = NULL;
	GdkImlibImage *im;
	GdkPixmap *pix;
	GdkBitmap *mask;
	int w,h;
	GtkWidget *frame = gtk_object_get_data(GTK_OBJECT(widget),"frame");
	GtkFileSelection *fs = gtk_object_get_data(GTK_OBJECT(frame),"fs");

	if((l = gtk_container_children(GTK_CONTAINER(frame))) != NULL) {
		pp = l->data;
		g_list_free(l);
	}

	if(pp)
		gtk_widget_destroy(pp);
	
	p = gtk_file_selection_get_filename(fs);
	if(!p || !my_g_is_file(p) || !(im = gdk_imlib_load_image (p)))
		return;

	w = im->rgb_width;
	h = im->rgb_height;
	if(w>h) {
		if(w>100) {
			h = h*(100.0/w);
			w = 100;
		}
	} else {
		if(h>100) {
			w = w*(100.0/h);
			h = 100;
		}
	}
	gdk_imlib_render(im,w,h);
	pix = gdk_imlib_move_image(im);
	mask = gdk_imlib_move_mask(im);

	pp = gtk_pixmap_new(pix,mask);
	gtk_widget_show(pp);
	gtk_container_add(GTK_CONTAINER(frame),pp);

	gdk_imlib_destroy_image(im);
}

static int
pentry_destroy(GnomePixmapEntry *pentry)
{
	pentry->preview = NULL;
	return FALSE;
}


static void
browse_clicked(GnomeFileEntry *fentry, GnomePixmapEntry *pentry)
{
	GtkWidget *w;
	GtkWidget *hbox;

	GtkFileSelection *fs;
	if(!fentry->fsw)
		return;
	fs = GTK_FILE_SELECTION(fentry->fsw);
	
	hbox = fs->file_list;
	do {
		hbox = hbox->parent;
		if(!hbox) {
			g_warning(_("Can't find an hbox, using a normal file "
				    "selection"));
			return;
		}
	} while(!GTK_IS_HBOX(hbox));

	w = gtk_frame_new(_("Preview"));
	gtk_widget_show(w);
	gtk_box_pack_end(GTK_BOX(hbox),w,FALSE,FALSE,0);
	gtk_widget_set_usize(w,110,110);
	gtk_object_set_data(GTK_OBJECT(w),"fs",fs);
	
	gtk_object_set_data(GTK_OBJECT(fs->file_list),"frame",w);
	gtk_signal_connect(GTK_OBJECT(fs->file_list),"select_row",
			   GTK_SIGNAL_FUNC(setup_preview),NULL);
	gtk_object_set_data(GTK_OBJECT(fs->selection_entry),"frame",w);
	gtk_signal_connect(GTK_OBJECT(fs->selection_entry),"changed",
			   GTK_SIGNAL_FUNC(setup_preview),NULL);
}

static void
gnome_pixmap_entry_init (GnomePixmapEntry *pentry)
{
	GtkWidget *w;
	char *p;

	gtk_box_set_spacing (GTK_BOX (pentry), 4);
	
	gtk_signal_connect(GTK_OBJECT(pentry),"destroy",
			   GTK_SIGNAL_FUNC(pentry_destroy), NULL);
	
	pentry->do_preview = TRUE;
	
	pentry->preview_sw = gtk_scrolled_window_new(NULL,NULL);
	gtk_box_pack_start (GTK_BOX (pentry), pentry->preview_sw, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pentry->preview_sw),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_widget_set_usize(pentry->preview_sw,100,100);
	gtk_widget_show (pentry->preview_sw);

	pentry->preview = gtk_label_new(_("No Image"));
	gtk_widget_show(pentry->preview);
	gtk_container_add(GTK_CONTAINER(pentry->preview_sw),
			  pentry->preview);

	pentry->fentry = gnome_file_entry_new (NULL,NULL);
	gtk_signal_connect_after(GTK_OBJECT(pentry->fentry),"browse_clicked",
				 GTK_SIGNAL_FUNC(browse_clicked),
				 pentry);
	gtk_box_pack_start (GTK_BOX (pentry), pentry->fentry, FALSE, FALSE, 0);
	gtk_widget_show (pentry->fentry);
	
	p = gnome_pixmap_file(".");
	gnome_file_entry_set_default_path(GNOME_FILE_ENTRY(pentry->fentry),p);
	g_free(p);
	
	w = gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(pentry->fentry));
	gtk_signal_connect(GTK_OBJECT(w),"changed",
			   GTK_SIGNAL_FUNC(entry_changed),
			   pentry);

	/*just in case there is a default that is an image*/
	entry_changed(w,pentry);
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

char *
gnome_pixmap_entry_get_filename(GnomePixmapEntry *pentry)
{
	/*this happens if it doesn't exist or isn't an image*/
	if(!GTK_IS_PIXMAP(pentry->preview))
		return NULL;
	return gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(pentry->fentry),
					      TRUE);
}
