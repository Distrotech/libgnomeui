/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-mdiP.h - functions from gnome-mdi.c needed by other mdi sources,
   but not a part of the public API amd the declaration of MDI's private
   parts

   Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
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

#ifndef __GNOME_MDIP_H__
#define __GNOME_MDIP_H__

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <libgnomeui/gnome-app.h>

#include "gnome-mdi.h"
#include "gnome-mdi-child.h"
#include "gnome-mdi-generic-child.h"

G_BEGIN_DECLS

/* keys for stuff that we'll assign to mdi data objects */
#define GNOME_MDI_TOOLBAR_INFO_KEY           "MDIToolbarUIInfo"
#define GNOME_MDI_MENUBAR_INFO_KEY           "MDIMenubarUIInfo"
#define GNOME_MDI_CHILD_MENU_INFO_KEY        "MDIChildMenuUIInfo"
#define GNOME_MDI_CHILD_TOOLBAR_INFO_KEY     "MDIChildToolbarUIInfo"
#define GNOME_MDI_CHILD_KEY                  "MDIChild"
#define GNOME_MDI_ITEM_COUNT_KEY             "MDIChildMenuItems"
#define GNOME_MDI_APP_KEY                    "MDIApp"
#define GNOME_MDI_PANED_KEY                  "MDIPaned"

/* name for the child's toolbar */
#define GNOME_MDI_CHILD_TOOLBAR_NAME         "MDIChildToolbar"

struct _GnomeMDIPrivate 
{
	gchar *appname, *title;

	gchar *menu_template;
	gchar *toolbar_template;

    /* probably only one of these would do, but... redundancy rules ;) */
	GnomeMDIChild *active_child;
	GtkWidget *active_view;  
	BonoboWindow *active_window;

	GList *windows;     /* toplevel windows - BonoboWindow widgets */
	GList *children;    /* children - GnomeMDIChild objects*/

	GSList *registered; /* see comment for gnome_mdi_(un)register() functions below for an explanation. */

    /* paths for insertion of mdi_child specific menus and mdi_child list
	   menu via gnome-app-helper routines */
	gchar *child_menu_path;
	gchar *child_list_path;

	GtkPositionType tab_pos;

	GnomeMDIMode mode : 3;

	guint signal_id;
	guint in_drag : 1;

	guint has_user_refcount : 1;
};

struct _GnomeMDIChildPrivate
{
	GtkObject *parent;               /* a pointer to the MDI */

	guint child_id;
	gchar *name;

	GList *views;

	GnomeUIInfo *menu_template;
	GnomeUIInfo *toolbar_template;

	/* default values for insertion of the child toolbar */
	GnomeDockItemBehavior behavior;
	GnomeDockPlacement placement;

	gint band_num, band_pos, offset;
};

struct _GnomeMDIGenericChildPrivate
{
	/* if any of these are set they override the virtual functions
	   in GnomeMDIChildClass. create_view is mandatory, as no default
	   handler is provided, others may be NULL */
	GnomeMDIChildViewCreator create_view;
	GnomeMDIChildMenuCreator create_menus;
	GnomeMDIChildConfigFunc  get_config_string;
	GnomeMDIChildLabelFunc   set_label;

	GtkCallbackMarshal create_view_cbm, create_menus_cbm,
		               get_config_string_cbm, set_label_cbm;
	GtkDestroyNotify   create_view_dn, create_menus_dn,
		               get_config_string_dn, set_label_dn;
	gpointer           create_view_data, create_menus_data,
		               get_config_string_data, set_label_data;
};

/* declare the functions from gnome-mdi.c that other MDI source files need,
   but are not part of the public API */
void       gnome_mdi_child_list_remove (GnomeMDI *mdi, GnomeMDIChild *child);
void       gnome_mdi_child_list_add    (GnomeMDI *mdi, GnomeMDIChild *child);
GtkWidget *gnome_mdi_new_toplevel      (GnomeMDI *mdi);
void       gnome_mdi_update_child      (GnomeMDI *mdi, GnomeMDIChild *child);
void       gnome_mdi_child_add_toolbar (GnomeMDIChild *mdi_child,
										GnomeApp *app,
									    GtkToolbar *toolbar);

G_END_DECLS

#endif /* __GNOME_MDIP_H__ */
