/* GnomeCanvas widget - Tk-like canvas widget for Gnome
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
#include <stdarg.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include "gnome-canvas.h"


extern GtkArg* gtk_object_collect_args (guint	*nargs,
					GtkType (*) (const gchar*),
					va_list	 args1,
					va_list	 args2);


/*** GnomeCanvasItem ***/


enum {
	ITEM_EVENT,
	ITEM_LAST_SIGNAL
};


typedef gint (* GnomeCanvasItemSignal1) (GtkObject *item,
					 gpointer   arg1,
					 gpointer   data);

static void gnome_canvas_item_marshal_signal_1 (GtkObject     *object,
						GtkSignalFunc  func,
						gpointer       func_data,
						GtkArg        *args);

static void gnome_canvas_item_class_init (GnomeCanvasItemClass *class);
static void gnome_canvas_item_shutdown   (GtkObject            *object);


static guint item_signals[ITEM_LAST_SIGNAL] = { 0 };

static GtkObjectClass *item_parent_class;


GtkType
gnome_canvas_item_get_type (void)
{
	static GtkType canvas_item_type = 0;

	if (!canvas_item_type) {
		GtkTypeInfo canvas_item_info = {
			"GnomeCanvasItem",
			sizeof (GnomeCanvasItem),
			sizeof (GnomeCanvasItemClass),
			(GtkClassInitFunc) gnome_canvas_item_class_init,
			(GtkObjectInitFunc) NULL,
			NULL, /* reserved_1 */
			NULL, /* reserved_1 */
			(GtkClassInitFunc) NULL
		};

		canvas_item_type = gtk_type_unique (gtk_object_get_type (), &canvas_item_info);
	}

	return canvas_item_type;
}

static void
gnome_canvas_item_class_init (GnomeCanvasItemClass *class)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) class;

	item_parent_class = gtk_type_class (gtk_object_get_type ());

	item_signals[ITEM_EVENT] =
		gtk_signal_new ("event",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeCanvasItemClass, event),
				gnome_canvas_item_marshal_signal_1,
				GTK_TYPE_BOOL, 1,
				GTK_TYPE_GDK_EVENT);

	gtk_object_class_add_signals (object_class, item_signals, ITEM_LAST_SIGNAL);

	object_class->shutdown = gnome_canvas_item_shutdown;

	class->reconfigure = NULL;
	class->realize = NULL;
	class->unrealize = NULL;
	class->map = NULL;
	class->unmap = NULL;
	class->draw = NULL;
	class->translate = NULL;
	class->event = NULL;
}

static void
item_post_create_setup (GnomeCanvasItem *item)
{
	GtkObject *obj;

	obj = GTK_OBJECT (item);

	gtk_object_ref (obj);
	gtk_object_sink (obj);

	gnome_canvas_group_add (GNOME_CANVAS_GROUP (item->parent), item);

	if (GTK_WIDGET_REALIZED (item->canvas) && GNOME_CANVAS_ITEM_CLASS (obj->klass)->realize)
		(* GNOME_CANVAS_ITEM_CLASS (obj->klass)->realize) (item);

	if (GTK_WIDGET_MAPPED (item->canvas) && GNOME_CANVAS_ITEM_CLASS (obj->klass)->map)
		(* GNOME_CANVAS_ITEM_CLASS (obj->klass)->map) (item);

	gnome_canvas_group_child_bounds (GNOME_CANVAS_GROUP (item->parent), item);
	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	item->canvas->need_repick = TRUE;
}

GnomeCanvasItem *
gnome_canvas_item_new (GnomeCanvas *canvas, GnomeCanvasGroup *parent, GtkType type, ...)
{
	GtkObject *obj;
	va_list args1, args2;
	guint nargs;
	GtkArg *args;
	GnomeCanvasItem *item;

	g_return_val_if_fail (canvas != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS (canvas), NULL);
	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS_GROUP (parent), NULL);
	g_return_val_if_fail (gtk_type_is_a (type, gnome_canvas_item_get_type ()), NULL);

	obj = gtk_type_new (type);

	item = GNOME_CANVAS_ITEM (obj);
	item->canvas = canvas;
	item->parent = GNOME_CANVAS_ITEM (parent);

	va_start (args1, type);
	va_start (args2, type);

	args = gtk_object_collect_args (&nargs, gtk_object_get_arg_type, args1, args2);
	gtk_object_setv (obj, nargs, args);
	g_free (args);

	va_end (args1);
	va_end (args2);

	item_post_create_setup (item);

	return item;
}

GnomeCanvasItem *
gnome_canvas_item_newv (GnomeCanvas *canvas, GnomeCanvasGroup *parent, GtkType type, guint nargs, GtkArg *args)
{
	GtkObject *obj;
	GnomeCanvasItem *item;

	g_return_val_if_fail (canvas != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS (canvas), NULL);
	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS_GROUP (parent), NULL);
	g_return_val_if_fail (gtk_type_is_a (type, gnome_canvas_item_get_type ()), NULL);

	obj = gtk_type_new (type);

	item = GNOME_CANVAS_ITEM (obj);
	item->canvas = canvas;
	item->parent = GNOME_CANVAS_ITEM (parent);

	gtk_object_setv (obj, nargs, args);

	item_post_create_setup (item);

	return item;
}

static void
gnome_canvas_item_shutdown (GtkObject *object)
{
	GnomeCanvasItem *item;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (object));

	item = GNOME_CANVAS_ITEM (object);

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	item->canvas->need_repick = TRUE;

	if (item->parent) {
		gnome_canvas_group_remove (GNOME_CANVAS_GROUP (item->parent), item);
		gtk_object_unref (object);
	}

	if (GTK_WIDGET_MAPPED (item->canvas) && GNOME_CANVAS_ITEM_CLASS (item->object.klass)->unmap)
		(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->unmap) (item);

	if (GTK_WIDGET_REALIZED (item->canvas) && GNOME_CANVAS_ITEM_CLASS (item->object.klass)->unrealize)
		(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->unrealize) (item);

	if (GTK_OBJECT_CLASS (item_parent_class)->shutdown)
		(* GTK_OBJECT_CLASS (item_parent_class)->shutdown) (object);
}

