/*
 * GnomeCanvas widget - Tk-like canvas widget for Gnome
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author:
 *   Federico Mena <federico@nuclecu.unam.mx>
 */

/*
 * TO-DO list for the canvas:
 *
 * - Implement hiding/showing of items (visibility flag).
 *
 * - Allow to specify whether GnomeCanvasImage sizes are in units or pixels (scale or don't scale).
 *
 * - Implement a flag for gnome_canvas_item_reparent() that tells the function to keep the item
 *   visually in the same place, that is, to keep it in the same place with respect to the canvas
 *   origin.
 *
 * - GC put functions for items.
 *
 * - Stipple for filling items.
 *
 * - Widget item (finish it).
 *
 * - GList *gnome_canvas_gimme_all_items_contained_in_this_area (GnomeCanvas *canvas, Rectangle area);
 *
 * - Have the canvas carry a list of areas to repaint.  If a request_repaint is made for an area
 *   that does not touch any of the areas of the list, queue the new area.  Else, grow the existing,
 *   touched area (see Gnumeric for reasons).
 *
 * - Curve support for line item.
 *
 * - Polygon item.
 *
 * - Arc item.
 *
 * - Sane font handling API.
 *
 * - Get_arg methods for items:
 *   - How to fetch the outline width and know whether it is in pixels or units?
 *
 * - Talk Raph into using a very similar API for GtkCaanvas.
 *
 * - item_class->output (item, "postscript")
 *
 * - item_class->output (item, "bridge_to_raph's_canvas")
 *
 * - Multiple exposure event compression; this may need to be in Gtk/Gdk instead.
 */
#include <stdio.h>
#include <config.h>
#include <math.h>
#include <gdk/gdkprivate.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include "gnome-canvas.h"


static void group_add    (GnomeCanvasGroup *group, GnomeCanvasItem *item);
static void group_remove (GnomeCanvasGroup *group, GnomeCanvasItem *item);


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

static void gnome_canvas_item_realize   (GnomeCanvasItem *item);
static void gnome_canvas_item_unrealize (GnomeCanvasItem *item);
static void gnome_canvas_item_map       (GnomeCanvasItem *item);
static void gnome_canvas_item_unmap     (GnomeCanvasItem *item);

static void emit_event                  (GnomeCanvas *canvas, GdkEvent *event);

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
	class->realize = gnome_canvas_item_realize;
	class->unrealize = gnome_canvas_item_unrealize;
	class->map = gnome_canvas_item_map;
	class->unmap = gnome_canvas_item_unmap;
	class->draw = NULL;
	class->translate = NULL;
	class->event = NULL;
}

GnomeCanvasItem *
gnome_canvas_item_new (GnomeCanvasGroup *parent, GtkType type, const gchar *first_arg_name, ...)
{
	GnomeCanvasItem *item;
	va_list args;

	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS_GROUP (parent), NULL);
	g_return_val_if_fail (gtk_type_is_a (type, gnome_canvas_item_get_type ()), NULL);

	item = GNOME_CANVAS_ITEM (gtk_type_new (type));

	va_start (args, first_arg_name);
	gnome_canvas_item_construct (item, parent, first_arg_name, args);
	va_end (args);

	return item;
}

GnomeCanvasItem *
gnome_canvas_item_newv (GnomeCanvasGroup *parent, GtkType type, guint nargs, GtkArg *args)
{
	GnomeCanvasItem *item;

	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS_GROUP (parent), NULL);
	g_return_val_if_fail (gtk_type_is_a (type, gnome_canvas_item_get_type ()), NULL);

	item = GNOME_CANVAS_ITEM(gtk_type_new (type));

	gnome_canvas_item_constructv (item, parent, nargs, args);

	return item;
}

static void
item_post_create_setup (GnomeCanvasItem *item)
{
	GtkObject *obj;

	obj = GTK_OBJECT (item);

	group_add (GNOME_CANVAS_GROUP (item->parent), item);

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	item->canvas->need_repick = TRUE;
}

void
gnome_canvas_item_construct (GnomeCanvasItem *item, GnomeCanvasGroup *parent, const gchar *first_arg_name, va_list args)
{
        GtkObject *obj;
	GSList *arg_list;
	GSList *info_list;
	char *error;

	g_return_if_fail (parent != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_GROUP (parent));

	obj = GTK_OBJECT(item);

	item->parent = GNOME_CANVAS_ITEM (parent);
	item->canvas = item->parent->canvas;

	arg_list = NULL;
	info_list = NULL;

	error = gtk_object_args_collect (GTK_OBJECT_TYPE (obj), &arg_list, &info_list, first_arg_name, args);

	if (error) {
		g_warning ("gnome_canvas_item_construct(): %s", error);
		g_free (error);
	} else {
		GSList *arg, *info;

		for (arg = arg_list, info = info_list; arg; arg = arg->next, info = info->next)
			gtk_object_arg_set (obj, arg->data, info->data);

		gtk_args_collect_cleanup (arg_list, info_list);
	}

	item_post_create_setup (item);
}
 
