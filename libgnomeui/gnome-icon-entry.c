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
#include <unistd.h> /*getcwd*/
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
#include "libgnome/gnome-mime.h"
#include "libgnome/gnome-util.h"
#include "gnome-dialog.h"
#include "gnome-stock.h"
#include "gnome-file-entry.h"
#include "gnome-icon-list.h"
#include "gnome-icon-sel.h"
#include "gnome-icon-entry.h"
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>


static void gnome_icon_entry_class_init (GnomeIconEntryClass *class);
static void gnome_icon_entry_init       (GnomeIconEntry      *ientry);
static void drag_data_get		(GtkWidget          *widget,
					 GdkDragContext     *context,
					 GtkSelectionData   *selection_data,
					 guint               info,
					 guint               time,
					 GnomeIconEntry     *ientry);
static void drag_data_received		(GtkWidget        *widget,
					 GdkDragContext   *context,
					 gint              x,
					 gint              y,
					 GtkSelectionData *selection_data,
					 guint             info,
					 guint32           time,
					 GnomeIconEntry   *ientry);

static GtkVBoxClass *parent_class;

static GtkTargetEntry drop_types[] = { { "text/uri-list", 0, 0 } };

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
	gchar *t;
	GdkImlibImage *im;
	GtkWidget *child;
	int w,h;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	t = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(ientry->fentry),
					   FALSE);

	child = GTK_BIN(ientry->pickbutton)->child;
	
	if(!t || !g_file_test (t,G_FILE_TEST_ISLINK|G_FILE_TEST_ISFILE) ||
	   !(im = gdk_imlib_load_image (t))) {
		if(GNOME_IS_PIXMAP(child)) {
			gtk_drag_source_unset (ientry->pickbutton);
			gtk_widget_destroy(child);
			child = gtk_label_new(_("No Icon"));
			gtk_widget_show(child);
			gtk_container_add(GTK_CONTAINER(ientry->pickbutton),
					  child);
		}
		g_free(t);
		return;
	}
	g_free(t);
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
	if(GNOME_IS_PIXMAP(child))
		gnome_pixmap_load_imlib_at_size (GNOME_PIXMAP(child),im, w, h);
	else {
		gtk_widget_destroy(child);
		child = gnome_pixmap_new_from_imlib_at_size (im, w, h);
		gtk_widget_show(child);
		gtk_container_add(GTK_CONTAINER(ientry->pickbutton), child);

		if(!GTK_WIDGET_NO_WINDOW(child)) {
			gtk_signal_connect (GTK_OBJECT (child), "drag_data_get",
					    GTK_SIGNAL_FUNC (drag_data_get),ientry);
			gtk_drag_source_set (child,
					     GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
					     drop_types, 1,
					     GDK_ACTION_COPY);
		}
	}
	gdk_imlib_destroy_image(im);
	gtk_drag_source_set (ientry->pickbutton,
			     GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
			     drop_types, 1,
			     GDK_ACTION_COPY);
}

static void
entry_activated(GtkWidget *widget, GnomeIconEntry *ientry)
{
	struct stat buf;
	GnomeIconSelection * gis;
	gchar *filename;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_ENTRY (widget));
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	filename = gtk_entry_get_text (GTK_ENTRY (widget));

	if (!filename)
		return;

	stat (filename, &buf);
	if (S_ISDIR (buf.st_mode)) {
		gis = gtk_object_get_user_data(GTK_OBJECT(ientry));
		gnome_icon_selection_clear (gis, TRUE);
		gnome_icon_selection_add_directory (gis, filename);
		if (gis->file_list)
			gnome_icon_selection_show_icons(gis);
	} else {
		/* We pretend like ok has been called */
		entry_changed (NULL, ientry);
		gtk_widget_hide (ientry->pick_dialog);
	}
}

