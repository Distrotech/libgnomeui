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
#include "gnome-pixmap.h"
#include <stdio.h>
#include "libart_lgpl/art_affine.h"
#include "libart_lgpl/art_rgb_affine.h"
#include "libart_lgpl/art_rgb_rgba_affine.h"
#include <libgnomeuiP.h>

static void gnome_pixmap_class_init    (GnomePixmapClass *class);
static void gnome_pixmap_init          (GnomePixmap      *gpixmap);
static void gnome_pixmap_destroy       (GtkObject        *object);
static gint gnome_pixmap_expose        (GtkWidget        *widget,
					GdkEventExpose   *event);
static void gnome_pixmap_size_request  (GtkWidget        *widget,
                                        GtkRequisition    *requisition);
static void gnome_pixmap_set_property  (GObject            *object,
                                        guint               param_id,
                                        const GValue       *value,
                                        GParamSpec         *pspec);
static void gnome_pixmap_get_property  (GObject            *object,
                                        guint               param_id,
                                        GValue             *value,
                                        GParamSpec         *pspec);

static void clear_provided_state_image (GnomePixmap *gpixmap,
                                        GtkStateType state);
static void clear_generated_state_image(GnomePixmap *gpixmap,
                                        GtkStateType state);
static void clear_provided_image       (GnomePixmap *gpixmap);
static void clear_scaled_image         (GnomePixmap *gpixmap);
static void clear_all_images           (GnomePixmap *gpixmap);
static void clear_generated_images     (GnomePixmap *gpixmap);
static void generate_image             (GnomePixmap *gpixmap,
                                        GtkStateType state);
static void set_size                   (GnomePixmap *gpixmap,
                                        gint width, gint height);

static GdkPixbuf* saturate_and_pixelate(GdkPixbuf *pixbuf,
                                        gfloat saturation, gboolean pixelate);

static GdkBitmap* create_mask(GnomePixmap *gpixmap, GdkPixbuf *pixbuf);

static GtkMiscClass *parent_class = NULL;

#define INTENSITY(r, g, b) ((r) * 0.30 + (g) * 0.59 + (b) * 0.11)

enum {
	PROP_0,
	PROP_PIXBUF,
	PROP_PIXBUF_WIDTH,
	PROP_PIXBUF_HEIGHT,
	PROP_FILE,
	PROP_XPM_D,
	PROP_DRAW_MODE,
	PROP_ALPHA_THRESHOLD
};

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
			NULL,
			NULL,
			NULL
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

	gpixmap->provided_image = NULL;
        gpixmap->generated_scaled_image = NULL;
        gpixmap->generated_scaled_mask = NULL;

	gpixmap->width = -1;
	gpixmap->height = -1;
	gpixmap->alpha_threshold = 128;
	gpixmap->mode = GNOME_PIXMAP_SIMPLE;

        for (i = 0; i < 5; i ++) {
                gpixmap->generated[i].pixbuf = NULL;
                gpixmap->generated[i].mask = NULL;

                gpixmap->provided[i].pixbuf = NULL;
                gpixmap->provided[i].mask = NULL;
		gpixmap->provided[i].saturation = 1.0;
		gpixmap->provided[i].pixelate = FALSE;

		if (i == GTK_STATE_INSENSITIVE) {
			gpixmap->provided[i].saturation = 0.8;
			gpixmap->provided[i].pixelate = TRUE;
		}
        }
}

