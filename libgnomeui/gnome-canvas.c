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
 * - Allow to specify whether GnomeCanvasImage sizes are in units or pixels (scale or don't scale).
 *
 * - Implement a flag for gnome_canvas_item_reparent() that tells the function to keep the item
 *   visually in the same place, that is, to keep it in the same place with respect to the canvas
 *   origin.
 *
 * - GC put functions for items.
 *
 * - Widget item (finish it).
 *
 * - GList *gnome_canvas_gimme_all_items_contained_in_this_area (GnomeCanvas *canvas, Rectangle area);
 *
 * - Retrofit all the primitive items with microtile support.
 *
 * - Curve support for line item.
 *
 * - Arc item.
 *
 * - Sane font handling API.
 *
 * - Get_arg methods for items:
 *   - How to fetch the outline width and know whether it is in pixels or units?
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
#include "libart_lgpl/art_rect.h"
#include "libart_lgpl/art_rect_uta.h"
#include "libart_lgpl/art_uta_rect.h"
#include "libart_lgpl/art_uta_ops.h"


static void group_add    (GnomeCanvasGroup *group, GnomeCanvasItem *item);
static void group_remove (GnomeCanvasGroup *group, GnomeCanvasItem *item);


/*** GnomeCanvasItem ***/


enum {
	ITEM_EVENT,
	ITEM_LAST_SIGNAL
};


static void gnome_canvas_request_update (GnomeCanvas *canvas);


typedef gint (* GnomeCanvasItemSignal1) (GtkObject *item,
					 gpointer   arg1,
					 gpointer   data);

static void gnome_canvas_item_marshal_signal_1 (GtkObject     *object,
						GtkSignalFunc  func,
						gpointer       func_data,
						GtkArg        *args);

static void gnome_canvas_item_class_init (GnomeCanvasItemClass *class);
static void gnome_canvas_item_init       (GnomeCanvasItem      *item);
static void gnome_canvas_item_shutdown   (GtkObject            *object);

static void gnome_canvas_item_realize   (GnomeCanvasItem *item);
static void gnome_canvas_item_unrealize (GnomeCanvasItem *item);
static void gnome_canvas_item_map       (GnomeCanvasItem *item);
static void gnome_canvas_item_unmap     (GnomeCanvasItem *item);

static int emit_event (GnomeCanvas *canvas, GdkEvent *event);

static guint item_signals[ITEM_LAST_SIGNAL] = { 0 };

static GtkObjectClass *item_parent_class;


/**
 * gnome_canvas_item_get_type:
 *
 * Registers the &GnomeCanvasItem class if necessary, and returns the type ID associated to it.
 * 
 * Return value:  The type ID of the &GnomeCanvasItem class.
 **/
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
			(GtkObjectInitFunc) gnome_canvas_item_init,
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

	class->realize = gnome_canvas_item_realize;
	class->unrealize = gnome_canvas_item_unrealize;
	class->map = gnome_canvas_item_map;
	class->unmap = gnome_canvas_item_unmap;
}

static void
gnome_canvas_item_init (GnomeCanvasItem *item)
{
	item->object.flags |= GNOME_CANVAS_ITEM_VISIBLE;
}


/**
 * gnome_canvas_item_new:
 * @parent: The parent group for the new item.
 * @type: The object type of the item.
 * @first_arg_name: A list of object argument name/value pairs, NULL-terminated, used to configure
 * the item.  For example, "fill_color", "black", "width_units", 5.0, NULL.
 * 
 * Creates a new canvas item with @parent as its parent group.  The item is created at the top of
 * its parent's stack, and starts up as visible.  The item is of the specified @type, for example,
 * it can be gnome_canvas_rect_get_type().  The list of object arguments/value pairs is used to
 * configure the item.
 * 
 * Return value: The newly-created item.
 **/
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


/**
 * gnome_canvas_item_newv:
 * @parent: The parent group for the new item.
 * @type: The object type of the item.
 * @nargs: The number of arguments used to configure the item.
 * @args: The list of arguments used to configure the item.
 * 
 * Creates a new canvas item with @parent as its parent group.  The item is created at the top of
 * its parent's stack, and starts up as visible.  The item is of the specified @type, for example,
 * it can be gnome_canvas_rect_get_type().  The list of object arguments is used to configure the
 * item.
 * 
 * Return value: The newly-created item.
 **/
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


/**
 * gnome_canvas_item_construct:
 * @item: The item to construct.
 * @parent: The parent group for the item.
 * @first_arg_name: The name of the first argument for configuring the item.
 * @args: The list of arguments used to configure the item.
 * 
 * Constructs a canvas item; meant for use only by item implementations.
 **/
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
 

/**
 * gnome_canvas_item_constructv:
 * @item: The item to construct.
 * @parent: The parent group for the item.
 * @nargs: The number of arguments used to configure the item.
 * @args: The list of arguments used to configure the item.
 * 
 * Constructs a canvas item; meant for use only by item implementations.
 **/
void
gnome_canvas_item_constructv(GnomeCanvasItem *item, GnomeCanvasGroup *parent, guint nargs, GtkArg *args)
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

/* If the item is visible, requests a redraw of it. */
static void
redraw_if_visible (GnomeCanvasItem *item)
{
	if (item->object.flags & GNOME_CANVAS_ITEM_VISIBLE)
		gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
}

static void
gnome_canvas_item_shutdown (GtkObject *object)
{
	GnomeCanvasItem *item;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (object));

	item = GNOME_CANVAS_ITEM (object);

	redraw_if_visible (item);

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