static void
setup_preview(GtkWidget *widget)
{
	gchar *p;
	GList *l;
	GtkWidget *pp = NULL;
	GdkImlibImage *im;
	int w,h;
	GtkWidget *frame;
	GtkFileSelection *fs;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

	frame = gtk_object_get_data(GTK_OBJECT(widget),"frame");
	fs = gtk_object_get_data(GTK_OBJECT(frame),"fs");

	if((l = gtk_container_children(GTK_CONTAINER(frame))) != NULL) {
		pp = l->data;
		g_list_free(l);
	}

	if(pp)
		gtk_widget_destroy(pp);
	
	p = gtk_file_selection_get_filename(fs);
	if(!p || !g_file_test (p,G_FILE_TEST_ISLINK|G_FILE_TEST_ISFILE) ||
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
	pp = gnome_pixmap_new_from_imlib_at_size (im, w, h);
	gtk_widget_show(pp);
	gtk_container_add(GTK_CONTAINER(frame),pp);

	gdk_imlib_destroy_image(im);
}

static void
ientry_destroy(GnomeIconEntry *ientry)
{
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	if(ientry->fentry)
		gtk_widget_unref (ientry->fentry);
	if(ientry->pick_dialog)
		gtk_widget_destroy(ientry->pick_dialog);
	if(ientry->pick_dialog_dir)
		g_free(ientry->pick_dialog_dir);
}


static void
browse_clicked(GnomeFileEntry *fentry, GnomeIconEntry *ientry)
{
	GtkWidget *w;
	GtkWidget *hbox;

	GtkFileSelection *fs;

	g_return_if_fail (fentry != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (fentry));
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

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
	gtk_signal_connect_while_alive(GTK_OBJECT(fs->selection_entry),
				       "changed",
				       GTK_SIGNAL_FUNC(setup_preview),NULL,
				       GTK_OBJECT(fs));
}

static void
icon_selected_cb(GtkButton * button, GnomeIconEntry * ientry)
{
	const gchar * icon;
	GnomeIconSelection * gis;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	gis =  gtk_object_get_user_data(GTK_OBJECT(ientry));
	gnome_icon_selection_stop_loading(gis);
	icon = gnome_icon_selection_get_icon(gis, TRUE);

	if (icon != NULL) {
		GtkWidget *e = gnome_icon_entry_gtk_entry(ientry);
		gtk_entry_set_text(GTK_ENTRY(e),icon);
		entry_changed (NULL, ientry);
	}
}

static void
cancel_pressed(GtkButton * button, GnomeIconEntry * ientry)
{
	GnomeIconSelection * gis;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	gis =  gtk_object_get_user_data(GTK_OBJECT(ientry));
	gnome_icon_selection_stop_loading(gis);
}


static void
gil_icon_selected_cb(GnomeIconList *gil, gint num, GdkEvent *event, GnomeIconEntry *ientry)
{
	const gchar * icon;
	GnomeIconSelection * gis;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	gis =  gtk_object_get_user_data(GTK_OBJECT(ientry));
	icon = gnome_icon_selection_get_icon(gis, TRUE);

	if (icon != NULL) {
		GtkWidget *e = gnome_icon_entry_gtk_entry(ientry);
		gtk_entry_set_text(GTK_ENTRY(e),icon);
		
	}

	if(event && event->type == GDK_2BUTTON_PRESS && ((GdkEventButton *)event)->button == 1) {
		gnome_icon_selection_stop_loading(gis);
		entry_changed (NULL, ientry);
		gtk_widget_hide(ientry->pick_dialog);
	}
}

static void
show_icon_selection(GtkButton * b, GnomeIconEntry * ientry)
{
	GnomeFileEntry *fe;
	gchar *p;
	gchar *curfile;
	GtkWidget *tl;

	g_return_if_fail (b != NULL);
	g_return_if_fail (GTK_IS_BUTTON (b));
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	fe = GNOME_FILE_ENTRY(ientry->fentry);
	p = gnome_file_entry_get_full_path(fe,FALSE);
	curfile = gnome_icon_entry_get_filename(ientry);

	/* Are we part of a modal window?  If so, we need to be modal too. */
	tl = gtk_widget_get_toplevel (GTK_WIDGET (b));
	
	if(!p) {
		if(fe->default_path)
			p = g_strdup(fe->default_path);
		else {
			/*get around the g_free/free issue*/
			gchar *cwd = g_get_current_dir ();
			p = g_strdup(cwd);
			g_free(cwd);
		}
		gtk_entry_set_text (GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (ientry->fentry))),
				    p);
	}

	/*figure out the directory*/
	if(!g_file_test (p,G_FILE_TEST_ISDIR)) {
		gchar *d;
		d = g_dirname (p);
		g_free (p);
		p = d;
		if(!g_file_test (p,G_FILE_TEST_ISDIR)) {
			g_free (p);
			if(fe->default_path)
				p = g_strdup(fe->default_path);
			else {
				/*get around the g_free/free issue*/
				gchar *cwd = g_get_current_dir ();
				p = g_strdup(cwd);
				free(cwd);
			}
			gtk_entry_set_text (GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (ientry->fentry))),
				    p);
			g_return_if_fail(g_file_test (p,G_FILE_TEST_ISDIR));
		}
	}
	

	if(ientry->pick_dialog==NULL ||
	   ientry->pick_dialog_dir==NULL ||
	   strcmp(p,ientry->pick_dialog_dir)!=0) {
		GtkWidget * iconsel;
		
		if(ientry->pick_dialog) {
			gtk_container_remove (GTK_CONTAINER (ientry->fentry->parent), ientry->fentry);
			gtk_widget_destroy(ientry->pick_dialog);
		}
		
		if(ientry->pick_dialog_dir)
			g_free(ientry->pick_dialog_dir);
		ientry->pick_dialog_dir = p;
		ientry->pick_dialog = 
			gnome_dialog_new(GNOME_FILE_ENTRY(ientry->fentry)->browse_dialog_title,
					 GNOME_STOCK_BUTTON_OK,
					 GNOME_STOCK_BUTTON_CANCEL,
					 NULL);
		if (GTK_WINDOW (tl)->modal) {
			gtk_window_set_modal (GTK_WINDOW (ientry->pick_dialog), TRUE);
			gnome_dialog_set_parent (GNOME_DIALOG (ientry->pick_dialog), GTK_WINDOW (tl)); 
		}
		gnome_dialog_close_hides(GNOME_DIALOG(ientry->pick_dialog), TRUE);
		gnome_dialog_set_close  (GNOME_DIALOG(ientry->pick_dialog), TRUE);

		gtk_window_set_policy(GTK_WINDOW(ientry->pick_dialog), 
				      TRUE, TRUE, TRUE);

		iconsel = gnome_icon_selection_new();

		gtk_object_set_user_data(GTK_OBJECT(ientry), iconsel);

		gnome_icon_selection_add_directory(GNOME_ICON_SELECTION(iconsel),
						   ientry->pick_dialog_dir);


		gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(ientry->pick_dialog)->vbox),
				   ientry->fentry, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(ientry->pick_dialog)->vbox),
				   iconsel, TRUE, TRUE, 0);

		gtk_widget_show_all(ientry->pick_dialog);

		gnome_icon_selection_show_icons(GNOME_ICON_SELECTION(iconsel));

		if(curfile)
			gnome_icon_selection_select_icon(GNOME_ICON_SELECTION(iconsel), 
							 g_filename_pointer(curfile));

		gnome_dialog_button_connect(GNOME_DIALOG(ientry->pick_dialog), 
					    0, /* OK button */
					    GTK_SIGNAL_FUNC(icon_selected_cb),
					    ientry);
		gnome_dialog_button_connect(GNOME_DIALOG(ientry->pick_dialog), 
					    1, /* Cancel button */
					    GTK_SIGNAL_FUNC(cancel_pressed),
					    ientry);
		gtk_signal_connect_after(GTK_OBJECT(GNOME_ICON_SELECTION(iconsel)->gil), "select_icon",
					 GTK_SIGNAL_FUNC(gil_icon_selected_cb),
					 ientry);
	} else {
		GnomeIconSelection *gis =
			gtk_object_get_user_data(GTK_OBJECT(ientry));
		if(!GTK_WIDGET_VISIBLE(ientry->pick_dialog))
			gtk_widget_show(ientry->pick_dialog);
		if(gis) gnome_icon_selection_show_icons(gis);
	}
}

