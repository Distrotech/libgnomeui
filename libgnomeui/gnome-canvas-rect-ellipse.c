/* Rectangle and ellipse item types for GnomeCanvas widgetr
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
#include "gnome-canvas-rect-ellipse.h"


/* Base class for rectangle and ellipse item types */


enum {
	ARG_0,
	ARG_X1,
	ARG_Y1,
	ARG_X2,
	ARG_Y2,
	ARG_FILL_COLOR,
	ARG_OUTLINE_COLOR,
	ARG_WIDTH_PIXELS,
	ARG_WIDTH_UNITS
};


static void gnome_canvas_re_class_init (GnomeCanvasREClass *class);
static void gnome_canvas_re_init       (GnomeCanvasRE      *re);
static void gnome_canvas_re_set_arg    (GtkObject          *object,
					GtkArg             *arg,
					guint               arg_id);

static void gnome_canvas_re_reconfigure (GnomeCanvasItem *item);
static void gnome_canvas_re_realize     (GnomeCanvasItem *item);
static void gnome_canvas_re_unrealize   (GnomeCanvasItem *item);



GtkType
gnome_canvas_re_get_type (void)
{
	static GtkType re_type = 0;

	if (!re_type) {
		GtkTypeInfo re_info = {
			"GnomeCanvasRE",
			sizeof (GnomeCanvasRE),
			sizeof (GnomeCanvasREClass),
			(GtkClassInitFunc) gnome_canvas_re_class_init,
			(GtkObjectInitFunc) gnome_canvas_re_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		re_type = gtk_type_unique (gnome_canvas_item_get_type (), &re_info);
	}

	return re_type;
}

static void
gnome_canvas_re_class_init (GnomeCanvasREClass *class)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	gtk_object_add_arg_type ("GnomeCanvasRE::x1",  		 GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_X1);
	gtk_object_add_arg_type ("GnomeCanvasRE::y1",  		 GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_Y1);
	gtk_object_add_arg_type ("GnomeCanvasRE::x2",  		 GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_X2);
	gtk_object_add_arg_type ("GnomeCanvasRE::y2",  		 GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_Y2);
	gtk_object_add_arg_type ("GnomeCanvasRE::fill_color",    GTK_TYPE_STRING, GTK_ARG_WRITABLE, ARG_FILL_COLOR);
	gtk_object_add_arg_type ("GnomeCanvasRE::outline_color", GTK_TYPE_STRING, GTK_ARG_WRITABLE, ARG_OUTLINE_COLOR);
	gtk_object_add_arg_type ("GnomeCanvasRE::width_pixels",  GTK_TYPE_UINT,   GTK_ARG_WRITABLE, ARG_WIDTH_PIXELS);
	gtk_object_add_arg_type ("GnomeCanvasRE::width_units",   GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_WIDTH_UNITS);

	object_class->set_arg = gnome_canvas_re_set_arg;

	item_class->reconfigure = gnome_canvas_re_reconfigure;
	item_class->realize = gnome_canvas_re_realize;
	item_class->unrealize = gnome_canvas_re_unrealize;
}

static void
gnome_canvas_re_init (GnomeCanvasRE *re)
{
	re->x1 = 0.0;
	re->y1 = 0.0;
	re->x2 = 0.0;
	re->y2 = 0.0;
	re->width = 0.0;
}

static void
recalc_bounds (GnomeCanvasRE *re)
{
	GnomeCanvasItem *item;
	double hwidth;

	item = GNOME_CANVAS_ITEM (re);

	if (re->width_pixels)
		hwidth = (re->width / item->canvas->pixels_per_unit) / 2.0;
	else
		hwidth = re->width / 2.0;

	gnome_canvas_w2c (item->canvas, re->x1 - hwidth, re->y1 - hwidth, &item->x1, &item->y1);
	gnome_canvas_w2c (item->canvas, re->x2 + hwidth, re->y2 + hwidth, &item->x2, &item->y2);

	/* Some safety fudging */

	item->x1 -= 2;
	item->y1 -= 2;
	item->x2 += 2;
	item->y2 += 2;
}

