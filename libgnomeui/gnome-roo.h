/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-roo.h - a Gnome Roo widget - WiW MDI child

   Copyright (C) 2000 Jaka Mocnik

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

   Author: Jaka Mocnik <jaka.mocnik@kiss.uni-lj.si>
*/

#ifndef __GNOME_ROO_H__
#define __GNOME_ROO_H__

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "libgnome/gnome-defs.h"

BEGIN_GNOME_DECLS

#define GNOME_ROO(obj)          GTK_CHECK_CAST (obj, gnome_roo_get_type (), GnomeRoo)
#define GNOME_ROO_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gnome_roo_get_type (), GnomeRooClass)
#define GNOME_IS_ROO(obj)       GTK_CHECK_TYPE (obj, gnome_roo_get_type ())

typedef struct _GnomeRoo       GnomeRoo;
typedef struct _GnomeRooClass  GnomeRooClass;

struct _GnomeRoo {
	GtkBin bin;

	gchar *title;

	/* the following are private */
	guint16 min_width, min_height, title_bar_height;
	guint resize_type;
	guint16 grab_x, grab_y;
	GtkAllocation user_allocation;
	GtkAllocation icon_allocation;
	guint16 flags;
	gchar *vis_title;
	GdkWindow *cover;
};

struct _GnomeRooClass {
	GtkBinClass parent_class;

	void (*close)(GnomeRoo *roo);
	void (*iconify)(GnomeRoo *roo);
	void (*uniconify)(GnomeRoo *roo);
	void (*maximize)(GnomeRoo *roo);
	void (*unmaximize)(GnomeRoo *roo);
	void (*select)(GnomeRoo *roo);
	void (*deselect)(GnomeRoo *roo);
};

guint     gnome_roo_get_type(void);
GtkWidget *gnome_roo_new(void);
void      gnome_roo_set_title(GnomeRoo *roo, const gchar *name);
void      gnome_roo_set_iconified(GnomeRoo *roo, gboolean iconified);
gboolean  gnome_roo_is_iconified(GnomeRoo *roo);
void      gnome_roo_set_maximized(GnomeRoo *roo, gboolean maximized);
gboolean  gnome_roo_is_maximized(GnomeRoo *roo);
gboolean  gnome_roo_is_selected(GnomeRoo *roo);
gboolean  gnome_roo_is_parked(GnomeRoo *roo);
void      gnome_roo_park(GnomeRoo *roo, gint x, gint y);
void      gnome_roo_unpark(GnomeRoo *roo);

END_GNOME_DECLS

#endif /* __GNOME_ROO_H__ */
