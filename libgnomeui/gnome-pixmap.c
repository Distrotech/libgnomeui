/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*-

   Copyright (C) 1999 Red Hat, Inc.
    
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

#include <config.h>
#include "gnome-pixmap.h"
#include <stdio.h>
#include "libart_lgpl/art_affine.h"
#include "libart_lgpl/art_rgb_affine.h"
#include "libart_lgpl/art_rgb_rgba_affine.h"

static void gnome_pixmap_class_init    (GnomePixmapClass *class);
static void gnome_pixmap_init          (GnomePixmap      *gpixmap);
static void gnome_pixmap_destroy       (GtkObject        *object);
static gint gnome_pixmap_expose        (GtkWidget        *widget,
					GdkEventExpose   *event);

static void clear_image (GnomePixmap *gpixmap, GtkStateType state);
static void clear_old_images           (GnomePixmap *gpixmap);

static GtkMiscClass *parent_class = NULL;

#define INTENSITY(r, g, b) ((r) * 0.30 + (g) * 0.59 + (b) * 0.11)


/*
 * Widget functions
 */

/**
 * gnome_pixmap_get_type:
 *
 * Registers the &GnomePixmap class if necessary, and returns the type ID
 * associated to it.
 *
 * Returns: the type ID of the &GnomePixmap class.
 */
guint
gnome_pixmap_get_type (void)
{
	static guint pixmap_type = 0;

	if (!pixmap_type) {
		GtkTypeInfo pixmap_info = {
			"GnomePixmap",
			sizeof (GnomePixmap),
			sizeof (GnomePixmapClass),
			(GtkClassInitFunc) gnome_pixmap_class_init,
			(GtkObjectInitFunc) gnome_pixmap_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		pixmap_type = gtk_type_unique (gtk_misc_get_type (), &pixmap_info);
	}

	return pixmap_type;
}

static void
gnome_pixmap_init (GnomePixmap *gpixmap)
{
        guint i;

        GTK_WIDGET_SET_FLAGS(GTK_WIDGET(gpixmap), GTK_NO_WINDOW);

        /* Default to pixmap mode, store data on
           server */
	gpixmap->original_image = NULL;

	gpixmap->width = -1;
	gpixmap->height = -1;
	gpixmap->alpha_threshold = 128;
	gpixmap->mode = GNOME_PIXMAP_SIMPLE;

        for (i = 0; i < 5; i ++) {
                gpixmap->image_data[i].pixbuf = NULL;
                gpixmap->image_data[i].mask = NULL;
		gpixmap->image_data[i].saturation = 1.0;
		gpixmap->image_data[i].pixelate = FALSE;

		if (i == GTK_STATE_INSENSITIVE) {
			gpixmap->image_data[i].saturation = 0.8;
			gpixmap->image_data[i].pixelate = TRUE;
		}
        }
}

static void
gnome_pixmap_class_init (GnomePixmapClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;

	object_class->destroy = gnome_pixmap_destroy;

	parent_class = gtk_type_class (gtk_misc_get_type ());

	widget_class->expose_event = gnome_pixmap_expose;
}

static void
gnome_pixmap_destroy (GtkObject *object)
{
	GnomePixmap *gpixmap;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (object));

	gpixmap = GNOME_PIXMAP (object);

        clear_old_images(gpixmap);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
generate_state (GnomePixmap *gpixmap, gint state)
{
	/* Make sure we're clean */
	clear_image (gpixmap, state);

	if (gpixmap->image_data[state].saturation == 1.0) {
		gpixmap->image_data[state].pixbuf = gpixmap->original_scaled_image;
		gpixmap->image_data[state].mask = gpixmap->original_scaled_mask;

		if (gpixmap->original_scaled_image)
			gdk_pixbuf_ref (gpixmap->original_scaled_image);
		if (gpixmap->original_scaled_mask)
			gdk_bitmap_ref (gpixmap->original_scaled_mask);
		return;
	} else if (gpixmap->original_scaled_image != NULL) {
		GdkPixbuf *target;
		gint i, j;
		gint width, height, has_alpha, rowstride;
		guchar *target_pixels;
		guchar *original_pixels;
		guchar *current_pixel;
		guchar intensity;

		has_alpha = gdk_pixbuf_get_has_alpha (gpixmap->original_scaled_image);
		width = gdk_pixbuf_get_width (gpixmap->original_scaled_image);
		height = gdk_pixbuf_get_height (gpixmap->original_scaled_image);
		rowstride = gdk_pixbuf_get_rowstride (gpixmap->original_scaled_image);
		target = gdk_pixbuf_new (ART_PIX_RGB,
					 has_alpha,
					 gdk_pixbuf_get_bits_per_sample (gpixmap->original_scaled_image),
					 width, height);
		target_pixels = gdk_pixbuf_get_pixels (target);
		original_pixels = gdk_pixbuf_get_pixels (gpixmap->original_scaled_image);

		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				current_pixel = original_pixels + i*rowstride + j*(has_alpha?4:3);
				intensity = INTENSITY (*(current_pixel), *(current_pixel + 1), *(current_pixel + 2));
				if (gpixmap->image_data[state].pixelate && (i+j)%2 == 0) {
					*(target_pixels + i*rowstride + j*(has_alpha?4:3)) = intensity/2 + 127;
					*(target_pixels + i*rowstride + j*(has_alpha?4:3) + 1) = intensity/2 + 127;
					*(target_pixels + i*rowstride + j*(has_alpha?4:3) + 2) = intensity/2 + 127;
				} else if (gpixmap->image_data[state].pixelate) {
#define DARK_FACTOR 0.7
					*(target_pixels + i*rowstride + j*(has_alpha?4:3)) =
						(guchar) (((1.0 - gpixmap->image_data[state].saturation) * intensity
							   + gpixmap->image_data[state].saturation * (*(current_pixel)))) * DARK_FACTOR;
					*(target_pixels + i*rowstride + j*(has_alpha?4:3) + 1) =
						(guchar) (((1.0 - gpixmap->image_data[state].saturation) * intensity
							   + gpixmap->image_data[state].saturation * (*(current_pixel + 1)))) * DARK_FACTOR;
					*(target_pixels + i*rowstride + j*(has_alpha?4:3) + 2) =
						(guchar) (((1.0 - gpixmap->image_data[state].saturation) * intensity
							   + gpixmap->image_data[state].saturation * (*(current_pixel + 2)))) * DARK_FACTOR;
				} else {
					*(target_pixels + i*rowstride + j*(has_alpha?4:3)) =
						(guchar) ((1.0 - gpixmap->image_data[state].saturation) * intensity
							  + gpixmap->image_data[state].saturation * (*(current_pixel)));
					*(target_pixels + i*rowstride + j*(has_alpha?4:3) + 1) =
						(guchar) ((1.0 - gpixmap->image_data[state].saturation) * intensity
							  + gpixmap->image_data[state].saturation * (*(current_pixel + 1)));
					*(target_pixels + i*rowstride + j*(has_alpha?4:3) + 2) =
						(guchar) ((1.0 - gpixmap->image_data[state].saturation) * intensity
							  + gpixmap->image_data[state].saturation * (*(current_pixel + 2)));
				}
				if (has_alpha)
					*(target_pixels + i*rowstride + j*(has_alpha?4:3) + 3) = *(original_pixels + i*rowstride + j*(has_alpha?4:3) + 3);
			}
		}

		gpixmap->image_data[state].pixbuf = target;
		gpixmap->image_data[state].mask = gpixmap->original_scaled_mask;
		if (gpixmap->original_scaled_mask)
			gdk_pixmap_ref (gpixmap->image_data[state].mask);
	}
}