static void
gnome_canvas_item_marshal_signal_1 (GtkObject *object, GtkSignalFunc func, gpointer func_data, GtkArg *args)
{
	GnomeCanvasItemSignal1 rfunc;
	gint *return_val;

	rfunc = (GnomeCanvasItemSignal1) func;
	return_val = GTK_RETLOC_BOOL (args[1]);

	*return_val = (* rfunc) (object,
				 GTK_VALUE_BOXED (args[0]),
				 func_data);
}

void
gnome_canvas_item_set (GnomeCanvasItem *item, ...)
{
	va_list args1;
	va_list args2;
	guint nargs;
	GtkArg *args;

	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));

	va_start (args1, item);
	va_start (args2, item);

	args = gtk_object_collect_args (&nargs, gtk_object_get_arg_type, args1, args2);

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	gtk_object_setv (GTK_OBJECT (item), nargs, args);
	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	item->canvas->need_repick = TRUE;

	g_free (args);

	va_end (args1);
	va_end (args2);
}

void
gnome_canvas_item_setv (GnomeCanvasItem *item, guint nargs, GtkArg *args)
{
	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	gtk_object_setv (GTK_OBJECT (item), nargs, args);
	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	item->canvas->need_repick = TRUE;
}

void
gnome_canvas_item_move (GnomeCanvasItem *item, double dx, double dy)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));

	if (!GNOME_CANVAS_ITEM_CLASS (item->object.klass)->translate) {
		g_warning ("Item type %s does not implement translate method.\n",
			   gtk_type_name (GTK_OBJECT_TYPE (item)));
		return;
	}

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->translate) (item, dx, dy);
	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	item->canvas->need_repick = TRUE;
}

void
gnome_canvas_item_w2i (GnomeCanvasItem *item, double *x, double *y)
{
	GnomeCanvasGroup *group;

	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));

	while (item->parent) {
		group = GNOME_CANVAS_GROUP (item->parent);

		if (x)
			*x -= group->xpos;

		if (y)
			*y -= group->ypos;

		item = item->parent;
	}
}

void
gnome_canvas_item_i2w (GnomeCanvasItem *item, double *x, double *y)
{
	GnomeCanvasGroup *group;

	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));

	while (item->parent) {
		group = GNOME_CANVAS_GROUP (item->parent);

		if (x)
			*x += group->xpos;

		if (y)
			*y += group->ypos;

		item = item->parent;
	}
}


/*** GnomeCanvasGroup ***/


enum {
	GROUP_ARG_0,
	GROUP_ARG_X,
	GROUP_ARG_Y
};


static void gnome_canvas_group_class_init  (GnomeCanvasGroupClass *class);
static void gnome_canvas_group_init        (GnomeCanvasGroup      *group);
static void gnome_canvas_group_set_arg     (GtkObject             *object,
					    GtkArg                *arg,
					    guint                  arg_id);
static void gnome_canvas_group_destroy     (GtkObject             *object);

static void   gnome_canvas_group_reconfigure (GnomeCanvasItem *item);
static void   gnome_canvas_group_realize     (GnomeCanvasItem *item);
static void   gnome_canvas_group_unrealize   (GnomeCanvasItem *item);
static void   gnome_canvas_group_map         (GnomeCanvasItem *item);
static void   gnome_canvas_group_unmap       (GnomeCanvasItem *item);
static void   gnome_canvas_group_draw        (GnomeCanvasItem *item, GdkDrawable *drawable,
					      int x, int y, int width, int height);
static double gnome_canvas_group_point       (GnomeCanvasItem *item, double x, double y, int cx, int cy,
					      GnomeCanvasItem **actual_item);
static void   gnome_canvas_group_translate   (GnomeCanvasItem *item, double dx, double dy);


static GnomeCanvasItemClass *group_parent_class;


GtkType
gnome_canvas_group_get_type (void)
{
	static GtkType group_type = 0;

	if (!group_type) {
		GtkTypeInfo group_info = {
			"GnomeCanvasGroup",
			sizeof (GnomeCanvasGroup),
			sizeof (GnomeCanvasGroupClass),
			(GtkClassInitFunc) gnome_canvas_group_class_init,
			(GtkObjectInitFunc) gnome_canvas_group_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		group_type = gtk_type_unique (gnome_canvas_item_get_type (), &group_info);
	}

	return group_type;
}

static void
gnome_canvas_group_class_init (GnomeCanvasGroupClass *class)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	group_parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	gtk_object_add_arg_type ("GnomeCanvasGroup::x", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, GROUP_ARG_X);
	gtk_object_add_arg_type ("GnomeCanvasGroup::y", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, GROUP_ARG_Y);

	object_class->set_arg = gnome_canvas_group_set_arg;
	object_class->destroy = gnome_canvas_group_destroy;

	item_class->reconfigure = gnome_canvas_group_reconfigure;
	item_class->realize = gnome_canvas_group_realize;
	item_class->unrealize = gnome_canvas_group_unrealize;
	item_class->map = gnome_canvas_group_map;
	item_class->unmap = gnome_canvas_group_unmap;
	item_class->draw = gnome_canvas_group_draw;
	item_class->point = gnome_canvas_group_point;
	item_class->translate = gnome_canvas_group_translate;
}

static void
gnome_canvas_group_init (GnomeCanvasGroup *group)
{
	group->xpos = 0.0;
	group->ypos = 0.0;
}

static void
gnome_canvas_group_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasItem *item;
	GnomeCanvasGroup *group;
	int recalc;

	item = GNOME_CANVAS_ITEM (object);
	group = GNOME_CANVAS_GROUP (object);

	recalc = FALSE;

	switch (arg_id) {
	case GROUP_ARG_X:
		group->xpos = GTK_VALUE_DOUBLE (*arg);
		recalc = TRUE;
		break;

	case GROUP_ARG_Y:
		group->ypos = GTK_VALUE_DOUBLE (*arg);
		recalc = TRUE;
		break;

	default:
		break;
	}

	if (recalc) {
		gnome_canvas_group_child_bounds (group, NULL);

		if (item->parent)
			gnome_canvas_group_child_bounds (GNOME_CANVAS_GROUP (item->parent), item);
	}
}

