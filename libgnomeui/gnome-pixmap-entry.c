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

/* GnomePixmapEntry widget - Combo box with "Browse" button for files and
 *			     preview space for pixmaps and a file picker in
 *			     electric eyes style (well kind of)
 *
 * Author: George Lebl <jirka@5z.com>
 */
#include <config.h>
#include "gnome-macros.h"

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
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

#include "gnome-i18nP.h"

#include <libgnome/gnome-util.h>
#include "gnome-file-entry.h"
#include "gnome-pixmap-entry.h"
#include "gnome-pixmap.h"

#include <libgnomevfs/gnome-vfs-mime.h>

struct _GnomePixmapEntryPrivate {
	GtkWidget *preview;
	GtkWidget *preview_sw;
	
	gchar *last_preview;

	guint32 do_preview : 1; /*put a preview frame with the pixmap next to
				  the entry*/
};


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
static void pentry_destroy		  (GtkObject *object);
static void pentry_finalize		  (GObject *object);
static void pentry_set_arg                (GtkObject *object,
					   GtkArg *arg,
					   guint arg_id);
static void pentry_get_arg                (GtkObject *object,
					   GtkArg *arg,
					   guint arg_id);

static GtkTargetEntry drop_types[] = { { "text/uri-list", 0, 0 } };

enum {
	ARG_0,
	ARG_DO_PREVIEW
};

GNOME_CLASS_BOILERPLATE (GnomePixmapEntry, gnome_pixmap_entry,
			 GnomeFileEntry, gnome_file_entry)

static void
gnome_pixmap_entry_class_init (GnomePixmapEntryClass *class)
{
	GtkObjectClass *object_class = GTK_OBJECT_CLASS(class);
	GObjectClass *gobject_class = G_OBJECT_CLASS(class);

	gtk_object_add_arg_type("GnomePixmapEntry::do_preview",
				GTK_TYPE_BOOL,
				GTK_ARG_READWRITE,
				ARG_DO_PREVIEW);

	object_class->destroy = pentry_destroy;
	gobject_class->finalize = pentry_finalize;
	object_class->get_arg = pentry_get_arg;
	object_class->set_arg = pentry_set_arg;

}

static void
pentry_set_arg (GtkObject *object,
		GtkArg *arg,
		guint arg_id)
{
	GnomePixmapEntry *self;

	self = GNOME_PIXMAP_ENTRY (object);

	switch (arg_id) {
	case ARG_DO_PREVIEW:
		gnome_pixmap_entry_set_preview(self, GTK_VALUE_BOOL(*arg));
		break;

	default:
		break;
	}
}

static void
pentry_get_arg (GtkObject *object,
		GtkArg *arg,
		guint arg_id)
{
	GnomePixmapEntry *self;

	self = GNOME_PIXMAP_ENTRY (object);

	switch (arg_id) {
	case ARG_DO_PREVIEW:
		GTK_VALUE_BOOL(*arg) = self->_priv->do_preview;
		break;
	default:
		break;
	}
}

static void
refresh_preview(GnomePixmapEntry *pentry)
{
	char *t;
        GdkPixbuf *pixbuf;

	g_return_if_fail (pentry != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry));

	if( ! pentry->_priv->preview)
		return;
	
	t = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(pentry), FALSE);

	if(pentry->_priv->last_preview && t
	   &&
	   strcmp(t,pentry->_priv->last_preview)==0) {
		g_free(t);
		return;
	}
	if(!t || !g_file_test(t,G_FILE_TEST_IS_SYMLINK|G_FILE_TEST_IS_REGULAR) ||
	   !(pixbuf = gdk_pixbuf_new_from_file (t, NULL))) {
		if (GTK_IS_IMAGE (pentry->_priv->preview)) {
			gtk_drag_source_unset (pentry->_priv->preview_sw);
			gtk_widget_destroy(pentry->_priv->preview->parent);
			pentry->_priv->preview = gtk_label_new(_("No Image"));
			gtk_widget_show(pentry->_priv->preview);
			gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pentry->_priv->preview_sw),
							      pentry->_priv->preview);
			g_free(pentry->_priv->last_preview);
			pentry->_priv->last_preview = NULL;
		}
		g_free(t);
		return;
	}
	if(GTK_IS_IMAGE(pentry->_priv->preview)) {
                gtk_image_set_from_pixbuf (GTK_IMAGE (pentry->_priv->preview),
					   pixbuf);
        } else {
		gtk_widget_destroy(pentry->_priv->preview->parent);
		pentry->_priv->preview = gtk_image_new_from_pixbuf (pixbuf);
		gtk_widget_show(pentry->_priv->preview);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pentry->_priv->preview_sw),
						      pentry->_priv->preview);
		if(!GTK_WIDGET_NO_WINDOW(pentry->_priv->preview)) {
			gtk_signal_connect (GTK_OBJECT (pentry->_priv->preview), "drag_data_get",
					    GTK_SIGNAL_FUNC (drag_data_get),pentry);
			gtk_drag_source_set (pentry->_priv->preview,
					     GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
					     drop_types, 1,
					     GDK_ACTION_COPY);
		}
		gtk_signal_connect (GTK_OBJECT (pentry->_priv->preview->parent), "drag_data_get",
				    GTK_SIGNAL_FUNC (drag_data_get),pentry);
		gtk_drag_source_set (pentry->_priv->preview->parent,
				     GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
				     drop_types, 1,
				     GDK_ACTION_COPY);
	}
	g_free(pentry->_priv->last_preview);
	pentry->_priv->last_preview = t;
        gdk_pixbuf_unref(pixbuf);
}

