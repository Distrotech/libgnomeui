/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1999 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */

/* gtkdial.c: Written by Owen Taylor <otaylor@redhat.com> */

/* needed for M_* under 'gcc -ansi -pedantic' on GNU/Linux */
#ifndef _BSD_SOURCE
#  define _BSD_SOURCE 1
#endif
#include <sys/types.h>

#include <math.h>
#include <stdio.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>

#include "gtkdial.h"

#define SCROLL_DELAY_LENGTH  300
#define DIAL_MIN_SIZE        75   /* the absoulte smallest the dial can be. */
#define DIAL_MAX_SIZE        200  /* How big this thing should get. */
#define DIAL_DEFAULT_SIZE    75   /* The default size */
#define DIAL_NUM_TICKS       25   /* How many ticks to draw */
/* Forward declararations */

static void gtk_dial_class_init               (GtkDialClass      *klass);
static void gtk_dial_init                     (GtkDial           *dial);
static void gtk_dial_destroy                  (GtkObject         *object);
static void gtk_dial_realize                  (GtkWidget         *widget);
static void gtk_dial_size_request             (GtkWidget         *widget,
					       GtkRequisition    *requisition);
static void gtk_dial_size_allocate            (GtkWidget         *widget,
					       GtkAllocation     *allocation);
static gint gtk_dial_expose                   (GtkWidget         *widget,
					       GdkEventExpose    *event);
static void gtk_dial_make_pixmap              (GtkDial           *dial);
static void gtk_dial_paint                    (GtkDial           *dial);
static gint gtk_dial_button_press             (GtkWidget         *widget,
					       GdkEventButton    *event);
static gint gtk_dial_button_release           (GtkWidget         *widget,
					       GdkEventButton    *event);
static gint gtk_dial_motion_notify            (GtkWidget         *widget,
					       GdkEventMotion    *event);
static gint gtk_dial_timer                    (GtkDial           *dial);

static void gtk_dial_update_mouse             (GtkDial *dial, gint x, gint y);
static void gtk_dial_update                   (GtkDial *dial);
static void gtk_dial_adjustment_changed       (GtkAdjustment    *adjustment,
						gpointer          data);
static void gtk_dial_adjustment_value_changed (GtkAdjustment    *adjustment,
						gpointer          data);

/* Local data */

static GtkWidgetClass *parent_class = NULL;

guint
gtk_dial_get_type ()
{
  static guint dial_type = 0;

  if (!dial_type)
    {
      GtkTypeInfo dial_info =
      {
	"GtkDial",
	sizeof (GtkDial),
	sizeof (GtkDialClass),
	(GtkClassInitFunc) gtk_dial_class_init,
	(GtkObjectInitFunc) gtk_dial_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL,
      };

      dial_type = gtk_type_unique (gtk_widget_get_type (), &dial_info);
    }

  return dial_type;
}

static void
gtk_dial_class_init (GtkDialClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_widget_get_type ());

  object_class->destroy = gtk_dial_destroy;

  /* Widget basics */
  widget_class->realize = gtk_dial_realize;
  widget_class->expose_event = gtk_dial_expose;
  widget_class->size_request = gtk_dial_size_request;
  widget_class->size_allocate = gtk_dial_size_allocate;
  /* Widget events */
  widget_class->button_press_event = gtk_dial_button_press;
  widget_class->button_release_event = gtk_dial_button_release;
  widget_class->motion_notify_event = gtk_dial_motion_notify;
}

static void
gtk_dial_init (GtkDial *dial)
{
  dial->button           = 0;
  dial->policy           = GTK_UPDATE_CONTINUOUS;
  dial->view_only        = FALSE;
  dial->timer            = 0;
  dial->radius           = 0;
  dial->pointer_width    = 0;
  dial->angle            = 0.0;
  dial->percentage       = 0.0;
  dial->old_value        = 0.0;
  dial->old_lower        = 0.0;
  dial->old_upper        = 0.0;
  dial->adjustment       = NULL;
  dial->offscreen_pixmap = NULL;
}

/**
 * gtk_dial_new
 * @adjustment: Pointer to GtkAdjustment object
 *
 * This function creates a new GtkDial widget, and ties it to a
 * specified GtkAdjustment. When the dial is moved, the adjustment is
 * updated, and vice-versa.
 *
 * Returns: Pointer to new GtkDial widget.
 **/