GnomeCanvasItem *
gnome_canvas_group_new (GnomeCanvas *canvas)
{
	GnomeCanvasItem *item;

	g_return_val_if_fail (canvas != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS (canvas), NULL);

	item = GNOME_CANVAS_ITEM (gtk_type_new (gnome_canvas_group_get_type ()));
	item->canvas = canvas;

	return item;
}

static void
gnome_canvas_group_destroy (GtkObject *object)
{
	GnomeCanvasGroup *group;
	GList *list;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_GROUP (object));

	group = GNOME_CANVAS_GROUP (object);

	for (list = group->item_list; list; list = list->next)
		gtk_object_destroy (list->data);

	if (GTK_OBJECT_CLASS (group_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (group_parent_class)->destroy) (object);
}

static void
gnome_canvas_group_reconfigure (GnomeCanvasItem *item)
{
	GnomeCanvasGroup *group;
	GList *list;
	GnomeCanvasItem *i;

	group = GNOME_CANVAS_GROUP (item);

	for (list = group->item_list; list; list = list->next) {
		i = list->data;

		if (GNOME_CANVAS_ITEM_CLASS (i->object.klass)->reconfigure)
			(* GNOME_CANVAS_ITEM_CLASS (i->object.klass)->reconfigure) (i);
	}
}

static void
gnome_canvas_group_realize (GnomeCanvasItem *item)
{
	GnomeCanvasGroup *group;
	GList *list;
	GnomeCanvasItem *i;

	group = GNOME_CANVAS_GROUP (item);

	for (list = group->item_list; list; list = list->next) {
		i = list->data;

		if (GNOME_CANVAS_ITEM_CLASS (i->object.klass)->realize)
			(* GNOME_CANVAS_ITEM_CLASS (i->object.klass)->realize) (i);
	}
}

static void
gnome_canvas_group_unrealize (GnomeCanvasItem *item)
{
	GnomeCanvasGroup *group;
	GList *list;
	GnomeCanvasItem *i;

	group = GNOME_CANVAS_GROUP (item);

	for (list = group->item_list; list; list = list->next) {
		i = list->data;

		if (GNOME_CANVAS_ITEM_CLASS (i->object.klass)->unrealize)
			(* GNOME_CANVAS_ITEM_CLASS (i->object.klass)->unrealize) (i);
	}
}

static void
gnome_canvas_group_map (GnomeCanvasItem *item)
{
	GnomeCanvasGroup *group;
	GList *list;
	GnomeCanvasItem *i;

	group = GNOME_CANVAS_GROUP (item);

	for (list = group->item_list; list; list = list->next) {
		i = list->data;

		if (GNOME_CANVAS_ITEM_CLASS (i->object.klass)->map)
			(* GNOME_CANVAS_ITEM_CLASS (i->object.klass)->unmap) (i);
	}
}

static void
gnome_canvas_group_unmap (GnomeCanvasItem *item)
{
	GnomeCanvasGroup *group;
	GList *list;
	GnomeCanvasItem *i;

	group = GNOME_CANVAS_GROUP (item);

	for (list = group->item_list; list; list = list->next) {
		i = list->data;

		if (GNOME_CANVAS_ITEM_CLASS (i->object.klass)->unmap)
			(* GNOME_CANVAS_ITEM_CLASS (i->object.klass)->unmap) (i);
	}
}

static void
gnome_canvas_group_draw (GnomeCanvasItem *item, GdkDrawable *drawable, int x, int y, int width, int height)
{
	GnomeCanvasGroup *group;
	GList *list;
	GnomeCanvasItem *i;

	group = GNOME_CANVAS_GROUP (item);

	for (list = group->item_list; list; list = list->next) {
		i = list->data;

		if (((item->x1 < (x + width))
		     && (item->y1 < (y + height))
		     && (item->x2 > x)
		     && (item->y2 > y))
		    || ((GTK_OBJECT_FLAGS (item) & GNOME_CANVAS_ITEM_ALWAYS_REDRAW)
			&& (item->x1 < item->canvas->redraw_x2)
			&& (item->y1 < item->canvas->redraw_y2)
			&& (item->x2 > item->canvas->redraw_x1)
			&& (item->y2 > item->canvas->redraw_y2)))
			if (GNOME_CANVAS_ITEM_CLASS (i->object.klass)->draw)
				(* GNOME_CANVAS_ITEM_CLASS (i->object.klass)->draw) (i, drawable, x, y, width, height);
	}
}

static double
gnome_canvas_group_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item)
{
	GnomeCanvasGroup *group;
	GList *list;
	GnomeCanvasItem *child, *point_item;
	int x1, y1, x2, y2;
	double gx, gy;
	double dist, best;
	int has_point;

	group = GNOME_CANVAS_GROUP (item);

	x1 = cx - item->canvas->close_enough;
	y1 = cy - item->canvas->close_enough;
	x2 = cx + item->canvas->close_enough;
	y2 = cy + item->canvas->close_enough;

	best = 0.0;
	*actual_item = NULL;

	gx = x - group->xpos;
	gy = y - group->ypos;

	for (list = group->item_list; list; list = list->next) {
		child = list->data;

		if ((child->x1 > x2) || (child->y1 > y2) || (child->x2 < x1) || (child->y2 < y1))
			continue;

		point_item = NULL; /* cater for incomplete item implementations */

		if (GNOME_CANVAS_ITEM_CLASS (child->object.klass)->point) {
			dist = (* GNOME_CANVAS_ITEM_CLASS (child->object.klass)->point) (child, gx, gy, cx, cy,
											 &point_item);
			has_point = TRUE;
		} else
			has_point = FALSE;

		if (has_point
		    && point_item
		    && ((int) (dist * item->canvas->pixels_per_unit + 0.5) <= item->canvas->close_enough)) {
			best = dist;
			*actual_item = point_item;
		}
	}

	return best;
}

static void
gnome_canvas_group_translate (GnomeCanvasItem *item, double dx, double dy)
{
	GnomeCanvasGroup *group;

	group = GNOME_CANVAS_GROUP (item);

	group->xpos += dx;
	group->ypos += dy;

	gnome_canvas_group_child_bounds (group, NULL);

	if (item->parent)
		gnome_canvas_group_child_bounds (GNOME_CANVAS_GROUP (item->parent), item);
}