static int change_timeout;
static GSList *changed_pentries = NULL;

static int
changed_timeout_func(gpointer data)
{
	GSList *li,*tmp;

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
	const char *p;
	GList *l;
	GtkWidget *pp = NULL;
        GdkPixbuf *pixbuf, *scaled;
	int w,h;
	GtkWidget *frame;
	GtkFileSelection *fs;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

	frame = gtk_object_get_data(GTK_OBJECT(widget),"frame");
	fs = gtk_object_get_data(GTK_OBJECT(frame),"fs");

	if((l = gtk_container_get_children(GTK_CONTAINER(frame))) != NULL) {
		pp = l->data;
		g_list_free(l);
	}

	if(pp)
		gtk_widget_destroy(pp);

	p = gtk_file_selection_get_filename(fs);
	if(!p || !g_file_test(p,G_FILE_TEST_IS_SYMLINK|G_FILE_TEST_IS_REGULAR) ||
	   !(pixbuf = gdk_pixbuf_new_from_file (p, NULL)))
		return;

	w = gdk_pixbuf_get_width(pixbuf);
	h = gdk_pixbuf_get_height(pixbuf);
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
	scaled = gdk_pixbuf_scale_simple
		(pixbuf, w, h, GDK_INTERP_BILINEAR);
        gdk_pixbuf_unref (pixbuf);
	pp = gtk_image_new_from_pixbuf (scaled);
        gdk_pixbuf_unref (scaled);

	gtk_widget_show(pp);

	gtk_container_add(GTK_CONTAINER(frame),pp);

}

static void
pentry_destroy(GtkObject *object)
{
	GnomePixmapEntry *pentry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP_ENTRY (object));

	/* remember, destroy can be run multiple times! */

	pentry = GNOME_PIXMAP_ENTRY(object);

	g_free(pentry->_priv->last_preview);
	pentry->_priv->last_preview = NULL;

	changed_pentries = g_slist_remove(changed_pentries, pentry);

	if(GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(object);

}

static void
pentry_finalize(GObject *object)
{
	GnomePixmapEntry *pentry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP_ENTRY (object));

	pentry = GNOME_PIXMAP_ENTRY(object);

	g_free(pentry->_priv);
	pentry->_priv = NULL;

	if(G_OBJECT_CLASS(parent_class)->finalize)
		(* G_OBJECT_CLASS(parent_class)->finalize)(object);

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
	GList *uris, *li;
	GnomeVFSURI *uri = NULL;
	GtkWidget *entry;
	
	g_return_if_fail (pentry != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry));

	entry = gnome_pixmap_entry_gtk_entry (pentry);

	/*here we extract the filenames from the URI-list we recieved*/
	uris = gnome_vfs_uri_list_parse (selection_data->data);

	/* FIXME: Support multiple files */
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

	gtk_entry_set_text (GTK_ENTRY (entry),
			    gnome_vfs_uri_get_path (uri));

	/*free the list of files we got*/
	gnome_vfs_uri_list_free (uris);
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

	file = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(pentry), TRUE);

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
turn_on_previewbox(GnomePixmapEntry *pentry)
{
	pentry->_priv->preview_sw = gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_ref(pentry->_priv->preview_sw);
	gtk_drag_dest_set (GTK_WIDGET (pentry->_priv->preview_sw),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drop_types, 1, GDK_ACTION_COPY);
	gtk_signal_connect (GTK_OBJECT (pentry->_priv->preview_sw), "drag_data_received",
			    GTK_SIGNAL_FUNC (drag_data_received),pentry);
	/*for some reason we can't do this*/
	/*gtk_signal_connect (GTK_OBJECT (pentry->_priv->preview_sw), "drag_data_get",
			    GTK_SIGNAL_FUNC (drag_data_get),pentry);*/
	gtk_box_pack_start (GTK_BOX (pentry), pentry->_priv->preview_sw, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pentry->_priv->preview_sw),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_widget_set_usize(pentry->_priv->preview_sw,100,100);
	gtk_widget_show (pentry->_priv->preview_sw);

	pentry->_priv->preview = gtk_label_new(_("No Image"));
	gtk_widget_ref(pentry->_priv->preview);
	gtk_widget_show(pentry->_priv->preview);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pentry->_priv->preview_sw),
					      pentry->_priv->preview);

	/*just in case there is a default that is an image*/
	refresh_preview(pentry);
}