GtkWidget*
gtk_dial_new (GtkAdjustment *adjustment)
{
  GtkDial        *dial;

  dial = gtk_type_new (gtk_dial_get_type ());

  if (!adjustment)
    adjustment  = (GtkAdjustment*) gtk_adjustment_new (0.0, 0.0, 0.0, 
						      0.0, 0.0, 0.0);
    gtk_dial_set_adjustment (dial, adjustment);

  return GTK_WIDGET (dial);
}

static void
gtk_dial_destroy (GtkObject *object)
{
  GtkDial *dial;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_DIAL (object));

  dial = GTK_DIAL (object);

  if (dial->adjustment)
    gtk_object_unref (GTK_OBJECT (dial->adjustment));

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/**
 * gtk_dial_get_adjustment
 * @dial: Pointer to GtkDial widget
 *
 * Description: Retrieves the GtkAdjustment associated with the
 * GtkDial @dial.
 *
 * Returns: Pointer to GtkAdjustment object.
 **/
GtkAdjustment*
gtk_dial_get_adjustment (GtkDial *dial)
{
  g_return_val_if_fail (dial != NULL, NULL);
  g_return_val_if_fail (GTK_IS_DIAL (dial), NULL);

  return dial->adjustment;
}

/**
 * gtk_dial_set_view_only
 * @dial: Pointer to GtkDial widget
 * @view_only: TRUE to set dial to read-only, FALSE to edit.
 *
 * Description: Specifies whether or not the user is to be able to
 * edit the value represented by the dial widget. If @view_only is
 * TRUE, the dial will be set to view-only mode, and the user will not 
 * be able to edit it. If @view_only is FALSE, the user will be able
 * to change the value represented.
 **/
void gtk_dial_set_view_only (GtkDial *dial,
			     gboolean view_only)
{
  g_return_if_fail(dial != NULL);
  g_return_if_fail(GTK_IS_DIAL (dial));

  dial->view_only = view_only;
}
/**
 * gtk_dial_set_update_policy
 * @dial: Pointer to GtkDial widget
 * @policy: New policy type
 * 
 * Description: Sets the update policy of the GtkDial @dial to one of either
 * %GTK_UPDATE_CONTINUOUS, %GTK_UPDATE_DISCONTINUOUS, or
 * %GTK_UPDATE_DELAYED. Please see Gtk+ documentation for an
 * explanation of these values.
 **/
void
gtk_dial_set_update_policy (GtkDial      *dial,
			     GtkUpdateType  policy)
{
  g_return_if_fail (dial != NULL);
  g_return_if_fail (GTK_IS_DIAL (dial));

  dial->policy = policy;
}

/**
 * gtk_dial_set_adjustment
 * @dial: Pointer to GtkDial widget
 * @adjustment: Pointer to GtkAdjustment object
 *
 * Description: Associates a new GtkAdjustment with GtkDial @dial
 * widget. The old adjustment is removed and replaced with the new.
 **/
void
gtk_dial_set_adjustment (GtkDial      *dial,
			  GtkAdjustment *adjustment)
{
  g_return_if_fail (dial != NULL);
  g_return_if_fail (GTK_IS_DIAL (dial));

  if (dial->adjustment)
    {
      gtk_signal_disconnect_by_data (GTK_OBJECT (dial->adjustment), 
				     (gpointer) dial);
      gtk_object_unref (GTK_OBJECT (dial->adjustment));
    }

  dial->adjustment = adjustment;
  gtk_object_ref (GTK_OBJECT (dial->adjustment));

  gtk_signal_connect (GTK_OBJECT (adjustment), "changed",
		      (GtkSignalFunc) gtk_dial_adjustment_changed,
		      (gpointer) dial);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      (GtkSignalFunc) gtk_dial_adjustment_value_changed,
		      (gpointer) dial);

  dial->old_value = adjustment->value;
  dial->old_lower = adjustment->lower;
  dial->old_upper = adjustment->upper;

  gtk_dial_update (dial);
}
/**
 * gtk_dial_set_percentage
 * @dial: Pointer to GtkDial widget
 * @percent: New percentage
 *
 * Description: Sets the GtkDial's value to @percent of
 * @dial->adjustment->upper. The upper value is set when the
 * GtkAdjustment is created.
 *
 * Returns: New value of adjustment.
 **/