void
gnome_canvas_group_add (GnomeCanvasGroup *group, GnomeCanvasItem *item)
{
	g_return_if_fail (group != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_GROUP (group));
	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));

	/* FIXME: should we reference the item? */

	if (!group->item_list) {
		group->item_list = g_list_append (group->item_list, item);
		group->item_list_end = group->item_list;
	} else
		group->item_list_end = g_list_append (group->item_list_end, item)->next;

	gnome_canvas_group_child_bounds (group, item);
}

void
gnome_canvas_group_remove (GnomeCanvasGroup *group, GnomeCanvasItem *item)
{
	GList *list_item;

	g_return_if_fail (group != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_GROUP (group));
	g_return_if_fail (item != NULL);

	list_item = g_list_find (group->item_list, item);
	if (!list_item)
		return;

	if (!list_item->next)
		group->item_list_end = list_item->prev;

	group->item_list = g_list_remove_link (group->item_list, list_item);
}

void
gnome_canvas_group_child_bounds (GnomeCanvasGroup *group, GnomeCanvasItem *item)
{
	GnomeCanvasItem *gitem;
	GList *list;
	int first;

	g_return_if_fail (group != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_GROUP (group));
	g_return_if_fail (!item || GNOME_IS_CANVAS_ITEM (item));

	gitem = GNOME_CANVAS_ITEM (group);

	if (item) {
		/* Just add the child's bounds to whatever we have now */

		if ((item->x1 == item->x2) || (item->y1 == item->y2))
			return; /* empty bounding box */

		if (item->x1 < gitem->x1)
			gitem->x1 = item->x1;

		if (item->y1 < gitem->y1)
			gitem->y1 = item->y1;

		if (item->x2 > gitem->x2)
			gitem->x2 = item->x2;

		if (item->y2 > gitem->y2)
			gitem->y2 = item->y2;

		/* Propagate upwards */

		if (gitem->parent)
			gnome_canvas_group_child_bounds (GNOME_CANVAS_GROUP (gitem->parent), gitem);
	} else {
		/* Rebuild every sub-group's bounds and reconstruct our own bounds */

		for (list = group->item_list, first = TRUE; list; list = list->next, first = FALSE) {
			item = list->data;

			if (GNOME_IS_CANVAS_GROUP (item))
				gnome_canvas_group_child_bounds (GNOME_CANVAS_GROUP (item), NULL);
			else if (GNOME_CANVAS_ITEM_CLASS (item->object.klass)->reconfigure)
					(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->reconfigure) (item);

			if (first) {
				gitem->x1 = item->x1;
				gitem->y1 = item->y1;
				gitem->x2 = item->x2;
				gitem->y2 = item->y2;
			} else {
				if (item->x1 < gitem->x1)
					gitem->x1 = item->x1;

				if (item->y1 < gitem->y1)
					gitem->y1 = item->y1;

				if (item->x2 > gitem->x2)
					gitem->x2 = item->x2;

				if (item->y2 > gitem->y2)
					gitem->y2 = item->y2;
			}
		}
	}
}


/*** GnomeCanvas ***/


static void gnome_canvas_class_init     (GnomeCanvasClass *class);
static void gnome_canvas_init           (GnomeCanvas      *canvas);
static void gnome_canvas_destroy        (GtkObject        *object);
static void gnome_canvas_map            (GtkWidget        *widget);
static void gnome_canvas_unmap          (GtkWidget        *widget);
static void gnome_canvas_realize        (GtkWidget        *widget);
static void gnome_canvas_unrealize      (GtkWidget        *widget);
static void gnome_canvas_draw           (GtkWidget        *widget,
					 GdkRectangle     *area);
static void gnome_canvas_size_request   (GtkWidget        *widget,
					 GtkRequisition   *requisition);
static void gnome_canvas_size_allocate  (GtkWidget        *widget,
					 GtkAllocation    *allocation);
static gint gnome_canvas_button         (GtkWidget        *widget,
					 GdkEventButton   *event);
static gint gnome_canvas_motion         (GtkWidget        *widget,
					 GdkEventMotion   *event);
static gint gnome_canvas_expose         (GtkWidget        *widget,
					 GdkEventExpose   *event);
static gint gnome_canvas_key            (GtkWidget        *widget,
					 GdkEventKey      *event);
static gint gnome_canvas_crossing       (GtkWidget        *widget,
					 GdkEventCrossing *event);


static GtkContainerClass *canvas_parent_class;


