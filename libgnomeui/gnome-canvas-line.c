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
#define NUM_STATIC_POINTS    256	/* number of static points to use to avoid allocating arrays */


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
static void gnome_canvas_line_get_arg    (GtkObject            *object,
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

	gtk_object_add_arg_type ("GnomeCanvasLine::points", GTK_TYPE_POINTER, GTK_ARG_READWRITE, ARG_POINTS);
	gtk_object_add_arg_type ("GnomeCanvasLine::fill_color", GTK_TYPE_STRING, GTK_ARG_WRITABLE, ARG_FILL_COLOR);
	gtk_object_add_arg_type ("GnomeCanvasLine::width_pixels", GTK_TYPE_UINT, GTK_ARG_WRITABLE, ARG_WIDTH_PIXELS);
	gtk_object_add_arg_type ("GnomeCanvasLine::width_units", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_WIDTH_UNITS);
	gtk_object_add_arg_type ("GnomeCanvasLine::cap_style", GTK_TYPE_GDK_CAP_STYLE, GTK_ARG_READWRITE, ARG_CAP_STYLE);
	gtk_object_add_arg_type ("GnomeCanvasLine::join_style", GTK_TYPE_GDK_JOIN_STYLE, GTK_ARG_READWRITE, ARG_JOIN_STYLE);
	gtk_object_add_arg_type ("GnomeCanvasLine::first_arrowhead", GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_FIRST_ARROWHEAD);
	gtk_object_add_arg_type ("GnomeCanvasLine::last_arrowhead", GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_LAST_ARROWHEAD);
	gtk_object_add_arg_type ("GnomeCanvasLine::smooth", GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_SMOOTH);
	gtk_object_add_arg_type ("GnomeCanvasLine::spline_steps", GTK_TYPE_UINT, GTK_ARG_READWRITE, ARG_SPLINE_STEPS);
	gtk_object_add_arg_type ("GnomeCanvasLine::arrow_shape_a", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_ARROW_SHAPE_A);
	gtk_object_add_arg_type ("GnomeCanvasLine::arrow_shape_b", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_ARROW_SHAPE_B);
	gtk_object_add_arg_type ("GnomeCanvasLine::arrow_shape_c", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_ARROW_SHAPE_C);

	object_class->destroy = gnome_canvas_line_destroy;
	object_class->set_arg = gnome_canvas_line_set_arg;
	object_class->get_arg = gnome_canvas_line_get_arg;

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
	line->shape_a = 0.0;
	line->shape_b = 0.0;
	line->shape_c = 0.0;
	line->spline_steps = DEFAULT_SPLINE_STEPS;
}

static void
gnome_canvas_line_destroy (GtkObject *object)
{
	GnomeCanvasLine *line;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_LINE (object));

	line = GNOME_CANVAS_LINE (object);

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
	double *coords;
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

	for (i = 1, coords = line->coords + 2; i < line->num_points; i++, coords += 2)
		GROW_BOUNDS (x1, y1, x2, y2, coords[0], coords[1]);

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
		for (i = line->num_points, coords = line->coords; i >= 3; i--, coords += 2) {
			double mx1, my1, mx2, my2;

			if (gnome_canvas_get_miter_points (coords[0], coords[1],
							   coords[2], coords[3],
							   coords[4], coords[5],
							   width,
							   &mx1, &my1, &mx2, &my2)) {
				GROW_BOUNDS (x1, y1, x2, y2, mx1, my1);
				GROW_BOUNDS (x1, y1, x2, y2, mx2, my2);
			}
		}

	/* Add the arrow points, if any */

	if (line->first_arrow && line->first_coords)
		for (i = 0, coords = line->first_coords; i < NUM_ARROW_POINTS; i++, coords += 2)
			GROW_BOUNDS (x1, y1, x2, y2, coords[0], coords[1]);

	if (line->last_arrow && line->last_coords)
		for (i = 0, coords = line->last_coords; i < NUM_ARROW_POINTS; i++, coords += 2)
			GROW_BOUNDS (x1, y1, x2, y2, coords[0], coords[1]);

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

	gnome_canvas_group_child_bounds (GNOME_CANVAS_GROUP (item->parent), item);
}

