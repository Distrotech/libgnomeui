/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
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

/* GnomeIconEntry widget - combo box with auto-saved history
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkalignment.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-ditem.h"
#include "libgnome/libgnomeP.h"
#include "gnome-macros.h"
#include "gnome-vfs-util.h"
#include "gnome-selectorP.h"
#include "gnome-icon-selector.h"
#include "gnome-icon-entry.h"
#include "gnome-dialog.h"
#include "gnome-stock.h"

#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-file-info.h>

#define ICON_SIZE 48

typedef struct _GnomeIconEntryAsyncData         GnomeIconEntryAsyncData;

struct _GnomeIconEntryAsyncData {
    GnomeSelectorAsyncHandle *async_handle;

    GnomeGdkPixbufAsyncHandle *handle;
    GnomeIconEntry *ientry;
    gchar *uri;
};

struct _GnomeIconEntryPrivate {
    GtkWidget *browse_button;
    GtkWidget *browse_dialog;
    GtkWidget *icon_selector;

    gchar *current_icon;
};
	

static void   gnome_icon_entry_class_init  (GnomeIconEntryClass *class);
static void   gnome_icon_entry_init        (GnomeIconEntry      *ientry);
static void   gnome_icon_entry_destroy     (GtkObject           *object);
static void   gnome_icon_entry_finalize    (GObject             *object);

static void   drag_data_get                (GtkWidget           *widget,
                                            GdkDragContext      *context,
                                            GtkSelectionData    *selection_data,
                                            guint                info,
                                            guint                time,
                                            GnomeIconEntry      *ientry);
static void   drag_data_received           (GtkWidget           *widget,
                                            GdkDragContext      *context,
                                            gint                 x,
                                            gint                 y,
                                            GtkSelectionData    *selection_data,
                                            guint                info,
                                            guint32              time,
                                            GnomeIconEntry      *ientry);

static gchar    *get_uri_handler           (GnomeSelector            *selector);
static void      set_uri_handler           (GnomeSelector            *selector,
                                            const gchar              *filename,
                                            GnomeSelectorAsyncHandle *async_handle);

static GtkTargetEntry drop_types[] = { { "text/uri-list", 0, 0 } };


/**
 * gnome_icon_entry_get_type
 *
 * Returns the type assigned to the GnomeIconEntry widget.
 **/
/* The following defines the get_type */
GNOME_CLASS_BOILERPLATE (GnomeIconEntry, gnome_icon_entry,
			 GnomeFileSelector, gnome_file_selector)

static void
gnome_icon_entry_class_init (GnomeIconEntryClass *class)
{
    GnomeSelectorClass *selector_class;
    GtkObjectClass *object_class;
    GObjectClass *gobject_class;

    selector_class = (GnomeSelectorClass *) class;
    object_class = (GtkObjectClass *) class;
    gobject_class = (GObjectClass *) class;

    parent_class = gtk_type_class (gnome_file_selector_get_type ());

    object_class->destroy = gnome_icon_entry_destroy;
    gobject_class->finalize = gnome_icon_entry_finalize;

    selector_class->get_uri = get_uri_handler;
    selector_class->set_uri = set_uri_handler;
}

static void
gnome_icon_entry_init (GnomeIconEntry *ientry)
{
    ientry->_priv = g_new0(GnomeIconEntryPrivate, 1);
}

/**
 * gnome_icon_entry_construct:
 * @ientry: Pointer to GnomeIconEntry object.
 * @history_id: If not %NULL, the text id under which history data is stored
 *
 * Constructs a #GnomeIconEntry object, for language bindings or subclassing
 * use #gnome_icon_entry_new from C
 *
 * Returns: 
 */
void
gnome_icon_entry_construct (GnomeIconEntry *ientry, 
			    const gchar *history_id,
			    const gchar *dialog_title)
{
    guint32 flags;

    g_return_if_fail (ientry != NULL);

    flags = GNOME_SELECTOR_DEFAULT_ENTRY_WIDGET |
	GNOME_SELECTOR_DEFAULT_BROWSE_DIALOG;

    gnome_icon_entry_construct_full (ientry, history_id, dialog_title,
				     NULL, NULL, NULL, flags);
}

static gchar *
get_uri_handler (GnomeSelector *selector)
{
    GnomeIconEntry *ientry;

    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_ICON_ENTRY (selector), FALSE);

    ientry = GNOME_ICON_ENTRY (selector);

    return g_strdup (ientry->_priv->current_icon);
}

