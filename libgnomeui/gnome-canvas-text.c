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
/* Text item type for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas
 * widget.  Tk is copyrighted by the Regents of the University of California,
 * Sun Microsystems, and other parties.
 *
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <config.h>
#include <math.h>
#include "gnome-canvas-text.h"
#include <pango/pangoft2.h>

#include "libart_lgpl/art_affine.h"
#include "libart_lgpl/art_rgb_a_affine.h"
#include "libart_lgpl/art_rgb.h"
#include "libart_lgpl/art_rgb_bitmap_affine.h"
#include "gnome-canvas-util.h"



/* Object argument IDs */
enum {
	PROP_0,
	PROP_TEXT,
	PROP_X,
	PROP_Y,
	PROP_FONT,
	PROP_FONT_DESC,
	PROP_ANCHOR,
	PROP_JUSTIFICATION,
	PROP_CLIP_WIDTH,
	PROP_CLIP_HEIGHT,
	PROP_CLIP,
	PROP_X_OFFSET,
	PROP_Y_OFFSET,
	PROP_FILL_COLOR,
	PROP_FILL_COLOR_GDK,
	PROP_FILL_COLOR_RGBA,
	PROP_FILL_STIPPLE,
	PROP_TEXT_WIDTH,
	PROP_TEXT_HEIGHT
};


static void gnome_canvas_text_class_init (GnomeCanvasTextClass *class);
static void gnome_canvas_text_init (GnomeCanvasText *text);
static void gnome_canvas_text_destroy (GtkObject *object);
static void gnome_canvas_text_set_property (GObject            *object,
					    guint               param_id,
					    const GValue       *value,
					    GParamSpec         *pspec,
					    const gchar        *trailer);
static void gnome_canvas_text_get_property (GObject            *object,
					    guint               param_id,
					    GValue             *value,
					    GParamSpec         *pspec,
					    const gchar        *trailer);

static void gnome_canvas_text_update (GnomeCanvasItem *item, double *affine,
				      ArtSVP *clip_path, int flags);
static void gnome_canvas_text_realize (GnomeCanvasItem *item);
static void gnome_canvas_text_unrealize (GnomeCanvasItem *item);
static void gnome_canvas_text_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
				    int x, int y, int width, int height);
static double gnome_canvas_text_point (GnomeCanvasItem *item, double x, double y, int cx, int cy,
				       GnomeCanvasItem **actual_item);
static void gnome_canvas_text_bounds (GnomeCanvasItem *item,
				      double *x1, double *y1, double *x2, double *y2);
static void gnome_canvas_text_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf);

static GnomeCanvasItemClass *parent_class;



/**
 * gnome_canvas_text_get_type:
 * @void: 
 * 
 * Registers the &GnomeCanvasText class if necessary, and returns the type ID
 * associated to it.
 * 
 * Return value: The type ID of the &GnomeCanvasText class.
 **/
