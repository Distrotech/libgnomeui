/* gnome-dock-item.c
 *
 * Copyright (C) 1998 Ettore Perazzoli
 * Copyright (C) 1998 Elliot Lee
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gdk/gdkx.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtktoolbar.h>
#include <gtk/gtkwindow.h>

#include "gnome-dock-item.h"

enum {
  ARG_0,
  ARG_SHADOW,
  ARG_ORIENTATION,
  ARG_PREFERRED_WIDTH,
  ARG_PREFERRED_HEIGHT
};

#define DRAG_HANDLE_SIZE 10

enum {
  DOCK_DRAG_BEGIN,
  DOCK_DRAG_END,
  DOCK_DRAG_MOTION,
  DOCK_DETACH,
  LAST_SIGNAL
};


static gboolean  check_guint_arg       (GtkObject     *object,
					const gchar   *name,
					guint         *value_return);
static guint     get_preferred_width   (GnomeDockItem *item);
static guint     get_preferred_height  (GnomeDockItem *item);

static void gnome_dock_item_class_init     (GnomeDockItemClass *klass);
static void gnome_dock_item_init           (GnomeDockItem      *dock_item);
static void gnome_dock_item_set_arg        (GtkObject         *object,
                                            GtkArg            *arg,
                                            guint              arg_id);
static void gnome_dock_item_get_arg        (GtkObject         *object,
                                            GtkArg            *arg,
                                            guint              arg_id);
static void gnome_dock_item_destroy        (GtkObject         *object);
static void gnome_dock_item_map            (GtkWidget         *widget);
static void gnome_dock_item_unmap          (GtkWidget         *widget);
static void gnome_dock_item_realize        (GtkWidget         *widget);
static void gnome_dock_item_unrealize      (GtkWidget         *widget);
static void gnome_dock_item_style_set      (GtkWidget         *widget,
                                            GtkStyle          *previous_style);
static void gnome_dock_item_size_request   (GtkWidget         *widget,
                                            GtkRequisition    *requisition);
static void gnome_dock_item_size_allocate  (GtkWidget         *widget,
                                            GtkAllocation     *real_allocation);
static void gnome_dock_item_add            (GtkContainer      *container,
                                            GtkWidget         *widget);
static void gnome_dock_item_remove         (GtkContainer      *container,
                                            GtkWidget         *widget);
static void gnome_dock_item_paint          (GtkWidget         *widget,
                                            GdkEventExpose    *event,
                                            GdkRectangle      *area);
static void gnome_dock_item_draw           (GtkWidget         *widget,
                                            GdkRectangle      *area);
static gint gnome_dock_item_expose         (GtkWidget         *widget,
                                            GdkEventExpose    *event);
static gint gnome_dock_item_button_changed (GtkWidget         *widget,
                                            GdkEventButton    *event);
static gint gnome_dock_item_motion         (GtkWidget         *widget,
                                            GdkEventMotion    *event);
static gint gnome_dock_item_delete_event   (GtkWidget         *widget,
                                            GdkEventAny       *event);


static GtkBinClass *parent_class;
static guint        dock_item_signals[LAST_SIGNAL] = { 0 };


/* Helper functions.  */

static gboolean
check_guint_arg (GtkObject *object,
		 const gchar *name,
		 guint *value_return)
{
  GtkArgInfo *info;
  gchar *error;

  error = gtk_object_arg_get_info (GTK_OBJECT_TYPE (object), name, &info);
  if (error != NULL)
    {
      g_free (error);
      return FALSE;
    }

  gtk_object_get (object, name, value_return, NULL);
  return TRUE;
}

static guint
get_preferred_width (GnomeDockItem *dock_item)
{
  GtkWidget *child;
  guint preferred_width = 0;

  child = GTK_BIN (dock_item)->child;

  if (child != NULL)
    {
      if (! check_guint_arg (GTK_OBJECT (child), "preferred_width", &preferred_width))
        {
          GtkRequisition child_requisition;
  
          gtk_widget_get_child_requisition (child, &child_requisition);
          preferred_width = child_requisition.width;
        }
    }

  if (dock_item->orientation == GTK_ORIENTATION_HORIZONTAL)
    preferred_width += GNOME_DOCK_ITEM_NOT_LOCKED (dock_item) ? DRAG_HANDLE_SIZE : 0;

  preferred_width += GTK_CONTAINER (dock_item)->border_width * 2;

  return preferred_width;
}

static guint
get_preferred_height (GnomeDockItem *dock_item)
{
  GtkWidget *child;
  guint preferred_height = 0;

  child = GTK_BIN (dock_item)->child;

  if (child != NULL)
    {
      if (! check_guint_arg (GTK_OBJECT (child), "preferred_height", &preferred_height))
      {
        GtkRequisition child_requisition;
  
        gtk_widget_get_child_requisition (child, &child_requisition);
        preferred_height = child_requisition.height;
      }
    }

  if (dock_item->orientation == GTK_ORIENTATION_VERTICAL)
    preferred_height += GNOME_DOCK_ITEM_NOT_LOCKED (dock_item) ? DRAG_HANDLE_SIZE : 0;

  preferred_height += GTK_CONTAINER (dock_item)->border_width * 2;

  return preferred_height;
}


