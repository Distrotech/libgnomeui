/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-pouchP.h - private parts ;) of GnomePouch and GnomeRoo widgets

   Copyright (C) 2000 Free Software Foundation
   All rights reserved.

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
/*
  @NOTATION@
*/

#ifndef __GNOME_POUCHP_H__
#define __GNOME_POUCHP_H__

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gnome-pouch.h"
#include "gnome-roo.h"

G_BEGIN_DECLS

struct _GnomePouchPrivate
{
	GnomeRoo *selected;
	GList *arranged;

	gboolean auto_arrange : 1;
	GtkCornerType icon_corner;
	GtkOrientation icon_orientation;

	GtkWidget *popup_menu;

	GtkCheckMenuItem *oitem[2], *citem[4], *aitem;
};

struct _GnomeRooPrivate
{
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

G_END_DECLS

#endif /* __GNOME_POUCHP_H__ */
