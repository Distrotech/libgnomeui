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
/* Polygon item type for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include "libart_lgpl/art_vpath.h"
#include "libart_lgpl/art_svp.h"
#include "libart_lgpl/art_svp_vpath.h"
#include "libart_lgpl/art_svp_vpath_stroke.h"
#include "gnome-canvas-polygon.h"
#include "gnome-canvas-util.h"
#include "gnometypebuiltins.h"


#define NUM_STATIC_POINTS 256	/* Number of static points to use to avoid allocating arrays */


#define GROW_BOUNDS(bx1, by1, bx2, by2, x, y) {	\
	if (x < bx1)				\
		bx1 = x;			\
						\
	if (x > bx2)				\
		bx2 = x;			\
						\
	if (y < by1)				\
		by1 = y;			\
						\
	if (y > by2)				\
		by2 = y;			\
}


enum {
	PROP_0,
	PROP_POINTS,
	PROP_FILL_COLOR,
	PROP_FILL_COLOR_GDK,
	PROP_FILL_COLOR_RGBA,
	PROP_OUTLINE_COLOR,
	PROP_OUTLINE_COLOR_GDK,
	PROP_OUTLINE_COLOR_RGBA,
	PROP_FILL_STIPPLE,
	PROP_OUTLINE_STIPPLE,
	PROP_WIDTH_PIXELS,
	PROP_WIDTH_UNITS
};


static void gnome_canvas_polygon_class_init (GnomeCanvasPolygonClass *class);
static void gnome_canvas_polygon_init       (GnomeCanvasPolygon      *poly);
static void gnome_canvas_polygon_destroy    (GtkObject               *object);
static void gnome_canvas_polygon_set_property (GObject              *object,
					       guint                 param_id,
					       const GValue         *value,
					       GParamSpec           *pspec,
					       const gchar          *trailer);
static void gnome_canvas_polygon_get_property (GObject              *object,
					       guint                 param_id,
					       GValue               *value,
					       GParamSpec           *pspec,
					       const gchar          *trailer);

static void   gnome_canvas_polygon_update      (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);
static void   gnome_canvas_polygon_realize     (GnomeCanvasItem *item);
static void   gnome_canvas_polygon_unrealize   (GnomeCanvasItem *item);
static void   gnome_canvas_polygon_draw        (GnomeCanvasItem *item, GdkDrawable *drawable,
						int x, int y, int width, int height);
static double gnome_canvas_polygon_point       (GnomeCanvasItem *item, double x, double y,
						int cx, int cy, GnomeCanvasItem **actual_item);
static void   gnome_canvas_polygon_translate   (GnomeCanvasItem *item, double dx, double dy);
static void   gnome_canvas_polygon_bounds      (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2);
static void   gnome_canvas_polygon_render      (GnomeCanvasItem *item, GnomeCanvasBuf *buf);


static GnomeCanvasItemClass *parent_class;