guint
gnome_dock_item_get_type (void)
{
  static guint dock_item_type = 0;

  if (!dock_item_type)
    {
      GtkTypeInfo dock_item_info =
      {
	"GnomeDockItem",
	sizeof (GnomeDockItem),
	sizeof (GnomeDockItemClass),
	(GtkClassInitFunc) gnome_dock_item_class_init,
	(GtkObjectInitFunc) gnome_dock_item_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      dock_item_type = gtk_type_unique (gtk_bin_get_type (), &dock_item_info);
    }

  return dock_item_type;
}

static void
gnome_dock_item_class_init (GnomeDockItemClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;

  object_class = (GtkObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;
  container_class = (GtkContainerClass *) class;

  parent_class = gtk_type_class (gtk_bin_get_type ());

  gtk_object_add_arg_type ("GnomeDockItem::shadow",
                           GTK_TYPE_SHADOW_TYPE, GTK_ARG_READWRITE,
                           ARG_SHADOW);
  gtk_object_add_arg_type ("GnomeDockItem::orientation",
                           GTK_TYPE_ORIENTATION, GTK_ARG_READWRITE,
                           ARG_ORIENTATION);
  gtk_object_add_arg_type ("GnomeDockItem::preferred_width",
                           GTK_TYPE_UINT, GTK_ARG_READABLE,
                           ARG_PREFERRED_WIDTH);
  gtk_object_add_arg_type ("GnomeDockItem::preferred_height",
                           GTK_TYPE_UINT, GTK_ARG_READABLE,
                           ARG_PREFERRED_HEIGHT);

  object_class->set_arg = gnome_dock_item_set_arg;
  object_class->get_arg = gnome_dock_item_get_arg;
  
  dock_item_signals[DOCK_DRAG_BEGIN] =
    gtk_signal_new ("dock_drag_begin",
                    GTK_RUN_LAST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GnomeDockItemClass, dock_drag_begin),
                    gtk_marshal_NONE__NONE,
                    GTK_TYPE_NONE, 0);

  dock_item_signals[DOCK_DRAG_MOTION] =
    gtk_signal_new ("dock_drag_motion",
                    GTK_RUN_LAST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GnomeDockItemClass, dock_drag_motion),
                    gtk_marshal_NONE__INT_INT,
                    GTK_TYPE_NONE, 2,
                    GTK_TYPE_INT,
                    GTK_TYPE_INT);

  dock_item_signals[DOCK_DRAG_END] =
    gtk_signal_new ("dock_drag_end",
                    GTK_RUN_LAST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GnomeDockItemClass, dock_drag_end),
                    gtk_marshal_NONE__NONE,
                    GTK_TYPE_NONE, 0);

  dock_item_signals[DOCK_DETACH] =
    gtk_signal_new ("dock_detach",
                    GTK_RUN_LAST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GnomeDockItemClass, dock_detach),
                    gtk_marshal_NONE__NONE,
                    GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, dock_item_signals, LAST_SIGNAL);
  
  object_class->destroy = gnome_dock_item_destroy;

  widget_class->map = gnome_dock_item_map;
  widget_class->unmap = gnome_dock_item_unmap;
  widget_class->realize = gnome_dock_item_realize;
  widget_class->unrealize = gnome_dock_item_unrealize;
  widget_class->style_set = gnome_dock_item_style_set;
  widget_class->size_request = gnome_dock_item_size_request;
  widget_class->size_allocate = gnome_dock_item_size_allocate;
  widget_class->draw = gnome_dock_item_draw;
  widget_class->expose_event = gnome_dock_item_expose;
  widget_class->button_press_event = gnome_dock_item_button_changed;
  widget_class->button_release_event = gnome_dock_item_button_changed;
  widget_class->motion_notify_event = gnome_dock_item_motion;
  widget_class->delete_event = gnome_dock_item_delete_event;

  container_class->add = gnome_dock_item_add;
  container_class->remove = gnome_dock_item_remove;
}

static void
gnome_dock_item_init (GnomeDockItem *dock_item)
{
  GTK_WIDGET_UNSET_FLAGS (dock_item, GTK_NO_WINDOW);

  dock_item->bin_window = NULL;
  dock_item->float_window = NULL;
  dock_item->shadow_type = GTK_SHADOW_OUT;

  dock_item->orientation = GTK_ORIENTATION_HORIZONTAL;
  dock_item->behavior = GNOME_DOCK_ITEM_BEH_NORMAL;

  dock_item->float_window_mapped = FALSE;
  dock_item->is_floating = FALSE;
  dock_item->in_drag = FALSE;

  dock_item->dragoff_x = 0;
  dock_item->dragoff_y = 0;

  dock_item->float_x = 0;
  dock_item->float_y = 0;
}

static void
gnome_dock_item_set_arg (GtkObject *object,
                         GtkArg *arg,
                         guint arg_id)
{
  GnomeDockItem *dock_item;

  dock_item = GNOME_DOCK_ITEM (object);

  switch (arg_id)
    {
    case ARG_SHADOW:
      gnome_dock_item_set_shadow_type (dock_item, GTK_VALUE_ENUM (*arg));
      break;
    case ARG_ORIENTATION:
      gnome_dock_item_set_orientation (dock_item, GTK_VALUE_ENUM (*arg));
      break;
    default:
      break;
    }
}

