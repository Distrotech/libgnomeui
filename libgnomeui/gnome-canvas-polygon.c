/* Polygon item type for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include "gnome-canvas-polygon.h"
#include "gnome-canvas-util.h"


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
	ARG_0,
	ARG_POINTS,
	ARG_FILL_COLOR,
	ARG_FILL_COLOR_GDK,
	ARG_OUTLINE_COLOR,
	ARG_OUTLINE_COLOR_GDK,
	ARG_WIDTH_PIXELS,
	ARG_WIDTH_UNITS
};


static void gnome_canvas_polygon_class_init (GnomeCanvasPolygonClass *class);
static void gnome_canvas_polygon_init       (GnomeCanvasPolygon      *poly);
static void gnome_canvas_polygon_destroy    (GtkObject               *object);
static void gnome_canvas_polygon_set_arg    (GtkObject               *object,
					     GtkArg                  *arg,
					     guint                    arg_id);
static void gnome_canvas_polygon_get_arg    (GtkObject               *object,
					     GtkArg                  *arg,
					     guint                    arg_id);

static void   gnome_canvas_polygon_reconfigure (GnomeCanvasItem *item);
static void   gnome_canvas_polygon_realize     (GnomeCanvasItem *item);
static void   gnome_canvas_polygon_unrealize   (GnomeCanvasItem *item);
static void   gnome_canvas_polygon_draw        (GnomeCanvasItem *item, GdkDrawable *drawable,
						int x, int y, int width, int height);
static double gnome_canvas_polygon_point       (GnomeCanvasItem *item, double x, double y,
						int cx, int cy, GnomeCanvasItem **actual_item);
static void   gnome_canvas_polygon_translate   (GnomeCanvasItem *item, double dx, double dy);
static void   gnome_canvas_polygon_bounds      (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2);


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

		polygon_type = gtk_type_unique (gnome_canvas_polygon_get_type (), &polygon_info);
	}

	return polygon_type;
}

static void
gnome_canvas_polygon_class_init (GnomeCanvasPolygonClass *class)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	gtk_object_add_arg_type ("GnomeCanvasPolygon::points", GTK_TYPE_POINTER, GTK_ARG_READWRITE, ARG_POINTS);
	gtk_object_add_arg_type ("GnomeCanvasPolygon::fill_color", GTK_TYPE_STRING, GTK_ARG_WRITABLE, ARG_FILL_COLOR);
	gtk_object_add_arg_type ("GnomeCanvasPolygon::fill_color_gdk", GTK_TYPE_GDK_COLOR, GTK_ARG_READWRITE, ARG_FILL_COLOR_GDK);
	gtk_object_add_arg_type ("GnomeCanvasPolygon::outline_color", GTK_TYPE_STRING, GTK_ARG_WRITABLE, ARG_OUTLINE_COLOR);
	gtk_object_add_arg_type ("GnomeCanvasPolygon::outline_color_gdk", GTK_TYPE_GDK_COLOR, GTK_ARG_READWRITE, ARG_OUTLINE_COLOR_GDK);
	gtk_object_add_arg_type ("GnomeCanvasPolygon::width_pixels", GTK_TYPE_UINT, GTK_ARG_WRITABLE, ARG_WIDTH_PIXELS);
	gtk_object_add_arg_type ("GnomeCanvasPolygon::width_units", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_WIDTH_UNITS);

	object_class->destroy = gnome_canvas_polygon_destroy;
	object_class->set_arg = gnome_canvas_polygon_set_arg;
	object_class->get_arg = gnome_canvas_polygon_get_arg;

	item_class->reconfigure = gnome_canvas_polygon_reconfigure;
	item_class->realize = gnome_canvas_polygon_realize;
	item_class->unrealize = gnome_canvas_polygon_unrealize;
	item_class->draw = gnome_canvas_polygon_draw;
	item_class->point = gnome_canvas_polygon_point;
	item_class->translate = gnome_canvas_polygon_translate;
	item_class->bounds = gnome_canvas_polygon_bounds;
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

	if (poly->coords)
		g_free (poly->coords);

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

	for (i = 1, coords = poly->coords + 2; i < poly->num_points; i++, coords += 2)
		GROW_BOUNDS (x1, y1, x2, y2, coords[0], coords[1]);

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

/* Recalculates the canvas bounds for the polygon */
static void
recalc_bounds (GnomeCanvasPolygon *poly)
{
	GnomeCanvasItem *item;
	double x1, y1, x2, y2;
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

	gnome_canvas_w2c (item->canvas, x1 + dx, y1 + dy, &item->x1, &item->y1);
	gnome_canvas_w2c (item-.canvas, x2 + dx, y2 + dy, &item->x2, &item->y2);

	/* Some safety fudging */

	item->x1--;
	item->y1--;
	item->x2++;
	item->y2++;

	gnome_canvas_group_child_bounds (GNOME_CANVAS_GROUP (item->parent), item);
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
gnome_canvas_polygon_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasItem *item;
	GnomeCanvasPolygon *poly;
	GnomeCanvasPoints *points;
	GdkColor color;

	item = GNOME_CANVAS_ITEM (object);
	poly = GNOME_CANVAS_POLYGON (object);

	switch (arg_id) {
	case ARG_POINTS:
		points = GTK_VALUE_POINTER (*arg);

		if (poly->coords) {
			g_free (poly->coords);
			poly->coords = NULL;
		}

		if (!points)
			poly->num_points = 0;
		else {
			poly->num_points = points->num_points;
			poly->coords = g_new (double, 2 * poly->num_points);
			memcpy (poly->coords, points->coords, 2 * poly->num_points * sizeof (double));
		}

		recalc_bounds (poly);
		break;

	case ARG_FILL_COLOR:
		if (gnome_canvas_get_color (item->canvas, GTK_VALUE_STRING (*arg), &color)) {
			poly->fill_set = TRUE;
			poly->fill_pixel = color.pixel;
			set_gc_foreground (poly->fill_gc, poly->fill_pixel);
		} else
			poly->fill_set = FALSE;

		break;

	case ARG_FILL_COLOR_GDK:
		poly->fill_set = TRUE;
		poly->fill_pixel = ((GdkColor *) GTK_VALUE_BOXED (*arg))->pixel;
		set_gc_foreground (poly->fill_gc, poly->fill_pixel);
		break;

	case ARG_OUTLINE_COLOR:
		if (gnome_canvas_get_color (item->canvas, GTK_VALUE_STRING (*arg), &color)) {
			poly->outline_set = TRUE;
			poly->outline_pixel = color.pixel;
			set_gc_foreground (poly->outline_gc, poly->outline_pixel);
		} else
			poly->outline_set = FALSE;

		break;

	case ARG_OUTLINE_COLOR_GDK:
		poly->outline_set = TRUE;
		poly->outline_pixel = ((GdkColor *) GTK_VALUE_BOXED (*arg))->pixel;
		set_gc_foreground (poly->outline_gc, poly->outline_pixel);
		break;

	case ARG_WIDTH_PIXELS:
		poly->width = GTK_VALUE_UINT (*arg);
		poly->width_pixels = TRUE;
		set_outline_gc_width (poly);
		recalc_bounds (poly)
		break;

	case ARG_WIDTH_UNITS:
		poly->width = fabs (GTK_VALUE_DOUBLE (*arg));
		poly->width_pixels = FALSE;
		set_outline_gc_width (poly);
		recalc_bounds (poly);
		break;

	default:
		break;
	}
}

