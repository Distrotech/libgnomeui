/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-pouch.h - definition of a Gnome Pouch widget - a WiW MDI container

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

#ifndef __GNOME_POUCH_H__
#define __GNOME_POUCH_H__

#include <gtk/gtk.h>
#include <gdk/gdk.h>


#include "gnome-roo.h"

G_BEGIN_DECLS

#define GNOME_TYPE_POUCH            (gnome_pouch_get_type ())
#define GNOME_POUCH(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_POUCH, GnomePouch))
#define GNOME_POUCH_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_POUCH, GnomePouchClass))
#define GNOME_IS_POUCH(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_POUCH))
#define GNOME_IS_POUCH_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_POUCH))
#define GNOME_POUCH_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_POUCH, GnomePouchClass))

typedef struct _GnomePouch       GnomePouch;
typedef struct _GnomePouchClass  GnomePouchClass;

typedef struct _GnomePouchPrivate GnomePouchPrivate;

struct _GnomePouch {
	GtkFixed fixed;

	GnomePouchPrivate *priv;
};

struct _GnomePouchClass {
	GtkFixedClass parent_class;

	void (*close_child)(GnomePouch *pouch, GnomeRoo *roo);
	void (*iconify_child)(GnomePouch *pouch, GnomeRoo *roo);
	void (*uniconify_child)(GnomePouch *pouch, GnomeRoo *roo);
	void (*maximize_child)(GnomePouch *pouch, GnomeRoo *roo);
	void (*unmaximize_child)(GnomePouch *pouch, GnomeRoo *roo);
	void (*select_child)(GnomePouch *pouch, GnomeRoo *roo);
	void (*unselect_child)(GnomePouch *pouch, GnomeRoo *roo);
};

guint      gnome_pouch_get_type(void) G_GNUC_CONST;
GtkWidget *gnome_pouch_new(void);
void       gnome_pouch_select_roo(GnomePouch *pouch, GnomeRoo *roo);
GnomeRoo  *gnome_pouch_get_selected(GnomePouch *pouch);
void       gnome_pouch_move(GnomePouch *pouch, GnomeRoo *roo, gint x, gint y);
void       gnome_pouch_set_icon_arrangement(GnomePouch *pouch,
											GtkCornerType corner,
											GtkOrientation orientation);
void       gnome_pouch_arrange_icons(GnomePouch *pouch);
void       gnome_pouch_enable_auto_arrange(GnomePouch *pouch,
										   gboolean auto_arr);
void       gnome_pouch_enable_popup_menu(GnomePouch *pouch, gboolean enable);
void       gnome_pouch_tile_children(GnomePouch *pouch);
void       gnome_pouch_cascade_children(GnomePouch *pouch);

G_END_DECLS

#endif /* __GNOME_POUCH_H__ */