static void
paint_with_pixbuf (GnomePixmap *gpixmap, GdkRectangle *area)
{
	GtkWidget *widget;
        GdkPixbuf *draw_source;
        GdkBitmap *draw_mask;
        GtkMisc   *misc;
	gint x_off, y_off;
	gint top_clip, bottom_clip, left_clip, right_clip;

        g_return_if_fail (GTK_WIDGET_DRAWABLE(gpixmap));

        misc = GTK_MISC (gpixmap);
        widget = GTK_WIDGET (gpixmap);

	/* We find the pixmap */
        draw_source = gpixmap->image_data[GTK_WIDGET_STATE (widget)].pixbuf;
        draw_mask = gpixmap->image_data[GTK_WIDGET_STATE (widget)].mask;

	if (draw_source == NULL)
		generate_state (gpixmap, GTK_WIDGET_STATE (widget));

        draw_source = gpixmap->image_data[GTK_WIDGET_STATE (widget)].pixbuf;
        draw_mask = gpixmap->image_data[GTK_WIDGET_STATE (widget)].mask;

        if (draw_source == NULL)
		return;

	/* Now we actually want to draw the image */
	/* The first thing we do for that, is write the images coords in the
	 * drawable's coordinate system. */
	x_off = (widget->allocation.x * (1.0 - misc->xalign) +
		 (widget->allocation.x + widget->allocation.width
		  - (widget->requisition.width - misc->xpad * 2)) *
		 misc->xalign) + 0.5;
	y_off = (widget->allocation.y * (1.0 - misc->yalign) +
		 (widget->allocation.y + widget->allocation.height
		  - (widget->requisition.height - misc->ypad * 2)) *
		 misc->yalign) + 0.5;

	/* next, we want to do clipping, to find the coordinates in image space of
	 * the region to be drawn.  */
	left_clip = (x_off < area->x)?area->x - x_off:0;
	top_clip = (y_off < area->y)?area->y - y_off:0;
	if (x_off + gdk_pixbuf_get_width (draw_source) > area->x + area->width)
		right_clip = x_off + gdk_pixbuf_get_width (draw_source) - (area->x + area->width);
	else
		right_clip = 0;
	if (y_off + gdk_pixbuf_get_height (draw_source) > area->y + area->height)
		bottom_clip = y_off + gdk_pixbuf_get_height (draw_source) - (area->y + area->height);
	else
		bottom_clip = 0;

	/* it's in the allocation, but not the image, so we return */
	if (right_clip + left_clip >= gdk_pixbuf_get_width (draw_source)||
	    top_clip + bottom_clip >= gdk_pixbuf_get_height (draw_source))
		return;

#if 0
	g_print ("width=%d\theight=%d\n", gdk_pixbuf_get_width (draw_source), gdk_pixbuf_get_height (draw_source));
	g_print ("area->x=%d\tarea->y=%d\tarea->width=%d\tarea->height=%d\nx_off=%d\ty_off=%d\nright=%d\tleft=%d\ttop=%d\tbottom=%d\n\n", area->x, area->y, area->width, area->height, x_off, y_off, right_clip, left_clip, top_clip, bottom_clip);
#endif
	if (gpixmap->mode == GNOME_PIXMAP_SIMPLE || !gdk_pixbuf_get_has_alpha (draw_source)) {
		if (draw_mask) {
			gdk_gc_set_clip_mask (widget->style->black_gc, draw_mask);
			gdk_gc_set_clip_origin (widget->style->black_gc, x_off, y_off);
		}

		gdk_pixbuf_render_to_drawable (draw_source,
					       widget->window,
					       widget->style->black_gc,
					       left_clip, top_clip,
					       x_off + left_clip, y_off + top_clip,
					       gdk_pixbuf_get_width (draw_source) - left_clip - right_clip,
					       gdk_pixbuf_get_height (draw_source) - top_clip - bottom_clip,
					       GDK_RGB_DITHER_NORMAL,
					       0, 0); /* FIXME -- get the right offset */

		if (draw_mask) {
			gdk_gc_set_clip_mask (widget->style->black_gc, NULL);
			gdk_gc_set_clip_origin (widget->style->black_gc, 0, 0);
		}
	} else if (gpixmap->mode == GNOME_PIXMAP_COLOR) {
		GdkPixbuf *dest_source;
		gint i, j, height, width, rowstride, dest_rowstride;
		gint r, g, b;
		guchar *dest_pixels, *c, *a, *original_pixels;


		dest_source = gdk_pixbuf_new (ART_PIX_RGB,
					      FALSE,
					      gdk_pixbuf_get_bits_per_sample (draw_source),
					      gdk_pixbuf_get_width (draw_source) - left_clip - right_clip,
					      gdk_pixbuf_get_height (draw_source) - top_clip - bottom_clip);


		gdk_gc_set_clip_mask (widget->style->black_gc, draw_mask);
		gdk_gc_set_clip_origin (widget->style->black_gc, x_off, y_off);

		r = widget->style->bg[GTK_WIDGET_STATE (widget)].red >> 8;
		g = widget->style->bg[GTK_WIDGET_STATE (widget)].green >> 8;
		b = widget->style->bg[GTK_WIDGET_STATE (widget)].blue >> 8;
		height = gdk_pixbuf_get_height (dest_source);
		width = gdk_pixbuf_get_width (dest_source);
		rowstride = gdk_pixbuf_get_rowstride (draw_source);
		dest_rowstride = gdk_pixbuf_get_rowstride (dest_source);
		dest_pixels = gdk_pixbuf_get_pixels (dest_source);
		original_pixels = gdk_pixbuf_get_pixels (draw_source);
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				c = original_pixels + (i + top_clip)*rowstride + (j + left_clip)*4;
				a = c + 3;
				*(dest_pixels + i*dest_rowstride + j*3) = r + (((*c - r) * (*a) + 0x80) >> 8);
				c++;
				*(dest_pixels + i*dest_rowstride + j*3 + 1) = g + (((*c - g) * (*a) + 0x80) >> 8);
				c++;
				*(dest_pixels + i*dest_rowstride + j*3 + 2) = b + (((*c - b) * (*a) + 0x80) >> 8);
			}
		}

		gdk_pixbuf_render_to_drawable (dest_source,
					       widget->window,
					       widget->style->black_gc,
					       0, 0,
					       x_off + left_clip, y_off + top_clip,
					       width, height,
					       GDK_RGB_DITHER_NORMAL,
					       0, 0); /* FIXME -- get the right offset */

		gdk_gc_set_clip_mask (widget->style->black_gc, NULL);
		gdk_gc_set_clip_origin (widget->style->black_gc, 0, 0);

		gdk_pixbuf_unref (dest_source);
	}
}