static void
gnome_canvas_re_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasItem *item;
	GnomeCanvasRE *re;
	int calc_bounds;
	int calc_gcs;
	GdkColor color;

	item = GNOME_CANVAS_ITEM (object);
	re = GNOME_CANVAS_RE (object);

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);

	calc_bounds = FALSE;
	calc_gcs = FALSE;

	switch (arg_id) {
	case ARG_X1:
		re->x1 = GTK_VALUE_DOUBLE (*arg);
		calc_bounds = TRUE;
		break;

	case ARG_Y1:
		re->y1 = GTK_VALUE_DOUBLE (*arg);
		calc_bounds = TRUE;
		break;

	case ARG_X2:
		re->x2 = GTK_VALUE_DOUBLE (*arg);
		calc_bounds = TRUE;
		break;

	case ARG_Y2:
		re->y2 = GTK_VALUE_DOUBLE (*arg);
		calc_bounds = TRUE;
		break;

	case ARG_FILL_COLOR:
		if (gnome_canvas_get_color (item->canvas, GTK_VALUE_STRING (*arg), &color)) {
			re->fill_set = TRUE;
			re->fill_pixel = color.pixel;
			calc_gcs = TRUE;
		} else
			re->fill_set = FALSE;

		break;

	case ARG_OUTLINE_COLOR:
		if (gnome_canvas_get_color (item->canvas, GTK_VALUE_STRING (*arg), &color)) {
			re->outline_set = TRUE;
			re->outline_pixel = color.pixel;
			calc_gcs = TRUE;
		} else
			re->outline_set = FALSE;

		break;

	case ARG_WIDTH_PIXELS:
		re->width = GTK_VALUE_UINT (*arg);
		re->width_pixels = TRUE;
		calc_gcs = TRUE;
		calc_bounds = TRUE;
		break;

	case ARG_WIDTH_UNITS:
		re->width = fabs (GTK_VALUE_DOUBLE (*arg));
		re->width_pixels = FALSE;
		calc_gcs = TRUE;
		calc_bounds = TRUE;
		break;

	default:
		break;
	}

	if (calc_gcs)
		if (GNOME_CANVAS_ITEM_CLASS (item->object.klass)->reconfigure)
			(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->reconfigure) (item);

	if (calc_bounds)
		recalc_bounds (re);
		
	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
}

static void
gnome_canvas_re_reconfigure (GnomeCanvasItem *item)
{
	GnomeCanvasRE *re;
	GdkColor color;
	int width;

	re = GNOME_CANVAS_RE (item);

	if (re->fill_gc) {
		color.pixel = re->fill_pixel;
		gdk_gc_set_foreground (re->fill_gc, &color);
	}

	if (re->outline_gc) {
		color.pixel = re->outline_pixel;
		gdk_gc_set_foreground (re->outline_gc, &color);

		if (re->width_pixels)
			width = (int) re->width;
		else
			width = (int) (re->width * item->canvas->pixels_per_unit + 0.5);

		gdk_gc_set_line_attributes (re->outline_gc, width,
					    GDK_LINE_SOLID, GDK_CAP_PROJECTING, GDK_JOIN_MITER);
	}

	recalc_bounds (re);
}

static void
gnome_canvas_re_realize (GnomeCanvasItem *item)
{
	GnomeCanvasRE *re;

	re = GNOME_CANVAS_RE (item);

	re->fill_gc = gdk_gc_new (GTK_WIDGET (item->canvas)->window);
	re->outline_gc = gdk_gc_new (GTK_WIDGET (item->canvas)->window);

	gnome_canvas_re_reconfigure (item);
}

static void
gnome_canvas_re_unrealize (GnomeCanvasItem *item)
{
	GnomeCanvasRE *re;

	re = GNOME_CANVAS_RE (item);

	gdk_gc_unref (re->fill_gc);
	gdk_gc_unref (re->outline_gc);
}


/* Rectangle item */


static void gnome_canvas_rect_class_init (GnomeCanvasRectClass *class);

static void   gnome_canvas_rect_draw  (GnomeCanvasItem *item, GdkDrawable *drawable, int x, int y, int width, int height);
static double gnome_canvas_rect_point (GnomeCanvasItem *item, double x, double y, int cx, int cy,
				       GnomeCanvasItem **actual_item);