static void
gnome_pixmap_class_init (GnomePixmapClass *class)
{
        GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

        gobject_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;

	object_class->destroy = gnome_pixmap_destroy;

	parent_class = gtk_type_class (gtk_misc_get_type ());

	gobject_class->get_property = gnome_pixmap_get_property;
	gobject_class->set_property = gnome_pixmap_set_property;

        g_object_class_install_property
                (gobject_class,
                 PROP_PIXBUF,
                 g_param_spec_object ("pixbuf", NULL, NULL,
                                      GDK_TYPE_PIXBUF,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_PIXBUF_WIDTH,
                 g_param_spec_uint ("pixbuf_width", NULL, NULL,
                                    0, G_MAXINT, 0,
                                    (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_PIXBUF_HEIGHT,
                 g_param_spec_uint ("pixbuf_height", NULL, NULL,
                                    0, G_MAXINT, 0,
                                    (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILE,
                 g_param_spec_string ("file", NULL, NULL,
                                      NULL,
                                      G_PARAM_WRITABLE));
        g_object_class_install_property
                (gobject_class,
                 PROP_XPM_D,
                 g_param_spec_pointer ("xpm_d", NULL, NULL,
                                       G_PARAM_WRITABLE));
        g_object_class_install_property
                (gobject_class,
                 PROP_DRAW_MODE,
                 g_param_spec_enum ("draw_mode", NULL, NULL,
                                    GNOME_TYPE_PIXMAP_DRAW_MODE,
                                    GNOME_PIXMAP_SIMPLE,
                                    (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_ALPHA_THRESHOLD,
                 g_param_spec_int ("alpha_threshold", NULL, NULL,
                                   0, G_MAXINT, 128,
                                   (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	
	widget_class->expose_event = gnome_pixmap_expose;
        widget_class->size_request = gnome_pixmap_size_request;
}

static void
gnome_pixmap_set_property (GObject            *object,
                           guint               param_id,
                           const GValue       *value,
                           GParamSpec         *pspec)
{
        GnomePixmap *self;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GNOME_IS_PIXMAP (object));

	self = GNOME_PIXMAP (object);

	switch (param_id) {
	case PROP_PIXBUF:
                gnome_pixmap_set_pixbuf (self, (GdkPixbuf *) g_value_get_object (value));
		break;
	case PROP_PIXBUF_WIDTH:
		gnome_pixmap_set_pixbuf_size (self, g_value_get_uint (value),
					      self->height);
		break;
	case PROP_PIXBUF_HEIGHT:
		gnome_pixmap_set_pixbuf_size (self, self->width,
					      g_value_get_uint (value));
		break;
	case PROP_FILE: {
		GdkPixbuf *pixbuf;
                GError *error = NULL;
                const char *filename;

                filename = g_value_get_string (value);
		pixbuf = gdk_pixbuf_new_from_file (filename, &error);
                if (error != NULL) {
                        g_warning (G_STRLOC ": cannot open %s: %s",
                                   filename, error->message);
                        g_error_free (error);
                }
		if (pixbuf != NULL) {
			gnome_pixmap_set_pixbuf (self, pixbuf);
			gdk_pixbuf_unref (pixbuf);
		}
		break;
        }
	case PROP_XPM_D: {
		GdkPixbuf *pixbuf;
		pixbuf = gdk_pixbuf_new_from_xpm_data (g_value_get_pointer (value));
		if (pixbuf != NULL) {
			gnome_pixmap_set_pixbuf (self, pixbuf);
			gdk_pixbuf_unref (pixbuf);
		}
		break;
        }
	case PROP_DRAW_MODE:
		gnome_pixmap_set_draw_mode (self, g_value_get_enum (value));
		break;
	case PROP_ALPHA_THRESHOLD:
		gnome_pixmap_set_alpha_threshold (self, g_value_get_int (value));
		break;
	default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
gnome_pixmap_get_property (GObject            *object,
                           guint               param_id,
                           GValue             *value,
                           GParamSpec         *pspec)
{
        GnomePixmap *self;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GNOME_IS_PIXMAP (object));

	self = GNOME_PIXMAP (object);

	switch (param_id) {
	case PROP_PIXBUF:
		g_value_set_object (value, (GObject *) gnome_pixmap_get_pixbuf (self));
		break;
	case PROP_PIXBUF_WIDTH:
		g_value_set_int (value, self->width);
		break;
	case PROP_PIXBUF_HEIGHT:
		g_value_set_int (value, self->height);
		break;
	case PROP_DRAW_MODE:
		g_value_set_enum (value, gnome_pixmap_get_draw_mode (self));
		break;
	case PROP_ALPHA_THRESHOLD:
		g_value_set_int (value, gnome_pixmap_get_alpha_threshold (self));
		break;
	default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
gnome_pixmap_destroy (GtkObject *object)
{
	GnomePixmap *gpixmap;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (object));

	gpixmap = GNOME_PIXMAP (object);

        clear_all_images(gpixmap);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gnome_pixmap_size_request  (GtkWidget        *widget,
                            GtkRequisition    *requisition)
{
        /* We base size on the max of all provided images if w,h are -1
           else the scaled "main" image size (gpixmap->width, gpixmap->height) */
        gint maxwidth = 0;
        gint maxheight = 0;
        int i;
        GnomePixmap *gpixmap;

        gpixmap = GNOME_PIXMAP(widget);
        
        if (gpixmap->width >= 0 &&
            gpixmap->height >= 0) {
                /* shortcut if both sizes are set */
                maxwidth = gpixmap->width;
                maxheight = gpixmap->height;
        } else {
                if (gpixmap->provided_image != NULL) {
                        maxwidth = MAX(maxwidth, gdk_pixbuf_get_width(gpixmap->provided_image));
                        maxheight = MAX(maxheight, gdk_pixbuf_get_height(gpixmap->provided_image));
                }
                
                i = 0;
                
                while (i < 5) {
                        GdkPixbuf *pix = gpixmap->provided[i].pixbuf;
                        
                        if (pix != NULL) {
                                maxwidth = MAX(maxwidth, gdk_pixbuf_get_width(pix));
                                maxheight = MAX(maxheight, gdk_pixbuf_get_height(pix));
                        }
                        
                        ++i;
                }

                /* fix the size that was specified, if one was. */
                if (gpixmap->width >= 0)
                        maxwidth = gpixmap->width;
                
                if (gpixmap->height >= 0)
                        maxheight = gpixmap->height;
        }

        requisition->width = maxwidth + GTK_MISC (gpixmap)->xpad * 2;
        requisition->height = maxheight + GTK_MISC (gpixmap)->ypad * 2;
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

        /* Ensure we have this state, if we can think of a way to have
           it */
        generate_image (gpixmap, GTK_WIDGET_STATE (widget));
        
        draw_source = gpixmap->generated[GTK_WIDGET_STATE (widget)].pixbuf;
        draw_mask = gpixmap->generated[GTK_WIDGET_STATE (widget)].mask;
        
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


		dest_source = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
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
set_state_pixbuf(GnomePixmap* gpixmap, GtkStateType state, GdkPixbuf* pixbuf, GdkBitmap* mask)
{
        clear_generated_state_image(gpixmap, state);
        clear_provided_state_image(gpixmap, state);

        g_return_if_fail(gpixmap->provided[state].pixbuf == NULL);
        g_return_if_fail(gpixmap->provided[state].mask == NULL);

        gpixmap->provided[state].pixbuf = pixbuf;
        if (pixbuf)
                gdk_pixbuf_ref(pixbuf);

        gpixmap->provided[state].mask = mask;
        if (mask)
                gdk_bitmap_ref(mask);

        if (GTK_WIDGET_VISIBLE(gpixmap)) {
                gtk_widget_queue_resize(GTK_WIDGET(gpixmap));
                gtk_widget_queue_clear(GTK_WIDGET(gpixmap));
        }
}

static void
set_state_pixbufs(GnomePixmap* gpixmap, GdkPixbuf* pixbufs[5], GdkBitmap* masks[5])
{
        guint i;

        i = 0;
        while (i < 5) {

                set_state_pixbuf(gpixmap,
                                 i,
                                 pixbufs ? pixbufs[i] : NULL,
                                 masks ? masks[i] : NULL);
                ++i;
        }
}

static void
set_pixbuf(GnomePixmap* gpixmap, GdkPixbuf* pixbuf)
{
        if (pixbuf == gpixmap->provided_image)
                return;
        
        clear_generated_images(gpixmap);
        clear_provided_image(gpixmap);

        g_return_if_fail(gpixmap->provided_image == NULL);

        gpixmap->provided_image = pixbuf;

        if (pixbuf)
                gdk_pixbuf_ref(pixbuf);

        if (GTK_WIDGET_VISIBLE(gpixmap)) {
                gtk_widget_queue_resize(GTK_WIDGET(gpixmap));
                gtk_widget_queue_clear(GTK_WIDGET(gpixmap));
        }
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

        set_pixbuf(retval, pixbuf);

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
        
        set_size(gpixmap, width, height);
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
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (pixbuf != NULL);

        set_pixbuf(gpixmap, pixbuf);
}


/**
 * gnome_pixmap_get_pixbuf:
 * @gpixmap: A @GnomePixmap.
 *
 * Gets the current image used by @gpixmap, if you have set the
 * pixbuf. If you've only set the value for particular states,
 * then this won't return anything.
 *
 * Return value: A pixbuf.
 **/
GdkPixbuf *
gnome_pixmap_get_pixbuf (GnomePixmap      *gpixmap)
{
	g_return_val_if_fail (gpixmap != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_PIXMAP (gpixmap), NULL);

	return gpixmap->provided_image;
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
        
        set_state_pixbuf (gpixmap, state, pixbuf, mask);
}

/**
 * gnome_pixmap_set_state_pixbufs
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
gnome_pixmap_set_state_pixbufs (GnomePixmap *gpixmap,
                                GdkPixbuf   *pixbufs[5],
                                GdkBitmap   *masks[5])
{
        g_return_if_fail(gpixmap != NULL);
        g_return_if_fail(GNOME_IS_PIXMAP (gpixmap));
        
        set_state_pixbufs(gpixmap, pixbufs, masks);
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

        clear_all_images(gpixmap);

        if (GTK_WIDGET_VISIBLE(gpixmap)) {
                gtk_widget_queue_resize(GTK_WIDGET(gpixmap));
                gtk_widget_queue_clear(GTK_WIDGET(gpixmap));
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

	gpixmap->provided[state].saturation = saturation;
	gpixmap->provided[state].pixelate = pixelate;

        if (GTK_WIDGET_VISIBLE(gpixmap)) {
                gtk_widget_queue_clear(GTK_WIDGET(gpixmap));
        }
}

/**
 * gnome_pixmap_get_draw_vals:
 * @gpixmap: A @GnomePixmap.
 * @state: The the state to set the modifications to
 * @saturation: return location for the saturation offset
 * @pixelate: return value for whether to draw the insensitive stipple
 *
 * Retrieve the values set by gnome_pixmap_set_draw_vals(). Either
 * return location can be NULL if you don't care about it.
 *
 **/
void
gnome_pixmap_get_draw_vals           (GnomePixmap      *gpixmap,
                                      GtkStateType      state,
                                      gfloat           *saturation,
                                      gboolean         *pixelate)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (state >= 0 && state < 5);

        if (saturation)
                *saturation = gpixmap->provided[state].saturation;
        if (pixelate)
                *pixelate = gpixmap->provided[state].pixelate;
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
			    GnomePixmapDrawMode mode)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));

	if (gpixmap->mode == mode)
		return;
        
	gpixmap->mode = mode;
	clear_generated_images (gpixmap);

        if (GTK_WIDGET_VISIBLE (gpixmap)) {
                gtk_widget_queue_resize (GTK_WIDGET (gpixmap));
		gtk_widget_queue_clear (GTK_WIDGET (gpixmap));
	}
}

/**
 * gnome_pixmap_get_draw_mode:
 * @gpixmap: A @GnomePixmap.
 *
 * Gets the current draw mode.
 *
 * Return value: The current @GnomePixmapDrawMode setting.
 **/
GnomePixmapDrawMode
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

	clear_generated_images (gpixmap);
        
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
 * Internal functions
 */

static void
clear_provided_state_image (GnomePixmap *gpixmap,
                            GtkStateType state)
{
        if (gpixmap->provided[state].pixbuf != NULL) {
                gdk_pixbuf_unref(gpixmap->provided[state].pixbuf);
                gpixmap->provided[state].pixbuf = NULL;
        }
        
        if (gpixmap->provided[state].mask != NULL) {
                gdk_bitmap_unref(gpixmap->provided[state].mask);
                gpixmap->provided[state].mask = NULL;
        }
}

static void
clear_generated_state_image(GnomePixmap *gpixmap,
                            GtkStateType state)
{
        if (gpixmap->generated[state].pixbuf != NULL) {
                gdk_pixbuf_unref(gpixmap->generated[state].pixbuf);
                gpixmap->generated[state].pixbuf = NULL;
        }

        if (gpixmap->generated[state].mask != NULL) {
                gdk_bitmap_unref(gpixmap->generated[state].mask);
                gpixmap->generated[state].mask = NULL;
        }
}
        
static void
clear_provided_image (GnomePixmap *gpixmap)
{
        if (gpixmap->provided_image) {
                gdk_pixbuf_unref(gpixmap->provided_image);
                gpixmap->provided_image = NULL;
        }
}

static void
clear_scaled_image (GnomePixmap *gpixmap)
{
        if (gpixmap->generated_scaled_image) {
                gdk_pixbuf_unref(gpixmap->generated_scaled_image);
                gpixmap->generated_scaled_image = NULL;
        }
        if (gpixmap->generated_scaled_mask) {
                gdk_bitmap_unref(gpixmap->generated_scaled_mask);
                gpixmap->generated_scaled_mask = NULL;
        }
}

static void
clear_all_images (GnomePixmap *gpixmap)
{
        guint i;

        i = 0;
        while (i < 5) {

                clear_provided_state_image(gpixmap, i);

                ++i;
        }
        
        clear_generated_images(gpixmap);
        clear_provided_image(gpixmap);
}

static void
clear_generated_images (GnomePixmap *gpixmap)
{
        guint i;

        i = 0;
        while (i < 5) {

                clear_generated_state_image(gpixmap, i);

                ++i;
        }

        clear_scaled_image(gpixmap);
}

static void
generate_image (GnomePixmap *gpixmap,
                GtkStateType state)
{
        /* See if this image is already generated */
        if (gpixmap->generated[state].pixbuf != NULL)
                return;

        g_return_if_fail(gpixmap->generated[state].pixbuf == NULL);
        g_return_if_fail(gpixmap->generated[state].mask == NULL);
        
        /* To generate an image, we first use the provided image for a given
           state, if any; if not we use the gpixmap->provided_image; if that
           doesn't exist then we bail out. */
           
        if (gpixmap->provided[state].pixbuf != NULL) {
                gint width = gpixmap->width;
                gint height = gpixmap->height;
                GdkPixbuf *scaled;
                GdkPixbuf *generated;

                if (width >= 0 || height >= 0) {
                        if (width < 0)
                                width = gdk_pixbuf_get_width(gpixmap->provided[state].pixbuf);

                        if (height < 0)
                                height = gdk_pixbuf_get_height(gpixmap->provided[state].pixbuf);
                        
                        scaled = gdk_pixbuf_scale_simple (gpixmap->provided[state].pixbuf,
                                                          gpixmap->width,
                                                          gpixmap->height,
                                                          ART_FILTER_BILINEAR);
                } else {
                        /* just copy */
                        scaled = gpixmap->provided[state].pixbuf;
                        gdk_pixbuf_ref(scaled);
                }
                        
                generated = saturate_and_pixelate(scaled,
                                                  gpixmap->provided[state].saturation,
                                                  gpixmap->provided[state].pixelate);
                
                gpixmap->generated[state].pixbuf = generated;

                if ((scaled == gpixmap->provided[state].pixbuf) &&
                    gpixmap->provided[state].mask) {
                        /* use provided mask if it exists
                           and we did not have to scale */
                        gpixmap->generated[state].mask =
                                gpixmap->provided[state].mask;
                        gdk_bitmap_ref(gpixmap->generated[state].mask);
                } else {
                        /* create mask */
                        gpixmap->generated[state].mask =
                                create_mask(gpixmap, generated);
                }

                /* Drop intermediate image */
                gdk_pixbuf_unref(scaled);
        }
        
        /* Ensure we've generated the scaled image
           if we have an original image */
        if (gpixmap->provided_image != NULL &&
            gpixmap->generated_scaled_image == NULL) {
                gint width = gpixmap->width;
                gint height = gpixmap->height;
                
                if (width < 0)
                        width = gdk_pixbuf_get_width(gpixmap->provided_image);
                if (height < 0)
                        height = gdk_pixbuf_get_height(gpixmap->provided_image);
                
                if (gpixmap->width < 0 && /* orig w/h, not the "fixed" ones */
                    gpixmap->height < 0) {
                        /* Just copy */
                        gpixmap->generated_scaled_image = gpixmap->provided_image;
                        gdk_pixbuf_ref(gpixmap->generated_scaled_image);
                } else {
                        gpixmap->generated_scaled_image =
                                gdk_pixbuf_scale_simple (gpixmap->provided_image,
                                                         width,
                                                         height,
                                                         ART_FILTER_BILINEAR);
                }

                gpixmap->generated_scaled_mask =
                        create_mask(gpixmap, gpixmap->generated_scaled_image);
        }

        /* Now we generate the per-state image from the scaled
           copy of the provided image */
        if (gpixmap->generated_scaled_image != NULL) {
                GdkPixbuf *generated;

                g_return_if_fail(gpixmap->generated_scaled_mask);
                
                generated = saturate_and_pixelate(gpixmap->generated_scaled_image,
                                                  gpixmap->provided[state].saturation,
                                                  gpixmap->provided[state].pixelate);
                
                gpixmap->generated[state].pixbuf = generated;

                if (gpixmap->provided[state].mask) {
                        /* use provided mask if it exists */
                        gpixmap->generated[state].mask =
                                gpixmap->provided[state].mask;
                        gdk_bitmap_ref(gpixmap->generated[state].mask);
                } else {
                        /* If we just copied the generated_scaled_image
                           then also copy the mask */
                        if (generated == gpixmap->generated_scaled_image) {
                                gpixmap->generated[state].mask =
                                        gpixmap->generated_scaled_mask;
                                gdk_bitmap_ref(gpixmap->generated_scaled_mask);
                        } else {
                                /* create mask */
                                gpixmap->generated[state].mask =
                                        create_mask(gpixmap, generated);
                        }
                }
        }

        /* If we didn't have a provided_image or a provided image for the
           particular state, then we have no way to generate an image.
        */
}

static void
set_size (GnomePixmap *gpixmap,
          gint width, gint height)
{
        if (gpixmap->width == width &&
            gpixmap->height == height)
                return;
        
	/* FIXME: if old_width == -1 and pixbuf->width == width, we can just set
	 * the values and return as an optomization step. */
        
        clear_generated_images(gpixmap);
        
        gpixmap->width = width;
        gpixmap->height = height;
        
        if (GTK_WIDGET_VISIBLE (gpixmap)) {
                if ((GTK_WIDGET (gpixmap)->requisition.width != width) ||
                    (GTK_WIDGET (gpixmap)->requisition.height != height))
                        gtk_widget_queue_resize (GTK_WIDGET (gpixmap));
                else
                        gtk_widget_queue_clear (GTK_WIDGET (gpixmap));
	}
}

static GdkBitmap*
create_mask(GnomePixmap *gpixmap, GdkPixbuf *pixbuf)
{
        GdkBitmap *mask;
        gint width = gdk_pixbuf_get_width(pixbuf);
        gint height = gdk_pixbuf_get_height(pixbuf);
        
        mask = gdk_pixmap_new (NULL, width, height, 1);

        gdk_pixbuf_render_threshold_alpha(pixbuf,
                                          mask,
                                          0, 0, 0, 0,
                                          width, height,
                                          gpixmap->alpha_threshold);
                
        
        return mask;
}
 
static GdkPixbuf*
saturate_and_pixelate(GdkPixbuf *pixbuf, gfloat saturation, gboolean pixelate)
{
        if (saturation == 1.0) {
                gdk_pixbuf_ref(pixbuf);
                return pixbuf;
        } else {
		GdkPixbuf *target;
		gint i, j;
		gint width, height, has_alpha, rowstride;
		guchar *target_pixels;
		guchar *original_pixels;
		guchar *current_pixel;
		guchar intensity;

		has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
		width = gdk_pixbuf_get_width (pixbuf);
		height = gdk_pixbuf_get_height (pixbuf);
		rowstride = gdk_pixbuf_get_rowstride (pixbuf);
                
		target = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
					 has_alpha,
					 gdk_pixbuf_get_bits_per_sample (pixbuf),
					 width, height);
                
		target_pixels = gdk_pixbuf_get_pixels (target);
		original_pixels = gdk_pixbuf_get_pixels (pixbuf);

		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				current_pixel = original_pixels + i*rowstride + j*(has_alpha?4:3);
				intensity = INTENSITY (*(current_pixel), *(current_pixel + 1), *(current_pixel + 2));
				if (pixelate && (i+j)%2 == 0) {
					*(target_pixels + i*rowstride + j*(has_alpha?4:3)) = intensity/2 + 127;
					*(target_pixels + i*rowstride + j*(has_alpha?4:3) + 1) = intensity/2 + 127;
					*(target_pixels + i*rowstride + j*(has_alpha?4:3) + 2) = intensity/2 + 127;
				} else if (pixelate) {
#define DARK_FACTOR 0.7
					*(target_pixels + i*rowstride + j*(has_alpha?4:3)) =
						(guchar) (((1.0 - saturation) * intensity
							   + saturation * (*(current_pixel)))) * DARK_FACTOR;
					*(target_pixels + i*rowstride + j*(has_alpha?4:3) + 1) =
						(guchar) (((1.0 - saturation) * intensity
							   + saturation * (*(current_pixel + 1)))) * DARK_FACTOR;
					*(target_pixels + i*rowstride + j*(has_alpha?4:3) + 2) =
						(guchar) (((1.0 - saturation) * intensity
							   + saturation * (*(current_pixel + 2)))) * DARK_FACTOR;
				} else {
					*(target_pixels + i*rowstride + j*(has_alpha?4:3)) =
						(guchar) ((1.0 - saturation) * intensity
							  + saturation * (*(current_pixel)));
					*(target_pixels + i*rowstride + j*(has_alpha?4:3) + 1) =
						(guchar) ((1.0 - saturation) * intensity
							  + saturation * (*(current_pixel + 1)));
					*(target_pixels + i*rowstride + j*(has_alpha?4:3) + 2) =
						(guchar) ((1.0 - saturation) * intensity
							  + saturation * (*(current_pixel + 2)));
				}
				if (has_alpha)
					*(target_pixels + i*rowstride + j*(has_alpha?4:3) + 3) = *(original_pixels + i*rowstride + j*(has_alpha?4:3) + 3);
			}
		}

                return target;
	}
}