static void
set_uri_async_done_cb (gpointer data)
{
    GnomeIconEntryAsyncData *async_data = data;
    GnomeIconEntry *ientry;

    /* We're called from gnome_async_handle_unref() during finalizing. */

    g_return_if_fail (async_data != NULL);
    g_assert (GNOME_IS_ICON_ENTRY (async_data->ientry));

    ientry = async_data->ientry;

    g_message (G_STRLOC ": %p", async_data->async_handle);

    /* When the operation was successful, this is already NULL. */
    gnome_gdk_pixbuf_new_from_uri_cancel (async_data->handle);

    /* free the async data. */
    gtk_object_unref (GTK_OBJECT (async_data->ientry));
    g_free (async_data->uri);
    g_free (async_data);
}

static void
set_uri_async_cb (GnomeGdkPixbufAsyncHandle *handle,
		  GnomeVFSResult error, GdkPixbuf *pixbuf,
		  gpointer callback_data)
{
    GnomeIconEntryAsyncData *async_data = callback_data;
    GnomeIconEntry *ientry;
    GtkWidget *child;
    int w, h;

    g_return_if_fail (async_data != NULL);
    g_assert (GNOME_IS_ICON_ENTRY (async_data->ientry));

    ientry = GNOME_ICON_ENTRY (async_data->ientry);

    g_message (G_STRLOC ": %p - `%s'", ientry, async_data->uri);

    if (ientry->_priv->current_icon)
	g_free (ientry->_priv->current_icon);
    ientry->_priv->current_icon = NULL;

    child = GTK_BIN (ientry->_priv->browse_button)->child;

    gtk_drag_source_unset (ientry->_priv->browse_button);

    if ((error != GNOME_VFS_OK) || ((pixbuf == NULL))) {
	if (GNOME_IS_PIXMAP (child)) {
	    gtk_widget_destroy (child);
	    child = gtk_label_new (_("No Icon"));
	    gtk_widget_show (child);
	    gtk_container_add (GTK_CONTAINER (ientry->_priv->browse_button),
			       child);
	}

	_gnome_selector_async_handle_completed (async_data->async_handle, FALSE);

	return;
    }

    ientry->_priv->current_icon = g_strdup (async_data->uri);

    w = gdk_pixbuf_get_width (pixbuf);
    h = gdk_pixbuf_get_height (pixbuf);

    if (w > h) {
	if (w > ICON_SIZE) {
	    h = h * ((double) ICON_SIZE / w);
	    w = ICON_SIZE;
	}
    } else {
	if (h > ICON_SIZE) {
	    w = w * ((double) ICON_SIZE / h);
	    h = ICON_SIZE;
	}
    }

    if (GNOME_IS_PIXMAP (child)) {
	gnome_pixmap_clear (GNOME_PIXMAP (child));
	gnome_pixmap_set_pixbuf (GNOME_PIXMAP(child), pixbuf);
	gnome_pixmap_set_pixbuf_size (GNOME_PIXMAP (child), w, h);
    } else {
	gtk_widget_destroy (child);
	child = gnome_pixmap_new_from_pixbuf_at_size (pixbuf, w, h);
	gtk_widget_show (child);
	gtk_container_add (GTK_CONTAINER (ientry->_priv->browse_button),
			   child);

	if (!GTK_WIDGET_NO_WINDOW (child)) {
	    gtk_signal_connect (GTK_OBJECT (child), "drag_data_get",
				GTK_SIGNAL_FUNC (drag_data_get), ientry);
	    gtk_drag_source_set (child,
				 GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
				 drop_types, 1,
				 GDK_ACTION_COPY);
	}
    }

    gtk_drag_source_set (ientry->_priv->browse_button,
			 GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
			 drop_types, 1,
			 GDK_ACTION_COPY);

    /* This will be freed by our caller; we must set it to NULL here to avoid
     * that add_file_async_done_cb() attempts to free this. */
    async_data->handle = NULL;

    GNOME_CALL_PARENT_HANDLER (GNOME_SELECTOR_CLASS, set_uri,
			       (GNOME_SELECTOR (ientry),
				async_data->uri,
				async_data->async_handle));
}

static void
set_uri_done_cb (GnomeGdkPixbufAsyncHandle *handle,
		 gpointer callback_data)
{
    GnomeIconEntryAsyncData *async_data = callback_data;

    g_return_if_fail (async_data != NULL);

    g_message (G_STRLOC ": %p", async_data->async_handle);

    _gnome_selector_async_handle_remove (async_data->async_handle,
					 async_data);
}

