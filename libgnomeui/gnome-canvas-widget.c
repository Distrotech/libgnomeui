/* Widget item type for GnomeCanvas widget
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
#include <gtk/gtksignal.h>
#include "gnome-canvas-widget.h"

enum {
	ARG_0,
	ARG_WIDGET,
	ARG_X,
	ARG_Y,
	ARG_WIDTH,
	ARG_HEIGHT,
	ARG_ANCHOR,
	ARG_SIZE_PIXELS
};


static void gnome_canvas_widget_class_init (GnomeCanvasWidgetClass *class);
static void gnome_canvas_widget_init       (GnomeCanvasWidget      *witem);
static void gnome_canvas_widget_destroy    (GtkObject              *object);
static void gnome_canvas_widget_set_arg    (GtkObject              *object,
					    GtkArg                 *arg,
					    guint                   arg_id);
static void gnome_canvas_widget_get_arg    (GtkObject              *object,
					    GtkArg                 *arg,
					    guint                   arg_id);

static void   gnome_canvas_widget_update      (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);
static double gnome_canvas_widget_point       (GnomeCanvasItem *item, double x, double y,
					       int cx, int cy, GnomeCanvasItem **actual_item);
static void   gnome_canvas_widget_translate   (GnomeCanvasItem *item, double dx, double dy);
static void   gnome_canvas_widget_bounds      (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2);

static void gnome_canvas_widget_render (GnomeCanvasItem *item,
					GnomeCanvasBuf *buf);
static void gnome_canvas_widget_draw (GnomeCanvasItem *item,
				      GdkDrawable *drawable,
				      int x, int y,
				      int width, int height);

static GnomeCanvasItemClass *parent_class;


GtkType
gnome_canvas_widget_get_type (void)
{
	static GtkType witem_type = 0;

	if (!witem_type) {
		GtkTypeInfo witem_info = {
			"GnomeCanvasWidget",
			sizeof (GnomeCanvasWidget),
			sizeof (GnomeCanvasWidgetClass),
			(GtkClassInitFunc) gnome_canvas_widget_class_init,
			(GtkObjectInitFunc) gnome_canvas_widget_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		witem_type = gtk_type_unique (gnome_canvas_item_get_type (), &witem_info);
	}

	return witem_type;
}

static void
gnome_canvas_widget_class_init (GnomeCanvasWidgetClass *class)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	gtk_object_add_arg_type ("GnomeCanvasWidget::widget", GTK_TYPE_OBJECT, GTK_ARG_READWRITE, ARG_WIDGET);
	gtk_object_add_arg_type ("GnomeCanvasWidget::x", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_X);
	gtk_object_add_arg_type ("GnomeCanvasWidget::y", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_Y);
	gtk_object_add_arg_type ("GnomeCanvasWidget::width", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_WIDTH);
	gtk_object_add_arg_type ("GnomeCanvasWidget::height", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_HEIGHT);
	gtk_object_add_arg_type ("GnomeCanvasWidget::anchor", GTK_TYPE_ANCHOR_TYPE, GTK_ARG_READWRITE, ARG_ANCHOR);
	gtk_object_add_arg_type ("GnomeCanvasWidget::size_pixels", GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_SIZE_PIXELS);

	object_class->destroy = gnome_canvas_widget_destroy;
	object_class->set_arg = gnome_canvas_widget_set_arg;
	object_class->get_arg = gnome_canvas_widget_get_arg;

	item_class->update = gnome_canvas_widget_update;
	item_class->point = gnome_canvas_widget_point;
	item_class->translate = gnome_canvas_widget_translate;
	item_class->bounds = gnome_canvas_widget_bounds;
	item_class->render = gnome_canvas_widget_render;
	item_class->draw = gnome_canvas_widget_draw;
}

static void
gnome_canvas_widget_init (GnomeCanvasWidget *witem)
{
	witem->x = 0.0;
	witem->y = 0.0;
	witem->width = 0.0;
	witem->height = 0.0;
	witem->anchor = GTK_ANCHOR_NW;
	witem->size_pixels = FALSE;
}

static void
gnome_canvas_widget_destroy (GtkObject *object)
{
	GnomeCanvasWidget *witem;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_WIDGET (object));

	witem = GNOME_CANVAS_WIDGET (object);

	if (witem->widget && !witem->in_destroy) {
		gtk_signal_disconnect (GTK_OBJECT (witem->widget), witem->destroy_id);
		gtk_widget_destroy (witem->widget);
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
recalc_bounds (GnomeCanvasWidget *witem)
{
	GnomeCanvasItem *item;
	double wx, wy;

	item = GNOME_CANVAS_ITEM (witem);

	/* Get world coordinates */

	wx = witem->x;
	wy = witem->y;
	gnome_canvas_item_i2w (item, &wx, &wy);

	/* Get canvas pixel coordinates */

	gnome_canvas_w2c (item->canvas, wx, wy, &witem->cx, &witem->cy);

	/* Anchor widget item */

	switch (witem->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		witem->cx -= witem->cwidth / 2;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		witem->cx -= witem->cwidth;
		break;
	}

	switch (witem->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		break;

	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		witem->cy -= witem->cheight / 2;
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		witem->cy -= witem->cheight;
		break;
	}

	/* Bounds */

	item->x1 = witem->cx;
	item->y1 = witem->cy;
	item->x2 = witem->cx + witem->cwidth;
	item->y2 = witem->cy + witem->cheight;

	gnome_canvas_group_child_bounds (GNOME_CANVAS_GROUP (item->parent), item);

	if (witem->widget)
		gtk_layout_move (GTK_LAYOUT (item->canvas), witem->widget,
				 witem->cx + item->canvas->zoom_xofs,
				 witem->cy + item->canvas->zoom_yofs);
}

