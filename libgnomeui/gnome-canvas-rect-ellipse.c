/* Rectangle and ellipse item types for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas
 * widget.  Tk is copyrighted by the Regents of the University of California,
 * Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Authors: Federico Mena <federico@nuclecu.unam.mx>
 *          Raph Levien <raph@gimp.org>
 */

#include <config.h>
#include <math.h>
#include <gtk/gtkgc.h>
#include "gnome-canvas-rect-ellipse.h"
#include "gnome-canvas-util.h"
#include "libart_lgpl/art_vpath.h"
#include "libart_lgpl/art_svp.h"
#include "libart_lgpl/art_svp_vpath.h"
#include "libart_lgpl/art_rgb_svp.h"


/* Base class for rectangle and ellipse item types */

/* Object argument IDs */
enum {
	ARG_0,
	ARG_X1,
	ARG_Y1,
	ARG_X2,
	ARG_Y2,
	ARG_FILL_COLOR,
	ARG_FILL_COLOR_GDK,
	ARG_FILL_COLOR_RGBA,
	ARG_OUTLINE_COLOR,
	ARG_OUTLINE_COLOR_GDK,
	ARG_OUTLINE_COLOR_RGBA,
	ARG_FILL_STIPPLE,
	ARG_OUTLINE_STIPPLE,
	ARG_WIDTH_PIXELS,
	ARG_WIDTH_UNITS,
	ARG_WIDTH,
	ARG_WIDTH_IS_IN_PIXELS
};


/* Private data of the GnomeCanvasRE structure */
typedef struct {
	/* GCs for fill and outline */
	GdkGC *fill_gc;
	GdkGC *outline_gc;

	/* Stipples for fill and outline */
	GdkBitmap *fill_stipple;
	GdkBitmap *outline_stipple;

	/* SVPs for fill and outline shapes */
	ArtSVP *fill_svp;
	ArtSVP *outline_svp;

	/* Whether the fill and outline colors are set */
	guint fill_set : 1;
	guint outline_set : 1;

	/* Whether the outline width is specified in pixels or units */
	guint width_pixels : 1;

	/* Whether the item needs to be reshaped */
	guint need_shape_update : 1;

	/* Whether the fill or outline properties have changed */
	guint need_fill_update : 1;
	guint need_outline_update : 1;

	/* Whether we have the pixel or RGBA information for the colors */
	guint have_fill_pixel : 1;
	guint have_outline_pixel : 1;
} REPrivate;


static void gnome_canvas_re_class_init (GnomeCanvasREClass *class);
static void gnome_canvas_re_init       (GnomeCanvasRE      *re);
static void gnome_canvas_re_destroy    (GtkObject          *object);
static void gnome_canvas_re_set_arg    (GtkObject          *object,
					GtkArg             *arg,
					guint               arg_id);
static void gnome_canvas_re_get_arg    (GtkObject          *object,
					GtkArg             *arg,
					guint               arg_id);

static void gnome_canvas_re_update      (GnomeCanvasItem *item, double *affine,
					 ArtSVP *clip_path, int flags);
static void gnome_canvas_re_unrealize   (GnomeCanvasItem *item);
static void gnome_canvas_re_bounds      (GnomeCanvasItem *item,
					 double *x1, double *y1, double *x2, double *y2);

static void gnome_canvas_re_render      (GnomeCanvasItem *item, GnomeCanvasBuf *buf);


static GnomeCanvasItemClass *re_parent_class;


/**
 * gnome_canvas_re_get_type:
 * @void:
 *
 * Registers the &GnomeCanvasRE class if necessary, and returns the type ID
 * associated to it.
 *
 * Return value: The type ID of the &GnomeCanvasRE class.
 **/
