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

#ifndef __GTK_CALENDAR_H__
#define __GTK_CALENDAR_H__

#include <gdk/gdk.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkwidget.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GTK_CALENDAR(obj)          GTK_CHECK_CAST (obj, gtk_calendar_get_type (), GtkCalendar)
#define GTK_CALENDAR_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gtk_calendar_get_type (), GtkCalendarClass)
#define GTK_IS_CALENDAR(obj)       GTK_CHECK_TYPE (obj, gtk_calendar_get_type ())


typedef enum
{
  GTK_CALENDAR_SHOW_HEADING = 1,
  GTK_CALENDAR_SHOW_DAY_NAMES = 2,
} GtkCalendarDisplayOptions;

typedef enum
{
  GTK_CALENDAR_FONT_HEADING,
  GTK_CALENDAR_FONT_DAY_NAME,
  GTK_CALENDAR_FONT_DAY
} GtkCalendarFont;

typedef enum
{
  GTK_CALENDAR_COLOR_HEADING,
  GTK_CALENDAR_COLOR_DAY_NAME,
  GTK_CALENDAR_COLOR_PREV_MONTH,
  GTK_CALENDAR_COLOR_NEXT_MONTH,
  GTK_CALENDAR_COLOR_NORMAL_DAY
} GtkCalendarColor;

typedef struct _GtkCalendar            GtkCalendar;
typedef struct _GtkCalendarClass       GtkCalendarClass;

struct _GtkCalendar
{
  GtkWidget widget;

  GdkWindow *header_win, *day_name_win, *main_win;
  gint header_h, day_name_h, main_h;

  gint month; 
  gint year;
  gint selected_day;

  gint day_month[6][7];
  gint day[6][7];

  gint num_marked_dates;
  gint marked_date[31];
  GtkCalendarDisplayOptions  display_flags;
  GdkColor marked_date_color[31];

  /* Header Information */
  GdkWindow *arrow_win[4];
  gint       arrow_state[4];
  gint       arrow_width;
  gint       max_month_width;
  gint       max_year_width;

  /* Other info */
  gint calstarty, calnumrows;

  /* Style parameters for this widget */
  GdkGC *gc;
  GdkFont *month_font, *day_name_font, *day_font;
  GdkColor heading_color, day_name_color;
  GdkColor normal_day_color, prev_month_color;
  GdkColor next_month_color, default_marked_color;
  GdkColor cursor_colors[3];
  GdkColor background_color;
  GdkColor foreground_color;
  GdkColor highlight_back_color;
  GdkCursor *cross;

  gint day_width;
  GdkRectangle header_button[4];
  GdkRectangle rect_days[6][7];

  gint highlight_row;
  gint highlight_col;

  gint max_char_width;
};

struct _GtkCalendarClass
{
  GtkWidgetClass parent_class;

  /* Signal handlers */
  void (* gtk_calendar_month_changed) (GtkCalendarClass *);
  void (* gtk_calendar_day_selected) (GtkCalendarClass *);
  void (* gtk_calendar_day_selected_double_click) (GtkCalendarClass *);
  void (* gtk_calendar_prev_month) (GtkCalendarClass *);
  void (* gtk_calendar_next_month) (GtkCalendarClass *);
  void (* gtk_calendar_prev_year) (GtkCalendarClass *);
  void (* gtk_calendar_next_year) (GtkCalendarClass *);
 
};


guint      gtk_calendar_get_type     	(void);
GtkWidget* gtk_calendar_new          	(void);

gint       gtk_calendar_select_month 	(GtkCalendar *calendar, 
                                      	 gint month, gint year);
void       gtk_calendar_select_day   	(GtkCalendar *calendar, gint day);

gint       gtk_calendar_mark_day     	(GtkCalendar *calendar, gint day);
gint       gtk_calendar_unmark_day   	(GtkCalendar *calendar, gint day);
void       gtk_calendar_clear_marks  	(GtkCalendar *calendar);

/* Not yet implemented:
void       gtk_calendar_set_font      	(GtkCalendar *calendar, 
                                 	 GtkCalendarFont type, 
                                 	 GdkFont *font);
void       gtk_calendar_set_color     	(GtkCalendar *calendar, 
                                 	 GtkCalendarColor type, 
                                	 GdkColor *color);
					 */

void	   gtk_calendar_display_options (GtkCalendar *calendar,
                                         GtkCalendarDisplayOptions flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_CALENDAR_H__ */