static void
gnome_canvas_line_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasItem *item;
	GnomeCanvasLine *line;
	int calc_bounds;
	int recalc;
	GnomeCanvasPoints *points;
	GdkColor color;

	item = GNOME_CANVAS_ITEM (object);
	line = GNOME_CANVAS_LINE (object);

	calc_bounds = FALSE;
	recalc = FALSE;

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
		recalc = TRUE;
		break;

	case ARG_WIDTH_PIXELS:
		line->width = GTK_VALUE_UINT (*arg);
		line->width_pixels = TRUE;
		recalc = TRUE;
		break;

	case ARG_WIDTH_UNITS:
		line->width = fabs (GTK_VALUE_DOUBLE (*arg));
		line->width_pixels = FALSE;
		recalc = TRUE;
		break;

	case ARG_CAP_STYLE:
		line->cap = GTK_VALUE_ENUM (*arg);
		recalc = TRUE;
		break;

	case ARG_JOIN_STYLE:
		line->join = GTK_VALUE_ENUM (*arg);
		recalc = TRUE;
		break;

	case ARG_FIRST_ARROWHEAD:
		line->first_arrow = GTK_VALUE_BOOL (*arg);
		recalc = TRUE;
		break;

	case ARG_LAST_ARROWHEAD:
		line->last_arrow = GTK_VALUE_BOOL (*arg);
		recalc = TRUE;
		break;

	case ARG_SMOOTH:
		/* FIXME */
		break;

	case ARG_SPLINE_STEPS:
		/* FIXME */
		break;

	case ARG_ARROW_SHAPE_A:
		line->shape_a = fabs (GTK_VALUE_DOUBLE (*arg));
		recalc = TRUE;
		break;

	case ARG_ARROW_SHAPE_B:
		line->shape_b = fabs (GTK_VALUE_DOUBLE (*arg));
		recalc = TRUE;
		break;

	case ARG_ARROW_SHAPE_C:
		line->shape_c = fabs (GTK_VALUE_DOUBLE (*arg));
		recalc = TRUE;
		break;

	default:
		break;
	}

	if (recalc)
		(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->reconfigure) (item);

	if (calc_bounds)
		recalc_bounds (line);
}

static void
gnome_canvas_line_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasLine *line;
	GnomeCanvasPoints *points;

	line = GNOME_CANVAS_LINE (object);

	switch (arg_id) {
	case ARG_POINTS:
		if (line->num_points != 0) {
			points = gnome_canvas_points_new (line->num_points);
			memcpy (points->coords, line->coords, 2 * line->num_points * sizeof (double));
			GTK_VALUE_POINTER (*arg) = points;
		} else
			GTK_VALUE_POINTER (*arg) = NULL;
		break;

	case ARG_CAP_STYLE:
		GTK_VALUE_ENUM (*arg) = line->cap;
		break;

	case ARG_JOIN_STYLE:
		GTK_VALUE_ENUM (*arg) = line->join;
		break;

	case ARG_FIRST_ARROWHEAD:
		GTK_VALUE_BOOL (*arg) = line->first_arrow;
		break;

	case ARG_LAST_ARROWHEAD:
		GTK_VALUE_BOOL (*arg) = line->last_arrow;
		break;

	case ARG_SMOOTH:
		GTK_VALUE_BOOL (*arg) = line->smooth;
		break;

	case ARG_SPLINE_STEPS:
		GTK_VALUE_UINT (*arg) = line->spline_steps;
		break;

	case ARG_ARROW_SHAPE_A:
		GTK_VALUE_DOUBLE (*arg) = line->shape_a;
		break;

	case ARG_ARROW_SHAPE_B:
		GTK_VALUE_DOUBLE (*arg) = line->shape_b;
		break;

	case ARG_ARROW_SHAPE_C:
		GTK_VALUE_DOUBLE (*arg) = line->shape_c;
		break;

	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
}

