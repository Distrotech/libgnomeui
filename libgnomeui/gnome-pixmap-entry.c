/* GnomePixmapEntry widget - Combo box with "Browse" button for files and
 *			     preview space for pixmaps and a file picker in
 *			     electric eyes style (well kind of)
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: George Lebl <jirka@5z.com>
 */
#include <config.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
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
#include "libgnome/gnome-mime.h"
#include "gnome-file-entry.h"
#include "gnome-pixmap-entry.h"
#include "gnome-pixmap.h"

static void gnome_pixmap_entry_class_init (GnomePixmapEntryClass *class);
static void gnome_pixmap_entry_init       (GnomePixmapEntry      *pentry);
static void drag_data_get		  (GtkWidget          *widget,
					   GdkDragContext     *context,
					   GtkSelectionData   *selection_data,
					   guint               info,
					   guint               time,
					   GnomePixmapEntry   *pentry);
static void drag_data_received		  (GtkWidget        *widget,
					   GdkDragContext   *context,
					   gint              x,
					   gint              y,
					   GtkSelectionData *selection_data,
					   guint             info,
					   guint32           time,
					   GnomePixmapEntry *pentry);

static GtkVBoxClass *parent_class;

static GtkTargetEntry drop_types[] = { { "text/uri-list", 0, 0 } };

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
refresh_preview(GnomePixmapEntry *pentry)
{
	char *t;
	GdkImlibImage *im;

	g_return_if_fail (pentry != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry));

	if(!pentry->preview)
		return;

	t = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(pentry->fentry),
					   FALSE);

	if(pentry->last_preview && t && strcmp(t,pentry->last_preview)==0) {
		g_free(t);
		return;
	}
	if(!t || !g_file_test(t,G_FILE_TEST_ISLINK|G_FILE_TEST_ISFILE) ||
	   !(im = gdk_imlib_load_image (t))) {
		if(GNOME_IS_PIXMAP(pentry->preview)) {
			gtk_drag_source_unset (pentry->preview_sw);
			gtk_widget_destroy(pentry->preview->parent);
			pentry->preview = gtk_label_new(_("No Image"));
			gtk_widget_show(pentry->preview);
			gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pentry->preview_sw),
							      pentry->preview);
			g_free(pentry->last_preview);
			pentry->last_preview = NULL;
		}
		g_free(t);
		return;
	}
	if(GNOME_IS_PIXMAP(pentry->preview))
		gnome_pixmap_load_imlib (GNOME_PIXMAP(pentry->preview),im);
	else {
		gtk_widget_destroy(pentry->preview->parent);
		pentry->preview = gnome_pixmap_new_from_imlib (im);
		gtk_widget_show(pentry->preview);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pentry->preview_sw),
						      pentry->preview);
		if(!GTK_WIDGET_NO_WINDOW(pentry->preview)) {
			gtk_signal_connect (GTK_OBJECT (pentry->preview), "drag_data_get",
					    GTK_SIGNAL_FUNC (drag_data_get),pentry);
			gtk_drag_source_set (pentry->preview,
					     GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
					     drop_types, 1,
					     GDK_ACTION_COPY);
		}
		gtk_signal_connect (GTK_OBJECT (pentry->preview->parent), "drag_data_get",
				    GTK_SIGNAL_FUNC (drag_data_get),pentry);
		gtk_drag_source_set (pentry->preview->parent,
				     GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
				     drop_types, 1,
				     GDK_ACTION_COPY);
	}
	g_free(pentry->last_preview);
	pentry->last_preview = t;
	gdk_imlib_destroy_image(im);
}

static int change_timeout;
static GSList *changed_pentries = NULL;

static int
changed_timeout_func(gpointer data)
{
	GSList *li,*tmp;

	GDK_THREADS_ENTER();

	tmp = changed_pentries;
	changed_pentries = NULL;
	if(tmp) {
		for(li=tmp;li!=NULL;li=g_slist_next(li)) {
			refresh_preview(li->data);
		}
		g_slist_free(tmp);
		return TRUE;
	}
	change_timeout = 0;

	GDK_THREADS_LEAVE();

	return FALSE;
}

static void
entry_changed(GtkWidget *w, GnomePixmapEntry *pentry)
{
	g_return_if_fail (pentry != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry));

	if(change_timeout == 0) {
		refresh_preview(pentry);
		change_timeout =
			gtk_timeout_add(1000,changed_timeout_func,NULL);
	} else {
		if(!g_slist_find(changed_pentries,pentry))
			changed_pentries = g_slist_prepend(changed_pentries,
							   pentry);
	}
}

