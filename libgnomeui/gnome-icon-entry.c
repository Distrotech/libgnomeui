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
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkalignment.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-ditem.h"
#include "gnome-i18nP.h"
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
    GtkWidget *browse_dialog;
    GtkWidget *icon_selector;

    GtkWidget *preview;
    GtkWidget *preview_container;

    gchar *current_icon;

    gboolean constructed;

    gboolean is_pixmap_entry;
    guint preview_x, preview_y;
};
	
enum {
    PROP_0,

    /* Construction properties */
    PROP_IS_PIXMAP_ENTRY,

    /* Normal properties */
    PROP_PREVIEW_X,
    PROP_PREVIEW_Y
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
static void   gnome_icon_entry_get_property(GObject             *object,
					    guint                param_id,
					    GValue              *value,
					    GParamSpec          *pspec);
static void   gnome_icon_entry_set_property(GObject             *object,
					    guint                param_id,
					    const GValue        *value,
					    GParamSpec          *pspec);



static gchar    *get_uri_handler           (GnomeSelector            *selector);
static void      set_uri_handler           (GnomeSelector            *selector,
                                            const gchar              *filename,
                                            GnomeSelectorAsyncHandle *async_handle);
static void      do_construct_handler      (GnomeSelector            *selector);


static GObject*
gnome_icon_entry_constructor (GType                  type,
			      guint                  n_construct_properties,
			      GObjectConstructParam *construct_properties);


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

    gobject_class->get_property = gnome_icon_entry_get_property;
    gobject_class->set_property = gnome_icon_entry_set_property;

    /* Construction properties */
    g_object_class_install_property
	(gobject_class,
	 PROP_IS_PIXMAP_ENTRY,
	 g_param_spec_boolean ("is_pixmap_entry", NULL, NULL,
			       FALSE,
			       (G_PARAM_READABLE | G_PARAM_WRITABLE |
				G_PARAM_CONSTRUCT_ONLY)));

    /* Normal properties */
    g_object_class_install_property
	(gobject_class,
	 PROP_PREVIEW_X,
	 g_param_spec_uint ("preview_x", NULL, NULL,
			    0, G_MAXINT, ICON_SIZE,
			    (G_PARAM_READABLE | G_PARAM_WRITABLE |
			     G_PARAM_CONSTRUCT)));
    g_object_class_install_property
	(gobject_class,
	 PROP_PREVIEW_Y,
	 g_param_spec_uint ("preview_y", NULL, NULL,
			    0, G_MAXINT, ICON_SIZE,
			    (G_PARAM_READABLE | G_PARAM_WRITABLE |
			     G_PARAM_CONSTRUCT)));

    object_class->destroy = gnome_icon_entry_destroy;
    gobject_class->finalize = gnome_icon_entry_finalize;

    gobject_class->constructor = gnome_icon_entry_constructor;

    selector_class->get_uri = get_uri_handler;
    selector_class->set_uri = set_uri_handler;

    selector_class->do_construct = do_construct_handler;
}

static void
gnome_icon_entry_update_preview (GnomeIconEntry *ientry)
{
    g_return_if_fail (ientry != NULL);
    g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

    if (ientry->_priv->preview == NULL) {
	g_warning (G_STRLOC ": Can't change preview size if we aren't using "
		   " the default browse button.");
	return;
    }

    gtk_widget_set_usize (ientry->_priv->preview,
			  ientry->_priv->preview_x + 12,
			  ientry->_priv->preview_y + 12);
}

static void
gnome_icon_entry_set_property (GObject *object, guint param_id,
			       const GValue *value, GParamSpec *pspec)
{
    GnomeIconEntry *ientry;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_ICON_ENTRY (object));

    ientry = GNOME_ICON_ENTRY (object);

    switch (param_id) {
    case PROP_IS_PIXMAP_ENTRY:
	g_assert (!ientry->_priv->constructed);
	ientry->_priv->is_pixmap_entry = g_value_get_boolean (value);
	break;
    case PROP_PREVIEW_X:
	ientry->_priv->preview_x = g_value_get_uint (value);
	if (ientry->_priv->constructed)
	    gnome_icon_entry_update_preview (ientry);
	break;
    case PROP_PREVIEW_Y:
	ientry->_priv->preview_y = g_value_get_uint (value);
	if (ientry->_priv->constructed)
	    gnome_icon_entry_update_preview (ientry);
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	break;
    }
}