/**
 * gnome_canvas_item_set:
 * @item: The item to configure.
 * @first_arg_name: The list of object argument name/value pairs used to configure the item.
 * 
 * Configures a canvas item.  The arguments in the item are set to the specified values,
 * and the item is repainted as appropriate.
 **/
void
gnome_canvas_item_set (GnomeCanvasItem *item, const gchar *first_arg_name, ...)
{
	va_list args;

	va_start (args, first_arg_name);
	gnome_canvas_item_set_valist (item, first_arg_name, args);
	va_end (args);
}


/**
 * gnome_canvas_item_set_valist:
 * @item: The item to configure.
 * @first_arg_name: The name of the first argument used to configure the item.
 * @args: The list of object argument name/value pairs used to configure the item.
 * 
 * Configures a canvas item.  The arguments in the item are set to the specified values,
 * and the item is repainted as appropriate.
 **/
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

		redraw_if_visible (item);

		object = GTK_OBJECT (item);

		for (arg = arg_list, info = info_list; arg; arg = arg->next, info = info->next)
			gtk_object_arg_set (object, arg->data, info->data);

		gtk_args_collect_cleanup (arg_list, info_list);

		redraw_if_visible (item);
		item->canvas->need_repick = TRUE;
	}
}


/**
 * gnome_canvas_item_setv:
 * @item: The item to configure.
 * @nargs: The number of arguments used to configure the item.
 * @args: The arguments used to configure the item.
 * 
 * Configures a canvas item.  The arguments in the item are set to the specified values,
 * and the item is repainted as appropriate.
 **/
void
gnome_canvas_item_setv (GnomeCanvasItem *item, guint nargs, GtkArg *args)
{
	redraw_if_visible (item);
	gtk_object_setv (GTK_OBJECT (item), nargs, args);
	redraw_if_visible (item);

	item->canvas->need_repick = TRUE;
}


/**
 * gnome_canvas_item_move:
 * @item: The item to move.
 * @dx: The horizontal distance.
 * @dy: The vertical distance.
 * 
 * Moves a canvas item by the specified distance, which must be specified in canvas units.
 **/
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

	redraw_if_visible (item);
	(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->translate) (item, dx, dy);
	redraw_if_visible (item);

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


/**
 * gnome_canvas_item_raise:
 * @item: The item to raise in its parent's stack.
 * @positions: The number of steps to raise the item.
 * 
 * Raises the item in its parent's stack by the specified number of positions.  If
 * the number of positions is greater than the distance to the top of the stack, then
 * the item is put at the top.
 **/
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

	redraw_if_visible (item);
	item->canvas->need_repick = TRUE;
}


/**
 * gnome_canvas_item_lower:
 * @item: The item to lower in its parent's stack.
 * @positions: The number of steps to lower the item.
 * 
 * Lowers the item in its parent's stack by the specified number of positions.  If
 * the number of positions is greater than the distance to the bottom of the stack, then
 * the item is put at the bottom.
 **/
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

	redraw_if_visible (item);
	item->canvas->need_repick = TRUE;
}


/**
 * gnome_canvas_item_raise_to_top:
 * @item: The item to raise in its parent's stack.
 * 
 * Raises an item to the top of its parent's stack.
 **/
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

	redraw_if_visible (item);
	item->canvas->need_repick = TRUE;
}


/**
 * gnome_canvas_item_lower_to_bottom:
 * @item: The item to lower in its parent's stack.
 * 
 * Lowers an item to the bottom of its parent's stack.
 **/
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

	redraw_if_visible (item);
	item->canvas->need_repick = TRUE;
}


/**
 * gnome_canvas_item_show:
 * @item: The item to show.
 * 
 * Shows an item.  If the item was already shown, then no action is taken.
 **/
void
gnome_canvas_item_show (GnomeCanvasItem *item)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));

	if (item->object.flags & GNOME_CANVAS_ITEM_VISIBLE)
		return;

	item->object.flags |= GNOME_CANVAS_ITEM_VISIBLE;

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	item->canvas->need_repick = TRUE;
}


/**
 * gnome_canvas_item_hide:
 * @item: The item to hide.
 * 
 * Hides an item.  If the item was already hidden, then no action is taken.
 **/
void
gnome_canvas_item_hide (GnomeCanvasItem *item)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));

	if (!(item->object.flags & GNOME_CANVAS_ITEM_VISIBLE))
		return;

	item->object.flags &= ~GNOME_CANVAS_ITEM_VISIBLE;

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	item->canvas->need_repick = TRUE;
}


/**
 * gnome_canvas_item_grab:
 * @item: The item to grab.
 * @event_mask: Mask of events that will be sent to this item.
 * @cursor: If non-NULL, the cursor that will be used while the grab is active.
 * @etime: The timestamp required for grabbing the mouse, or GDK_CURRENT_TIME.
 * 
 * Specifies that all events that match the specified event mask should be sent to the specified
 * item, and also grabs the mouse by calling gdk_pointer_grab().  The event mask is also used when
 * grabbing the pointer.  If @cursor is not NULL, then that cursor is used while the grab is active.
 * The @etime parameter is the timestamp required for grabbing the mouse.
 * 
 * Return value: If an item was already grabbed, it returns %AlreadyGrabbed.  If the specified
 * item is not visible, then it returns GrabNotViewable.  Else, it returns the result of
 * calling gdk_pointer_grab().
 **/
