/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * gnome-window-icon.c: convenience functions for window mini-icons
 *
 * Copyright 2000 Helix Code, Inc.
 *
 * Author:  Jacob Berkman  <jacob@helixcode.com>
 */

#include "config.h"

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <stdlib.h>

#include "libgnomeui/gnome-client.h"
#include "libgnomeui/gnome-window-icon.h"

#define GNOME_DESKTOP_ICON "GNOME_DESKTOP_ICON"

#define WINHINT_KEY "gnome-libs.WM_HINTS.icon_info"
typedef struct {
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	GdkWindow *window;
} IconData;

static IconData default_icon = { NULL };

static void
icon_unref (IconData *icon)
{
	g_return_if_fail (icon);
	g_return_if_fail (icon->pixmap);

	gdk_pixmap_unref (icon->pixmap);
	if (icon->mask)
		gdk_bitmap_unref (icon->mask);

	icon->pixmap = icon->mask = NULL;
}

static void
window_realized (GtkWindow *w, IconData *data)
{
	gdk_window_set_icon (GTK_WIDGET (w)->window, data->window, data->pixmap, data->mask);
}

static void
window_destroyed (GtkWindow *w, IconData *data)
{
	if (data->pixmap != default_icon.pixmap)
		icon_unref (data);

	if (data->window)
		gdk_window_unref (data->window);

	g_free (data);
}

#if 0
static gint
icon_window_expose (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	IconData *icon_data = data;
	GdkRectangle *area;
	GdkGC *gc;

	g_return_val_if_fail (icon_data != NULL, FALSE);
	g_return_val_if_fail (icon_data->window != NULL, FALSE);

	area = &((GdkEventExpose *)event)->area;
	
	if (!icon_data->pixmap) {
		gdk_window_clear_area (icon_data->window,
				       area->x, area->y,
				       area->width, area->height);
		return FALSE;
	}

	gc = gdk_gc_new (icon_data->window);

	if (icon_data->mask)
		gdk_gc_set_clip_mask (gc, icon_data->mask);
	
	gdk_draw_pixmap (icon_data->window, gc,
			 icon_data->pixmap,
			 0, 0, 0, 0, -1, -1);

	if (icon_data->mask)
		gdk_gc_set_clip_mask (gc, NULL);

	gdk_gc_unref (gc);

	return FALSE;
}
#endif

static void
icon_set (GtkWindow *w, IconData *icon_data)
{
	IconData *idata = NULL;
	gboolean do_connect = TRUE;

	g_return_if_fail (w != NULL);
	g_return_if_fail (GTK_IS_WINDOW (w));

	g_return_if_fail (icon_data != NULL);

	if (!icon_data->pixmap)
		return;

	idata = gtk_object_get_data (GTK_OBJECT (w), WINHINT_KEY);
	if (idata) {
		do_connect = FALSE;
		if (idata->pixmap != default_icon.pixmap)
			icon_unref (idata);
	}
	else 
		idata = g_malloc (sizeof (IconData));

	*idata = *icon_data;

#if 0
	if (!idata->window) {
		GdkWindowAttr wa;
		gint height, width;
		/*
		 * According to ICCCM, we should be using the root window's visual and
		 * the default colormap.  This does not work very well on multi-depth
		 * machines, so we just use imlib's visual and colormap, which seems to
		 * work most places.
		 *
		 * See http://www.tronche.com/gui/x/icccm/sec-4.html#s-4.1.9
		 */
		gdk_window_get_size (idata->pixmap, &width, &height);
		wa.visual      = gdk_imlib_get_visual   (); /* gdk_window_get_visual (GDK_ROOT_PARENT ()); */
		wa.colormap    = gdk_imlib_get_colormap (); /* gdk_colormap_get_system (); */
		wa.window_type = GDK_WINDOW_CHILD;
		wa.wclass      = GDK_INPUT_OUTPUT;
		wa.event_mask  = GDK_EXPOSURE_MASK;
		wa.width       = width;
		wa.height      = height;

		idata->window = gdk_window_new (GDK_ROOT_PARENT (), &wa, 
						GDK_WA_VISUAL | GDK_WA_COLORMAP);

		gdk_window_add_filter (idata->window, icon_window_expose, idata);
	}

	if (idata->mask)
		gdk_window_shape_combine_mask (idata->window, idata->mask, 0, 0);
#endif
	if (do_connect) {
		gtk_signal_connect_after (GTK_OBJECT (w), "realize",
					  GTK_SIGNAL_FUNC (window_realized),
					  idata);
		gtk_signal_connect (GTK_OBJECT (w), "destroy",
				    GTK_SIGNAL_FUNC (window_destroyed),
				    idata);
	}

	if (GTK_WIDGET_REALIZED (w))
		window_realized (w, idata);
				    
}