static void
drag_data_received (GtkWidget        *widget,
		    GdkDragContext   *context,
		    gint              x,
		    gint              y,
		    GtkSelectionData *selection_data,
		    guint             info,
		    guint32           time,
		    GnomeIconEntry   *ientry)
{
	GList *files;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	/*here we extract the filenames from the URI-list we recieved*/
	files = gnome_uri_list_extract_filenames(selection_data->data);
	/*if there's isn't a file*/
	if(!files) {
		gtk_drag_finish(context,FALSE,FALSE,time);
		return;
	}

	gnome_icon_entry_set_icon(ientry,files->data);

	/*free the list of files we got*/
	gnome_uri_list_free_strings (files);
}

static void  
drag_data_get  (GtkWidget          *widget,
		GdkDragContext     *context,
		GtkSelectionData   *selection_data,
		guint               info,
		guint               time,
		GnomeIconEntry     *ientry)
{
	gchar *string;
	gchar *file;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	file = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(ientry->fentry),
					      TRUE);

	if(!file) {
		/*FIXME: cancel the drag*/
		return;
	}

	string = g_strdup_printf("file:%s\r\n",file);
	g_free(file);
	gtk_selection_data_set (selection_data,
				selection_data->target,
				8, string, strlen(string)+1);
	g_free(string);
}


