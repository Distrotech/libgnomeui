/* Text item type for GnomeCanvas widget
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
#include "gnome-canvas-text.h"


enum {
	ARG_0,
	ARG_TEXT,
	ARG_X,
	ARG_Y,
	ARG_FONT,
	ARG_ANCHOR,
	ARG_JUSTIFICATION,
	ARG_FILL_COLOR
};


static void gnome_canvas_text_class_init (GnomeCanvasTextClass *class);
static void gnome_canvas_text_init       (GnomeCanvasText      *text);
static void gnome_canvas_text_destroy    (GtkObject            *object);
static void gnome_canvas_text_set_arg    (GtkObject            *object,
					  GtkArg               *arg,
					  guint                 arg_id);
static void gnome_canvas_text_get_arg    (GtkObject            *object,
					  GtkArg               *arg,
					  guint                 arg_id);

static void   gnome_canvas_text_reconfigure (GnomeCanvasItem *item);
static void   gnome_canvas_text_realize     (GnomeCanvasItem *item);
static void   gnome_canvas_text_unrealize   (GnomeCanvasItem *item);
static void   gnome_canvas_text_draw        (GnomeCanvasItem *item, GdkDrawable *drawable,
					     int x, int y, int width, int height);
static double gnome_canvas_text_point       (GnomeCanvasItem *item, double x, double y,
					     int cx, int cy, GnomeCanvasItem **actual_item);
static void   gnome_canvas_text_translate   (GnomeCanvasItem *item, double dx, double dy);


static GnomeCanvasItemClass *parent_class;


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

static void
gnome_canvas_text_class_init (GnomeCanvasTextClass *class)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	gtk_object_add_arg_type ("GnomeCanvasText::text", GTK_TYPE_STRING, GTK_ARG_WRITABLE, ARG_TEXT);
	gtk_object_add_arg_type ("GnomeCanvasText::x", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_X);
	gtk_object_add_arg_type ("GnomeCanvasText::y", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_Y);
	gtk_object_add_arg_type ("GnomeCanvasText::font", GTK_TYPE_STRING, GTK_ARG_WRITABLE, ARG_FONT);
	gtk_object_add_arg_type ("GnomeCanvasText::anchor", GTK_TYPE_ANCHOR_TYPE, GTK_ARG_WRITABLE, ARG_ANCHOR);
	gtk_object_add_arg_type ("GnomeCanvasText::justification", GTK_TYPE_JUSTIFICATION, GTK_ARG_WRITABLE, ARG_JUSTIFICATION);
	gtk_object_add_arg_type ("GnomeCanvasText::fill_color", GTK_TYPE_STRING, GTK_ARG_WRITABLE, ARG_FILL_COLOR);

	object_class->destroy = gnome_canvas_text_destroy;
	object_class->set_arg = gnome_canvas_text_set_arg;
	object_class->get_arg = gnome_canvas_text_get_arg;

	item_class->reconfigure = gnome_canvas_text_reconfigure;
	item_class->realize = gnome_canvas_text_realize;
	item_class->unrealize = gnome_canvas_text_unrealize;
	item_class->draw = gnome_canvas_text_draw;
	item_class->point = gnome_canvas_text_point;
	item_class->translate = gnome_canvas_text_translate;
}

static void
gnome_canvas_text_init (GnomeCanvasText *text)
{
	text->x = 0.0;
	text->y = 0.0;
	text->font = gdk_font_load ("fixed"); /* lame default */
	g_assert (text->font != NULL);
	text->anchor = GTK_ANCHOR_CENTER;
	text->justification = GTK_JUSTIFY_LEFT;
}

static void
gnome_canvas_text_destroy (GtkObject *object)
{
	GnomeCanvasText *text;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_TEXT (object));

	text = GNOME_CANVAS_TEXT (object);

	if (text->text)
		g_free (text->text);

	gdk_font_unref (text->font);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
recalc_bounds (GnomeCanvasText *text)
{
	GnomeCanvasItem *item;
	double wx, wy;

	item = GNOME_CANVAS_ITEM (text);

	/* Get world coordinates */

	wx = text->x;
	wy = text->y;
	gnome_canvas_item_i2w (item, &wx, &wy);

	/* Get canvas pixel coordinates */

	gnome_canvas_w2c (item->canvas, wx, wy, &text->cx, &text->cy);

	/* Calculate text dimensions */

	text->width = text->text ? gdk_string_width (text->font, text->text) : 0;
	text->height = text->font->ascent + text->font->descent;

	/* Anchor text */

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		text->cx -= text->width / 2;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		text->cx -= text->width;
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
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		text->cy -= text->height;
		break;
	}

	/* Bounds */

	item->x1 = text->cx;
	item->y1 = text->cy;
	item->x2 = text->cx + text->width;
	item->y2 = text->cy + text->height;

	gnome_canvas_group_child_bounds (GNOME_CANVAS_GROUP (item->parent), item);
}

