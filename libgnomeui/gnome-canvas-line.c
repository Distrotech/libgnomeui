/* Line/curve item type for GnomeCanvas widget
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
#include "gnome-canvas-line.h"
#include "gnome-canvas-util.h"


#define DEFAULT_SPLINE_STEPS 12		/* this is what Tk uses */
#define NUM_ARROW_POINTS     6		/* number of points in an arrowhead */
#define NUM_STATIC_POINTS    32		/* number of static points to use to avoid allocating arrays */


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
	ARG_WIDTH_PIXELS,
	ARG_WIDTH_UNITS,
	ARG_CAP_STYLE,
	ARG_JOIN_STYLE,
	ARG_FIRST_ARROWHEAD,
	ARG_LAST_ARROWHEAD,
	ARG_SMOOTH,
	ARG_SPLINE_STEPS,
	ARG_ARROW_SHAPE_A,
	ARG_ARROW_SHAPE_B,
	ARG_ARROW_SHAPE_C
};


static void gnome_canvas_line_class_init (GnomeCanvasLineClass *class);
static void gnome_canvas_line_init       (GnomeCanvasLine      *line);
static void gnome_canvas_line_destroy    (GtkObject            *object);
static void gnome_canvas_line_set_arg    (GtkObject            *object,
					  GtkArg               *arg,
					  guint                 arg_id);

static void   gnome_canvas_line_reconfigure (GnomeCanvasItem *item);
static void   gnome_canvas_line_realize     (GnomeCanvasItem *item);
static void   gnome_canvas_line_unrealize   (GnomeCanvasItem *item);
static void   gnome_canvas_line_draw        (GnomeCanvasItem *item, GdkDrawable *drawable,
					     int x, int y, int width, int height);
static double gnome_canvas_line_point       (GnomeCanvasItem *item, double x, double y,
					     int cx, int cy, GnomeCanvasItem **actual_item);
static void   gnome_canvas_line_translate   (GnomeCanvasItem *item, double dx, double dy);


static GnomeCanvasItemClass *parent_class;


GtkType
gnome_canvas_line_get_type (void)
{
	static GtkType line_type = 0;

	if (!line_type) {
		GtkTypeInfo line_info = {
			"GnomeCanvasLine",
			sizeof (GnomeCanvasLine),
			sizeof (GnomeCanvasLineClass),
			(GtkClassInitFunc) gnome_canvas_line_class_init,
			(GtkObjectInitFunc) gnome_canvas_line_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		line_type = gtk_type_unique (gnome_canvas_item_get_type (), &line_info);
	}

	return line_type;
}

static void
gnome_canvas_line_class_init (GnomeCanvasLineClass *class)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	gtk_object_add_arg_type ("GnomeCanvasLine::points", GTK_TYPE_POINTER, GTK_ARG_WRITABLE, ARG_POINTS);
	gtk_object_add_arg_type ("GnomeCanvasLine::fill_color", GTK_TYPE_STRING, GTK_ARG_WRITABLE, ARG_FILL_COLOR);
	gtk_object_add_arg_type ("GnomeCanvasLine::width_pixels", GTK_TYPE_UINT, GTK_ARG_WRITABLE, ARG_WIDTH_PIXELS);
	gtk_object_add_arg_type ("GnomeCanvasLine::width_units", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_WIDTH_UNITS);
	gtk_object_add_arg_type ("GnomeCanvasLine::cap_style", GTK_TYPE_GDK_CAP_STYLE, GTK_ARG_WRITABLE, ARG_CAP_STYLE);
	gtk_object_add_arg_type ("GnomeCanvasLine::join_style", GTK_TYPE_GDK_JOIN_STYLE, GTK_ARG_WRITABLE, ARG_JOIN_STYLE);
	gtk_object_add_arg_type ("GnomeCanvasLine::first_arrowhead", GTK_TYPE_BOOL, GTK_ARG_WRITABLE, ARG_FIRST_ARROWHEAD);
	gtk_object_add_arg_type ("GnomeCanvasLine::last_arrowhead", GTK_TYPE_BOOL, GTK_ARG_WRITABLE, ARG_LAST_ARROWHEAD);
	gtk_object_add_arg_type ("GnomeCanvasLine::smooth", GTK_TYPE_BOOL, GTK_ARG_WRITABLE, ARG_SMOOTH);
	gtk_object_add_arg_type ("GnomeCanvasLine::spline_steps", GTK_TYPE_UINT, GTK_ARG_WRITABLE, ARG_SPLINE_STEPS);
	gtk_object_add_arg_type ("GnomeCanvasLine::arrow_shape_a", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_ARROW_SHAPE_A);
	gtk_object_add_arg_type ("GnomeCanvasLine::arrow_shape_b", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_ARROW_SHAPE_B);
	gtk_object_add_arg_type ("GnomeCanvasLine::arrow_shape_c", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_ARROW_SHAPE_C);

	object_class->destroy = gnome_canvas_line_destroy;
	object_class->set_arg = gnome_canvas_line_set_arg;

	item_class->reconfigure = gnome_canvas_line_reconfigure;
	item_class->realize = gnome_canvas_line_realize;
	item_class->unrealize = gnome_canvas_line_unrealize;
	item_class->draw = gnome_canvas_line_draw;
	item_class->point = gnome_canvas_line_point;
	item_class->translate = gnome_canvas_line_translate;
}