static void
gnome_icon_entry_init (GnomeIconEntry *ientry)
{
	GtkWidget *w;
	gchar *p;

	gtk_box_set_spacing (GTK_BOX (ientry), 4);
	
	gtk_signal_connect(GTK_OBJECT(ientry),"destroy",
			   GTK_SIGNAL_FUNC(ientry_destroy), NULL);
	
	ientry->pick_dialog = NULL;
	ientry->pick_dialog_dir = NULL;
	
	w = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_widget_show(w);
	gtk_box_pack_start (GTK_BOX (ientry), w, TRUE, TRUE, 0);
	ientry->pickbutton = gtk_button_new_with_label(_("No Icon"));
	gtk_drag_dest_set (GTK_WIDGET (ientry->pickbutton),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drop_types, 1, GDK_ACTION_COPY);
	gtk_signal_connect (GTK_OBJECT (ientry->pickbutton),
			    "drag_data_received",
			    GTK_SIGNAL_FUNC (drag_data_received),ientry);
	gtk_signal_connect (GTK_OBJECT (ientry->pickbutton), "drag_data_get",
			    GTK_SIGNAL_FUNC (drag_data_get),ientry);

	gtk_signal_connect(GTK_OBJECT(ientry->pickbutton), "clicked",
			   GTK_SIGNAL_FUNC(show_icon_selection),ientry);
	/*FIXME: 60x60 is just larger then default 48x48, though icon sizes
	  are supposed to be selectable I guess*/
	gtk_widget_set_usize(ientry->pickbutton,60,60);
	gtk_container_add (GTK_CONTAINER (w), ientry->pickbutton);
	gtk_widget_show (ientry->pickbutton);

	ientry->fentry = gnome_file_entry_new (NULL,NULL);
	/*BORPORP */
	gnome_file_entry_set_modal (GNOME_FILE_ENTRY (ientry->fentry), TRUE);
	gtk_widget_ref (ientry->fentry);
	gtk_signal_connect_after(GTK_OBJECT(ientry->fentry),"browse_clicked",
				 GTK_SIGNAL_FUNC(browse_clicked),
				 ientry);

	gtk_widget_show (ientry->fentry);
	
	p = gnome_pixmap_file(".");
	gnome_file_entry_set_default_path(GNOME_FILE_ENTRY(ientry->fentry),p);
	g_free(p);
	
	w = gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(ientry->fentry));
/*	gtk_signal_connect_while_alive(GTK_OBJECT(w), "changed",
				       GTK_SIGNAL_FUNC(entry_changed),
				       ientry, GTK_OBJECT(ientry));*/
	gtk_signal_connect_while_alive(GTK_OBJECT(w), "activate",
				       GTK_SIGNAL_FUNC(entry_activated),
				       ientry, GTK_OBJECT(ientry));
	
	
	/*just in case there is a default that is an image*/
	entry_changed(w,ientry);
}

/**
 * gnome_icon_entry_new:
 * @history_id: the id given to #gnome_entry_new
 * @browse_dialog_title: title of the browse dialog and icon selection dialog
 *
 * Description: Creates a new icon entry widget
 *
 * Returns: Returns the new object
 **/
