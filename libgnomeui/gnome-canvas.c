/* GnomeCanvas widget - Tk-like canvas widget for Gnome
 *
 * This widget is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <math.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include "gnome-canvas.h"


/* Standard tags and keys */

GnomeCanvasKey _gnome_canvas_current       = "current";
GnomeCanvasKey _gnome_canvas_all           = "all";

GnomeCanvasKey _gnome_canvas_end           = "end";
GnomeCanvasKey _gnome_canvas_x             = "x";
GnomeCanvasKey _gnome_canvas_y             = "y";
GnomeCanvasKey _gnome_canvas_x1            = "x1";
GnomeCanvasKey _gnome_canvas_y1            = "y1";
GnomeCanvasKey _gnome_canvas_x2            = "x2";
GnomeCanvasKey _gnome_canvas_y2            = "y2";
GnomeCanvasKey _gnome_canvas_fill_color    = "fill_color";
GnomeCanvasKey _gnome_canvas_outline_color = "outline_color";
GnomeCanvasKey _gnome_canvas_width_pixels  = "width_pixels";
GnomeCanvasKey _gnome_canvas_width_units   = "width_units";


typedef void (*ForeachFunc) (GnomeCanvas *canvas, GnomeCanvasItem *item, va_list args, gpointer data);


/* Binding structure */
typedef struct {
	GdkEventType type;
	int id;
	GnomeCanvasId cid;
	gpointer user_data;
} Binding;


/* All registered item types */
static GHashTable *item_types;


/* Signal stuff */

enum {
	ITEM_EVENT,
	LAST_SIGNAL
};

typedef gint (*GnomeCanvasSignal1) (GtkObject *object,
				    gpointer   item,      /* shit, we can't pass the structure by value */
				    GdkEvent  *event,
				    gpointer   user_data, /* data from the item binding */
				    gpointer   data);     /* data from the Gtk signal */

static void gnome_canvas_marshal_signal_1 (GtkObject     *object,
					   GtkSignalFunc  func,
					   gpointer       func_data,
					   GtkArg        *args);


/* Widget stuff declarations */

static void gnome_canvas_class_init     (GnomeCanvasClass *class);
static void gnome_canvas_init           (GnomeCanvas      *canvas);
static void gnome_canvas_destroy        (GtkObject        *object);
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


static GtkContainerClass *parent_class;

static guint canvas_signals[LAST_SIGNAL] = { 0 };


void
gnome_canvas_register_type (GnomeCanvasItemType *type)
{
	g_return_if_fail (type != NULL);

	if (!item_types)
		item_types = g_hash_table_new (g_str_hash, g_str_equal);

	g_hash_table_insert (item_types, type->type, type);
}

void
gnome_canvas_uninit (void)
{
	if (!item_types)
		return;

	g_hash_table_destroy (item_types);

	item_types = NULL;
}

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

extern void gnome_canvas_register_rect_ellipse (void);

static void
gnome_canvas_class_init (GnomeCanvasClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	GtkContainerClass *container_class;

	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;
	container_class = (GtkContainerClass *) class;

	parent_class = gtk_type_class (gtk_container_get_type ());

	canvas_signals[ITEM_EVENT] =
		gtk_signal_new ("item_event",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeCanvasClass, item_event),
				gnome_canvas_marshal_signal_1,
				GTK_TYPE_BOOL, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_GDK_EVENT,
				GTK_TYPE_POINTER);

	gtk_object_class_add_signals (object_class, canvas_signals, LAST_SIGNAL);

	object_class->destroy = gnome_canvas_destroy;

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

	class->item_event = NULL;

	/* Register the default canvas item types */

	gnome_canvas_register_rect_ellipse ();
}

static int
has_tag (GnomeCanvasItem *item, GnomeCanvasKey tag)
{
	return (g_hash_table_lookup (item->tags, (gpointer) tag) != NULL);
}

static guint
binding_hash (gpointer key)
{
	return GPOINTER_TO_INT (key);
}

static gint
binding_compare (gpointer a, gpointer b)
{
	return GPOINTER_TO_INT (a) - GPOINTER_TO_INT (b);
}

static void
init_bindings (GnomeCanvas *canvas)
{
	canvas->bindings = g_hash_table_new (binding_hash, binding_compare);
	canvas->binding_id = 0;
}

static void
destroy_binding (gpointer data, gpointer user_data)
{
	g_free (data);
}