static void
gnome_dock_item_get_arg (GtkObject *object,
                         GtkArg *arg,
                         guint arg_id)
{
  GnomeDockItem *dock_item;

  dock_item = GNOME_DOCK_ITEM (object);

  switch (arg_id)
    {
    case ARG_SHADOW:
      GTK_VALUE_ENUM (*arg) = gnome_dock_item_get_shadow_type (dock_item);
      break;
    case ARG_ORIENTATION:
      GTK_VALUE_ENUM (*arg) = gnome_dock_item_get_orientation (dock_item);
      break;
    case ARG_PREFERRED_HEIGHT:
      GTK_VALUE_UINT (*arg) = get_preferred_height (dock_item);
      break;
    case ARG_PREFERRED_WIDTH:
      GTK_VALUE_UINT (*arg) = get_preferred_width (dock_item);
      break;
    default:
      arg->type = GTK_TYPE_INVALID;
      break;
    }
}

static void
gnome_dock_item_destroy (GtkObject *object)
{
  GnomeDockItem *di;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GNOME_IS_DOCK_ITEM (object));

  di = GNOME_DOCK_ITEM (object);
  g_free (di->name);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gnome_dock_item_map (GtkWidget *widget)
{
  GtkBin *bin;
  GnomeDockItem *di;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GNOME_IS_DOCK_ITEM (widget));

  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);

  bin = GTK_BIN (widget);
  di = GNOME_DOCK_ITEM (widget);

  gdk_window_show (di->bin_window);
  if (! di->is_floating)
    gdk_window_show (widget->window);

  if (di->is_floating && !di->float_window_mapped)
    gnome_dock_item_detach (di, di->float_x, di->float_y);

  if (bin->child
      && GTK_WIDGET_VISIBLE (bin->child)
      && !GTK_WIDGET_MAPPED (bin->child))
    gtk_widget_map (bin->child);
}

static void
gnome_dock_item_unmap (GtkWidget *widget)
{
  GnomeDockItem *di;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GNOME_IS_DOCK_ITEM (widget));

  GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);

  di = GNOME_DOCK_ITEM (widget);

  gdk_window_hide (widget->window);
  if (di->float_window_mapped)
    {
      gdk_window_hide (di->float_window);
      di->float_window_mapped = FALSE;
    }
}

static void
gnome_dock_item_realize (GtkWidget *widget)
{
  GdkWindowAttr attributes;
  gint attributes_mask;
  GnomeDockItem *di;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GNOME_IS_DOCK_ITEM (widget));

  di = GNOME_DOCK_ITEM (widget);

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = (gtk_widget_get_events (widget)
			   | GDK_EXPOSURE_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, widget);

  attributes.x = 0;
  attributes.y = 0;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask |= (gtk_widget_get_events (widget) |
			    GDK_EXPOSURE_MASK |
			    GDK_BUTTON1_MOTION_MASK |
			    GDK_POINTER_MOTION_HINT_MASK |
			    GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  di->bin_window = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (di->bin_window, widget);
  if (GTK_BIN (di)->child)
    gtk_widget_set_parent_window (GTK_BIN (di)->child, di->bin_window);
  
  attributes.x = 0;
  attributes.y = 0;
  attributes.width = widget->requisition.width;
  attributes.height = widget->requisition.height;
  attributes.window_type = GDK_WINDOW_TOPLEVEL;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = (gtk_widget_get_events (widget) |
			   GDK_KEY_PRESS_MASK |
			   GDK_ENTER_NOTIFY_MASK |
			   GDK_LEAVE_NOTIFY_MASK |
			   GDK_FOCUS_CHANGE_MASK |
			   GDK_STRUCTURE_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  di->float_window = gdk_window_new (NULL, &attributes, attributes_mask);
  gdk_window_set_transient_for(di->float_window, gdk_window_get_toplevel(widget->window));
  gdk_window_set_user_data (di->float_window, widget);
  gdk_window_set_decorations (di->float_window, 0);
  
  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_WIDGET_STATE (di));
  gtk_style_set_background (widget->style, di->bin_window, GTK_WIDGET_STATE (di));
  gtk_style_set_background (widget->style, di->float_window, GTK_WIDGET_STATE (di));
  gdk_window_set_back_pixmap (widget->window, NULL, TRUE);

  if (di->is_floating)
    gnome_dock_item_detach (di, di->float_x, di->float_y);
}