static void
gnome_canvas_line_init (GnomeCanvasLine *line)
{
	line->width = 0.0;
	line->cap = GDK_CAP_BUTT;
	line->join = GDK_JOIN_MITER;
	line->width_pixels = TRUE;
	line->spline_steps = DEFAULT_SPLINE_STEPS;
}

static void
gnome_canvas_line_destroy (GtkObject *object)
{
	GnomeCanvasLine *line;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_LINE (object));

	if (line->coords)
		g_free (line->coords);

	if (line->first_coords)
		g_free (line->first_coords);

	if (line->last_coords)
		g_free (line->last_coords);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
recalc_bounds (GnomeCanvasLine *line)
{
	GnomeCanvasItem *item;
	double x1, y1, x2, y2;
	double width;
	double dx, dy;
	int i;

	item = GNOME_CANVAS_ITEM (line);

	if (line->num_points == 0) {
		item->x1 = item->y1 = item->x2 = item->y2 = 0;
		return;
	}

	/* Find bounding box of line's points */

	x1 = x2 = line->coords[0];
	y1 = y2 = line->coords[1];

	for (i = 1; i < line->num_points; i++)
		GROW_BOUNDS (x1, y1, x2, y2, line->coords[2 * i], line->coords[2 * i + 1]);

	/* Add possible over-estimate for wide lines */

	if (line->width_pixels)
		width = line->width / item->canvas->pixels_per_unit;
	else
		width = line->width;

	x1 -= width;
	y1 -= width;
	x2 += width;
	y2 += width;

	/* For mitered lines, make a second pass through all the points.  Compute the location of
	 * the two miter vertex points and add them to the bounding box.
	 */

	if (line->join == GDK_JOIN_MITER)
		for (i = 0; i < line->num_points; i++) {
			double mx1, my1, mx2, my2;

			if (gnome_canvas_get_miter_points (line->coords[2 * i],
							   line->coords[2 * i + 1],
							   line->coords[2 * i + 2],
							   line->coords[2 * i + 3],
							   line->coords[2 * i + 4],
							   line->coords[2 * i + 5],
							   width,
							   &mx1, &my1, &mx2, &my2)) {
				GROW_BOUNDS (x1, y1, x2, y2, mx1, my1);
				GROW_BOUNDS (x1, y1, x2, y2, mx2, my2);
			}
		}

	/* Add the arrow points, if any */

	if (line->first_arrow)
		for (i = 0; i < NUM_ARROW_POINTS; i++)
			GROW_BOUNDS (x1, y1, x2, y2,
				     line->first_coords[2 * i],
				     line->first_coords[2 * i + 1]);

	if (line->last_arrow)
		for (i = 0; i < NUM_ARROW_POINTS; i++)
			GROW_BOUNDS (x1, y1, x2, y2,
				     line->last_coords[2 * i],
				     line->last_coords[2 * i + 1]);

	/* Convert to canvas pixel coords */

	dx = dy = 0.0;
	gnome_canvas_item_i2w (item, &dx, &dy);

	gnome_canvas_w2c (item->canvas, x1 + dx, y1 + dy, &item->x1, &item->y1);
	gnome_canvas_w2c (item->canvas, x2 + dx, y2 + dy, &item->x2, &item->y2);

	/* Some safety fudging */

	item->x1--;
	item->y1--;
	item->x2++;
	item->y2++;
}

