/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GTK Calendar Widget
 * Copyright (C) 1998 Cesar Miquel and Shawn T. Amundson
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <config.h>
#include "libgnome/libgnome.h"
#include "gtkcalendar.h"
#include "libgnome/lib_date.h"

#define HELVETICA_10_FONT  "-adobe-helvetica-bold-r-normal-*-10-*-*-*-*-*-*-*"
#define FIXED_10_FONT      "-b&h-lucidatypewriter-medium-r-normal-*-10-*-*-*-*-*-*-*"


#define CALENDAR_MARGIN          4
#define CALENDAR_YSEP            4

#define DAY_XPAD		 2
#define DAY_YPAD		 2
#define DAY_XSEP                 0 /* not really good for small calendar */
#define DAY_YSEP		 0 /* not really good for small calendar */

enum {
  ARROW_YEAR_LEFT,
  ARROW_YEAR_RIGHT,
  ARROW_MONTH_LEFT,
  ARROW_MONTH_RIGHT
};

enum {
  MONTH_PREV,
  MONTH_CURRENT,
  MONTH_NEXT
};

static GdkColor black = {0, 0, 0, 0}, 
  green = {0, 0, 50000, 0}, 
  blue = {0, 0, 0, 65535}, 
  gray25 = {0, 16384, 16384, 16384}, 
  gray50 = {0, 32768, 32768, 32768}, 
  gray75 = {0, 49152, 49152, 49152}, 
  gray90 = {0, 58981, 58981, 58981}, 
  white = {0, 65535, 65535, 65535};


static GtkWidgetClass *parent_class = NULL;

enum {
  MONTH_CHANGED_SIGNAL,
  DAY_SELECTED_SIGNAL,
  DAY_SELECTED_DOUBLE_CLICK_SIGNAL,
  PREV_MONTH_SIGNAL,
  NEXT_MONTH_SIGNAL,
  PREV_YEAR_SIGNAL,
  NEXT_YEAR_SIGNAL,
  LAST_SIGNAL
};

static gint gtk_calendar_signals[LAST_SIGNAL] = { 0 };

typedef void (*GtkCalendarSignalDate) (GtkObject *object, guint arg1, guint arg2, guint arg3, gpointer data);

static void gtk_calendar_class_init     (GtkCalendarClass *class);
static void gtk_calendar_init           (GtkCalendar *gcal);
static void gtk_calendar_realize        (GtkWidget *widget);
static void gtk_calendar_size_request   (GtkWidget *widget, 
		                         GtkRequisition *requisition);
static void gtk_calendar_size_allocate  (GtkWidget *widget, 
		                         GtkAllocation *allocation);
static gint gtk_calendar_expose         (GtkWidget *widget, 
		                         GdkEventExpose *event);
static gint gtk_calendar_button_press   (GtkWidget *widget, 
		                         GdkEventButton *event);
static gint gtk_calendar_header_button  (GtkWidget *widget, 
		                         GdkEventButton *event);
static void gtk_calendar_main_button    (GtkWidget *widget, 
		                         GdkEventButton *event);
static gint gtk_calendar_motion_notify  (GtkWidget  *widget, 
		                         GdkEventMotion *event);
static gint gtk_calendar_enter_notify   (GtkWidget  *widget, 
		                         GdkEventCrossing *event);
static gint gtk_calendar_leave_notify   (GtkWidget  *widget, 
		                         GdkEventCrossing *event);
static void gtk_calendar_paint          (GtkWidget *widget, GdkRectangle *area);
static int  gtk_calendar_paint_arrow    (GtkWidget *widget, guint arrow);
void gtk_calendar_paint_header          (GtkWidget *widget);
void gtk_calendar_paint_day_names       (GtkWidget *widget);
void gtk_calendar_paint_main            (GtkWidget *widget);
static void gtk_calendar_draw           (GtkWidget *widget, GdkRectangle *area);
static void gtk_calendar_map            (GtkWidget *widget);
static void gtk_calendar_unmap          (GtkWidget *widget);

static void gtk_calendar_compute_days (GtkCalendar *calendar);
static gint left_x_for_column (GtkCalendar *calendar, gint column);
static gint top_y_for_row (GtkCalendar *calendar, gint row);


guint
gtk_calendar_get_type ()
{
  static guint calendar_type = 0;
  
  if (!calendar_type)
    {
      GtkTypeInfo calendar_info =
      {
	"GtkCalendar",
	sizeof (GtkCalendar),
	sizeof (GtkCalendarClass),
	(GtkClassInitFunc) gtk_calendar_class_init,
	(GtkObjectInitFunc) gtk_calendar_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL,
      };
      
      calendar_type = gtk_type_unique (gtk_widget_get_type (), &calendar_info);
    }
  
  return calendar_type;
}

