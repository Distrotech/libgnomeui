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
#include <gdk/gdkx.h> /* for BlackPixel */

#include "libart_lgpl/art_affine.h"
#include "libart_lgpl/art_rgb.h"
#include "libart_lgpl/art_rgb_bitmap_affine.h"
#include "gnome-canvas-util.h"



/* This defines a line of text */
struct line {
	char *text;	/* Line's text, it is a pointer into the text->text string */
	int length;	/* Line's length in characters */
	int width;	/* Line's width in pixels */
};



/* Object argument IDs */
enum {
	PROP_0,
	PROP_TEXT,
	PROP_X,
	PROP_Y,
	PROP_FONT,
        PROP_FONTSET,
	PROP_FONT_GDK,
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

static GnomeCanvasTextSuckFont *gnome_canvas_suck_font (GdkFont *font);
static void gnome_canvas_suck_font_free (GnomeCanvasTextSuckFont *suckfont);


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
                 PROP_X,
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
                 PROP_FONTSET,
                 g_param_spec_string ("font_set", NULL, NULL,
                                      NULL,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_FONT_GDK,
                 g_param_spec_boxed ("font_gdk", NULL, NULL,
				     GTK_TYPE_GDK_FONT,
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
                 g_param_spec_object ("fill_color_gdk", NULL, NULL,
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

	if (text->lines)
		g_free (text->lines);
	text->lines = NULL;

	if (text->font)
		gdk_font_unref (text->font);
	text->font = NULL;

	if (text->suckfont)
		gnome_canvas_suck_font_free (text->suckfont);
	text->suckfont = NULL;

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

	if (text->text && text->font)
		text->height = (text->font->ascent + text->font->descent) * text->num_lines;
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

	if (text->text && text->font)
		text->height = (text->font->ascent + text->font->descent) * text->num_lines;
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

/* Calculates the line widths (in pixels) of the text's splitted lines */
static void
calc_line_widths (GnomeCanvasText *text)
{
	struct line *lines;
	int i;

	lines = text->lines;
	text->max_width = 0;

	if (!lines)
		return;

	for (i = 0; i < text->num_lines; i++) {
		if (lines->length != 0) {
			if (text->font)
				lines->width = gdk_text_width (text->font,
							       lines->text, lines->length);
			else
				lines->width = 0;

			if (lines->width > text->max_width)
				text->max_width = lines->width;
		}

		lines++;
	}
}

/* Splits the text of the text item into lines */
static void
split_into_lines (GnomeCanvasText *text)
{
	char *p;
	struct line *lines;
	int len;

	/* Free old array of lines */

	if (text->lines)
		g_free (text->lines);

	text->lines = NULL;
	text->num_lines = 0;

	if (!text->text)
		return;

	/* First, count the number of lines */

	for (p = text->text; *p; p++)
		if (*p == '\n')
			text->num_lines++;

	text->num_lines++;

	/* Allocate array of lines and calculate split positions */

	text->lines = lines = g_new0 (struct line, text->num_lines);
	len = 0;

	for (p = text->text; *p; p++) {
		if (*p == '\n') {
			lines->length = len;
			lines++;
			len = 0;
		} else if (len == 0) {
			len++;
			lines->text = p;
		} else
			len++;
	}

	lines->length = len;

	calc_line_widths (text);
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

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_TEXT (object));

	item = GNOME_CANVAS_ITEM (object);
	text = GNOME_CANVAS_TEXT (object);

	color_changed = FALSE;
	have_pixel = FALSE;

	switch (param_id) {
	case PROP_TEXT:
		if (text->text)
			g_free (text->text);

		text->text = g_strdup (g_value_get_string (value));
		split_into_lines (text);
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

	case PROP_FONT:
		if (text->font)
			gdk_font_unref (text->font);

		text->font = gdk_font_load (g_value_get_string (value));

		if (item->canvas->aa) {
			if (text->suckfont)
				gnome_canvas_suck_font_free (text->suckfont);

			text->suckfont = gnome_canvas_suck_font (text->font);
		}

		calc_line_widths (text);
		recalc_bounds (text);
		break;

	case PROP_FONTSET:
		if (text->font)
			gdk_font_unref (text->font);

		text->font = gdk_fontset_load (g_value_get_string (value));

		if (item->canvas->aa) {
			if (text->suckfont)
				gnome_canvas_suck_font_free (text->suckfont);

			text->suckfont = gnome_canvas_suck_font (text->font);
		}

		calc_line_widths (text);
		recalc_bounds (text);
		break;

	case PROP_FONT_GDK:
		if (text->font)
			gdk_font_unref (text->font);

		text->font = g_value_get_boxed (value);
		gdk_font_ref (text->font);

		if (item->canvas->aa) {
			if (text->suckfont)
				gnome_canvas_suck_font_free (text->suckfont);

			text->suckfont = gnome_canvas_suck_font (text->font);
		}

		calc_line_widths (text);
		recalc_bounds (text);
		break;

	case PROP_ANCHOR:
		text->anchor = g_value_get_enum (value);
		recalc_bounds (text);
		break;

	case PROP_JUSTIFICATION:
		text->justification = g_value_get_enum (value);
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

        case PROP_FILL_COLOR:
		if (g_value_get_string (value))
			gdk_color_parse (g_value_get_string (value), &color);

		text->rgba = ((color.red & 0xff00) << 16 |
			      (color.green & 0xff00) << 8 |
			      (color.blue & 0xff00) |
			      0xff);
		color_changed = TRUE;
		break;

	case PROP_FILL_COLOR_GDK:
		pcolor = g_value_get_boxed (value);
		if (pcolor) {
			color = *pcolor;
			gdk_color_context_query_color (item->canvas->cc, &color);
			have_pixel = TRUE;
		}

		text->rgba = ((color.red & 0xff00) << 16 |
			      (color.green & 0xff00) << 8 |
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
		g_value_set_string (value, g_strdup (text->text));
		break;

	case PROP_X:
		g_value_set_double (value, text->x);
		break;

	case PROP_Y:
		g_value_set_double (value, text->y);
		break;

	case PROP_FONT_GDK:
		g_value_set_boxed (value, text->font);
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

	case PROP_FILL_COLOR_GDK:
		color = g_new (GdkColor, 1);
		color->pixel = text->pixel;
		gdk_color_context_query_color (text->item.canvas->cc, color);
		g_value_set_boxed (value, color);
		break;

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

/* Calculates the x position of the specified line of text, based on the text's justification */
static double
get_line_xpos_item_relative (GnomeCanvasText *text, struct line *line)
{
	double x;

	x = text->x;

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		x -= text->max_width / 2;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		x -= text->max_width;
		break;
	}

	switch (text->justification) {
	case GTK_JUSTIFY_RIGHT:
		x += text->max_width - line->width;
		break;

	case GTK_JUSTIFY_CENTER:
		x += (text->max_width - line->width) * 0.5;
		break;

	default:
		/* For GTK_JUSTIFY_LEFT, we don't have to do anything.  We do not support
		 * GTK_JUSTIFY_FILL, yet.
		 */
		break;
	}

	return x;
}

/* Calculates the y position of the first line of text. */
static double
get_line_ypos_item_relative (GnomeCanvasText *text)
{
	double y;

	y = text->y;

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		break;

	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		y -= text->height / 2;
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		y -= text->height;
		break;
	}

	return y;
}

/* Calculates the x position of the specified line of text, based on the text's justification */
static int
get_line_xpos (GnomeCanvasText *text, struct line *line)
{
	int x;

	x = text->cx;

	switch (text->justification) {
	case GTK_JUSTIFY_RIGHT:
		x += text->max_width - line->width;
		break;

	case GTK_JUSTIFY_CENTER:
		x += (text->max_width - line->width) / 2;
		break;

	default:
		/* For GTK_JUSTIFY_LEFT, we don't have to do anything.  We do not support
		 * GTK_JUSTIFY_FILL, yet.
		 */
		break;
	}

	return x;
}

/* Draw handler for the text item */
static void
gnome_canvas_text_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
			int x, int y, int width, int height)
{
	GnomeCanvasText *text;
	GdkRectangle rect;
	struct line *lines;
	int i;
	int xpos, ypos;

	text = GNOME_CANVAS_TEXT (item);

	if (!text->text || !text->font)
		return;

	if (text->clip) {
		rect.x = text->clip_cx - x;
		rect.y = text->clip_cy - y;
		rect.width = text->clip_cwidth;
		rect.height = text->clip_cheight;

		gdk_gc_set_clip_rectangle (text->gc, &rect);
	}

	lines = text->lines;
	ypos = text->cy + text->font->ascent;

	if (text->stipple)
		gnome_canvas_set_stipple_origin (item->canvas, text->gc);

	for (i = 0; i < text->num_lines; i++) {
		if (lines->length != 0) {
			xpos = get_line_xpos (text, lines);

			gdk_draw_text (drawable,
				       text->font,
				       text->gc,
				       xpos - x,
				       ypos - y,
				       lines->text,
				       lines->length);
		}

		ypos += text->font->ascent + text->font->descent;
		lines++;
	}

	if (text->clip)
		gdk_gc_set_clip_rectangle (text->gc, NULL);
}

/* Render handler for the text item */
static void
gnome_canvas_text_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf)
{
	GnomeCanvasText *text;
	guint32 fg_color;
	double xpos, ypos;
	struct line *lines;
	int i, j;
	double affine[6];
	GnomeCanvasTextSuckFont *suckfont;
	int dx, dy;
	ArtPoint start_i, start_c;

	text = GNOME_CANVAS_TEXT (item);

	if (!text->text || !text->font || !text->suckfont)
		return;

	suckfont = text->suckfont;

	fg_color = text->rgba;

        gnome_canvas_buf_ensure_buf (buf);

	lines = text->lines;
	start_i.y = get_line_ypos_item_relative (text);

	art_affine_scale (affine, item->canvas->pixels_per_unit, item->canvas->pixels_per_unit);
	for (i = 0; i < 6; i++)
		affine[i] = text->affine[i];

	for (i = 0; i < text->num_lines; i++) {
		if (lines->length != 0) {
			start_i.x = get_line_xpos_item_relative (text, lines);
			art_affine_point (&start_c, &start_i, text->affine);
			xpos = start_c.x;
			ypos = start_c.y;

			for (j = 0; j < lines->length; j++) {
				GnomeCanvasTextSuckChar *ch;

				ch = &suckfont->chars[(unsigned char)((lines->text)[j])];

				affine[4] = xpos;
				affine[5] = ypos;
				art_rgb_bitmap_affine (
					buf->buf,
					buf->rect.x0, buf->rect.y0, buf->rect.x1, buf->rect.y1,
					buf->buf_rowstride,
					suckfont->bitmap + (ch->bitmap_offset >> 3),
					ch->width,
					suckfont->bitmap_height,
					suckfont->bitmap_width >> 3,
					fg_color,
					affine,
					ART_FILTER_NEAREST, NULL);

				dx = ch->left_sb + ch->width + ch->right_sb;
				xpos += dx * affine[0];
				ypos += dx * affine[1];
			}
		}

		dy = text->font->ascent + text->font->descent;
		start_i.y += dy;
		lines++;
	}

	buf->is_bg = 0;
}

/* Point handler for the text item */
static double
gnome_canvas_text_point (GnomeCanvasItem *item, double x, double y,
			 int cx, int cy, GnomeCanvasItem **actual_item)
{
	GnomeCanvasText *text;
	int i;
	struct line *lines;
	int x1, y1, x2, y2;
	int font_height;
	int dx, dy;
	double dist, best;

	text = GNOME_CANVAS_TEXT (item);

	*actual_item = item;

	/* The idea is to build bounding rectangles for each of the lines of
	 * text (clipped by the clipping rectangle, if it is activated) and see
	 * whether the point is inside any of these.  If it is, we are done.
	 * Otherwise, calculate the distance to the nearest rectangle.
	 */

	if (text->font)
		font_height = text->font->ascent + text->font->descent;
	else
		font_height = 0;

	best = 1.0e36;

	lines = text->lines;

	for (i = 0; i < text->num_lines; i++) {
		/* Compute the coordinates of rectangle for the current line,
		 * clipping if appropriate.
		 */

		x1 = get_line_xpos (text, lines);
		y1 = text->cy + i * font_height;
		x2 = x1 + lines->width;
		y2 = y1 + font_height;

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

		/* Next! */

		lines++;
	}

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



/* Routines for sucking fonts from the X server */

static GnomeCanvasTextSuckFont *
gnome_canvas_suck_font (GdkFont *font)
{
	GnomeCanvasTextSuckFont *suckfont;
	int i;
	int x, y;
	char text[1];
	int lbearing, rbearing, ch_width, ascent, descent;
	GdkPixmap *pixmap;
	GdkColor black, white;
	GdkImage *image;
	GdkGC *gc;
	guchar *line;
	int width, height;
	int black_pixel, pixel;

	if (!font)
		return NULL;

	suckfont = g_new (GnomeCanvasTextSuckFont, 1);

	height = font->ascent + font->descent;
	x = 0;
	for (i = 0; i < 256; i++) {
		text[0] = i;
		gdk_text_extents (font, text, 1,
				  &lbearing, &rbearing, &ch_width, &ascent, &descent);
		suckfont->chars[i].left_sb = lbearing;
		suckfont->chars[i].right_sb = ch_width - rbearing;
		suckfont->chars[i].width = rbearing - lbearing;
		suckfont->chars[i].ascent = ascent;
		suckfont->chars[i].descent = descent;
		suckfont->chars[i].bitmap_offset = x;
		x += (ch_width + 31) & -32;
	}

	width = x;

	suckfont->bitmap_width = width;
	suckfont->bitmap_height = height;
	suckfont->ascent = font->ascent;

	pixmap = gdk_pixmap_new (NULL, suckfont->bitmap_width,
				 suckfont->bitmap_height, 1);
	gc = gdk_gc_new (pixmap);
	gdk_gc_set_font (gc, font);

	black_pixel = BlackPixel (gdk_display, DefaultScreen (gdk_display));
	black.pixel = black_pixel;
	white.pixel = WhitePixel (gdk_display, DefaultScreen (gdk_display));
	gdk_gc_set_foreground (gc, &white);
	gdk_draw_rectangle (pixmap, gc, 1, 0, 0, width, height);

	gdk_gc_set_foreground (gc, &black);
	for (i = 0; i < 256; i++) {
		text[0] = i;
		gdk_draw_text (pixmap, font, gc,
			       suckfont->chars[i].bitmap_offset - suckfont->chars[i].left_sb,
			       font->ascent,
			       text, 1);
	}

	/* The handling of the image leaves me with distinct unease.  But this
	 * is more or less copied out of gimp/app/text_tool.c, so it _ought_ to
	 * work. -RLL
	 */

	image = gdk_image_get (GDK_DRAWABLE (pixmap), 0, 0, width, height);
	suckfont->bitmap = g_malloc0 ((width >> 3) * height);

	line = suckfont->bitmap;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pixel = gdk_image_get_pixel (image, x, y);
			if (pixel == black_pixel)
				line[x >> 3] |= 128 >> (x & 7);
		}
		line += width >> 3;
	}

	gdk_image_destroy (image);

	/* free the pixmap */
	gdk_pixmap_unref (pixmap);

	/* free the gc */
	gdk_gc_destroy (gc);

	return suckfont;
}

static void
gnome_canvas_suck_font_free (GnomeCanvasTextSuckFont *suckfont)
{
	g_free (suckfont->bitmap);
	g_free (suckfont);
}