static void
do_destroy (GtkObject *object, gpointer data)
{
	GnomeCanvasWidget *witem;

	witem = data;

	witem->in_destroy = TRUE;

	gtk_object_destroy (data);
}

static void
gnome_canvas_widget_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasItem *item;
	GnomeCanvasWidget *witem;
	GtkObject *obj;
	int update;
	int calc_bounds;

	item = GNOME_CANVAS_ITEM (object);
	witem = GNOME_CANVAS_WIDGET (object);

	update = FALSE;
	calc_bounds = FALSE;

	switch (arg_id) {
	case ARG_WIDGET:
		if (witem->widget) {
			gtk_signal_disconnect (GTK_OBJECT (witem->widget), witem->destroy_id);
			gtk_container_remove (GTK_CONTAINER (item->canvas), witem->widget);
		}

		obj = GTK_VALUE_OBJECT (*arg);
		if (obj) {
			witem->widget = GTK_WIDGET (obj);
			witem->destroy_id = gtk_signal_connect (obj, "destroy",
								(GtkSignalFunc) do_destroy,
								witem);
			gtk_layout_put (GTK_LAYOUT (item->canvas), witem->widget,
					witem->cx + item->canvas->zoom_xofs,
					witem->cy + item->canvas->zoom_yofs);
		}

		update = TRUE;
		break;

	case ARG_X:
	        if (witem->x != GTK_VALUE_DOUBLE (*arg))
		{
		        witem->x = GTK_VALUE_DOUBLE (*arg);
			calc_bounds = TRUE;
		}
		break;

	case ARG_Y:
	        if (witem->y != GTK_VALUE_DOUBLE (*arg))
		{
		        witem->y = GTK_VALUE_DOUBLE (*arg);
			calc_bounds = TRUE;
		}
		break;

	case ARG_WIDTH:
	        if (witem->width != fabs (GTK_VALUE_DOUBLE (*arg)))
		{
		        witem->width = fabs (GTK_VALUE_DOUBLE (*arg));
			update = TRUE;
		}
		break;

	case ARG_HEIGHT:
	        if (witem->height != fabs (GTK_VALUE_DOUBLE (*arg)))
		{
		        witem->height = fabs (GTK_VALUE_DOUBLE (*arg));
			update = TRUE;
		}
		break;

	case ARG_ANCHOR:
	        if (witem->anchor != GTK_VALUE_ENUM (*arg))
		{
		        witem->anchor = GTK_VALUE_ENUM (*arg);
			update = TRUE;
		}
		break;

	case ARG_SIZE_PIXELS:
	        if (witem->size_pixels != GTK_VALUE_BOOL (*arg))
		{
		        witem->size_pixels = GTK_VALUE_BOOL (*arg);
			update = TRUE;
		}
		break;

	default:
		break;
	}

	if (update)
		(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->update) (item, NULL, NULL, 0);

	if (calc_bounds)
		recalc_bounds (witem);
}

