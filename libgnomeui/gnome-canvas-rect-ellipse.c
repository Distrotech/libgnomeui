/* Rectangle and ellipse item types for GnomeCanvas widget
 *
 * This widget is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <math.h>
#include "gnome-canvas.h"


typedef struct {
	GnomeCanvasItem item;

	double x1, y1, x2, y2;		/* Corners of item */
	double width;			/* Outline width */

	int fill_set : 1;
	int outline_set : 1;
	int width_pixels : 1;		/* Is the width specified in pixels or units? */

	gulong fill_pixel;		/* Color for filling */
	gulong outline_pixel;		/* Color for outline */

	GdkGC *fill_gc;			/* GC for filling */
	GdkGC *outline_gc;		/* GC for outline */
} RectEllipse;


static void          re_create         (GnomeCanvas *canvas, GnomeCanvasItem *item, va_list args);
static void          re_destroy        (GnomeCanvas *canvas, GnomeCanvasItem *item);
static void          re_configure      (GnomeCanvas *canvas, GnomeCanvasItem *item, va_list args, int reconfigure);
static void          re_realize        (GnomeCanvas *canvas, GnomeCanvasItem *item);
static void          re_unrealize      (GnomeCanvas *canvas, GnomeCanvasItem *item);
static void          re_map            (GnomeCanvas *canvas, GnomeCanvasItem *item);
static void          re_unmap          (GnomeCanvas *canvas, GnomeCanvasItem *item);
static void          re_draw           (GnomeCanvas *canvas, GnomeCanvasItem *item, GdkDrawable *drawable,
					int x, int y, int width, int heigth);

static double        rect_point        (GnomeCanvas *canvas, GnomeCanvasItem *item, double x, double y);
static GtkVisibility rect_intersect    (GnomeCanvas *canvas, GnomeCanvasItem *item,
					double x, double y, double width, double height);

static double        ellipse_point     (GnomeCanvas *canvas, GnomeCanvasItem *item, double x, double y);
static GtkVisibility ellipse_intersect (GnomeCanvas *canvas, GnomeCanvasItem *item,
					double x, double y, double width, double height);


static GnomeCanvasItemType rect_type = {
	"rectangle",
	sizeof (RectEllipse),
	FALSE,
	re_create,
	re_destroy,
	re_configure,
	re_realize,
	re_unrealize,
	re_map,
	re_unmap,
	re_draw,
	rect_point,
	rect_intersect
};

static GnomeCanvasItemType ellipse_type = {
	"ellipse",
	sizeof (RectEllipse),
	FALSE,
	re_create,
	re_destroy,
	re_configure,
	re_realize,
	re_map,
	re_unmap,
	re_unrealize,
	re_draw,
	ellipse_point,
	ellipse_intersect
};

void
gnome_canvas_register_rect_ellipse (void)
{
	gnome_canvas_register_type (&rect_type);
	gnome_canvas_register_type (&ellipse_type);
}

static void
re_calc_bounds (GnomeCanvas *canvas, RectEllipse *re)
{
	if (re->width_pixels) {
		gnome_canvas_w2c (canvas,
				  re->x1 - re->width / 2,
				  re->y1 - re->width / 2,
				  &re->item.x1,
				  &re->item.y1);
		gnome_canvas_w2c (canvas,
				  re->x2 + re->width / 2,
				  re->y2 + re->width / 2,
				  &re->item.x2,
				  &re->item.y2);
	} else {
		gnome_canvas_w2c (canvas,
				  re->x1 - re->width * canvas->pixels_per_unit / 2,
				  re->y1 - re->width * canvas->pixels_per_unit / 2,
				  &re->item.x1,
				  &re->item.y1);
		gnome_canvas_w2c (canvas,
				  re->x2 + re->width * canvas->pixels_per_unit / 2,
				  re->y2 + re->width * canvas->pixels_per_unit / 2,
				  &re->item.x2,
				  &re->item.y2);
	}

	/* Some safety fudging */

	re->item.x1--;
	re->item.y1--;
	re->item.x2++;
	re->item.y2++;
}