void
gnome_canvas_item_constructv(GnomeCanvasItem *item, GnomeCanvasGroup *parent,
			     guint nargs, GtkArg *args)
{
	GtkObject *obj;

	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));
	g_return_if_fail (parent != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_GROUP (parent));

	obj = GTK_OBJECT (item);

	item->parent = GNOME_CANVAS_ITEM (parent);
	item->canvas = item->parent->canvas;

	gtk_object_setv (obj, nargs, args);

	item_post_create_setup (item);
}

static void
gnome_canvas_item_shutdown (GtkObject *object)
{
	GnomeCanvasItem *item;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (object));

	item = GNOME_CANVAS_ITEM (object);

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);

	/* Make the canvas forget about us */

	if (item == item->canvas->current_item) {
		item->canvas->current_item = NULL;
		item->canvas->need_repick = TRUE;
	}

	if (item == item->canvas->new_current_item) {
		item->canvas->new_current_item = NULL;
		item->canvas->need_repick = TRUE;
	}

	if (item == item->canvas->grabbed_item) {
		item->canvas->grabbed_item = NULL;
		gdk_pointer_ungrab (GDK_CURRENT_TIME);
	}

	if (item == item->canvas->focused_item) 
		item->canvas->focused_item = NULL;

	
	/* Normal destroy stuff */

	if (item->object.flags & GNOME_CANVAS_ITEM_MAPPED)
		(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->unmap) (item);

	if (item->object.flags & GNOME_CANVAS_ITEM_REALIZED)
		(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->unrealize) (item);

	if (item->parent)
		group_remove (GNOME_CANVAS_GROUP (item->parent), item);

	if (GTK_OBJECT_CLASS (item_parent_class)->shutdown)
		(* GTK_OBJECT_CLASS (item_parent_class)->shutdown) (object);
}

static void
gnome_canvas_item_realize (GnomeCanvasItem *item)
{
	GTK_OBJECT_SET_FLAGS (item, GNOME_CANVAS_ITEM_REALIZED);
}

static void
gnome_canvas_item_unrealize (GnomeCanvasItem *item)
{
	GTK_OBJECT_UNSET_FLAGS (item, GNOME_CANVAS_ITEM_REALIZED);
}

static void
gnome_canvas_item_map (GnomeCanvasItem *item)
{
	GTK_OBJECT_SET_FLAGS (item, GNOME_CANVAS_ITEM_MAPPED);
}

static void
gnome_canvas_item_unmap (GnomeCanvasItem *item)
{
	GTK_OBJECT_UNSET_FLAGS (item, GNOME_CANVAS_ITEM_MAPPED);
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
gnome_canvas_item_set (GnomeCanvasItem *item, const gchar *first_arg_name, ...)
{
	va_list args;

	va_start (args, first_arg_name);
	gnome_canvas_item_set_valist (item, first_arg_name, args);
	va_end (args);
}

void 
gnome_canvas_item_set_valist (GnomeCanvasItem *item, const gchar *first_arg_name, va_list args)
{
	GSList *arg_list;
	GSList *info_list;
	char *error;

	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));

	arg_list = NULL;
	info_list = NULL;

	error = gtk_object_args_collect (GTK_OBJECT_TYPE (item), &arg_list, &info_list, first_arg_name, args);

	if (error) {
		g_warning ("gnome_canvas_item_set(): %s", error);
		g_free (error);
	} else if (arg_list) {
		GSList *arg;
		GSList *info;
		GtkObject *object;

		gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);

		object = GTK_OBJECT (item);

		for (arg = arg_list, info = info_list; arg; arg = arg->next, info = info->next)
			gtk_object_arg_set (object, arg->data, info->data);

		gtk_args_collect_cleanup (arg_list, info_list);

		gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
		item->canvas->need_repick = TRUE;
	}
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

static void
put_item_after (GList *link, GList *before)
{
	GnomeCanvasGroup *parent;

	if (link == before)
		return;

	parent = GNOME_CANVAS_GROUP (GNOME_CANVAS_ITEM (link->data)->parent);

	if (before == NULL) {
		if (link == parent->item_list)
			return;

		link->prev->next = link->next;

		if (link->next)
			link->next->prev = link->prev;
		else
			parent->item_list_end = link->prev;

		link->prev = before;
		link->next = parent->item_list;
		link->next->prev = link;
		parent->item_list = link;
	} else {
		if ((link == parent->item_list_end) && (before == parent->item_list_end->prev))
			return;

		if (link->next)
			link->next->prev = link->prev;

		if (link->prev)
			link->prev->next = link->next;
		else {
			parent->item_list = link->next;
			parent->item_list->prev = NULL;
		}

		link->prev = before;
		link->next = before->next;

		link->prev->next = link;

		if (link->next)
			link->next->prev = link;
		else
			parent->item_list_end = link;
	}
}

void
gnome_canvas_item_raise (GnomeCanvasItem *item, int positions)
{
	GList *link, *before;
	GnomeCanvasGroup *parent;

	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));
	g_return_if_fail (positions >= 1);

	if (!item->parent)
		return;

	parent = GNOME_CANVAS_GROUP (item->parent);
	link = g_list_find (parent->item_list, item);
	g_assert (link != NULL);

	for (before = link; positions && before; positions--)
		before = before->next;

	if (!before)
		before = parent->item_list_end;

	put_item_after (link, before);

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	item->canvas->need_repick = TRUE;
}

