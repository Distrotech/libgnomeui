
/*
 * gtk-clock: The GTK clock widget
 * (C)1998 The Free Software Foundation
 *
 * Author: Szekeres István (szekeres@cyberspace.mht.bme.hu)
 */

#ifndef __GTK_CLOCK_H__
#define __GTK_CLOCK_H__

#include <time.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#define GTK_CLOCK(obj) GTK_CHECK_CAST(obj, gtk_clock_get_type(), GtkClock)
#define GTK_CLOCK_CLASS(class) GTK_CHECK_CAST_CLASS(class, gtk_clock_get_type(), GtkClockClass)
#define GTK_IS_CLOCK(obj) GTK_CHECK_TYPE(obj, gtk_clock_get_type())
	
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

guint gtk_clock_get_type(void);
GtkWidget *gtk_clock_new(GtkClockType type);
void gtk_clock_set_format(GtkClock *gclock, const gchar *fmt);
void gtk_clock_set_seconds(GtkClock *gclock, time_t seconds);
void gtk_clock_set_update_interval(GtkClock *gclock, gint seconds);
void gtk_clock_start(GtkClock *gclock);
void gtk_clock_stop(GtkClock *gclock);

END_GNOME_DECLS

#endif /* __GTK_CLOCK_H__ */
