/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-mdiP.h - functions from gnome-mdi.c needed by other mdi sources,
   but not a part of the public API

   Copyright (C) 1997, 1998, 1999 Free Software Foundation

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

#ifndef __GNOME_MDIP_H__
#define __GNOME_MDIP_H__

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gnome-app.h"
#include "gnome-app-helper.h"
#include "gnome-mdi-child.h"

BEGIN_GNOME_DECLS

/* keys for stuff that we'll assign to mdi data objects */
#define GNOME_MDI_TOOLBAR_INFO_KEY           "MDIToolbarUIInfo"
#define GNOME_MDI_MENUBAR_INFO_KEY           "MDIMenubarUIInfo"
#define GNOME_MDI_CHILD_MENU_INFO_KEY        "MDIChildMenuUIInfo"
#define GNOME_MDI_CHILD_KEY                  "MDIChild"
#define GNOME_MDI_ITEM_COUNT_KEY             "MDIChildMenuItems"
#define GNOME_MDI_APP_KEY                    "MDIApp"

/* declare the functions from gnome-mdi.c that other source files need,
   but are a part of the public API */
void       gnome_mdi_child_list_remove (GnomeMDI *mdi, GnomeMDIChild *child);
void       gnome_mdi_child_list_add    (GnomeMDI *mdi, GnomeMDIChild *child);
GtkWidget *gnome_mdi_new_toplevel      (GnomeMDI *mdi);
void       gnome_mdi_update_child      (GnomeMDI *mdi, GnomeMDIChild *child);

END_GNOME_DECLS

#endif /* __GNOME_MDIP_H__ */