void
gnome_canvas_item_lower (GnomeCanvasItem *item, int positions)
{
	GList *link, *before;
	GnomeCanvasGroup *parent;

	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));
	g_return_if_fail (positions >= 1);

	if (!item->parent)
		return;

	parent = GNOME_CANVAS_GROUP (item->parent);
	link = g_list_find (parent->item_list, item);
	g_assert (link != NULL);

	if (link->prev)
		for (before = link->prev; positions && before; positions--)
			before = before->prev;
	else
		before = NULL;

	put_item_after (link, before);

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	item->canvas->need_repick = TRUE;
}

void
gnome_canvas_item_raise_to_top (GnomeCanvasItem *item)
{
	GList *link;
	GnomeCanvasGroup *parent;

	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));

	if (!item->parent)
		return;

	parent = GNOME_CANVAS_GROUP (item->parent);
	link = g_list_find (parent->item_list, item);
	g_assert (link != NULL);

	put_item_after (link, parent->item_list_end);

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	item->canvas->need_repick = TRUE;
}

void
gnome_canvas_item_lower_to_bottom (GnomeCanvasItem *item)
{
	GList *link;
	GnomeCanvasGroup *parent;

	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));

	if (!item->parent)
		return;

	parent = GNOME_CANVAS_GROUP (item->parent);
	link = g_list_find (parent->item_list, item);
	g_assert (link != NULL);

	put_item_after (link, NULL);

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	item->canvas->need_repick = TRUE;
}

int
gnome_canvas_item_grab (GnomeCanvasItem *item, guint event_mask, GdkCursor *cursor, guint32 etime)
{
	int retval;

	g_return_val_if_fail (item != NULL, GrabNotViewable);
	g_return_val_if_fail (GNOME_IS_CANVAS_ITEM (item), GrabNotViewable);
	g_return_val_if_fail (GTK_WIDGET_MAPPED (item->canvas), GrabNotViewable);

	if (item->canvas->grabbed_item)
		return AlreadyGrabbed;

	retval = gdk_pointer_grab (item->canvas->layout.bin_window,
				   FALSE,
				   event_mask,
				   NULL,
				   cursor,
				   etime);

	if (retval != GrabSuccess)
		return retval;

	item->canvas->grabbed_item = item;
	item->canvas->grabbed_event_mask = event_mask;
	item->canvas->current_item = item; /* So that events go to the grabbed item */

	return retval;
}

void
gnome_canvas_item_ungrab (GnomeCanvasItem *item, guint32 etime)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));

	if (item->canvas->grabbed_item != item)
		return;

	item->canvas->grabbed_item = NULL;

	gdk_pointer_ungrab (etime);
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

/* Returns whether the item is an inferior of or is equal to the parent. */
static int
is_descendant (GnomeCanvasItem *item, GnomeCanvasItem *parent)
{
	for (; item; item = item->parent)
		if (item == parent)
			return TRUE;

	return FALSE;
}

/*
 * gnome_canvas_item_grab_focus:
 * @item: the item that is grabbing the focus
 *
 * Grabs the keyboard focus
 */
void
gnome_canvas_item_grab_focus (GnomeCanvasItem *item)
{
	GnomeCanvasItem *focused_item;
	GdkEvent ev;
	gint finished;
	
	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));
	g_return_if_fail (GTK_WIDGET_CAN_FOCUS (GTK_WIDGET (item->canvas)));
	
	focused_item = item->canvas->focused_item;
	
	if (focused_item){
		ev.type = GDK_FOCUS_CHANGE;
		ev.focus_change.send_event = FALSE;
		ev.focus_change.in = FALSE;

		emit_event (item->canvas, &ev);
	}

	item->canvas->focused_item = item;
	gtk_widget_grab_focus (GTK_WIDGET (item->canvas));
}

/*
 * gnome_canvas_item_reparent:
 * @item:      Item to reparent
 * @new_group: New group where the item is moved to
 *
 * This moves the item from its current group to the group specified
 * in NEW_GROUP.
 */