gfloat 
gtk_dial_set_percentage (GtkDial *dial, gfloat percent)
{
 
  g_return_val_if_fail (dial != NULL, 0.0);
  g_return_val_if_fail (GTK_IS_DIAL (dial), 0.0);

  if (percent <= 1.0)
    {
      dial->percentage = percent;
      dial->adjustment->value = percent * dial->adjustment->upper;
      gtk_dial_update (dial);
    }
  
  return dial->adjustment->value;
}

/**
 * gtk_dial_get_percentage
 * @dial: Pointer to GtkDial widget
 *
 * Description: Retrieves the current percentage held in the dial widget.
 *
 * Returns: Current percentage.
 **/
gfloat
gtk_dial_get_percentage (GtkDial *dial)
{
  g_return_val_if_fail (dial != NULL, 0.0);
  g_return_val_if_fail (GTK_IS_DIAL (dial), 0.0);

  return dial->percentage;
}

/**
 * gtk_dial_get_value
 * @dial: Pointer to GtkDial widget
 *
 * Description: Retrieves the current value helt in the dial widget.
 *
 * Returns: Current value
 **/
gfloat
gtk_dial_get_value (GtkDial *dial)
{
  g_return_val_if_fail (dial != NULL, 0.0);
  g_return_val_if_fail (GTK_IS_DIAL (dial), 0.0);

  return dial->adjustment->value;
}

/**
 * gtk_dial_set_value
 * @dial: Pointer to GtkDial widget
 * @value: New value
 *
 * Description: Sets the current value held in the GtkDial's
 * adjustment object to @value.
 *
 * Returns: New percentage of value to the adjustment's upper limit.
 **/
gfloat
gtk_dial_set_value (GtkDial *dial, gfloat value)
{
  g_return_val_if_fail (dial != NULL, 0.0);
  g_return_val_if_fail (GTK_IS_DIAL (dial), 0.0);

  if (value <= dial->adjustment->upper)
    {
      dial->adjustment->value = value;
      dial->percentage = value / dial->adjustment->upper;
      gtk_dial_update (dial);
    }
  return dial->percentage;
}
static void
gtk_dial_realize (GtkWidget *widget)
{
  GtkDial *dial;
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_DIAL (widget));

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  dial = GTK_DIAL (widget);

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget) | 
    GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | 
    GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
    GDK_POINTER_MOTION_HINT_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (widget->parent->window, &attributes, 
				   attributes_mask);

  widget->style = gtk_style_attach (widget->style, widget->window);

  gdk_window_set_user_data (widget->window, widget);

  gtk_style_set_background (widget->style, widget->window, GTK_STATE_ACTIVE);
  gtk_dial_make_pixmap (dial);
}

static void 
gtk_dial_size_request (GtkWidget      *widget,
		       GtkRequisition *requisition)
{
  if (requisition->width < DIAL_MIN_SIZE  ||
      requisition->height < DIAL_MIN_SIZE)
    {
      requisition->width  = DIAL_MIN_SIZE;
      requisition->height = DIAL_MIN_SIZE;
      return;
    }
  if (requisition->width > DIAL_MAX_SIZE ||
      requisition->height > DIAL_MAX_SIZE)
    {
      requisition->width = DIAL_MAX_SIZE;
      requisition->height = DIAL_MAX_SIZE;
      return;
    }
  
}

/*
 * In here I would like to restrict the size of the dial to specific max
 * size, when it gets to big it just looks silly, plus it slows down.
 */
static void
gtk_dial_size_allocate (GtkWidget     *widget,
			GtkAllocation *allocation)
{
  GtkDial *dial;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_DIAL (widget));
  g_return_if_fail (allocation != NULL);

  widget->allocation = *allocation;
  dial = GTK_DIAL (widget);

  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, 
			      allocation->height);
    }
  dial->radius = MIN (allocation->width, allocation->height) * 0.45;
  dial->pointer_width = dial->radius / 5;
  gtk_dial_make_pixmap (GTK_DIAL (widget));

}