GtkType
gnome_canvas_get_type (void)
{
	static GtkType canvas_type = 0;

	if (!canvas_type) {
		GtkTypeInfo canvas_info = {
			"GnomeCanvas",
			sizeof (GnomeCanvas),
			sizeof (GnomeCanvasClass),
			(GtkClassInitFunc) gnome_canvas_class_init,
			(GtkObjectInitFunc) gnome_canvas_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		canvas_type = gtk_type_unique (gtk_container_get_type (), &canvas_info);
	}

	return canvas_type;
}

static void
gnome_canvas_class_init (GnomeCanvasClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;

	canvas_parent_class = gtk_type_class (gtk_container_get_type ());

	object_class->destroy = gnome_canvas_destroy;

	widget_class->map = gnome_canvas_map;
	widget_class->unmap = gnome_canvas_unmap;
	widget_class->realize = gnome_canvas_realize;
	widget_class->unrealize = gnome_canvas_unrealize;
	widget_class->draw = gnome_canvas_draw;
	widget_class->size_request = gnome_canvas_size_request;
	widget_class->size_allocate = gnome_canvas_size_allocate;
	widget_class->button_press_event = gnome_canvas_button;
	widget_class->button_release_event = gnome_canvas_button;
	widget_class->motion_notify_event = gnome_canvas_motion;
	widget_class->expose_event = gnome_canvas_expose;
	widget_class->key_press_event = gnome_canvas_key;
	widget_class->key_release_event = gnome_canvas_key;
	widget_class->enter_notify_event = gnome_canvas_crossing;
	widget_class->leave_notify_event = gnome_canvas_crossing;
}

static void
gnome_canvas_init (GnomeCanvas *canvas)
{
	GTK_WIDGET_UNSET_FLAGS (canvas, GTK_NO_WINDOW);

	canvas->idle_id = -1;

	canvas->scroll_x1 = 0.0;
	canvas->scroll_y1 = 0.0;
	canvas->scroll_x2 = 100.0; /* not unreasonable defaults, I hope */
	canvas->scroll_y2 = 100.0;

	canvas->pixels_per_unit = 1.0;

	canvas->width  = 100;
	canvas->height = 100;
}

static void
gnome_canvas_destroy (GtkObject *object)
{
	GnomeCanvas *canvas;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (object));

	canvas = GNOME_CANVAS (object);

	gtk_object_destroy (GTK_OBJECT (canvas->root));

	if (GTK_OBJECT_CLASS (canvas_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (canvas_parent_class)->destroy) (object);
}

GtkWidget *
gnome_canvas_new (GdkVisual *visual, GdkColormap *colormap)
{
	GnomeCanvas *canvas;

	g_return_val_if_fail (visual != NULL, NULL);
	g_return_val_if_fail (colormap != NULL, NULL);
	
	canvas = GNOME_CANVAS (gtk_type_new (gnome_canvas_get_type ()));

	canvas->visual = visual;
	canvas->colormap = colormap;
	canvas->cc = gdk_color_context_new (visual, colormap);

	canvas->root = gnome_canvas_group_new (canvas);

	return GTK_WIDGET (canvas);
}

static void
gnome_canvas_map (GtkWidget *widget)
{
	GnomeCanvas *canvas;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (widget));

	if (GTK_WIDGET_MAPPED (widget))
		return;

	/* Normal widget mapping stuff */

	if (GTK_WIDGET_CLASS (canvas_parent_class)->map)
		(* GTK_WIDGET_CLASS (canvas_parent_class)->map) (widget);

	canvas = GNOME_CANVAS (widget);

	/* Map items */

	if (GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->map)
		(* GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->map) (canvas->root);
}

static void
gnome_canvas_unmap (GtkWidget *widget)
{
	GnomeCanvas *canvas;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (widget));

	canvas = GNOME_CANVAS (widget);

	if (canvas->need_redraw) {
		canvas->need_redraw = FALSE;
		canvas->redraw_x1 = 0;
		canvas->redraw_y1 = 0;
		canvas->redraw_x2 = 0;
		canvas->redraw_y2 = 0;
		gtk_idle_remove (canvas->idle_id);
	}

	/* Unmap items */

	if (GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->unmap)
		(* GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->unmap) (canvas->root);

	/* Normal widget unmapping stuff */

	if (GTK_WIDGET_CLASS (canvas_parent_class)->unmap)
		(* GTK_WIDGET_CLASS (canvas_parent_class)->unmap) (widget);
}

static void
gnome_canvas_realize (GtkWidget *widget)
{
	GnomeCanvas *canvas;
	GdkWindowAttr attributes;
	gint attributes_mask;
	int border_width;
	GdkColor c;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (widget));

	canvas = GNOME_CANVAS (widget);

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

	border_width = GTK_CONTAINER (widget)->border_width;

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x + (widget->allocation.width - canvas->width) / 2;
	attributes.y = widget->allocation.y + (widget->allocation.height - canvas->height) / 2;
	attributes.width = canvas->width;
	attributes.height = canvas->height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = canvas->visual;
	attributes.colormap = canvas->colormap;
	attributes.event_mask = (gtk_widget_get_events (widget)
				 | GDK_EXPOSURE_MASK
				 | GDK_BUTTON_PRESS_MASK
				 | GDK_BUTTON_RELEASE_MASK
				 | GDK_POINTER_MOTION_MASK
				 | GDK_KEY_PRESS_MASK
				 | GDK_KEY_RELEASE_MASK
				 | GDK_ENTER_NOTIFY_MASK
				 | GDK_LEAVE_NOTIFY_MASK);

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

	widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
					 &attributes,
					 attributes_mask);
	gdk_window_set_user_data (widget->window, canvas);

	widget->style = gtk_style_attach (widget->style, widget->window);

	if (canvas->bg_set) {
		c.pixel = canvas->bg_pixel;
		gdk_window_set_background (widget->window, &c);
	} else
		gdk_window_set_background (widget->window, &widget->style->bg[GTK_STATE_NORMAL]);

	canvas->pixmap_gc = gdk_gc_new (widget->window);

	/* Realize items */

	if (GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->realize)
		(* GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->realize) (canvas->root);
}

static void
gnome_canvas_unrealize (GtkWidget *widget)
{
	GnomeCanvas *canvas;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (widget));

	canvas = GNOME_CANVAS (widget);

	/* Unrealize items */

	if (GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->unrealize)
		(* GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->unrealize) (canvas->root);

	gdk_gc_destroy (canvas->pixmap_gc);
	canvas->pixmap_gc = NULL;

	if (GTK_WIDGET_CLASS (canvas_parent_class)->unrealize)
		(* GTK_WIDGET_CLASS (canvas_parent_class)->unrealize) (widget);
}

static void
gnome_canvas_draw (GtkWidget *widget, GdkRectangle *area)
{
	GnomeCanvas *canvas;
	GdkRectangle rect;
	int border_width;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (widget));

	if (!GTK_WIDGET_DRAWABLE (widget))
		return;

	canvas = GNOME_CANVAS (widget);

	if (!area) {
		rect.x = 0;
		rect.y = 0;
		rect.width = canvas->width;
		rect.height = canvas->height;
	} else {
		border_width = GTK_CONTAINER (widget)->border_width;

		rect = *area;
		rect.x -= border_width;
		rect.y -= border_width;
	}

	gnome_canvas_request_redraw (canvas,
				     rect.x + canvas->display_x1,
				     rect.y + canvas->display_y1,
				     rect.x + rect.width + canvas->display_x1,
				     rect.y + rect.height + canvas->display_y1);
}