static void
gnome_pixmap_entry_init (GnomePixmapEntry *pentry)
{
	GtkWidget *w;
	char *p;

	gtk_box_set_spacing (GTK_BOX (pentry), 4);

	pentry->_priv = g_new0(GnomePixmapEntryPrivate, 1);

	pentry->_priv->do_preview = 1;

	pentry->_priv->last_preview = NULL;

	pentry->_priv->preview = NULL;
	pentry->_priv->preview_sw = NULL;

	gtk_signal_connect_after(GTK_OBJECT(pentry),"browse_clicked",
				 GTK_SIGNAL_FUNC(browse_clicked),
				 pentry);

	p = gnome_program_locate_file (NULL /* program */,
				       GNOME_FILE_DOMAIN_PIXMAP,
				       ".",
				       FALSE /* only_if_exists */,
				       NULL /* ret_locations */);
	gnome_file_entry_set_default_path(GNOME_FILE_ENTRY(pentry),p);
	g_free(p);

	w = gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(pentry));
	gtk_signal_connect(GTK_OBJECT(w),"changed",
			   GTK_SIGNAL_FUNC(entry_changed),
			   pentry);

	/*just in case there is a default that is an image*/
	refresh_preview(pentry);
}

/**
 * gnome_pixmap_entry_construct:
 * @pentry: A #GnomePixmapEntry object to construct
 * @history_id: The id given to #gnome_entry_new
 * @browse_dialog_title: Title of the browse dialog
 * @do_preview: %TRUE if preview is desired, %FALSE if not.
 *
 * Description: Constructs the @pentry object.  If do_preview is %FALSE,
 * there is no preview, the files are not loaded and thus are not
 * checked to be real image files.
 **/
void
gnome_pixmap_entry_construct (GnomePixmapEntry *pentry, const gchar *history_id,
			      const gchar *browse_dialog_title, gboolean do_preview)
{
	GtkWidget *gentry;
	gentry = gnome_file_entry_gnome_entry(GNOME_FILE_ENTRY(pentry));

	gnome_file_entry_construct (GNOME_FILE_ENTRY (pentry),
				    history_id,
				    browse_dialog_title);

	pentry->_priv->do_preview = do_preview?1:0;
	if(do_preview)
		turn_on_previewbox(pentry);
}

/**
 * gnome_pixmap_entry_new:
 * @history_id: The id given to #gnome_entry_new
 * @browse_dialog_title: Title of the browse dialog
 * @do_preview: boolean
 *
 * Description: Creates a new pixmap entry widget, if do_preview is false,
 * there is no preview, the files are not loaded and thus are not
 * checked to be real image files.
 *
 * Returns: New GnomePixmapEntry object.
 **/
GtkWidget *
gnome_pixmap_entry_new (const gchar *history_id, const gchar *browse_dialog_title, gboolean do_preview)
{
	GnomePixmapEntry *pentry;

	pentry = gtk_type_new (gnome_pixmap_entry_get_type ());

	gnome_pixmap_entry_construct (pentry, history_id, browse_dialog_title, do_preview);
	return GTK_WIDGET (pentry);
}

#ifndef GNOME_DISABLE_DEPRECATED_SOURCE
/**
 * gnome_pixmap_entry_gnome_file_entry:
 * @pentry: Pointer to GnomePixmapEntry widget
 *
 * Description: Get the #GnomeFileEntry component of the
 * GnomePixmapEntry widget for lower-level manipulation.
 *
 * Returns: #GnomeFileEntry widget
 **/
GtkWidget *
gnome_pixmap_entry_gnome_file_entry (GnomePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry), NULL);

	return GTK_WIDGET (pentry);
}

/**
 * gnome_pixmap_entry_gnome_entry:
 * @pentry: Pointer to GnomePixmapEntry widget
 *
 * Description: Get the #GnomeEntry component of the
 * GnomePixmapEntry widget for lower-level manipulation.
 *
 * Returns: #GnomeEntry widget
 **/
GtkWidget *
gnome_pixmap_entry_gnome_entry (GnomePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry), NULL);

	return gnome_file_entry_gnome_entry(GNOME_FILE_ENTRY(pentry));
}

/**
 * gnome_pixmap_entry_gtk_entry:
 * @pentry: Pointer to GnomePixmapEntry widget
 *
 * Description: Get the #GtkEntry component of the
 * GnomePixmapEntry for Gtk+-level manipulation.
 *
 * Returns: #GtkEntry widget
 **/