int
gnome_canvas_item_grab (GnomeCanvasItem *item, guint event_mask, GdkCursor *cursor, guint32 etime)
{
	int retval;

	g_return_val_if_fail (item != NULL, GrabNotViewable);
	g_return_val_if_fail (GNOME_IS_CANVAS_ITEM (item), GrabNotViewable);
	g_return_val_if_fail (GTK_WIDGET_MAPPED (item->canvas), GrabNotViewable);

	if (item->canvas->grabbed_item)
		return AlreadyGrabbed;

	if (!(item->object.flags & GNOME_CANVAS_ITEM_VISIBLE))
		return GrabNotViewable;

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


/**
 * gnome_canvas_item_ungrab:
 * @item: The item that was grabbed in the canvas.
 * @etime: The timestamp for ungrabbing the mouse.
 * 
 * Ungrabs the item, which must have been grabbed in the canvas, and ungrabs the mouse.
 **/
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


/**
 * gnome_canvas_item_w2i:
 * @item: The item whose coordinate system will be used for conversion.
 * @x: If non-NULL, the X world coordinate to convert to item-relative coordinates.
 * @y: If non-NULL, Y world coordinate to convert to item-relative coordinates.
 * 
 * Converts from world coordinates to item-relative coordinates.  The converted coordinates
 * are returned in the same variables.
 **/
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


/**
 * gnome_canvas_item_i2w:
 * @item: The item whose coordinate system will be used for conversion.
 * @x: If non-NULL, the X item-relative coordinate to convert to world coordinates.
 * @y: If non-NULL, the Y item-relative coordinate to convert to world coordinates.
 * 
 * Converts from item-relative coordinates to world coordinates.  The converted coordinates
 * are returned in the same variables.
 **/
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
 * gnome_canvas_item_reparent:
 * @item:      Item to reparent
 * @new_group: New group where the item is moved to
 *
 * This moves the item from its current group to the group specified
 * in NEW_GROUP.
 */

/**
 * gnome_canvas_item_reparent:
 * @item: The item whose parent should be changed.
 * @new_group: The new parent group.
 * 
 * Changes the parent of the specified item to be the new group.  The item keeps its group-relative
 * coordinates as for its old parent, so the item may change its absolute position within the
 * canvas.
 **/
void
gnome_canvas_item_reparent (GnomeCanvasItem *item, GnomeCanvasGroup *new_group)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));
	g_return_if_fail (new_group != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_GROUP (new_group));

	/* Both items need to be in the same canvas */
	g_return_if_fail (item->canvas == GNOME_CANVAS_ITEM (new_group)->canvas);

	/* The group cannot be an inferior of the item or be the item itself -- this also takes care
	 * of the case where the item is the root item of the canvas.
	 */
	g_return_if_fail (!is_descendant (GNOME_CANVAS_ITEM (new_group), item));

	/* Everything is ok, now actually reparent the item */

	gtk_object_ref (GTK_OBJECT (item)); /* protect it from the unref in group_remove */

	redraw_if_visible (item);

	group_remove (GNOME_CANVAS_GROUP (item->parent), item);
	item->parent = GNOME_CANVAS_ITEM (new_group);
	group_add (new_group, item);

	/* Rebuild the new parent group's bounds.  This will take care of reconfiguring the item and
	 * all its children.
	 */

	gnome_canvas_group_child_bounds (new_group, NULL);

	/* Redraw and repick */

	redraw_if_visible (item);
	item->canvas->need_repick = TRUE;

	gtk_object_unref (GTK_OBJECT (item));
}

/**
 * gnome_canvas_item_grab_focus:
 * @item: The item to which keyboard events should be sent.
 * 
 * Makes the specified item take the keyboard focus.  If the canvas itself did not have
 * the focus, it sets it as well.
 **/
void
gnome_canvas_item_grab_focus (GnomeCanvasItem *item)
{
	GnomeCanvasItem *focused_item;
	GdkEvent ev;

	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));
	g_return_if_fail (GTK_WIDGET_CAN_FOCUS (GTK_WIDGET (item->canvas)));

	focused_item = item->canvas->focused_item;

	if (focused_item) {
		ev.focus_change.type = GDK_FOCUS_CHANGE;
		ev.focus_change.window = GTK_LAYOUT (item->canvas)->bin_window;
		ev.focus_change.send_event = FALSE;
		ev.focus_change.in = FALSE;

		emit_event (item->canvas, &ev);
	}

	item->canvas->focused_item = item;
	gtk_widget_grab_focus (GTK_WIDGET (item->canvas));
}


/**
 * gnome_canvas_item_get_bounds:
 * @item: The item to query the bounding box for.
 * @x1: If non-NULL, returns the leftmost edge of the bounding box.
 * @y1: If non-NULL, returns the upper edge of the bounding box.
 * @x2: If non-NULL, returns the rightmost edge of the bounding box.
 * @y2: If non-NULL, returns the lower edge of the bounding box.
 * 
 * Queries the bounding box of the specified item.  The bounds are returned in item-relative
 * coordinates.
 **/
void
gnome_canvas_item_get_bounds (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	double tx1, ty1, tx2, ty2;

	g_return_if_fail (item != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));

	tx1 = ty1 = tx2 = ty2 = 0.0;

	if (GNOME_CANVAS_ITEM_CLASS (item->object.klass)->bounds)
		(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->bounds) (item, &tx1, &ty1, &tx2, &ty2);

	if (x1)
		*x1 = tx1;

	if (y1)
		*y1 = ty1;

	if (x2)
		*x2 = tx2;

	if (y2)
		*y2 = ty2;
}
  

/**
 * gnome_canvas_item_request_update
 * @item:
 *
 * Description: Request that the update method of the item gets called sometime before the next
 * render (generally from the idle loop).
 **/