static void
reconfigure_arrows (GnomeCanvasLine *line)
{
	double *poly, *coords;
	double dx, dy, length;
	double sin_theta, cos_theta, tmp;
	double frac_height;	/* Line width as fraction of arrowhead width */
	double backup;		/* Distance to backup end points so the line ends in the middle of the arrowhead */
	double vx, vy;		/* Position of arrowhead vertex */
	double shape_a, shape_b, shape_c;
	double width;
	int i;

	if (line->num_points == 0)
		return;

	/* Set up things */

	if (line->first_arrow) {
		if (line->first_coords) {
			line->coords[0] = line->first_coords[0];
			line->coords[1] = line->first_coords[1];
		} else
			line->first_coords = g_new (double, 2 * NUM_ARROW_POINTS);
	} else if (line->first_coords) {
		line->coords[0] = line->first_coords[0];
		line->coords[1] = line->first_coords[1];

		g_free (line->first_coords);
		line->first_coords = NULL;
	}

	i = 2 * (line->num_points - 1);

	if (line->last_arrow) {
		if (line->last_coords) {
			line->coords[i] = line->last_coords[0];
			line->coords[i + 1] = line->last_coords[1];
		} else
			line->last_coords = g_new (double, 2 * NUM_ARROW_POINTS);
	} else if (line->last_coords) {
		line->coords[i] = line->last_coords[0];
		line->coords[i + 1] = line->last_coords[1];

		g_free (line->last_coords);
		line->last_coords = NULL;
	}

	if (!line->first_arrow && !line->last_arrow)
		return;

	if (line->width_pixels)
		width = line->width / line->item.canvas->pixels_per_unit;
	else
		width = line->width;

	/* Add fudge value for better-looking results */

	shape_a = line->shape_a;
	shape_b = line->shape_b;
	shape_c = line->shape_c + width / 2.0;

	if (line->width_pixels) {
		shape_a /= line->item.canvas->pixels_per_unit;
		shape_b /= line->item.canvas->pixels_per_unit;
		shape_c /= line->item.canvas->pixels_per_unit;
	}

	shape_a += 0.001;
	shape_b += 0.001;
	shape_c += 0.001;

	/* Compute the polygon for the first arrowhead and adjust the first point in the line so
	 * that the line does not stick out past the leading edge of the arrowhead.
	 */

	frac_height = (line->width / 2.0) / shape_c;
	backup = frac_height * shape_b + shape_a * (1.0 - frac_height) / 2.0;

	if (line->first_arrow) {
		poly = line->first_coords;
		poly[0] = poly[10] = line->coords[0];
		poly[1] = poly[11] = line->coords[1];

		dx = poly[0] - line->coords[2];
		dy = poly[1] - line->coords[3];
		length = sqrt (dx * dx + dy * dy);
		if (length < GNOME_CANVAS_EPSILON)
			sin_theta = cos_theta = 0.0;
		else {
			sin_theta = dy / length;
			cos_theta = dx / length;
		}

		vx = poly[0] - shape_a * cos_theta;
		vy = poly[1] - shape_a * sin_theta;

		tmp = shape_c * sin_theta;

		poly[2] = poly[0] - shape_b * cos_theta + tmp;
		poly[8] = poly[2] - 2.0 * tmp;

		tmp = shape_c * cos_theta;

		poly[3] = poly[1] - shape_b * sin_theta - tmp;
		poly[9] = poly[3] + 2.0 * tmp;

		poly[4] = poly[2] * frac_height + vx * (1.0 - frac_height);
		poly[5] = poly[3] * frac_height + vy * (1.0 - frac_height);
		poly[6] = poly[8] * frac_height + vx * (1.0 - frac_height);
		poly[7] = poly[9] * frac_height + vy * (1.0 - frac_height);

		/* Move the first point towards the second so that the corners at the end of the
		 * line are inside the arrowhead.
		 */

		line->coords[0] = poly[0] - backup * cos_theta;
		line->coords[1] = poly[1] - backup * sin_theta;
	}

	/* Same process for last arrowhead */

	if (line->last_arrow) {
		coords = line->coords + 2 * (line->num_points - 2);
		poly = line->last_coords;
		poly[0] = poly[10] = coords[2];
		poly[1] = poly[11] = coords[3];

		dx = poly[0] - coords[0];
		dy = poly[1] - coords[1];
		length = sqrt (dx * dx + dy * dy);
		if (length < GNOME_CANVAS_EPSILON)
			sin_theta = cos_theta = 0.0;
		else {
			sin_theta = dy / length;
			cos_theta = dx / length;
		}

		vx = poly[0] - shape_a * cos_theta;
		vy = poly[1] - shape_a * sin_theta;

		tmp = shape_c * sin_theta;

		poly[2] = poly[0] - shape_b * cos_theta + tmp;
		poly[8] = poly[2] - 2.0 * tmp;

		tmp = shape_c * cos_theta;

		poly[3] = poly[1] - shape_b * sin_theta - tmp;
		poly[9] = poly[3] + 2.0 * tmp;

		poly[4] = poly[2] * frac_height + vx * (1.0 - frac_height);
		poly[5] = poly[3] * frac_height + vy * (1.0 - frac_height);
		poly[6] = poly[8] * frac_height + vx * (1.0 - frac_height);
		poly[7] = poly[9] * frac_height + vy * (1.0 - frac_height);

		coords[2] = poly[0] - backup * cos_theta;
		coords[3] = poly[1] - backup * sin_theta;
	}
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

	reconfigure_arrows (line);

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
gnome_canvas_line_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
			int x, int y, int width, int height)
{
	GnomeCanvasLine *line;
	GdkPoint static_points[NUM_STATIC_POINTS];
	GdkPoint *points;
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

	item_to_canvas (item->canvas, line->coords, points, line->num_points, dx, dy, x, y);
	gdk_draw_lines (drawable, line->gc, points, line->num_points);

	if (points != static_points)
		g_free (points);

	/* Draw arrowheads */

	points = static_points;

	if (line->first_arrow) {
		item_to_canvas (item->canvas, line->first_coords, points, NUM_ARROW_POINTS, dx, dy, x, y);
		gdk_draw_polygon (drawable, line->gc, TRUE, points, NUM_ARROW_POINTS);
	}

	if (line->last_arrow) {
		item_to_canvas (item->canvas, line->last_coords, points, NUM_ARROW_POINTS, dx, dy, x, y);
		gdk_draw_polygon (drawable, line->gc, TRUE, points, NUM_ARROW_POINTS);
	}
}

