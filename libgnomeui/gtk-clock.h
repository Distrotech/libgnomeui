
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

#ifdef __cplusplus
extern "C" {
#endif

#define GTK_CLOCK(obj) GTK_CHECK_CAST(obj, gtk_clock_get_type(), GtkClock)
#define GTK_CLOCK_CLASS(class) GTK_CHECK_CAST_CLASS(class, gtk_clock_get_type(), GtkClockClass)
#define GTK_IS_CLOCK(obj) GTK_CHECK_TYPE(obj, gtk_clock_get_type())
	
typedef struct _GtkClock GtkClock;
typedef struct _GtkClockClass GtkClockClass;

typedef enum _GtkClockType GtkClockType;

enum _GtkClockType {
	GTK_CLOCK_INCREASING,
	GTK_CLOCK_DECREASING,
	GTK_CLOCK_REALTIME
};

struct _GtkClock {
	GtkLabel widget;
	GtkClockType type;
	gint timer_id;
	gint update_interval;
	time_t seconds;
	time_t stopped;
	gchar *fmt;
	struct tm *tm;
};

struct _GtkClockClass {
	GtkLabelClass parent_class;
};

guint gtk_clock_get_type(void);
GtkWidget *gtk_clock_new(GtkClockType type);
void gtk_clock_set_format(GtkClock *clock, gchar *fmt);
void gtk_clock_set_seconds(GtkClock *clock, time_t seconds);
void gtk_clock_set_update_interval(GtkClock *clock, gint seconds);
void gtk_clock_start(GtkClock *clock);
void gtk_clock_stop(GtkClock *clock);

#ifdef __cplusplus
}
#endif

#endif /* __GTK_CLOCK_H__ */
