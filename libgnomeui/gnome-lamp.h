/* Lamps and Beacons - Color-Reactiveness for Gnome
   Copyright (C) 1997, 1998 Free Software Foundation

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Eckehard Berns  */

#ifndef __GNOME_LAMP_H__
#define __GNOME_LAMP_H__

#include <gdk/gdktypes.h>
#include <libgnome/gnome-defs.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkpixmap.h>
#include <libgnomeui/gnome-pixmap.h>

BEGIN_GNOME_DECLS

#define GNOME_LAMP(obj)             GTK_CHECK_CAST(obj, gnome_lamp_get_type(), GnomeLamp)
#define GNOME_LAMP_CLASS(klass)     GTK_CHECK_CAST_CLASS(obj, gnome_lamp_get_type(), GnomeLamp)
#define GNOME_IS_LAMP(obj)          GTK_CHECK_TYPE(obj, gnome_lamp_get_type())

typedef struct _GnomeLamp           GnomeLamp;
typedef struct _GnomeLampClass      GnomeLampClass;

#define GNOME_LAMP_MAX_SEQUENCE 32

struct _GnomeLamp {
	GnomePixmap gpixmap;

	const char *mask;
	GdkColor color;
	char color_seq[GNOME_LAMP_MAX_SEQUENCE + 1];
	gint index;
};

struct _GnomeLampClass {
	GnomePixmapClass parent_class;
};

typedef struct _GnomeLampType GnomeLampType;
struct _GnomeLampType {
	char *name;
	GnomePixmap *pixmap;
	const char *mask;
	char color_seq[GNOME_LAMP_MAX_SEQUENCE + 1];
};

#define GNOME_LAMP_CLEAR    "C"
#define GNOME_LAMP_RED      "R"
#define GNOME_LAMP_GREEN    "G"
#define GNOME_LAMP_BLUE     "B"
#define GNOME_LAMP_YELLOW   "Y"
#define GNOME_LAMP_AQUA     "A"
#define GNOME_LAMP_PURPLE   "P"

#define GNOME_LAMP_IDLE      "idle"
#define GNOME_LAMP_BUSY      "busy"
#define GNOME_LAMP_INPUT     "input"
#define GNOME_LAMP_WARNING   "warning"
#define GNOME_LAMP_ERROR     "error"
#define GNOME_LAMP_PROC0     "proc00"
#define GNOME_LAMP_PROC10    "proc10"
#define GNOME_LAMP_PROC20    "proc20"
#define GNOME_LAMP_PROC30    "proc30"
#define GNOME_LAMP_PROC40    "proc40"
#define GNOME_LAMP_PROC50    "proc50"
#define GNOME_LAMP_PROC60    "proc60"
#define GNOME_LAMP_PROC70    "proc70"
#define GNOME_LAMP_PROC80    "proc80"
#define GNOME_LAMP_PROC90    "proc90"
#define GNOME_LAMP_FINISHED  GNOME_LAMP_IDLE
#define GNOME_LAMP_PROC100   GNOME_LAMP_FINISHED

guint         gnome_lamp_get_type       (void);
GtkWidget    *gnome_lamp_new            (void);
GtkWidget    *gnome_lamp_new_with_color (const GdkColor *color);
GtkWidget    *gnome_lamp_new_with_type  (const char *type);
void          gnome_lamp_set_color      (GnomeLamp *lamp,
					 const GdkColor *color);
void          gnome_lamp_set_sequence   (GnomeLamp *lamp, const char *seq);
void          gnome_lamp_set_type       (GnomeLamp *lamp, const char *type);


END_GNOME_DECLS

#endif /* GNOME_LAMP_H */