static void
gnome_dock_item_unrealize (GtkWidget *widget)
{
  GnomeDockItem *di;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GNOME_IS_DOCK_ITEM (widget));

  di = GNOME_DOCK_ITEM (widget);

  gdk_window_set_user_data (di->bin_window, NULL);
  gdk_window_destroy (di->bin_window);
  di->bin_window = NULL;
  gdk_window_set_user_data (di->float_window, NULL);
  gdk_window_destroy (di->float_window);
  di->float_window = NULL;

  if (GTK_WIDGET_CLASS (parent_class)->unrealize)
    (* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

static void
gnome_dock_item_style_set (GtkWidget *widget,
                           GtkStyle  *previous_style)
{
  GnomeDockItem *di;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GNOME_IS_DOCK_ITEM (widget));

  di = GNOME_DOCK_ITEM (widget);

  if (GTK_WIDGET_REALIZED (widget) &&
      !GTK_WIDGET_NO_WINDOW (widget))
    {
      gtk_style_set_background (widget->style, widget->window,
                                widget->state);
      gtk_style_set_background (widget->style, di->bin_window, widget->state);
      gtk_style_set_background (widget->style, di->float_window, widget->state);
      if (GTK_WIDGET_DRAWABLE (widget))
	gdk_window_clear (widget->window);
    }
}

static void
gnome_dock_item_size_request (GtkWidget      *widget,
                              GtkRequisition *requisition)
{
  GtkBin *bin;
  GnomeDockItem *dock_item;
  GtkRequisition child_requisition;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GNOME_IS_DOCK_ITEM (widget));
  g_return_if_fail (requisition != NULL);

  bin = GTK_BIN (widget);
  dock_item = GNOME_DOCK_ITEM (widget);

  /* If our child is not visible, we still request its size, since
     we won't have any useful hint for our size otherwise.  */
  if (bin->child != NULL)
    gtk_widget_size_request (bin->child, &child_requisition);
  else
    {
      child_requisition.width = 0;
      child_requisition.height = 0;
    }

  if (dock_item->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      requisition->width = 
        GNOME_DOCK_ITEM_NOT_LOCKED (dock_item) ? DRAG_HANDLE_SIZE : 0;
      if (bin->child != NULL)
        {
          requisition->width += child_requisition.width;
          requisition->height = child_requisition.height;
        }
      else
        requisition->height = 0;
    }
  else
    {
      requisition->height = 
        GNOME_DOCK_ITEM_NOT_LOCKED (dock_item) ? DRAG_HANDLE_SIZE : 0;
      if (bin->child != NULL)
        {
          requisition->width = child_requisition.width;
          requisition->height += child_requisition.height;
        }
      else
        requisition->width = 0;
    }

  requisition->width += GTK_CONTAINER (widget)->border_width * 2;
  requisition->height += GTK_CONTAINER (widget)->border_width * 2;
}

static void
gnome_dock_item_size_allocate (GtkWidget     *widget,
                               GtkAllocation *allocation)
{
  GtkBin *bin;
  GnomeDockItem *di;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GNOME_IS_DOCK_ITEM (widget));
  g_return_if_fail (allocation != NULL);
  
  bin = GTK_BIN (widget);
  di = GNOME_DOCK_ITEM (widget);

  widget->allocation = *allocation;

  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_move_resize (widget->window,
                            widget->allocation.x,
                            widget->allocation.y,
                            widget->allocation.width,
                            widget->allocation.height);

  if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
    {
      GtkWidget *child;
      GtkAllocation child_allocation;
      int border_width;

      child = bin->child;
      border_width = GTK_CONTAINER (widget)->border_width;

      child_allocation.x = border_width;
      child_allocation.y = border_width;

      if (GNOME_DOCK_ITEM_NOT_LOCKED(di))
        {
          if (di->orientation == GTK_ORIENTATION_HORIZONTAL)
            child_allocation.x += DRAG_HANDLE_SIZE;
          else
            child_allocation.y += DRAG_HANDLE_SIZE;
        }

      if (di->is_floating)
	{
          GtkRequisition child_requisition;
	  guint float_width;
	  guint float_height;

	  gtk_widget_get_child_requisition (child, &child_requisition);

          child_allocation.width = child_requisition.width;
          child_allocation.height = child_requisition.height;

	  float_width = child_allocation.width + 2 * border_width;
	  float_height = child_allocation.height + 2 * border_width;
	  
	  if (di->orientation == GTK_ORIENTATION_HORIZONTAL)
	    float_width += DRAG_HANDLE_SIZE;
	  else
	    float_height += DRAG_HANDLE_SIZE;

	  if (GTK_WIDGET_REALIZED (di))
	    {
	      gdk_window_resize (di->float_window,
				 float_width,
				 float_height);
	      gdk_window_move_resize (di->bin_window,
				      0,
				      0,
				      float_width,
				      float_height);
	    }
	}
      else
	{
	  child_allocation.width = MAX (1, (int) widget->allocation.width - 2 * border_width);
	  child_allocation.height = MAX (1, (int) widget->allocation.height - 2 * border_width);

          if (GNOME_DOCK_ITEM_NOT_LOCKED (di))
            {
              if (di->orientation == GTK_ORIENTATION_HORIZONTAL)
		child_allocation.width = MAX ((int) child_allocation.width - DRAG_HANDLE_SIZE, 1);
              else
		child_allocation.height = MAX ((int) child_allocation.height - DRAG_HANDLE_SIZE, 1);
            }

	  if (GTK_WIDGET_REALIZED (di))
	    gdk_window_move_resize (di->bin_window,
				    0,
				    0,
				    widget->allocation.width,
				    widget->allocation.height);
	}

      gtk_widget_size_allocate (bin->child, &child_allocation);
    }
}