static void
gnome_icon_entry_get_property (GObject *object, guint param_id, GValue *value,
			       GParamSpec *pspec)
{
    GnomeIconEntry *ientry;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_ICON_ENTRY (object));

    ientry = GNOME_ICON_ENTRY (object);

    switch (param_id) {
    case PROP_IS_PIXMAP_ENTRY:
	g_value_set_boolean (value, ientry->_priv->is_pixmap_entry);
	break;
    case PROP_PREVIEW_X:
	g_value_set_uint (value, ientry->_priv->preview_x);
	break;
    case PROP_PREVIEW_Y:
	g_value_set_uint (value, ientry->_priv->preview_y);
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	break;
    }
}

static void
gnome_icon_entry_init (GnomeIconEntry *ientry)
{
    ientry->_priv = g_new0(GnomeIconEntryPrivate, 1);

    g_message (G_STRLOC);
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
    int w, h;

    g_return_if_fail (async_data != NULL);
    g_assert (GNOME_IS_ICON_ENTRY (async_data->ientry));

    ientry = GNOME_ICON_ENTRY (async_data->ientry);

    g_message (G_STRLOC ": %p - `%s'", ientry, async_data->uri);

    if (ientry->_priv->current_icon)
	g_free (ientry->_priv->current_icon);
    ientry->_priv->current_icon = NULL;

    gtk_drag_source_unset (ientry->_priv->preview);

    if ((error != GNOME_VFS_OK) || ((pixbuf == NULL))) {
	if (GNOME_IS_PIXMAP (ientry->_priv->preview)) {
	    gtk_widget_destroy (ientry->_priv->preview);
	    ientry->_priv->preview = gtk_label_new (_("No Icon"));
	    gtk_widget_show (ientry->_priv->preview);
	    gtk_container_add (GTK_CONTAINER (ientry->_priv->preview_container),
			       ientry->_priv->preview);
	}

	_gnome_selector_async_handle_completed (async_data->async_handle, FALSE);

	return;
    }

    ientry->_priv->current_icon = g_strdup (async_data->uri);

    w = gdk_pixbuf_get_width (pixbuf);
    h = gdk_pixbuf_get_height (pixbuf);

    if (!ientry->_priv->is_pixmap_entry) {
	if (w > h) {
	    if (w > ientry->_priv->preview_x) {
		h = h * ((double) ientry->_priv->preview_x / w);
		w = ientry->_priv->preview_x;
	    }
	} else {
	    if (h > ientry->_priv->preview_y) {
		w = w * ((double) ientry->_priv->preview_y / h);
		h = ientry->_priv->preview_y;
	    }
	}
    }

    if (GNOME_IS_PIXMAP (ientry->_priv->preview)) {
	gnome_pixmap_clear (GNOME_PIXMAP (ientry->_priv->preview));
	gnome_pixmap_set_pixbuf (GNOME_PIXMAP (ientry->_priv->preview), pixbuf);
	gnome_pixmap_set_pixbuf_size (GNOME_PIXMAP (ientry->_priv->preview), w, h);
    } else {
	gtk_widget_destroy (ientry->_priv->preview);
	ientry->_priv->preview = gnome_pixmap_new_from_pixbuf_at_size (pixbuf, w, h);
	gtk_widget_show (ientry->_priv->preview);
	gtk_container_add (GTK_CONTAINER (ientry->_priv->preview_container),
			   ientry->_priv->preview);

	if (!GTK_WIDGET_NO_WINDOW (ientry->_priv->preview)) {
	    gtk_signal_connect (GTK_OBJECT (ientry->_priv->preview),
				"drag_data_get",
				GTK_SIGNAL_FUNC (drag_data_get), ientry);
	    gtk_drag_source_set (ientry->_priv->preview,
				 GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
				 drop_types, 1,
				 GDK_ACTION_COPY);
	}
    }

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

static gboolean
get_value_boolean (GnomeIconEntry *ientry, const gchar *prop_name)
{
    GValue value = { 0, };
    gboolean retval;

    g_value_init (&value, G_TYPE_BOOLEAN);
    g_object_get_property (G_OBJECT (ientry), prop_name, &value);
    retval = g_value_get_boolean (&value);
    g_value_unset (&value);

    return retval;
}

static gboolean
has_value_widget (GnomeIconEntry *ientry, const gchar *prop_name)
{
	GValue value = { 0, };
	gboolean retval;

	g_value_init (&value, GTK_TYPE_WIDGET);
	g_object_get_property (G_OBJECT (ientry), prop_name, &value);
	retval = g_value_get_object (&value) != NULL;
	g_value_unset (&value);

	return retval;
}

static void
do_construct_handler (GnomeSelector *selector)
{
    GnomeIconEntry *ientry;
    GnomeVFSDirectoryFilter *filter;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_ICON_ENTRY (selector));

    ientry = GNOME_ICON_ENTRY (selector);

    if (get_value_boolean (ientry, "want_default_behaviour")) {
	if (ientry->_priv->is_pixmap_entry)
	    g_object_set (G_OBJECT (ientry),
			  "want_default_behaviour", FALSE,
			  "use_default_entry_widget", TRUE,
			  "use_default_selector_widget", TRUE,
			  "use_default_browse_dialog", TRUE,
			  "want_browse_button", TRUE,
			  "want_clear_button", FALSE,
			  "want_default_button", FALSE,
			  NULL);
	else
	    g_object_set (G_OBJECT (ientry),
			  "want_default_behaviour", FALSE,
			  "use_default_entry_widget", FALSE,
			  "use_default_selector_widget", TRUE,
			  "use_default_browse_dialog", TRUE,
			  "want_browse_button", FALSE,
			  "want_clear_button", FALSE,
			  "want_default_button", FALSE,
			  NULL);
    }

    /* Create the default browser dialog if requested. */
    if (get_value_boolean (ientry, "use_default_browse_dialog") &&
	!has_value_widget (ientry, "browse_dialog") &&
	!ientry->_priv->is_pixmap_entry) {
	GtkWidget *browse_dialog;
	gchar *dialog_title, *history_id;
	GValue value = { 0, };

	g_message (G_STRLOC ": default browse dialog");

	g_value_init (&value, G_TYPE_STRING);
	g_object_get_property (G_OBJECT (ientry), "dialog_title", &value);
	dialog_title = g_value_dup_string (&value);
	g_value_unset (&value);

	g_value_init (&value, G_TYPE_STRING);
	g_object_get_property (G_OBJECT (ientry), "history_id", &value);
	history_id = g_value_dup_string (&value);
	g_value_unset (&value);

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
	    (history_id, dialog_title);

	gnome_icon_selector_add_defaults
	    (GNOME_ICON_SELECTOR (ientry->_priv->icon_selector));

#if 0
	gnome_selector_set_to_defaults
	    (GNOME_SELECTOR (ientry->_priv->icon_selector));
#endif

	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (browse_dialog)->vbox),
			    ientry->_priv->icon_selector, TRUE, TRUE, 0);

	gtk_widget_show_all (ientry->_priv->icon_selector);

	g_value_init (&value, GTK_TYPE_WIDGET);
	g_value_set_object (&value, G_OBJECT (browse_dialog));
	g_object_set_property (G_OBJECT (ientry), "browse_dialog", &value);
	g_value_unset (&value);

	g_free (dialog_title);
	g_free (history_id);
    }

    /* Create the default entry widget if requested. */
    if (get_value_boolean (ientry, "use_default_selector_widget") &&
	!has_value_widget (ientry, "selector_widget")) {
	GtkWidget *selector_widget, *w;
	GValue value = { 0, };

	g_message (G_STRLOC ": default selector widget");

	if (ientry->_priv->is_pixmap_entry) {
	    selector_widget = gtk_scrolled_window_new (NULL, NULL);
	    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (selector_widget),
					    GTK_POLICY_AUTOMATIC,
					    GTK_POLICY_AUTOMATIC);

	    ientry->_priv->preview = gtk_label_new (_("No Icon"));

	    gtk_scrolled_window_add_with_viewport
		(GTK_SCROLLED_WINDOW (selector_widget),
		 ientry->_priv->preview);

	    ientry->_priv->preview_container = ientry->_priv->preview->parent;

	    gtk_widget_set_usize (selector_widget,
				  ientry->_priv->preview_x + 12,
				  ientry->_priv->preview_y + 12);
	} else {
	    selector_widget = gtk_hbox_new (FALSE, 4);

	    w = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	    gtk_box_pack_start (GTK_BOX (selector_widget), w, TRUE, FALSE, 0);

	    ientry->_priv->preview_container = gtk_button_new_with_label (_("No Icon"));
	    ientry->_priv->preview = GTK_BIN (ientry->_priv->preview_container)->child;

	    gtk_widget_set_usize (ientry->_priv->preview,
				  ientry->_priv->preview_x + 12,
				  ientry->_priv->preview_y + 12);

	    gtk_container_add (GTK_CONTAINER (w),
			       ientry->_priv->preview_container);
	}

	gtk_drag_dest_set (GTK_WIDGET (ientry->_priv->preview),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drop_types, 1, GDK_ACTION_COPY);
	gtk_signal_connect (GTK_OBJECT (ientry->_priv->preview),
			    "drag_data_received",
			    GTK_SIGNAL_FUNC (drag_data_received), ientry);
	gtk_signal_connect (GTK_OBJECT (ientry->_priv->preview),
			    "drag_data_get", GTK_SIGNAL_FUNC (drag_data_get),
			    ientry);

	if (!ientry->_priv->is_pixmap_entry)
	    gtk_signal_connect_object
		(GTK_OBJECT (ientry->_priv->preview_container),
		 "clicked", GTK_SIGNAL_FUNC (show_icon_selection),
		 GTK_OBJECT (ientry));

	gtk_widget_ref (ientry->_priv->preview_container);

	gtk_widget_show_all (selector_widget);

	g_value_init (&value, GTK_TYPE_WIDGET);
	g_value_set_object (&value, G_OBJECT (selector_widget));
	g_object_set_property (G_OBJECT (ientry), "selector_widget", &value);
	g_value_unset (&value);
    }

    GNOME_CALL_PARENT_HANDLER (GNOME_SELECTOR_CLASS, do_construct, (selector));

    filter = gnome_vfs_directory_filter_new_custom
	(ientry_directory_filter_func,
	 GNOME_VFS_DIRECTORY_FILTER_NEEDS_NAME |
	 GNOME_VFS_DIRECTORY_FILTER_NEEDS_TYPE |
	 GNOME_VFS_DIRECTORY_FILTER_NEEDS_MIMETYPE,
	 ientry);

    gnome_file_selector_set_filter (GNOME_FILE_SELECTOR (ientry), filter);

    g_message (G_STRLOC);
}

