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



G_BEGIN_DECLS

#define GNOME_TYPE_ROO            (gnome_roo_get_type ())
#define GNOME_ROO(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_ROO, GnomeRoo))
#define GNOME_ROO_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ROO, GnomeRooClass))
#define GNOME_IS_ROO(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_ROO))
#define GNOME_IS_ROO_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ROO))
#define GNOME_ROO_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_ROO, GnomeRooClass))

typedef struct _GnomeRoo        GnomeRoo;
typedef struct _GnomeRooClass   GnomeRooClass;

typedef struct _GnomeRooPrivate GnomeRooPrivate;

struct _GnomeRoo {
	GtkBin bin;

	GnomeRooPrivate *priv;
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

guint        gnome_roo_get_type(void) G_GNUC_CONST;
GtkWidget   *gnome_roo_new(void);
const gchar *gnome_roo_get_title(GnomeRoo *roo);
void         gnome_roo_set_title(GnomeRoo *roo, const gchar *name);
void         gnome_roo_set_iconified(GnomeRoo *roo, gboolean iconified);
gboolean     gnome_roo_is_iconified(GnomeRoo *roo);
void         gnome_roo_set_maximized(GnomeRoo *roo, gboolean maximized);
gboolean     gnome_roo_is_maximized(GnomeRoo *roo);
gboolean     gnome_roo_is_selected(GnomeRoo *roo);
gboolean     gnome_roo_is_parked(GnomeRoo *roo);
void         gnome_roo_park(GnomeRoo *roo, gint x, gint y);
void         gnome_roo_unpark(GnomeRoo *roo);

G_END_DECLS

#endif /* __GNOME_ROO_H__ */