static void
set_uri_handler (GnomeSelector *selector, const gchar *uri,
		 GnomeSelectorAsyncHandle *async_handle)
{
    GnomeIconEntry *ientry;
    GnomeIconEntryAsyncData *async_data;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_ICON_ENTRY (selector));

    ientry = GNOME_ICON_ENTRY (selector);

    async_data = g_new0 (GnomeIconEntryAsyncData, 1);
    async_data->async_handle = async_handle;
    async_data->ientry = ientry;
    async_data->uri = g_strdup (uri);

    gtk_object_ref (GTK_OBJECT (async_data->ientry));

    _gnome_selector_async_handle_add (async_handle, async_data,
				      set_uri_async_done_cb);

    async_data->handle = gnome_gdk_pixbuf_new_from_uri_async
	(uri, set_uri_async_cb, set_uri_done_cb, async_data);
}

static void
show_icon_selection (GnomeIconEntry *ientry)
{
    g_return_if_fail (ientry != NULL);
    g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

    gtk_signal_emit_by_name (GTK_OBJECT (ientry), "browse");
}

static void
browse_dialog_ok_cb (GtkButton *button, GnomeIconEntry *ientry)
{
    GSList *selection;
    gchar *uri;

    g_return_if_fail (ientry != NULL);
    g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

    selection = gnome_selector_get_selection
	(GNOME_SELECTOR (ientry->_priv->icon_selector));
    uri = g_slist_nth_data (selection, 0);

    g_message (G_STRLOC ": `%s'", uri);

    gnome_selector_set_uri (GNOME_SELECTOR (ientry), NULL, uri, NULL, NULL);

    g_slist_free (selection);
}

static void  
drag_data_get (GtkWidget          *widget,
	       GdkDragContext     *context,
	       GtkSelectionData   *selection_data,
	       guint               info,
	       guint               time,
	       GnomeIconEntry     *ientry)
{
    gchar *uri;

    g_return_if_fail (ientry != NULL);
    g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

    uri = gnome_selector_get_uri (GNOME_SELECTOR (ientry));
    if (!uri) {
	gtk_drag_finish (context, FALSE, FALSE, time);
	return;
    }

    gtk_selection_data_set (selection_data,
			    selection_data->target,
			    8, uri, strlen (uri)+1);
    g_free (uri);
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
    GList *files, *li;

    g_return_if_fail (ientry != NULL);
    g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

    /*here we extract the filenames from the URI-list we recieved*/
    files = gnome_uri_list_extract_filenames (selection_data->data);
    /*if there's isn't a file*/
    if (!files) {
	gtk_drag_finish (context, FALSE, FALSE, time);
	return;
    }

    for (li = files; li != NULL; li = li->next) {
	const char *mimetype;

	mimetype = gnome_vfs_mime_type_from_name (li->data);

	if (mimetype && !strcmp (mimetype, "application/x-gnome-app-info")) {
	    /* hmmm a desktop, try loading the icon from that */
	    GnomeDesktopItem *item;
	    const char *icon;

	    item = gnome_desktop_item_new_from_file
		(li->data,
		 GNOME_DESKTOP_ITEM_LOAD_NO_SYNC |
		 GNOME_DESKTOP_ITEM_LOAD_NO_OTHER_SECTIONS);
	    if (!item)
		continue;
	    icon = gnome_desktop_item_get_icon_path (item);

	    gnome_selector_set_uri (GNOME_SELECTOR (ientry), NULL,
				    icon, NULL, NULL);
	    gnome_desktop_item_unref (item);
	    break;
	} else {
	    gnome_selector_set_uri (GNOME_SELECTOR (ientry), NULL,
				    li->data, NULL, NULL);
	    break;
	}
    }

    /*free the list of files we got*/
    gnome_uri_list_free_strings (files);
}

static gboolean
ientry_directory_filter_func (const GnomeVFSFileInfo *info, gpointer data)
{
    GnomeIconEntry *ientry;
    const gchar *mimetype;

    g_return_val_if_fail (data != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_ICON_ENTRY (data), FALSE);

    ientry = GNOME_ICON_ENTRY (data);

    mimetype = gnome_vfs_file_info_get_mime_type ((GnomeVFSFileInfo *) info);

    if (!mimetype || strncmp (mimetype, "image", sizeof("image")-1))
	return FALSE;
    else
	return TRUE;
}