GtkType
gnome_canvas_polygon_get_type (void)
{
	static GtkType polygon_type = 0;

	if (!polygon_type) {
		GtkTypeInfo polygon_info = {
			"GnomeCanvasPolygon",
			sizeof (GnomeCanvasPolygon),
			sizeof (GnomeCanvasPolygonClass),
			(GtkClassInitFunc) gnome_canvas_polygon_class_init,
			(GtkObjectInitFunc) gnome_canvas_polygon_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		polygon_type = gtk_type_unique (gnome_canvas_item_get_type (), &polygon_info);
	}

	return polygon_type;
}

static void
gnome_canvas_polygon_class_init (GnomeCanvasPolygonClass *class)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	gobject_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	gobject_class->set_property = gnome_canvas_polygon_set_property;
	gobject_class->get_property = gnome_canvas_polygon_get_property;

        g_object_class_install_property
                (gobject_class,
                 PROP_POINTS,
                 g_param_spec_boxed ("points", NULL, NULL,
				     GTK_TYPE_GNOME_CANVAS_POINTS,
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
                 PROP_OUTLINE_COLOR,
                 g_param_spec_string ("outline_color", NULL, NULL,
                                      NULL,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_OUTLINE_COLOR_GDK,
                 g_param_spec_boxed ("outline_color_gdk", NULL, NULL,
				     GTK_TYPE_GDK_COLOR,
				     (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_OUTLINE_COLOR_RGBA,
                 g_param_spec_uint ("outline_color_rgba", NULL, NULL,
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
                 PROP_OUTLINE_STIPPLE,
                 g_param_spec_object ("outline_stipple", NULL, NULL,
                                      GDK_TYPE_DRAWABLE,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_WIDTH_PIXELS,
                 g_param_spec_uint ("width_pixels", NULL, NULL,
				    0, G_MAXUINT, 0,
				    (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_WIDTH_UNITS,
                 g_param_spec_double ("width_units", NULL, NULL,
				      0.0, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	object_class->destroy = gnome_canvas_polygon_destroy;

	item_class->update = gnome_canvas_polygon_update;
	item_class->realize = gnome_canvas_polygon_realize;
	item_class->unrealize = gnome_canvas_polygon_unrealize;
	item_class->draw = gnome_canvas_polygon_draw;
	item_class->point = gnome_canvas_polygon_point;
	item_class->translate = gnome_canvas_polygon_translate;
	item_class->bounds = gnome_canvas_polygon_bounds;
	item_class->render = gnome_canvas_polygon_render;
}

static void
gnome_canvas_polygon_init (GnomeCanvasPolygon *poly)
{
	poly->width = 0.0;
}

static void
gnome_canvas_polygon_destroy (GtkObject *object)
{
	GnomeCanvasPolygon *poly;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_POLYGON (object));

	poly = GNOME_CANVAS_POLYGON (object);

	/* remember, destroy can be run multiple times! */

	if (poly->coords)
		g_free (poly->coords);
	poly->coords = NULL;

	if (poly->fill_stipple)
		gdk_bitmap_unref (poly->fill_stipple);
	poly->fill_stipple = NULL;

	if (poly->outline_stipple)
		gdk_bitmap_unref (poly->outline_stipple);
	poly->outline_stipple = NULL;

	if (poly->fill_svp)
		art_svp_free (poly->fill_svp);
	poly->fill_svp = NULL;

	if (poly->outline_svp)
		art_svp_free (poly->outline_svp);
	poly->outline_svp = NULL;

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/* Computes the bounding box of the polygon.  Assumes that the number of points in the polygon is
 * not zero.
 */
static void
get_bounds (GnomeCanvasPolygon *poly, double *bx1, double *by1, double *bx2, double *by2)
{
	double *coords;
	double x1, y1, x2, y2;
	double width;
	int i;

	/* Compute bounds of vertices */

	x1 = x2 = poly->coords[0];
	y1 = y2 = poly->coords[1];

	for (i = 1, coords = poly->coords + 2; i < poly->num_points; i++, coords += 2) {
		GROW_BOUNDS (x1, y1, x2, y2, coords[0], coords[1]);
	}

	/* Add outline width */

	if (poly->width_pixels)
		width = poly->width / poly->item.canvas->pixels_per_unit;
	else
		width = poly->width;

	width /= 2.0;

	x1 -= width;
	y1 -= width;
	x2 += width;
	y2 += width;

	/* Done */

	*bx1 = x1;
	*by1 = y1;
	*bx2 = x2;
	*by2 = y2;
}

/* Computes the bounding box of the polygon, in canvas coordinates.  Assumes that the number of points in the polygon is
 * not zero.
 */
static void
get_bounds_canvas (GnomeCanvasPolygon *poly, double *bx1, double *by1, double *bx2, double *by2, double affine[6])
{
	GnomeCanvasItem *item;
	ArtDRect bbox_world;
	ArtDRect bbox_canvas;

	item = GNOME_CANVAS_ITEM (poly);

	get_bounds (poly, &bbox_world.x0, &bbox_world.y0, &bbox_world.x1, &bbox_world.y1);

	art_drect_affine_transform (&bbox_canvas, &bbox_world, affine);
	/* include 1 pixel of fudge */
	*bx1 = bbox_canvas.x0 - 1;
	*by1 = bbox_canvas.y0 - 1;
	*bx2 = bbox_canvas.x1 + 1;
	*by2 = bbox_canvas.y1 + 1;
}

/* Recalculates the canvas bounds for the polygon */
static void
recalc_bounds (GnomeCanvasPolygon *poly)
{
	GnomeCanvasItem *item;
	double x1, y1, x2, y2;
	int cx1, cy1, cx2, cy2;
	double dx, dy;

	item = GNOME_CANVAS_ITEM (poly);

	if (poly->num_points == 0) {
		item->x1 = item->y1 = item->x2 = item->y2 = 0;
		return;
	}

	/* Get bounds in world coordinates */

	get_bounds (poly, &x1, &y1, &x2, &y2);

	/* Convert to canvas pixel coords */

	dx = dy = 0.0;
	gnome_canvas_item_i2w (item, &dx, &dy);

	gnome_canvas_w2c (item->canvas, x1 + dx, y1 + dy, &cx1, &cy1);
	gnome_canvas_w2c (item->canvas, x2 + dx, y2 + dy, &cx2, &cy2);
	item->x1 = cx1;
	item->y1 = cy1;
	item->x2 = cx2;
	item->y2 = cy2;

	/* Some safety fudging */

	item->x1--;
	item->y1--;
	item->x2++;
	item->y2++;

	gnome_canvas_group_child_bounds (GNOME_CANVAS_GROUP (item->parent), item);
}

/* Sets the points of the polygon item to the specified ones.  If needed, it will add a point to
 * close the polygon.
 */
static void
set_points (GnomeCanvasPolygon *poly, GnomeCanvasPoints *points)
{
	int duplicate;

	/* See if we need to duplicate the first point */

	duplicate = ((points->coords[0] != points->coords[2 * points->num_points - 2])
		     || (points->coords[1] != points->coords[2 * points->num_points - 1]));

	if (duplicate)
		poly->num_points = points->num_points + 1;
	else
		poly->num_points = points->num_points;

	poly->coords = g_new (double, 2 * poly->num_points);
	memcpy (poly->coords, points->coords, 2 * points->num_points * sizeof (double));

	if (duplicate) {
		poly->coords[2 * poly->num_points - 2] = poly->coords[0];
		poly->coords[2 * poly->num_points - 1] = poly->coords[1];
	}
}

/* Convenience function to set a GC's foreground color to the specified pixel value */
static void
set_gc_foreground (GdkGC *gc, gulong pixel)
{
	GdkColor c;

	if (!gc)
		return;

	c.pixel = pixel;
	gdk_gc_set_foreground (gc, &c);
}

/* Sets the stipple pattern for the specified gc */
static void
set_stipple (GdkGC *gc, GdkBitmap **internal_stipple, GdkBitmap *stipple, int reconfigure)
{
	if (*internal_stipple && !reconfigure)
		gdk_bitmap_unref (*internal_stipple);

	*internal_stipple = stipple;
	if (stipple && !reconfigure)
		gdk_bitmap_ref (stipple);

	if (gc) {
		if (stipple) {
			gdk_gc_set_stipple (gc, stipple);
			gdk_gc_set_fill (gc, GDK_STIPPLED);
		} else
			gdk_gc_set_fill (gc, GDK_SOLID);
	}
}

/* Recalculate the outline width of the polygon and set it in its GC */
static void
set_outline_gc_width (GnomeCanvasPolygon *poly)
{
	int width;

	if (!poly->outline_gc)
		return;

	if (poly->width_pixels)
		width = (int) poly->width;
	else
		width = (int) (poly->width * poly->item.canvas->pixels_per_unit + 0.5);

	gdk_gc_set_line_attributes (poly->outline_gc, width,
				    GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
}

static void
gnome_canvas_polygon_set_property (GObject              *object,
				   guint                 param_id,
				   const GValue         *value,
				   GParamSpec           *pspec,
				   const gchar          *trailer)
{
	GnomeCanvasItem *item;
	GnomeCanvasPolygon *poly;
	GnomeCanvasPoints *points;
	GdkColor color = { 0, 0, 0, 0, };
	GdkColor *pcolor;
	int have_pixel;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_POLYGON (object));

	item = GNOME_CANVAS_ITEM (object);
	poly = GNOME_CANVAS_POLYGON (object);
	have_pixel = FALSE;

	switch (param_id) {
	case PROP_POINTS:
		points = g_value_get_boxed (value);

		if (poly->coords) {
			g_free (poly->coords);
			poly->coords = NULL;
		}

		if (!points)
			poly->num_points = 0;
		else
			set_points (poly, points);

		gnome_canvas_item_request_update (item);
		break;

        case PROP_FILL_COLOR:
	case PROP_FILL_COLOR_GDK:
	case PROP_FILL_COLOR_RGBA:
		switch (param_id) {
		case PROP_FILL_COLOR:
			if (g_value_get_string (value) &&
			    gdk_color_parse (g_value_get_string (value), &color))
				poly->fill_set = TRUE;
			else
				poly->fill_set = FALSE;

			poly->fill_color = ((color.red & 0xff00) << 16 |
					    (color.green & 0xff00) << 8 |
					    (color.blue & 0xff00) |
					    0xff);
			break;

		case PROP_FILL_COLOR_GDK:
			pcolor = g_value_get_boxed (value);
			poly->fill_set = pcolor != NULL;

			if (pcolor) {
				GdkColormap *colormap;

				color = *pcolor;
				colormap = gtk_widget_get_colormap (GTK_WIDGET (item->canvas));
				gdk_rgb_find_color (colormap, &color);
				have_pixel = TRUE;
			}

			poly->fill_color = ((color.red & 0xff00) << 16 |
					    (color.green & 0xff00) << 8 |
					    (color.blue & 0xff00) |
					    0xff);
			break;

		case PROP_FILL_COLOR_RGBA:
			poly->fill_set = TRUE;
			poly->fill_color = g_value_get_uint (value);
			break;
		}
#ifdef VERBOSE
		g_print ("poly fill color = %08x\n", poly->fill_color);
#endif
		if (have_pixel)
			poly->fill_pixel = color.pixel;
		else
			poly->fill_pixel = gnome_canvas_get_color_pixel (item->canvas,
									 poly->fill_color);

		set_gc_foreground (poly->fill_gc, poly->fill_pixel);
                gnome_canvas_item_request_redraw_svp (item, poly->fill_svp);
		break;

        case PROP_OUTLINE_COLOR:
	case PROP_OUTLINE_COLOR_GDK:
	case PROP_OUTLINE_COLOR_RGBA:
		switch (param_id) {
		case PROP_OUTLINE_COLOR:
			if (g_value_get_string (value) &&
			    gdk_color_parse (g_value_get_string (value), &color))
				poly->outline_set = TRUE;
			else
				poly->outline_set = FALSE;

			poly->outline_color = ((color.red & 0xff00) << 16 |
					       (color.green & 0xff00) << 8 |
					       (color.blue & 0xff00) |
					       0xff);
			break;

		case PROP_OUTLINE_COLOR_GDK:
			pcolor = g_value_get_boxed (value);
			poly->outline_set = pcolor != NULL;

			if (pcolor) {
				GdkColormap *colormap;

				color = *pcolor;
				colormap = gtk_widget_get_colormap (GTK_WIDGET (item->canvas));
				gdk_rgb_find_color (colormap, &color);
				have_pixel = TRUE;
			}

			poly->outline_color = ((color.red & 0xff00) << 16 |
					       (color.green & 0xff00) << 8 |
					       (color.blue & 0xff00) |
					       0xff);
			break;

		case PROP_OUTLINE_COLOR_RGBA:
			poly->outline_set = TRUE;
			poly->outline_color = g_value_get_uint (value);
			break;
		}
#ifdef VERBOSE
		g_print ("poly outline color = %08x\n", poly->outline_color);
#endif
		if (have_pixel)
			poly->outline_pixel = color.pixel;
		else
			poly->outline_pixel = gnome_canvas_get_color_pixel (item->canvas,
									    poly->outline_color);

		set_gc_foreground (poly->outline_gc, poly->outline_pixel);
                gnome_canvas_item_request_redraw_svp (item, poly->outline_svp);
		break;

	case PROP_FILL_STIPPLE:
		set_stipple (poly->fill_gc, &poly->fill_stipple, (GdkBitmap *) g_value_get_object (value), FALSE);
		gnome_canvas_item_request_update (item);
		break;

	case PROP_OUTLINE_STIPPLE:
		set_stipple (poly->outline_gc, &poly->outline_stipple, (GdkBitmap *) g_value_get_object (value), FALSE);
		gnome_canvas_item_request_update (item);
		break;

	case PROP_WIDTH_PIXELS:
		poly->width = g_value_get_uint (value);
		poly->width_pixels = TRUE;
		set_outline_gc_width (poly);
#ifdef OLD_XFORM
		recalc_bounds (poly);
#else
		gnome_canvas_item_request_update (item);
#endif
		break;

	case PROP_WIDTH_UNITS:
		poly->width = fabs (g_value_get_double (value));
		poly->width_pixels = FALSE;
		set_outline_gc_width (poly);
#ifdef OLD_XFORM
		recalc_bounds (poly);
#else
		gnome_canvas_item_request_update (item);
#endif
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

/* Allocates a GdkColor structure filled with the specified pixel, and puts it into the specified
 * value for returning it in the get_property method.
 */
static void
get_color_value (GnomeCanvasPolygon *poly, gulong pixel, GValue *value)
{
	GdkColor *color;
	GdkColormap *colormap;

	color = g_new (GdkColor, 1);
	color->pixel = pixel;

	colormap = gtk_widget_get_colormap (GTK_WIDGET (poly));
	gdk_rgb_find_color (colormap, color);
	g_value_set_boxed (value, color);
}

static void
gnome_canvas_polygon_get_property (GObject              *object,
				   guint                 param_id,
				   GValue               *value,
				   GParamSpec           *pspec,
				   const gchar          *trailer)
{
	GnomeCanvasPolygon *poly;
	GnomeCanvasPoints *points;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_POLYGON (object));

	poly = GNOME_CANVAS_POLYGON (object);

	switch (param_id) {
	case PROP_POINTS:
		if (poly->num_points != 0) {
			points = gnome_canvas_points_new (poly->num_points);
			memcpy (points->coords, poly->coords, 2 * poly->num_points * sizeof (double));
			g_value_set_boxed (value, points);
		} else
			g_value_set_boxed (value, NULL);
		break;

	case PROP_FILL_COLOR_GDK:
		get_color_value (poly, poly->fill_pixel, value);
		break;

	case PROP_OUTLINE_COLOR_GDK:
		get_color_value (poly, poly->outline_pixel, value);
		break;

	case PROP_FILL_COLOR_RGBA:
		g_value_set_uint (value, poly->fill_color);
		break;

	case PROP_OUTLINE_COLOR_RGBA:
		g_value_set_uint (value, poly->outline_color);
		break;

	case PROP_FILL_STIPPLE:
		g_value_set_object (value, (GObject *) poly->fill_stipple);
		break;

	case PROP_OUTLINE_STIPPLE:
		g_value_set_object (value, (GObject *) poly->outline_stipple);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
gnome_canvas_polygon_render (GnomeCanvasItem *item,
			     GnomeCanvasBuf *buf)
{
	GnomeCanvasPolygon *poly;

	poly = GNOME_CANVAS_POLYGON (item);

	if (poly->fill_svp != NULL)
		gnome_canvas_render_svp (buf, poly->fill_svp, poly->fill_color);

	if (poly->outline_svp != NULL)
		gnome_canvas_render_svp (buf, poly->outline_svp, poly->outline_color);
}

static void
gnome_canvas_polygon_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	GnomeCanvasPolygon *poly;
	ArtVpath *vpath;
	int i;
	ArtPoint pi, pc;
	ArtSVP *svp;
	double width;
	double x1, y1, x2, y2;

	poly = GNOME_CANVAS_POLYGON (item);

	if (parent_class->update)
		(* parent_class->update) (item, affine, clip_path, flags);

	if (item->canvas->aa) {
		gnome_canvas_item_reset_bounds (item);

		vpath = art_new (ArtVpath, poly->num_points + 2);

		for (i = 0; i < poly->num_points; i++) {
			pi.x = poly->coords[i * 2];
			pi.y = poly->coords[i * 2 + 1];
			art_affine_point (&pc, &pi, affine);
			vpath[i].code = i == 0 ? ART_MOVETO : ART_LINETO;
			vpath[i].x = pc.x;
			vpath[i].y = pc.y;
		}
		vpath[i].code = ART_END;
		vpath[i].x = 0;
		vpath[i].y = 0;

		if (poly->fill_set) {
			svp = art_svp_from_vpath (vpath);
			gnome_canvas_item_update_svp_clip (item, &poly->fill_svp, svp, clip_path);
		}

		if (poly->outline_set) {
			if (poly->width_pixels)
				width = poly->width;
			else
				width = poly->width * item->canvas->pixels_per_unit;

			if (width < 0.5)
				width = 0.5;

			svp = art_svp_vpath_stroke (vpath,
						    ART_PATH_STROKE_JOIN_ROUND,
						    ART_PATH_STROKE_CAP_ROUND,
						    width,
						    4,
						    0.5);

			gnome_canvas_item_update_svp_clip (item, &poly->outline_svp, svp, clip_path);
		}
		art_free (vpath);

	} else {
		set_outline_gc_width (poly);
		set_gc_foreground (poly->fill_gc, poly->fill_pixel);
		set_gc_foreground (poly->outline_gc, poly->outline_pixel);
		set_stipple (poly->fill_gc, &poly->fill_stipple, poly->fill_stipple, TRUE);
		set_stipple (poly->outline_gc, &poly->outline_stipple, poly->outline_stipple, TRUE);

		get_bounds_canvas (poly, &x1, &y1, &x2, &y2, affine);
		gnome_canvas_update_bbox (item, x1, y1, x2, y2);
	}
}

static void
gnome_canvas_polygon_realize (GnomeCanvasItem *item)
{
	GnomeCanvasPolygon *poly;

	poly = GNOME_CANVAS_POLYGON (item);

	if (parent_class->realize)
		(* parent_class->realize) (item);

	poly->fill_gc = gdk_gc_new (item->canvas->layout.bin_window);
	poly->outline_gc = gdk_gc_new (item->canvas->layout.bin_window);

#ifdef OLD_XFORM
	(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->update) (item, NULL, NULL, 0);
#endif
}

static void
gnome_canvas_polygon_unrealize (GnomeCanvasItem *item)
{
	GnomeCanvasPolygon *poly;

	poly = GNOME_CANVAS_POLYGON (item);

	gdk_gc_unref (poly->fill_gc);
	poly->fill_gc = NULL;
	gdk_gc_unref (poly->outline_gc);
	poly->outline_gc = NULL;

	if (parent_class->unrealize)
		(* parent_class->unrealize) (item);
}

/* Converts an array of world coordinates into an array of canvas pixel coordinates.  Takes in the
 * item->world deltas and the drawable deltas.
 */
static void
item_to_canvas (GnomeCanvas *canvas, double *item_coords, GdkPoint *canvas_coords, int num_points,
		double i2c[6])
{
	int i;
	ArtPoint pi, pc;

#ifdef VERBOSE
	{
		char str[128];
		art_affine_to_string (str, i2c);
		g_print ("polygon item_to_canvas %s\n", str);
	}
#endif

	for (i = 0; i < num_points; i++) {
		pi.x = item_coords[i * 2];
		pi.y = item_coords[i * 2 + 1];
		art_affine_point (&pc, &pi, i2c);
		canvas_coords->x = floor (pc.x + 0.5);
		canvas_coords->y = floor (pc.y + 0.5);
		canvas_coords++;
	}
}

static void
gnome_canvas_polygon_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
			   int x, int y, int width, int height)
{
	GnomeCanvasPolygon *poly;
	GdkPoint static_points[NUM_STATIC_POINTS];
	GdkPoint *points;
	double i2c[6];

	poly = GNOME_CANVAS_POLYGON (item);

	if (poly->num_points == 0)
		return;

	/* Build array of canvas pixel coordinates */

	if (poly->num_points <= NUM_STATIC_POINTS)
		points = static_points;
	else
		points = g_new (GdkPoint, poly->num_points);

	gnome_canvas_item_i2c_affine (item, i2c);

	i2c[4] -= x;
	i2c[5] -= y;

	item_to_canvas (item->canvas, poly->coords, points, poly->num_points, i2c);

	if (poly->fill_set) {
		if (poly->fill_stipple)
			gnome_canvas_set_stipple_origin (item->canvas, poly->fill_gc);

		gdk_draw_polygon (drawable, poly->fill_gc, TRUE, points, poly->num_points);
	}

	if (poly->outline_set) {
		if (poly->outline_stipple)
			gnome_canvas_set_stipple_origin (item->canvas, poly->outline_gc);

		gdk_draw_polygon (drawable, poly->outline_gc, FALSE, points, poly->num_points);
	}

	/* Done */

	if (points != static_points)
		g_free (points);
}

static double
gnome_canvas_polygon_point (GnomeCanvasItem *item, double x, double y,
			    int cx, int cy, GnomeCanvasItem **actual_item)
{
	GnomeCanvasPolygon *poly;
	double dist;
	double width;

	poly = GNOME_CANVAS_POLYGON (item);

	*actual_item = item;

	dist = gnome_canvas_polygon_to_point (poly->coords, poly->num_points, x, y);

	if (poly->outline_set) {
		if (poly->width_pixels)
			width = poly->width / item->canvas->pixels_per_unit;
		else
			width = poly->width;

		dist -= width / 2.0;

		if (dist < 0.0)
			dist = 0.0;
	}

	return dist;
}

static void
gnome_canvas_polygon_translate (GnomeCanvasItem *item, double dx, double dy)
{
	GnomeCanvasPolygon *poly;
	int i;
	double *coords;

	poly = GNOME_CANVAS_POLYGON (item);

	for (i = 0, coords = poly->coords; i < poly->num_points; i++, coords += 2) {
		coords[0] += dx;
		coords[1] += dy;
	}

	recalc_bounds (poly);
}

static void
gnome_canvas_polygon_bounds (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	GnomeCanvasPolygon *poly;

	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_POLYGON (item));

	poly = GNOME_CANVAS_POLYGON (item);

	if (poly->num_points == 0) {
		*x1 = *y1 = *x2 = *y2 = 0.0;
		return;
	}

	get_bounds (poly, x1, y1, x2, y2);
}