static void
destroy_binding_list (gpointer key, gpointer value, gpointer user_data)
{
	g_list_foreach (value, destroy_binding, user_data);
	g_list_free (value);
}

static void
destroy_bindings (GnomeCanvas *canvas)
{
	g_hash_table_foreach (canvas->bindings, destroy_binding_list, NULL);
	g_hash_table_destroy (canvas->bindings);
	canvas->bindings = NULL;
}

static gint
binding_order (gpointer a, gpointer b)
{
	Binding *ba, *bb;

	ba = a;
	bb = b;

	/* The "all" tag comes first.  Then come all other tags.  Then come individual items.
	 * Bindings of the same category are inserted sequentially at the end of their sub-list.
	 */

	if (ba->cid.type != bb->cid.type) {
		if ((ba->cid.type == GNOME_CANVAS_ID_TAG) && (bb->cid.type == GNOME_CANVAS_ID_ITEM))
			return -1;
		else
			return 1;
	}

	if (ba->cid.type == GNOME_CANVAS_ID_TAG) {
		if ((ba->cid.data == GNOME_CANVAS_ALL) && (bb->cid.data == GNOME_CANVAS_ALL))
			return 1;
		else
			return -1;
	} else
		return 1;
}

static int
add_binding (GnomeCanvas *canvas, GdkEventType type, GnomeCanvasId cid, gpointer user_data)
{
	Binding *binding;
	GList *list;
	gpointer key;

	binding = g_new (Binding, 1);
	binding->type = type;
	binding->id = canvas->binding_id++;
	binding->cid = cid;
	binding->user_data = user_data;

	key = GINT_TO_POINTER ((int) type);

	list = g_hash_table_lookup (canvas->bindings, key);
	list = g_list_insert_sorted (list, binding, binding_order);

	g_hash_table_insert (canvas->bindings, key, list);

	return binding->id;
}

static GList *
get_bindings_for_event (GnomeCanvas *canvas, GdkEventType type)
{
	return g_hash_table_lookup (canvas->bindings, GINT_TO_POINTER ((int) type));
}

static void
execute_bindings (GnomeCanvas *canvas, GdkEvent *event)
{
	GList *list;
	int emit;
	Binding *b;
	gint finished;

	if (!canvas->current_item)
		return;

	printf ("aqui\n");

	list = get_bindings_for_event (canvas, event->type);

	for (finished = FALSE; list && !finished; list = list->next) {
		b = list->data;

		emit = FALSE;

		switch (b->cid.type) {
		case GNOME_CANVAS_ID_TAG:
			if (has_tag (canvas->current_item, b->cid.data))
				emit = TRUE;
			break;

		case GNOME_CANVAS_ID_ITEM:
			if (canvas->current_item == b->cid.data)
				emit = TRUE;
			break;

		default:
			break;
		}

		if (emit) {
			printf ("emitiendo\n");
			gtk_signal_emit (GTK_OBJECT (canvas), canvas_signals[ITEM_EVENT],
					 &b->cid,
					 event,
					 b->user_data,
					 &finished);
		}
	}
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

	init_bindings (canvas);
}

static void
gnome_canvas_destroy (GtkObject *object)
{
	GnomeCanvas *canvas;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (object));

	canvas = GNOME_CANVAS (object);

	/* FIXME */

	destroy_bindings (canvas);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
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

	canvas->cc = gdk_color_context_new (canvas->visual, canvas->colormap);

	return GTK_WIDGET (canvas);
}

static void
gnome_canvas_unmap (GtkWidget *widget)
{
	GnomeCanvas *canvas;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (widget));

	canvas = GNOME_CANVAS (widget);

	if (canvas->flags & GNOME_CANVAS_NEED_REDRAW) {
		canvas->flags &= ~GNOME_CANVAS_NEED_REDRAW;
		canvas->redraw_x1 = 0;
		canvas->redraw_y1 = 0;
		canvas->redraw_x2 = 0;
		canvas->redraw_y2 = 0;
		gtk_idle_remove (canvas->idle_id);
	}

	if (GTK_WIDGET_CLASS (parent_class)->unmap)
		(* GTK_WIDGET_CLASS (parent_class)->unmap) (widget);
}