static gint
gnome_pixmap_expose (GtkWidget *widget, GdkEventExpose *event)
{
	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_PIXMAP (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	if (GTK_WIDGET_DRAWABLE (widget))
		paint_with_pixbuf (GNOME_PIXMAP (widget), &event->area);

	return FALSE;
}


/*
 * Set image data, create with image data
 */

static void
clear_image(GnomePixmap *gpixmap, GtkStateType state)
{
        if (gpixmap->image_data[state].pixbuf != NULL) {
                gdk_pixbuf_unref(gpixmap->image_data[state].pixbuf);
                gpixmap->image_data[state].pixbuf = NULL;
        }

        if (gpixmap->image_data[state].mask != NULL) {
                gdk_bitmap_unref(gpixmap->image_data[state].mask);
                gpixmap->image_data[state].mask = NULL;
        }
}

static void
clear_old_images(GnomePixmap *gpixmap)
{
        guint i;

        i = 0;
        while (i < 5) {

                clear_image(gpixmap, i);

                ++i;
        }
}

static void
resize_to_fit(GnomePixmap *gpixmap)
{
        /* We base size on GTK_STATE_NORMAL, or failing that the first state
           we can find. */
        guint i;
        gint width, height;
        gint oldwidth, oldheight;
	GdkPixbuf *pix = NULL;

        oldwidth = GTK_WIDGET (gpixmap)->requisition.width;
        oldheight = GTK_WIDGET (gpixmap)->requisition.height;

        width = 0;
        height = 0;


	pix = gpixmap->image_data[GTK_STATE_NORMAL].pixbuf;

	if (pix == NULL) {
		i = 0;
		while (i < 5) {
			pix = gpixmap->image_data[i].pixbuf;

			if (pix != NULL)
				break;

			++i;
		}
	}

	if (pix != NULL) {
		width = pix->art_pixbuf->width;
		height = pix->art_pixbuf->height;
	}

        if (width * height > 0) {
                GTK_WIDGET (gpixmap)->requisition.width = width + GTK_MISC (gpixmap)->xpad * 2;
                GTK_WIDGET (gpixmap)->requisition.height = height + GTK_MISC (gpixmap)->ypad * 2;
        } else {
                GTK_WIDGET (gpixmap)->requisition.width = 0;
                GTK_WIDGET (gpixmap)->requisition.height = 0;
        }

        if (GTK_WIDGET_VISIBLE (gpixmap)) {
                if ((GTK_WIDGET (gpixmap)->requisition.width != oldwidth) ||
                    (GTK_WIDGET (gpixmap)->requisition.height != oldheight))
                        gtk_widget_queue_resize (GTK_WIDGET (gpixmap));
                else
                        gtk_widget_queue_clear (GTK_WIDGET (gpixmap));
	}
}

static void
set_pixbuf(GnomePixmap* gpixmap, GtkStateType state, GdkPixbuf* pixbuf, GdkBitmap* mask)
{
        clear_image(gpixmap, state);

        if (pixbuf != NULL) {
		gint width, height;

		width = gpixmap->width;
		height = gpixmap->height;

		if (width == -1 && gpixmap->original_image)
			width = gdk_pixbuf_get_width (gpixmap->original_image);

		if (height == -1 && gpixmap->original_image)
			height = gdk_pixbuf_get_height (gpixmap->original_image);

		if (width == -1 || height == -1 ||
		    (width == gdk_pixbuf_get_width (gpixmap->original_image) &&
		     height ==  gdk_pixbuf_get_height (gpixmap->original_image))) {
			gpixmap->image_data[state].pixbuf = pixbuf;
			gdk_pixbuf_ref(pixbuf);
		} else {
			gpixmap->image_data[state].pixbuf = gnome_pixbuf_scale (pixbuf, width, height);
			/* We want to regenerate the mask */
			mask = NULL;
		}
        }

        if (mask != NULL) {
                gpixmap->image_data[state].mask = mask;
		gdk_bitmap_ref (mask);
        } else {
                /* Create the mask if we have alpha and a pixbuf */
                if (pixbuf && gdk_pixbuf_get_has_alpha(pixbuf)) {
                        GdkBitmap *mask = NULL;

                        mask = gdk_pixmap_new(NULL,
                                              gdk_pixbuf_get_width(pixbuf),
                                              gdk_pixbuf_get_height(pixbuf),
                                              1);

                        gdk_pixbuf_render_threshold_alpha(pixbuf, mask,
                                                          0, 0, 0, 0,
                                                          gdk_pixbuf_get_width(pixbuf),
                                                          gdk_pixbuf_get_height(pixbuf),
                                                          gpixmap->alpha_threshold);

                        gpixmap->image_data[state].mask = mask;
                }
        }
}

static void
set_pixbufs(GnomePixmap* gpixmap, GdkPixbuf* pixbufs[5], GdkBitmap* masks[5])
{
        guint i;

        i = 0;
        while (i < 5) {

                set_pixbuf(gpixmap,
                           i,
                           pixbufs ? pixbufs[i] : NULL,
                           masks ? masks[i] : NULL);
                ++i;
        }

        resize_to_fit(gpixmap);
}

static void
free_buffer (gpointer user_data, gpointer data)
{
        g_return_if_fail (data != NULL);
        g_return_if_fail (user_data == NULL);
	g_free (data);
}




/*
 * Public functions
 */


/**
 * gnome_pixmap_new:
 * @void:
 *
 * Creates a new empty @GnomePixmap.
 *
 * Return value: A newly-created @GnomePixmap
 **/
GtkWidget*
gnome_pixmap_new (void)
{
        GtkWidget* widget;

        widget = gtk_type_new(gnome_pixmap_get_type());

	return widget;
}

/**
 * gnome_pixmap_new_from_file:
 * @filename: The filename of the file to be loaded.
 *
 * Note that the new_from_file functions give you no way to detect errors;
 * if the file isn't found/loaded, you get an empty widget.
 * to detect errors just do:
 *
 * <programlisting>
 * gdk_pixbuf_new_from_file (filename);
 * if (pixbuf != NULL) {
 *         gpixmap = gnome_pixmap_new_from_pixbuf (pixbuf);
 * } else {
 *         // handle your error...
 * }
 * </programlisting>
 * 
 * Return value: A newly allocated @GnomePixmap with the file at @filename loaded.
 **/
GtkWidget*
gnome_pixmap_new_from_file          (const char *filename)
{
	GtkWidget *retval = NULL;
	GdkPixbuf *pixbuf;
	g_return_val_if_fail (filename != NULL, NULL);

	pixbuf = gdk_pixbuf_new_from_file (filename);
	if (pixbuf != NULL) {
		retval = gnome_pixmap_new_from_pixbuf (pixbuf);
		gdk_pixbuf_unref (pixbuf);
	} else {
		retval = gnome_pixmap_new ();
	}

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
gnome_pixmap_new_from_file_at_size          (const gchar *filename, gint width, gint height)
{
        GtkWidget *retval = NULL;

	g_return_val_if_fail (filename != NULL, NULL);
	g_return_val_if_fail (width >= -1, NULL);
	g_return_val_if_fail (height >= -1, NULL);

	retval = gnome_pixmap_new_from_file (filename);
	gnome_pixmap_set_pixbuf_size (GNOME_PIXMAP (retval), width, height);

        return retval;
}

/**
 * gnome_pixmap_new_from_file_at_size:
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
gnome_pixmap_new_from_xpm_d         (const char **xpm_data)
{
	GtkWidget *retval = NULL;
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (xpm_data != NULL, NULL);

	pixbuf = gdk_pixbuf_new_from_xpm_data (xpm_data);
	if (pixbuf != NULL) {
		retval = gnome_pixmap_new_from_pixbuf (pixbuf);
		gdk_pixbuf_unref (pixbuf);
	} else {
		retval = gnome_pixmap_new ();
	}

        return retval;
}

/**
 * gnome_pixmap_new_from_file_at_size:
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

	g_return_val_if_fail (xpm_data != NULL, NULL);
	g_return_val_if_fail (width >= -1, NULL);
	g_return_val_if_fail (height >= -1, NULL);

	retval = gnome_pixmap_new_from_xpm_d (xpm_data);
	gnome_pixmap_set_pixbuf_size (GNOME_PIXMAP (retval), width, height);

        return retval;
}


/**
 * gnome_pixmap_new_from_file_at_size:
 * @pixbuf: The pixbuf to be loaded.
 *
 * Loads a new @GnomePixmap from the @pixbuf.
 *
 * Return value: value: A newly allocated @GnomePixmap with the image from @pixbuf loaded.
 **/
GtkWidget*
gnome_pixmap_new_from_pixbuf          (GdkPixbuf *pixbuf)
{
	GnomePixmap *retval;

	retval = gtk_type_new (gnome_pixmap_get_type ());

	g_return_val_if_fail (pixbuf != NULL, GTK_WIDGET (retval));

	retval->original_image = pixbuf;
	gdk_pixbuf_ref (pixbuf);

	retval->original_scaled_image = pixbuf;
	gdk_pixbuf_ref (pixbuf);

	if (gdk_pixbuf_get_has_alpha (pixbuf)) {
		retval->original_scaled_mask
			= gdk_pixmap_new (NULL,
					  gdk_pixbuf_get_width (pixbuf),
					  gdk_pixbuf_get_height (pixbuf),
					  1);
		gdk_pixbuf_render_threshold_alpha
			(pixbuf,
			 retval->original_scaled_mask,
			 0, 0, 0, 0,
			 gdk_pixbuf_get_width (pixbuf),
			 gdk_pixbuf_get_height (pixbuf),
			 retval->alpha_threshold);
	}

	/* Now we handle our requisition */
	GTK_WIDGET (retval)->requisition.width = gdk_pixbuf_get_width (pixbuf) + GTK_MISC (retval)->xpad * 2;
	GTK_WIDGET (retval)->requisition.height = gdk_pixbuf_get_height (pixbuf) + GTK_MISC (retval)->ypad * 2;

	return GTK_WIDGET (retval);
        /*return gnome_pixmap_new_from_pixbuf_at_size(pixbuf, -1, -1);*/
}

/**
 * gnome_pixmap_new_from_file_at_size:
 * @pixbuf: The pixbuf be loaded.
 * @width: The width to scale the image to.
 * @height: The height to scale the image to.
 *
 * Loads a new @GnomePixmap from the @pixbuf, and scales it (if necessary) to
 * the size indicated by @height and @width.  If either are set to -1, then the
 * "natural" dimension of the image is used in place.
 *
 * Return value: value: A newly allocated @GnomePixmap with the image from @pixbuf loaded.
 **/
GtkWidget*
gnome_pixmap_new_from_pixbuf_at_size  (GdkPixbuf *pixbuf, gint width, gint height)
{
        GtkWidget *retval = NULL;

	g_return_val_if_fail (pixbuf != NULL, NULL);
	g_return_val_if_fail (width >= -1, NULL);
	g_return_val_if_fail (height >= -1, NULL);

	retval = gnome_pixmap_new_from_pixbuf (pixbuf);
	gnome_pixmap_set_pixbuf_size (GNOME_PIXMAP (retval), width, height);

        return retval;
}

/**
 * gnome_pixmap_set_pixbuf_size:
 * @gpixmap: A @GnomePixmap.
 * @width: The new width.
 * @height: The new height.
 *
 * Sets the current size of the image displayed.  If there were custom "state"
 * pixbufs set, as a side effect, they are discarded and must be re set at the
 * new size.
 *
 **/
/* Setters and getters */
void
gnome_pixmap_set_pixbuf_size (GnomePixmap      *gpixmap,
			      gint              width,
			      gint              height)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));

	/* did we change anything? */
	if (width == gpixmap->width && height == gpixmap->height)
		return;

	/* FIXME: if old_width == -1 and pixbuf->width == width, we can just set
	 * the values and return as an optomization step. */

	clear_old_images (gpixmap);

	gpixmap->width = width;
	gpixmap->height = height;

	if (width == -1)
		width = gdk_pixbuf_get_width (gpixmap->original_image);
	if (height == -1)
		height = gdk_pixbuf_get_height (gpixmap->original_image);

	if (gpixmap->original_scaled_image)
		gdk_pixbuf_unref (gpixmap->original_scaled_image);
	if (gpixmap->original_scaled_mask)
		gdk_bitmap_unref (gpixmap->original_scaled_mask);

	gpixmap->original_scaled_image = gnome_pixbuf_scale (gpixmap->original_image,
							     width, height);
	gpixmap->original_scaled_mask = gdk_pixmap_new (NULL, width, height, 1);
	gdk_pixbuf_render_threshold_alpha(gpixmap->original_scaled_image,
					  gpixmap->original_scaled_mask,
					  0, 0, 0, 0,
					  width, height,
					  gpixmap->alpha_threshold);

	GTK_WIDGET (gpixmap)->requisition.width = width + GTK_MISC (gpixmap)->xpad * 2;
	GTK_WIDGET (gpixmap)->requisition.height = height + GTK_MISC (gpixmap)->ypad * 2;

        if (GTK_WIDGET_VISIBLE (gpixmap))
		gtk_widget_queue_resize (GTK_WIDGET (gpixmap));
}