GtkType
gnome_canvas_re_get_type (void)
{
	static GtkType re_type = 0;

	if (!re_type) {
		static const GtkTypeInfo re_info = {
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

/* Class initialization function for the rect/ellipse item */
static void
gnome_canvas_re_class_init (GnomeCanvasREClass *class)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	re_parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	gtk_object_add_arg_type ("GnomeCanvasRE::x1",
				 GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_X1);
	gtk_object_add_arg_type ("GnomeCanvasRE::y1",
				 GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_Y1);
	gtk_object_add_arg_type ("GnomeCanvasRE::x2",
				 GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_X2);
	gtk_object_add_arg_type ("GnomeCanvasRE::y2",
				 GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_Y2);
	gtk_object_add_arg_type ("GnomeCanvasRE::fill_color",
				 GTK_TYPE_STRING, GTK_ARG_WRITABLE, ARG_FILL_COLOR);
	gtk_object_add_arg_type ("GnomeCanvasRE::fill_color_gdk",
				 GTK_TYPE_GDK_COLOR, GTK_ARG_READWRITE, ARG_FILL_COLOR_GDK);
	gtk_object_add_arg_type ("GnomeCanvasRE::fill_color_rgba",
				 GTK_TYPE_UINT, GTK_ARG_READWRITE, ARG_FILL_COLOR_RGBA);
	gtk_object_add_arg_type ("GnomeCanvasRE::outline_color",
				 GTK_TYPE_STRING, GTK_ARG_WRITABLE, ARG_OUTLINE_COLOR);
	gtk_object_add_arg_type ("GnomeCanvasRE::outline_color_gdk",
				 GTK_TYPE_GDK_COLOR, GTK_ARG_READWRITE, ARG_OUTLINE_COLOR_GDK);
	gtk_object_add_arg_type ("GnomeCanvasRE::outline_color_rgba",
				 GTK_TYPE_UINT, GTK_ARG_READWRITE, ARG_OUTLINE_COLOR_RGBA);
	gtk_object_add_arg_type ("GnomeCanvasRE::fill_stipple",
				 GTK_TYPE_GDK_WINDOW, GTK_ARG_READWRITE, ARG_FILL_STIPPLE);
	gtk_object_add_arg_type ("GnomeCanvasRE::outline_stipple",
				 GTK_TYPE_GDK_WINDOW, GTK_ARG_READWRITE, ARG_OUTLINE_STIPPLE);
	gtk_object_add_arg_type ("GnomeCanvasRE::width_pixels",
				 GTK_TYPE_UINT, GTK_ARG_WRITABLE, ARG_WIDTH_PIXELS);
	gtk_object_add_arg_type ("GnomeCanvasRE::width_units",
				 GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_WIDTH_UNITS);
	gtk_object_add_arg_type ("GnomeCanvasRE::width",
				 GTK_TYPE_DOUBLE, GTK_ARG_READABLE, ARG_WIDTH);
	gtk_object_add_arg_type ("GnomeCanvasRE::width_is_in_pixels",
				 GTK_TYPE_BOOL, GTK_ARG_READABLE, ARG_WIDTH_IS_IN_PIXELS);

	object_class->destroy = gnome_canvas_re_destroy;
	object_class->set_arg = gnome_canvas_re_set_arg;
	object_class->get_arg = gnome_canvas_re_get_arg;

	item_class->update = gnome_canvas_re_update;
	item_class->unrealize = gnome_canvas_re_unrealize;
	item_class->render = gnome_canvas_re_render;
	item_class->bounds = gnome_canvas_re_bounds;
}

/* Object initiailization function for the rect/ellipse item */
static void
gnome_canvas_re_init (GnomeCanvasRE *re)
{
	REPrivate *priv;

	priv = g_new0 (REPrivate, 1);
	re->priv = priv;

	re->x1 = 0.0;
	re->y1 = 0.0;
	re->x2 = 0.0;
	re->y2 = 0.0;
	re->width = 0.0;
}

/* Converts a rectangle in world coordinates to canvas pixel coordinates and
 * queues a redraw of the result.
 */
static void
redraw_world_rect (GnomeCanvas *canvas, double x1, double y1, double x2, double y2)
{
	double w2c[6];
	ArtPoint w1, c1, w2, c2;

	gnome_canvas_w2c_affine (canvas, w2c);

	w1.x = x1;
	w1.y = y1;
	w2.x = x2;
	w2.y = y2;

	art_affine_point (&c1, &w1, w2c);
	art_affine_point (&c2, &w2, w2c);

	/* Do we need fudging here? */

	gnome_canvas_request_redraw (canvas,
				     (int) floor (c1.x + 0.5),
				     (int) floor (c1.y + 0.5),
				     (int) floor (c2.x + 0.5) + 1,
				     (int) floor (c2.y + 0.5) + 1);
}

/* Destroy handler for the rect/ellipse item */
static void
gnome_canvas_re_destroy (GtkObject *object)
{
	GnomeCanvasRE *re;
	GnomeCanvasItem *item;
	REPrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_RE (object));

	re = GNOME_CANVAS_RE (object);
	item = GNOME_CANVAS_ITEM (re);
	priv = re->priv;

	/* Redraw last known area */

	if (item->canvas->aa)
		; /* FIXME */
	else
		redraw_world_rect (item->canvas, item->x1, item->y1, item->x2, item->y2);

	/* Free everything */

	if (priv->fill_stipple)
		gdk_bitmap_unref (priv->fill_stipple);

	if (priv->outline_stipple)
		gdk_bitmap_unref (priv->outline_stipple);

	if (priv->fill_svp)
		art_svp_free (priv->fill_svp);

	if (priv->outline_svp)
		art_svp_free (priv->outline_svp);

	g_free (priv);

	if (GTK_OBJECT_CLASS (re_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (re_parent_class)->destroy) (object);
}

/* Set_arg handler for the rect/ellipse item */
static void
gnome_canvas_re_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasItem *item;
	GnomeCanvasRE *re;
	REPrivate *priv;
	GdkColor color = { 0, 0, 0, 0 };
	GdkColor *pcolor;
	char *str;
	GdkBitmap *bitmap;
	int have_pixel;

	item = GNOME_CANVAS_ITEM (object);
	re = GNOME_CANVAS_RE (object);
	priv = re->priv;

	have_pixel = FALSE;

	switch (arg_id) {
	case ARG_X1:
		re->x1 = GTK_VALUE_DOUBLE (*arg);
		priv->need_shape_update = TRUE;
		gnome_canvas_item_request_update (item);
		break;

	case ARG_Y1:
		re->y1 = GTK_VALUE_DOUBLE (*arg);
		priv->need_shape_update = TRUE;
		gnome_canvas_item_request_update (item);
		break;

	case ARG_X2:
		re->x2 = GTK_VALUE_DOUBLE (*arg);
		priv->need_shape_update = TRUE;
		gnome_canvas_item_request_update (item);
		break;

	case ARG_Y2:
		re->y2 = GTK_VALUE_DOUBLE (*arg);
		priv->need_shape_update = TRUE;
		gnome_canvas_item_request_update (item);
		break;

	case ARG_FILL_COLOR:
		str = GTK_VALUE_STRING (*arg);
		if (str) {
			gdk_color_parse (str, &color);
			re->fill_color = ((color.red & 0xff00) << 16 |
					  (color.green & 0xff00) << 8 |
					  (color.blue & 0xff00) |
					  0xff);
			priv->fill_set = TRUE;
		} else {
			re->fill_color = 0; /* All transparent */
			priv->fill_set = FALSE;
		}

		priv->have_fill_pixel = FALSE;
		priv->need_fill_update = TRUE;
		gnome_canvas_item_request_update (item);
		break;

	case ARG_FILL_COLOR_GDK:
		pcolor = GTK_VALUE_BOXED (*arg);
		if (pcolor) {
			re->fill_pixel = pcolor->pixel;
			priv->fill_set = TRUE;
		} else {
			re->fill_pixel = 0;
			priv->fill_set = FALSE;
		}

		priv->have_fill_pixel = TRUE;
		priv->need_fill_update = TRUE;
		gnome_canvas_item_request_update (item);
		break;

	case ARG_FILL_COLOR_RGBA:
		re->fill_color = GTK_VALUE_UINT (*arg);
		priv->fill_set = TRUE;
		priv->have_fill_pixel = FALSE;
		priv->need_fill_update = TRUE;
		gnome_canvas_item_request_update (item);
		break;

	case ARG_OUTLINE_COLOR:
		str = GTK_VALUE_STRING (*arg);
		if (str) {
			gdk_color_parse (str, &color);
			re->outline_color = ((color.red & 0xff00) << 16 |
					     (color.green & 0xff00) << 8 |
					     (color.blue & 0xff00) |
					     0xff);
			priv->outline_set = TRUE;
		} else {
			re->outline_color = 0; /* All transparent */
			priv->outline_set = FALSE;
		}

		priv->have_outline_pixel = FALSE;
		priv->need_outline_update = TRUE;
		gnome_canvas_item_request_update (item);
		break;

	case ARG_OUTLINE_COLOR_GDK:
		pcolor = GTK_VALUE_BOXED (*arg);
		if (pcolor) {
			re->outline_pixel = pcolor->pixel;
			priv->outline_set = TRUE;
		} else {
			re->outline_pixel = 0;
			priv->outline_set = FALSE;
		}

		priv->have_outline_pixel = TRUE;
		priv->need_outline_update = TRUE;
		gnome_canvas_item_request_update (item);
		break;

	case ARG_OUTLINE_COLOR_RGBA:
		re->outline_color = GTK_VALUE_UINT (*arg);
		priv->outline_set = TRUE;
		priv->have_outline_pixel = FALSE;
		priv->need_outline_update = TRUE;
		gnome_canvas_item_request_update (item);
		break;

	case ARG_FILL_STIPPLE:
		bitmap = GTK_VALUE_BOXED (*arg);
		if (bitmap)
			gdk_bitmap_ref (bitmap);

		if (priv->fill_stipple)
			gdk_bitmap_unref (priv->fill_stipple);

		priv->fill_stipple = bitmap;
		priv->need_fill_update = TRUE;
		gnome_canvas_item_request_update (item);
		break;

	case ARG_OUTLINE_STIPPLE:
		bitmap = GTK_VALUE_BOXED (*arg);
		if (bitmap)
			gdk_bitmap_ref (bitmap);

		if (priv->outline_stipple)
			gdk_bitmap_unref (priv->outline_stipple);

		priv->outline_stipple = bitmap;
		priv->need_outline_update = TRUE;
		gnome_canvas_item_request_update (item);
		break;

	case ARG_WIDTH_PIXELS:
		re->width = GTK_VALUE_UINT (*arg);
		priv->width_pixels = TRUE;
		priv->need_shape_update = TRUE;
		priv->need_outline_update = TRUE;
		gnome_canvas_item_request_update (item);
		break;

	case ARG_WIDTH_UNITS:
		re->width = fabs (GTK_VALUE_DOUBLE (*arg));
		priv->width_pixels = FALSE;
		priv->need_shape_update = TRUE;
		priv->need_outline_update = TRUE;
		gnome_canvas_item_request_update (item);
		break;

	default:
		break;
	}
}

/* Ensures that all the color information for a rect/ellipse item is available,
 * and returns both the RGBA and pixel values that need to be stored.
 */
static void
ensure_color_info (GnomeCanvas *canvas, guint *ret_rgba, gulong *ret_pixel,
		   guint rgba, gulong pixel, int is_set, int have_pixel)
{
	GdkColor color;

	if (have_pixel) {
		*ret_pixel = pixel;

		if (is_set) {
			color.pixel = pixel;
			gdk_color_context_query_color (canvas->cc, &color);
			*ret_rgba = ((color.red & 0xff00) << 16 |
				     (color.green & 0xff00) << 8 |
				     (color.blue & 0xff00) |
				     0xff);
		} else
			*ret_rgba = 0; /* All transparent */
	} else {
		*ret_rgba = rgba;

		if (is_set)
			*ret_pixel = gnome_canvas_get_color_pixel (canvas, rgba);
		else
			*ret_pixel = 0; /* Transparent, so we can't do any better */
	}
}

/* This is used by the gnome_canvas_re_get_arg() to query colors from a
 * rect/ellipse item.  It is ugly because it has to handle items that may or may
 * not need color updates, and if they do, they may have different parameters
 * set or not.
 */
static void
get_color_arg (GnomeCanvasRE *re, guint rgba, gulong pixel,
	       int is_set, int have_pixel, int need_update,
	       int want_gdk, GtkArg *arg)
{
	guint ret_rgba;
	gulong ret_pixel;

	if (need_update)
		ensure_color_info (re->item.canvas, &ret_rgba, &ret_pixel,
				   rgba, pixel, is_set, have_pixel);
	else {
		ret_rgba = rgba;
		ret_pixel = pixel;
	}

	if (want_gdk) {
		GdkColor *color;

		color = g_new (GdkColor, 1);
		color->pixel = ret_pixel;
		color->red = ((ret_rgba & 0xff000000) >> 16) + ((ret_rgba & 0xff000000) >> 24);
		color->green = ((ret_rgba & 0x00ff0000) >> 8) + ((ret_rgba & 0x00ff0000) >> 16);
		color->blue = (ret_rgba & 0x0000ff00) + ((ret_rgba & 0x0000ff00) >> 8);

		GTK_VALUE_BOXED (*arg) = color;
	} else
		GTK_VALUE_UINT (*arg) = ret_rgba;
}

/* Get_arg handler for the rect/ellipse item */
static void
gnome_canvas_re_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasRE *re;
	REPrivate *priv;

	re = GNOME_CANVAS_RE (object);
	priv = re->priv;

	switch (arg_id) {
	case ARG_X1:
		GTK_VALUE_DOUBLE (*arg) = re->x1;
		break;

	case ARG_Y1:
		GTK_VALUE_DOUBLE (*arg) = re->y1;
		break;

	case ARG_X2:
		GTK_VALUE_DOUBLE (*arg) = re->x2;
		break;

	case ARG_Y2:
		GTK_VALUE_DOUBLE (*arg) = re->y2;
		break;

	case ARG_FILL_COLOR_GDK:
		get_color_arg (re, re->fill_color, re->fill_pixel,
			       priv->fill_set, priv->have_fill_pixel, priv->need_fill_update,
			       TRUE, arg);
		break;

	case ARG_FILL_COLOR_RGBA:
		get_color_arg (re, re->fill_color, re->fill_pixel,
			       priv->fill_set, priv->have_fill_pixel, priv->need_fill_update,
			       FALSE, arg);
		break;

	case ARG_OUTLINE_COLOR_GDK:
		get_color_arg (re, re->outline_color, re->outline_pixel,
			       priv->outline_set, priv->have_outline_pixel, priv->need_outline_update,
			       TRUE, arg);
		break;

	case ARG_OUTLINE_COLOR_RGBA:
		get_color_arg (re, re->outline_color, re->outline_pixel,
			       priv->outline_set, priv->have_outline_pixel, priv->need_outline_update,
			       FALSE, arg);
		break;

	case ARG_FILL_STIPPLE:
		GTK_VALUE_BOXED (*arg) = priv->fill_stipple;
		break;

	case ARG_OUTLINE_STIPPLE:
		GTK_VALUE_BOXED (*arg) = priv->outline_stipple;
		break;

	case ARG_WIDTH:
		GTK_VALUE_DOUBLE (*arg) = re->width;
		break;

	case ARG_WIDTH_IS_IN_PIXELS:
		GTK_VALUE_BOOL (*arg) = priv->width_pixels;
		break;

	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
}

/* Updates a possibly existing GC in the rect/ellipse item.  It uses the gtk_gc
 * memoization functions so that many similar canvas items can share GCs.
 */
static void
update_gc (GnomeCanvas *canvas, GdkGC **gc, gulong pixel,
	   int need_width, double width, int width_pixels,
	   GdkBitmap *stipple)
{
	GdkGCValues values;
	int mask;

	if (*gc)
		gtk_gc_release (*gc);

	values.foreground.pixel = pixel;
	mask = GDK_GC_FOREGROUND;

	if (need_width) {
		mask |= GDK_GC_LINE_WIDTH;

		if (width_pixels)
			values.line_width = (int) width;
		else
			values.line_width = (int) (width * canvas->pixels_per_unit + 0.5);
	}

	if (stipple) {
		mask |= GDK_GC_FILL | GDK_GC_STIPPLE;

		values.fill = GDK_STIPPLED;
		values.stipple = stipple;
	}

	*gc = gtk_gc_get (gtk_widget_get_visual (GTK_WIDGET (canvas))->depth,
			  gtk_widget_get_colormap (GTK_WIDGET (canvas)),
			  &values,
			  mask);
}

/* Gets the corners of an rect/ellipse item, so that x1 <= x2 and y1 <= y2 */
static void
get_corners (GnomeCanvasRE *re, double *x1, double *y1, double *x2, double *y2)
{
	if (re->x1 < re->x2) {
		*x1 = re->x1;
		*x2 = re->x2;
	} else {
		*x1 = re->x2;
		*x2 = re->x1;
	}

	if (re->y1 < re->y2) {
		*y1 = re->y1;
		*y2 = re->y2;
	} else {
		*y1 = re->y2;
		*y2 = re->y1;
	}
}

/* Both the rectangle and ellipse items can share the same shape update code for
 * the non-antialiased case.  For them we just repaint and recalculate the
 * bounding box if appropriate.
 */
static void
update_non_aa (GnomeCanvasRE *re, double *affine, ArtSVP *clip_path, int flags)
{
	GnomeCanvasItem *item;
	REPrivate *priv;

	item = GNOME_CANVAS_ITEM (re);
	priv = re->priv;

	/* Redraw old area if necessary */

	if (((flags & GNOME_CANVAS_UPDATE_VISIBILITY)
	     && !(GTK_OBJECT_FLAGS (re) & GNOME_CANVAS_ITEM_VISIBLE))
	    || (flags & GNOME_CANVAS_UPDATE_AFFINE)
	    || priv->need_shape_update)
		redraw_world_rect (item->canvas, item->x1, item->y1, item->x2, item->y2);

	/* If we need a shape update, or if the item changed visibility
	 * to shown, recompute the bounding box.
	 */
	if (priv->need_shape_update
	    || ((flags & GNOME_CANVAS_UPDATE_VISIBILITY)
		&& (GTK_OBJECT_FLAGS (re) & GNOME_CANVAS_ITEM_VISIBLE))
	    || (flags & GNOME_CANVAS_UPDATE_AFFINE)) {
		ArtPoint i1, i2, w1, w2;
		double hwidth;
		double i2w[6];

		get_corners (re, &i1.x, &i1.y, &i2.x, &i2.y);
		if (priv->width_pixels)
			hwidth = (re->width / item->canvas->pixels_per_unit) / 2.0;
		else
			hwidth = re->width / 2.0;

		i1.x -= hwidth;
		i1.y -= hwidth;
		i2.x += hwidth;
		i2.y += hwidth;

		gnome_canvas_item_i2w_affine (item, i2w);
		art_affine_point (&w1, &i1, i2w);
		art_affine_point (&w2, &i2, i2w);

		item->x1 = w1.x;
		item->y1 = w1.y;
		item->x2 = w2.x;
		item->y2 = w2.y;

		priv->need_shape_update = FALSE;
	}

	/* If the fill our outline changed, we need to redraw, anyways */

	redraw_world_rect (item->canvas, item->x1, item->y1, item->x2, item->y2);
	priv->need_fill_update = FALSE;
	priv->need_outline_update = FALSE;

}

/* Update handler for the rect/ellipse item.  Both items share the outline and
 * fill color properties, so we update them if necessary.
 */
static void
gnome_canvas_re_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	GnomeCanvasRE *re;
	REPrivate *priv;
	guint rgba;
	gulong pixel;

	re = GNOME_CANVAS_RE (item);
	priv = re->priv;

	if (re_parent_class->update)
		(* re_parent_class->update) (item, affine, clip_path, flags);

	if (priv->need_fill_update) {
		ensure_color_info (item->canvas, &rgba, &pixel,
				   re->fill_color, re->fill_pixel,
				   priv->fill_set, priv->have_fill_pixel);
		re->fill_color = rgba;
		re->fill_pixel = pixel;

		if (!item->canvas->aa)
			update_gc (item->canvas, &priv->fill_gc, re->fill_pixel,
				   FALSE, 0.0, FALSE,
				   priv->fill_stipple);

		priv->need_fill_update = FALSE;
	}

	if (priv->need_outline_update) {
		ensure_color_info (item->canvas, &rgba, &pixel,
				   re->outline_color, re->outline_pixel,
				   priv->outline_set, priv->have_outline_pixel);
		re->outline_color = rgba;
		re->outline_pixel = pixel;

		if (!item->canvas->aa)
			update_gc (item->canvas, &priv->outline_gc, re->outline_pixel,
				   TRUE, re->width, priv->width_pixels,
				   priv->outline_stipple);

		priv->need_outline_update = FALSE;
	}

	/* Both the rectangle and ellipse items can share the same update code
	 * for the non-antialiased case.
	 */

	if (!item->canvas->aa)
		update_non_aa (re, affine, clip_path, flags);
}