static void
setup_preview(GtkWidget *widget)
{
	char *p;
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
	pp = gnome_pixmap_new_from_imlib_at_size(im,w,h);
	gtk_widget_show(pp);

	gtk_container_add(GTK_CONTAINER(frame),pp);

	gdk_imlib_destroy_image(im);
}

static void
pentry_destroy(GnomePixmapEntry *pentry)
{
	g_return_if_fail (pentry != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry));

	pentry->preview = NULL;
	g_free(pentry->last_preview);
	pentry->last_preview = NULL;
	changed_pentries = g_slist_remove(changed_pentries,pentry);
}


static void
browse_clicked(GnomeFileEntry *fentry, GnomePixmapEntry *pentry)
{
	GtkWidget *w;
	GtkWidget *hbox;

	GtkFileSelection *fs;

	g_return_if_fail (fentry != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (fentry));
	g_return_if_fail (pentry != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry));

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
drag_data_received (GtkWidget        *widget,
		    GdkDragContext   *context,
		    gint              x,
		    gint              y,
		    GtkSelectionData *selection_data,
		    guint             info,
		    guint32           time,
		    GnomePixmapEntry *pentry)
{
	GtkWidget *entry;
	GList *files;

	g_return_if_fail (pentry != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry));

	entry = gnome_pixmap_entry_gtk_entry(pentry);

	/*here we extract the filenames from the URI-list we recieved*/
	files = gnome_uri_list_extract_filenames(selection_data->data);
	/*if there's isn't a file*/
	if(!files) {
		gtk_drag_finish(context,FALSE,FALSE,time);
		return;
	}

	gtk_entry_set_text (GTK_ENTRY(entry), files->data);

	/*free the list of files we got*/
	gnome_uri_list_free_strings (files);
}

static void
drag_data_get  (GtkWidget          *widget,
		GdkDragContext     *context,
		GtkSelectionData   *selection_data,
		guint               info,
		guint               time,
		GnomePixmapEntry   *pentry)
{
	char *string;
	char *file;

	g_return_if_fail (pentry != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry));

	file = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(pentry->fentry),
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
gnome_pixmap_entry_init (GnomePixmapEntry *pentry)
{
	GtkWidget *w;
	char *p;

	gtk_box_set_spacing (GTK_BOX (pentry), 4);

	gtk_signal_connect(GTK_OBJECT(pentry),"destroy",
			   GTK_SIGNAL_FUNC(pentry_destroy), NULL);

	pentry->do_preview = TRUE;

	pentry->last_preview = NULL;

	pentry->preview_sw = gtk_scrolled_window_new(NULL,NULL);
	gtk_drag_dest_set (GTK_WIDGET (pentry->preview_sw),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drop_types, 1, GDK_ACTION_COPY);
	gtk_signal_connect (GTK_OBJECT (pentry->preview_sw), "drag_data_received",
			    GTK_SIGNAL_FUNC (drag_data_received),pentry);
	/*for some reason we can't do this*/
	/*gtk_signal_connect (GTK_OBJECT (pentry->preview_sw), "drag_data_get",
			    GTK_SIGNAL_FUNC (drag_data_get),pentry);*/
	gtk_box_pack_start (GTK_BOX (pentry), pentry->preview_sw, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pentry->preview_sw),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_widget_set_usize(pentry->preview_sw,100,100);
	gtk_widget_show (pentry->preview_sw);

	pentry->preview = gtk_label_new(_("No Image"));
	gtk_widget_show(pentry->preview);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pentry->preview_sw),
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
	refresh_preview(pentry);
}

/**
 * gnome_pixmap_entry_construct:
 * @pentry: A #GnomePixmapEntry object to construct
 * @history_id: the id given to #gnome_entry_new
 * @browse_dialog_title: title of the browse dialog
 * @do_preview: boolean
 *
 * Description: Constructs the @gentry object.
 **/
void
gnome_pixmap_entry_construct (GnomePixmapEntry *pentry, const gchar *history_id,
			      const gchar *browse_dialog_title, gboolean do_preview)
{
	GtkWidget *gentry;
	gentry = gnome_file_entry_gnome_entry(GNOME_FILE_ENTRY(pentry->fentry));

	gnome_entry_set_history_id (GNOME_ENTRY (gentry), history_id);
	gnome_file_entry_set_title (GNOME_FILE_ENTRY(pentry->fentry),
				    browse_dialog_title);

	pentry->do_preview = do_preview;
	if(!do_preview)
		gtk_widget_hide(pentry->preview_sw);

}