/**
 * gnome_pixmap_get_pixbuf_size:
 * @gpixmap: A @GnomePixmap
 * @width: A pointer to place the width in.
 * @height: A pointer to place the height in.
 *
 * Sets @width and @height to be the widgets current dimensions.  They will
 * return the width or height of the image, or -1, -1 if the image's "natural"
 * dimensions are used.  Either or both dimension arguments may be NULL, as
 * necessary.
 *
 **/
void
gnome_pixmap_get_pixbuf_size (GnomePixmap      *gpixmap,
			      gint             *width,
			      gint             *height)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));

	if (width)
		*width = gpixmap->width;
	if (height)
		*height = gpixmap->height;
}


/**
 * gnome_pixmap_set_pixbuf:
 * @gpixmap: A @GnomePixmap.
 * @pixbuf: The new pixbuf.
 *
 * Sets the image shown to be that of the pixbuf.  If there is a current image
 * used by the @gpixmap, it is discarded along with any pixmaps at a particular
 * state.  However, the @gpixmap will keep the same geometry as the old image,
 * or if the width or height are set to -1, it will inherit the new image's
 * geometry.
 *
 **/
void
gnome_pixmap_set_pixbuf (GnomePixmap *gpixmap,
			 GdkPixbuf *pixbuf)
{
	gint old_width, old_height;

	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (pixbuf != NULL);

	if (pixbuf == gpixmap->original_image)
		return;

	old_width = gpixmap->width;
	old_height = gpixmap->height;

	/* make sure it's empty */
        gdk_pixbuf_ref (pixbuf);
	gnome_pixmap_clear (gpixmap);
	gpixmap->original_image = pixbuf;

	if (gdk_pixbuf_get_has_alpha (pixbuf)) {
		gpixmap->original_scaled_mask
			= gdk_pixmap_new (NULL,
					  gdk_pixbuf_get_width (pixbuf),
					  gdk_pixbuf_get_height (pixbuf),
					  1);
		gdk_pixbuf_render_threshold_alpha
			(pixbuf,
			 gpixmap->original_scaled_mask,
			 0, 0, 0, 0,
			 gdk_pixbuf_get_width (pixbuf),
			 gdk_pixbuf_get_height (pixbuf),
			 gpixmap->alpha_threshold);
	}
	if (old_width == -1 && old_height == -1) {
		GTK_WIDGET (gpixmap)->requisition.width = gdk_pixbuf_get_width (pixbuf) + GTK_MISC (gpixmap)->xpad * 2;
		GTK_WIDGET (gpixmap)->requisition.height = gdk_pixbuf_get_height (pixbuf) + GTK_MISC (gpixmap)->ypad * 2;
	} else {
		/* We fake the old size, and set the original one back  */
		gpixmap->width = -1;
		gpixmap->height = -1;
		gnome_pixmap_set_pixbuf_size (gpixmap, old_width, old_height);
	}
}


/**
 * gnome_pixmap_get_pixbuf:
 * @gpixmap: A @GnomePixmap.
 *
 * Gets the current image used by @gpixmap.
 *
 * Return value: A pixbuf.
 **/
GdkPixbuf *
gnome_pixmap_get_pixbuf (GnomePixmap      *gpixmap)
{
	g_return_val_if_fail (gpixmap != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PIXMAP (gpixmap), NULL);

	return gpixmap->original_image;
}


/**
 * gnome_pixmap_set_pixbuf_at_state:
 * @gpixmap: A @GnomePixmap.
 * @state: The state being set.
 * @pixbuf: The new image for the state.
 * @mask: The mask for the new image.
 *
 * Sets a custom image for the image at @state.  For example, you can set the
 * prelighted appearance to a different image from the normal one.  If
 * necessary, the image will be scaled to the appropriate size.  The mask can
 * also be optionally NULL.  The image set will be modified by the draw vals as
 * normal.
 *
 **/
void
gnome_pixmap_set_pixbuf_at_state (GnomePixmap *gpixmap,
				  GtkStateType state,
				  GdkPixbuf *pixbuf,
				  GdkBitmap *mask)
{
        g_return_if_fail (gpixmap != NULL);
        g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));

	if ((state == GTK_STATE_NORMAL) && (gpixmap->original_image == NULL)) {
		/* We've never set the original image.  Set one so we don't fall
		 * over in some states */
		gpixmap->original_image = pixbuf;
		gdk_pixbuf_ref (gpixmap->original_image);

		gpixmap->original_scaled_image = pixbuf;
		gdk_pixbuf_ref (gpixmap->original_scaled_image);

		if (mask != NULL) {
			gpixmap->original_scaled_mask = mask;
			gdk_bitmap_ref (gpixmap->original_scaled_mask);
		}
		GTK_WIDGET (gpixmap)->requisition.width = gdk_pixbuf_get_width (pixbuf) + GTK_MISC (gpixmap)->xpad * 2;
		GTK_WIDGET (gpixmap)->requisition.height = gdk_pixbuf_get_height (pixbuf) + GTK_MISC (gpixmap)->ypad * 2;
	}

        set_pixbuf (gpixmap, state, pixbuf, mask);
}

