/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*-

   Copyright (C) 1999 Red Hat, Inc.
   All rights reserved.
    
   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   GnomePixmap Developers: Havoc Pennington, Jonathan Blandford
*/
/*
  @NOTATION@
*/

#include <config.h>
#include "gnome-macros.h"

#ifndef GNOME_DISABLE_DEPRECATED_SOURCE

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gnome-pixmap.h"


static void gnome_pixmap_class_init    (GnomePixmapClass *class);
static void gnome_pixmap_init          (GnomePixmap      *gpixmap);

/* Not used currently */
struct _GnomePixmapPrivate {
	int dummy;
};

/**
 * gnome_pixmap_get_type:
 *
 * Registers the &GnomePixmap class if necessary, and returns the type ID
 * associated to it.
 *
 * Returns: the type ID of the &GnomePixmap class.
 */
GNOME_CLASS_BOILERPLATE (GnomePixmap, gnome_pixmap,
			 GtkImage, gtk_image)

/*
 * Widget functions
 */

static void
gnome_pixmap_init (GnomePixmap *gpixmap)
{
}

static void
gnome_pixmap_class_init (GnomePixmapClass *class)
{
}



/*
 * Public functions
 */


/**
 * gnome_pixmap_new_from_file:
 * @filename: The filename of the file to be loaded.
 *
 * Note that the new_from_file functions give you no way to detect errors;
 * if the file isn't found/loaded, you get an empty widget.
 * to detect errors just do:
 *
 * <programlisting>
 * pixbuf = gdk_pixbuf_new_from_file (filename);
 * if (pixbuf != NULL) {
 *         gpixmap = gtk_image_new_from_pixbuf (pixbuf);
 * } else {
 *         // handle your error...
 * }
 * </programlisting>
 * 
 * Return value: A newly allocated @GnomePixmap with the file at @filename loaded.
 **/
GtkWidget*
gnome_pixmap_new_from_file (const char *filename)
{
	GtkWidget *retval = gtk_type_new (gnome_pixmap_get_type ());
	gtk_image_set_from_file (GTK_IMAGE (retval), filename);
	return retval;
}

/**
 * gnome_pixmap_new_from_file_at_size:
 * @filename: The filename of the file to be loaded.
 * @width: The width to scale the image to.
 * @height: The height to scale the image to.
 *
 * Loads a new @GnomePixmap from a file, and scales it (if necessary) to the
 * size indicated by @height and @width.  If either are set to -1, then the
 * "natural" dimension of the image is used in place.  See
 * @gnome_pixmap_new_from_file for information on error handling.
 *
 * Return value: value: A newly allocated @GnomePixmap with the file at @filename loaded.
 **/
GtkWidget*
gnome_pixmap_new_from_file_at_size (const gchar *filename, gint width, gint height)
{
	GtkWidget *retval = NULL;
	GdkPixbuf *pixbuf;
        GError *error;

	g_return_val_if_fail (filename != NULL, NULL);

        error = NULL;
	pixbuf = gdk_pixbuf_new_from_file (filename, &error);
        if (error != NULL) {
                g_warning (G_STRLOC ": cannot open %s: %s",
                           filename, error->message);
                g_error_free (error);
        }
	if (pixbuf != NULL) {
		GdkPixbuf *scaled = gdk_pixbuf_scale_simple (pixbuf,
							     width,
							     height,
							     GDK_INTERP_BILINEAR);
		retval = gtk_type_new (gnome_pixmap_get_type ());
		gtk_image_set_from_pixbuf (GTK_IMAGE (retval), scaled);
		gdk_pixbuf_unref (scaled);
		gdk_pixbuf_unref (pixbuf);
	} else {
		retval = gtk_type_new (gnome_pixmap_get_type ());
	}
	return retval;
}

/**
 * gnome_pixmap_new_from_xpm_d:
 * @xpm_data: The xpm data to be loaded.
 *
 * Loads a new @GnomePixmap from the @xpm_data, and scales it (if necessary) to
 * the size indicated by @height and @width.  If either are set to -1, then the
 * "natural" dimension of the image is used in place.
 *
 * Return value: value: A newly allocated @GnomePixmap with the image from @xpm_data loaded.
 **/
GtkWidget*
gnome_pixmap_new_from_xpm_d (const char **xpm_data)
{
	GtkWidget *retval = NULL;
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (xpm_data != NULL, NULL);

	pixbuf = gdk_pixbuf_new_from_xpm_data (xpm_data);
	if (pixbuf != NULL) {
		retval = gtk_type_new (gnome_pixmap_get_type ());
		gtk_image_set_from_pixbuf (GTK_IMAGE (retval), pixbuf);
		gdk_pixbuf_unref (pixbuf);
	} else {
		retval = gtk_type_new (gnome_pixmap_get_type ());
	}
	return retval;
}