static void
gnome_canvas_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	GnomeCanvas *canvas;
	int border_width;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (widget));
	g_return_if_fail (requisition != NULL);

	canvas = GNOME_CANVAS (widget);

	border_width = GTK_CONTAINER (widget)->border_width;

	requisition->width = canvas->width + 2 * border_width;
	requisition->height = canvas->height + 2 * border_width;
}

static void
gnome_canvas_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	GnomeCanvas *canvas;
	int border_width;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (widget));
	g_return_if_fail (allocation != NULL);

	canvas = GNOME_CANVAS (widget);

	widget->allocation = *allocation;

	border_width = GTK_CONTAINER (widget)->border_width;

	canvas->width = allocation->width - 2 * border_width;
	canvas->height = allocation->height - 2 * border_width;

	if (GTK_WIDGET_REALIZED (widget))
		gdk_window_move_resize (widget->window,
					allocation->x + border_width,
					allocation->y + border_width,
					canvas->width,
					canvas->height);
}

static void
window_to_world (GnomeCanvas *canvas, double *x, double *y)
{
	*x = canvas->scroll_x1 + (*x + canvas->display_x1) / canvas->pixels_per_unit;
	*y = canvas->scroll_y1 + (*y + canvas->display_y1) / canvas->pixels_per_unit;
}

static void
emit_event (GnomeCanvas *canvas, GdkEvent *event)
{
	GdkEvent ev;
	gint finished;
	GnomeCanvasItem *item;

	ev = *event;

	/* Convert to world coordinates -- we have two cases because of diferent offsets of the
	 * fields in the event structures.
	 */

	switch (ev.type) {
	case GDK_ENTER_NOTIFY:
	case GDK_LEAVE_NOTIFY:
		window_to_world (canvas, &ev.crossing.x, &ev.crossing.y);
		break;
		
	case GDK_MOTION_NOTIFY:
	case GDK_BUTTON_PRESS:
	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
	case GDK_BUTTON_RELEASE:
		window_to_world (canvas, &ev.motion.x, &ev.motion.y);
		break;

	default:
		break;
	}

	/* The event is propagated up the hierarchy (for if someone connected to a group instead of
	 * a leaf event), and emission is stopped if a handler returns TRUE, just like for GtkWidget
	 * events.
	 */

	for (finished = FALSE, item = canvas->current_item; item && !finished; item = item->parent)
		gtk_signal_emit (GTK_OBJECT (item), item_signals[ITEM_EVENT],
				 &ev,
				 &finished);
}

static void
pick_current_item (GnomeCanvas *canvas, GdkEvent *event)
{
	int button_down;
	double x, y;
	int cx, cy;

	/* If a button is down, we'll perform enter and leave events on the current item, but not
	 * enter on any other item.  This is more or less like X pointer grabbing for canvas items.  */

	button_down = canvas->state & (GDK_BUTTON1_MASK
				       | GDK_BUTTON2_MASK
				       | GDK_BUTTON3_MASK
				       | GDK_BUTTON4_MASK
				       | GDK_BUTTON5_MASK);
	if (!button_down)
		canvas->left_grabbed_item = FALSE;

	/* Save the event in the canvas.  This is used to synthesize enter and leave events in case
	 * the current item changes.  It is also used to re-pick the current item if the current one
	 * gets deleted.  Also, synthesize an enter event.
	 */

	if (event != &canvas->pick_event) {
		if ((event->type == GDK_MOTION_NOTIFY) || (event->type == GDK_BUTTON_RELEASE)) {
			/* these fields have the same offsets in both types of events */

			canvas->pick_event.crossing.type       = GDK_ENTER_NOTIFY;
			canvas->pick_event.crossing.window     = event->motion.window;
			canvas->pick_event.crossing.send_event = event->motion.send_event;
			canvas->pick_event.crossing.subwindow  = NULL;
			canvas->pick_event.crossing.x          = event->motion.x;
			canvas->pick_event.crossing.y          = event->motion.y;
			canvas->pick_event.crossing.mode       = GDK_CROSSING_NORMAL;
			canvas->pick_event.crossing.detail     = GDK_NOTIFY_NONLINEAR;
			canvas->pick_event.crossing.focus      = FALSE;
			canvas->pick_event.crossing.state      = event->motion.state;

			/* these fields don't have the same offsets in both types of events */

			if (event->type == GDK_MOTION_NOTIFY) {
				canvas->pick_event.crossing.x_root = event->motion.x_root;
				canvas->pick_event.crossing.y_root = event->motion.y_root;
			} else {
				canvas->pick_event.crossing.x_root = event->button.x_root;
				canvas->pick_event.crossing.y_root = event->button.y_root;
			}
		} else
			canvas->pick_event = *event;
	}

	/* Don't do anything else if this is a recursive call */

	if (canvas->in_repick)
		return;

	/* LeaveNotify means that there is no current item, so we don't look for one */

	if (canvas->pick_event.type != GDK_LEAVE_NOTIFY) {
		/* these fields don't have the same offsets in both types of events */

		if (canvas->pick_event.type == GDK_ENTER_NOTIFY) {
			x = canvas->pick_event.crossing.x + canvas->display_x1;
			y = canvas->pick_event.crossing.y + canvas->display_y1;
		} else {
			x = canvas->pick_event.motion.x + canvas->display_x1;
			y = canvas->pick_event.motion.y + canvas->display_y1;
		}

		/* canvas pixel coords */

		cx = (int) (x + 0.5);
		cy = (int) (y + 0.5);

		/* world coords */

		x = canvas->scroll_x1 + x / canvas->pixels_per_unit;
		y = canvas->scroll_y1 + y / canvas->pixels_per_unit;

		/* find the closest item */

		(* GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->point) (canvas->root, x, y, cx, cy,
										 &canvas->new_current_item);
	} else
		canvas->new_current_item = NULL;

	if ((canvas->new_current_item == canvas->current_item) && !canvas->left_grabbed_item)
		return; /* current item did not change */

	/* Synthesize events for old and new current items */

	if ((canvas->new_current_item != canvas->current_item)
	    && (canvas->current_item != NULL)
	    && !canvas->left_grabbed_item) {
		GdkEvent new_event;
		GnomeCanvasItem *item;

		item = canvas->current_item;

		new_event = canvas->pick_event;
		new_event.type = GDK_LEAVE_NOTIFY;

		new_event.crossing.detail = GDK_NOTIFY_ANCESTOR;
		canvas->in_repick = TRUE;
		emit_event (canvas, &new_event);
		canvas->in_repick = FALSE;

		/* Check whether someone deleted the current event */

		if ((item == canvas->current_item) && !button_down)
			/*remove_tag (item, GNOME_CANVAS_CURRENT)*/; /* really nothing to do? */
	}

	/* new_current_item may have been set to NULL during the call to emit_event() above */

	if ((canvas->new_current_item != canvas->current_item) && button_down) {
		canvas->left_grabbed_item = TRUE;
		return;
	}

	/* Handle the rest of cases */

	canvas->left_grabbed_item = FALSE;
	canvas->current_item = canvas->new_current_item;

	if (canvas->current_item != NULL) {
		GdkEvent new_event;

		new_event = canvas->pick_event;
		new_event.type = GDK_ENTER_NOTIFY;
		new_event.crossing.detail = GDK_NOTIFY_ANCESTOR;
		emit_event (canvas, &new_event);
	}
}