static void
gtk_calendar_class_init (GtkCalendarClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  
  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  
  parent_class = gtk_type_class (gtk_widget_get_type ());
  
  widget_class->realize = gtk_calendar_realize;
  widget_class->expose_event = gtk_calendar_expose;
  widget_class->draw = gtk_calendar_draw;
  widget_class->size_request = gtk_calendar_size_request;
  widget_class->size_allocate = gtk_calendar_size_allocate;
  widget_class->button_press_event = gtk_calendar_button_press;
  widget_class->motion_notify_event = gtk_calendar_motion_notify;             
  widget_class->enter_notify_event = gtk_calendar_enter_notify;             
  widget_class->leave_notify_event = gtk_calendar_leave_notify;             
  widget_class->map = gtk_calendar_map;
  widget_class->unmap = gtk_calendar_unmap;
 
  gtk_calendar_signals[MONTH_CHANGED_SIGNAL] = 
    gtk_signal_new ("month_changed",
		    GTK_RUN_FIRST, object_class->type, 
		    GTK_SIGNAL_OFFSET (GtkCalendarClass, gtk_calendar_month_changed),
		    gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
  gtk_calendar_signals[DAY_SELECTED_SIGNAL] = 
    gtk_signal_new ("day_selected",
		    GTK_RUN_FIRST, object_class->type, 
		    GTK_SIGNAL_OFFSET (GtkCalendarClass, gtk_calendar_day_selected),
		    gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
  gtk_calendar_signals[DAY_SELECTED_DOUBLE_CLICK_SIGNAL] = 
    gtk_signal_new ("day_selected_double_click",
		    GTK_RUN_FIRST, object_class->type, 
		    GTK_SIGNAL_OFFSET (GtkCalendarClass, gtk_calendar_day_selected_double_click),
		    gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
  gtk_calendar_signals[PREV_MONTH_SIGNAL] = 
    gtk_signal_new ("prev_month",
		    GTK_RUN_FIRST, object_class->type, 
		    GTK_SIGNAL_OFFSET (GtkCalendarClass, gtk_calendar_prev_month),
		    gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
  gtk_calendar_signals[NEXT_MONTH_SIGNAL] = 
    gtk_signal_new ("next_month",
		    GTK_RUN_FIRST, object_class->type, 
		    GTK_SIGNAL_OFFSET (GtkCalendarClass, gtk_calendar_next_month),
		    gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
  gtk_calendar_signals[PREV_YEAR_SIGNAL] = 
    gtk_signal_new ("prev_year",
		    GTK_RUN_FIRST, object_class->type, 
		    GTK_SIGNAL_OFFSET (GtkCalendarClass, gtk_calendar_prev_year),
		    gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
  gtk_calendar_signals[NEXT_YEAR_SIGNAL] = 
    gtk_signal_new ("next_year",
		    GTK_RUN_FIRST, object_class->type, 
		    GTK_SIGNAL_OFFSET (GtkCalendarClass, gtk_calendar_next_year),
		    gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, gtk_calendar_signals, LAST_SIGNAL);


  class->gtk_calendar_month_changed = NULL;
  class->gtk_calendar_day_selected = NULL;
  class->gtk_calendar_day_selected_double_click = NULL;
  class->gtk_calendar_prev_month = NULL;
  class->gtk_calendar_next_month = NULL;
  class->gtk_calendar_prev_year = NULL;
  class->gtk_calendar_next_year = NULL;

  return; 
} 

static void
gtk_calendar_map (GtkWidget *widget)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_CALENDAR (widget));

  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
  gdk_window_show (widget->window);
}

static void
gtk_calendar_unmap (GtkWidget *widget)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_CALENDAR (widget));

  GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
  gdk_window_hide (widget->window);
}

static void
gtk_calendar_init (GtkCalendar *gcal)
{
  time_t secs;
  struct tm *tm;
  GdkColormap *cmap;
  GdkFont *fixed, *heading;
  gint i;
  gint max_char_width;
  gint str_width;
  gchar buffer[255];

  /* Set default fonts */
  heading = gdk_font_load (HELVETICA_10_FONT);
  fixed   = gdk_font_load (FIXED_10_FONT);

  /* Set defaults */
  secs = time(NULL);
  tm = localtime(&secs);
  gcal->month = tm->tm_mon;
  gcal->year  = 1900 + tm->tm_year;

  /* Set style default style for widget */

  /* Alloc colors in color map */
  cmap = gdk_colormap_get_system ();
  gdk_color_alloc (cmap, &black);
  gdk_color_alloc (cmap, &green);
  gdk_color_alloc (cmap, &blue);
  gdk_color_alloc (cmap, &gray25);
  gdk_color_alloc (cmap, &gray50);
  gdk_color_alloc (cmap, &gray75);
  gdk_color_alloc (cmap, &gray90);
  gdk_color_alloc (cmap, &white);

  /* Set default colors */
  gcal->default_marked_color = blue;
  gcal->heading_color = black;
  gcal->day_name_color = blue;
  gcal->normal_day_color = black;
  gcal->prev_month_color = gray75;
  gcal->next_month_color = gray75;
  gcal->cursor_colors[0] = gray25;
  gcal->cursor_colors[1] = gray50;
  gcal->cursor_colors[2] = gray75;
  gcal->foreground_color = black;
  gcal->background_color = white;
  gcal->highlight_back_color = gray90;


  gcal->month_font = heading;
  gcal->day_name_font = heading;
  gcal->day_font = fixed;

  for(i=0;i<31;i++)
    gcal->marked_date[i] = FALSE;

  gcal->arrow_width = 16;

  /* Create cursor */
  /* gcal->cross = gdk_cursor_new (GDK_PLUS); */

  gcal->highlight_row = -1;
  gcal->highlight_col = -1;

  max_char_width = 0;
  for (i = 0; i < 9; i++) {
    sprintf (buffer, "%d", i);
    str_width = gdk_string_measure (gcal->day_font, buffer);
    if (str_width > max_char_width)
      max_char_width = str_width;
  };
  gcal->max_char_width = max_char_width;
  gcal->dirty = 1;
  gcal->frozen = 0;
}

GtkWidget*
gtk_calendar_new ()
{
  return GTK_WIDGET (gtk_type_new (gtk_calendar_get_type ()));
} 

/* column_from_x: returns the column 0-6 that the 
 * x pixel of the xwindow is in */
static gint
column_from_x(GtkCalendar *calendar, gint event_x) {
  gint c, column;
  gint x_left, x_right;

  column = -1;
 
  for (c = 0; c < 7; c++) {
    x_left = left_x_for_column(calendar, c);
    x_right = x_left + calendar->day_width;

    if (event_x > x_left && event_x < x_right) {
      column = c;
      break;
    }
  }

  return column;
}

/* row_from_y: returns the row 0-5 that the 
 * y pixel of the xwindow is in */
static gint
row_from_y(GtkCalendar *calendar, gint event_y) {
  gint r, row;
  gint height;
  gint y_top, y_bottom;
  
  height = calendar->day_font->ascent + DAY_YPAD*2;
  row = -1;

  for (r = 0; r < 6; r++) {
    y_top = top_y_for_row(calendar, r);
    y_bottom = y_top + height;

    if (event_y > y_top && event_y < y_bottom) {
      row = r;
      break;
    }
  }

  return row;
}

/* left_x_for_column: returns the x coordinate
 * for the left of the column */
static gint
left_x_for_column(GtkCalendar *calendar, gint column) {
  gint width;
  gint x_left;

  width = calendar->day_width;
  x_left = CALENDAR_MARGIN + (width + DAY_YSEP) * column;

  return x_left;
}

/* top_y_for_row: returns the y coordinate
 * for the top of the row */
static gint
top_y_for_row(GtkCalendar *calendar, gint row) {
  gint height;
  gint y_top;

  height = calendar->day_font->ascent + DAY_YPAD*2;
  y_top = CALENDAR_MARGIN + row * (height + DAY_YSEP);

  return y_top;
}

/* This function should be done by the toolkit, but we don't like the
 * GTK arrows because they don't look good on this widget */
static void
draw_arrow_right(GdkWindow *window, GdkGC *gc, gint x, gint y, gint size)
{ 
  gint i;

  for (i = 0; i <= size / 2; i++) {
    gdk_draw_line(window, gc, 
                  x + i,
                  y + i,
                  x + i, 
                  y + size - i);
  }
}

/* This function should be done by the toolkit, but we don't like the
 * GTK arrows because they don't look good on this widget */
static void
draw_arrow_left(GdkWindow *window, GdkGC *gc, gint x, gint y, gint size)
{ 
  gint i;

  for (i = 0; i <= size / 2; i++) {
    gdk_draw_line(window, gc, 
                  x + size/2 - i,
                  y + i,
                  x + size/2 - i, 
                  y + size - i);
  }
}

static void
gtk_calendar_set_month_prev (GtkCalendar *calendar) {
	
     if (calendar->display_flags & GTK_CALENDAR_NO_MONTH_CHANGE)
       return;
	
     if (calendar->month == 0) {
	  calendar->month = 11;
	  calendar->year--;
      } else {
	  calendar->month--;
      }

      gtk_signal_emit (GTK_OBJECT (calendar), 
  		       gtk_calendar_signals[PREV_MONTH_SIGNAL]);
      gtk_signal_emit (GTK_OBJECT (calendar), 
  		       gtk_calendar_signals[MONTH_CHANGED_SIGNAL]);

      gtk_calendar_paint (GTK_WIDGET(calendar), NULL);
}

static void
gtk_calendar_set_month_next (GtkCalendar *calendar) {
	
     if (calendar->display_flags & GTK_CALENDAR_NO_MONTH_CHANGE)
       return;
	
     if (calendar->month == 11) {
         calendar->month = 0;
         calendar->year++;
     } else {
 	 calendar->month++; 
     }
     gtk_signal_emit (GTK_OBJECT (calendar), 
  		      gtk_calendar_signals[NEXT_MONTH_SIGNAL]);
     gtk_signal_emit (GTK_OBJECT (calendar), 
  	              gtk_calendar_signals[MONTH_CHANGED_SIGNAL]);

     gtk_calendar_paint (GTK_WIDGET(calendar), NULL);
}

static void
gtk_calendar_set_year_prev (GtkCalendar *calendar) {
    calendar->year--;
    gtk_signal_emit (GTK_OBJECT (calendar), 
  		     gtk_calendar_signals[PREV_YEAR_SIGNAL]);
    gtk_signal_emit (GTK_OBJECT (calendar), 
  	             gtk_calendar_signals[MONTH_CHANGED_SIGNAL]);
    gtk_calendar_paint (GTK_WIDGET(calendar), NULL);
}

static void
gtk_calendar_set_year_next (GtkCalendar *calendar) {
    calendar->year++;
    gtk_signal_emit (GTK_OBJECT (calendar), 
  		     gtk_calendar_signals[NEXT_YEAR_SIGNAL]);
    gtk_signal_emit (GTK_OBJECT (calendar), 
  	             gtk_calendar_signals[MONTH_CHANGED_SIGNAL]);
    gtk_calendar_paint (GTK_WIDGET(calendar), NULL);
}

static void
gtk_calendar_main_button (GtkWidget      *widget,
			    GdkEventButton *event)
{
  GtkCalendar *gcal;
  gint x, y;
  gint row, col;
  gint day_month;

  gcal = GTK_CALENDAR (widget);
   
  x = (gint) (event->x);
  y = (gint) (event->y);
   
  row = row_from_y(gcal, y); 
  col = column_from_x(gcal, x); 

  day_month = gcal->day_month[row][col];

  if (day_month == MONTH_CURRENT) {  
    if (event->type == GDK_2BUTTON_PRESS) 
      gtk_signal_emit (GTK_OBJECT (gcal), 
		      gtk_calendar_signals[DAY_SELECTED_DOUBLE_CLICK_SIGNAL]);
    else
      gtk_calendar_select_day(gcal, gcal->day[row][col]);
  } else if (day_month == MONTH_PREV) {
      gtk_calendar_set_month_prev(gcal);
  } else if (day_month == MONTH_NEXT) {
      gtk_calendar_set_month_next(gcal);
  }
}

static void
gtk_calendar_realize (GtkWidget *widget)
{
  GtkCalendar *gcal;
  GdkWindowAttr attributes;
  gint attributes_mask;
  gint i;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_CALENDAR (widget));
  
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  gcal = GTK_CALENDAR (widget);

  /* Main Window -------------------------  */
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget) |
    GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
    GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK; 
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (widget->parent->window, 
				   &attributes, attributes_mask);

  /* Header window ------------------------------------- */
  if (gcal->display_flags & GTK_CALENDAR_SHOW_HEADING) 
    {
      attributes.x = 0;
      attributes.y = 0;
      attributes.width = widget->allocation.width;
      attributes.height = gcal->header_h;
      gcal->header_win = gdk_window_new (widget->window, 
					 &attributes, attributes_mask);
      
      gdk_window_set_background (gcal->header_win, &widget->style->bg[GTK_STATE_NORMAL]);
      gdk_window_show (gcal->header_win);
      gdk_window_set_user_data (gcal->header_win, widget);

      attributes.x = 3;
      attributes.y = 3;
      attributes.width = gcal->arrow_width;
      attributes.height = gcal->header_h - 7;
      for (i = 0; i < 4; i++) {
	switch (i)
	  {
	  case ARROW_MONTH_LEFT:
	    attributes.x = 3;
	    break;
	  case ARROW_MONTH_RIGHT:
	    attributes.x = gcal->arrow_width + gcal->max_month_width;
	    break;
	  case ARROW_YEAR_LEFT:
	    attributes.x = widget->allocation.width - 
                            (3 + 2*gcal->arrow_width + gcal->max_year_width);
	    break;
	  case ARROW_YEAR_RIGHT:
	    attributes.x = widget->allocation.width - 3 - gcal->arrow_width;
	    break;
	  }

        gcal->arrow_win[i] = gdk_window_new (gcal->header_win, 
					    &attributes, attributes_mask);
        gcal->arrow_state[i] = GTK_STATE_NORMAL;
        gdk_window_set_background (gcal->arrow_win[i], 
                                   &widget->style->bg[GTK_STATE_NORMAL]);
        gdk_window_show (gcal->arrow_win[i]);
        gdk_window_set_user_data (gcal->arrow_win[i], widget);
      }

    } 
  else 
    {
      gcal->header_win = NULL;
      for (i = 0; i < 4; i++) {
        gcal->arrow_win[i] = NULL;
      }
    }

  /* Day names  window --------------------------------- */
  if ( gcal->display_flags & GTK_CALENDAR_SHOW_DAY_NAMES)
    {
      attributes.x = 0;
      attributes.y = gcal->header_h;
      attributes.width = widget->allocation.width;
      attributes.height = gcal->day_name_h;
      gcal->day_name_win = gdk_window_new (widget->window, 
					   &attributes, attributes_mask);
      gdk_window_set_background (gcal->day_name_win, &gcal->background_color);
      gdk_window_show (gcal->day_name_win);
      gdk_window_set_user_data (gcal->day_name_win, widget);
    }
  else
    gcal->day_name_win = NULL;

  /* Days window --------------------------------------- */
  attributes.x = 0;
  attributes.y = gcal->header_h + gcal->day_name_h;
  attributes.width = widget->allocation.width;
  attributes.height = gcal->main_h;
  gcal->main_win = gdk_window_new (widget->window, 
				   &attributes, attributes_mask);
  gdk_window_set_background (gcal->main_win,  &gcal->background_color);
  gdk_window_show (gcal->main_win);


 
  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);

  gdk_window_set_user_data (widget->window, widget);
  gdk_window_set_user_data (gcal->main_win, widget);

  /* Set widgets gc */
  gcal->gc = gdk_gc_new (widget->window);

}                                      