GtkType
gnome_canvas_rect_get_type (void)
{
	static GtkType rect_type = 0;

	if (!rect_type) {
		GtkTypeInfo rect_info = {
			"GnomeCanvasRect",
			sizeof (GnomeCanvasRect),
			sizeof (GnomeCanvasRectClass),
			(GtkClassInitFunc) gnome_canvas_rect_class_init,
			(GtkObjectInitFunc) NULL,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		rect_type = gtk_type_unique (gnome_canvas_re_get_type (), &rect_info);
	}

	return rect_type;
}

static void
gnome_canvas_rect_class_init (GnomeCanvasRectClass *class)
{
	GnomeCanvasItemClass *item_class;

	item_class = (GnomeCanvasItemClass *) class;

	item_class->draw = gnome_canvas_rect_draw;
	item_class->point = gnome_canvas_rect_point;
}

static void
gnome_canvas_rect_draw (GnomeCanvasItem *item, GdkDrawable *drawable, int x, int y, int width, int height)
{
	GnomeCanvasRE *re;
	double wx1, wy1, wx2, wy2;
	int x1, y1, x2, y2;

	re = GNOME_CANVAS_RE (item);

	/* Get world coordinates */

	wx1 = re->x1;
	wy1 = re->y1;
	wx2 = re->x2;
	wy2 = re->y2;

	gnome_canvas_item_i2w (item, &wx1, &wy1);
	gnome_canvas_item_i2w (item, &wx2, &wy2);

	/* Get canvas pixel coordinates */

	gnome_canvas_w2c (item->canvas, wx1, wy1, &x1, &y1);
	gnome_canvas_w2c (item->canvas, wx2, wy2, &x2, &y2);

	if (re->fill_set)
		gdk_draw_rectangle (drawable,
				    re->fill_gc,
				    TRUE,
				    x1 - x,
				    y1 - y,
				    x2 - x1 + 1,
				    y2 - y1 + 1);

	if (re->outline_set)
		gdk_draw_rectangle (drawable,
				    re->outline_gc,
				    FALSE,
				    x1 - x,
				    y1 - y,
				    x2 - x1 + 1,
				    y2 - y1 + 1);
}

static double
gnome_canvas_rect_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item)
{
	GnomeCanvasRE *re;
	double x1, y1, x2, y2;
	double hwidth;
	double dx, dy;
	double tmp;

	re = GNOME_CANVAS_RE (item);

	*actual_item = item;

	/* Find the bounds for the rectangle plus its outline width */

	x1 = re->x1;
	y1 = re->y1;
	x2 = re->x2;
	y2 = re->y2;

	if (re->outline_set) {
		if (re->width_pixels)
			hwidth = (re->width / item->canvas->pixels_per_unit) / 2.0;
		else
			hwidth = re->width / 2.0;

		x1 -= hwidth;
		y1 -= hwidth;
		x2 += hwidth;
		y2 += hwidth;
	} else
		hwidth = 0.0;

	/* Is point inside rectangle (which can be hollow if it has no fill set)? */

	if ((x >= x1) && (y >= y1) && (x <= x2) && (y <= y2)) {
		if (re->fill_set || !re->outline_set)
			return 0.0;

		dx = x - x1;
		tmp = x2 - x;
		if (tmp < dx)
			dx = tmp;

		dy = y - y1;
		tmp = y2 - y;
		if (tmp < dy)
			dy = tmp;

		if (dy < dx)
			dx = dy;

		dx -= 2.0 * hwidth;

		if (dx < 0.0)
			return 0.0;
		else
			return dx;
	}

	/* Point is outside rectangle */

	if (x < x1)
		dx = x1 - x;
	else if (x > x2)
		dx = x - x2;
	else
		dx = 0.0;

	if (y < y1)
		dy = y1 - y;
	else if (y > y2)
		dy = y - y2;
	else
		dy = 0.0;

	return sqrt (dx * dx + dy * dy);
}


/* Ellipse item */


static void gnome_canvas_ellipse_class_init (GnomeCanvasEllipseClass *class);

static void   gnome_canvas_ellipse_draw  (GnomeCanvasItem *item, GdkDrawable *drawable, int x, int y,
					  int width, int height);
static double gnome_canvas_ellipse_point (GnomeCanvasItem *item, double x, double y, int cx, int cy,
					  GnomeCanvasItem **actual_item);


GtkType
gnome_canvas_ellipse_get_type (void)
{
	static GtkType ellipse_type = 0;

	if (!ellipse_type) {
		GtkTypeInfo ellipse_info = {
			"GnomeCanvasEllipse",
			sizeof (GnomeCanvasEllipse),
			sizeof (GnomeCanvasEllipseClass),
			(GtkClassInitFunc) gnome_canvas_ellipse_class_init,
			(GtkObjectInitFunc) NULL,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		ellipse_type = gtk_type_unique (gnome_canvas_re_get_type (), &ellipse_info);
	}

	return ellipse_type;
}

static void
gnome_canvas_ellipse_class_init (GnomeCanvasEllipseClass *class)
{
	GnomeCanvasItemClass *item_class;

	item_class = (GnomeCanvasItemClass *) class;

	item_class->draw = gnome_canvas_ellipse_draw;
	item_class->point = gnome_canvas_ellipse_point;
}

static void
gnome_canvas_ellipse_draw (GnomeCanvasItem *item, GdkDrawable *drawable, int x, int y, int width, int height)
{
	GnomeCanvasRE *re;
	double wx1, wy1, wx2, wy2;
	int x1, y1, x2, y2;

	re = GNOME_CANVAS_RE (item);

	/* Get world coordinates */

	wx1 = re->x1;
	wy1 = re->y1;
	wx2 = re->x2;
	wy2 = re->y2;

	gnome_canvas_item_i2w (item, &wx1, &wy1);
	gnome_canvas_item_i2w (item, &wx2, &wy2);

	/* Get canvas pixel coordinates */

	gnome_canvas_w2c (item->canvas, wx1, wy1, &x1, &y1);
	gnome_canvas_w2c (item->canvas, wx2, wy2, &x2, &y2);

	if (re->fill_set)
		gdk_draw_arc (drawable,
			      re->fill_gc,
			      TRUE,
			      x1 - x,
			      y1 - y,
			      x2 - x1 + 1,
			      y2 - y1 + 1,
			      0 * 64,
			      360 * 64);

	if (re->outline_set)
		gdk_draw_arc (drawable,
			      re->outline_gc,
			      FALSE,
			      x1 - x,
			      y1 - y,
			      x2 - x1 + 1,
			      y2 - y1 + 1,
			      0 * 64,
			      360 * 64);
}

static double
gnome_canvas_ellipse_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item)
{
	/* FIXME */

	return 10000.0;
}