/* Allocates a GdkColor structure filled with the specified pixel, and puts it into the specified
 * arg for returning it in the get_arg method.
 */
static void
get_color_arg (GnomeCanvasPolygon *poly, gulong pixel, GtkArg *arg)
{
	GdkColor *color;

	color = g_new (GdkColor, 1);
	color->pixel = pixel;
	gdk_color_context_query_color (poly->item.canvas->cc, color);
	GTK_VALUE_BOXED (*arg) = color;
}

static void
gnome_canvas_polygon_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasPolygon *poly;
	GnomeCanvasPoints *points;
	GdkColor *color;

	poly = GNOME_CANVAS_POLYGON (object);

	switch (arg_id) {
	case ARG_POINTS:
		if (poly->num_points != 0) {
			points = gnome_canvas_points_new (poly->num_points);
			memcpy (points->coords, poly->coords, 2 * poly->num_points * sizeof (double));
			GTK_VALUE_POINTER (*arg) = points;
		} else
			GTK_VALUE_POINTER (*arg) = NULL;
		break;

	case ARG_FILL_COLOR_GDK:
		get_color_arg (poly, poly->fill_pixel, arg);
		break;
		
	case ARG_OUTLINE_COLOR_GDK:
		get_color_arg (poly, poly->outline_pixel, arg);
		break;

	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
}