static void
gnome_canvas_realize (GtkWidget *widget)
{
	GnomeCanvas *canvas;
	GdkWindowAttr attributes;
	gint attributes_mask;
	int border_width;
	GdkColor c;
	GList *list;
	GnomeCanvasItem *item;

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

	if (canvas->flags & GNOME_CANVAS_BG_SET) {
		c.pixel = canvas->bg_pixel;
		gdk_window_set_background (widget->window, &c);
	} else
		gdk_window_set_background (widget->window, &widget->style->bg[GTK_STATE_NORMAL]);

	canvas->pixmap_gc = gdk_gc_new (widget->window);

	/* Realize items */

	for (list = canvas->item_list; list; list = list->next) {
		item = list->data;

		(* item->type->realize) (canvas, item);
	}
}

static void
gnome_canvas_unrealize (GtkWidget *widget)
{
	GnomeCanvas *canvas;
	GList *list;
	GnomeCanvasItem *item;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (widget));

	canvas = GNOME_CANVAS (widget);

	/* Unrealize items */

	for (list = canvas->item_list; list; list = list->next) {
		item = list->data;

		(* item->type->unrealize) (canvas, item);
	}

	gdk_gc_destroy (canvas->pixmap_gc);
	canvas->pixmap_gc = NULL;

	if (GTK_WIDGET_CLASS (parent_class)->unrealize)
		(* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
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

static GnomeCanvasItem *
find_closest_item (GnomeCanvas *canvas, int x, int y)
{
	GList *list;
	GnomeCanvasItem *item;
	GnomeCanvasItem *best;
	int x1, y1, x2, y2;
	double dx, dy;
	double dist;

	x1 = x - canvas->close_enough;
	y1 = y - canvas->close_enough;
	x2 = x + canvas->close_enough;
	y2 = y + canvas->close_enough;

	best = NULL;

	for (list = canvas->item_list; list; list = list->next) {
		item = list->data;

		if ((item->x1 > x2) || (item->y1 > y2) || (item->x2 < x1) || (item->y2 < y1))
			continue;

		gnome_canvas_c2w (canvas, x, y, &dx, &dy);

		dist = (* item->type->point) (canvas, item, dx, dy);

		if ((int) (dist / canvas->pixels_per_unit + 0.5) <= canvas->close_enough)
			best = item;
	}

	return best;
}

static void
add_tag (GnomeCanvasItem *item, GnomeCanvasKey tag)
{
	if (!has_tag (item, tag))
		g_hash_table_insert (item->tags, (gpointer) tag, (gpointer) tag);
}

static void
remove_tag (GnomeCanvasItem *item, GnomeCanvasKey tag)
{
	if (has_tag (item, tag))
		g_hash_table_remove (item->tags, (gpointer) tag);
}

static void
pick_current_item (GnomeCanvas *canvas, GdkEvent *event)
{
	int button_down;
	double x, y;

	/* If a button is down, we'll perform enter and leave events on the current item, but not
	 * enter on any other item.  This is more or less like X pointer grabbing for canvas items.
	 */

	button_down = canvas->state & (GDK_BUTTON1_MASK
				       | GDK_BUTTON2_MASK
				       | GDK_BUTTON3_MASK
				       | GDK_BUTTON4_MASK
				       | GDK_BUTTON5_MASK);
	if (!button_down)
		canvas->flags &= ~GNOME_CANVAS_LEFT_GRABBED_ITEM;

	/* Save the event in the canvas.  This is used to synthesize enter and leave events in case
	 * the current item changes.  It is also used to re-pick the current item if the current one
	 * gets deleted.  Also, synthesize an enter event.
	 */

	if (event != &canvas->pick_event) {
		if ((event->type == GDK_MOTION_NOTIFY) || (event->type == GDK_BUTTON_RELEASE)) {
			canvas->pick_event.crossing.type       = GDK_ENTER_NOTIFY;
			canvas->pick_event.crossing.window     = event->motion.window;
			canvas->pick_event.crossing.send_event = event->motion.send_event;
			canvas->pick_event.crossing.subwindow  = NULL;
			canvas->pick_event.crossing.x          = event->motion.x;
			canvas->pick_event.crossing.y          = event->motion.y;
			canvas->pick_event.crossing.x_root     = event->motion.x_root;
			canvas->pick_event.crossing.y_root     = event->motion.y_root;
			canvas->pick_event.crossing.mode       = GDK_CROSSING_NORMAL;
			canvas->pick_event.crossing.detail     = GDK_NOTIFY_NONLINEAR;
			canvas->pick_event.crossing.focus      = FALSE;
			canvas->pick_event.crossing.state      = event->motion.state;
		} else
			canvas->pick_event = *event;
	}

	/* Don't do anything else if this is a recursive call */

	if (canvas->flags & GNOME_CANVAS_IN_REPICK)
		return;

	/* LeaveNotify means that there is no current item, so we don't look for one */

	if (canvas->pick_event.type != GDK_LEAVE_NOTIFY) {
		x = event->crossing.x + canvas->display_x1;
		y = event->crossing.y + canvas->display_y1;
		canvas->new_current_item = find_closest_item (canvas, (int) (x + 0.5), (int) (y + 0.5));
	} else
		canvas->new_current_item = NULL;

	if ((canvas->new_current_item == canvas->current_item)
	    && !(canvas->flags & GNOME_CANVAS_LEFT_GRABBED_ITEM))
		return; /* current item did not change */

	/* Synthesize events for old and new current items */

	if ((canvas->new_current_item != canvas->current_item)
	    && (canvas->current_item != NULL)
	    && !(canvas->flags & GNOME_CANVAS_LEFT_GRABBED_ITEM)) {
		GdkEvent new_event;
		GnomeCanvasItem *item;

		item = canvas->current_item;

		new_event = canvas->pick_event;
		new_event.type = GDK_LEAVE_NOTIFY;

		new_event.crossing.detail = GDK_NOTIFY_ANCESTOR;
		canvas->flags |= GNOME_CANVAS_IN_REPICK;
		execute_bindings (canvas, &new_event);
		canvas->flags &= ~GNOME_CANVAS_IN_REPICK;

		/* Check whether someone deleted the current event */

		if ((item == canvas->current_item) && !button_down)
			remove_tag (item, GNOME_CANVAS_CURRENT);
	}

	/* new_current_item may have been set to NULL during the call to execute_bindings() above */

	if ((canvas->new_current_item != canvas->current_item) && button_down) {
		canvas->flags |= GNOME_CANVAS_LEFT_GRABBED_ITEM;
		return;
	}

	/* Handle the case where the GNOME_CANVAS_LEFT_GRABBED_ITEM flag was set */

	canvas->flags &= ~GNOME_CANVAS_LEFT_GRABBED_ITEM;
	canvas->current_item = canvas->new_current_item;

	if (canvas->current_item != NULL) {
		GdkEvent new_event;

		add_tag (canvas->current_item, GNOME_CANVAS_CURRENT);

		new_event = canvas->pick_event;
		new_event.type = GDK_ENTER_NOTIFY;
		new_event.crossing.detail = GDK_NOTIFY_ANCESTOR;
		execute_bindings (canvas, &new_event);
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
		execute_bindings (canvas, (GdkEvent *) event);
		break;

	case GDK_BUTTON_RELEASE:
		/* Process the event as if the button were pressed, then repick after the button has
		 * been released
		 */
		
		canvas->state = event->state;
		execute_bindings (canvas, (GdkEvent *) event);
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

	printf ("gnome_canvas_motion\n");

	canvas->state = event->state;
	pick_current_item (canvas, (GdkEvent *) event);
	execute_bindings (canvas, (GdkEvent *) event);

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

	execute_bindings (canvas, (GdkEvent *) event);

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

static GnomeCanvasItemType *
get_item_type (char *type)
{
	return g_hash_table_lookup (item_types, type);
}

GnomeCanvasId
gnome_canvas_create_item (GnomeCanvas *canvas, char *type, ...)
{
	GnomeCanvasId cid;
	GnomeCanvasItemType *item_type;
	GnomeCanvasItem *item;
	va_list args;

	cid.type = GNOME_CANVAS_ID_EMPTY;
	cid.data = NULL;
	
	g_return_val_if_fail (canvas != NULL, cid);
	g_return_val_if_fail (GNOME_IS_CANVAS (canvas), cid);
	g_return_val_if_fail (type != NULL, cid);

	item_type = get_item_type (type);
	if (!item_type) {
		g_warning ("Unknown canvas item type %s", type);
		return cid;
	}

	item = g_malloc0 (item_type->item_size);
	item->type = item_type;
	item->tags = g_hash_table_new (g_str_hash, g_str_equal);

	add_tag (item, GNOME_CANVAS_ALL); /* all objects have the "all" tag set */

	va_start (args, type);
	(* item_type->create) (canvas, item, args); /* ask the item to create itself */
	va_end (args);

	if (GTK_WIDGET_REALIZED (canvas))
		(* item_type->realize) (canvas, item);

	/* Insert item into list and set our flags */

	if (!canvas->item_list) {
		canvas->item_list = g_list_append (canvas->item_list, item);
		canvas->item_list_end = canvas->item_list;
	} else
		canvas->item_list_end = g_list_append (canvas->item_list_end, item)->next;

	gnome_canvas_request_redraw (canvas, item->x1, item->y1, item->x2, item->y2);

	canvas->flags |= GNOME_CANVAS_NEED_REPICK;

	/* Construct the stuff we will return */

	cid.type = GNOME_CANVAS_ID_ITEM;
	cid.data = item;

	return cid;
}

GnomeCanvasId
gnome_canvas_create_tag (GnomeCanvas *canvas, GnomeCanvasKey tag)
{
	GnomeCanvasId cid;

	cid.type = GNOME_CANVAS_ID_EMPTY;
	cid.data = NULL;

	g_return_val_if_fail (canvas != NULL, cid);
	g_return_val_if_fail (tag != NULL, cid);

	cid.type = GNOME_CANVAS_ID_TAG;
	cid.data = (gpointer) tag;

	return cid;
}

static void
foreach_id (GnomeCanvas *canvas, GnomeCanvasId cid, ForeachFunc func, va_list args, gpointer data)
{
	GList *list;
	GnomeCanvasItem *item;

	switch (cid.type) {
	case GNOME_CANVAS_ID_ITEM:
		item = cid.data;
		(* func) (canvas, item, args, data);
		break;

	case GNOME_CANVAS_ID_TAG:
		for (list = canvas->item_list; list; list = list->next) {
			item = list->data;

			if (has_tag (item, cid.data))
				(* func) (canvas, item, args, data);
		}
		break;

	default:
		break;
	}
}

static void
configure_item (GnomeCanvas *canvas, GnomeCanvasItem *item, va_list args, gpointer data)
{
	(* item->type->configure) (canvas, item, args, FALSE);
	gnome_canvas_request_redraw (canvas, item->x1, item->y1, item->x2, item->y2);
}

void
gnome_canvas_configure (GnomeCanvas *canvas, GnomeCanvasId cid, ...)
{
	va_list args;

	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	va_start (args, cid);
	foreach_id (canvas, cid, configure_item, args, NULL);
	va_end (args);
}

int
gnome_canvas_bind (GnomeCanvas *canvas, GnomeCanvasId cid, GdkEventType type, gpointer user_data)
{
	g_return_val_if_fail (canvas != NULL, -1);
	g_return_val_if_fail (GNOME_IS_CANVAS (canvas), -1);

	if (cid.type == GNOME_CANVAS_ID_EMPTY)
		return -1;

	return add_binding (canvas, type, cid, user_data);
}

void
gnome_canvas_unbind (GnomeCanvas *canvas, int binding_id)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	/* FIXME */
}

void
gnome_canvas_set_background (GnomeCanvas *canvas, GnomeCanvasColor color_type, ...)
{
	va_list args;
	GdkColor c;
	GtkWidget *widget;
	int set;

	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	widget = GTK_WIDGET (canvas);

	va_start (args, color_type);
	set = GNOME_CANVAS_VGET_COLOR (canvas, &c, color_type, args);
	va_end (args);

	if (set) {
		canvas->flags |= GNOME_CANVAS_BG_SET;
		canvas->bg_pixel = c.pixel;
	} else
		canvas->flags &= ~GNOME_CANVAS_BG_SET;

	if (GTK_WIDGET_REALIZED (widget)) {
		if (set)
			gdk_window_set_background (widget->window, &c);
		else
			gdk_window_set_background (widget->window, &widget->style->bg[GTK_STATE_NORMAL]);

		/* FIXME: should we queue a redraw or just gdk_window_clear_area_e() ? */
		if (GTK_WIDGET_DRAWABLE (widget))
			gdk_window_clear_area_e (widget->window,
						 0, 0,
						 widget->allocation.width,
						 widget->allocation.height);
	}
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
	GList *list;
	GnomeCanvasItem *item;

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

	if (canvas->flags & GNOME_CANVAS_BG_SET) {
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

	canvas->draw_x1 = draw_x1;
	canvas->draw_y1 = draw_y1;

	for (list = canvas->item_list; list; list = list->next) {
		item = list->data;

		if (((item->x1 < draw_x2)
		     && (item->y1 < draw_y2)
		     && (item->x2 > draw_x1)
		     && (item->y2 > draw_y1))
		    || (item->type->always_redraw
			&& (item->x1 < canvas->redraw_x2)
			&& (item->y1 < canvas->redraw_y2)
			&& (item->x2 > canvas->redraw_x1)
			&& (item->y2 > canvas->redraw_y2)))
			(* item->type->draw) (canvas, item, pixmap,
					      draw_x1, draw_y1,
					      width, height);
	}

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

	while (canvas->flags & GNOME_CANVAS_NEED_REPICK) {
		canvas->flags &= ~GNOME_CANVAS_NEED_REPICK;
		pick_current_item (canvas, &canvas->pick_event);
	}

	/* Paint if able to */

	if (GTK_WIDGET_DRAWABLE (canvas))
		paint (canvas);

	canvas->flags &= ~GNOME_CANVAS_NEED_REDRAW;
	canvas->redraw_x1 = 0;
	canvas->redraw_y1 = 0;
	canvas->redraw_x2 = 0;
	canvas->redraw_y2 = 0;

	/* FIXME: scrollbars */

	return FALSE;
}

static void
reconfigure_items (GnomeCanvas *canvas)
{
	GList *list;
	GnomeCanvasItem *item;
	va_list args;

	for (list = canvas->item_list; list; list = list->next) {
		item = list->data;

		(* item->type->configure) (canvas, item, args, TRUE);
	}
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

	reconfigure_items (canvas);
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

	reconfigure_items (canvas);
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
gnome_canvas_request_redraw (GnomeCanvas *canvas, int x1, int y1, int x2, int y2)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	if ((x1 == x2) || (y1 == y2))
		return;

	if (canvas->flags & GNOME_CANVAS_NEED_REDRAW) {
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

		canvas->flags |= GNOME_CANVAS_NEED_REDRAW;
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

void
gnome_canvas_c2s (GnomeCanvas *canvas, int cx, int cy, int *sx, int *sy)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	if (sx)
		*sx = cx - canvas->display_x1;

	if (sy)
		*sy = cy - canvas->display_y1;
}

void
gnome_canvas_s2c (GnomeCanvas *canvas, int sx, int sy, int *cx, int *cy)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	if (cx)
		*cx = sx + canvas->display_x1;

	if (cy)
		*cy = sy + canvas->display_y1;
}

int
gnome_canvas_get_color (GnomeCanvas *canvas, GdkColor *color, GnomeCanvasColor color_type, ...)
{
	va_list args;
	GdkColor *c;
	gint n;
	int alloc;

	g_return_val_if_fail (canvas != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_CANVAS (canvas), FALSE);
	g_return_val_if_fail (color != NULL, FALSE);

	if (color_type == GNOME_CANVAS_COLOR_NONE)
		return FALSE;

	alloc = FALSE;

	va_start (args, color_type);

	switch (color_type) {
	case GNOME_CANVAS_COLOR_STRING:
		gdk_color_parse (va_arg (args, gchar *), color);
		alloc = TRUE;
		break;

	case GNOME_CANVAS_COLOR_RGB:
		color->red   = va_arg (args, unsigned int);
		color->green = va_arg (args, unsigned int);
		color->blue  = va_arg (args, unsigned int);
		alloc = TRUE;
		break;

	case GNOME_CANVAS_COLOR_RGB_D:
		color->red   = 0xffff * va_arg (args, double);
		color->green = 0xffff * va_arg (args, double);
		color->blue  = 0xffff * va_arg (args, double);
		alloc = TRUE;
		break;

	case GNOME_CANVAS_COLOR_GDK:
		c = va_arg (args, GdkColor *);
		*color = *c;
		break;

	default:
		g_warning ("Unknown color type %d\n", (int) color_type);
	}

	va_end (args);

	if (alloc) {
		color->pixel = 0;
		n = 0;

		gdk_color_context_get_pixels (canvas->cc,
					      &color->red,
					      &color->green,
					      &color->blue,
					      1,
					      &color->pixel,
					      &n);
	}

	return TRUE;
}

static void
gnome_canvas_marshal_signal_1 (GtkObject     *object,
			       GtkSignalFunc  func,
			       gpointer       func_data,
			       GtkArg        *args)
{
	GnomeCanvasSignal1 rfunc;
	gint *return_val;

	rfunc = (GnomeCanvasSignal1) func;
	return_val = GTK_RETLOC_BOOL (args[3]);

	*return_val = (* rfunc) (object,
				 GTK_VALUE_POINTER (args[0]),
				 GTK_VALUE_BOXED (args[1]),
				 GTK_VALUE_POINTER (args[2]),
				 func_data);
}