void
gnome_canvas_item_request_update (GnomeCanvasItem *item)
{
	if (!(item->object.flags & GNOME_CANVAS_ITEM_NEED_UPDATE)) {
		item->object.flags |= GNOME_CANVAS_ITEM_NEED_UPDATE;
		if (item->parent != NULL) {
			/* Recurse up the tree */
			gnome_canvas_item_request_update (item->parent);
		} else {
			/* Have reached the top of the tree, make sure the update call gets scheduled. */
			gnome_canvas_request_update (item->canvas);
		}
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
static void   gnome_canvas_group_bounds      (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2);
static void   gnome_canvas_group_update      (GnomeCanvasItem *item);
static void   gnome_canvas_group_render      (GnomeCanvasItem *item,
					      GnomeCanvasBuf *buf);


static GnomeCanvasItemClass *group_parent_class;


/**
 * gnome_canvas_group_get_type:
 *
 * Registers the &GnomeCanvasGroup class if necessary, and returns the type ID associated to it.
 * 
 * Return value:  The type ID of the &GnomeCanvasGroup class.
 **/
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
	item_class->bounds = gnome_canvas_group_bounds;
	item_class->update = gnome_canvas_group_update;
	item_class->render = gnome_canvas_group_render;
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
	GnomeCanvasItem *child = 0;

	group = GNOME_CANVAS_GROUP (item);

	for (list = group->item_list; list; list = list->next) {
		child = list->data;

		if (((child->object.flags & GNOME_CANVAS_ITEM_VISIBLE)
		     && ((child->x1 < (x + width))
			 && (child->y1 < (y + height))
			 && (child->x2 > x)
			 && (child->y2 > y)))
		    || ((GTK_OBJECT_FLAGS (child) & GNOME_CANVAS_ITEM_ALWAYS_REDRAW)
			&& (child->x1 < child->canvas->redraw_x2)
			&& (child->y1 < child->canvas->redraw_y2)
			&& (child->x2 > child->canvas->redraw_x1)
			&& (child->y2 > child->canvas->redraw_y2)))
			if (GNOME_CANVAS_ITEM_CLASS (child->object.klass)->draw)
				(* GNOME_CANVAS_ITEM_CLASS (child->object.klass)->draw) (child, drawable, x, y, width, height);
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

		if ((child->object.flags & GNOME_CANVAS_ITEM_VISIBLE)
		    && GNOME_CANVAS_ITEM_CLASS (child->object.klass)->point) {
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
gnome_canvas_group_bounds (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	GnomeCanvasGroup *group;
	GnomeCanvasItem *child;
	GList *list;
	double tx1, ty1, tx2, ty2;
	double minx, miny, maxx, maxy;
	int set;

	group = GNOME_CANVAS_GROUP (item);

	/* Get the bounds of the first visible item */

	child = NULL; /* Unnecessary but eliminates a warning. */

	set = FALSE;

	for (list = group->item_list; list; list = list->next) {
		child = list->data;

		if (child->object.flags & GNOME_CANVAS_ITEM_VISIBLE) {
			set = TRUE;
			gnome_canvas_item_get_bounds (child, &minx, &miny, &maxx, &maxy);
			break;
		}
	}

	/* If there were no visible items, return the group's origin */

	if (!set) {
		*x1 = *x2 = group->xpos;
		*y1 = *y2 = group->ypos;
		return;
	}

	/* Now we can grow the bounds using the rest of the items */

	list = list->next;

	for (; list; list = list->next) {
		if (!(child->object.flags & GNOME_CANVAS_ITEM_VISIBLE))
			continue;

		gnome_canvas_item_get_bounds (list->data, &tx1, &ty1, &tx2, &ty2);

		if (tx1 < minx)
			minx = tx1;

		if (ty1 < miny)
			miny = ty1;

		if (tx2 > maxx)
			maxx = tx2;

		if (ty2 > maxy)
			maxy = ty2;
	}

	/* Make the bounds be relative to our parent's coordinate system */

	if (item->parent) {
		minx += group->xpos;
		miny += group->ypos;
		maxx += group->xpos;
		maxy += group->ypos;
	}

	*x1 = minx;
	*y1 = miny;
	*x2 = maxx;
	*y2 = maxy;
}

static void
gnome_canvas_group_update (GnomeCanvasItem *item)
{
	GnomeCanvasGroup *group;
	GnomeCanvasItem *child;
	GList *list;

	group = GNOME_CANVAS_GROUP (item);

	for (list = group->item_list; list; list = list->next) {
		child = list->data;

		if (child->object.flags & GNOME_CANVAS_ITEM_NEED_UPDATE) {
			(* GNOME_CANVAS_ITEM_CLASS (child->object.klass)->update) (child);
			child->object.flags &= ~GNOME_CANVAS_ITEM_NEED_UPDATE;
		}
	}
}

static void
gnome_canvas_group_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf)
{
	GnomeCanvasGroup *group;
	GnomeCanvasItem *child;
	GList *list;

	group = GNOME_CANVAS_GROUP (item);

	for (list = group->item_list; list; list = list->next) {
		child = list->data;

		if (((child->object.flags & GNOME_CANVAS_ITEM_VISIBLE)
		     && ((child->x1 < buf->rect.x1)
			 && (child->y1 < buf->rect.y1)
			 && (child->x2 > buf->rect.x0)
			 && (child->y2 > buf->rect.y0)))
		    || ((GTK_OBJECT_FLAGS (child) & GNOME_CANVAS_ITEM_ALWAYS_REDRAW)
			&& (child->x1 < child->canvas->redraw_x2)
			&& (child->y1 < child->canvas->redraw_y2)
			&& (child->x2 > child->canvas->redraw_x1)
			&& (child->y2 > child->canvas->redraw_y2)))
			if (GNOME_CANVAS_ITEM_CLASS (child->object.klass)->render)
				(* GNOME_CANVAS_ITEM_CLASS (child->object.klass)->render) (child, buf);
	}
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


/**
 * gnome_canvas_group_child_bounds:
 * @group: The group to notify about a child's bounds changes.
 * @item: If non-NULL, the item whose bounds changed.  Otherwise, specifies that the whole
 * group's bounding box and its subgroups' should be recomputed recursively.
 * 
 * For use only by item implementations.  If the specified @item is non-NULL, then it notifies
 * the group that the item's bounding box has changed, and thus the group should be adjusted
 * accordingly.  If the specified @item is NULL, then the group's bounding box and its
 * subgroup's will be recomputed recursively.
 **/
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



/**
 * gnome_canvas_get_type:
 *
 * Registers the &GnomeCanvas class if necessary, and returns the type ID associated to it.
 * 
 * Return value:  The type ID of the &GnomeCanvas class.
 **/
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
	GTK_WIDGET_SET_FLAGS (canvas, GTK_CAN_FOCUS);

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

	gdk_color_context_free (canvas->cc);

	if (GTK_OBJECT_CLASS (canvas_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (canvas_parent_class)->destroy) (object);
}


/**
 * gnome_canvas_new:
 *
 * Creates a new empty canvas.  If the user wishes to use the image item inside this canvas, then
 * the gdk_imlib visual and colormap should be pushed into Gtk+'s stack before calling this
 * function, and they can be popped afterwards.
 * 
 * Return value: The newly-created canvas.
 **/
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

static int
emit_event (GnomeCanvas *canvas, GdkEvent *event)
{
	GdkEvent ev;
	gint finished;
	GnomeCanvasItem *item;
	guint mask;

	/* Perform checks for grabbed items */

	if (canvas->grabbed_item && !is_descendant (canvas->current_item, canvas->grabbed_item))
		return FALSE;

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
			return FALSE;
	}

	/* Convert to world coordinates -- we have two cases because of diferent offsets of the
	 * fields in the event structures.
	 */

	ev = *event;

	switch (ev.type) {
	case GDK_ENTER_NOTIFY:
	case GDK_LEAVE_NOTIFY:
		gnome_canvas_window_to_world (canvas, ev.crossing.x, ev.crossing.y, &ev.crossing.x, &ev.crossing.y);
		break;

	case GDK_MOTION_NOTIFY:
	case GDK_BUTTON_PRESS:
	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
	case GDK_BUTTON_RELEASE:
		gnome_canvas_window_to_world (canvas, ev.motion.x, ev.motion.y, &ev.motion.x, &ev.motion.y);
		break;

	default:
		break;
	}

	/* Choose where we send the event */

	item = canvas->current_item;
	
	if (canvas->focused_item && ((event->type == GDK_KEY_PRESS) || (event->type == GDK_KEY_RELEASE)))
		item = canvas->focused_item;

	/* The event is propagated up the hierarchy (for if someone connected to a group instead of
	 * a leaf event), and emission is stopped if a handler returns TRUE, just like for GtkWidget
	 * events.
	 */
	for (finished = FALSE; item && !finished; item = item->parent)
		gtk_signal_emit (GTK_OBJECT (item), item_signals[ITEM_EVENT],
				 &ev,
				 &finished);

	return finished;
}

static int
pick_current_item (GnomeCanvas *canvas, GdkEvent *event)
{
	int button_down;
	double x, y;
	int cx, cy;
	int retval;

	retval = FALSE;

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
		return retval;

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

		if (canvas->root->object.flags & GNOME_CANVAS_ITEM_VISIBLE)
			(* GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->point) (canvas->root, x, y, cx, cy,
											 &canvas->new_current_item);
		else
			canvas->new_current_item = NULL;
	} else
		canvas->new_current_item = NULL;

	if ((canvas->new_current_item == canvas->current_item) && !canvas->left_grabbed_item)
		return retval; /* current item did not change */

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
		retval = emit_event (canvas, &new_event);
		canvas->in_repick = FALSE;
	}

	/* new_current_item may have been set to NULL during the call to emit_event() above */

	if ((canvas->new_current_item != canvas->current_item) && button_down) {
		canvas->left_grabbed_item = TRUE;
		return retval;
	}

	/* Handle the rest of cases */

	canvas->left_grabbed_item = FALSE;
	canvas->current_item = canvas->new_current_item;

	if (canvas->current_item != NULL) {
		GdkEvent new_event;

		new_event = canvas->pick_event;
		new_event.type = GDK_ENTER_NOTIFY;
		new_event.crossing.detail = GDK_NOTIFY_ANCESTOR;
		retval = emit_event (canvas, &new_event);
	}

	return retval;
}

static gint
gnome_canvas_button (GtkWidget *widget, GdkEventButton *event)
{
	GnomeCanvas *canvas;
	int mask;
	int retval;

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_CANVAS (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	retval = FALSE;

	canvas = GNOME_CANVAS (widget);

	if (event->window != canvas->layout.bin_window)
		return retval;

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
		retval = emit_event (canvas, (GdkEvent *) event);
		break;

	case GDK_BUTTON_RELEASE:
		/*
		 * Process the event as if the button were pressed, then repick after the button has
		 * been released
		 */
		
		canvas->state = event->state;
		retval = emit_event (canvas, (GdkEvent *) event);
		event->state ^= mask;
		canvas->state = event->state;
		pick_current_item (canvas, (GdkEvent *) event);
		event->state ^= mask;
		break;

	default:
		g_assert_not_reached ();
	}

	return retval;
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
	return emit_event (canvas, (GdkEvent *) event);
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

	return emit_event (canvas, (GdkEvent *) event);
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
	return pick_current_item (canvas, (GdkEvent *) event);
}

static gint
gnome_canvas_focus_in (GtkWidget *widget, GdkEventFocus *event)
{
	GnomeCanvas *canvas;
	
	canvas = GNOME_CANVAS (widget);

	if (canvas->focused_item)
		return emit_event (canvas, (GdkEvent *) event);
	else
		return FALSE;
}

static gint
gnome_canvas_focus_out (GtkWidget *widget, GdkEventFocus *event)
{
	GnomeCanvas *canvas;
	
	canvas = GNOME_CANVAS (widget);

	if (canvas->focused_item)
		return emit_event (canvas, (GdkEvent *) event);
	else
		return FALSE;
}

#define IMAGE_WIDTH 512
#define IMAGE_HEIGHT 512

#define IMAGE_WIDTH_AA 256
#define IMAGE_HEIGHT_AA 64

static void
paint (GnomeCanvas *canvas)
{
	GtkWidget *widget;
	int draw_x1, draw_y1;
	int draw_x2, draw_y2;
	int width, height;
	GdkPixmap *pixmap;
	ArtIRect *rects;
	gint n_rects, i;

	if (canvas->need_update) {
		(* GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->update) (canvas->root);

		canvas->root->object.flags &= ~GNOME_CANVAS_ITEM_NEED_UPDATE;
		canvas->need_update = FALSE;
	}

	if (!canvas->need_redraw)
		return;

	if (canvas->aa)
		rects = art_rect_list_from_uta (canvas->redraw_area, IMAGE_WIDTH_AA, IMAGE_HEIGHT_AA, &n_rects);
	else
		rects = art_rect_list_from_uta (canvas->redraw_area, IMAGE_WIDTH, IMAGE_HEIGHT, &n_rects);

	art_uta_free (canvas->redraw_area);

	canvas->redraw_area = NULL;

	widget = GTK_WIDGET (canvas);

	for (i = 0; i < n_rects; i++) {
		canvas->redraw_x1 = rects[i].x0;
		canvas->redraw_y1 = rects[i].y0;
		canvas->redraw_x2 = rects[i].x1;
		canvas->redraw_y2 = rects[i].y1;

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

		if ((draw_x1 < draw_x2) && (draw_y1 < draw_y2)) {

			canvas->draw_xofs = draw_x1;
			canvas->draw_yofs = draw_y1;

			width = draw_x2 - draw_x1;
			height = draw_y2 - draw_y1;

			if (canvas->aa) {
				GnomeCanvasBuf buf;
				buf.buf = g_new (guchar, IMAGE_WIDTH_AA * IMAGE_HEIGHT_AA * 3);
				buf.buf_rowstride = IMAGE_WIDTH_AA * 3;
				buf.rect.x0 = draw_x1;
				buf.rect.y0 = draw_y1;
				buf.rect.x1 = draw_x2;
				buf.rect.y1 = draw_y2;
				buf.bg_color = 0xffffff; /* FIXME; should be the same as the style's background color */
				buf.is_bg = 1;
				buf.is_buf = 0;

				if (canvas->root->object.flags & GNOME_CANVAS_ITEM_VISIBLE) {
					if ((* GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->render) != NULL)
						(* GNOME_CANVAS_ITEM_CLASS (canvas->root->object.klass)->render) (canvas->root, &buf);
					/* else warning? */
				}

				if (buf.is_bg) {
					gdk_rgb_gc_set_foreground (canvas->pixmap_gc, buf.bg_color);
					gdk_draw_rectangle (canvas->layout.bin_window,
							    canvas->pixmap_gc,
							    TRUE,
							    0, 0,
							    width, height);
				} else {
					gdk_draw_rgb_image (canvas->layout.bin_window,
							    canvas->pixmap_gc,
							    draw_x1 - DISPLAY_X1 (canvas) + canvas->zoom_xofs,
							    draw_y1 - DISPLAY_Y1 (canvas) + canvas->zoom_yofs,
							    width, height,
							    GDK_RGB_DITHER_NONE,
							    buf.buf,
							    IMAGE_WIDTH_AA * 3);
				}
				canvas->draw_xofs = draw_x1;
				canvas->draw_yofs = draw_y1;
				g_free (buf.buf);
			
			} else {
				pixmap = gdk_pixmap_new (canvas->layout.bin_window, width, height, gtk_widget_get_visual (widget)->depth);

				gdk_gc_set_foreground (canvas->pixmap_gc, &widget->style->bg[GTK_STATE_NORMAL]);
				gdk_draw_rectangle (pixmap,
						    canvas->pixmap_gc,
						    TRUE,
						    0, 0,
						    width, height);

				/* Draw the items that intersect the area */

				if (canvas->root->object.flags & GNOME_CANVAS_ITEM_VISIBLE)
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
	  	}
	}

	art_free (rects);

	canvas->need_redraw = FALSE;
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

	return FALSE;
}


/**
 * gnome_canvas_root:
 * @canvas: The canvas whose root group should be extracted.
 * 
 * Queries the root group of a canvas.
 * 
 * Return value: The root group of the canvas.
 **/
GnomeCanvasGroup *
gnome_canvas_root (GnomeCanvas *canvas)
{
	g_return_val_if_fail (canvas != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS (canvas), NULL);

	return GNOME_CANVAS_GROUP (canvas->root);
}


/**
 * gnome_canvas_set_scroll_region:
 * @canvas: The canvas to set the scroll region for.
 * @x1: Leftmost limit of the scrolling region.
 * @y1: Upper limit of the scrolling region.
 * @x2: Rightmost limit of the scrolling region.
 * @y2: Lower limit of the scrolling region.
 * 
 * Sets the scrolling region of the canvas to the specified rectangle.  The canvas
 * will then be able to scroll only within this region.  The view of the canvas is
 * adjusted as appropriate to display as much of the new region as possible.
 **/
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


/**
 * gnome_canvas_get_scroll_region:
 * @canvas: The canvas whose scroll region should be queried.
 * @x1: If non-NULL, returns the leftmost limit of the scrolling region.
 * @y1: If non-NULL, returns the upper limit of the scrolling region.
 * @x2: If non-NULL, returns the rightmost limit of the scrolling region.
 * @y2: If non-NULL, returns the lower limit of the scrolling region.
 * 
 * Queries the scroll region of the canvas.
 **/
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


/**
 * gnome_canvas_set_pixels_per_unit:
 * @canvas: The canvas whose zoom factor should be changed.
 * @n: The number of pixels that correspond to one canvas unit.
 * 
 * Sets the zooming factor of the canvas by specifying the number of pixels that correspond to
 * one canvas unit.
 **/
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


/**
 * gnome_canvas_set_size:
 * @canvas: The canvas to set the window size for.
 * @width: Width of the canvas window.
 * @height: Height of the canvas window.
 * 
 * Sets the size of the canvas window.  Typically only called before the canvas is shown for
 * the first time.
 **/
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


/**
 * gnome_canvas_scroll_to:
 * @canvas: The canvas to scroll.
 * @cx: Horizontal scrolling offset in canvas pixel units.
 * @cy: Vertical scrolling offset in canvas pixel units.
 * 
 * Makes the canvas scroll to the specified offsets, given in canvas pixel units.  The canvas
 * will adjust the view so that it is not outside the scrolling region.  This function is typically
 * not used, as it is better to hook scrollbars to the canvas layout's scrolling adjusments.
 **/
void
gnome_canvas_scroll_to (GnomeCanvas *canvas, int cx, int cy)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	scroll_to (canvas, cx, cy);
}


/**
 * gnome_canvas_get_scroll_offsets:
 * @canvas: The canvas whose scrolling offsets should be queried.
 * @cx: If non-NULL, returns the horizontal scrolling offset.
 * @cy: If non-NULL, returns the vertical scrolling offset.
 * 
 * Queries the scrolling offsets of the canvas.
 **/
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


/**
 * gnome_canvas_update_now:
 * @canvas: The canvas whose view should be updated.
 * 
 * Forces an immediate redraw or update of the canvas.  If the canvas does not have
 * any pending redraw requests, then no action is taken.  This is typically only used
 * by applications that need explicit control of when the display is updated, like games.
 * It is not needed by normal applications.
 **/
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


/* Invariant: the idle handler is always present when need_update or need_redraw. */

/**
 * gnome_canvas_request_update
 * @canvas:
 *
 * Description: Schedules an update () invocation for the next idle loop.
 **/

static void
gnome_canvas_request_update (GnomeCanvas *canvas)
{
	if (!(canvas->need_update || canvas->need_redraw)) {
		canvas->idle_id = gtk_idle_add (idle_handler, canvas);
	}
	canvas->need_update = TRUE;
}

/**
 * gnome_canvas_request_redraw_uta:
 * @canvas: The canvas whose area needs to be redrawn.
 * @uta: Microtile array that specifies the area to be redrawn.
 * 
 * Informs a canvas that the specified area, given as a microtile array, needs to be repainted.
 **/
void
gnome_canvas_request_redraw_uta (GnomeCanvas *canvas,
                                 ArtUta *uta)
{
	ArtUta *uta2;

	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	if (canvas->need_redraw) {
		uta2 = art_uta_union (uta, canvas->redraw_area);
		art_uta_free (uta);
		art_uta_free (canvas->redraw_area);
		canvas->redraw_area = uta2;
	} else {
		canvas->redraw_area = uta;
		canvas->need_redraw = TRUE;
		canvas->idle_id = gtk_idle_add (idle_handler, canvas);
	}
}


/**
 * gnome_canvas_request_redraw:
 * @canvas: The canvas whose area needs to be redrawn.
 * @x1: Leftmost coordinate of the rectangle to be redrawn.
 * @y1: Upper coordinate of the rectangle to be redrawn.
 * @x2: Rightmost coordinate of the rectangle to be redrawn, plus 1.
 * @y2: Lower coordinate of the rectangle to be redrawn, plus 1.
 * 
 * Convenience function that informs a canvas that the specified area, specified as a
 * rectangle, needs to be repainted.  This function converts the rectangle to a microtile
 * array and feeds it to gnome_canvas_request_redraw_uta().  The rectangle includes
 * @x1 and @y1, but not @x2 and @y2.
 **/
void
gnome_canvas_request_redraw (GnomeCanvas *canvas, int x1, int y1, int x2, int y2)
{
	ArtUta *uta;
	ArtIRect bbox;
	ArtIRect visible;
	ArtIRect clip;

	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	if (!GTK_WIDGET_DRAWABLE (canvas) || (x1 == x2) || (y1 == y2))
		return;

	bbox.x0 = x1;
	bbox.y0 = y1;
	bbox.x1 = x2;
	bbox.y1 = y2;

	visible.x0 = DISPLAY_X1 (canvas) - canvas->zoom_xofs;
	visible.y0 = DISPLAY_Y1 (canvas) - canvas->zoom_yofs;
	visible.x1 = visible.x0 + canvas->width;
	visible.y1 = visible.y0 + canvas->height;

	art_irect_intersect (&clip, &bbox, &visible);

	if (!art_irect_empty (&clip)) {
		uta = art_uta_from_irect (&clip);
		gnome_canvas_request_redraw_uta (canvas, uta);
	}
}


/**
 * gnome_canvas_w2c:
 * @canvas: The canvas whose coordinates need conversion.
 * @wx: World X coordinate.
 * @wy: World Y coordinate.
 * @cx: If non-NULL, returns the converted X pixel coordinate.
 * @cy: If non-NULL, returns the converted Y pixel coordinate.
 * 
 * Converts world coordinates into canvas pixel coordinates.  Usually only needed
 * by item implementations.
 **/
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

/**
 * gnome_canvas_w2c_d:
 * @canvas: The canvas whose coordinates need conversion.
 * @wx: World X coordinate.
 * @wy: World Y coordinate.
 * @cx: If non-NULL, returns the converted X pixel coordinate.
 * @cy: If non-NULL, returns the converted Y pixel coordinate.
 * 
 * Converts world coordinates into canvas pixel coordinates.  Usually only needed
 * by item implementations. This version results in double coordinates, which
 * are useful in antialiased implementations.
 **/
void
gnome_canvas_w2c_d (GnomeCanvas *canvas, double wx, double wy, double *cx, double *cy)
{
	double x, y;

	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	if (cx)
		*cx = (wx - canvas->scroll_x1) * canvas->pixels_per_unit;

	if (cy)
		*cy = (wy - canvas->scroll_y1) * canvas->pixels_per_unit;
}


/**
 * gnome_canvas_c2w:
 * @canvas: The canvas whose coordinates need conversion.
 * @cx: Canvas pixel X coordinate.
 * @cy: Canvas pixel Y coordinate.
 * @wx: If non-NULL, returns the converted X world coordinate.
 * @wy: If non-NULL, returns the converted Y world coordinate.
 * 
 * Converts canvas pixel coordinates to world coordinates.  Usually only needed
 * by item implementations.
 **/
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


/**
 * gnome_canvas_window_to_world:
 * @canvas: The canvas whose coordinates need conversion.
 * @winx: Window-relative X coordinate.
 * @winy: Window-relative Y coordinate.
 * @worldx: If non-NULL, returns the converted X world coordinate.
 * @worldy: If non-NULL, returns the converted Y world coordinate.
 * 
 * Converts window-relative coordinates into world coordinates.  Use this when you need to
 * convert from mouse coordinates into world coordinates, for example.
 **/
void
gnome_canvas_window_to_world (GnomeCanvas *canvas, double winx, double winy, double *worldx, double *worldy)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	if (worldx)
		*worldx = canvas->scroll_x1 + (winx + DISPLAY_X1 (canvas) - canvas->zoom_xofs) / canvas->pixels_per_unit;

	if (worldy)
		*worldy = canvas->scroll_y1 + (winy + DISPLAY_Y1 (canvas) - canvas->zoom_yofs) / canvas->pixels_per_unit;
}