static void
gnome_canvas_polygon_reconfigure (GnomeCanvasItem *item)
{
	GnomeCanvasPolygon *poly;

	poly = GNOME_CANVAS_POLYGON (item);

	if (parent_class->reconfigure)
		(* parent_class->reconfigure) (item);

	set_gc_foreground (poly->fill_gc, poly->fill_pixel);
	set_gc_foreground (poly->outline_gc, poly->outline_pixel);
	set_outline_gc_width (poly);

	recalc_bounds (poly);
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

	(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->reconfigure) (item);
}

static void
gnome_canvas_polygon_unrealize (GnomeCanvasItem *item)
{
	GnomeCanvasPolygon *poly;

	poly = GNOME_CANVAS_POLYGON (item);

	gdk_gc_unref (poly->fill_gc);
	gdk_gc_unref (poly->outline_gc);

	if (parent_class->unrealize)
		(* parent_class->unrealize) (item);
}

/* Converts an array of world coordinates into an array of canvas pixel coordinates.  Takes in the
 * item->world deltas and the drawable deltas.
 */
static void
item_to_canvas (GnomeCanvas *canvas, double *item_coords, GdkPoint *canvas_coords, int num_points,
		double wdx, double wdy, int cdx, int cdy)
{
	int i;
	int cx, cy;

	for (i = 0; i < num_points; i++, item_coords += 2, canvas_coords++) {
		gnome_canvas_w2c (canvas, wdx + item_coords[0], wdy + item_coords[1], &cx, &cy);
		canvas_coords->x = cx - cdx;
		canvas_coords->y = cy - cdy;
	}
}

static void
gnome_canvas_polygon_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
			   int x, int y, int width, int height)
{
	GnomeCanvasPolygon *poly;
	GdkPoint static_points[NUM_STATIC_POINTS];
	GdkPoint *points;
	double dx, dy;
	int cx, cy;
	int i;

	poly = GNOME_CANVAS_POLYGON (item);

	if (poly->num_points == 0)
		return;

	/* Build array of canvas pixel coordinates */

	if (poly->num_points <= NUM_STATIC_POINTS)
		points = static_points;
	else
		points = g_new (GdkPoint, poly->num_points);

	dx = dy = 0.0;
	gnome_canvas_item_i2w (item, &dx, &dy);

	item_to_canvas (item-.canvas, poly->coords, points, dx, dy, x, y);

	if (poly->fill_set)
		gdk_draw_polygon (drawable, poly->fill_gc, TRUE, points, poly->num_points);

	if (poly->outline_set)
		gdk_draw_polygon (drawable, poly->outline_gc, FALSE, points, poly->num_points);

	/* Done */

	if (points != static_points)
		g_free (points);
}

static double
gnome_canvas_polygon_point (GnomeCanvasItem *item, double x, double y,
			    int cx, int cy, GnomeCanvasItem **actual_item)
{
	
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