/**
 * gnome_pixmap_set_pixbufs_at_state:
 * @gpixmap: A @GnomePixmap.
 * @pixbufs: The images.
 * @masks: The masks.
 *
 * Sets a custom image for all the possible states of the image.  Both @pixbufs
 * and @masks are indexed by a GtkStateType.  Any or all of the images can be
 * NULL, as necessary.  The image set will be modified by the draw vals as
 * normal.
 *
 **/
void
gnome_pixmap_set_pixbufs_at_state (GnomePixmap *gpixmap,
				   GdkPixbuf   *pixbufs[5],
				   GdkBitmap   *masks[5])
{
        g_return_if_fail(gpixmap != NULL);
        g_return_if_fail(GNOME_IS_PIXMAP (gpixmap));

	if ((gpixmap->original_image == NULL) && (pixbufs[GTK_STATE_NORMAL] != NULL)) {
		/* We are setting the images from scratch */
		gpixmap->original_image = pixbufs[GTK_STATE_NORMAL];
		gdk_pixbuf_ref (gpixmap->original_image);

		gpixmap->original_scaled_image = pixbufs[GTK_STATE_NORMAL];
		gdk_pixbuf_ref (gpixmap->original_scaled_image);

		if (masks[GTK_STATE_NORMAL] != NULL) {
			gpixmap->original_scaled_mask = masks[GTK_STATE_NORMAL];
			gdk_bitmap_ref (gpixmap->original_scaled_mask);
		}
		GTK_WIDGET (gpixmap)->requisition.width = gdk_pixbuf_get_width (pixbufs[GTK_STATE_NORMAL]) + GTK_MISC (gpixmap)->xpad * 2;
		GTK_WIDGET (gpixmap)->requisition.height = gdk_pixbuf_get_height (pixbufs[GTK_STATE_NORMAL]) + GTK_MISC (gpixmap)->ypad * 2;
	}

        set_pixbufs(gpixmap, pixbufs, masks);
}

/**
 * gnome_pixmap_clear:
 * @gpixmap: A @GnomePixmap.
 *
 * Removes any images from @gpixmap.  If still visible, the image will appear empty.
 *
 **/
void
gnome_pixmap_clear (GnomePixmap *gpixmap)
{
        g_return_if_fail(gpixmap != NULL);
        g_return_if_fail(GNOME_IS_PIXMAP (gpixmap));

        clear_old_images(gpixmap);
	if (gpixmap->original_image) {
		gdk_pixbuf_unref (gpixmap->original_image);
		gpixmap->original_image = NULL;
	}

	if (gpixmap->original_scaled_image) {
		gdk_pixbuf_unref (gpixmap->original_scaled_image);
		gpixmap->original_scaled_image = NULL;
	}

	if (gpixmap->original_scaled_mask) {
		gdk_pixmap_unref (gpixmap->original_scaled_mask);
		gpixmap->original_scaled_mask = NULL;
	}

        if (GTK_WIDGET_VISIBLE (gpixmap)) {
		gtk_widget_queue_clear (GTK_WIDGET (gpixmap));
	}
}

/**
 * gnome_pixmap_set_draw_vals:
 * @gpixmap: A @GnomePixmap.
 * @state: The the state to set the modifications to
 * @saturation: The saturtion offset.
 * @pixelate: Draw the insensitive stipple.
 *
 * Sets the modification parameters for a particular state.  The saturation
 * level determines the amount of color in the image.  The default level of 1.0
 * leaves the color unchanged while a level of 0.0 means the image is fully
 * saturated, and has no color.  @saturation can be set to values greater then 1.0,
 * or less then 0.0, but this produces less meaningful results.  If @pixelate is
 * set to TRUE, then in adition to any saturation, a light stipple is overlayed
 * over the image.
 *
 **/
void
gnome_pixmap_set_draw_vals (GnomePixmap *gpixmap,
			    GtkStateType state,
			    gfloat saturation,
			    gboolean pixelate)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (state >= 0 && state < 5);

	gpixmap->image_data[state].saturation = saturation;
	gpixmap->image_data[state].pixelate = pixelate;
}

/**
 * gnome_pixmap_set_draw_mode:
 * @gpixmap: A @GnomePixmap.
 * @mode: The new drawing mode.
 *
 * This sets the drawing mode of the image to be @mode.  The image
 * must have an alpha channel if @GNOME_PIXMAP_COLOR is to be used.
 **/
void
gnome_pixmap_set_draw_mode (GnomePixmap *gpixmap,
			    GnomePixmapDraw mode)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));

	if (gpixmap->mode == mode)
		return;

	if (gpixmap->original_image && !gdk_pixbuf_get_has_alpha (gpixmap->original_image))
		return;

	gpixmap->mode = mode;
	clear_old_images (gpixmap);

        if (GTK_WIDGET_VISIBLE (gpixmap)) {
		gtk_widget_queue_clear (GTK_WIDGET (gpixmap));
	}
}

/**
 * gnome_pixmap_get_draw_mode:
 * @gpixmap: A @GnomePixmap.
 *
 * Gets the current draw mode.
 *
 * Return value: The current @GnomePixmapDraw setting.
 **/
GnomePixmapDraw
gnome_pixmap_get_draw_mode (GnomePixmap *gpixmap)
{
	g_return_val_if_fail (gpixmap != NULL, GNOME_PIXMAP_SIMPLE);
	g_return_val_if_fail (GNOME_IS_PIXMAP (gpixmap), GNOME_PIXMAP_SIMPLE);

	return gpixmap->mode;
}

/**
 * gnome_pixmap_set_alpha_threshold:
 * @gpixmap: A @GnomePixmap.
 * @alpha_threshold: The alpha threshold
 *
 * Sets the alpha threshold for @gpixmap.  It is used to determine which pixels
 * are shown when the image has an alpha channel, and is only used if no mask is
 * set.
 *
 **/
void
gnome_pixmap_set_alpha_threshold (GnomePixmap *gpixmap,
				  gint alpha_threshold)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (alpha_threshold >= 0 || alpha_threshold <= 255);

	if (alpha_threshold == gpixmap->alpha_threshold)
		return;

	gpixmap->alpha_threshold = alpha_threshold;
	clear_old_images (gpixmap);



	if (gpixmap->original_scaled_mask)
		gdk_pixbuf_render_threshold_alpha
			(gpixmap->original_scaled_image,
			 gpixmap->original_scaled_mask,
			 0, 0, 0, 0,
			 gdk_pixbuf_get_width (gpixmap->original_scaled_image),
			 gdk_pixbuf_get_height (gpixmap->original_scaled_image),
			 gpixmap->alpha_threshold);

        if (GTK_WIDGET_VISIBLE (gpixmap)) {
		gtk_widget_queue_clear (GTK_WIDGET (gpixmap));
	}
}

/**
 * gnome_pixmap_get_alpha_threshold:
 * @gpixmap: A @GnomePixmap.
 *
 * Gets the current alpha threshold.
 *
 * Return value: The alpha threshold
 **/
gint
gnome_pixmap_get_alpha_threshold (GnomePixmap *gpixmap)
{
	g_return_val_if_fail (gpixmap != NULL, 0);
	g_return_val_if_fail (GNOME_IS_PIXMAP (gpixmap), 0);

	return gpixmap->alpha_threshold;
}




/*
 * gdk_pixbuf helper_functions
 */