static void
gnome_canvas_widget_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasWidget *witem;

	witem = GNOME_CANVAS_WIDGET (object);

	switch (arg_id) {
	case ARG_WIDGET:
		GTK_VALUE_OBJECT (*arg) = GTK_OBJECT (witem->widget);
		break;

	case ARG_X:
		GTK_VALUE_DOUBLE (*arg) = witem->x;
		break;

	case ARG_Y:
		GTK_VALUE_DOUBLE (*arg) = witem->y;
		break;

	case ARG_WIDTH:
		GTK_VALUE_DOUBLE (*arg) = witem->width;
		break;

	case ARG_HEIGHT:
		GTK_VALUE_DOUBLE (*arg) = witem->height;
		break;

	case ARG_ANCHOR:
		GTK_VALUE_ENUM (*arg) = witem->anchor;
		break;

	case ARG_SIZE_PIXELS:
		GTK_VALUE_BOOL (*arg) = witem->size_pixels;
		break;

	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
}

static void
gnome_canvas_widget_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	GnomeCanvasWidget *witem;

	witem = GNOME_CANVAS_WIDGET (item);

	if (parent_class->update)
		(* parent_class->update) (item, affine, clip_path, flags);

	if (witem->widget) {
		if (witem->size_pixels) {
			witem->cwidth = (int) (witem->width + 0.5);
			witem->cheight = (int) (witem->height + 0.5);
		} else {
			witem->cwidth = (int) (witem->width * item->canvas->pixels_per_unit + 0.5);
			witem->cheight = (int) (witem->height * item->canvas->pixels_per_unit + 0.5);
		}

		gtk_widget_set_usize (witem->widget, witem->cwidth, witem->cheight);
	} else {
		witem->cwidth = 0.0;
		witem->cheight = 0.0;
	}

	recalc_bounds (witem);
}

static void
gnome_canvas_widget_render (GnomeCanvasItem *item,
			    GnomeCanvasBuf *buf)
{
	GnomeCanvasWidget *witem;

	witem = GNOME_CANVAS_WIDGET (item);

	if (witem->widget) 
		gtk_widget_queue_draw (witem->widget);
}

static void
gnome_canvas_widget_draw (GnomeCanvasItem *item,
			  GdkDrawable *drawable,
			  int x, int y,
			  int width, int height)
{
	GnomeCanvasWidget *witem;

	witem = GNOME_CANVAS_WIDGET (item);

	if (witem->widget)
		gtk_widget_queue_draw (witem->widget);
}

static double
gnome_canvas_widget_point (GnomeCanvasItem *item, double x, double y,
			   int cx, int cy, GnomeCanvasItem **actual_item)
{
	GnomeCanvasWidget *witem;
	double x1, y1, x2, y2;
	double dx, dy;

	witem = GNOME_CANVAS_WIDGET (item);

	*actual_item = item;

	gnome_canvas_c2w (item->canvas, witem->cx, witem->cy, &x1, &y1);

	x2 = x1 + (witem->cwidth - 1) / item->canvas->pixels_per_unit;
	y2 = y1 + (witem->cheight - 1) / item->canvas->pixels_per_unit;

	/* Is point inside widget bounds? */

	if ((x >= x1) && (y >= y1) && (x <= x2) && (y <= y2))
		return 0.0;

	/* Point is outside widget bounds */

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
gnome_canvas_widget_translate (GnomeCanvasItem *item, double dx, double dy)
{
	GnomeCanvasWidget *witem;

	witem = GNOME_CANVAS_WIDGET (item);

	witem->x = dx;
	witem->y = dy;

	recalc_bounds (witem);
}

static void
gnome_canvas_widget_bounds (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	GnomeCanvasWidget *witem;

	witem = GNOME_CANVAS_WIDGET (item);

	*x1 = witem->x;
	*y1 = witem->y;

	switch (witem->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		*x1 -= witem->width / 2.0;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		*x1 -= witem->width;
		break;
	}

	switch (witem->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		break;

	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		*y1 -= witem->height / 2.0;
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		*y1 -= witem->height;
		break;
	}

	*x2 = *x1 + witem->width;
	*y2 = *y1 + witem->height;
}