/* Unrealize handler for the rect/ellipse item */
static void
gnome_canvas_re_unrealize (GnomeCanvasItem *item)
{
	GnomeCanvasRE *re;
	REPrivate *priv;

	re = GNOME_CANVAS_RE (item);
	priv = re->priv;

	if (priv->fill_gc) {
		gtk_gc_release (priv->fill_gc);
		priv->fill_gc = NULL;
	}

	if (priv->outline_gc) {
		gtk_gc_release (priv->outline_gc);
		priv->outline_gc = NULL;
	}

	if (re_parent_class->unrealize)
		(* re_parent_class->unrealize) (item);
}

/* Bounds handler for the rect/ellipse item */
static void
gnome_canvas_re_bounds (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	GnomeCanvasRE *re;
	REPrivate *priv;
	double hwidth;
	double rx1, ry1, rx2, ry2;

	re = GNOME_CANVAS_RE (item);
	priv = re->priv;

	if (priv->width_pixels)
		hwidth = (re->width / item->canvas->pixels_per_unit) / 2.0;
	else
		hwidth = re->width / 2.0;

	get_corners (re, &rx1, &ry1, &rx2, &ry2);

	*x1 = rx1 - hwidth;
	*y1 = ry1 - hwidth;
	*x2 = rx2 + hwidth;
	*y2 = ry2 + hwidth;
}

