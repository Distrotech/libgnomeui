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
#include "gtkcalendar.h"
#include "libgnome/lib_date.h"

#define HELVETICA_10_FONT  "-adobe-helvetica-bold-r-normal-*-10-*-*-*-*-*-*-*"
#define FIXED_10_FONT      "-b&h-lucidatypewriter-medium-r-normal-*-10-*-*-*-*-*-*-*"

/*#define CALENDAR_XSEP            3*/
#define CALENDAR_XSEP            1
#define CALENDAR_YSEP            4
#define BASELINESKIP             4
#define CALENDAR_MARGIN          8

#define THISMONTH                1
#define OTHERMONTH               2
#define MARKEDDATE               3

enum {
  ARROW_YEAR_LEFT,
  ARROW_YEAR_RIGHT,
  ARROW_MONTH_LEFT,
  ARROW_MONTH_RIGHT
};

/*
 *       ----------------
 *       Global variables
 *       ----------------
 */

/* GdkCursor *cross; */
int calstarty, calnumrows;
int pointerrow, pointercol;
GdkColor black = {0, 0, 0, 0}, 
  green = {0, 0, 50000, 0}, 
  blue = {0, 0, 0, 65535}, 
  gray25 = {0, 16384, 16384, 16384}, 
  gray50 = {0, 32768, 32768, 32768}, 
  gray75 = {0, 49152, 49152, 49152}, 
  white = {0, 65535, 65535, 65535};


static GtkWidgetClass *parent_class = NULL;


/* Signals -------------------------------------------- */

enum {
  MONTH_CHANGED_SIGNAL, LAST_SIGNAL };

/*
  YEAR_CHANGED_SIGNAL,
  DAY_SELECTED_SIGNAL,
  LAST_SIGNAL
};*/

static gint gtk_calendar_signals[LAST_SIGNAL] = { 0 };
                                          
/*
 *    -------------------
 *    Function prototypes
 *    -------------------
 */

static void gtk_calendar_class_init (GtkCalendarClass *class);
static void gtk_calendar_init (GtkCalendar *gcal);
static void gtk_calendar_realize (GtkWidget *widget);
static void gtk_calendar_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void gtk_calendar_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static gint gtk_calendar_expose (GtkWidget *widget, GdkEventExpose *event);
static gint gtk_calendar_button_press (GtkWidget *widget, GdkEventButton *event);
static gint gtk_calendar_header_button (GtkWidget *widget, GdkEventButton *event);
static gint gtk_calendar_main_button (GtkWidget *widget, GdkEventButton *event);
static gint gtk_calendar_button_release (GtkWidget *widget, GdkEventButton *event);
static gint gtk_calendar_motion_notify (GtkWidget  *widget, GdkEventMotion *event);
static gint gtk_calendar_enter_notify (GtkWidget  *widget, GdkEventCrossing *event);
static gint gtk_calendar_leave_notify (GtkWidget  *widget, GdkEventCrossing *event);
static void gtk_calendar_paint (GtkWidget *widget, GdkRectangle *area);
static void gtk_calendar_paint_arrow (GtkWidget *widget, guint arrow);
void gtk_calendar_paint_header (GtkWidget *widget);
void gtk_calendar_paint_day_names (GtkWidget *widget);
void gtk_calendar_paint_main (GtkWidget *widget);
static void gtk_calendar_draw (GtkWidget *widget, GdkRectangle *area);
static void gtk_calendar_map (GtkWidget *widget);
static void gtk_calendar_unmap (GtkWidget *widget);



/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_det_type
   ---------------------------------------------------------------------- */

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


/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_class_init
   ---------------------------------------------------------------------- */

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
  widget_class->button_release_event = gtk_calendar_button_release;
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

  gtk_object_class_add_signals (object_class, gtk_calendar_signals, LAST_SIGNAL);

  class->gtk_calendar_month_changed = NULL;

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


/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_init
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