static gint
gtk_dial_expose (GtkWidget      *widget,
		 GdkEventExpose *event)
{
  GtkDial *dial;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_DIAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (event->count > 0)
    return FALSE;
  
  if (GTK_WIDGET_DRAWABLE (widget))
   {
     dial = GTK_DIAL (widget);
     gdk_draw_pixmap (widget->window,
                      widget->style->black_gc,
                      dial->offscreen_pixmap,
                      0, 0, 0, 0,
                      widget->allocation.width, 
                      widget->allocation.height);
   }
  return FALSE;
}

static gint
gtk_dial_button_press (GtkWidget      *widget,
		       GdkEventButton *event)
{
  GtkDial *dial;
  gint dx, dy;
  double s, c;
  double d_parallel;
  double d_perpendicular;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_DIAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  dial = GTK_DIAL (widget);

  /* Determine if button press was within pointer region - we 
     do this by computing the parallel and perpendicular distance of
     the point where the mouse was pressed from the line passing through
     the pointer */
  
  dx = event->x - widget->allocation.width / 2;
  dy = widget->allocation.height / 2 - event->y;
  
  s = sin(dial->angle);
  c = cos(dial->angle);
  
  d_parallel = s*dy + c*dx;
  d_perpendicular = fabs(s*dx - c*dy);
  
  if (!dial->button &&
      (d_perpendicular < dial->pointer_width/2) &&
      (d_parallel > - dial->pointer_width))
    {
      gtk_grab_add (widget);

      dial->button = event->button;
      gtk_dial_update_mouse (dial, event->x, event->y);
    }

  return FALSE;
}

static gint
gtk_dial_button_release (GtkWidget      *widget,
			  GdkEventButton *event)
{
  GtkDial *dial;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_DIAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  dial = GTK_DIAL (widget);

  if (dial->button == event->button)
    {
      gtk_grab_remove (widget);

      dial->button = 0;

      if (dial->policy == GTK_UPDATE_DELAYED)
	gtk_timeout_remove (dial->timer);
      
      if ((dial->policy != GTK_UPDATE_CONTINUOUS) &&
	  (dial->old_value != dial->adjustment->value))
	gtk_signal_emit_by_name (GTK_OBJECT (dial->adjustment), "value_changed");
    }

  return FALSE;
}

static gint
gtk_dial_motion_notify (GtkWidget      *widget,
			 GdkEventMotion *event)
{
  GtkDial *dial;
  GdkModifierType mods;
  gint x, y, mask;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_DIAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  dial = GTK_DIAL (widget);

  if (dial->button != 0)
    {
      x = event->x;
      y = event->y;

      if (event->is_hint || (event->window != widget->window))
	gdk_window_get_pointer (widget->window, &x, &y, &mods);

      switch (dial->button)
	{
	case 1:
	  mask = GDK_BUTTON1_MASK;
	  break;
	case 2:
	  mask = GDK_BUTTON2_MASK;
	  break;
	case 3:
	  mask = GDK_BUTTON3_MASK;
	  break;
	default:
	  mask = 0;
	  break;
	}

      if (mods & mask)
	gtk_dial_update_mouse (dial, x,y);
    }

  return FALSE;
}

static gint
gtk_dial_timer (GtkDial *dial)
{
  GDK_THREADS_ENTER();
  
  g_return_val_if_fail (dial != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_DIAL (dial), FALSE);

  if (dial->policy == GTK_UPDATE_DELAYED)
    gtk_signal_emit_by_name (GTK_OBJECT (dial->adjustment), "value_changed");

  GDK_THREADS_LEAVE();

  return FALSE;
}