#if 0
GdkPixbuf*
gnome_pixbuf_scale(GdkPixbuf* gdk_pixbuf,
                   gint w, gint h)
{
	GdkPixbuf *retval;
        long w_old, h_old;

	gint i, j, has_alpha, rowstride;
	guchar *pixels;
	double affine[6];
#if 0
        ArtPixBuf* retval;
        ArtPixBuf* pixbuf;
	unsigned char *data, *p, *p2, *p3, *data_old;
        long w_old, h_old;
        long rowstride_old;
	long x, y, w2, h2, x_old, y_old, x2, y2, i, x3, y3;
	long yw, xw, ww, hw, r, g, b, a, r2, g2, b2, a2;
	int new_channels;
	int new_rowstride;
#endif
	g_return_val_if_fail (gdk_pixbuf != NULL, NULL);

#if 0
        pixbuf = gdk_pixbuf->art_pixbuf;
#endif

	g_return_val_if_fail (gdk_pixbuf_get_format (gdk_pixbuf) == ART_PIX_RGB, NULL);
	g_return_val_if_fail (gdk_pixbuf_get_n_channels (gdk_pixbuf) == 3 || gdk_pixbuf_get_n_channels (gdk_pixbuf) == 4, NULL);
	g_return_val_if_fail (gdk_pixbuf_get_bits_per_sample (gdk_pixbuf) == 8, NULL);
        g_return_val_if_fail (w > 0, NULL);
        g_return_val_if_fail (h > 0, NULL);

        w_old = gdk_pixbuf_get_width (gdk_pixbuf);
        h_old = gdk_pixbuf_get_height (gdk_pixbuf);

#if 0
        /* optimization if no scaling is needed */
        if (w_old == w && h_old == h) {
                GdkPixbuf *copy;
                guchar* data_copy;

                data_copy = g_malloc(gdk_pixbuf_get_height(gdk_pixbuf) *
                                     gdk_pixbuf_get_rowstride(gdk_pixbuf));

                memcpy(data_copy,
                       gdk_pixbuf->art_pixbuf->pixels,
                       gdk_pixbuf->art_pixbuf->height * gdk_pixbuf->art_pixbuf->rowstride);

                copy = gdk_pixbuf_new_from_data(data_copy,
                                                gdk_pixbuf_get_format(gdk_pixbuf),
                                                gdk_pixbuf_get_has_alpha(gdk_pixbuf),
                                                gdk_pixbuf_get_width(gdk_pixbuf),
                                                gdk_pixbuf_get_height(gdk_pixbuf),
                                                gdk_pixbuf_get_rowstride(gdk_pixbuf),
                                                free_buffer, NULL);

                return copy;
        }
#endif

	has_alpha = gdk_pixbuf_get_has_alpha (gdk_pixbuf);
	retval = gdk_pixbuf_new (ART_PIX_RGB, has_alpha,
				 gdk_pixbuf_get_bits_per_sample (gdk_pixbuf),
				 w, h);
	pixels = gdk_pixbuf_get_pixels (retval);
	rowstride = gdk_pixbuf_get_rowstride (retval);
	g_return_val_if_fail (pixels != NULL, NULL);

	art_affine_scale (affine, (gfloat)w/(gfloat)w_old, (gfloat)h/(gfloat)h_old);


	for (i = 0; i < h; i ++) {
		for (j = 0; j < w; j++) {
			pixels[j*(has_alpha?3:4) + i*rowstride] = 0x00;
			pixels[j*(has_alpha?3:4) + i*rowstride + 1] = 0x00;
			pixels[j*(has_alpha?3:4) + i*rowstride + 2] = 0x00;
			if (has_alpha)
				pixels[j*(has_alpha?3:4) + i*rowstride + 3] = 0xFF;
		}
	}
	if (gdk_pixbuf_get_has_alpha (gdk_pixbuf))
		art_rgb_rgba_affine (gdk_pixbuf_get_pixels (retval),
				     0, 0, w, h,
				     gdk_pixbuf_get_rowstride (retval),
				     gdk_pixbuf_get_pixels (gdk_pixbuf),
				     gdk_pixbuf_get_width (gdk_pixbuf),
				     gdk_pixbuf_get_height (gdk_pixbuf),
				     gdk_pixbuf_get_rowstride (gdk_pixbuf),
				     affine,
				     ART_FILTER_NEAREST,
				     NULL);
	else
		art_rgb_affine (gdk_pixbuf_get_pixels (retval),
				0, 0, w, h,
				gdk_pixbuf_get_rowstride (retval),
				gdk_pixbuf_get_pixels (gdk_pixbuf),
				gdk_pixbuf_get_width (gdk_pixbuf),
				gdk_pixbuf_get_height (gdk_pixbuf),
				gdk_pixbuf_get_rowstride (gdk_pixbuf),
				affine,
				ART_FILTER_NEAREST,
				NULL);
	return retval;
#if 0
        rowstride_old = gdk_pixbuf_get_rowstride (gdk_pixbuf);
        data_old = gdk_pixbuf_get_pixels (gdk_pixbuf);

	/* Always align rows to 32-bit boundaries */

	new_channels = pixbuf->has_alpha ? 4 : 3;
        /* for debugging just use the simple rowstride */
	new_rowstride = w * new_channels; /* 4 * ((new_channels * w + 3) / 4); */

        data = g_malloc (h * new_rowstride);

        if (pixbuf->has_alpha)
		retval = art_pixbuf_new_rgba_dnotify (data, w, h, new_rowstride,
                                                      NULL, free_buffer);
	else
                retval = art_pixbuf_new_rgb_dnotify (data, w, h, new_rowstride,
                                                     NULL, free_buffer);

	p = data;

        /* Multiply stuff by 256 to avoid floating point. */
	ww = (w_old << 8) / w;  /* old-width-per-new-width * 256 */
	hw = (h_old << 8) / h;  /* old-height-per-new-height * 256 */
	h2 = h << 8;
	w2 = w << 8;

	for (y = 0; y < h2; y += 256) {
		y_old = (y * h_old) / h;
		y2 = y_old & 0xff;
		y_old >>= 8;
		for (x = 0; x < w2; x += 256) {
			x_old = (x * w_old) / w;
			x2 = x_old & 0xff;
			x_old >>= 8;
			i = x_old * (pixbuf->has_alpha ? 4 : 3) + (y_old * rowstride_old);
			p2 = data_old + i;

			r2 = g2 = b2 = a2 = 0;
			yw = hw;
			y3 = y2;
			while (yw) {
				xw = ww;
				x3 = x2;
				p3 = p2;
				r = g = b = a = 0;
				while (xw) {
					if ((256 - x3) < xw)
						i = 256 - x3;
					else
						i = xw;

                                        if (pixbuf->has_alpha) {
						r += p3[0] * i;
						g += p3[1] * i;
						b += p3[2] * i;
                                                a += p3[3] * i;
                                        } else {
						r += p3[0] * i;
						g += p3[1] * i;
						b += p3[2] * i;
					}
					p3 += pixbuf->n_channels;
					xw -= i;
					x3 = 0;
				}
				if ((256 - y3) < yw) {
                                        if (pixbuf->has_alpha) {
						if (a != 0) {
							r2 += r * (256 - y3);
							g2 += g * (256 - y3);
							b2 += b * (256 - y3);
						} else {
						}
                                                a2 += a * (256 - y3);
                                        } else {
						r2 += r * (256 - y3);
						g2 += g * (256 - y3);
						b2 += b * (256 - y3);
					}
					yw -= 256 - y3;
				} else {
                                        if (pixbuf->has_alpha) {
						if (a != 0) {
							r2 += r * yw;
							g2 += g * yw;
							b2 += b * yw;
						} else {
						}
                                                a2 += a * yw;
                                        } else {
						r2 += r * yw;
						g2 += g * yw;
						b2 += b * yw;
					}
					yw = 0;
				}
				y3 = 0;
				p2 += rowstride_old;
			}
                        r2 /= ww * hw;
                        g2 /= ww * hw;
                        b2 /= ww * hw;

                        *(p++) = r2 & 0xff;
                        *(p++) = g2 & 0xff;
                        *(p++) = b2 & 0xff;

                        if (pixbuf->has_alpha) {
                                a2 /= ww * hw;
                                *(p++) = a2 & 0xff;
                        }
		}
	}
	return gdk_pixbuf_new_from_art_pixbuf(retval);
#endif
}
#endif