static GObject*
gnome_icon_entry_constructor (GType                  type,
			      guint                  n_construct_properties,
			      GObjectConstructParam *construct_properties)
{
    GObject *object = G_OBJECT_CLASS (parent_class)->constructor (type,
								  n_construct_properties,
								  construct_properties);
    GnomeIconEntry *ientry = GNOME_ICON_ENTRY (object);

    g_message (G_STRLOC ": %d - %d", type, GNOME_TYPE_ICON_ENTRY);

    if (type == GNOME_TYPE_ICON_ENTRY)
	gnome_selector_do_construct (GNOME_SELECTOR (ientry));

    ientry->_priv->constructed = TRUE;

    g_message (G_STRLOC);

    return object;
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

    ientry = g_object_new (gnome_icon_entry_get_type (),
			   "history_id", history_id,
			   "dialog_title", dialog_title,
			   NULL);

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

	if (ientry->_priv->preview_container)
	    gtk_widget_unref (ientry->_priv->preview_container);
	ientry->_priv->preview_container = NULL;

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

void
gnome_icon_entry_set_preview_size (GnomeIconEntry *ientry,
				   guint preview_x, guint preview_y)
{
    g_return_if_fail (ientry != NULL);
    g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

    ientry->_priv->preview_x = preview_x;
    ientry->_priv->preview_y = preview_y;

    gnome_icon_entry_update_preview (ientry);
}