GtkType
gnome_canvas_text_get_type (void)
{
	static GtkType text_type = 0;

	if (!text_type) {
		GtkTypeInfo text_info = {
			"GnomeCanvasText",
			sizeof (GnomeCanvasText),
			sizeof (GnomeCanvasTextClass),
			(GtkClassInitFunc) gnome_canvas_text_class_init,
			(GtkObjectInitFunc) gnome_canvas_text_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		text_type = gtk_type_unique (gnome_canvas_item_get_type (), &text_info);
	}

	return text_type;
}

/* Class initialization function for the text item */
static void
gnome_canvas_text_class_init (GnomeCanvasTextClass *class)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	gobject_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	gobject_class->set_property = gnome_canvas_text_set_property;
	gobject_class->get_property = gnome_canvas_text_get_property;

        g_object_class_install_property
                (gobject_class,
                 PROP_TEXT,
                 g_param_spec_string ("text", NULL, NULL,
                                      NULL,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_X,
                 g_param_spec_double ("x", NULL, NULL,
				      G_MINDOUBLE, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_Y,
                 g_param_spec_double ("y", NULL, NULL,
				      G_MINDOUBLE, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_FONT,
                 g_param_spec_string ("font", NULL, NULL,
                                      NULL,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_FONT_DESC,
                 g_param_spec_boxed ("font_desc", NULL, NULL,
				     GTK_TYPE_PANGO_FONT_DESCRIPTION,
				     (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_ANCHOR,
                 g_param_spec_enum ("anchor", NULL, NULL,
                                    GTK_TYPE_ANCHOR_TYPE,
                                    GTK_ANCHOR_NW,
                                    (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_JUSTIFICATION,
                 g_param_spec_enum ("justification", NULL, NULL,
                                    GTK_TYPE_JUSTIFICATION,
                                    GTK_JUSTIFY_LEFT,
                                    (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_CLIP_WIDTH,
                 g_param_spec_double ("clip_width", NULL, NULL,
				      G_MINDOUBLE, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_CLIP_HEIGHT,
                 g_param_spec_double ("clip_height", NULL, NULL,
				      G_MINDOUBLE, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_CLIP,
                 g_param_spec_boolean ("clip", NULL, NULL,
				       FALSE,
				       (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_X_OFFSET,
                 g_param_spec_double ("x_offset", NULL, NULL,
				      G_MINDOUBLE, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_Y_OFFSET,
                 g_param_spec_double ("y_offset", NULL, NULL,
				      G_MINDOUBLE, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_COLOR,
                 g_param_spec_string ("fill_color", NULL, NULL,
                                      NULL,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_COLOR_GDK,
                 g_param_spec_boxed ("fill_color_gdk", NULL, NULL,
				     GTK_TYPE_GDK_COLOR,
				     (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_COLOR_RGBA,
                 g_param_spec_uint ("fill_color_rgba", NULL, NULL,
				    0, G_MAXUINT, 0,
				    (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_STIPPLE,
                 g_param_spec_object ("fill_stipple", NULL, NULL,
                                      GDK_TYPE_DRAWABLE,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_TEXT_WIDTH,
                 g_param_spec_double ("text_width", NULL, NULL,
				      0.0, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_TEXT_HEIGHT,
                 g_param_spec_double ("text_height", NULL, NULL,
				      0.0, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	object_class->destroy = gnome_canvas_text_destroy;

	item_class->update = gnome_canvas_text_update;
	item_class->realize = gnome_canvas_text_realize;
	item_class->unrealize = gnome_canvas_text_unrealize;
	item_class->draw = gnome_canvas_text_draw;
	item_class->point = gnome_canvas_text_point;
	item_class->bounds = gnome_canvas_text_bounds;
	item_class->render = gnome_canvas_text_render;
}

/* Object initialization function for the text item */
static void
gnome_canvas_text_init (GnomeCanvasText *text)
{
	text->x = 0.0;
	text->y = 0.0;
	text->anchor = GTK_ANCHOR_CENTER;
	text->justification = GTK_JUSTIFY_LEFT;
	text->clip_width = 0.0;
	text->clip_height = 0.0;
	text->xofs = 0.0;
	text->yofs = 0.0;
	text->layout = NULL;
}

/* Destroy handler for the text item */
static void
gnome_canvas_text_destroy (GtkObject *object)
{
	GnomeCanvasText *text;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_TEXT (object));

	text = GNOME_CANVAS_TEXT (object);

	/* remember, destroy can be run multiple times! */

	if (text->text)
		g_free (text->text);
	text->text = NULL;

	if (text->layout)
	  g_object_unref (G_OBJECT (text->layout));
	text->layout = NULL;

	if (text->font_desc)
		pango_font_description_free (text->font_desc);
	text->font_desc = NULL;

	if (text->stipple)
		gdk_bitmap_unref (text->stipple);
	text->stipple = NULL;

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
get_bounds_item_relative (GnomeCanvasText *text, double *px1, double *py1, double *px2, double *py2)
{
	GnomeCanvasItem *item;
	double x, y;
	double clip_x, clip_y;

	item = GNOME_CANVAS_ITEM (text);

	x = text->x;
	y = text->y;

	clip_x = x;
	clip_y = y;

	/* Calculate text dimensions */

	if (text->layout)
	        pango_layout_get_pixel_size (text->layout,
					     &text->max_width,
					     &text->height);
	else
		text->height = 0;

	/* Anchor text */

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		x -= text->max_width / 2;
		clip_x -= text->clip_width / 2;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		x -= text->max_width;
		clip_x -= text->clip_width;
		break;
	}

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		break;

	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		y -= text->height / 2;
		clip_y -= text->clip_height / 2;
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		y -= text->height;
		clip_y -= text->clip_height;
		break;
	}

	/* Bounds */

	if (text->clip) {
		/* maybe do bbox intersection here? */
		*px1 = clip_x;
		*py1 = clip_y;
		*px2 = clip_x + text->clip_width;
		*py2 = clip_y + text->clip_height;
	} else {
		*px1 = x;
		*py1 = y;
		*px2 = x + text->max_width;
		*py2 = y + text->height;
	}
}

static void
get_bounds (GnomeCanvasText *text, double *px1, double *py1, double *px2, double *py2)
{
	GnomeCanvasItem *item;
	double wx, wy;

	item = GNOME_CANVAS_ITEM (text);

	/* Get canvas pixel coordinates for text position */

	wx = text->x;
	wy = text->y;
	gnome_canvas_item_i2w (item, &wx, &wy);
	gnome_canvas_w2c (item->canvas, wx + text->xofs, wy + text->yofs, &text->cx, &text->cy);

	/* Get canvas pixel coordinates for clip rectangle position */

	gnome_canvas_w2c (item->canvas, wx, wy, &text->clip_cx, &text->clip_cy);
	text->clip_cwidth = text->clip_width * item->canvas->pixels_per_unit;
	text->clip_cheight = text->clip_height * item->canvas->pixels_per_unit;

	/* Calculate text dimensions */

	if (text->layout)
	        pango_layout_get_pixel_size (text->layout,
					     &text->max_width,
					     &text->height);
	else
		text->height = 0;

	/* Anchor text */

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		text->cx -= text->max_width / 2;
		text->clip_cx -= text->clip_cwidth / 2;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		text->cx -= text->max_width;
		text->clip_cx -= text->clip_cwidth;
		break;
	}

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		break;

	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		text->cy -= text->height / 2;
		text->clip_cy -= text->clip_cheight / 2;
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		text->cy -= text->height;
		text->clip_cy -= text->clip_cheight;
		break;
	}

	/* Bounds */

	if (text->clip) {
		*px1 = text->clip_cx;
		*py1 = text->clip_cy;
		*px2 = text->clip_cx + text->clip_cwidth;
		*py2 = text->clip_cy + text->clip_cheight;
	} else {
		*px1 = text->cx;
		*py1 = text->cy;
		*px2 = text->cx + text->max_width;
		*py2 = text->cy + text->height;
	}
}

/* Recalculates the bounding box of the text item.  The bounding box is defined
 * by the text's extents if the clip rectangle is disabled.  If it is enabled,
 * the bounding box is defined by the clip rectangle itself.
 */
static void
recalc_bounds (GnomeCanvasText *text)
{
	GnomeCanvasItem *item;

	item = GNOME_CANVAS_ITEM (text);

	get_bounds (text, &item->x1, &item->y1, &item->x2, &item->y2);

	gnome_canvas_group_child_bounds (GNOME_CANVAS_GROUP (item->parent), item);
}

/* Convenience function to set the text's GC's foreground color */
static void
set_text_gc_foreground (GnomeCanvasText *text)
{
	GdkColor c;

	if (!text->gc)
		return;

	c.pixel = text->pixel;
	gdk_gc_set_foreground (text->gc, &c);
}

/* Sets the stipple pattern for the text */
static void
set_stipple (GnomeCanvasText *text, GdkBitmap *stipple, int reconfigure)
{
	if (text->stipple && !reconfigure)
		gdk_bitmap_unref (text->stipple);

	text->stipple = stipple;
	if (stipple && !reconfigure)
		gdk_bitmap_ref (stipple);

	if (text->gc) {
		if (stipple) {
			gdk_gc_set_stipple (text->gc, stipple);
			gdk_gc_set_fill (text->gc, GDK_STIPPLED);
		} else
			gdk_gc_set_fill (text->gc, GDK_SOLID);
	}
}

/* Set_arg handler for the text item */
static void
gnome_canvas_text_set_property (GObject            *object,
				guint               param_id,
				const GValue       *value,
				GParamSpec         *pspec,
				const gchar        *trailer)
{
	GnomeCanvasItem *item;
	GnomeCanvasText *text;
	GdkColor color = { 0, 0, 0, 0, };
	GdkColor *pcolor;
	gboolean color_changed;
	int have_pixel;
	PangoAlignment align;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_TEXT (object));

	item = GNOME_CANVAS_ITEM (object);
	text = GNOME_CANVAS_TEXT (object);

	color_changed = FALSE;
	have_pixel = FALSE;
	

	if (!text->layout) {
	        PangoContext *gtk_context, *context;
		gtk_context = gtk_widget_get_pango_context (GTK_WIDGET (item->canvas));
		
	        if (item->canvas->aa)  {
		        gchar *lang;
		        context = pango_ft2_get_context ();
			lang = pango_context_get_lang (gtk_context);
			pango_context_set_lang (context, lang);
			g_free (lang);
			pango_context_set_base_dir (context,
						    pango_context_get_base_dir (gtk_context));
		} else 
	                context = gtk_context;

		text->layout = pango_layout_new (context);
		
	        if (item->canvas->aa)
		        g_object_unref (G_OBJECT (context));
	}

	switch (param_id) {
	case PROP_TEXT:
		if (text->text)
			g_free (text->text);

		text->text = g_value_dup_string (value);
		pango_layout_set_text (text->layout, text->text, -1);
		recalc_bounds (text);
		break;

	case PROP_X:
		text->x = g_value_get_double (value);
		recalc_bounds (text);
		break;

	case PROP_Y:
		text->y = g_value_get_double (value);
		recalc_bounds (text);
		break;

	case PROP_FONT: {
		const char *font_name;

		font_name = g_value_get_string (value);
		if (font_name) {
			if (text->font_desc)
				pango_font_description_free (text->font_desc);

			text->font_desc = pango_font_description_from_string (font_name);

			pango_layout_set_font_description (text->layout, text->font_desc);
			recalc_bounds (text);
		}
		break;
	}

	case PROP_FONT_DESC:
		if (text->font_desc)
			pango_font_description_free (text->font_desc);

		text->font_desc = g_value_get_as_pointer (value);

		pango_layout_set_font_description (text->layout, text->font_desc);
		recalc_bounds (text);
		break;

	case PROP_ANCHOR:
		text->anchor = g_value_get_enum (value);
		recalc_bounds (text);
		break;

	case PROP_JUSTIFICATION:
		text->justification = g_value_get_enum (value);

		switch (text->justification) {
		case GTK_JUSTIFY_LEFT:
		        align = PANGO_ALIGN_LEFT;
			break;
		case GTK_JUSTIFY_CENTER:
		        align = PANGO_ALIGN_CENTER;
			break;
		case GTK_JUSTIFY_RIGHT:
		        align = PANGO_ALIGN_RIGHT;
			break;
		default:
		        /* GTK_JUSTIFY_FILL isn't supported yet. */
		        align = PANGO_ALIGN_LEFT;
			break;
		}		  
		pango_layout_set_alignment (text->layout, align);
		recalc_bounds (text);
		break;

	case PROP_CLIP_WIDTH:
		text->clip_width = fabs (g_value_get_double (value));
		recalc_bounds (text);
		break;

	case PROP_CLIP_HEIGHT:
		text->clip_height = fabs (g_value_get_double (value));
		recalc_bounds (text);
		break;

	case PROP_CLIP:
		text->clip = g_value_get_boolean (value);
		recalc_bounds (text);
		break;

	case PROP_X_OFFSET:
		text->xofs = g_value_get_double (value);
		recalc_bounds (text);
		break;

	case PROP_Y_OFFSET:
		text->yofs = g_value_get_double (value);
		recalc_bounds (text);
		break;

        case PROP_FILL_COLOR: {
		const char *color_name;

		color_name = g_value_get_string (value);
		if (color_name) {
			gdk_color_parse (color_name, &color);

			text->rgba = ((color.red & 0xff00) << 16 |
				      (color.green & 0xff00) << 8 |
				      (color.blue & 0xff00) |
				      0xff);
			color_changed = TRUE;
		}
		break;
	}

	case PROP_FILL_COLOR_GDK:
		pcolor = g_value_get_boxed (value);
		if (pcolor) {
		    GdkColormap *colormap;

		    color = *pcolor;
		    colormap = gtk_widget_get_colormap (GTK_WIDGET (item->canvas));
		    gdk_rgb_find_color (colormap, &color);
		    have_pixel = TRUE;
		}

		text->rgba = ((color.red & 0xff00) << 16 |
			      (color.green & 0xff00) << 8|
			      (color.blue & 0xff00) |
			      0xff);
		color_changed = TRUE;
		break;

        case PROP_FILL_COLOR_RGBA:
		text->rgba = g_value_get_uint (value);
		color_changed = TRUE;
		break;

	case PROP_FILL_STIPPLE:
		set_stipple (text, g_value_get_boxed (value), FALSE);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}

	if (color_changed) {
		if (have_pixel)
			text->pixel = color.pixel;
		else
			text->pixel = gnome_canvas_get_color_pixel (item->canvas, text->rgba);

		if (!item->canvas->aa)
			set_text_gc_foreground (text);

		gnome_canvas_item_request_update (item);
	}
}

/* Get_arg handler for the text item */
static void
gnome_canvas_text_get_property (GObject            *object,
				guint               param_id,
				GValue             *value,
				GParamSpec         *pspec,
				const gchar        *trailer)
{
	GnomeCanvasText *text;
	GdkColor *color;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_TEXT (object));

	text = GNOME_CANVAS_TEXT (object);

	switch (param_id) {
	case PROP_TEXT:
		g_value_set_string (value, text->text);
		break;

	case PROP_X:
		g_value_set_double (value, text->x);
		break;

	case PROP_Y:
		g_value_set_double (value, text->y);
		break;

	case PROP_FONT_DESC:
		g_value_set_boxed (value, text->font_desc);
		break;

	case PROP_ANCHOR:
		g_value_set_enum (value, text->anchor);
		break;

	case PROP_JUSTIFICATION:
		g_value_set_enum (value, text->justification);
		break;

	case PROP_CLIP_WIDTH:
		g_value_set_enum (value, text->clip_width);
		break;

	case PROP_CLIP_HEIGHT:
		g_value_set_double (value, text->clip_height);
		break;

	case PROP_CLIP:
		g_value_set_boolean (value, text->clip);
		break;

	case PROP_X_OFFSET:
		g_value_set_double (value, text->xofs);
		break;

	case PROP_Y_OFFSET:
		g_value_set_double (value, text->yofs);
		break;

	case PROP_FILL_COLOR_GDK: {
		GdkColormap *colormap;

		color = g_new (GdkColor, 1);
		color->pixel = text->pixel;
		colormap = gtk_widget_get_colormap (GTK_WIDGET (text->item.canvas));
		gdk_rgb_find_color (colormap, color);
		g_value_set_boxed (value, color);
		break;
	}
	case PROP_FILL_COLOR_RGBA:
		g_value_set_uint (value, text->rgba);
		break;

	case PROP_FILL_STIPPLE:
		g_value_set_boxed (value, text->stipple);
		break;

	case PROP_TEXT_WIDTH:
		g_value_set_double (value, text->max_width / text->item.canvas->pixels_per_unit);
		break;

	case PROP_TEXT_HEIGHT:
		g_value_set_double (value, text->height / text->item.canvas->pixels_per_unit);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

/* Update handler for the text item */
static void
gnome_canvas_text_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	GnomeCanvasText *text;
	double x1, y1, x2, y2;
	ArtDRect i_bbox, c_bbox;
	int i;

	text = GNOME_CANVAS_TEXT (item);

	if (parent_class->update)
		(* parent_class->update) (item, affine, clip_path, flags);

	if (!item->canvas->aa) {
		set_text_gc_foreground (text);
		set_stipple (text, text->stipple, TRUE);
		get_bounds (text, &x1, &y1, &x2, &y2);

		gnome_canvas_update_bbox (item, x1, y1, x2, y2);
	} else {
		/* aa rendering */
		for (i = 0; i < 6; i++)
			text->affine[i] = affine[i];
		get_bounds_item_relative (text, &i_bbox.x0, &i_bbox.y0, &i_bbox.x1, &i_bbox.y1);
		art_drect_affine_transform (&c_bbox, &i_bbox, affine);
		gnome_canvas_update_bbox (item, c_bbox.x0, c_bbox.y0, c_bbox.x1, c_bbox.y1);

	}
}

/* Realize handler for the text item */
static void
gnome_canvas_text_realize (GnomeCanvasItem *item)
{
	GnomeCanvasText *text;

	text = GNOME_CANVAS_TEXT (item);

	if (parent_class->realize)
		(* parent_class->realize) (item);

	text->gc = gdk_gc_new (item->canvas->layout.bin_window);
}

/* Unrealize handler for the text item */
static void
gnome_canvas_text_unrealize (GnomeCanvasItem *item)
{
	GnomeCanvasText *text;

	text = GNOME_CANVAS_TEXT (item);

	gdk_gc_unref (text->gc);
	text->gc = NULL;

	if (parent_class->unrealize)
		(* parent_class->unrealize) (item);
}

/* Draw handler for the text item */
static void
gnome_canvas_text_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
			int x, int y, int width, int height)
{
	GnomeCanvasText *text;
	GdkRectangle rect;

	text = GNOME_CANVAS_TEXT (item);

	if (!text->text || !text->font_desc)
		return;

	if (text->clip) {
		rect.x = text->clip_cx - x;
		rect.y = text->clip_cy - y;
		rect.width = text->clip_cwidth;
		rect.height = text->clip_cheight;

		gdk_gc_set_clip_rectangle (text->gc, &rect);
	}

	if (text->stipple)
		gnome_canvas_set_stipple_origin (item->canvas, text->gc);


	gdk_draw_layout (drawable, text->gc, text->cx - x, text->cy - y, text->layout);

	if (text->clip)
		gdk_gc_set_clip_rectangle (text->gc, NULL);
}

/* Render handler for the text item */
static void
gnome_canvas_text_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf)
{
	GnomeCanvasText *text;
	guint32 fg_color;
	int i;
	double affine[6];
	ArtPoint start_i, start_c;
	FT_Bitmap bitmap;
	

	text = GNOME_CANVAS_TEXT (item);

	if (!text->text || !text->font_desc)
		return;

	fg_color = text->rgba >> 8;

        gnome_canvas_buf_ensure_buf (buf);

	bitmap.rows = text->height;
	bitmap.width = text->max_width;
	bitmap.pitch = (text->max_width+3)&~3;
	bitmap.buffer = g_malloc0 (bitmap.rows * bitmap.pitch);
	bitmap.num_grays = 256;
	bitmap.pixel_mode = ft_pixel_mode_grays;
	pango_ft2_render_layout (&bitmap, text->layout, 0, 0);

	art_affine_scale (affine, item->canvas->pixels_per_unit, item->canvas->pixels_per_unit);
	for (i = 0; i < 6; i++)
		affine[i] = text->affine[i];

	start_i.x = text->cx;
	start_i.y = text->cy;
	art_affine_point (&start_c, &start_i, text->affine);
	affine[4] = start_c.x;
	affine[5] = start_c.y;
	art_rgb_a_affine (buf->buf,
			  buf->rect.x0, buf->rect.y0, buf->rect.x1, buf->rect.y1,
			  buf->buf_rowstride,
			  bitmap.buffer,
			  bitmap.width,
			  bitmap.rows,
			  bitmap.pitch,
			  fg_color,
			  affine,
			  ART_FILTER_NEAREST, NULL);

	g_free (bitmap.buffer);
	
	buf->is_bg = 0;
}

/* Point handler for the text item */
static double
gnome_canvas_text_point (GnomeCanvasItem *item, double x, double y,
			 int cx, int cy, GnomeCanvasItem **actual_item)
{
	GnomeCanvasText *text;
	PangoLayoutIter *iter;
	int x1, y1, x2, y2;
	int dx, dy;
	double dist, best;

	text = GNOME_CANVAS_TEXT (item);

	*actual_item = item;

	/* The idea is to build bounding rectangles for each of the lines of
	 * text (clipped by the clipping rectangle, if it is activated) and see
	 * whether the point is inside any of these.  If it is, we are done.
	 * Otherwise, calculate the distance to the nearest rectangle.
	 */

	best = 1.0e36;

	iter = pango_layout_get_iter (text->layout);
	do {
 	        PangoRectangle log_rect;

		pango_layout_iter_get_line_extents (iter, NULL, &log_rect);
		
		x1 = PANGO_PIXELS (log_rect.x);
		y1 = PANGO_PIXELS (log_rect.y);
		x2 = PANGO_PIXELS (log_rect.x+log_rect.width);
		y2 = PANGO_PIXELS (log_rect.x+log_rect.height);
		
		if (text->clip) {
			if (x1 < text->clip_cx)
				x1 = text->clip_cx;

			if (y1 < text->clip_cy)
				y1 = text->clip_cy;

			if (x2 > (text->clip_cx + text->clip_width))
				x2 = text->clip_cx + text->clip_width;

			if (y2 > (text->clip_cy + text->clip_height))
				y2 = text->clip_cy + text->clip_height;

			if ((x1 >= x2) || (y1 >= y2))
				continue;
		}

		/* Calculate distance from point to rectangle */

		if (cx < x1)
			dx = x1 - cx;
		else if (cx >= x2)
			dx = cx - x2 + 1;
		else
			dx = 0;

		if (cy < y1)
			dy = y1 - cy;
		else if (cy >= y2)
			dy = cy - y2 + 1;
		else
			dy = 0;

		if ((dx == 0) && (dy == 0))
			return 0.0;

		dist = sqrt (dx * dx + dy * dy);
		if (dist < best)
			best = dist;
		
	} while (pango_layout_iter_next_line(iter));

	return best / item->canvas->pixels_per_unit;
}

/* Bounds handler for the text item */
static void
gnome_canvas_text_bounds (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	GnomeCanvasText *text;
	double width, height;

	text = GNOME_CANVAS_TEXT (item);

	*x1 = text->x;
	*y1 = text->y;

	if (text->clip) {
		width = text->clip_width;
		height = text->clip_height;
	} else {
		width = text->max_width / item->canvas->pixels_per_unit;
		height = text->height / item->canvas->pixels_per_unit;
	}

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		*x1 -= width / 2.0;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		*x1 -= width;
		break;
	}

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		break;

	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		*y1 -= height / 2.0;
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		*y1 -= height;
		break;
	}

	*x2 = *x1 + width;
	*y2 = *y1 + height;
}