static void
gtk_calendar_size_request (GtkWidget      *widget,
			   GtkRequisition *requisition)
{
  GtkCalendar *cal;
  gint height, width;
  gint font_width_day, font_width_day_name, font_width;
  gint max_month_width, str_width;
  gint i;
  gchar buffer[255];

  cal = GTK_CALENDAR (widget);

  /* Calculate calendar size */
  if (cal->display_flags & GTK_CALENDAR_SHOW_HEADING)
    cal->header_h = cal->month_font->ascent + cal->month_font->descent + 
                     CALENDAR_YSEP * 2 + 4;
  else
    cal->header_h = 0;
 
  if (cal->display_flags & GTK_CALENDAR_SHOW_DAY_NAMES)
    cal->day_name_h = cal->day_name_font->ascent + 
                       cal->day_name_font->descent + 10;
  else
    cal->day_name_h = 0;

  cal->main_h = CALENDAR_MARGIN*2 + 6 * (cal->day_font->ascent + DAY_YPAD*2) + DAY_YSEP*5;

  font_width_day = gdk_string_measure (cal->day_font, "WW");
  font_width_day_name = gdk_string_measure (cal->day_name_font, "WW");

  if (font_width_day > font_width_day_name)
    font_width = font_width_day;
  else
    font_width = font_width_day_name;

  height = cal->header_h + cal->day_name_h + cal->main_h;
  width =  7 * (font_width + DAY_XPAD*2) + (DAY_XSEP * 6) + CALENDAR_MARGIN*2;

  requisition->width = width;
  requisition->height = height;

  /* Calculate max month string width */
  max_month_width = 0;
  for (i = 1; i <= 12; i++) {
    sprintf (buffer, "%s", _(month_name[i]));
    str_width = gdk_string_measure (cal->month_font, buffer);
    if (str_width > max_month_width)
      max_month_width = str_width;
  };
  max_month_width += 8;
  cal->max_month_width = max_month_width;

  cal->max_year_width = gdk_string_measure (cal->month_font, "8888") + 8;
}    