static void
gnome_canvas_re_render (GnomeCanvasItem *item,
			GnomeCanvasBuf *buf)
{
	GnomeCanvasRE *re;
	REPrivate *priv;

	re = GNOME_CANVAS_RE (item);
	priv = re->priv;

#ifdef VERBOSE
	g_print ("gnome_canvas_re_render (%d, %d) - (%d, %d) fill=%08x outline=%08x\n",
		 buf->rect.x0, buf->rect.y0, buf->rect.x1, buf->rect.y1, re->fill_color, re->outline_color);
#endif

	if (priv->fill_svp)
		gnome_canvas_render_svp (buf, priv->fill_svp, re->fill_color);

	if (priv->outline_svp) {
		gnome_canvas_render_svp (buf, priv->outline_svp, re->outline_color);
	}
}


/* Rectangle item */

static void gnome_canvas_rect_class_init (GnomeCanvasRectClass *class);

static void   gnome_canvas_rect_update (GnomeCanvasItem *item, double *affine,
					ArtSVP *clip_path, int flags);
static void   gnome_canvas_rect_draw   (GnomeCanvasItem *item, GdkDrawable *drawable,
					int x, int y, int width, int height);
static double gnome_canvas_rect_point  (GnomeCanvasItem *item, double x, double y, int cx, int cy,
				        GnomeCanvasItem **actual_item);