static gint
gnome_canvas_button (GtkWidget *widget, GdkEventButton *event)
{
	GnomeCanvas *canvas;
	int mask;

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_CANVAS (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	canvas = GNOME_CANVAS (widget);

	switch (event->button) {
	case 1:
		mask = GDK_BUTTON1_MASK;
		break;
	case 2:
		mask = GDK_BUTTON2_MASK;
		break;
	case 3:
		mask = GDK_BUTTON3_MASK;
		break;
	case 4:
		mask = GDK_BUTTON4_MASK;
		break;
	case 5:
		mask = GDK_BUTTON5_MASK;
		break;
	default:
		mask = 0;
	}

	switch (event->type) {
	case GDK_BUTTON_PRESS:
	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
		/* Pick the current item as if the button were not pressed, and then process the
		 * event.  */

		canvas->state = event->state;
		pick_current_item (canvas, (GdkEvent *) event);
		canvas->state ^= mask;
		emit_event (canvas, (GdkEvent *) event);
		break;

	case GDK_BUTTON_RELEASE:
		/* Process the event as if the button were pressed, then repick after the button has
		 * been released
		 */
		
		canvas->state = event->state;
		emit_event (canvas, (GdkEvent *) event);
		event->state ^= mask;
		canvas->state = event->state;
		pick_current_item (canvas, (GdkEvent *) event);
		event->state ^= mask;
		break;

	default:
		g_assert_not_reached ();
	}

	return FALSE;
}

static gint
gnome_canvas_motion (GtkWidget *widget, GdkEventMotion *event)
{
	GnomeCanvas *canvas;

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_CANVAS (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	canvas = GNOME_CANVAS (widget);

	canvas->state = event->state;
	pick_current_item (canvas, (GdkEvent *) event);
	emit_event (canvas, (GdkEvent *) event);

	return FALSE;
}

static gint
gnome_canvas_expose (GtkWidget *widget, GdkEventExpose *event)
{
	GnomeCanvas *canvas;

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_CANVAS (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	if (!GTK_WIDGET_DRAWABLE (widget))
		return FALSE;

	canvas = GNOME_CANVAS (widget);

	gnome_canvas_request_redraw (canvas,
				     event->area.x + canvas->display_x1,
				     event->area.y + canvas->display_y1,
				     event->area.x + event->area.width + canvas->display_x1,
				     event->area.y + event->area.height + canvas->display_y1);

	return FALSE;
}

static gint
gnome_canvas_key (GtkWidget *widget, GdkEventKey *event)
{
	GnomeCanvas *canvas;

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_CANVAS (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	canvas = GNOME_CANVAS (widget);

	emit_event (canvas, (GdkEvent *) event);

	return FALSE;
}

static gint
gnome_canvas_crossing (GtkWidget *widget, GdkEventCrossing *event)
{
	GnomeCanvas *canvas;

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_CANVAS (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	canvas = GNOME_CANVAS (widget);

	canvas->state = event->state;
	pick_current_item (canvas, (GdkEvent *) event);

	return FALSE;
}

static void
paint (GnomeCanvas *canvas)
{
	GtkWidget *widget;
	int draw_x1, draw_y1;
	int draw_x2, draw_y2;
	int width, height;
	GdkPixmap *pixmap;
	GdkColor c;

	if (!((canvas->redraw_x1 < canvas->redraw_x2) && (canvas->redraw_y1 < canvas->redraw_y2)))
		return;

	widget = GTK_WIDGET (canvas);

	/* Compute the intersection between the viewport and the area that needs redrawing.  We
	 * can't simply do this with gdk_rectangle_intersect() because a GdkRectangle only stores
	 * 16-bit values.
	 */

	draw_x1 = canvas->display_x1;
	draw_y1 = canvas->display_y1;
	draw_x2 = canvas->display_x1 + widget->allocation.width;
	draw_y2 = canvas->display_y1 + widget->allocation.height;

	if (canvas->redraw_x1 > draw_x1)
		draw_x1 = canvas->redraw_x1;

	if (canvas->redraw_y1 > draw_y1)
		draw_y1 = canvas->redraw_y1;

	if (canvas->redraw_x2 < draw_x2)
		draw_x2 = canvas->redraw_x2;

	if (canvas->redraw_y2 < draw_y2)
		draw_y2 = canvas->redraw_y2;

	if ((draw_x1 > draw_x2) || (draw_y1 > draw_y2))
		return;

	/* Set up the temporary pixmap */

	width = draw_x2 - draw_x1;
	height = draw_y2 - draw_y1;

	pixmap = gdk_pixmap_new (widget->window, width, height, gtk_widget_get_visual (widget)->depth);

	if (canvas->bg_set) {
		c.pixel = canvas->bg_pixel;
		gdk_gc_set_foreground (canvas->pixmap_gc, &c);
	} else
		gdk_gc_set_foreground (canvas->pixmap_gc, &widget->style->bg[GTK_STATE_NORMAL]);

	gdk_draw_rectangle (pixmap,
			    canvas->pixmap_gc,
			    TRUE,
			    0, 0,
			    width, height);

	/* Draw the items that intersect the area */

	(* GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->draw) (canvas->root, pixmap,
									draw_x1, draw_y1,
									width, height);

	/* Copy the pixmap to the window and clean up */

	gdk_draw_pixmap (widget->window,
			 canvas->pixmap_gc,
			 pixmap,
			 0, 0,
			 draw_x1 - canvas->display_x1, draw_y1 - canvas->display_y1,
			 width, height);

	gdk_pixmap_unref (pixmap);
}

static gint
idle_handler (gpointer data)
{
	GnomeCanvas *canvas;

	canvas = data;

	/* Pick new current item */

	while (canvas->need_repick) {
		canvas->need_repick = FALSE;
		pick_current_item (canvas, &canvas->pick_event);
	}

	/* Paint if able to */

	if (GTK_WIDGET_DRAWABLE (canvas))
		paint (canvas);

	canvas->need_redraw = FALSE;
	canvas->redraw_x1 = 0;
	canvas->redraw_y1 = 0;
	canvas->redraw_x2 = 0;
	canvas->redraw_y2 = 0;

	/* FIXME: scrollbars */

	return FALSE;
}

GnomeCanvasItem *
gnome_canvas_root (GnomeCanvas *canvas)
{
	g_return_val_if_fail (canvas != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS (canvas), NULL);

	return canvas->root;
}

void
gnome_canvas_set_scroll_region (GnomeCanvas *canvas, double x1, double y1, double x2, double y2)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	canvas->scroll_x1 = x1;
	canvas->scroll_y1 = y1;
	canvas->scroll_x2 = x2;
	canvas->scroll_y2 = y2;

	canvas->display_x1 = 0;
	canvas->display_y1 = 0;

	canvas->need_repick = TRUE;
	(* GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->reconfigure) (canvas->root);
	gtk_widget_draw (GTK_WIDGET (canvas), NULL);
}

void
gnome_canvas_set_pixels_per_unit (GnomeCanvas *canvas, double n)
{
	double cx, cy;

	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));
	g_return_if_fail (n > GNOME_CANVAS_EPSILON);

	/* Re-center view */

	gnome_canvas_c2w (canvas,
			  canvas->display_x1 + canvas->width / 2,
			  canvas->display_y1 + canvas->height / 2,
			  &cx,
			  &cy);

	canvas->pixels_per_unit = n;

	gnome_canvas_w2c (canvas,
			  cx - (canvas->width / (2.0 * n)),
			  cy - (canvas->height / (2.0 * n)),
			  &canvas->display_x1,
			  &canvas->display_y1);

	canvas->need_repick = TRUE;
	(* GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->reconfigure) (canvas->root);
	gtk_widget_draw (GTK_WIDGET (canvas), NULL);
}

void
gnome_canvas_set_size (GnomeCanvas *canvas, int width, int height)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));
	g_return_if_fail (width > 0);
	g_return_if_fail (height > 0);

	canvas->width = width;
	canvas->height = height;

	gtk_widget_queue_resize (GTK_WIDGET (canvas));
}