static void
draw_textured_frame (GtkWidget *widget, GdkWindow *window, GdkRectangle *rect, GtkShadowType shadow,
		     GdkRectangle *clip)
{
  gtk_paint_handle(widget->style, window, GTK_STATE_NORMAL, shadow,
                   clip, widget, "dockitem",
                   rect->x, rect->y, rect->width, rect->height, 
                   GTK_ORIENTATION_VERTICAL);
}

static void
gnome_dock_item_paint (GtkWidget      *widget,
                       GdkEventExpose *event,
                       GdkRectangle   *area)
{
  GtkBin *bin;
  GnomeDockItem *di;
  guint width;
  guint height;
  guint border_width;
  GdkRectangle rect;
  gint drag_handle_size = DRAG_HANDLE_SIZE;

  if (!GNOME_DOCK_ITEM_NOT_LOCKED (widget))
    drag_handle_size = 0;

  bin = GTK_BIN (widget);
  di = GNOME_DOCK_ITEM (widget);

  border_width = GTK_CONTAINER (di)->border_width;

  if (di->is_floating)
    {
      width = bin->child->allocation.width + 2 * border_width;
      height = bin->child->allocation.height + 2 * border_width;
    }
  else if (di->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      width = widget->allocation.width - drag_handle_size;
      height = widget->allocation.height;
    }
  else
    {
      width = widget->allocation.width;
      height = widget->allocation.height - drag_handle_size;
    }

  if (!event)
    gtk_paint_box(widget->style,
                  di->bin_window,
                  GTK_WIDGET_STATE (widget),
                  di->shadow_type,
                  area, widget,
                  "dockitem_bin",
                  0, 0, -1, -1);
  else
    gtk_paint_box(widget->style,
                  di->bin_window,
                  GTK_WIDGET_STATE (widget),
                  di->shadow_type,
                  &event->area, widget,
                  "dockitem_bin",
                  0, 0, -1, -1);

  /* We currently draw the handle _above_ the relief of the dockitem.
     It could also be drawn on the same level...  */

  if (GNOME_DOCK_ITEM_NOT_LOCKED (di))
    {
      GdkRectangle dest;

      rect.x = 0;
      rect.y = 0;
      
      if (di->orientation == GTK_ORIENTATION_HORIZONTAL)
        {
          rect.width = DRAG_HANDLE_SIZE;
          rect.height = height;
        }
      else
        {
          rect.width = width;
          rect.height = DRAG_HANDLE_SIZE;
        }

      if (gdk_rectangle_intersect (event ? &event->area : area, &rect, &dest))
	draw_textured_frame (widget, di->bin_window, &rect,
			     GTK_SHADOW_OUT,
			     event ? &event->area : area);
    }    
  
  if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
    {
      GdkRectangle child_area;
      GdkEventExpose child_event;

      if (!event) /* we were called from draw() */
	{
	  if (gtk_widget_intersect (bin->child, area, &child_area))
	    gtk_widget_draw (bin->child, &child_area);
	}
      else /* we were called from expose() */
	{
	  child_event = *event;
	  
	  if (GTK_WIDGET_NO_WINDOW (bin->child) &&
	      gtk_widget_intersect (bin->child, &event->area, &child_event.area))
	    gtk_widget_event (bin->child, (GdkEvent *) &child_event);
	}
    }
}

static void
gnome_dock_item_draw (GtkWidget    *widget,
                      GdkRectangle *area)
{
  GnomeDockItem *di;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GNOME_IS_DOCK_ITEM (widget));

  di = GNOME_DOCK_ITEM (widget);

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      if (di->is_floating)
	{
          GdkRectangle r;

	  /* The area parameter does not make sense in this case, so
	     we repaint everything.  */
	  r.x = 0;
	  r.y = 0;
	  r.width = (2 * GTK_CONTAINER (di)->border_width
                     + DRAG_HANDLE_SIZE);
	  r.height = r.width + GTK_BIN (di)->child->allocation.height;
	  r.width += GTK_BIN (di)->child->allocation.width;

	  gnome_dock_item_paint (widget, NULL, &r);
	}
      else
	gnome_dock_item_paint (widget, NULL, area);
    }
}

static gint
gnome_dock_item_expose (GtkWidget      *widget,
                        GdkEventExpose *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GNOME_IS_DOCK_ITEM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (GTK_WIDGET_DRAWABLE (widget) && event->window != widget->window)
    gnome_dock_item_paint (widget, event, NULL);
  
  return FALSE;
}