static void
re_create (GnomeCanvas *canvas, GnomeCanvasItem *item, va_list args)
{
	RectEllipse *re;

	re = (RectEllipse *) item;

	re->x1 = va_arg (args, double);
	re->y1 = va_arg (args, double);
	re->x2 = va_arg (args, double);
	re->y2 = va_arg (args, double);

	re->width = 0.0;
	re->fill_set = FALSE;
	re->outline_set = FALSE;
	re->width_pixels = TRUE;
	re->fill_pixel = 0;
	re->outline_pixel = 0;
	re->fill_gc = NULL;
	re->outline_gc = NULL;

	re_configure (canvas, item, args, FALSE);

	re_calc_bounds (canvas, re);
}

static void
re_destroy (GnomeCanvas *canvas, GnomeCanvasItem *item)
{
	/* FIXME */
}

static void
re_configure (GnomeCanvas *canvas, GnomeCanvasItem *item, va_list args, int reconfigure)
{
	RectEllipse *re;
	int done;
	gpointer key;
	int calc_gcs;
	int calc_bounds;
	double dval;
	int ival;
	unsigned int uival;
	double width, height;
	GdkColor color;

	re = (RectEllipse *) item;
	done = FALSE;

	calc_gcs = FALSE;
	calc_bounds = FALSE;

	if (!reconfigure)
		do {
			key = (gpointer) va_arg (args, GnomeCanvasKey);

			if (key == GNOME_CANVAS_END)
				done = TRUE;
			else if (key == GNOME_CANVAS_X) {
				dval = va_arg (args, double);
				width = re->x2 - re->x1;
				re->x1 = dval;
				re->x2 = re->x1 + width;

				calc_bounds = TRUE;
			} else if (key == GNOME_CANVAS_Y) {
				dval = va_arg (args, double);
				height = re->y2 - re->y1;
				re->y1 = dval;
				re->y2 = re->y1 + height;

				calc_bounds = TRUE;
			} else if (key == GNOME_CANVAS_X1) {
				dval = va_arg (args, double);
				if (dval < re->x2)
					re->x1 = dval;

				calc_bounds = TRUE;
			} else if (key == GNOME_CANVAS_Y1) {
				dval = va_arg (args, double);
				if (dval < re->y2)
					re->y1 = dval;

				calc_bounds = TRUE;
			} else if (key == GNOME_CANVAS_X2) {
				dval = va_arg (args, double);
				if (dval > re->x1)
					re->x2 = dval;

				calc_bounds = TRUE;
			} else if (key == GNOME_CANVAS_Y2) {
				dval = va_arg (args, double);
				if (dval > re->y1)
					re->y2 = dval;

				calc_bounds = TRUE;
			} else if (key == GNOME_CANVAS_FILL_COLOR) {
				ival = va_arg (args, int);
				if (GNOME_CANVAS_VGET_COLOR (canvas, &color, ival, args)) {
					re->fill_set = TRUE;
					re->fill_pixel = color.pixel;

					calc_gcs = TRUE;
				} else
					re->fill_set = FALSE;
			} else if (key == GNOME_CANVAS_OUTLINE_COLOR) {
				ival = va_arg (args, int);
				if (GNOME_CANVAS_VGET_COLOR (canvas, &color, ival, args)) {
					re->outline_set = TRUE;
					re->outline_pixel = color.pixel;

					calc_gcs = TRUE;
				} else
					re->outline_set = FALSE;
			} else if (key == GNOME_CANVAS_WIDTH_PIXELS) {
				uival = va_arg (args, unsigned int);
				re->width = uival;
				re->width_pixels = TRUE;

				calc_gcs = TRUE;
				calc_bounds = TRUE;
			} else if (key == GNOME_CANVAS_WIDTH_UNITS) {
				dval = va_arg (args, double);
				re->width = fabs (dval);
				re->width_pixels = FALSE;

				calc_gcs = TRUE;
				calc_bounds = TRUE;
			} else {
				g_warning ("Unknown canvas rectangle/ellipse configuration option %s", (char *) key);
				done = TRUE;
			}
		} while (!done);

	if (calc_gcs || reconfigure) {
		if (re->fill_gc) {
			color.pixel = re->fill_pixel;
			gdk_gc_set_foreground (re->fill_gc, &color);
		}

		if (re->outline_gc) {
			color.pixel = re->outline_pixel;
			gdk_gc_set_foreground (re->outline_gc, &color);

			if (re->width_pixels)
				gdk_gc_set_line_attributes (re->outline_gc,
							    (int) re->width,
							    GDK_LINE_SOLID,
							    GDK_CAP_PROJECTING,
							    GDK_JOIN_MITER);
			else
				gdk_gc_set_line_attributes (re->outline_gc,
							    (int) (re->width * canvas->pixels_per_unit + 0.5),
							    GDK_LINE_SOLID,
							    GDK_CAP_PROJECTING,
							    GDK_JOIN_MITER);
		}
	}

	if (calc_bounds || reconfigure)
		re_calc_bounds (canvas, re);
}