static void
gnome_canvas_line_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasItem *item;
	GnomeCanvasLine *line;
	int calc_bounds;
	int calc_gcs;
	GnomeCanvasPoints *points;
	GdkColor color;

	item = GNOME_CANVAS_ITEM (object);
	line = GNOME_CANVAS_LINE (object);

	calc_bounds = FALSE;
	calc_gcs = FALSE;

	switch (arg_id) {
	case ARG_POINTS:
		points = GTK_VALUE_POINTER (*arg);

		if (line->coords) {
			g_free (line->coords);
			line->coords = NULL;
		}

		if (!points)
			line->num_points = 0;
		else {
			line->num_points = points->num_points;
			line->coords = g_new (double, 2 * line->num_points);
			memcpy (line->coords, points->coords, 2 * line->num_points * sizeof (double));
		}

		calc_bounds = TRUE;
		break;

	case ARG_FILL_COLOR:
		gnome_canvas_get_color (item->canvas, GTK_VALUE_STRING (*arg), &color);
		line->pixel = color.pixel;
		calc_gcs = TRUE;
		break;

	case ARG_WIDTH_PIXELS:
		line->width = GTK_VALUE_UINT (*arg);
		line->width_pixels = TRUE;
		calc_gcs = TRUE;
		break;

	case ARG_WIDTH_UNITS:
		line->width = fabs (GTK_VALUE_DOUBLE (*arg));
		line->width_pixels = FALSE;
		calc_gcs = TRUE;
		break;

	case ARG_CAP_STYLE:
		line->cap = GTK_VALUE_ENUM (*arg);
		calc_gcs = TRUE;
		break;

	case ARG_JOIN_STYLE:
		line->join = GTK_VALUE_ENUM (*arg);
		calc_gcs = TRUE;
		break;

	case ARG_FIRST_ARROWHEAD:
		/* FIXME */
		break;

	case ARG_LAST_ARROWHEAD:
		/* FIXME */
		break;

	case ARG_SMOOTH:
		/* FIXME */
		break;

	case ARG_SPLINE_STEPS:
		/* FIXME */
		break;

	case ARG_ARROW_SHAPE_A:
		/* FIXME */
		break;

	case ARG_ARROW_SHAPE_B:
		/* FIXME */
		break;

	case ARG_ARROW_SHAPE_C:
		/* FIXME */
		break;

	default:
		break;
	}

	if (calc_gcs)
		(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->reconfigure) (item);

	if (calc_bounds)
		recalc_bounds (line);
}

static void
gnome_canvas_line_reconfigure (GnomeCanvasItem *item)
{
	GnomeCanvasLine *line;
	GdkColor color;
	int width;

	line = GNOME_CANVAS_LINE (item);

	if (line->gc) {
		color.pixel = line->pixel;
		gdk_gc_set_foreground (line->gc, &color);
		gdk_gc_set_foreground (line->arrow_gc, &color);

		if (line->width_pixels)
			width = (int) line->width;
		else
			width = (int) (line->width * item->canvas->pixels_per_unit + 0.5);

		gdk_gc_set_line_attributes (line->gc,
					    width,
					    GDK_LINE_SOLID,
					    (line->first_arrow || line->last_arrow) ? GDK_CAP_BUTT : line->cap,
					    line->join);
	}

	recalc_bounds (line);
}

static void
gnome_canvas_line_realize (GnomeCanvasItem *item)
{
	GnomeCanvasLine *line;

	line = GNOME_CANVAS_LINE (item);

	line->gc = gdk_gc_new (GTK_WIDGET (item->canvas)->window);
	line->arrow_gc = gdk_gc_new (GTK_WIDGET (item->canvas)->window);

	(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->reconfigure) (item);
}

static void
gnome_canvas_line_unrealize (GnomeCanvasItem *item)
{
	GnomeCanvasLine *line;

	line = GNOME_CANVAS_LINE (item);

	gdk_gc_unref (line->gc);
	gdk_gc_unref (line->arrow_gc);
}

static void
gnome_canvas_line_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
			int x, int y, int width, int height)
{
	GnomeCanvasLine *line;
	GdkPoint static_points[NUM_STATIC_POINTS];
	GdkPoint *points;
	int i;
	int cx, cy;
	double dx, dy;

	line = GNOME_CANVAS_LINE (item);

	if (line->num_points == 0)
		return;

	/* Build array of canvas pixel coordinates */

	if (line->num_points <= NUM_STATIC_POINTS)
		points = static_points;
	else
		points = g_new (GdkPoint, line->num_points);

	dx = dy = 0.0;
	gnome_canvas_item_i2w (item, &dx, &dy);

	for (i = 0; i < line->num_points; i++) {
		gnome_canvas_w2c (item->canvas,
				  dx + line->coords[2 * i],
				  dy + line->coords[2 * i + 1],
				  &cx, &cy);
		points[i].x = cx - x;
		points[i].y = cy - y;
	}

	gdk_draw_lines (drawable, line->gc, points, line->num_points);

	if (points != static_points)
		g_free (points);
}

static double
gnome_canvas_line_point (GnomeCanvasItem *item, double x, double y,
			 int cx, int cy, GnomeCanvasItem **actual_item)
{
	*actual_item = item;
	return 10000.0; /* FIXME */
}

static void
gnome_canvas_line_translate (GnomeCanvasItem *item, double dx, double dy)
{
	/* FIXME */
}