/**
 * gnome_pixmap_new_from_xpm_d_at_size:
 * @xpm_data: The xpm data to be loaded.
 * @width: The width to scale the image to.
 * @height: The height to scale the image to.
 *
 * Loads a new @GnomePixmap from the @xpm_data, and scales it (if necessary) to
 * the size indicated by @height and @width.  If either are set to -1, then the
 * "natural" dimension of the image is used in place.
 *
 * Return value: value: A newly allocated @GnomePixmap with the image from @xpm_data loaded.
 **/
GtkWidget*
gnome_pixmap_new_from_xpm_d_at_size (const char **xpm_data, int width, int height)
{
	GtkWidget *retval = NULL;
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (xpm_data != NULL, NULL);

	pixbuf = gdk_pixbuf_new_from_xpm_data (xpm_data);
	if (pixbuf != NULL) {
		GdkPixbuf *scaled = gdk_pixbuf_scale_simple (pixbuf,
							     width,
							     height,
							     GDK_INTERP_BILINEAR);
		retval = gtk_type_new (gnome_pixmap_get_type ());
		gtk_image_set_from_pixbuf (GTK_IMAGE (retval), scaled);
		gdk_pixbuf_unref (scaled);
		gdk_pixbuf_unref (pixbuf);
	} else {
		retval = gtk_type_new (gnome_pixmap_get_type ());
	}
	return retval;
}


GtkWidget *
gnome_pixmap_new_from_gnome_pixmap (GnomePixmap *gpixmap)
{
	GtkWidget *retval = NULL;
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (gpixmap != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PIXMAP (gpixmap), NULL);

	pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (gpixmap));
	if (pixbuf != NULL) {
		retval = gtk_type_new (gnome_pixmap_get_type ());
		gtk_image_set_from_pixbuf (GTK_IMAGE (retval), pixbuf);
	} else {
		retval = gtk_type_new (gnome_pixmap_get_type ());
	}
	return retval;
}

void
gnome_pixmap_load_file (GnomePixmap *gpixmap, const char *filename)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));

	gtk_image_set_from_file (GTK_IMAGE (gpixmap), filename);
}

void
gnome_pixmap_load_file_at_size (GnomePixmap *gpixmap,
				const char *filename,
				int width, int height)
{
	GdkPixbuf *pixbuf;
        GError *error;

	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (filename != NULL);

        error = NULL;
	pixbuf = gdk_pixbuf_new_from_file (filename, &error);
        if (error != NULL) {
                g_warning (G_STRLOC ": cannot open %s: %s",
                           filename, error->message);
                g_error_free (error);
        }
	if (pixbuf != NULL) {
		GdkPixbuf *scaled = gdk_pixbuf_scale_simple (pixbuf,
							     width,
							     height,
							     GDK_INTERP_BILINEAR);
		gtk_image_set_from_pixbuf (GTK_IMAGE (gpixmap), scaled);
		gdk_pixbuf_unref (scaled);
		gdk_pixbuf_unref (pixbuf);
	} else {
		gtk_image_set_from_file (GTK_IMAGE (gpixmap), NULL);
	}
}

void
gnome_pixmap_load_xpm_d (GnomePixmap *gpixmap,
			 const char **xpm_data)
{
	GdkPixbuf *pixbuf;

	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));

	pixbuf = gdk_pixbuf_new_from_xpm_data (xpm_data);
	if (pixbuf != NULL) {
		gtk_image_set_from_pixbuf (GTK_IMAGE (gpixmap), pixbuf);
		gdk_pixbuf_unref (pixbuf);
	} else {
		gtk_image_set_from_file (GTK_IMAGE (gpixmap), NULL);
	}
}

void
gnome_pixmap_load_xpm_d_at_size (GnomePixmap *gpixmap,
				 const char **xpm_data,
				 int width, int height)
{
	GdkPixbuf *pixbuf;

	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));

	pixbuf = gdk_pixbuf_new_from_xpm_data (xpm_data);
	if (pixbuf != NULL) {
		GdkPixbuf *scaled = gdk_pixbuf_scale_simple (pixbuf,
							     width,
							     height,
							     GDK_INTERP_BILINEAR);
		gtk_image_set_from_pixbuf (GTK_IMAGE (gpixmap), scaled);
		gdk_pixbuf_unref (scaled);
		gdk_pixbuf_unref (pixbuf);
	} else {
		gtk_image_set_from_file (GTK_IMAGE (gpixmap), NULL);
	}
}

#endif /* GNOME_DISABLE_DEPRECATED_SOURCE */