static void
gtk_calendar_size_allocate (GtkWidget     *widget,
			      GtkAllocation *allocation)
{
  GtkCalendar *cal;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_CALENDAR (widget));
  g_return_if_fail (allocation != NULL);
  
  widget->allocation = *allocation;

  cal = GTK_CALENDAR (widget);
  
  cal->day_width = (allocation->width - (CALENDAR_MARGIN * 2) - (DAY_XSEP * 6))/7;
  
  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
                              allocation->width, allocation->height);
      if (GTK_CALENDAR(widget)->display_flags & GTK_CALENDAR_SHOW_HEADING)
        {
           gdk_window_move_resize (cal->header_win,
			      0, 0,
                              allocation->width, cal->header_h);
           gdk_window_move_resize (cal->arrow_win[ARROW_MONTH_LEFT],
			      3, 3, 
                              cal->arrow_width, 
                              cal->header_h - 7);
           gdk_window_move_resize (cal->arrow_win[ARROW_MONTH_RIGHT],
			      cal->arrow_width + cal->max_month_width, 3, 
                              cal->arrow_width, 
                              cal->header_h - 7);
           gdk_window_move_resize (cal->arrow_win[ARROW_YEAR_LEFT],
			      allocation->width - 
                               (3 + 2*cal->arrow_width + cal->max_year_width), 
                              3, 
                              cal->arrow_width, 
                              cal->header_h - 7);
           gdk_window_move_resize (cal->arrow_win[ARROW_YEAR_RIGHT],
			      allocation->width - 3 - cal->arrow_width, 3, 
                              cal->arrow_width, 
                              cal->header_h - 7);
	}
      gdk_window_move_resize (cal->day_name_win,
			      0, cal->header_h,
                              allocation->width, cal->day_name_h);
      gdk_window_move_resize (cal->main_win,
			      0, cal->header_h + cal->day_name_h,
                              allocation->width, cal->main_h);
    }
}

