/* GnomeIconEntry widget - Combo box with "Browse" button for files and
 *			   A pick button which can display a list of icons
 *			   in a current directory, the browse button displays
 *			   same dialog as pixmap-entry
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: George Lebl <jirka@5z.com>
 * icon selection based on original dentry-edit code which was:
 *	Written by: Havoc Pennington, based on code by John Ellis.
 */
#include <config.h>
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
#include "gnome-dialog.h"
#include "gnome-stock.h"
#include "gnome-file-entry.h"
#include "gnome-icon-sel.h"
#include "gnome-icon-entry.h"


static void gnome_icon_entry_class_init (GnomeIconEntryClass *class);
static void gnome_icon_entry_init       (GnomeIconEntry      *ientry);

static GtkVBoxClass *parent_class;

guint
gnome_icon_entry_get_type (void)
{
	static guint icon_entry_type = 0;

	if (!icon_entry_type) {
		GtkTypeInfo icon_entry_info = {
			"GnomeIconEntry",
			sizeof (GnomeIconEntry),
			sizeof (GnomeIconEntryClass),
			(GtkClassInitFunc) gnome_icon_entry_class_init,
			(GtkObjectInitFunc) gnome_icon_entry_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		icon_entry_type = gtk_type_unique (gtk_vbox_get_type (),
						   &icon_entry_info);
	}

	return icon_entry_type;
}

static void
gnome_icon_entry_class_init (GnomeIconEntryClass *class)
{
	parent_class = gtk_type_class (gtk_hbox_get_type ());
}

static void
entry_changed(GtkWidget *widget, GnomeIconEntry *ientry)
{
	char *t = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(ientry->fentry),
						 FALSE);
	GdkImlibImage *im;
	GdkPixmap *pix;
	GdkBitmap *mask;
	GtkWidget *child;
	int w,h;
	
	child = GTK_BIN(ientry->pickbutton)->child;
	
	if(!t || !g_file_test(t,G_FILE_TEST_ISLINK|G_FILE_TEST_ISFILE) ||
	   !(im = gdk_imlib_load_image (t))) {
		if(GTK_IS_PIXMAP(child)) {
			gtk_widget_destroy(child);
			child = gtk_label_new(_("No Icon"));
			gtk_widget_show(child);
			gtk_container_add(GTK_CONTAINER(ientry->pickbutton),
					  child);
		}
		return;
	}
	w = im->rgb_width;
	h = im->rgb_height;
	if(w>h) {
		if(w>48) {
			h = h*(48.0/w);
			w = 48;
		}
	} else {
		if(h>48) {
			w = w*(48.0/h);
			h = 48;
		}
	}
	gdk_imlib_render(im,w,h);
	pix = gdk_imlib_move_image (im);
	mask = gdk_imlib_move_mask (im);

	if(GTK_IS_PIXMAP(child))
		gtk_pixmap_set(GTK_PIXMAP(child),pix,mask);
	else {
		gtk_widget_destroy(child);
		child = gtk_pixmap_new(pix,mask);
		gtk_widget_show(child);
		gtk_container_add(GTK_CONTAINER(ientry->pickbutton),child);
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
	if(!p || !g_file_test(p,G_FILE_TEST_ISLINK|G_FILE_TEST_ISFILE) ||
	   !(im = gdk_imlib_load_image (p)))
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
ientry_destroy(GnomeIconEntry *ientry)
{
	if(ientry->pick_dialog)
		gtk_widget_destroy(ientry->pick_dialog);
	if(ientry->pick_dialog_dir)
		g_free(ientry->pick_dialog_dir);
	return FALSE;
}


static void
browse_clicked(GnomeFileEntry *fentry, GnomeIconEntry *ientry)
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
	gtk_signal_connect_while_alive(GTK_OBJECT(fs->selection_entry),"changed",
				       GTK_SIGNAL_FUNC(setup_preview),NULL,
				       GTK_OBJECT(fs));
}

static void
icon_selected_cb(GtkButton * button, GnomeIconEntry * ientry)
{
	const gchar * icon;
	GnomeIconSelection * gis;

	gis =  gtk_object_get_user_data(GTK_OBJECT(ientry));
	icon = gnome_icon_selection_get_icon(gis, TRUE);

	if (icon != NULL) {
		GtkWidget *e = gnome_icon_entry_gtk_entry(ientry);
		gtk_entry_set_text(GTK_ENTRY(e),icon);
	}
}

static void
show_icon_selection(GtkButton * b, GnomeIconEntry * ientry)
{
	GnomeFileEntry *fe = GNOME_FILE_ENTRY(ientry->fentry);
	char *p = gnome_file_entry_get_full_path(fe,FALSE);
	char *curfile = gnome_icon_entry_get_filename(ientry);
	
	if(!p) {
		if(fe->default_path)
			p = g_strdup(fe->default_path);
		else {
			/*get around the g_free/free issue*/
			char *cwd = getcwd(NULL,0);
			p = g_strdup(cwd);
			free(cwd);
		}
	}

	/*figure out the directory*/
	if(!g_file_test(p,G_FILE_TEST_ISDIR)) {
		char *d;
		d = g_dirname(p);
		g_free(p);
		p = d;
		if(!g_file_test(p,G_FILE_TEST_ISDIR)) {
			g_free(p);
			return;
		}
	}
	

	if(ientry->pick_dialog==NULL ||
	   ientry->pick_dialog_dir==NULL ||
	   strcmp(p,ientry->pick_dialog_dir)!=0) {
		GtkWidget * iconsel;
		
		if(ientry->pick_dialog)
			gtk_widget_destroy(ientry->pick_dialog);
		
		if(ientry->pick_dialog_dir)
			g_free(ientry->pick_dialog_dir);
		ientry->pick_dialog_dir = p;

		ientry->pick_dialog = 
			gnome_dialog_new(GNOME_FILE_ENTRY(ientry->fentry)->browse_dialog_title,
					 GNOME_STOCK_BUTTON_OK,
					 GNOME_STOCK_BUTTON_CANCEL,
					 NULL);
		gnome_dialog_close_hides(GNOME_DIALOG(ientry->pick_dialog), TRUE);
		gnome_dialog_set_close  (GNOME_DIALOG(ientry->pick_dialog), TRUE);

		gtk_window_set_policy(GTK_WINDOW(ientry->pick_dialog), 
				      TRUE, TRUE, TRUE);

		iconsel = gnome_icon_selection_new();

		gnome_icon_selection_add_directory(GNOME_ICON_SELECTION(iconsel),
						   ientry->pick_dialog_dir);


		gtk_widget_set_usize(GNOME_ICON_SELECTION(iconsel)->clist , 250, 350);

		gtk_container_add(GTK_CONTAINER(GNOME_DIALOG(ientry->pick_dialog)->vbox),
				  iconsel);

		gtk_widget_show_all(ientry->pick_dialog);

		gnome_icon_selection_show_icons(GNOME_ICON_SELECTION(iconsel));

		if(curfile)
			gnome_icon_selection_select_icon(GNOME_ICON_SELECTION(iconsel), 
							 g_filename_pointer(curfile));

		gnome_dialog_button_connect(GNOME_DIALOG(ientry->pick_dialog), 
					    0, /* OK button */
					    GTK_SIGNAL_FUNC(icon_selected_cb),
					    ientry);
		gtk_object_set_user_data(GTK_OBJECT(ientry), iconsel);
	} else {
		if(!GTK_WIDGET_VISIBLE(ientry->pick_dialog))
			gtk_widget_show(ientry->pick_dialog);
	}
}

static void
gnome_icon_entry_init (GnomeIconEntry *ientry)
{
	GtkWidget *w;
	char *p;

	gtk_box_set_spacing (GTK_BOX (ientry), 4);
	
	gtk_signal_connect(GTK_OBJECT(ientry),"destroy",
			   GTK_SIGNAL_FUNC(ientry_destroy), NULL);
	
	ientry->pick_dialog = NULL;
	ientry->pick_dialog_dir = NULL;
	
	w = gtk_hbox_new(FALSE,0);
	gtk_widget_show(w);
	gtk_box_pack_start (GTK_BOX (ientry), w, TRUE, TRUE, 0);
	ientry->pickbutton = gtk_button_new_with_label(_("No Icon"));
	gtk_signal_connect(GTK_OBJECT(ientry->pickbutton), "clicked",
			   GTK_SIGNAL_FUNC(show_icon_selection),ientry);
	/*FIXME: 60x60 is just larger then default 48x48, though icon sizes
	  are supposed to be selectable I guess*/
	gtk_widget_set_usize(ientry->pickbutton,60,60);
	gtk_box_pack_start (GTK_BOX (w), ientry->pickbutton,
			    TRUE, FALSE, 0);
	gtk_widget_show (ientry->pickbutton);

	ientry->fentry = gnome_file_entry_new (NULL,NULL);
	gtk_signal_connect_after(GTK_OBJECT(ientry->fentry),"browse_clicked",
				 GTK_SIGNAL_FUNC(browse_clicked),
				 ientry);
	gtk_box_pack_start (GTK_BOX (ientry), ientry->fentry, FALSE, FALSE, 0);
	gtk_widget_show (ientry->fentry);
	
	p = gnome_pixmap_file(".");
	gnome_file_entry_set_default_path(GNOME_FILE_ENTRY(ientry->fentry),p);
	g_free(p);
	
	w = gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(ientry->fentry));
	gtk_signal_connect_while_alive(GTK_OBJECT(w),"changed",
				       GTK_SIGNAL_FUNC(entry_changed),
				       ientry, GTK_OBJECT(ientry));

	/*just in case there is a default that is an image*/
	entry_changed(w,ientry);
}