/**
 * gnome_pixmap_entry_new:
 * @history_id: the id given to #gnome_entry_new
 * @browse_dialog_title: title of the browse dialog
 * @do_preview: boolean
 *
 * Description: Creates a new pixmap entry widget, if do_preview is false,
 * the preview is hidden but the files are still loaded so that it's easy
 * to show it. If you need a pixmap entry without the preview you just
 * use the GnomeFileEntry
 *
 * Returns: Returns the new object
 **/
GtkWidget *
gnome_pixmap_entry_new (const gchar *history_id, const gchar *browse_dialog_title, gboolean do_preview)
{
	GnomePixmapEntry *pentry;

	pentry = gtk_type_new (gnome_pixmap_entry_get_type ());

	gnome_pixmap_entry_construct (pentry, history_id, browse_dialog_title, do_preview);
	return GTK_WIDGET (pentry);
}

/**
 * gnome_pixmap_entry_gnome_file_entry:
 * @pentry: the GnomePixmapEntry to work with
 *
 * Description: Get the GnomeFileEntry widget that's part of the entry
 *
 * Returns: Returns GnomeFileEntry widget
 **/
GtkWidget *
gnome_pixmap_entry_gnome_file_entry (GnomePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry), NULL);

	return pentry->fentry;
}

/**
 * gnome_pixmap_entry_gnome_entry:
 * @pentry: the GnomePixmapEntry to work with
 *
 * Description: Get the GnomeEntry widget that's part of the entry
 *
 * Returns: Returns GnomeEntry widget
 **/
GtkWidget *
gnome_pixmap_entry_gnome_entry (GnomePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry), NULL);

	return gnome_file_entry_gnome_entry(GNOME_FILE_ENTRY(pentry->fentry));
}

/**
 * gnome_pixmap_entry_gtk_entry:
 * @pentry: the GnomePixmapEntry to work with
 *
 * Description: Get the GtkEntry widget that's part of the entry
 *
 * Returns: Returns GtkEntry widget
 **/
GtkWidget *
gnome_pixmap_entry_gtk_entry (GnomePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry), NULL);

	return gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (pentry->fentry));
}

/**
 * gnome_pixmap_entry_set_pixmap_subdir:
 * @pentry: the GnomePixmapEntry to work with
 * @subdir: sbudirectory
 *
 * Description: Sets the subdirectory below gnome's default
 * pixmap directory to use as the default path for the file
 * entry
 *
 * Returns:
 **/
void
gnome_pixmap_entry_set_pixmap_subdir(GnomePixmapEntry *pentry,
				     const gchar *subdir)
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

/**
 * gnome_pixmap_entry_set_preview:
 * @pentry: the GnomePixmapEntry to work with
 * @do_preview: bool
 *
 * Description: Sets if the preview should be shown or hidden, the files will
 * be loaded anyhow, so it doesn't make the thing any more faster
 *
 * Returns:
 **/
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

/**
 * gnome_pixmap_entry_set_preview_size:
 * @pentry: the GnomePixmapEntry to work with
 * @preview_w: preview width
 * @preview_h: preview height
 *
 * Description: Sets the minimum size of the preview frame
 *
 * Returns:
 **/
void
gnome_pixmap_entry_set_preview_size(GnomePixmapEntry *pentry,
				    int preview_w,
				    int preview_h)
{
	g_return_if_fail (pentry != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry));
	g_return_if_fail (preview_w>=0 && preview_h>=0);

	gtk_widget_set_usize(pentry->preview_sw,preview_w,preview_h);
}

/* Ensures that a pixmap entry is not in the waiting list for a preview update.  */
static void
ensure_update (GnomePixmapEntry *pentry)
{
	GSList *l;

	if (change_timeout == 0)
		return;

	l = g_slist_find (changed_pentries, pentry);
	if (!l)
		return;

	refresh_preview (pentry);
	changed_pentries = g_slist_remove_link (changed_pentries, l);
	g_slist_free_1 (l);

	if (!changed_pentries) {
		gtk_timeout_remove (change_timeout);
		change_timeout = 0;
	}
}

/**
 * gnome_pixmap_entry_get_filename:
 * @pentry: the GnomePixmapEntry to work with
 *
 * Description: Gets the file name of the image if it was possible
 * to load it into the preview
 *
 * Returns: a newly allocated string with the path or NULL if it
 * couldn't load the file
 **/
char *
gnome_pixmap_entry_get_filename(GnomePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry), NULL);

	ensure_update (pentry);

	/*this happens if it doesn't exist or isn't an image*/
	if (!GNOME_IS_PIXMAP (pentry->preview))
		return NULL;

	return gnome_file_entry_get_full_path (GNOME_FILE_ENTRY (pentry->fentry), TRUE);
}