static void
gnome_canvas_text_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasItem *item;
	GnomeCanvasText *text;
	int calc_bounds;
	int calc_gcs;
	GdkColor color;

	item = GNOME_CANVAS_ITEM (object);
	text = GNOME_CANVAS_TEXT (object);

	calc_bounds = FALSE;
	calc_gcs = FALSE;

	switch (arg_id) {
	case ARG_TEXT:
		if (text->text)
			g_free (text->text);

		text->text = g_strdup (GTK_VALUE_STRING (*arg));
		calc_bounds = TRUE;
		break;

	case ARG_X:
		text->x = GTK_VALUE_DOUBLE (*arg);
		calc_bounds = TRUE;
		break;

	case ARG_Y:
		text->y = GTK_VALUE_DOUBLE (*arg);
		calc_bounds = TRUE;
		break;

	case ARG_FONT:
		if (text->font)
			gdk_font_unref (text->font);

		text->font = gdk_font_load (GTK_VALUE_STRING (*arg));
		if (!text->font) {
			text->font = gdk_font_load ("fixed");
			g_assert (text->font != NULL);
		}

		calc_bounds = TRUE;
		break;

	case ARG_ANCHOR:
		text->anchor = GTK_VALUE_ENUM (*arg);
		calc_bounds = TRUE;
		break;

	case ARG_JUSTIFICATION:
		text->justification = GTK_VALUE_ENUM (*arg);
		break;

	case ARG_FILL_COLOR:
		if (gnome_canvas_get_color (item->canvas, GTK_VALUE_STRING (*arg), &color))
			text->pixel = color.pixel;
		else
			text->pixel = 0;

		calc_gcs = TRUE;
		break;

	default:
		break;
	}

	if (calc_gcs)
		(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->reconfigure) (item);

	if (calc_bounds)
		recalc_bounds (text);
}

static void
gnome_canvas_text_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasText *text;

	text = GNOME_CANVAS_TEXT (object);

	switch (arg_id) {
	case ARG_TEXT:
		GTK_VALUE_STRING (*arg) = g_strdup (text->text);
		break;

	case ARG_X:
		GTK_VALUE_DOUBLE (*arg) = text->x;
		break;

	case ARG_Y:
		GTK_VALUE_DOUBLE (*arg) = text->y;
		break;

	case ARG_ANCHOR:
		GTK_VALUE_ENUM (*arg) = text->anchor;
		break;

	case ARG_JUSTIFICATION:
		GTK_VALUE_ENUM (*arg) = text->justification;
		break;

	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
}

static void
gnome_canvas_text_reconfigure (GnomeCanvasItem *item)
{
	GnomeCanvasText *text;
	GdkColor color;

	text = GNOME_CANVAS_TEXT (item);

	if (text->gc) {
		color.pixel = text->pixel;
		gdk_gc_set_foreground (text->gc, &color);
	}

	recalc_bounds (text);
}

static void
gnome_canvas_text_realize (GnomeCanvasItem *item)
{
	GnomeCanvasText *text;

	text = GNOME_CANVAS_TEXT (item);

	text->gc = gdk_gc_new (GTK_WIDGET (item->canvas)->window);
	(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->reconfigure) (item);
}

static void
gnome_canvas_text_unrealize (GnomeCanvasItem *item)
{
	GnomeCanvasText *text;

	text = GNOME_CANVAS_TEXT (item);

	gdk_gc_unref (text->gc);
}

static void
gnome_canvas_text_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
			int x, int y, int width, int height)
{
	GnomeCanvasText *text;

	text = GNOME_CANVAS_TEXT (item);

	if (text->text)
		gdk_draw_string (drawable,
				 text->font,
				 text->gc,
				 text->cx - x,
				 text->cy - y + text->font->ascent,
				 text->text);
}

static double
gnome_canvas_text_point (GnomeCanvasItem *item, double x, double y,
			 int cx, int cy, GnomeCanvasItem **actual_item)
{
	GnomeCanvasText *text;
	double w, h;
	double x1, y1, x2, y2;
	double dx, dy;

	text = GNOME_CANVAS_TEXT (item);

	*actual_item = item;

	w = text->width / item->canvas->pixels_per_unit;
	h = text->height / item->canvas->pixels_per_unit;

	x1 = text->x;
	y1 = text->y;

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		x1 -= w / 2.0;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		x1 -= w;
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
		y1 -= h / 2.0;
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		y1 -= h;
		break;
	}

	x2 = x1 + w;
	y2 = y1 + h;

	/* Is point inside text bounds? */

	if ((x >= x1) && (y >= y1) && (x <= x2) && (y <= y2))
		return 0.0;

	/* Point is outside text bounds */

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

static void
gnome_canvas_text_translate (GnomeCanvasItem *item, double dx, double dy)
{
	GnomeCanvasText *text;

	text = GNOME_CANVAS_TEXT (item);

	text->x += dx;
	text->y += dy;

	recalc_bounds (text);
}