static GnomeCanvasREClass *rect_parent_class;


/**
 * gnome_canvas_rect_get_type:
 * @void:
 *
 * Registers the &GnomeCanvasRect class if necesary, and returns the type ID
 * associated to it.
 *
 * Return value: The type ID of the &GnomeCanvasRect class.
 **/
GtkType
gnome_canvas_rect_get_type (void)
{
	static GtkType rect_type = 0;

	if (!rect_type) {
		static const GtkTypeInfo rect_info = {
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

/* Class initialization function for the rectangle item */
static void
gnome_canvas_rect_class_init (GnomeCanvasRectClass *class)
{
	GnomeCanvasItemClass *item_class;

	item_class = (GnomeCanvasItemClass *) class;

	rect_parent_class = gtk_type_class (gnome_canvas_re_get_type ());

	item_class->draw = gnome_canvas_rect_draw;
	item_class->point = gnome_canvas_rect_point;
	item_class->update = gnome_canvas_rect_update;
}

/* Update handler for the rectangle item */
static void
gnome_canvas_rect_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	GnomeCanvasRE *re;
	REPrivate *priv;

	re = GNOME_CANVAS_RE (item);
	priv = re->priv;

	if (GNOME_CANVAS_ITEM_CLASS (rect_parent_class)->update)
		(* GNOME_CANVAS_ITEM_CLASS (rect_parent_class)->update) (
			item, affine, clip_path, flags);

	/* GnomeCanvasRE::update() did handle everything for the non-AA case */
	if (!item->canvas->aa)
		return;
}

#if 0
static void
gnome_canvas_rect_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	GnomeCanvasRE *re;
	ArtVpath vpath[11];
	ArtVpath *vpath2;
	double x0, y0, x1, y1;
	double dx, dy;
	double halfwidth;
	int i;

#ifdef VERBOSE
	{
		char str[128];

		art_affine_to_string (str, affine);
		g_print ("gnome_canvas_rect_update item %x %s\n", item, str);

		if (item->xform) {
		  g_print ("xform = %g %g\n", item->xform[0], item->xform[1]);
		}
	}
#endif
	gnome_canvas_re_update_shared (item, affine, clip_path, flags);

	re = GNOME_CANVAS_RE (item);

	if (item->canvas->aa) {
		x0 = re->x1;
		y0 = re->y1;
		x1 = re->x2;
		y1 = re->y2;

		gnome_canvas_item_reset_bounds (item);

		if (re->fill_set) {
			vpath[0].code = ART_MOVETO;
			vpath[0].x = x0;
			vpath[0].y = y0;
			vpath[1].code = ART_LINETO;
			vpath[1].x = x0;
			vpath[1].y = y1;
			vpath[2].code = ART_LINETO;
			vpath[2].x = x1;
			vpath[2].y = y1;
			vpath[3].code = ART_LINETO;
			vpath[3].x = x1;
			vpath[3].y = y0;
			vpath[4].code = ART_LINETO;
			vpath[4].x = x0;
			vpath[4].y = y0;
			vpath[5].code = ART_END;
			vpath[5].x = 0;
			vpath[5].y = 0;

			vpath2 = art_vpath_affine_transform (vpath, affine);

			gnome_canvas_item_update_svp_clip (item, &re->fill_svp, art_svp_from_vpath (vpath2), clip_path);
			art_free (vpath2);
		} else
			gnome_canvas_item_update_svp (item, &re->fill_svp, NULL);

		if (re->outline_set) {
			/* We do the stroking by hand because it's simple enough
			   and could save time. */

			if (re->width_pixels)
				halfwidth = re->width * 0.5;
			else
				halfwidth = re->width * item->canvas->pixels_per_unit * 0.5;

			if (halfwidth < 0.25)
				halfwidth = 0.25;

			i = 0;
			vpath[i].code = ART_MOVETO;
			vpath[i].x = x0 - halfwidth;
			vpath[i].y = y0 - halfwidth;
			i++;
			vpath[i].code = ART_LINETO;
			vpath[i].x = x0 - halfwidth;
			vpath[i].y = y1 + halfwidth;
			i++;
			vpath[i].code = ART_LINETO;
			vpath[i].x = x1 + halfwidth;
			vpath[i].y = y1 + halfwidth;
			i++;
			vpath[i].code = ART_LINETO;
			vpath[i].x = x1 + halfwidth;
			vpath[i].y = y0 - halfwidth;
			i++;
			vpath[i].code = ART_LINETO;
			vpath[i].x = x0 - halfwidth;
			vpath[i].y = y0 - halfwidth;
			i++;

			if (x1 - halfwidth > x0 + halfwidth &&
			    y1 - halfwidth > y0 + halfwidth) {
				vpath[i].code = ART_MOVETO;
				vpath[i].x = x0 + halfwidth;
				vpath[i].y = y0 + halfwidth;
				i++;
				vpath[i].code = ART_LINETO;
				vpath[i].x = x1 - halfwidth;
				vpath[i].y = y0 + halfwidth;
				i++;
				vpath[i].code = ART_LINETO;
				vpath[i].x = x1 - halfwidth;
				vpath[i].y = y1 - halfwidth;
				i++;
				vpath[i].code = ART_LINETO;
				vpath[i].x = x0 + halfwidth;
				vpath[i].y = y1 - halfwidth;
				i++;
				vpath[i].code = ART_LINETO;
				vpath[i].x = x0 + halfwidth;
				vpath[i].y = y0 + halfwidth;
				i++;
			}
			vpath[i].code = ART_END;
			vpath[i].x = 0;
			vpath[i].y = 0;

			vpath2 = art_vpath_affine_transform (vpath, affine);

			gnome_canvas_item_update_svp_clip (item, &re->outline_svp, art_svp_from_vpath (vpath2), clip_path);
			art_free (vpath2);
		} else
			gnome_canvas_item_update_svp (item, &re->outline_svp, NULL);
	} else {
		/* xlib rendering - just update the bbox */
		get_bounds (re, &x0, &y0, &x1, &y1);
		gnome_canvas_update_bbox (item, x0, y0, x1, y1);
	}
}
#endif

/* Draw handler for the rectangle item */
static void
gnome_canvas_rect_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
			int x, int y, int width, int height)
{
	GnomeCanvasRE *re;
	REPrivate *priv;
	ArtPoint i1, i2, c1, c2;
	double i2c[6];
	int x1, y1, w, h;

	re = GNOME_CANVAS_RE (item);
	priv = re->priv;

	get_corners (re, &i1.x, &i1.y, &i2.x, &i2.y);

	gnome_canvas_item_i2c_affine (item, i2c);
	art_affine_point (&c1, &i1, i2c);
	art_affine_point (&c2, &i2, i2c);

	x1 = (int) floor (c1.x - x + 0.5);
	y1 = (int) floor (c1.y - y + 0.5);
	w = (int) floor (c2.x - c1.x + 0.5);
	h = (int) floor (c2.y - c1.y + 0.5);

	/* If we have stipples, set the offsets in the GCs and reset them later,
	 * because these GCs are supposed to be read-only.
	 */

	if (priv->fill_stipple)
		gnome_canvas_set_stipple_origin (item->canvas, priv->fill_gc);

	if (priv->outline_stipple)
		gnome_canvas_set_stipple_origin (item->canvas, priv->outline_gc);

	/* If the colors were specified in RGBA, threshold out the alpha to see
	 * if we should actually paint the fill or outline.
	 */

	if (priv->fill_set
	    && (priv->have_fill_pixel || (re->fill_color & 0xff) >= 128))
		gdk_draw_rectangle (drawable, priv->fill_gc, TRUE, x1, y1, w + 1, h + 1);

	if (priv->outline_set
	    && (priv->have_outline_pixel || (re->outline_color & 0xff) >= 128))
		gdk_draw_rectangle (drawable, priv->outline_gc, FALSE, x1, y1, w, h);

	/* Reset stipple offsets */

	if (priv->fill_stipple)
		gdk_gc_set_ts_origin (priv->fill_gc, 0, 0);

	if (priv->outline_stipple)
		gdk_gc_set_ts_origin (priv->outline_gc, 0, 0);
}