GtkWidget *
gnome_pixmap_entry_gtk_entry (GnomePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry), NULL);

	return gnome_file_entry_gtk_entry
		(GNOME_FILE_ENTRY (pentry));
}
#endif /* GNOME_DISABLE_DEPRECATED_SOURCE */

/**
 * gnome_pixmap_entry_scrolled_window:
 * @pentry: Pointer to GnomePixmapEntry widget
 *
 * Description: Get the #GtkScrolledWindow widget that the preview
 * is contained in.  Could be %NULL
 *
 * Returns: #GtkScrolledWindow widget or %NULL
 **/
GtkWidget *
gnome_pixmap_entry_scrolled_window(GnomePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry), NULL);

	return pentry->_priv->preview_sw;
}

/**
 * gnome_pixmap_entry_preview_widget:
 * @pentry: Pointer to GnomePixmapEntry widget
 *
 * Description: Get the widget that is the preview.  Don't assume any
 * type of widget.  Currently either #GnomePixmap or #GtkLabel, but it
 * could change in the future. Could be %NULL
 *
 * Returns: the preview widget pointer or %NULL
 **/
GtkWidget *
gnome_pixmap_entry_preview_widget(GnomePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry), NULL);

	return pentry->_priv->preview;
}

/**
 * gnome_pixmap_entry_set_pixmap_subdir:
 * @pentry: Pointer to GnomePixmapEntry widget
 * @subdir: Subdirectory
 *
 * Description: Sets the default path for the file entry. The new
 * subdirectory should be specified relative to the default GNOME
 * pixmap directory.
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

	p = gnome_program_locate_file (NULL /* program */,
				       GNOME_FILE_DOMAIN_PIXMAP,
				       subdir,
				       FALSE /* only_if_exists */,
				       NULL /* ret_locations */);
	gnome_file_entry_set_default_path(GNOME_FILE_ENTRY(pentry),p);
	g_free(p);
}

/**
 * gnome_pixmap_entry_set_preview:
 * @pentry: Pointer to GnomePixmapEntry widget
 * @do_preview: %TRUE to show previews, %FALSE to not show them.
 *
 * Description: Sets whether or not the preview box is shown above
 * the entry.  If the preview is on, we also load the files and check
 * for them being real images.  If it is off, we don't check files
 * to be real image files.
 * 
 * Returns:
 **/
void
gnome_pixmap_entry_set_preview (GnomePixmapEntry *pentry, gboolean do_preview)
{
	g_return_if_fail (pentry != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry));

	if(pentry->_priv->do_preview ? 1 : 0 == do_preview ? 1 : 0)
		return;

	pentry->_priv->do_preview = do_preview ? 1 : 0;

	if(do_preview) {
		g_assert(pentry->_priv->preview_sw == NULL);
		g_assert(pentry->_priv->preview == NULL);
		turn_on_previewbox(pentry);
	} else {
		g_assert(pentry->_priv->preview_sw != NULL);
		g_assert(pentry->_priv->preview != NULL);
		gtk_widget_destroy(pentry->_priv->preview_sw);

		gtk_widget_unref(pentry->_priv->preview_sw);
		pentry->_priv->preview_sw = NULL;

		gtk_widget_unref(pentry->_priv->preview);
		pentry->_priv->preview = NULL;
	}
}

/**
 * gnome_pixmap_entry_set_preview_size:
 * @pentry: Pointer to GnomePixmapEntry widget
 * @preview_w: Preview width in pixels
 * @preview_h: Preview height in pixels
 *
 * Description: Sets the minimum size of the preview frame in pixels.
 * This works only if the preview is enabled.
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

	if(pentry->_priv->preview_sw)
		gtk_widget_set_usize(pentry->_priv->preview_sw,
				     preview_w,preview_h);
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
 * @pentry: Pointer to GnomePixmapEntry widget
 *
 * Description: Gets the filename of the image if the preview
 * successfully loaded if preview is disabled.  If the preview is
 * disabled, the file is only checked if it exists or not.
 *
 * Returns: Newly allocated string containing path, or %NULL on error. 
 **/
char *
gnome_pixmap_entry_get_filename(GnomePixmapEntry *pentry)
{
	g_return_val_if_fail (pentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PIXMAP_ENTRY (pentry), NULL);

	if (pentry->_priv->do_preview) {
		ensure_update (pentry);

		/*this happens if it doesn't exist or isn't an image*/
		if (!GTK_IS_IMAGE (pentry->_priv->preview))
			return NULL;
	}

	return gnome_file_entry_get_full_path (GNOME_FILE_ENTRY (pentry), TRUE);
}