static void
gtk_calendar_init (GtkCalendar *gcal)
{
  time_t secs;
  struct tm *tm;
  GdkColormap *cmap;
  GdkFont *fixed, *heading;
  gint i;

  /* Set default fonts */
  heading = gdk_font_load (HELVETICA_10_FONT);
  fixed   = gdk_font_load (FIXED_10_FONT);

  /* Set defaults */
  secs = time(NULL);
  tm = localtime(&secs);
  gcal->month = tm->tm_mon+1;
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


  gcal->month_font = heading;
  gcal->day_name_font = heading;
  gcal->day_font = fixed;

  for(i=0;i<31;i++)
    gcal->marked_date[i] = FALSE;

  gcal->arrow_width = 16;

  /* Create cursor */
  /* gcal->cross = gdk_cursor_new (GDK_PLUS); */

}

/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_new
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

GtkWidget*
gtk_calendar_new ()
{
  return GTK_WIDGET (gtk_type_new (gtk_calendar_get_type ()));
} 

/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_realize
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

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
      attributes.y = gcal->header_h + 1;
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

/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_size_request
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

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
                       cal->day_name_font->descent + BASELINESKIP + 6;
  else
    cal->day_name_h = 0;

  cal->main_h = 6 * (cal->day_font->ascent + 
                 cal->day_font->descent + BASELINESKIP );

  font_width_day = gdk_string_measure (cal->day_font, "WWW");
  font_width_day_name = gdk_string_measure (cal->day_name_font, "WWW");
  /*printf("font width = %d %d\n", font_width_day, font_width_day_name);*/
  if (font_width_day > font_width_day_name)
    font_width = font_width_day;
  else
    font_width = font_width_day_name;

  height = cal->header_h + cal->day_name_h + cal->main_h;
  width =  7 * font_width + (CALENDAR_XSEP * 6) + CALENDAR_MARGIN*2;
  printf("req width: %d   req height: %d\n", width, height);
  requisition->width = width;
  requisition->height = height;

  /* Calculate max month string width */
  max_month_width = 0;
  for (i = 0; i < 12; i++) {
    sprintf (buffer, "%s", month_name[i]);
    str_width = gdk_string_measure (cal->month_font, buffer);
    if (str_width > max_month_width)
      max_month_width = str_width;
  };
  max_month_width += 8;
  cal->max_month_width = max_month_width;

  cal->max_year_width = gdk_string_measure (cal->month_font, "8888") + 8;
}    

/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_size_allocate
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

static void
gtk_calendar_size_allocate (GtkWidget     *widget,
			      GtkAllocation *allocation)
{
  GtkCalendar *cal;
  gint y_arrow;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_CALENDAR (widget));
  g_return_if_fail (allocation != NULL);
  
  widget->allocation = *allocation;


  if (GTK_WIDGET_REALIZED (widget))
    {
      cal = GTK_CALENDAR (widget);
      
      cal->day_width = (allocation->width - (CALENDAR_MARGIN * 2) - (CALENDAR_XSEP * 6))/7;
      y_arrow = (cal->header_h - 8) / 2;
      printf("--- header %d, y_arrow %d\n",cal->header_h, y_arrow);
      gdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
                              allocation->width, allocation->height);
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
      gdk_window_move_resize (cal->day_name_win,
			      0, cal->header_h,
                              allocation->width, cal->day_name_h);
      gdk_window_move_resize (cal->main_win,
			      0, cal->header_h + cal->day_name_h,
                              allocation->width, cal->main_h);
    }
}

/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_expose
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