void
gnome_canvas_item_reparent (GnomeCanvasItem *item, GnomeCanvasGroup *new_group)
{
	GnomeCanvasItem *old_parent;

	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));
	g_return_if_fail (new_group != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_GROUP (new_group));

	/* Both items need to be in the same canvas */

	g_return_if_fail (item->canvas == GNOME_CANVAS_ITEM (new_group)->canvas);

	/*
	 * The group cannot be an inferior of the item or be the item itself -- this also takes care
	 * of the case where the item is the root item of the canvas.
	 */

	g_return_if_fail (!is_descendant (GNOME_CANVAS_ITEM (new_group), item));

	/* Everything is ok, now actually reparent the item */

	gtk_object_ref (GTK_OBJECT (item)); /* protect it from the unref in group_remove */

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);

	if (item->parent)
		group_remove (GNOME_CANVAS_GROUP (item->parent), item);

	item->parent = GNOME_CANVAS_ITEM (new_group);
	group_add (new_group, item);

	/*
	 * Rebuild the new parent group's bounds.  This will take care of reconfiguring the item and
	 * all its children.
	 */

	gnome_canvas_group_child_bounds (new_group, NULL);

	/* Redraw and repick */

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	item->canvas->need_repick = TRUE;

	gtk_object_unref (GTK_OBJECT (item));
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
static void gnome_canvas_group_get_arg     (GtkObject             *object,
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

	gtk_object_add_arg_type ("GnomeCanvasGroup::x", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, GROUP_ARG_X);
	gtk_object_add_arg_type ("GnomeCanvasGroup::y", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, GROUP_ARG_Y);

	object_class->set_arg = gnome_canvas_group_set_arg;
	object_class->get_arg = gnome_canvas_group_get_arg;
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

static void
gnome_canvas_group_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasGroup *group;

	group = GNOME_CANVAS_GROUP (object);

	switch (arg_id) {
	case GROUP_ARG_X:
		GTK_VALUE_DOUBLE (*arg) = group->xpos;
		break;

	case GROUP_ARG_Y:
		GTK_VALUE_DOUBLE (*arg) = group->ypos;
		break;

	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
}

static void
gnome_canvas_group_destroy (GtkObject *object)
{
	GnomeCanvasGroup *group;
	GnomeCanvasItem *child;
	GList *list;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_GROUP (object));

	group = GNOME_CANVAS_GROUP (object);

	list = group->item_list;
	while (list) {
		child = list->data;
		list = list->next;

		gtk_object_destroy (GTK_OBJECT (child));
	}

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

		if (!(i->object.flags & GNOME_CANVAS_ITEM_REALIZED))
			(* GNOME_CANVAS_ITEM_CLASS (i->object.klass)->realize) (i);
	}

	(* group_parent_class->realize) (item);
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

		if (i->object.flags & GNOME_CANVAS_ITEM_REALIZED)
			(* GNOME_CANVAS_ITEM_CLASS (i->object.klass)->unrealize) (i);
	}

	(* group_parent_class->unrealize) (item);
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

		if (!(i->object.flags & GNOME_CANVAS_ITEM_MAPPED))
			(* GNOME_CANVAS_ITEM_CLASS (i->object.klass)->map) (i);
	}

	(* group_parent_class->map) (item);
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

		if (i->object.flags & GNOME_CANVAS_ITEM_MAPPED)
			(* GNOME_CANVAS_ITEM_CLASS (i->object.klass)->unmap) (i);
	}

	(* group_parent_class->unmap) (item);
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

	dist = 0.0; /* keep gcc happy */

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

static void
group_add (GnomeCanvasGroup *group, GnomeCanvasItem *item)
{
	gtk_object_ref (GTK_OBJECT (item));
	gtk_object_sink (GTK_OBJECT (item));

	if (!group->item_list) {
		group->item_list = g_list_append (group->item_list, item);
		group->item_list_end = group->item_list;
	} else
		group->item_list_end = g_list_append (group->item_list_end, item)->next;

	if (group->item.object.flags & GNOME_CANVAS_ITEM_REALIZED)
		(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->realize) (item);

	if (group->item.object.flags & GNOME_CANVAS_ITEM_MAPPED)
		(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->map) (item);

	gnome_canvas_group_child_bounds (group, item);
}

static void
group_remove (GnomeCanvasGroup *group, GnomeCanvasItem *item)
{
	GList *children;

	g_return_if_fail (group != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_GROUP (group));
	g_return_if_fail (item != NULL);

	for (children = group->item_list; children; children = children->next)
		if (children->data == item) {
			if (item->object.flags & GNOME_CANVAS_ITEM_MAPPED)
				(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->unmap) (item);

			if (item->object.flags & GNOME_CANVAS_ITEM_REALIZED)
				(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->unrealize) (item);

			/* Unparent the child */

			item->parent = NULL;
			gtk_object_unref (GTK_OBJECT (item));

			/* Remove it from the list */

			if (children == group->item_list_end)
				group->item_list_end = children->prev;

			group->item_list = g_list_remove_link (group->item_list, children);
			g_list_free (children);
			break;
		}
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

static gint gnome_canvas_focus_in       (GtkWidget        *widget,
					 GdkEventFocus    *event);
static gint gnome_canvas_focus_out      (GtkWidget        *widget,
					 GdkEventFocus    *event);

static GtkLayoutClass *canvas_parent_class;


#define DISPLAY_X1(canvas) (GNOME_CANVAS (canvas)->layout.xoffset)
#define DISPLAY_Y1(canvas) (GNOME_CANVAS (canvas)->layout.yoffset)


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

		canvas_type = gtk_type_unique (gtk_layout_get_type (), &canvas_info);
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

	canvas_parent_class = gtk_type_class (gtk_layout_get_type ());

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
	widget_class->focus_in_event = gnome_canvas_focus_in;
	widget_class->focus_out_event = gnome_canvas_focus_out;
}

static void
panic_root_destroyed (GtkObject *object, gpointer data)
{
	g_error ("Eeeek, root item %p of canvas %p was destroyed!", object, data);
}