GtkWidget *
gnome_icon_entry_new (const gchar *history_id, const gchar *browse_dialog_title)
{
	GnomeIconEntry *ientry;
	GtkWidget *gentry;

	ientry = gtk_type_new (gnome_icon_entry_get_type ());
	
        /* Keep in sync with gnome_entry_new() - or better yet, 
           add a _construct() method once we are in development
           branch. 
        */

	gentry = gnome_file_entry_gnome_entry(GNOME_FILE_ENTRY(ientry->fentry));

	gnome_entry_set_history_id (GNOME_ENTRY (gentry), history_id);
	gnome_entry_load_history (GNOME_ENTRY (gentry));
	gnome_file_entry_set_title (GNOME_FILE_ENTRY(ientry->fentry),
				    browse_dialog_title);
	
	return GTK_WIDGET (ientry);
}

/**
 * gnome_icon_entry_gnome_file_entry:
 * @ientry: the GnomeIconEntry to work with
 *
 * Description: Get the GnomeFileEntry widget that's part of the entry
 *
 * Returns: Returns GnomeFileEntry widget
 **/
GtkWidget *
gnome_icon_entry_gnome_file_entry (GnomeIconEntry *ientry)
{
	g_return_val_if_fail (ientry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry), NULL);

	return ientry->fentry;
}

/**
 * gnome_icon_entry_gnome_entry:
 * @ientry: the GnomeIconEntry to work with
 *
 * Description: Get the GnomeEntry widget that's part of the entry
 *
 * Returns: Returns GnomeEntry widget
 **/
GtkWidget *
gnome_icon_entry_gnome_entry (GnomeIconEntry *ientry)
{
	g_return_val_if_fail (ientry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry), NULL);

	return gnome_file_entry_gnome_entry(GNOME_FILE_ENTRY(ientry->fentry));
}

/**
 * gnome_icon_entry_gtk_entry:
 * @ientry: the GnomeIconEntry to work with
 *
 * Description: Get the GtkEntry widget that's part of the entry
 *
 * Returns: Returns GtkEntry widget
 **/
GtkWidget *
gnome_icon_entry_gtk_entry (GnomeIconEntry *ientry)
{
	g_return_val_if_fail (ientry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry), NULL);

	return gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (ientry->fentry));
}

/**
 * gnome_icon_entry_set_pixmap_subdir:
 * @ientry: the GnomeIconEntry to work with
 * @subdir: subdirectory
 *
 * Description: Sets the subdirectory below gnome's default
 * pixmap directory to use as the default path for the file
 * entry.
 *
 * Returns:
 **/
void
gnome_icon_entry_set_pixmap_subdir(GnomeIconEntry *ientry,
				   const gchar *subdir)
{
	gchar *p;
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));
	
	if(!subdir)
		subdir = ".";

	p = gnome_pixmap_file(subdir);
	gnome_file_entry_set_default_path(GNOME_FILE_ENTRY(ientry->fentry),p);
	g_free(p);
}

/**
 * gnome_icon_entry_set_icon:
 * @ientry: the GnomeIconEntry to work with
 * @filename: a filename
 * 
 * Description: Sets the icon of GnomeIconEntry to be the one pointed to by
 * @filename (in the current subdirectory).
 *
 * Returns:
 **/
void
gnome_icon_entry_set_icon(GnomeIconEntry *ientry,
			  const gchar *filename)
{
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));
	
	if(!filename)
		filename = "";

	gtk_entry_set_text (GTK_ENTRY (gnome_icon_entry_gtk_entry (ientry)),
			    filename);
	entry_changed (NULL, ientry);
}

/**
 * gnome_icon_entry_get_filename:
 * @ientry: the GnomeIconEntry to work with
 *
 * Description: Gets the file name of the image if it was possible
 * to load it into the preview. That is, it will only return a filename
 * if the image exists and it was possible to load it as an image.
 *
 * Returns: a newly allocated string with the path or %NULL if it
 * couldn't load the file
 **/
gchar *
gnome_icon_entry_get_filename(GnomeIconEntry *ientry)
{
	GtkWidget *child;

	g_return_val_if_fail (ientry != NULL,NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry),NULL);

	child = GTK_BIN(ientry->pickbutton)->child;
	
	/* this happens if it doesn't exist or isn't an image */
	if(!GNOME_IS_PIXMAP(child))
		return NULL;
	
	return gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(ientry->fentry),
					      TRUE);
}