static double
gnome_canvas_line_point (GnomeCanvasItem *item, double x, double y,
			 int cx, int cy, GnomeCanvasItem **actual_item)
{
	GnomeCanvasLine *line;
	double *line_points, *coords;
	double static_points[2 * NUM_STATIC_POINTS];
	double poly[10];
	double best, dist;
	double dx, dy;
	double width;
	int num_points, i;
	int changed_miter_to_bevel;

	line = GNOME_CANVAS_LINE (item);

	*actual_item = item;

	best = 1.0e36;

	/* Handle smoothed lines by generating an expanded set ot points */

	if (line->smooth && (line->num_points > 2)) {
		/* FIXME */
	} else {
		num_points = line->num_points;
		line_points = line->coords;
	}

	/* Compute a polygon for each edge of the line and test the point against it.  The effective
	 * width of the line is adjusted so that it will be at least one pixel thick (so that zero
	 * pixel-wide lines can be pickedup as well).
	 */

	if (line->width_pixels)
		width = line->width / item->canvas->pixels_per_unit;
	else
		width = line->width;

	if (width < (1.0 / item->canvas->pixels_per_unit))
		width = 1.0 / item->canvas->pixels_per_unit;

	changed_miter_to_bevel = 0;

	for (i = num_points, coords = line_points; i >= 2; i--, coords += 2) {
		/* If rounding is done around the first point, then compute distance between the
		 * point and the first point.
		 */

		if (((line->cap == GDK_CAP_ROUND) && (i == num_points))
		    || ((line->join == GDK_JOIN_ROUND) && (i != num_points))) {
			dx = coords[0] - x;
			dy = coords[1] - y;
			dist = sqrt (dx * dx + dy * dy) - width / 2.0;
			if (dist < GNOME_CANVAS_EPSILON) {
				best = 0.0;
				goto done;
			} else if (dist < best)
				best = dist;
		}

		/* Compute the polygonal shape corresponding to this edge, with two points for the
		 * first point of the edge and two points for the last point of the edge.
		 */

		if (i == num_points)
			gnome_canvas_get_butt_points (coords[2], coords[3], coords[0], coords[1],
						      width, (line->cap == GDK_CAP_PROJECTING),
						      poly, poly + 1, poly + 2, poly + 3);
		else if ((line->join == GDK_JOIN_MITER) && !changed_miter_to_bevel) {
			poly[0] = poly[6];
			poly[1] = poly[7];
			poly[2] = poly[4];
			poly[3] = poly[5];
		} else {
			gnome_canvas_get_butt_points (coords[2], coords[3], coords[0], coords[1],
						      width, FALSE,
						      poly, poly + 1, poly + 2, poly + 3);

			/* If this line uses beveled joints, then check the distance to a polygon
			 * comprising the last two points of the previous polygon and the first two
			 * from this polygon; this checks the wedges that fill the mitered point.
			 */

			if ((line->join == GDK_JOIN_BEVEL) || changed_miter_to_bevel) {
				poly[8] = poly[0];
				poly[9] = poly[1];

				dist = gnome_canvas_polygon_to_point (poly, 5, x, y);
				if (dist < GNOME_CANVAS_EPSILON) {
					best = 0.0;
					goto done;
				} else if (dist < best)
					best = dist;

				changed_miter_to_bevel = FALSE;
			}
		}

		if (i == 2)
			gnome_canvas_get_butt_points (coords[0], coords[1], coords[2], coords[3],
						      width, (line->cap == GDK_CAP_PROJECTING),
						      poly + 4, poly + 5, poly + 6, poly + 7);
		else if (line->join == GDK_JOIN_MITER) {
			if (!gnome_canvas_get_miter_points (coords[0], coords[1],
							    coords[2], coords[3],
							    coords[4], coords[5],
							    width,
							    poly + 4, poly + 5, poly + 6, poly + 7)) {
				changed_miter_to_bevel = TRUE;
				gnome_canvas_get_butt_points (coords[0], coords[1], coords[2], coords[3],
							      width, FALSE,
							      poly + 4, poly + 5, poly + 6, poly + 7);
			}
		} else
			gnome_canvas_get_butt_points (coords[0], coords[1], coords[2], coords[3],
						      width, FALSE,
						      poly + 4, poly + 5, poly + 6, poly + 7);

		poly[8] = poly[0];
		poly[9] = poly[1];

		dist = gnome_canvas_polygon_to_point (poly, 5, x, y);
		if (dist < GNOME_CANVAS_EPSILON) {
			best = 0.0;
			goto done;
		} else if (dist < best)
			best = dist;
	}

	/* If caps are rounded, check the distance to the cap around the final end point of the line */

	if (line->cap == GDK_CAP_ROUND) {
		dx = coords[0] - x;
		dy = coords[1] - y;
		dist = sqrt (dx * dx + dy * dy) - width / 2.0;
		if (dist < GNOME_CANVAS_EPSILON) {
			best = 0.0;
			goto done;
		} else
			best = dist;
	}

	/* If there are arrowheads, check the distance to them */

	if (line->first_arrow) {
		dist = gnome_canvas_polygon_to_point (line->first_coords, NUM_ARROW_POINTS, x, y);
		if (dist < GNOME_CANVAS_EPSILON) {
			best = 0.0;
			goto done;
		} else
			best = dist;
	}

	if (line->last_arrow) {
		dist = gnome_canvas_polygon_to_point (line->last_coords, NUM_ARROW_POINTS, x, y);
		if (dist < GNOME_CANVAS_EPSILON) {
			best = 0.0;
			goto done;
		} else
			best = dist;
	}

done:

	if ((line_points != static_points) && (line_points != line->coords))
		g_free (line_points);

	return best;
}

static void
gnome_canvas_line_translate (GnomeCanvasItem *item, double dx, double dy)
{
	GnomeCanvasLine *line;
	int i;
	double *coords;

	line = GNOME_CANVAS_LINE (item);

	for (i = 0, coords = line->coords; i < line->num_points; i++, coords += 2) {
		coords[0] += dx;
		coords[1] += dy;
	}

	if (line->first_arrow)
		for (i = 0, coords = line->first_coords; i < NUM_ARROW_POINTS; i++, coords += 2) {
			coords[0] += dx;
			coords[1] += dy;
		}

	if (line->last_arrow)
		for (i = 0, coords = line->last_coords; i < NUM_ARROW_POINTS; i++, coords += 2) {
			coords[0] += dx;
			coords[1] += dy;
		}

	recalc_bounds (line);
}