static void
gnome_canvas_init (GnomeCanvas *canvas)
{
	canvas->idle_id = -1;

	canvas->scroll_x1 = 0.0;
	canvas->scroll_y1 = 0.0;
	canvas->scroll_x2 = canvas->layout.width;
	canvas->scroll_y2 = canvas->layout.height;

	canvas->pixels_per_unit = 1.0;

	canvas->width  = 100; /* default window size */
	canvas->height = 100;

	gtk_layout_set_hadjustment (GTK_LAYOUT (canvas), NULL);
	gtk_layout_set_vadjustment (GTK_LAYOUT (canvas), NULL);

	canvas->cc = gdk_color_context_new (gtk_widget_get_visual (GTK_WIDGET (canvas)),
					    gtk_widget_get_colormap (GTK_WIDGET (canvas)));

	/* Create the root item as a special case */

	canvas->root = GNOME_CANVAS_ITEM (gtk_type_new (gnome_canvas_group_get_type ()));
	canvas->root->canvas = canvas;

	gtk_object_ref (GTK_OBJECT (canvas->root));
	gtk_object_sink (GTK_OBJECT (canvas->root));

	canvas->root_destroy_id = gtk_signal_connect (GTK_OBJECT (canvas->root), "destroy",
						      (GtkSignalFunc) panic_root_destroyed,
						      canvas);

	canvas->need_repick = TRUE;
}

static void
gnome_canvas_destroy (GtkObject *object)
{
	GnomeCanvas *canvas;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (object));

	canvas = GNOME_CANVAS (object);

	gtk_signal_disconnect (GTK_OBJECT (canvas->root), canvas->root_destroy_id);
	gtk_object_unref (GTK_OBJECT (canvas->root));

	if (GTK_OBJECT_CLASS (canvas_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (canvas_parent_class)->destroy) (object);
}

GtkWidget *
gnome_canvas_new (void)
{
	return GTK_WIDGET (gtk_type_new (gnome_canvas_get_type ()));
}

static void
gnome_canvas_map (GtkWidget *widget)
{
	GnomeCanvas *canvas;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (widget));

	/* Normal widget mapping stuff */

	if (GTK_WIDGET_CLASS (canvas_parent_class)->map)
		(* GTK_WIDGET_CLASS (canvas_parent_class)->map) (widget);

	canvas = GNOME_CANVAS (widget);

	/* Map items */

	if (GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->map)
		(* GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->map) (canvas->root);
}

static void
shutdown_transients (GnomeCanvas *canvas)
{
	if (canvas->need_redraw) {
		canvas->need_redraw = FALSE;
		canvas->redraw_x1 = 0;
		canvas->redraw_y1 = 0;
		canvas->redraw_x2 = 0;
		canvas->redraw_y2 = 0;
		gtk_idle_remove (canvas->idle_id);
	}

	if (canvas->grabbed_item) {
		canvas->grabbed_item = NULL;
		gdk_pointer_ungrab (GDK_CURRENT_TIME);
	}
}

static void
gnome_canvas_unmap (GtkWidget *widget)
{
	GnomeCanvas *canvas;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (widget));

	canvas = GNOME_CANVAS (widget);

	shutdown_transients (canvas);

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

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (widget));

	/* Normal widget realization stuff */

	if (GTK_WIDGET_CLASS (canvas_parent_class)->realize)
		(* GTK_WIDGET_CLASS (canvas_parent_class)->realize) (widget);

	canvas = GNOME_CANVAS (widget);

	gdk_window_set_events (canvas->layout.bin_window,
			       (gdk_window_get_events (canvas->layout.bin_window)
				 | GDK_EXPOSURE_MASK
				 | GDK_BUTTON_PRESS_MASK
				 | GDK_BUTTON_RELEASE_MASK
				 | GDK_POINTER_MOTION_MASK
				 | GDK_KEY_PRESS_MASK
				 | GDK_KEY_RELEASE_MASK
				 | GDK_ENTER_NOTIFY_MASK
				 | GDK_LEAVE_NOTIFY_MASK
				 | GDK_FOCUS_CHANGE_MASK));

	/* Create our own temporary pixmap gc and realize all the items */

	canvas->pixmap_gc = gdk_gc_new (canvas->layout.bin_window);

	(* GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->realize) (canvas->root);
}

static void
gnome_canvas_unrealize (GtkWidget *widget)
{
	GnomeCanvas *canvas;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (widget));

	canvas = GNOME_CANVAS (widget);

	shutdown_transients (canvas);

	/* Unrealize items and parent widget */

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

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (widget));

	if (!GTK_WIDGET_DRAWABLE (widget))
		return;

	canvas = GNOME_CANVAS (widget);

	gnome_canvas_request_redraw (canvas,
				     area->x + DISPLAY_X1 (canvas) - canvas->zoom_xofs,
				     area->y + DISPLAY_Y1 (canvas) - canvas->zoom_yofs,
				     area->x + area->width + DISPLAY_X1 (canvas) - canvas->zoom_xofs,
				     area->y + area->height + DISPLAY_Y1 (canvas) - canvas->zoom_yofs);
}

static void
gnome_canvas_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	GnomeCanvas *canvas;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (widget));
	g_return_if_fail (requisition != NULL);

	if (GTK_WIDGET_CLASS (canvas_parent_class)->size_request)
		(* GTK_WIDGET_CLASS (canvas_parent_class)->size_request) (widget, requisition);

	canvas = GNOME_CANVAS (widget);

	if (requisition->width < canvas->width)
		requisition->width = canvas->width;

	if (requisition->height < canvas->height)
		requisition->height = canvas->height;
}