static gint
gnome_dock_item_button_changed (GtkWidget      *widget,
                                GdkEventButton *event)
{
  GnomeDockItem *di;
  gboolean event_handled;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GNOME_IS_DOCK_ITEM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  di = GNOME_DOCK_ITEM (widget);

  if (event->window != di->bin_window)
    return FALSE;

  if (!GNOME_DOCK_ITEM_NOT_LOCKED(widget))
    return FALSE;

  event_handled = FALSE;

  if (event->button == 1 && event->type == GDK_BUTTON_PRESS)
    {
      GtkWidget *child;
      gboolean in_handle;
      
      child = GTK_BIN (di)->child;
      
      switch (di->orientation)
	{
	case GTK_ORIENTATION_HORIZONTAL:
	  in_handle = event->x < DRAG_HANDLE_SIZE;
	  break;
	case GTK_ORIENTATION_VERTICAL:
	  in_handle = event->y < DRAG_HANDLE_SIZE;
	  break;
	default:
	  in_handle = FALSE;
	  break;
	}

      if (!child)
	{
	  in_handle = FALSE;
	  event_handled = TRUE;
	}
      
      if (in_handle)
	{
	  di->dragoff_x = event->x;
	  di->dragoff_y = event->y;

	  di->in_drag = TRUE;

          gnome_dock_item_grab_pointer (di);

	  gtk_object_set_data (GTK_OBJECT (di),
			       "gnome_dock_item_pre_drag_orientation",
			       GINT_TO_POINTER (di->orientation));

          gtk_signal_emit (GTK_OBJECT (widget),
                           dock_item_signals[DOCK_DRAG_BEGIN]);

	  event_handled = TRUE;
	}
    }
  else if (event->type == GDK_BUTTON_RELEASE && di->in_drag)
    {
      gdk_pointer_ungrab (GDK_CURRENT_TIME);
      
      di->in_drag = FALSE;

      gtk_signal_emit (GTK_OBJECT (widget),
                       dock_item_signals[DOCK_DRAG_END]);
      event_handled = TRUE;
    }

  return event_handled;
}

static gint
gnome_dock_item_motion (GtkWidget      *widget,
                        GdkEventMotion *event)
{
  GnomeDockItem *di;
  gint new_x, new_y;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GNOME_IS_DOCK_ITEM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  di = GNOME_DOCK_ITEM (widget);
  if (!di->in_drag)
    return FALSE;

  if (event->window != di->bin_window)
    return FALSE;

  gdk_window_get_pointer (NULL, &new_x, &new_y, NULL);
  
  new_x -= di->dragoff_x;
  new_y -= di->dragoff_y;

  gtk_signal_emit (GTK_OBJECT (widget),
                   dock_item_signals[DOCK_DRAG_MOTION],
                   new_x, new_y);

  return TRUE;
}

static void
gnome_dock_item_add (GtkContainer *container,
                     GtkWidget    *widget)
{
  GnomeDockItem *dock_item;
  GtkArgInfo *info_p;
  gchar *error;

  g_return_if_fail (GNOME_IS_DOCK_ITEM (container));
  g_return_if_fail (GTK_BIN (container)->child == NULL);
  g_return_if_fail (widget->parent == NULL);

  dock_item = GNOME_DOCK_ITEM (container);

  gtk_widget_set_parent_window (widget, dock_item->bin_window);
  GTK_CONTAINER_CLASS (parent_class)->add (container, widget);

  error = gtk_object_arg_get_info (GTK_OBJECT_TYPE (widget),
				   "orientation", &info_p);
  if (error)
    g_free (error);
  else
    gtk_object_set (GTK_OBJECT (widget),
		    "orientation", dock_item->orientation,
		    NULL);
}

static void
gnome_dock_item_set_floating (GnomeDockItem *item, gboolean val)
{
  item->is_floating = val;

  /* If there is a child and it supports the 'is_floating' flag
   * set that too.
   */
  if (item->bin.child != NULL) {
    GtkArgInfo *info_p;
    gchar *error = gtk_object_arg_get_info (GTK_OBJECT_TYPE (item->bin.child),
					    "is_floating", &info_p);
    if (error)
      g_free (error);
    else
      gtk_object_set (GTK_OBJECT (item->bin.child), "is_floating", val, NULL);
  }
}

static void
gnome_dock_item_remove (GtkContainer *container,
                        GtkWidget    *widget)
{
  GnomeDockItem *di;

  g_return_if_fail (GNOME_IS_DOCK_ITEM (container));
  g_return_if_fail (GTK_BIN (container)->child == widget);

  di = GNOME_DOCK_ITEM (container);

  if (di->is_floating)
    {
      gnome_dock_item_set_floating (di, FALSE);
      if (GTK_WIDGET_REALIZED (di))
	{
	  gdk_window_hide (di->float_window);
	  gdk_window_reparent (di->bin_window, GTK_WIDGET (di)->window, 0, 0);
          gdk_window_show (widget->window);
	}
      di->float_window_mapped = FALSE;
    }
  if (di->in_drag)
    {
      gdk_pointer_ungrab (GDK_CURRENT_TIME);
      di->in_drag = FALSE;
    }
  
  GTK_CONTAINER_CLASS (parent_class)->remove (container, widget);
}

static gint
gnome_dock_item_delete_event (GtkWidget *widget,
                              GdkEventAny  *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GNOME_IS_DOCK_ITEM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  return TRUE;
}



/**
 * gnome_dock_item_construct:
 * @new: a #GnomeDockItem.
 * @name: Name for the new item
 * @behavior: Behavior for the new item
 * 
 * Description: Constructs the @new GnomeDockItem named @name, with the
 * specified @behavior.
 * 
 * Returns: A new GnomeDockItem widget.
 **/