static gint
gtk_calendar_expose (GtkWidget      *widget,
                     GdkEventExpose *event)
{
  GtkCalendar *calendar;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_CALENDAR (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  calendar = GTK_CALENDAR (widget);

  if (GTK_WIDGET_DRAWABLE (widget)) {
    if (event->window == calendar->main_win) 
      gtk_calendar_paint_main (widget);

    if (event->window == calendar->header_win) 
      gtk_calendar_paint_header (widget);

    if (event->window == calendar->day_name_win) 
      gtk_calendar_paint_day_names (widget);
  }

  return FALSE;
}

static void
gtk_calendar_draw (GtkWidget    *widget,
                   GdkRectangle *area)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_CALENDAR (widget));
  g_return_if_fail (area != NULL);

  if (GTK_WIDGET_DRAWABLE (widget))
    gtk_calendar_paint (widget, area);

}


static void
gtk_calendar_paint (GtkWidget    *widget,
                    GdkRectangle *area)
{
  GtkCalendar *calendar;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (widget->window != NULL);
  g_return_if_fail (GTK_IS_CALENDAR (widget));

  calendar = GTK_CALENDAR (widget);

  /* Clear main window */
  gdk_window_clear (widget->window);

  if (calendar->header_win != NULL)
    gtk_calendar_paint_header (widget);  

  if (calendar->day_name_win != NULL)
    gtk_calendar_paint_day_names (widget);  

  if (calendar->main_win != NULL)
    gtk_calendar_paint_main (widget);  

}

void
gtk_calendar_paint_header (GtkWidget *widget)
{
  GtkCalendar *calendar;
  GdkGC *gc;
  char buffer[255];
  int y, y_arrow;
  gint cal_width, cal_height;
  gint str_width;
  gint max_month_width;
  gint max_year_width;

  calendar = GTK_CALENDAR (widget);
  gc = calendar->gc;

  /* Clear window */
  gdk_window_clear (calendar->header_win);
 
  cal_width = widget->allocation.width;
  cal_height = widget->allocation.height;
 
  max_month_width = calendar->max_month_width;
  max_year_width = calendar->max_year_width;

  gdk_gc_set_foreground (gc, &calendar->background_color); 
  gtk_draw_shadow(widget->style, calendar->header_win,
                  GTK_STATE_NORMAL, GTK_SHADOW_OUT,
                  0, 0, cal_width, calendar->header_h);


  /* Draw title */
  y = calendar->header_h - (calendar->header_h - calendar->month_font->ascent
      + calendar->month_font->descent) / 2;
  y_arrow = (calendar->header_h - 9) / 2;

  /* Draw year and its arrows */
  sprintf (buffer, "%d", calendar->year);
  str_width = gdk_string_measure (calendar->day_name_font, buffer);
  gdk_gc_set_foreground (gc, &calendar->heading_color);
  gdk_draw_string (calendar->header_win, calendar->month_font, gc, 
                   cal_width - (3 + calendar->arrow_width + max_year_width - (max_year_width - str_width)/2), 
                   y, buffer);

  /* Draw month */ 
  sprintf (buffer, "%s", _(month_name[calendar->month + 1]));
  str_width = gdk_string_measure (calendar->month_font, buffer);
  gdk_draw_string (calendar->header_win, calendar->month_font, gc, 
                   3 + calendar->arrow_width + (max_month_width - str_width)/2, 
                   y, buffer);

  y += CALENDAR_YSEP + calendar->month_font->descent;

  gdk_gc_set_foreground (gc, &calendar->background_color);

  gtk_calendar_paint_arrow (widget, ARROW_MONTH_LEFT);  
  gtk_calendar_paint_arrow (widget, ARROW_MONTH_RIGHT);  
  gtk_calendar_paint_arrow (widget, ARROW_YEAR_LEFT);  
  gtk_calendar_paint_arrow (widget, ARROW_YEAR_RIGHT);  

}