static void
scroll_to (GnomeCanvas *canvas, int cx, int cy)
{
	int scroll_maxx, scroll_maxy;
	int right_limit, bottom_limit;
	int old_zoom_xofs, old_zoom_yofs;
	int changed, changed_x, changed_y;

	/*
	 * Adjust the scrolling offset and the zoom offset to keep as much as possible of the canvas
	 * scrolling region in view.
	 */

	gnome_canvas_w2c (canvas, canvas->scroll_x2, canvas->scroll_y2, &scroll_maxx, &scroll_maxy);

	right_limit = scroll_maxx - canvas->width;
	bottom_limit = scroll_maxy - canvas->height;

	old_zoom_xofs = canvas->zoom_xofs;
	old_zoom_yofs = canvas->zoom_yofs;

	if (right_limit < 0) {
		cx = 0;
		canvas->zoom_xofs = (canvas->width - scroll_maxx) / 2;
		scroll_maxx = canvas->width;
	} else if (cx < 0) {
		cx = 0;
		canvas->zoom_xofs = 0;
	} else if (cx > right_limit) {
		cx = right_limit;
		canvas->zoom_xofs = 0;
	} else
		canvas->zoom_xofs = 0;

	if (bottom_limit < 0) {
		cy = 0;
		canvas->zoom_yofs = (canvas->height - scroll_maxy) / 2;
		scroll_maxy = canvas->height;
	} else if (cy < 0) {
		cy = 0;
		canvas->zoom_yofs = 0;
	} else if (cy > bottom_limit) {
		cy = bottom_limit;
		canvas->zoom_yofs = 0;
	} else
		canvas->zoom_yofs = 0;

	changed_x = ((int) canvas->layout.hadjustment->value) != cx;
	changed_y = ((int) canvas->layout.vadjustment->value) != cy;

	changed = ((canvas->zoom_xofs != old_zoom_xofs)
		   || (canvas->zoom_yofs != old_zoom_yofs)
		   || (changed_x && changed_y));

	if (changed)
		gtk_layout_freeze (GTK_LAYOUT (canvas));

	if ((scroll_maxx != (int) canvas->layout.width) || (scroll_maxy != (int) canvas->layout.height))
		gtk_layout_set_size (GTK_LAYOUT (canvas), scroll_maxx, scroll_maxy);

	if (changed_x) {
		canvas->layout.hadjustment->value = cx;
		gtk_signal_emit_by_name (GTK_OBJECT (canvas->layout.hadjustment), "value_changed");
	}

	if (changed_y) {
		canvas->layout.vadjustment->value = cy;
		gtk_signal_emit_by_name (GTK_OBJECT (canvas->layout.vadjustment), "value_changed");
	}

	if (changed)
		gtk_layout_thaw (GTK_LAYOUT (canvas));
}

static void
gnome_canvas_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	GnomeCanvas *canvas;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (widget));
	g_return_if_fail (allocation != NULL);

	if (GTK_WIDGET_CLASS (canvas_parent_class)->size_allocate)
		(* GTK_WIDGET_CLASS (canvas_parent_class)->size_allocate) (widget, allocation);

	canvas = GNOME_CANVAS (widget);

	canvas->width = allocation->width;
	canvas->height = allocation->height;

	/* Recenter the view, if appropriate */

	scroll_to (canvas, DISPLAY_X1 (canvas), DISPLAY_Y1 (canvas));

	canvas->layout.hadjustment->page_size = allocation->width;
	canvas->layout.hadjustment->page_increment = allocation->width / 2;
	gtk_signal_emit_by_name (GTK_OBJECT (canvas->layout.hadjustment), "changed");

	canvas->layout.vadjustment->page_size = allocation->height;
	canvas->layout.vadjustment->page_increment = allocation->height / 2;
	gtk_signal_emit_by_name (GTK_OBJECT (canvas->layout.vadjustment), "changed");
}

static void
window_to_world (GnomeCanvas *canvas, double *x, double *y)
{
	*x = canvas->scroll_x1 + (*x + DISPLAY_X1 (canvas) - canvas->zoom_xofs) / canvas->pixels_per_unit;
	*y = canvas->scroll_y1 + (*y + DISPLAY_Y1 (canvas) - canvas->zoom_yofs) / canvas->pixels_per_unit;
}