GtkWidget *
gnome_icon_entry_new (char *history_id, char *browse_dialog_title)
{
	GnomeIconEntry *ientry;
	GtkWidget *gentry;

	ientry = gtk_type_new (gnome_icon_entry_get_type ());
	
	gentry = gnome_file_entry_gnome_entry(GNOME_FILE_ENTRY(ientry->fentry));

	gnome_entry_set_history_id (GNOME_ENTRY (gentry), history_id);
	gnome_file_entry_set_title (GNOME_FILE_ENTRY(ientry->fentry),
				    browse_dialog_title);
	
	return GTK_WIDGET (ientry);
}

GtkWidget *
gnome_icon_entry_gnome_file_entry (GnomeIconEntry *ientry)
{
	g_return_val_if_fail (ientry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry), NULL);

	return ientry->fentry;
}

GtkWidget *
gnome_icon_entry_gnome_entry (GnomeIconEntry *ientry)
{
	g_return_val_if_fail (ientry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry), NULL);

	return gnome_file_entry_gnome_entry(GNOME_FILE_ENTRY(ientry->fentry));
}

GtkWidget *
gnome_icon_entry_gtk_entry (GnomeIconEntry *ientry)
{
	g_return_val_if_fail (ientry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry), NULL);

	return gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (ientry->fentry));
}

void
gnome_icon_entry_set_pixmap_subdir(GnomeIconEntry *ientry,
				     const char *subdir)
{
	char *p;
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));
	
	if(!subdir)
		subdir = ".";

	p = gnome_pixmap_file(subdir);
	gnome_file_entry_set_default_path(GNOME_FILE_ENTRY(ientry->fentry),p);
	g_free(p);
}

char *
gnome_icon_entry_get_filename(GnomeIconEntry *ientry)
{
	/*this happens if it doesn't exist or isn't an image*/
	if(!GTK_IS_PIXMAP(GTK_BIN(ientry->pickbutton)->child))
		return NULL;
	return gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(ientry->fentry),
					      TRUE);
}