void
gtk_calendar_paint_day_names (GtkWidget *widget)
{
  GtkCalendar *calendar;
  GdkGC *gc;
  char buffer[255];
  int y, h;
  int day;
  int day_width, day_height, cal_width;
  gint cal_height;
  int day_wid_sep;
  int str_width;
  static char *dayinletters[] =
  {N_("Su"), N_("Mo"), N_("Tu"), N_("We"), N_("Th"), N_("Fr"), N_("Sa")};

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_CALENDAR (widget));

  calendar = GTK_CALENDAR (widget);
  gc = calendar->gc;

  /* Clear window */
  gdk_window_clear (calendar->day_name_win);
  
  day_width = calendar->day_width;
  day_height = calendar->day_font->ascent;
  cal_width = widget->allocation.width;
  cal_height = widget->allocation.height;
  day_wid_sep = day_width + DAY_XSEP;
 
  h = calendar->day_name_font->ascent;
  y = h + 5;

  gdk_gc_set_foreground (gc, &calendar->day_name_color);
  gdk_gc_set_background (gc, &calendar->day_name_color);
  gdk_draw_rectangle (calendar->day_name_win, gc, TRUE,
							 CALENDAR_MARGIN, 2, 
							 day_wid_sep * 7, h+6);

  gdk_gc_set_foreground (gc, &calendar->background_color);
  for (day = 0; day < 7; day++)
    {
      sprintf (buffer, "%s", dayinletters[day]);
      str_width = gdk_string_measure (calendar->day_name_font, buffer);
      gdk_draw_string (calendar->day_name_win, calendar->day_name_font, 
                       gc, CALENDAR_MARGIN + day_wid_sep * day + 
		       (day_width - str_width)/2, y, buffer);
    }

}

static void
gtk_calendar_paint_day (GtkWidget *widget, gint row, gint col)
{
  GtkCalendar *calendar;
  GdkGC *gc;
  gchar buffer[255];
  gint day;
  gint day_height;
  gint max_char_width;
  gint str_width;
  gint x_left;
  gint x_loc;
  gint y_top;
  gint y_baseline;
  gint day_xspace;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_CALENDAR (widget));
  g_return_if_fail (row < 6);
  g_return_if_fail (col < 7);

  calendar = GTK_CALENDAR (widget);

  day_height = calendar->day_font->ascent + DAY_YPAD*2;

  /* this should not be in this function */
  max_char_width = calendar->max_char_width;

  day_xspace = calendar->day_width - max_char_width*2;

  day = calendar->day[row][col];
  sprintf (buffer, "%d", day);
  str_width = max_char_width;
  if (day > 9)
    str_width *= 2;

  x_left = left_x_for_column(calendar, col);
  x_loc = x_left + calendar->day_width - (str_width + day_xspace/2);

  y_top = top_y_for_row(calendar, row);
  y_baseline = y_top + day_height/2 + calendar->day_font->ascent/2;

  gdk_window_clear_area(calendar->main_win, x_left, y_top,
    		        calendar->day_width, day_height);

  gc = calendar->gc;
  if (calendar->day_month[row][col] == MONTH_PREV) {
    gdk_gc_set_foreground (gc, &calendar->prev_month_color);
  } else if (calendar->day_month[row][col] == MONTH_NEXT) {
    gdk_gc_set_foreground (gc, &calendar->next_month_color);
  } else {
    if (calendar->highlight_row == row && calendar->highlight_col == col) {
      gdk_gc_set_foreground (gc, &calendar->highlight_back_color);
      gdk_draw_rectangle(calendar->main_win, gc, TRUE, x_left, y_top,
    		         calendar->day_width, day_height);
    }

    if (calendar->selected_day == day) {
      gdk_gc_set_foreground (gc, &calendar->normal_day_color);
      gdk_draw_rectangle(calendar->main_win, gc, FALSE, x_left, y_top,
    		         calendar->day_width-1, day_height-1);
    }

    if (calendar->marked_date[day-1])
      gdk_gc_set_foreground (gc, &calendar->default_marked_color);
    else 
      gdk_gc_set_foreground (gc, &calendar->normal_day_color);
  }
  gdk_draw_string (calendar->main_win, 
                   calendar->day_font, gc, 
                   x_loc, y_baseline, buffer);

}


void
gtk_calendar_paint_main (GtkWidget *widget)
{
  GtkCalendar *calendar;
  gint row, col;
 
  g_return_if_fail (widget != NULL);
  g_return_if_fail (widget->window != NULL);
  g_return_if_fail (GTK_IS_CALENDAR (widget));

  calendar = GTK_CALENDAR (widget);
  if (calendar->frozen)
    return;
  calendar->dirty = 0;
  gdk_window_clear (calendar->main_win);

  gtk_calendar_compute_days(calendar); /* REMOVE later */

  for (col = 0; col < 7; col++) 
    for (row = 0; row < 6; row++)
      gtk_calendar_paint_day(widget, row, col);
}

static void
gtk_calendar_compute_days (GtkCalendar *calendar)
{
  gint month;
  gint year;
  gint ndays_in_month;
  gint ndays_in_prev_month;
  gint first_day;
  gint row;
  gint col;
  gint day;

  g_return_if_fail (calendar != NULL);
  g_return_if_fail (GTK_IS_CALENDAR (calendar));

  year = calendar->year;
  month = calendar->month + 1;
  
  ndays_in_month = month_length[leap(year)][month];

  first_day = day_of_week(year, month, 1);
  if (first_day == 7)
    first_day = 0;

  /* Compute days of previous month */ 
  if (month > 1)
    ndays_in_prev_month = month_length[leap(year)][month-1];
  else
    ndays_in_prev_month = month_length[leap(year)][12];
  day = ndays_in_prev_month - first_day + 1;

  row = 0;
  if (first_day > 0) {
    for (col = 0; col < first_day; col++) {
      calendar->day[row][col] = day;
      calendar->day_month[row][col] = MONTH_PREV;
      day++;
    }
  }

  /* Compute days of current month */ 
  col = first_day;
  for (day = 1; day <= ndays_in_month; day++) {
    calendar->day[row][col] = day;
    calendar->day_month[row][col] = MONTH_CURRENT;

    col++;
    if (col == 7) {
      row++;
      col = 0;
    }
  }

  /* Compute days of next month */ 
  day = 1;
  for (; row <= 5; row++) {
     for (; col <= 6; col++) {
       calendar->day[row][col] = day;
       calendar->day_month[row][col] = MONTH_NEXT;
       day++;
     }
     col = 0;
  }
}