GdkPixbuf*
gnome_pixbuf_scale(GdkPixbuf* gdk_pixbuf,
                   gint w, gint h)
{
        ArtPixBuf* retval;
        ArtPixBuf* pixbuf;
	unsigned char *data, *p, *p2, *p3, *data_old;
        long w_old, h_old;
        long rowstride_old;
	long x, y, w2, h2, x_old, y_old, x2, y2, i, x3, y3;
	long yw, xw, ww, hw, r, g, b, a, r2, g2, b2, a2;
	int new_channels;
	int new_rowstride;

	g_return_val_if_fail (gdk_pixbuf != NULL, NULL);

        pixbuf = gdk_pixbuf->art_pixbuf;

	g_return_val_if_fail (pixbuf->format == ART_PIX_RGB, NULL);
	g_return_val_if_fail (pixbuf->n_channels == 3 || pixbuf->n_channels == 4, NULL);
	g_return_val_if_fail (pixbuf->bits_per_sample == 8, NULL);
        g_return_val_if_fail (w > 0, NULL);
        g_return_val_if_fail (h > 0, NULL);

        w_old = pixbuf->width;
        h_old = pixbuf->height;

        /* optimization if no scaling is needed */
        if (w_old == w && h_old == h) {
                GdkPixbuf *copy;
                guchar* data_copy;

                data_copy = g_malloc(gdk_pixbuf_get_height(gdk_pixbuf) *
                                     gdk_pixbuf_get_rowstride(gdk_pixbuf));


                memcpy(data_copy,
                       gdk_pixbuf->art_pixbuf->pixels,
                       gdk_pixbuf->art_pixbuf->height * gdk_pixbuf->art_pixbuf->rowstride);

                copy = gdk_pixbuf_new_from_data(data_copy,
                                                gdk_pixbuf_get_format(gdk_pixbuf),
                                                gdk_pixbuf_get_has_alpha(gdk_pixbuf),
                                                gdk_pixbuf_get_width(gdk_pixbuf),
                                                gdk_pixbuf_get_height(gdk_pixbuf),
                                                gdk_pixbuf_get_rowstride(gdk_pixbuf),
                                                free_buffer, NULL);

                return copy;
        }

        rowstride_old = pixbuf->rowstride;
        data_old = pixbuf->pixels;

	/* Always align rows to 32-bit boundaries */

	new_channels = pixbuf->has_alpha ? 4 : 3;
        /* for debugging just use the simple rowstride */
	new_rowstride = w * new_channels; /* 4 * ((new_channels * w + 3) / 4); */

        data = g_malloc (h * new_rowstride);

        if (pixbuf->has_alpha)
		retval = art_pixbuf_new_rgba_dnotify (data, w, h, new_rowstride,
                                                      NULL, free_buffer);
	else
                retval = art_pixbuf_new_rgb_dnotify (data, w, h, new_rowstride,
                                                     NULL, free_buffer);

	p = data;

        /* Multiply stuff by 256 to avoid floating point. */

	ww = (w_old << 8) / w;  /* old-width-per-new-width * 256 */
	hw = (h_old << 8) / h;  /* old-height-per-new-height * 256 */
	h2 = h << 8;
	w2 = w << 8;

	for (y = 0; y < h2; y += 256) {
		y_old = (y * h_old) / h;
		y2 = y_old & 0xff;
		y_old >>= 8;
		for (x = 0; x < w2; x += 256) {
			x_old = (x * w_old) / w;
			x2 = x_old & 0xff;
			x_old >>= 8;
			i = x_old * (pixbuf->has_alpha ? 4 : 3) + (y_old * rowstride_old);
			p2 = data_old + i;

			r2 = g2 = b2 = a2 = 0;
			yw = hw;
			y3 = y2;
			while (yw) {
				xw = ww;
				x3 = x2;
				p3 = p2;
				r = g = b = a = 0;
				while (xw) {

					if ((256 - x3) < xw)
						i = 256 - x3;
					else
						i = xw;


                                        r += p3[0] * i;
                                        g += p3[1] * i;
                                        b += p3[2] * i;

                                        if (pixbuf->has_alpha) {
                                                a += p3[3] * i;
                                        }

					p3 += pixbuf->n_channels;
					xw -= i;
					x3 = 0;
				}
				if ((256 - y3) < yw) {
					r2 += r * (256 - y3);
					g2 += g * (256 - y3);
					b2 += b * (256 - y3);
                                        if (pixbuf->has_alpha) {
                                                a2 += a * (256 - y3);
                                        }
					yw -= 256 - y3;
				} else {
					r2 += r * yw;
					g2 += g * yw;
					b2 += b * yw;
                                        if (pixbuf->has_alpha) {
                                                a2 += a * yw;
                                        }
					yw = 0;
				}
				y3 = 0;
				p2 += rowstride_old;
			}
                        r2 /= ww * hw;
                        g2 /= ww * hw;
                        b2 /= ww * hw;

                        *(p++) = r2 & 0xff;
                        *(p++) = g2 & 0xff;
                        *(p++) = b2 & 0xff;

                        if (pixbuf->has_alpha) {
                                a2 /= ww * hw;
                                *(p++) = a2 & 0xff;
                        }
		}
	}

	return gdk_pixbuf_new_from_art_pixbuf(retval);
}

void
gnome_pixbuf_render(GdkPixbuf  *pixbuf,
                    GdkPixmap **pixmap,
                    GdkBitmap **mask_retval,
		    gint        alpha_threshold)
{
        GdkBitmap *mask = NULL;

        g_return_if_fail(pixbuf != NULL);

        /* generate mask */
        if (gdk_pixbuf_get_has_alpha(pixbuf)) {
                mask = gdk_pixmap_new(NULL,
                                      gdk_pixbuf_get_width(pixbuf),
                                      gdk_pixbuf_get_height(pixbuf),
                                      1);

                gdk_pixbuf_render_threshold_alpha(pixbuf, mask,
                                                  0, 0, 0, 0,
                                                  gdk_pixbuf_get_width(pixbuf),
                                                  gdk_pixbuf_get_height(pixbuf),
                                                  alpha_threshold);
        }

        /* Draw to pixmap */

        if (pixmap != NULL) {
                GdkGC* gc;

                *pixmap = gdk_pixmap_new(NULL,
                                         gdk_pixbuf_get_width(pixbuf),
                                         gdk_pixbuf_get_height(pixbuf),
                                         gdk_rgb_get_visual()->depth);

                gc = gdk_gc_new(*pixmap);

                gdk_gc_set_clip_mask(gc, mask);

                gdk_pixbuf_render_to_drawable(pixbuf, *pixmap,
                                              gc,
                                              0, 0, 0, 0,
                                              gdk_pixbuf_get_width(pixbuf),
                                              gdk_pixbuf_get_height(pixbuf),
                                              GDK_RGB_DITHER_NORMAL,
                                              0, 0);

                gdk_gc_unref(gc);
        }

        if (mask_retval)
                *mask_retval = mask;
        else
                gdk_bitmap_unref(mask);
}



/*
 * defunct code
 */
#if 0
/* code snippet to copy for getting base_color from a window */
/*
	if (window) {
		GtkStyle *style = gtk_widget_get_style(window);
		scale_base.r = style->bg[state].red >> 8;
		scale_base.g = style->bg[state].green >> 8;
		scale_base.b = style->bg[state].blue >> 8;
	}
*/
#endif

#if 0
static void
match_representation_to_mode  (GnomePixmap *gpixmap)
{

        if (gpixmap->flags & GNOME_PIXMAP_USE_PIXBUF) {
                /* Convert all pixmaps to pixbuf */
                guint i = 0;
                while (i < 5) {
                        if (gpixmap->image_data[i].pixmap != NULL) {
                                GdkPixbuf *pixbuf;
                                gint w, h;

                                gdk_window_get_size(gpixmap->image_data[i].pixmap,
                                                    &w, &h);

                                pixbuf = gdk_pixbuf_rgb_from_drawable(gpixmap->image_data[i].pixmap,
                                                                      0, 0,
                                                                      w, h);

                                /* This deletes the pixmap */
                                set_pixbuf(gpixmap, i, pixbuf, NULL);
                        }

                        ++i;
                }

        } else if (gpixmap->flags & GNOME_PIXMAP_USE_PIXMAP) {
                /* Convert all pixbuf to pixmap */
                guint i = 0;
                while (i < 5) {
                        if (gpixmap->image_data[i].pixbuf != NULL) {
                                GdkPixmap *pixmap = NULL;
                                GdkBitmap *mask = NULL;

                                gnome_pixbuf_render(gpixmap->image_data[i].pixbuf,
                                                    &pixmap,
                                                    &mask);
                                /* deletes the pixbuf */
                                set_pixmap(gpixmap, i, pixmap, mask);
                        }

                        ++i;
                }
        }
}
#endif