void
gnome_canvas_scroll_to (GnomeCanvas *canvas, int x, int y)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	canvas->display_x1 = x;
	canvas->display_y1 = y;

	gtk_widget_draw (GTK_WIDGET (canvas), NULL);
}

void
gnome_canvas_update_now (GnomeCanvas *canvas)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	if (!canvas->need_redraw)
		return;

	idle_handler (canvas);
	gtk_idle_remove (canvas->idle_id);
	gdk_flush (); /* flush the X queue to ensure repaint */
}

void
gnome_canvas_request_redraw (GnomeCanvas *canvas, int x1, int y1, int x2, int y2)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	if ((x1 == x2) || (y1 == y2))
		return;

	if (canvas->need_redraw) {
		if (x1 < canvas->redraw_x1)
			canvas->redraw_x1 = x1;

		if (y1 < canvas->redraw_y1)
			canvas->redraw_y1 = y1;

		if (x2 > canvas->redraw_x2)
			canvas->redraw_x2 = x2;

		if (y2 > canvas->redraw_y2)
			canvas->redraw_y2 = y2;
	} else {
		canvas->redraw_x1 = x1;
		canvas->redraw_y1 = y1;
		canvas->redraw_x2 = x2;
		canvas->redraw_y2 = y2;

		canvas->need_redraw = TRUE;
		canvas->idle_id = gtk_idle_add (idle_handler, canvas);
	}
}

void
gnome_canvas_w2c (GnomeCanvas *canvas, double wx, double wy, int *cx, int *cy)
{
	double x, y;

	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	if (cx) {
		x = (wx - canvas->scroll_x1) * canvas->pixels_per_unit;

		if (x > 0.0)
			*cx = (int) (x + 0.5);
		else
			*cx = (int) ceil (x - 0.5); /* is this the fastest way to do this? */
	}

	if (cy) {
		y = (wy - canvas->scroll_y1) * canvas->pixels_per_unit;

		if (y > 0.0)
			*cy = (int) (y + 0.5);
		else
			*cy = (int) ceil (y - 0.5);
	}
}

void
gnome_canvas_c2w (GnomeCanvas *canvas, int cx, int cy, double *wx, double *wy)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	if (wx)
		*wx = canvas->scroll_x1 + cx / canvas->pixels_per_unit;

	if (wy)
		*wy = canvas->scroll_y1 + cy / canvas->pixels_per_unit;
}

int
gnome_canvas_get_color (GnomeCanvas *canvas, char *spec, GdkColor *color)
{
	gint n;

	g_return_val_if_fail (canvas != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_CANVAS (canvas), FALSE);
	g_return_val_if_fail (color != NULL, FALSE);

	if (!spec) {
		color->pixel = 0;
		color->red = 0;
		color->green = 0;
		color->blue = 0;
		return FALSE;
	}

	gdk_color_parse (spec, color);

	color->pixel = 0;
	n = 0;
	gdk_color_context_get_pixels (canvas->cc,
				      &color->red,
				      &color->green,
				      &color->blue,
				      1,
				      &color->pixel,
				      &n);

	return TRUE;
}