static void
gtk_dial_update_mouse (GtkDial *dial, gint x, gint y)
{
  gint xc, yc;
  gfloat old_value;

  g_return_if_fail (dial != NULL);
  g_return_if_fail (GTK_IS_DIAL (dial));

  if(dial->view_only)
    return;

  xc = GTK_WIDGET(dial)->allocation.width / 2;
  yc = GTK_WIDGET(dial)->allocation.height / 2;

  old_value = dial->adjustment->value;
  dial->angle = atan2(yc-y, x-xc);

  if (dial->angle < -M_PI/2.)
    dial->angle += 2*M_PI;

  if (dial->angle < -M_PI/6)
    dial->angle = -M_PI/6;

  if (dial->angle > 7.*M_PI/6.)
    dial->angle = 7.*M_PI/6.;

  dial->adjustment->value = dial->adjustment->lower + (7.*M_PI/6 - dial->angle) *
    (dial->adjustment->upper - dial->adjustment->lower) / (4.*M_PI/3.);

  if (dial->adjustment->value != old_value)
    {
      if (dial->policy == GTK_UPDATE_CONTINUOUS)
	{
	  gtk_signal_emit_by_name (GTK_OBJECT (dial->adjustment), 
                                   "value_changed");
	}
      else
	{
          gtk_dial_paint (dial);

	  if (dial->policy == GTK_UPDATE_DELAYED)
	    {
	      if (dial->timer)
		gtk_timeout_remove (dial->timer);

	      dial->timer = gtk_timeout_add (SCROLL_DELAY_LENGTH,
					     (GtkFunction) gtk_dial_timer,
					     (gpointer) dial);
	    }
	}
    }
}

static void
gtk_dial_update (GtkDial *dial)
{
  gfloat new_value;
  
  g_return_if_fail (dial != NULL);
  g_return_if_fail (GTK_IS_DIAL (dial));

  new_value = dial->adjustment->value;
  
  if (new_value < dial->adjustment->lower)
    new_value = dial->adjustment->lower;

  if (new_value > dial->adjustment->upper)
    new_value = dial->adjustment->upper;

  if (new_value != dial->adjustment->value)
    {
      dial->adjustment->value = new_value;
      gtk_signal_emit_by_name (GTK_OBJECT (dial->adjustment), "value_changed");
    }
  /*
   * save the percentage of the values..
   * ( remember to * 100.00 to get the usual representation of %)
   */
  dial->percentage = dial->adjustment->value / dial->adjustment->upper;
  dial->angle = 7.*M_PI/6. - (new_value - dial->adjustment->lower) * 
    4.*M_PI/3. / (dial->adjustment->upper - dial->adjustment->lower);
  gtk_dial_paint (dial);
}

static void
gtk_dial_adjustment_changed (GtkAdjustment *adjustment,
			      gpointer       data)
{
  GtkDial *dial;

  g_return_if_fail (adjustment != NULL);
  g_return_if_fail (data != NULL);

  dial = GTK_DIAL (data);

  if ((dial->old_value != adjustment->value) ||
      (dial->old_lower != adjustment->lower) ||
      (dial->old_upper != adjustment->upper))
    {
      gtk_dial_update (dial);

      dial->old_value = adjustment->value;
      dial->old_lower = adjustment->lower;
      dial->old_upper = adjustment->upper;
    }
}

static void
gtk_dial_adjustment_value_changed (GtkAdjustment *adjustment,
				    gpointer       data)
{
  GtkDial *dial;

  g_return_if_fail (adjustment != NULL);
  g_return_if_fail (data != NULL);

  dial = GTK_DIAL (data);

  if (dial->old_value != adjustment->value)
    {
      gtk_dial_update (dial);

      dial->old_value = adjustment->value;
    }
}

static void
gtk_dial_make_pixmap (GtkDial *dial)
{
  GtkWidget *widget;

  g_return_if_fail (dial != NULL);
  g_return_if_fail (GTK_IS_DIAL (dial));

  if (GTK_WIDGET_REALIZED (dial))
   {
      widget = GTK_WIDGET (dial);
      if (dial->offscreen_pixmap)
        gdk_pixmap_unref (dial->offscreen_pixmap);

      dial->offscreen_pixmap = gdk_pixmap_new (widget->window,
                                               widget->allocation.width,
                                               widget->allocation.height,
                                               -1);
      gdk_window_set_back_pixmap (widget->window, 
                                  dial->offscreen_pixmap, FALSE);
      gdk_draw_rectangle (dial->offscreen_pixmap,
                          widget->style->bg_gc[GTK_STATE_NORMAL],
                          TRUE,
                          0, 0,
                          widget->allocation.width,
                          widget->allocation.height);
      gtk_dial_paint (dial);
   }

}