static void
re_realize (GnomeCanvas *canvas, GnomeCanvasItem *item)
{
	RectEllipse *re;
	va_list args;

	re = (RectEllipse *) item;

	re->fill_gc = gdk_gc_new (GTK_WIDGET (canvas)->window);
	re->outline_gc = gdk_gc_new (GTK_WIDGET (canvas)->window);

	re_configure (canvas, item, args, TRUE);
}

static void
re_unrealize (GnomeCanvas *canvas, GnomeCanvasItem *item)
{
	RectEllipse *re;

	re = (RectEllipse *) item;

	gdk_gc_unref (re->fill_gc);
	gdk_gc_unref (re->outline_gc);
}

static void
re_map (GnomeCanvas *canvas, GnomeCanvasItem *item)
{
	/* Do nothing */
}

static void
re_unmap (GnomeCanvas *canvas, GnomeCanvasItem *item)
{
	/* Do nothing */
}

static void
re_draw (GnomeCanvas *canvas, GnomeCanvasItem *item, GdkDrawable *drawable,
	 int x, int y, int width, int heigth)
{
	RectEllipse *re;
	int x1, y1, x2, y2;

	re = (RectEllipse *) item;

	/* Compute the item's bounding box */

	gnome_canvas_w2c (canvas, re->x1, re->y1, &x1, &y1);
	gnome_canvas_w2c (canvas, re->x2, re->y2, &x2, &y2);

	if (item->type == &rect_type) {
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
	} else if (item->type == &ellipse_type) {
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
	} else
		g_assert_not_reached ();
}

static double
rect_point (GnomeCanvas *canvas, GnomeCanvasItem *item, double x, double y)
{
	RectEllipse *rect;
	double x1, y1, x2, y2;
	double hwidth;
	double dx, dy;
	double tmp;

	rect = (RectEllipse *) item;

	/* Find the bounds for the rectangle plus its outline width */

	x1 = rect->x1;
	y1 = rect->y1;
	x2 = rect->x2;
	y2 = rect->y2;

	if (rect->outline_set) {
		if (rect->width_pixels)
			hwidth = rect->width * canvas->pixels_per_unit / 2.0;
		else
			hwidth = rect->width / 2.0;

		x1 -= hwidth;
		y1 -= hwidth;
		x2 += hwidth;
		y2 += hwidth;
	}
	else
		hwidth = 0.0;

	/* Is point inside rectangle (which can be hollow if it has no fill set)? */

	if ((x >= x1) && (y >= y1) && (x <= x2) && (y <= y2)) {
		if (rect->fill_set || !rect->outline_set)
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

	return 1000.0; /* FIXME */

	/* AQUI AQUI AQUI */
}

static GtkVisibility
rect_intersect (GnomeCanvas *canvas, GnomeCanvasItem *item,
		double x, double y, double width, double height)
{
	/* FIXME */
	return GTK_VISIBILITY_NONE;
}

static double
ellipse_point (GnomeCanvas *canvas, GnomeCanvasItem *item, double x, double y)
{
	/* FIXME */
	return 0.0;
}

static GtkVisibility
ellipse_intersect (GnomeCanvas *canvas, GnomeCanvasItem *item,
		   double x, double y, double width, double height)
{
	/* FIXME */
	return GTK_VISIBILITY_NONE;
}