void
gnome_dock_item_construct (GnomeDockItem *new,
			   const gchar *name,
			   GnomeDockItemBehavior behavior)
{
	g_return_if_fail (new != NULL);
	g_return_if_fail (GNOME_IS_DOCK_ITEM (new));
	
	new->name = g_strdup (name);
	new->behavior = behavior;
}

/**
 * gnome_dock_item_new:
 * @name: Name for the new item
 * @behavior: Behavior for the new item
 * 
 * Description: Create a new GnomeDockItem named @name, with the
 * specified @behavior.
 * 
 * Returns: A new GnomeDockItem widget.
 **/
GtkWidget*
gnome_dock_item_new (const gchar *name,
                     GnomeDockItemBehavior behavior)
{
  GnomeDockItem *new;

  new = GNOME_DOCK_ITEM (gtk_type_new (gnome_dock_item_get_type ()));

  gnome_dock_item_construct (new, name, behavior);

  return GTK_WIDGET (new);
}

/**
 * gnome_dock_item_get_child:
 * @item: A GnomeDockItem widget
 * 
 * Description: Retrieve the child of @item.
 * 
 * Returns: The child of @item.
 **/
GtkWidget *
gnome_dock_item_get_child (GnomeDockItem *item)
{
  return GTK_BIN (item)->child;
}

/**
 * gnome_dock_item_get_name:
 * @item: A GnomeDockItem widget.
 * 
 * Description: Retrieve the name of @item.
 * 
 * Return value: The name of @item as a malloc()ed zero-terminated
 * string.
 **/
gchar *
gnome_dock_item_get_name (GnomeDockItem *item)
{
  return g_strdup (item->name);
}

/**
 * gnome_dock_item_set_shadow_type:
 * @dock_item: A GnomeDockItem widget
 * @type: The shadow type for @dock_item
 * 
 * Description: Set the shadow type for @dock_item.
 **/
void
gnome_dock_item_set_shadow_type (GnomeDockItem  *dock_item,
                                 GtkShadowType  type)
{
  g_return_if_fail (dock_item != NULL);
  g_return_if_fail (GNOME_IS_DOCK_ITEM (dock_item));

  if (dock_item->shadow_type != type)
    {
      dock_item->shadow_type = type;

      if (GTK_WIDGET_DRAWABLE (dock_item))
        gtk_widget_queue_clear (GTK_WIDGET (dock_item));
      gtk_widget_queue_resize (GTK_WIDGET (dock_item));
    }
}

/**
 * gnome_dock_item_get_shadow_type:
 * @dock_item: A GnomeDockItem widget.
 * 
 * Description: Retrieve the shadow type of @dock_item.
 * 
 * Returns: @dock_item's shadow type.
 **/
GtkShadowType
gnome_dock_item_get_shadow_type (GnomeDockItem  *dock_item)
{
  g_return_val_if_fail (dock_item != NULL, GTK_SHADOW_OUT);
  g_return_val_if_fail (GNOME_IS_DOCK_ITEM (dock_item), GTK_SHADOW_OUT);

  return dock_item->shadow_type;
}

/**
 * gnome_dock_item_set_orientation:
 * @dock_item: A GnomeDockItem widget
 * @orientation: New orientation for @dock_item
 * 
 * Description: Set the orientation for @dock_item.
 * 
 * Returns: %TRUE if the operation succeeds, %FALSE if it fails.
 **/
gboolean
gnome_dock_item_set_orientation (GnomeDockItem *dock_item,
                                 GtkOrientation orientation)
{
  GtkArgInfo *info_p;
  gchar *error;

  g_return_val_if_fail (dock_item != NULL, FALSE);
  g_return_val_if_fail (GNOME_IS_DOCK_ITEM (dock_item), FALSE);

  if (dock_item->orientation != orientation)
    {
      if ((orientation == GTK_ORIENTATION_VERTICAL
           && (dock_item->behavior & GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL))
          || (orientation == GTK_ORIENTATION_HORIZONTAL
              && (dock_item->behavior & GNOME_DOCK_ITEM_BEH_NEVER_HORIZONTAL)))
        return FALSE;

      dock_item->orientation = orientation;

      if (dock_item->bin.child != NULL) {
	error = gtk_object_arg_get_info (GTK_OBJECT_TYPE (dock_item->bin.child),
					 "orientation", &info_p);
	if (error)
	  g_free (error);
	else
	  gtk_object_set (GTK_OBJECT (dock_item->bin.child),
			  "orientation", orientation,
			  NULL);
      }
      if (GTK_WIDGET_DRAWABLE (dock_item))
        gtk_widget_queue_clear (GTK_WIDGET (dock_item));
      gtk_widget_queue_resize (GTK_WIDGET (dock_item));
    }

  return TRUE;
}

/**
 * gnome_dock_item_get_orientation:
 * @dock_item: A GnomeDockItem widget.
 * 
 * Description: Retrieve the orientation of @dock_item.
 * 
 * Returns: The current orientation of @dock_item.
 **/
GtkOrientation
gnome_dock_item_get_orientation (GnomeDockItem *dock_item)
{
  g_return_val_if_fail (dock_item != NULL,
                        GTK_ORIENTATION_HORIZONTAL);
  g_return_val_if_fail (GNOME_IS_DOCK_ITEM (dock_item),
                        GTK_ORIENTATION_HORIZONTAL);

  return dock_item->orientation;
}