/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_display_options
   DESCRIPTION:	Set display options (whether to display the
                heading and the month headings)

                 flags is can be an XOR of 
                   GTK_CALENDAR_SHOW_HEADING
                   GTK_CALENDAR_SHOW_DAY_NAMES.
   ---------------------------------------------------------------------- */

void gtk_calendar_display_options (GtkCalendar *calendar,
				   GtkCalendarDisplayOptions flags)
{
  calendar->display_flags = flags;
}

gint
gtk_calendar_select_month (GtkCalendar *calendar, gint month, gint year)
{
  g_return_val_if_fail (calendar != NULL, FALSE);
  g_return_val_if_fail (month >= 0 && month <= 11, FALSE);

  calendar->month = month;
  calendar->year  = year;

  gtk_calendar_compute_days (calendar);
  
  if (GTK_WIDGET_DRAWABLE (GTK_WIDGET(calendar)))
    gtk_calendar_paint(GTK_WIDGET(calendar), NULL);

  gtk_signal_emit (GTK_OBJECT (calendar), 
       	           gtk_calendar_signals[MONTH_CHANGED_SIGNAL]);
  return TRUE;
}

void
gtk_calendar_select_day (GtkCalendar *calendar, gint day)
{
  gint row, r;
  gint col, c;
  g_return_if_fail (calendar != NULL);

  gtk_calendar_compute_days (calendar);
  if (calendar->selected_day > 0) {
    row = -1;
    col = -1;

    for (r = 0; r < 6; r++) 
      for (c = 0; c < 7; c++) 
        if (calendar->day_month[r][c] == MONTH_CURRENT
            && calendar->day[r][c] == calendar->selected_day) {
          row = r;
          col = c;
        }

    g_return_if_fail (row != -1);
    g_return_if_fail (col != -1);

    calendar->selected_day = 0;
    if (GTK_WIDGET_DRAWABLE (GTK_WIDGET(calendar)))
      gtk_calendar_paint_day(GTK_WIDGET(calendar), row, col);
  }

  calendar->selected_day = day;
	
  if (day != 0)
    {
       row = -1;
       col = -1;
       for (r = 0; r <6; r++) 
         for (c = 0; c < 7; c++) 
           if (calendar->day_month[r][c] == MONTH_CURRENT &&
               calendar->day[r][c] == day) {
             row = r;
             col = c;
           }

      g_return_if_fail (row != -1);
      g_return_if_fail (col != -1);

      if (GTK_WIDGET_DRAWABLE (GTK_WIDGET(calendar)))
        gtk_calendar_paint_day(GTK_WIDGET(calendar), row, col);
  }

  gtk_signal_emit (GTK_OBJECT (calendar), 
                   gtk_calendar_signals[DAY_SELECTED_SIGNAL]);
}

void
gtk_calendar_clear_marks (GtkCalendar *calendar)
{
  gint day;

  g_return_if_fail (calendar != NULL);

  for (day = 0; day < 31; day ++) {
    calendar->marked_date[day] = FALSE;
  }

  if (GTK_WIDGET_DRAWABLE (GTK_WIDGET(calendar))){
    calendar->dirty = 1;
    gtk_calendar_paint_main(GTK_WIDGET(calendar));
  }
}

gint
gtk_calendar_mark_day (GtkCalendar *calendar, gint day)
{ 
  g_return_val_if_fail (calendar != NULL, FALSE);
  
  if (day>=1 && day <=31)  
    calendar->marked_date[day-1] = TRUE;

  if (GTK_WIDGET_DRAWABLE (GTK_WIDGET(calendar))){
    calendar->dirty = 1;	  
    gtk_calendar_paint_main(GTK_WIDGET(calendar));
  }
  return TRUE;
}

gint
gtk_calendar_unmark_day (GtkCalendar *calendar, gint day)
{
  g_return_val_if_fail (calendar != NULL, FALSE);
  
  if (day>=1 && day <=31)
    calendar->marked_date[day-1] = FALSE;

  if (GTK_WIDGET_DRAWABLE (GTK_WIDGET(calendar))){
    calendar->dirty = 1;
    gtk_calendar_paint_main(GTK_WIDGET(calendar));
  }
  return TRUE;
}

void
gtk_calendar_get_date (GtkCalendar *calendar, gint *year, gint *month, gint *day)
{
  g_return_if_fail (calendar != NULL);
  g_return_if_fail (GTK_IS_CALENDAR (calendar));

  if (year)
	  *year = calendar->year;

  if (month)
	  *month = calendar->month;

  if (day)
	  *day = calendar->selected_day;
}

