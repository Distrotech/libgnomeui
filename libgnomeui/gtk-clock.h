/*
 * Copyright (C) 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */


/*
 * gtk-clock: The GTK clock widget
 *
 * Author: Szekeres István (szekeres@cyberspace.mht.bme.hu)
 */

#ifndef __GTK_CLOCK_H__
#define __GTK_CLOCK_H__

#include <time.h>
#include <gtk/gtk.h>


G_BEGIN_DECLS

#define GTK_TYPE_CLOCK            (gtk_clock_get_type ())
#define GTK_CLOCK(obj)            (GTK_CHECK_CAST((obj), GTK_TYPE_CLOCK, GtkClock))
#define GTK_CLOCK_CLASS(klass)    (GTK_CHECK_CAST_CLASS((klass), GTK_TYPE_CLOCK, GtkClockClass))
#define GTK_IS_CLOCK(obj)         (GTK_CHECK_TYPE((obj), GTK_TYPE_CLOCK))
#define GTK_IS_CLOCK_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_CLOCK))
#define GTK_CLOCK_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GTK_TYPE_CLOCK, GtkClockClass))
	
typedef struct _GtkClock GtkClock;
typedef struct _GtkClockClass GtkClockClass;

typedef enum {
  /* update struct when adding values to enum */
	GTK_CLOCK_INCREASING,
	GTK_CLOCK_DECREASING,
	GTK_CLOCK_REALTIME
} GtkClockType;

struct _GtkClock {
	GtkLabel widget;
	gchar *fmt;
	struct tm *tm;

	time_t seconds;
	time_t stopped;
	GtkClockType type : 3;
	gint timer_id;
	gint update_interval;
};

struct _GtkClockClass {
	GtkLabelClass parent_class;
};

guint gtk_clock_get_type(void) G_GNUC_CONST;
GtkWidget *gtk_clock_new(GtkClockType type);
void gtk_clock_construct(GtkClock *gclock, GtkClockType type);
void gtk_clock_set_format(GtkClock *gclock, const gchar *fmt);
void gtk_clock_set_seconds(GtkClock *gclock, time_t seconds);
void gtk_clock_set_update_interval(GtkClock *gclock, gint seconds);
void gtk_clock_start(GtkClock *gclock);
void gtk_clock_stop(GtkClock *gclock);

G_END_DECLS

#endif /* __GTK_CLOCK_H__ */