static void
emit_event (GnomeCanvas *canvas, GdkEvent *event)
{
	GdkEvent ev;
	gint finished;
	GnomeCanvasItem *item;
	guint mask;

	/* Perform checks for grabbed items */

	if (canvas->grabbed_item && !is_descendant (canvas->current_item, canvas->grabbed_item))
		return;

	if (canvas->grabbed_item) {
		switch (event->type) {
		case GDK_ENTER_NOTIFY:
			mask = GDK_ENTER_NOTIFY_MASK;
			break;

		case GDK_LEAVE_NOTIFY:
			mask = GDK_LEAVE_NOTIFY_MASK;
			break;

		case GDK_MOTION_NOTIFY:
			mask = GDK_POINTER_MOTION_MASK;
			break;

		case GDK_BUTTON_PRESS:
		case GDK_2BUTTON_PRESS:
		case GDK_3BUTTON_PRESS:
			mask = GDK_BUTTON_PRESS_MASK;
			break;

		case GDK_BUTTON_RELEASE:
			mask = GDK_BUTTON_RELEASE_MASK;
			break;

		case GDK_KEY_PRESS:
			mask = GDK_KEY_PRESS_MASK;
			break;

		case GDK_KEY_RELEASE:
			mask = GDK_KEY_RELEASE_MASK;
			break;

		default:
			mask = 0;
			break;
		}

		if (!(mask & canvas->grabbed_event_mask))
			return;

	}

	/*
	 * Convert to world coordinates -- we have two cases because of diferent offsets of the
	 * fields in the event structures.
	 */

	ev = *event;

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

	/*
	 * Choose where do we send the event
	 */
	item = canvas->current_item;
	
	if (event->type == GDK_KEY_PRESS || event->type == GDK_KEY_RELEASE || event->type == GDK_FOCUS_CHANGE)
		if (canvas->focused_item)
			item = canvas->focused_item;
	    
	/*
	 * The event is propagated up the hierarchy (for if someone connected to a group instead of
	 * a leaf event), and emission is stopped if a handler returns TRUE, just like for GtkWidget
	 * events.
	 */
	for (finished = FALSE, item = canvas->current_item; item && !finished; item = item->parent)
	  {
		gtk_signal_emit (GTK_OBJECT (item), item_signals[ITEM_EVENT],
				 &ev,
				 &finished);
	  }
}