static gint
gtk_calendar_button_press (GtkWidget      *widget,
			   GdkEventButton *event)
{
  GtkCalendar *gcal;
  gint x, y;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_CALENDAR (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  gcal = GTK_CALENDAR (widget);
   
  x = (gint) (event->x);
  y = (gint) (event->y);

  if (event->window == gcal->arrow_win[ARROW_MONTH_LEFT])
      gtk_calendar_set_month_prev(gcal);

  if (event->window == gcal->arrow_win[ARROW_MONTH_RIGHT])
      gtk_calendar_set_month_next(gcal);

  if (event->window == gcal->arrow_win[ARROW_YEAR_LEFT]) 
      gtk_calendar_set_year_prev(gcal);

  if (event->window == gcal->arrow_win[ARROW_YEAR_RIGHT]) 
      gtk_calendar_set_year_next(gcal);

  if (event->window == gcal->main_win)
    gtk_calendar_main_button (widget, event);

  return FALSE;
}

static gint
gtk_calendar_motion_notify (GtkWidget      *widget,
			    GdkEventMotion *event)
{
  GtkCalendar *cal;
  gint event_x, event_y;
  gint row, col;
  gint old_row, old_col;

  cal = (GtkCalendar*) widget;
  event_x = (gint) (event->x);
  event_y = (gint) (event->y);

  if (event->window == cal->main_win) {

    row = row_from_y(cal, event_y);
    col = column_from_x(cal, event_x);

    if (row != cal->highlight_row || cal->highlight_col != col) {
      old_row = cal->highlight_row;
      old_col = cal->highlight_col;
      if (old_row > -1 && old_col > -1) {
        cal->highlight_row = -1;
        cal->highlight_col = -1;
        gtk_calendar_paint_day(widget, old_row, old_col);
      }

      cal->highlight_row = row;
      cal->highlight_col = col;

      if (row > -1 && col > -1)
        gtk_calendar_paint_day(widget, row, col);
    }
  }
  return TRUE;
}      

static gint
gtk_calendar_enter_notify (GtkWidget      *widget,
		           GdkEventCrossing *event)
{
  GtkCalendar *cal;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  cal = GTK_CALENDAR(widget); 

  if (event->window == cal->arrow_win[ARROW_MONTH_LEFT]) {
    cal->arrow_state[ARROW_MONTH_LEFT] = GTK_STATE_PRELIGHT;
    gtk_calendar_paint_arrow(widget, ARROW_MONTH_LEFT);
  }

  if (event->window == cal->arrow_win[ARROW_MONTH_RIGHT]) {
    cal->arrow_state[ARROW_MONTH_RIGHT] = GTK_STATE_PRELIGHT;
    gtk_calendar_paint_arrow(widget, ARROW_MONTH_RIGHT);
  }

  if (event->window == cal->arrow_win[ARROW_YEAR_LEFT]) {
    cal->arrow_state[ARROW_YEAR_LEFT] = GTK_STATE_PRELIGHT;
    gtk_calendar_paint_arrow(widget, ARROW_YEAR_LEFT);
  }

  if (event->window == cal->arrow_win[ARROW_YEAR_RIGHT]) {
    cal->arrow_state[ARROW_YEAR_RIGHT] = GTK_STATE_PRELIGHT;
    gtk_calendar_paint_arrow(widget, ARROW_YEAR_RIGHT);
  }

  return TRUE;
}

static gint
gtk_calendar_leave_notify (GtkWidget      *widget,
		           GdkEventCrossing *event)
{
  GtkCalendar *cal;
  gint row;
  gint col;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  cal = GTK_CALENDAR(widget); 

  if (event->window == cal->main_win) {
    row = cal->highlight_row;
    col = cal->highlight_col;
    cal->highlight_row = -1;
    cal->highlight_col = -1;
    if (row > -1 && col > -1)
      gtk_calendar_paint_day(widget, row, col);
  }

  if (event->window == cal->arrow_win[ARROW_MONTH_LEFT]) {
    cal->arrow_state[ARROW_MONTH_LEFT] = GTK_STATE_NORMAL;
    gtk_calendar_paint_arrow(widget, ARROW_MONTH_LEFT);
  }

  if (event->window == cal->arrow_win[ARROW_MONTH_RIGHT]) {
    cal->arrow_state[ARROW_MONTH_RIGHT] = GTK_STATE_NORMAL;
    gtk_calendar_paint_arrow(widget, ARROW_MONTH_RIGHT);
  }

  if (event->window == cal->arrow_win[ARROW_YEAR_LEFT]) {
    cal->arrow_state[ARROW_YEAR_LEFT] = GTK_STATE_NORMAL;
    gtk_calendar_paint_arrow(widget, ARROW_YEAR_LEFT);
  }

  if (event->window == cal->arrow_win[ARROW_YEAR_RIGHT]) {
    cal->arrow_state[ARROW_YEAR_RIGHT] = GTK_STATE_NORMAL;
    gtk_calendar_paint_arrow(widget, ARROW_YEAR_RIGHT);
  }

  return TRUE;
}

static int
gtk_calendar_paint_arrow (GtkWidget *widget, guint arrow)
{
  GdkWindow *window;
  GdkGC *gc;
  GtkCalendar *cal;
  gint state;
  gint width, height;

  g_return_val_if_fail (widget != NULL, FALSE);

  cal = GTK_CALENDAR(widget);
  window = cal->arrow_win[arrow];
  state  = cal->arrow_state[arrow];
  gc = cal->gc;

  gdk_window_clear (window);
  gdk_window_set_background (window,
                             &widget->style->bg[state]);
  gdk_window_get_size(window, &width, &height);
  gdk_window_clear_area (window,
                         0,0,
                         width,height);

  gdk_gc_set_foreground (gc, &cal->foreground_color);

  if (arrow == ARROW_MONTH_LEFT || arrow == ARROW_YEAR_LEFT) {
    draw_arrow_left(window, gc, width/2 - 3, height/2 - 4, 8);
  } else {
    draw_arrow_right(window, gc, width/2 - 2, height/2 - 4, 8);
  }
  return FALSE;   
}

void
gtk_calendar_freeze (GtkCalendar *calendar)
{
  g_return_if_fail (GTK_IS_CALENDAR (calendar));
  calendar->frozen++;
}

void
gtk_calendar_thaw (GtkCalendar *calendar)
{
  g_return_if_fail (GTK_IS_CALENDAR (calendar));
  if (calendar->frozen){
      calendar->frozen--;
      if (calendar->frozen)
        return;
      if (calendar->dirty){
	if (GTK_WIDGET_DRAWABLE (calendar))
	  gtk_calendar_paint_main (GTK_WIDGET (calendar));
      }
  }
}