#if 0
static void
build_insensitive_pixmap      (GnomePixmap *gpixmap)
{
        /* FIXME This code is also in gtk/gtkpixmap.c */
        GdkGC *gc;
        GdkPixmap *normal;
        GdkBitmap *normal_mask;
        GdkPixmap *insensitive;
        gint w, h, x, y;
        GdkGCValues vals;
        GdkVisual *visual;
        GdkImage *image;
        GdkColorContext *cc;
        GdkColor color;
        GdkColormap *cmap;
        gint32 red, green, blue;
        GtkStyle *style;
        GtkWidget *widget;
        GdkColor c;
        gboolean failed;

        widget = GTK_WIDGET (gpixmap);

        g_return_if_fail(widget != NULL);

        normal = gpixmap->image_data[GTK_STATE_NORMAL].pixmap;
        normal_mask = gpixmap->image_data[GTK_STATE_NORMAL].mask;

        if (normal == NULL)
                return; /* Give up */

        g_return_if_fail(gpixmap->image_data[GTK_STATE_INSENSITIVE].pixmap == NULL);
        g_return_if_fail(gpixmap->image_data[GTK_STATE_INSENSITIVE].mask == NULL);

        gdk_window_get_size(normal, &w, &h);
        image = gdk_image_get(normal, 0, 0, w, h);
        insensitive = gdk_pixmap_new(widget->window, w, h, -1);
        gc = gdk_gc_new (normal);

        visual = gtk_widget_get_visual(widget);
        cmap = gtk_widget_get_colormap(widget);
        cc = gdk_color_context_new(visual, cmap);

        if ((cc->mode != GDK_CC_MODE_TRUE) &&
            (cc->mode != GDK_CC_MODE_MY_GRAY)) {

                gdk_draw_image(insensitive, gc, image, 0, 0, 0, 0, w, h);

                style = gtk_widget_get_style(widget);
                color = style->bg[0];
                gdk_gc_set_foreground (gc, &color);
                for (y = 0; y < h; y++) {
                        for (x = y % 2; x < w; x += 2) {
                                gdk_draw_point(insensitive, gc, x, y);
                        }
                }

        } else {

                gdk_gc_get_values(gc, &vals);
                style = gtk_widget_get_style(widget);

                color = style->bg[0];
                red = color.red;
                green = color.green;
                blue = color.blue;

                for (y = 0; y < h; y++) {
                        for (x = 0; x < w; x++) {
                                c.pixel = gdk_image_get_pixel(image, x, y);
                                gdk_color_context_query_color(cc, &c);
                                c.red = (((gint32)c.red - red) >> 1) + red;
                                c.green = (((gint32)c.green - green) >> 1) + green;
                                c.blue = (((gint32)c.blue - blue) >> 1) + blue;
                                c.pixel = gdk_color_context_get_pixel(cc, c.red, c.green, c.blue,
                                                                      &failed);
                                gdk_image_put_pixel(image, x, y, c.pixel);
                        }
                }

                for (y = 0; y < h; y++) {
                        for (x = y % 2; x < w; x += 2) {
                                c.pixel = gdk_image_get_pixel(image, x, y);
                                gdk_color_context_query_color(cc, &c);
                                c.red = (((gint32)c.red - red) >> 1) + red;
                                c.green = (((gint32)c.green - green) >> 1) + green;
                                c.blue = (((gint32)c.blue - blue) >> 1) + blue;
                                c.pixel = gdk_color_context_get_pixel(cc, c.red, c.green, c.blue,
                                                                      &failed);
                                gdk_image_put_pixel(image, x, y, c.pixel);
                        }
                }

                gdk_draw_image(insensitive, gc, image, 0, 0, 0, 0, w, h);
        }

        gpixmap->image_data[GTK_STATE_INSENSITIVE].pixmap = insensitive;
        gpixmap->image_data[GTK_STATE_INSENSITIVE].mask = normal_mask;

        /* Just ref the normal mask, don't create a new mask. */
        if (normal_mask != NULL) {
                gdk_bitmap_ref(normal_mask);
        }

        gdk_image_destroy(image);
        gdk_color_context_free(cc);
        gdk_gc_destroy(gc);
}
#endif

/*
 * Build insensitive
 */
#if 0
static void
build_insensitive_pixbuf      (GnomePixmap *gpixmap)
{
        GdkPixbuf *normal;
        GdkBitmap *normal_mask;
        gint32 red, green, blue;
        GdkPixbuf *insensitive;
        GdkColor color;
        GtkStyle *style;
        GtkWidget *widget;


        widget = GTK_WIDGET (gpixmap);

        g_return_if_fail(widget != NULL);

        normal = gpixmap->image_data[GTK_STATE_NORMAL].pixbuf;
        normal_mask = gpixmap->image_data[GTK_STATE_NORMAL].mask;

        if (normal == NULL)
                return; /* Give up */

        g_return_if_fail(gpixmap->image_data[GTK_STATE_INSENSITIVE].pixbuf == NULL);
        g_return_if_fail(gpixmap->image_data[GTK_STATE_INSENSITIVE].mask == NULL);

        style = gtk_widget_get_style(widget);

        color = style->bg[0];
        red = color.red/255;
        green = color.green/255;
        blue = color.blue/255;

        insensitive = gnome_pixbuf_build_insensitive(normal, red, green, blue);

        gpixmap->image_data[GTK_STATE_INSENSITIVE].pixbuf = insensitive;
        gpixmap->image_data[GTK_STATE_INSENSITIVE].mask   = normal_mask;

        /* Just ref the normal mask, don't create a new mask. */
        if (normal_mask != NULL) {
                gdk_bitmap_ref(normal_mask);
        }
}
#endif

#if 0
GdkPixbuf *
gnome_pixbuf_build_insensitive(GdkPixbuf* normal,
                               guchar red_bg, guchar green_bg, guchar blue_bg)
{
        gint32 red, green, blue;
        GdkPixbuf *insensitive;
        gint w, h, x, y;

        g_return_val_if_fail(normal != NULL, NULL);

        w = gdk_pixbuf_get_width(normal);
        h = gdk_pixbuf_get_height(normal);

        insensitive = gdk_pixbuf_new(gdk_pixbuf_get_format(normal),
                                     gdk_pixbuf_get_has_alpha(normal),
                                     gdk_pixbuf_get_bits_per_sample(normal),
                                     w, h);

        red = red_bg;
        green = green_bg;
        blue = blue_bg;

        /* Fade each pixel */
        for (y = 0; y < h; y++) {
                for (x = 0; x < w; x++) {
                        guchar* src_pixel;
                        guchar* dest_pixel;

                        src_pixel = normal->art_pixbuf->pixels +
                                y * normal->art_pixbuf->rowstride +
                                x * (normal->art_pixbuf->has_alpha ? 4 : 3);

                        dest_pixel = insensitive->art_pixbuf->pixels +
                                y * insensitive->art_pixbuf->rowstride +
                                x * (insensitive->art_pixbuf->has_alpha ? 4 : 3);

                        dest_pixel[0] = ((src_pixel[0] - red) >> 1) + red;
                        dest_pixel[1] = ((src_pixel[1] - green) >> 1) + green;
                        dest_pixel[2] = ((src_pixel[2] - blue) >> 1) + blue;
                }
        }

        /* Now do another fade pass, skipping every other pixel */
        for (y = 0; y < h; y++) {
                for (x = y % 2; x < w; x += 2) {
                        guchar* src_pixel;
                        guchar* dest_pixel;

                        src_pixel = insensitive->art_pixbuf->pixels +
                                y * insensitive->art_pixbuf->rowstride +
                                x * (insensitive->art_pixbuf->has_alpha ? 4 : 3);

                        dest_pixel = insensitive->art_pixbuf->pixels +
                                y * insensitive->art_pixbuf->rowstride +
                                x * (insensitive->art_pixbuf->has_alpha ? 4 : 3);

                        dest_pixel[0] = ((src_pixel[0] - red) >> 1) + red;
                        dest_pixel[1] = ((src_pixel[1] - green) >> 1) + green;
                        dest_pixel[2] = ((src_pixel[2] - blue) >> 1) + blue;
                }
        }

        return insensitive;
}
#endif