static void
pick_current_item (GnomeCanvas *canvas, GdkEvent *event)
{
	int button_down;
	double x, y;
	int cx, cy;

	/*
	 * If a button is down, we'll perform enter and leave events on the current item, but not
	 * enter on any other item.  This is more or less like X pointer grabbing for canvas items.
	 */

	button_down = canvas->state & (GDK_BUTTON1_MASK
				       | GDK_BUTTON2_MASK
				       | GDK_BUTTON3_MASK
				       | GDK_BUTTON4_MASK
				       | GDK_BUTTON5_MASK);
	if (!button_down)
		canvas->left_grabbed_item = FALSE;

	/*
	 * Save the event in the canvas.  This is used to synthesize enter and leave events in case
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
			x = canvas->pick_event.crossing.x + DISPLAY_X1 (canvas) - canvas->zoom_xofs;
			y = canvas->pick_event.crossing.y + DISPLAY_Y1 (canvas) - canvas->zoom_yofs;
		} else {
			x = canvas->pick_event.motion.x + DISPLAY_X1 (canvas) - canvas->zoom_xofs;
			y = canvas->pick_event.motion.y + DISPLAY_Y1 (canvas) - canvas->zoom_yofs;
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

	if (event->window != canvas->layout.bin_window)
		return FALSE;

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
		/*
		 * Pick the current item as if the button were not pressed, and then process the
		 * event.
		 */

		canvas->state = event->state;
		pick_current_item (canvas, (GdkEvent *) event);
		canvas->state ^= mask;
		emit_event (canvas, (GdkEvent *) event);
		break;

	case GDK_BUTTON_RELEASE:
		/*
		 * Process the event as if the button were pressed, then repick after the button has
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

	if (event->window != canvas->layout.bin_window)
		return FALSE;

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

	canvas = GNOME_CANVAS (widget);

	if (!GTK_WIDGET_DRAWABLE (widget) || (event->window != canvas->layout.bin_window))
		return FALSE;

	gnome_canvas_request_redraw (canvas,
				     event->area.x + DISPLAY_X1 (canvas) - canvas->zoom_xofs,
				     event->area.y + DISPLAY_Y1 (canvas) - canvas->zoom_yofs,
				     event->area.x + event->area.width + DISPLAY_X1 (canvas) - canvas->zoom_xofs,
				     event->area.y + event->area.height + DISPLAY_Y1 (canvas) - canvas->zoom_yofs);

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

	if (event->window != canvas->layout.bin_window)
		return FALSE;

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

	if (event->window != canvas->layout.bin_window)
		return FALSE;

	canvas->state = event->state;
	pick_current_item (canvas, (GdkEvent *) event);

	return FALSE;
}

static gint
gnome_canvas_focus_in (GtkWidget *widget, GdkEventFocus *event)
{
	GnomeCanvas *canvas;
	
	canvas = GNOME_CANVAS (widget);

	if (canvas->focused_item){
		emit_event (canvas, (GdkEvent *) event);

		return TRUE;
	}
	return FALSE;
}

static gint
gnome_canvas_focus_out (GtkWidget *widget, GdkEventFocus *event)
{
	GnomeCanvas *canvas;
	
	canvas = GNOME_CANVAS (widget);
	if (canvas->focused_item){
		emit_event (canvas, (GdkEvent *) event);
		return TRUE;
	}
	
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

	if (!((canvas->redraw_x1 < canvas->redraw_x2) && (canvas->redraw_y1 < canvas->redraw_y2)))
		return;

	widget = GTK_WIDGET (canvas);

	/*
	 * Compute the intersection between the viewport and the area that needs redrawing.  We
	 * can't simply do this with gdk_rectangle_intersect() because a GdkRectangle only stores
	 * 16-bit values.
	 */

	draw_x1 = DISPLAY_X1 (canvas) - canvas->zoom_xofs;
	draw_y1 = DISPLAY_Y1 (canvas) - canvas->zoom_yofs;
	draw_x2 = draw_x1 + canvas->width;
	draw_y2 = draw_y1 + canvas->height;

	if (canvas->redraw_x1 > draw_x1)
		draw_x1 = canvas->redraw_x1;

	if (canvas->redraw_y1 > draw_y1)
		draw_y1 = canvas->redraw_y1;

	if (canvas->redraw_x2 < draw_x2)
		draw_x2 = canvas->redraw_x2;

	if (canvas->redraw_y2 < draw_y2)
		draw_y2 = canvas->redraw_y2;

	if ((draw_x1 >= draw_x2) || (draw_y1 >= draw_y2))
		return;

	/* Set up the temporary pixmap */

	width = draw_x2 - draw_x1;
	height = draw_y2 - draw_y1;

	pixmap = gdk_pixmap_new (canvas->layout.bin_window, width, height, gtk_widget_get_visual (widget)->depth);

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
#if 0
	gdk_draw_line (pixmap,
		       widget->style->black_gc,
		       0, 0,
		       width - 1, height - 1);
	gdk_draw_line (pixmap,
		       widget->style->black_gc,
		       width - 1, 0,
		       0, height - 1);
#endif
	/* Copy the pixmap to the window and clean up */

	gdk_draw_pixmap (canvas->layout.bin_window,
			 canvas->pixmap_gc,
			 pixmap,
			 0, 0,
			 draw_x1 - DISPLAY_X1 (canvas) + canvas->zoom_xofs,
			 draw_y1 - DISPLAY_Y1 (canvas) + canvas->zoom_yofs,
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

GnomeCanvasGroup *
gnome_canvas_root (GnomeCanvas *canvas)
{
	g_return_val_if_fail (canvas != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS (canvas), NULL);

	return GNOME_CANVAS_GROUP (canvas->root);
}

void
gnome_canvas_set_scroll_region (GnomeCanvas *canvas, double x1, double y1, double x2, double y2)
{
	double wxofs, wyofs;
	int xofs, yofs;

	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	/*
	 * Set the new scrolling region.  If possible, do not move the visible contents of the
	 * canvas.
	 */

	gnome_canvas_c2w (canvas,
			  DISPLAY_X1 (canvas) - canvas->zoom_xofs,
			  DISPLAY_Y1 (canvas) - canvas->zoom_yofs,
			  &wxofs, &wyofs);

	canvas->scroll_x1 = x1;
	canvas->scroll_y1 = y1;
	canvas->scroll_x2 = x2;
	canvas->scroll_y2 = y2;

	gnome_canvas_w2c (canvas, wxofs, wyofs, &xofs, &yofs);

	gtk_layout_freeze (GTK_LAYOUT (canvas));

	scroll_to (canvas, xofs, yofs);

	canvas->need_repick = TRUE;
	(* GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->reconfigure) (canvas->root);

	gtk_layout_thaw (GTK_LAYOUT (canvas));
}

void
gnome_canvas_get_scroll_region (GnomeCanvas *canvas, double *x1, double *y1, double *x2, double *y2)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	if (x1)
		*x1 = canvas->scroll_x1;

	if (y1)
		*y1 = canvas->scroll_y1;

	if (x2)
		*x2 = canvas->scroll_x2;

	if (y2)
		*y2 = canvas->scroll_y2;
}

void
gnome_canvas_set_pixels_per_unit (GnomeCanvas *canvas, double n)
{
	double cx, cy;
	int x1, y1;

	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));
	g_return_if_fail (n > GNOME_CANVAS_EPSILON);

	/* Re-center view */

	gnome_canvas_c2w (canvas,
			  DISPLAY_X1 (canvas) - canvas->zoom_xofs + canvas->width / 2,
			  DISPLAY_Y1 (canvas) - canvas->zoom_yofs + canvas->height / 2,
			  &cx,
			  &cy);

	canvas->pixels_per_unit = n;

	gnome_canvas_w2c (canvas,
			  cx - (canvas->width / (2.0 * n)),
			  cy - (canvas->height / (2.0 * n)),
			  &x1, &y1);

	gtk_layout_freeze (GTK_LAYOUT (canvas));

	scroll_to (canvas, x1, y1);

	canvas->need_repick = TRUE;
	(* GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->reconfigure) (canvas->root);

	gtk_layout_thaw (GTK_LAYOUT (canvas));
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
gnome_canvas_scroll_to (GnomeCanvas *canvas, int cx, int cy)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	scroll_to (canvas, cx, cy);
}

void
gnome_canvas_get_scroll_offsets (GnomeCanvas *canvas, int *cx, int *cy)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	if (cx)
		*cx = canvas->layout.hadjustment->value;

	if (cy)
		*cy = canvas->layout.vadjustment->value;
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

	if (!GTK_WIDGET_DRAWABLE (canvas) || (x1 == x2) || (y1 == y2))
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