static void
gtk_dial_paint (GtkDial *dial)
{
  GtkWidget *widget;
  GdkPoint  points[3];
  GdkGC     *gc1;
  GdkGC     *gc2;
  gdouble   s, c, theta;
  gint      xc, yc;
  gint      tick_length;
  gint      i, t_x, t_y;
  char      buf[20];

  g_return_if_fail (dial != NULL);
  g_return_if_fail (GTK_IS_DIAL (dial));

  if (dial->offscreen_pixmap)
   {
     widget = GTK_WIDGET (dial);
     gdk_draw_rectangle (dial->offscreen_pixmap,
			 widget->style->bg_gc[GTK_STATE_NORMAL],
			 TRUE,
			 0, 0,
			 widget->allocation.width,
			 widget->allocation.height);

     xc = (widget->allocation.width)/2;
     yc = (widget->allocation.height)/2;

     for (i = 0; i < DIAL_NUM_TICKS; i++)
      {
          theta = (i * M_PI/18. - M_PI/6.);
          s     = sin(theta);
          c     = cos(theta);
          tick_length = (i % 6 == 0) ? dial->pointer_width : 
                                       dial->pointer_width/2;
          gdk_draw_line (dial->offscreen_pixmap,
                         widget->style->fg_gc[widget->state],
                         xc + c * (dial->radius - tick_length),
                         yc - s * (dial->radius - tick_length),
                         xc + c * dial->radius,
                         yc - s * dial->radius);
      }

     t_x = xc + c * dial->radius;
     t_y = yc - s * dial->radius;
     g_snprintf (buf, sizeof(buf), "%3.0f%%", 
	         (dial->percentage * 100.0));

     i = gdk_string_width (widget->style->font, buf) / 2;
     gdk_draw_string (dial->offscreen_pixmap,
		      widget->style->font,
		      widget->style->black_gc,
		      xc - i,
		      t_y + (tick_length /2) + widget->style->font->ascent + 1,
		      buf);

     s = sin(dial->angle);
     c = cos(dial->angle);
     /* 
      * Build the pointer
      */
     points[0].x = xc + s*dial->pointer_width/2;
     points[0].y = yc + c*dial->pointer_width/2;
     points[1].x = xc + c*dial->radius;
     points[1].y = yc - s*dial->radius;
     points[2].x = xc - s*dial->pointer_width/2;
     points[2].y = yc - c*dial->pointer_width/2;
     gdk_draw_polygon (dial->offscreen_pixmap,
                       widget->style->fg_gc[widget->state],
                       TRUE,
                       points, 3);
     /*
      * Our own frame. (Can't use gtk cause this is our own pixmap)
      */
     gc1 = widget->style->light_gc[GTK_STATE_NORMAL];
     gc2 = widget->style->dark_gc[GTK_STATE_NORMAL];
     /* 
      * Bottom and right side
      */
     gdk_draw_line (dial->offscreen_pixmap,
		    gc1,
		    0, 
		    widget->allocation.height - 1,
		    widget->allocation.width -1,
		    widget->allocation.height - 1);
     gdk_draw_line (dial->offscreen_pixmap,
		    gc1,
		    widget->allocation.width -1,
		    0,
		    widget->allocation.width -1,
		    widget->allocation.height - 1 );
     gdk_draw_line (dial->offscreen_pixmap,
		    widget->style->bg_gc[GTK_STATE_NORMAL],
		    1,
		    widget->allocation.height - 2,
		    widget->allocation.width - 2,
		    widget->allocation.height - 2);
     gdk_draw_line (dial->offscreen_pixmap,
		    widget->style->bg_gc[GTK_STATE_NORMAL],
		    widget->allocation.width - 2,
		    1,
		    widget->allocation.width - 2,
		    widget->allocation.height - 2);
     /*
      * Top and left side
      */
     gdk_draw_line (dial->offscreen_pixmap,
		    widget->style->black_gc,
		    0,
		    0,
		    widget->allocation.width - 1, 
		    0);
     gdk_draw_line (dial->offscreen_pixmap,
		    widget->style->black_gc,
		    0,
		    0,
		    0,
		    widget->allocation.height - 1);
     gdk_draw_line (dial->offscreen_pixmap,
		    gc2,
		    1, 1,
		    widget->allocation.width - 2,
		    1);
     gdk_draw_line (dial->offscreen_pixmap,
		    gc2,
		    1, 1, 1,
		    widget->allocation.height - 2);
     gtk_widget_draw (widget, NULL);
     gdk_window_clear (widget->window);
  }
}

/* EOF */