void
gnome_icon_entry_construct_full (GnomeIconEntry *ientry,
				 const gchar *history_id,
				 const gchar *dialog_title,
				 GtkWidget *entry_widget,
				 GtkWidget *selector_widget,
				 GtkWidget *browse_dialog,
				 guint32 flags)
{
    GnomeVFSDirectoryFilter *filter;
    guint32 newflags = flags;

    g_return_if_fail (ientry != NULL);

    /* Create the default selector widget if requested. */
    if (flags & GNOME_SELECTOR_DEFAULT_BROWSE_DIALOG) {
	if (browse_dialog != NULL) {
	    g_warning (G_STRLOC ": It makes no sense to use "
		       "GNOME_SELECTOR_DEFAULT_BROWSE_DIALOG "
		       "and pass a `browse_dialog' as well.");
	    return;
	}

	browse_dialog = gnome_dialog_new (dialog_title,
					  GNOME_STOCK_BUTTON_OK,
					  GNOME_STOCK_BUTTON_CANCEL,
					  NULL);

	ientry->_priv->browse_dialog = browse_dialog;

	gnome_dialog_close_hides (GNOME_DIALOG (browse_dialog), TRUE);
	gnome_dialog_set_close (GNOME_DIALOG (browse_dialog), TRUE);

	gtk_window_set_policy (GTK_WINDOW (browse_dialog),  TRUE, TRUE, TRUE);

	gnome_dialog_button_connect (GNOME_DIALOG (browse_dialog),
				     0, /* OK button */
				     GTK_SIGNAL_FUNC (browse_dialog_ok_cb),
				     ientry);

	ientry->_priv->icon_selector = gnome_icon_selector_new
	    (history_id, dialog_title, 0);

	gnome_icon_selector_add_defaults
	    (GNOME_ICON_SELECTOR (ientry->_priv->icon_selector));

	gnome_selector_set_to_defaults
	    (GNOME_SELECTOR (ientry->_priv->icon_selector));

	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (browse_dialog)->vbox),
			    ientry->_priv->icon_selector, TRUE, TRUE, 0);

	gtk_widget_show_all (ientry->_priv->icon_selector);

	newflags &= ~GNOME_SELECTOR_DEFAULT_BROWSE_DIALOG;
    }

    /* Create the default selector widget if requested. */
    if (flags & GNOME_SELECTOR_DEFAULT_ENTRY_WIDGET) {
	GtkWidget *w;

	if (entry_widget != NULL) {
	    g_warning (G_STRLOC ": It makes no sense to use "
		       "GNOME_SELECTOR_DEFAULT_ENTRY_WIDGET "
		       "and pass a `entry_widget' as well.");
	    return;
	}

	entry_widget = gtk_hbox_new (FALSE, 4);

	w = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_box_pack_start (GTK_BOX (entry_widget), w, TRUE, FALSE, 0);

	ientry->_priv->browse_button = gtk_button_new_with_label (_("No Icon"));
	gtk_drag_dest_set (GTK_WIDGET (ientry->_priv->browse_button),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drop_types, 1, GDK_ACTION_COPY);
	gtk_signal_connect (GTK_OBJECT (ientry->_priv->browse_button),
			    "drag_data_received",
			    GTK_SIGNAL_FUNC (drag_data_received), ientry);
	gtk_signal_connect (GTK_OBJECT (ientry->_priv->browse_button),
			    "drag_data_get", GTK_SIGNAL_FUNC (drag_data_get),
			    ientry);
	gtk_signal_connect_object (GTK_OBJECT (ientry->_priv->browse_button),
				   "clicked", show_icon_selection,
				   GTK_OBJECT (ientry));

	/*FIXME: 60x60 is just larger then default 48x48, though icon sizes
	  are supposed to be selectable I guess*/
	gtk_widget_set_usize (ientry->_priv->browse_button,
			      ICON_SIZE + 12, ICON_SIZE + 12);
	gtk_container_add (GTK_CONTAINER (w), ientry->_priv->browse_button);
	gtk_widget_show_all (ientry->_priv->browse_button);

	newflags &= ~GNOME_SELECTOR_DEFAULT_ENTRY_WIDGET;
    }

    gnome_file_selector_construct (GNOME_FILE_SELECTOR (ientry),
				   history_id, dialog_title,
				   entry_widget, selector_widget,
				   browse_dialog, newflags);

    filter = gnome_vfs_directory_filter_new_custom
	(ientry_directory_filter_func,
	 GNOME_VFS_DIRECTORY_FILTER_NEEDS_NAME |
	 GNOME_VFS_DIRECTORY_FILTER_NEEDS_TYPE |
	 GNOME_VFS_DIRECTORY_FILTER_NEEDS_MIMETYPE,
	 ientry);

    gnome_file_selector_set_filter (GNOME_FILE_SELECTOR (ientry), filter);
}