static void
load_icon (GdkImlibImage *im, IconData *icon_data, gboolean destroy)
{
	if (!im)
		return;

	gdk_imlib_render (im, im->rgb_width, im->rgb_height);
	
	icon_data->pixmap = gdk_imlib_copy_image (im);
	icon_data->mask   = gdk_imlib_copy_mask  (im);

	/*gdk_window_set_colormap (icon_data->pixmap, gdk_colormap_get_system ());*/

	if (destroy)
		gdk_imlib_destroy_image (im);
}

/**
 * gnome_window_icon_set_from_default:
 * @w: the #GtkWidget to set the icon on
 *
 * Description: Makes the #GtkWindow @w use the default icon.
 */
void
gnome_window_icon_set_from_default (GtkWindow *w)
{
	icon_set (w, &default_icon);
}

/**
 * gnome_window_icon_set_from_file:
 * @w: the #GtkWidget to set the icon on
 * @filename: the name of the file to load
 *
 * Description: Makes the #GtkWindow @w use the icon in @filename.
 */
void
gnome_window_icon_set_from_file (GtkWindow *w, const char *filename)
{
	IconData icon_data = { NULL };
	load_icon (gdk_imlib_load_image ((char *)filename),
		     &icon_data, TRUE);
	icon_set (w, &icon_data);
}

/**
 * gnome_window_icon_set_from_imlib:
 * @w: the #GtkWidget to set the icon on
 * @im: the imlib image to use for the icon
 * 
 * Description: Makes the #GtkWindow @w use the icon in @im.
 */
void
gnome_window_icon_set_from_imlib (GtkWindow *w, GdkImlibImage *im)
{
	IconData icon_data = { NULL };
	load_icon (im, &icon_data, FALSE);
	icon_set (w, &icon_data);
}

/**
 * gnome_window_icon_set_default_from_file:
 * @filename: filename for the default window icon
 *
 * Description: Set the default icon to the image in @filename, if one 
 * of the gnome_window_icon_set_default_from* functions have not already
 * been called.
 */
void
gnome_window_icon_set_default_from_file (const char *filename)
{	
	if (default_icon.pixmap)
		return;
	load_icon (gdk_imlib_load_image ((char *)filename),
		   &default_icon, TRUE);
}

/** 
 * gnome_window_icon_set_default_from_imlib:
 * @im: imlib image for the default icon 
 *
 * Description: Set the default icon to the image in @im, if one of
 * the gnome_window_icon_set_default_from* functions have not already
 * been called.
 */
void
gnome_window_icon_set_default_from_imlib (GdkImlibImage *im)
{
	if (default_icon.pixmap)
		return;
	load_icon (im, &default_icon, FALSE);
}

/**
 * gnome_window_icon_init:
 *
 * Description: Initialize the gnome window icon by checking the
 * GNOME_DESKTOP_ICON environment variable.  This function is 
 * automatically called by the gnome_init process.
 */
void
gnome_window_icon_init ()
{
	GnomeClient *client;
	char *filename;

	filename = getenv (GNOME_DESKTOP_ICON);
        if (!filename || !filename[0])
		return;

	gnome_window_icon_set_default_from_file (filename);

	/* remove it from our environment */
	putenv (GNOME_DESKTOP_ICON);

	client = gnome_master_client ();
	if (!GNOME_CLIENT_CONNECTED (client))
		return;
	
	/* save it for restarts */
	gnome_client_set_environment (client, GNOME_DESKTOP_ICON, filename);
}