/**
 * gnome_dock_item_get_behavior:
 * @dock_item: A GnomeDockItem widget.
 * 
 * Description: Retrieve the behavior of @dock_item.
 * 
 * Returns: The behavior of @dock_item.
 **/
GnomeDockItemBehavior
gnome_dock_item_get_behavior (GnomeDockItem *dock_item)
{
  g_return_val_if_fail (dock_item != NULL,
                        GNOME_DOCK_ITEM_BEH_NORMAL);
  g_return_val_if_fail (GNOME_IS_DOCK_ITEM (dock_item),
                        GNOME_DOCK_ITEM_BEH_NORMAL);

  return dock_item->behavior;
}



/* Private interface.  */

void
gnome_dock_item_grab_pointer (GnomeDockItem *item)
{
  GdkCursor *fleur;

  fleur = gdk_cursor_new (GDK_FLEUR);

  /* Hm, not sure this is the right thing to do, but it seems to work.  */
  while (gdk_pointer_grab (item->bin_window,
                           FALSE,
                           (GDK_BUTTON1_MOTION_MASK |
                            GDK_POINTER_MOTION_HINT_MASK |
                            GDK_BUTTON_RELEASE_MASK),
                           NULL,
                           fleur,
                           GDK_CURRENT_TIME) != 0);

  gdk_cursor_destroy (fleur);
}

gboolean
gnome_dock_item_detach (GnomeDockItem *item, gint x, gint y)
{
  GtkRequisition requisition;
  GtkAllocation allocation;
  GtkOrientation pre_drag_orientation;

  if (item->behavior & GNOME_DOCK_ITEM_BEH_NEVER_FLOATING)
    return FALSE;

  item->float_x = x;
  item->float_y = y;

  gnome_dock_item_set_floating (item, TRUE);

  if (! GTK_WIDGET_REALIZED (item))
    return TRUE;

  pre_drag_orientation = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (item),
							       "gnome_dock_item_pre_drag_orientation"));
  gnome_dock_item_set_orientation (item, pre_drag_orientation);

  gtk_widget_size_request (GTK_WIDGET (item), &requisition);

  gdk_window_move_resize (item->float_window,
                          x, y, requisition.width, requisition.height);

  gdk_window_reparent (item->bin_window, item->float_window, 0, 0);
  gdk_window_set_hints (item->float_window, x, y, 0, 0, 0, 0, GDK_HINT_POS);

  gdk_window_show (item->float_window);
  item->float_window_mapped = TRUE;

  allocation.x = allocation.y = 0;
  allocation.width = requisition.width;
  allocation.height = requisition.height;
  gtk_widget_size_allocate (GTK_WIDGET (item), &allocation);

  gdk_window_hide (GTK_WIDGET (item)->window);
  gnome_dock_item_draw (GTK_WIDGET (item), NULL);

  gdk_window_set_transient_for(item->float_window,
                               gdk_window_get_toplevel(GTK_WIDGET (item)->window));

  return TRUE;
}

void
gnome_dock_item_attach (GnomeDockItem *item, GtkWidget *parent, gint x, gint y)
{
  if (GTK_WIDGET (item)->parent != GTK_WIDGET (parent))
    {
      gdk_window_move_resize (GTK_WIDGET (item)->window, -1, -1, 0, 0);
      gtk_widget_reparent (GTK_WIDGET (item), parent);

      gdk_window_hide (item->float_window);

      gdk_window_reparent (item->bin_window, GTK_WIDGET (item)->window, 0, 0);
      gdk_window_show (GTK_WIDGET (item)->window);

      item->float_window_mapped = FALSE;
      gnome_dock_item_set_floating (item, FALSE);

      gtk_widget_queue_resize (GTK_WIDGET (item));

      gnome_dock_item_grab_pointer (item);
    }
}

void
gnome_dock_item_drag_floating (GnomeDockItem *item, gint x, gint y)
{
  if (item->is_floating)
    {
      gdk_window_move (item->float_window, x, y);
      gdk_window_raise (item->float_window);

      item->float_x = x;
      item->float_y = y;
    }
}

void
gnome_dock_item_handle_size_request (GnomeDockItem *item,
                                     GtkRequisition *requisition)
{
  GtkBin *bin;
  GtkContainer *container;

  bin = GTK_BIN (item);
  container = GTK_CONTAINER (item);

  if (bin->child != NULL)
    gtk_widget_size_request (bin->child, requisition);

  if (item->orientation == GTK_ORIENTATION_HORIZONTAL)
    requisition->width += DRAG_HANDLE_SIZE;
  else
    requisition->height += DRAG_HANDLE_SIZE;

  requisition->width += container->border_width * 2;
  requisition->height += container->border_width * 2;
}

void
gnome_dock_item_get_floating_position (GnomeDockItem *item,
                                       gint *x, gint *y)
{
  if (GTK_WIDGET_REALIZED (item) && item->is_floating)
    gdk_window_get_position (item->float_window, x, y);
  else
    {
      *x = item->float_x;
      *y = item->float_y;
    }
}