/**
 * gnome_canvas_world_to_window:
 * @canvas: The canvas whose coordinates need conversion.
 * @worldx: World X coordinate.
 * @worldy: World Y coordinate.
 * @winx: If non-NULL, returns the converted X window-relative coordinate.
 * @winy: If non-NULL, returns the converted Y window-relative coordinate.
 * 
 * Converts world coordinates into window-relative coordinates.
 **/
void
gnome_canvas_world_to_window (GnomeCanvas *canvas, double worldx, double worldy, double *winx, double *winy)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	if (winx)
		*winx = (canvas->pixels_per_unit)*(worldx - canvas->scroll_x1) -
		  DISPLAY_X1(canvas) + canvas->zoom_xofs;

	if (winy)
		*winy = (canvas->pixels_per_unit)*(worldy - canvas->scroll_y1) -
		  DISPLAY_Y1(canvas) + canvas->zoom_yofs;
}



/**
 * gnome_canvas_get_color:
 * @canvas: The canvas in which to allocate the color.
 * @spec: X color specification, or NULL for "transparent".
 * @color: Returns the allocated color.
 * 
 * Allocates a color based on the specified X color specification.  As a convenience to item
 * implementations, it returns TRUE if the color was allocated, or FALSE if the specification
 * was NULL.  A NULL color specification is considered as "transparent" by the canvas.
 * 
 * Return value: TRUE if @spec is non-NULL and the color is allocated.  If @spec is NULL,
 * then returns FALSE.
 **/
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


/**
 * gnome_canvas_set_stipple_origin:
 * @canvas: The canvas relative to which the stipple origin should be set.
 * @gc: GC on which to set the stipple origin.
 * 
 * Sets the stipple origin of the specified GC as is appropriate for the canvas.  This is
 * typically only needed by item implementations.
 **/
void
gnome_canvas_set_stipple_origin (GnomeCanvas *canvas, GdkGC *gc)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));
	g_return_if_fail (gc != NULL);

	gdk_gc_set_ts_origin (gc, -canvas->draw_xofs, -canvas->draw_yofs);
}