/**
 * gnome_icon_entry_new
 * @history_id: If not %NULL, the text id under which history data is stored
 *
 * Description: Creates a new GnomeIconEntry widget.  If  @history_id is
 * not %NULL, then the history list will be saved and restored between
 * uses under the given id.
 *
 * Returns: Newly-created GnomeIconEntry widget.
 */
GtkWidget *
gnome_icon_entry_new (const gchar *history_id, const gchar *dialog_title)
{
    GnomeIconEntry *ientry;

    ientry = gtk_type_new (gnome_icon_entry_get_type ());

    gnome_icon_entry_construct (ientry, history_id, dialog_title);

    return GTK_WIDGET (ientry);
}

static void
gnome_icon_entry_destroy (GtkObject *object)
{
    GnomeIconEntry *ientry;

    /* remember, destroy can be run multiple times! */

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_ICON_ENTRY (object));

    ientry = GNOME_ICON_ENTRY (object);

    if (ientry->_priv) {
	if (ientry->_priv->browse_dialog)
	    gtk_object_unref (GTK_OBJECT (ientry->_priv->browse_dialog));
	ientry->_priv->browse_dialog = NULL;
	ientry->_priv->icon_selector = NULL;

	if (ientry->_priv->browse_button) {
	    GtkWidget *child;

	    child = GTK_BIN (ientry->_priv->browse_button)->child;
	    gtk_widget_destroy (child);
	}
	ientry->_priv->browse_button = NULL;

	g_free (ientry->_priv->current_icon);
	ientry->_priv->current_icon = NULL;
    }

    GNOME_CALL_PARENT_HANDLER (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gnome_icon_entry_finalize (GObject *object)
{
    GnomeIconEntry *ientry;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_ICON_ENTRY (object));

    ientry = GNOME_ICON_ENTRY (object);

    g_free (ientry->_priv);
    ientry->_priv = NULL;

    GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, finalize, (object));
}

/**
 * gnome_icon_entry_get_icon_selector:
 * @ientry: the GnomeIconEntry to work with
 * 
 * Description: Returns the GnomeIconSelector object which is used
 * in the browse dialog.  You must gtk_object_unref the return value
 * when you're done with it.
 *
 * Returns: The GnomeIconSelector widget.
 **/
GtkWidget *
gnome_icon_entry_get_icon_selector (GnomeIconEntry *ientry)
{
    g_return_val_if_fail (ientry != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry), NULL);

    gtk_object_ref (GTK_OBJECT (ientry->_priv->icon_selector));
    return ientry->_priv->icon_selector;
}


/**
 * gnome_icon_entry_set_pixmap_subdir:
 * @ientry: the GnomeIconEntry to work with
 * @subdir: subdirectory
 *
 * Description: Sets the subdirectory below gnome's default
 * pixmap directory to use as the default path for the file
 * entry.  The path can also be an absolute one.  If %NULL is passed
 * then the pixmap directory itself is used.
 *
 * Returns:
 **/
void
gnome_icon_entry_set_pixmap_subdir (GnomeIconEntry *ientry,
				    const gchar *subdir)
{
    g_return_if_fail (ientry != NULL);
    g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

    g_warning (G_STRLOC ": this function is deprecated and has "
	       "no effect at all.");
}

/**
 * gnome_icon_entry_set_filename:
 * @ientry: the GnomeIconEntry to work with
 * @filename: a filename
 * 
 * Description: Sets the icon of GnomeIconEntry to be the one pointed to by
 * @filename (in the current subdirectory).
 *
 * Returns: %TRUE if icon was loaded ok, %FALSE otherwise
 **/
gboolean
gnome_icon_entry_set_filename (GnomeIconEntry *ientry,
			       const gchar *filename)
{
	g_return_val_if_fail (ientry != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry), FALSE);

	g_warning (G_STRLOC ": this function is deprecated; use "
		   "gnome_selector_set_uri instead.");

	gnome_selector_set_uri (GNOME_SELECTOR (ientry), NULL,
				filename, NULL, NULL);

	return FALSE;
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
gnome_icon_entry_get_filename (GnomeIconEntry *ientry)
{
	g_return_val_if_fail (ientry != NULL,NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry),NULL);

	g_warning (G_STRLOC ": this function is deprecated; use "
		   "gnome_selector_get_uri instead.");

	return gnome_selector_get_uri (GNOME_SELECTOR (ientry));
}