static gint
gtk_calendar_expose (GtkWidget      *widget,
                     GdkEventExpose *event)
{
  GdkEventExpose child_event;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_CALENDAR (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (GTK_WIDGET_DRAWABLE (widget))
    gtk_calendar_paint (widget, &event->area);

  return FALSE;
}

/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_draw
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

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

/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_paint
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

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

/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_paint_header
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

void
gtk_calendar_paint_header (GtkWidget *widget)
{
  GtkCalendar *calendar;
  GdkRectangle *r;
  GdkGC *gc;
  char buffer[255];
  int y, y_arrow;
  gint cal_width, cal_height;
  gint i, str_width;
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
  sprintf (buffer, "%s", month_name[calendar->month]);
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

/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_paint_day_names
   DESCRIPTION:	
   ---------------------------------------------------------------------- */
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
  {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

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
  day_wid_sep = day_width + CALENDAR_XSEP;
 
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

/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_paint_main
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

void
gtk_calendar_paint_main (GtkWidget *widget)
{
  GtkCalendar *calendar;
  GdkGC *gc;
  char buffer[255];
  int i, y;
  int day;
  int ndays_in_month;
  int ndays_in_prev_month;
  int month, year;
  int first_day;
  int row, col;
  int day_width, day_height, cal_width, xloc;
  gint cal_height;
  int day_wid_sep;
  int str_width;
  gint max_char_width;
  gint max_month_width;
  gint day_xspace;
  gint py, pm;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (widget->window != NULL);
  g_return_if_fail (GTK_IS_CALENDAR (widget));

  calendar = GTK_CALENDAR (widget);
  gc = calendar->gc;

  /* Clear main window */
  gdk_window_clear (calendar->main_win);
  
  day_width = calendar->day_width;
  day_height = calendar->day_font->ascent;
  cal_width = widget->allocation.width;
  cal_height = widget->allocation.height;
  day_wid_sep = day_width + CALENDAR_XSEP;
 
  max_char_width = 0;
  for (i = 0; i < 9; i++) {
    sprintf (buffer, "%d", i);
    str_width = gdk_string_measure (calendar->day_font, buffer);
    if (str_width > max_char_width)
      max_char_width = str_width;
  };
  day_xspace = day_width - max_char_width*2;

  max_month_width = 0;
  for (i = 0; i < 12; i++) {
    sprintf (buffer, "%d", month_name[calendar->month]);
    str_width = gdk_string_measure (calendar->day_font, buffer);
    if (str_width > max_month_width)
      max_month_width = str_width;
  };


  gdk_gc_set_foreground (gc, &calendar->foreground_color);
  y = day_height + 3;
  calendar->calstarty = y;
  calendar->calnumrows = 0;
 
  year = calendar->year;
  month = calendar->month;
  ndays_in_month = month_length[leap(year)][month];
  col = 0;

  first_day = day_of_week(year, month, 1);
  if (first_day == 7)
    first_day = 0;
  
  first_in_week(week_number(year, month, 1)+2, &py, &pm, &day); 

  /* Draw days of previous month */ 
  if (month>1)
    ndays_in_prev_month = month_length[leap(year)][month-1];
  else
    ndays_in_prev_month = month_length[leap(year)][12];
  day = ndays_in_prev_month - first_day + 1;
  if (first_day > 0) {
    gdk_gc_set_foreground (gc, &calendar->prev_month_color);
    for (i=0; i < first_day; i++) {
      sprintf (buffer, "%d", day);
      str_width = max_char_width;
      if (day > 9)
        str_width *= 2;
      xloc =  CALENDAR_MARGIN + day_wid_sep * col + day_width - 
	      (str_width + day_xspace/2); 
      gdk_draw_string (calendar->main_win, 
                       calendar->day_font, gc, 
                       xloc, y, buffer);
      day++;
      col++;
    }
  }

  /* Draw this month dates */
  row = 0;
  col = first_day;
 
  for (i=1; i <= ndays_in_month; i++) {
    sprintf (buffer, "%d", i);
    str_width = max_char_width;
    if (i > 9)
      str_width *= 2;
    xloc =  CALENDAR_MARGIN + day_wid_sep * col + day_width - 
            (str_width + day_xspace/2);
    if (calendar->marked_date[i-1])
      gdk_gc_set_foreground (gc, &calendar->default_marked_color);
    else
      gdk_gc_set_foreground (gc, &calendar->normal_day_color);
    gdk_draw_string (calendar->main_win, 
                     calendar->day_font, gc, 
                     xloc, y, buffer);

    col++;
    if (col == 7) {
      row++;
      col = 0;
      y += day_height + BASELINESKIP;
    }
  }

  gdk_gc_set_foreground (gc, &calendar->next_month_color);
  day = 1;
  for (i = row; i <= 5; i++) {
    for (; col <= 6; col++) {
      sprintf (buffer, "%d", day);
      str_width = max_char_width;
      if (i > 9)
        str_width *= 2;
      xloc =  CALENDAR_MARGIN + day_wid_sep * col + day_width - 
              (str_width + day_xspace/2);
      gdk_draw_string (calendar->main_win, 
                       calendar->day_font, gc, 
                       xloc, y, buffer);
      day++;
    }
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

/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_select_month
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

gint
gtk_calendar_select_month (GtkCalendar *calendar, gint month, gint year)
{
  g_return_val_if_fail (calendar != NULL, FALSE);

  if (month<1 &&  month > 12)
    return FALSE;

  calendar->month = month;
  calendar->year  = year;
}


/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_mark_day
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

gint
gtk_calendar_mark_day (GtkCalendar *calendar, gint day)
{ 
  g_return_val_if_fail (calendar != NULL, FALSE);
  
  if (day>=1 && day <=31)  
    calendar->marked_date[day-1] = TRUE;
}

/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_unmark_day
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

gint
gtk_calendar_unmark_day (GtkCalendar *calendar, gint day)
{
  g_return_val_if_fail (calendar != NULL, FALSE);
  
  if (day>=1 && day <=31)
    calendar->marked_date[day-1] = FALSE;
}



/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_button_press
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

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

  if (event->window == gcal->arrow_win[ARROW_MONTH_LEFT]) {
      if (gcal->month == 1) {
	  gcal->month = 12;
	  gcal->year--;
      } else {
	  gcal->month--;
      }
      gtk_signal_emit (GTK_OBJECT (widget), 
  		       gtk_calendar_signals[MONTH_CHANGED_SIGNAL]);

      gtk_calendar_paint (widget , NULL);
  }

  if (event->window == gcal->arrow_win[ARROW_MONTH_RIGHT]) {
      if (gcal->month == 12) {
	  gcal->month = 1;
	  gcal->year++;
      } else {
 	  gcal->month++; 
      }
      gtk_signal_emit (GTK_OBJECT (widget), 
  		       gtk_calendar_signals[MONTH_CHANGED_SIGNAL]);

      gtk_calendar_paint (widget , NULL);
  }

  if (event->window == gcal->arrow_win[ARROW_YEAR_LEFT]) {
      gcal->year--;
      gtk_calendar_paint (widget , NULL);
  }

  if (event->window == gcal->arrow_win[ARROW_YEAR_RIGHT]) {
      gcal->year++;
      gtk_calendar_paint (widget , NULL);
  }

  if ( event->window == gcal->main_win )
    gtk_calendar_main_button (widget, event);

  printf("Button pressed at %d, %d of window %x\n", x, y, event->window);
 
  return FALSE;
}

/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_main_button
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

static gint
gtk_calendar_main_button (GtkWidget      *widget,
			    GdkEventButton *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_CALENDAR (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  
  
}



/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_button_release
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

static gint
gtk_calendar_button_release (GtkWidget      *widget,
			       GdkEventButton *event)
{
  GtkCalendar *gcal;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_CALENDAR (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  gcal = GTK_CALENDAR (widget);
  
  return FALSE;
}

/* ----------------------------------------------------------------------
   NAME:	gtk_calendar_motion_notify
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

static gint
gtk_calendar_motion_notify (GtkWidget      *widget,
			    GdkEventMotion *event)
{
  GtkCalendar *cal;
  gint x, y;

  cal = (GtkCalendar*) widget;
  x = (gint) (event->x);
  y = (gint) (event->y);

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

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  cal = GTK_CALENDAR(widget); 

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

static void
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
   
}