static double
gnome_canvas_rect_point (GnomeCanvasItem *item, double x, double y, int cx, int cy,
			 GnomeCanvasItem **actual_item)
{
	GnomeCanvasRE *re;
	REPrivate *priv;
	double x1, y1, x2, y2;
	double hwidth;
	double dx, dy;
	double tmp;

	re = GNOME_CANVAS_RE (item);
	priv = re->priv;

	*actual_item = item;

	/* Find the bounds for the rectangle plus its outline width */

	x1 = re->x1;
	y1 = re->y1;
	x2 = re->x2;
	y2 = re->y2;

	if (priv->outline_set) {
		if (priv->width_pixels)
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
		if (priv->fill_set || !priv->outline_set)
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

static void   gnome_canvas_ellipse_update (GnomeCanvasItem *item, double *affine,
					   ArtSVP *clip_path, int flags);
static void   gnome_canvas_ellipse_draw   (GnomeCanvasItem *item, GdkDrawable *drawable, int x, int y,
					   int width, int height);
static double gnome_canvas_ellipse_point  (GnomeCanvasItem *item, double x, double y, int cx, int cy,
					   GnomeCanvasItem **actual_item);


/**
 * gnome_canvas_ellipse_get_type:
 * @void:
 *
 * Registers the &GnomeCanvasEllipse class if necessary, and returns the type ID
 * associated to it.
 *
 * Return value: The type ID of the &GnomeCanvasEllipse class.
 **/
GtkType
gnome_canvas_ellipse_get_type (void)
{
	static GtkType ellipse_type = 0;

	if (!ellipse_type) {
		static const GtkTypeInfo ellipse_info = {
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

/* Class initialization function for the ellipse item */
static void
gnome_canvas_ellipse_class_init (GnomeCanvasEllipseClass *class)
{
	GnomeCanvasItemClass *item_class;

	item_class = (GnomeCanvasItemClass *) class;

	item_class->draw = gnome_canvas_ellipse_draw;
	item_class->point = gnome_canvas_ellipse_point;
	item_class->update = gnome_canvas_ellipse_update;
}

static void
gnome_canvas_ellipse_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
			   int x, int y, int width, int height)
{
	GnomeCanvasRE *re;
	REPrivate *priv;
	double i2w[6], w2c[6], i2c[6];
	int x1, y1, x2, y2;
	ArtPoint i1, i2;
	ArtPoint c1, c2;

	re = GNOME_CANVAS_RE (item);
	priv = re->priv;

	/* Get canvas pixel coordinates */

	gnome_canvas_item_i2w_affine (item, i2w);
	gnome_canvas_w2c_affine (item->canvas, w2c);
	art_affine_multiply (i2c, i2w, w2c);

	i1.x = re->x1;
	i1.y = re->y1;
	i2.x = re->x2;
	i2.y = re->y2;
	art_affine_point (&c1, &i1, i2c);
	art_affine_point (&c2, &i2, i2c);
	x1 = c1.x;
	y1 = c1.y;
	x2 = c2.x;
	y2 = c2.y;

	if (priv->fill_set) {
		if (priv->fill_stipple)
			gnome_canvas_set_stipple_origin (item->canvas, priv->fill_gc);

		gdk_draw_arc (drawable,
			      priv->fill_gc,
			      TRUE,
			      x1 - x,
			      y1 - y,
			      x2 - x1,
			      y2 - y1,
			      0 * 64,
			      360 * 64);
	}

	if (priv->outline_set) {
		if (priv->outline_stipple)
			gnome_canvas_set_stipple_origin (item->canvas, priv->outline_gc);

		gdk_draw_arc (drawable,
			      priv->outline_gc,
			      FALSE,
			      x1 - x,
			      y1 - y,
			      x2 - x1,
			      y2 - y1,
			      0 * 64,
			      360 * 64);
	}
}

static double
gnome_canvas_ellipse_point (GnomeCanvasItem *item, double x, double y, int cx, int cy,
			    GnomeCanvasItem **actual_item)
{
	GnomeCanvasRE *re;
	REPrivate *priv;
	double dx, dy;
	double scaled_dist;
	double outline_dist;
	double center_dist;
	double width;
	double a, b;
	double diamx, diamy;

	re = GNOME_CANVAS_RE (item);
	priv = re->priv;

	*actual_item = item;

	if (priv->outline_set) {
		if (priv->width_pixels)
			width = re->width / item->canvas->pixels_per_unit;
		else
			width = re->width;
	} else
		width = 0.0;

	/* Compute the distance between the center of the ellipse and the point, with the ellipse
	 * considered as being scaled to a circle.
	 */

	dx = x - (re->x1 + re->x2) / 2.0;
	dy = y - (re->y1 + re->y2) / 2.0;
	center_dist = sqrt (dx * dx + dy * dy);

	a = dx / ((re->x2 + width - re->x1) / 2.0);
	b = dy / ((re->y2 + width - re->y1) / 2.0);
	scaled_dist = sqrt (a * a + b * b);

	/* If the scaled distance is greater than 1, then we are outside.  Compute the distance from
	 * the point to the edge of the circle, then scale back to the original un-scaled coordinate
	 * system.
	 */

	if (scaled_dist > 1.0)
		return (center_dist / scaled_dist) * (scaled_dist - 1.0);

	/* We are inside the outer edge of the ellipse.  If it is filled, then we are "inside".
	 * Otherwise, do the same computation as above, but also check whether we are inside the
	 * outline.
	 */

	if (priv->fill_set)
		return 0.0;

	if (scaled_dist > GNOME_CANVAS_EPSILON)
		outline_dist = (center_dist / scaled_dist) * (1.0 - scaled_dist) - width;
	else {
		/* Handle very small distance */

		diamx = re->x2 - re->x1;
		diamy = re->y2 - re->y1;

		if (diamx < diamy)
			outline_dist = (diamx - width) / 2.0;
		else
			outline_dist = (diamy - width) / 2.0;
	}

	if (outline_dist < 0.0)
		return 0.0;

	return outline_dist;
}

#define N_PTS 126

static void
gnome_canvas_gen_ellipse (ArtVpath *vpath, double x0, double y0,
			  double x1, double y1)
{
	int i;

	for (i = 0; i < N_PTS + 1; i++) {
		double th;

		th = (2 * M_PI * i) / N_PTS;
		vpath[i].code = i == 0 ? ART_MOVETO : ART_LINETO;
		vpath[i].x = (x0 + x1) * 0.5 + (x1 - x0) * 0.5 * cos (th);
		vpath[i].y = (y0 + y1) * 0.5 - (y1 - y0) * 0.5 * sin (th);
	}
}

static void
gnome_canvas_ellipse_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
#if 0
	GnomeCanvasRE *re;
	ArtVpath vpath[N_PTS + N_PTS + 3];
	ArtVpath *vpath2;
	double x0, y0, x1, y1;
	int i;

#ifdef VERBOSE
	g_print ("gnome_canvas_rect_update item %x\n", item);
#endif

	gnome_canvas_re_update_shared (item, affine, clip_path, flags);
	re = GNOME_CANVAS_RE (item);

	if (item->canvas->aa) {

#ifdef VERBOSE
		{
			char str[128];

			art_affine_to_string (str, affine);
			g_print ("g_c_r_e affine = %s\n", str);
		}
		g_print ("item %x (%g, %g) - (%g, %g)\n",
			 item,
			 re->x1, re->y1, re->x2, re->y2);
#endif

		if (re->fill_set) {
			gnome_canvas_gen_ellipse (vpath, re->x1, re->y1, re->x2, re->y2);
			vpath[N_PTS + 1].code = ART_END;
			vpath[N_PTS + 1].x = 0;
			vpath[N_PTS + 1].y = 0;

			vpath2 = art_vpath_affine_transform (vpath, affine);

			gnome_canvas_item_update_svp_clip (item, &re->fill_svp, art_svp_from_vpath (vpath2), clip_path);
			art_free (vpath2);
		} else
			gnome_canvas_item_update_svp (item, &re->fill_svp, NULL);

		if (re->outline_set) {
			double halfwidth;

			if (re->width_pixels)
				halfwidth = (re->width / item->canvas->pixels_per_unit) * 0.5;
			else
				halfwidth = re->width * 0.5;

			if (halfwidth < 0.25)
				halfwidth = 0.25;

			i = 0;
			gnome_canvas_gen_ellipse (vpath + i,
						  re->x1 - halfwidth, re->y1 - halfwidth,
						  re->x2 + halfwidth, re->y2 + halfwidth);
			i = N_PTS + 1;
			if (re->x2 - halfwidth > re->x1 + halfwidth &&
			    re->y2 - halfwidth > re->y1 + halfwidth) {
				gnome_canvas_gen_ellipse (vpath + i,
							  re->x1 + halfwidth, re->y2 - halfwidth,
							  re->x2 - halfwidth, re->y1 + halfwidth);
				i += N_PTS + 1;
			}
			vpath[i].code = ART_END;
			vpath[i].x = 0;
			vpath[i].y = 0;

			vpath2 = art_vpath_affine_transform (vpath, affine);

			gnome_canvas_item_update_svp_clip (item, &re->outline_svp, art_svp_from_vpath (vpath2), clip_path);
			art_free (vpath2);
		} else
			gnome_canvas_item_update_svp (item, &re->outline_svp, NULL);
	} else {
		/* xlib rendering - just update the bbox */
		get_bounds (re, &x0, &y0, &x1, &y1);
		gnome_canvas_update_bbox (item, x0, y0, x1, y1);
	}
#endif
}
