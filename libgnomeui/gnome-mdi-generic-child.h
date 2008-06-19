/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-mdi-generic-child.h - definition of a generic MDI child class

   Copyright (C) 1997 - 2001 Free Software Foundation

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

   Author: Jaka Mocnik <jaka@gnu.org>
   Interp modifications: James Henstridge <james@daa.com.au>
*/

#ifndef __GNOME_MDI_GENERIC_CHILD_H__
#define __GNOME_MDI_GENERIC_CHILD_H__

#ifndef GNOME_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gnome-mdi-child.h"

G_BEGIN_DECLS

#define GNOME_TYPE_MDI_GENERIC_CHILD            (gnome_mdi_generic_child_get_type ())
#define GNOME_MDI_GENERIC_CHILD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_MDI_GENERIC_CHILD, GnomeMDIGenericChild))
#define GNOME_MDI_GENERIC_CHILD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_MDI_GENERIC_CHILD, GnomeMDIGenericChildClass))
#define GNOME_IS_MDI_GENERIC_CHILD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_MDI_GENERIC_CHILD))
#define GNOME_IS_MDI_GENERIC_CHILD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_MDI_GENERIC_CHILD))
#define GNOME_MDI_GENERIC_CHILD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_MDI_GENERIC_CHILD, GnomeMDIGenericChildClass))
/* The source backward-compatibility macro GNOME_IS_MDI_MDI_CHILD(obj)
   is in gnome-compat.h.  */

typedef struct _GnomeMDIGenericChild        GnomeMDIGenericChild;
typedef struct _GnomeMDIGenericChildClass   GnomeMDIGenericChildClass;

struct _GnomeMDIGenericChild {
	GnomeMDIChild mdi_child;

	/* if any of these are set they override the virtual functions
	   in GnomeMDIChildClass. create_view is mandatory, as no default
	   handler is provided, others may be NULL */
	GnomeMDIChildViewCreator create_view;
	GnomeMDIChildMenuCreator create_menus;
	GnomeMDIChildConfigFunc  get_config_string;
	GnomeMDIChildLabelFunc   set_label;

	GtkCallbackMarshal create_view_cbm, create_menus_cbm,
		               get_config_string_cbm, set_label_cbm;
	GDestroyNotify   create_view_dn, create_menus_dn,
		               get_config_string_dn, set_label_dn;
	gpointer           create_view_data, create_menus_data,
		               get_config_string_data, set_label_data;

	gpointer reserved;
};

struct _GnomeMDIGenericChildClass {
	GnomeMDIChildClass parent_class;
};

GType                 gnome_mdi_generic_child_get_type (void);

GnomeMDIGenericChild *gnome_mdi_generic_child_new     (const gchar *name);

void gnome_mdi_generic_child_set_view_creator     (GnomeMDIGenericChild *child,
												   GnomeMDIChildViewCreator func,
                                                   gpointer data);
void gnome_mdi_generic_child_set_view_creator_full(GnomeMDIGenericChild *child,
												   GnomeMDIChildViewCreator func,
												   GtkCallbackMarshal marshal,
												   gpointer data,
												   GDestroyNotify notify);
void gnome_mdi_generic_child_set_menu_creator     (GnomeMDIGenericChild *child,
												   GnomeMDIChildMenuCreator func,
                                                   gpointer data);
void gnome_mdi_generic_child_set_menu_creator_full(GnomeMDIGenericChild *child,
												   GnomeMDIChildMenuCreator func,
												   GtkCallbackMarshal marshal,
												   gpointer data,
												   GDestroyNotify notify);
void gnome_mdi_generic_child_set_config_func      (GnomeMDIGenericChild *child,
												   GnomeMDIChildConfigFunc func,
                                                   gpointer data);
void gnome_mdi_generic_child_set_config_func_full (GnomeMDIGenericChild *child,
												   GnomeMDIChildConfigFunc func,
												   GtkCallbackMarshal marshal,
												   gpointer data,
												   GDestroyNotify notify);
void gnome_mdi_generic_child_set_label_func       (GnomeMDIGenericChild *child,
												   GnomeMDIChildLabelFunc func,
                                                   gpointer data);
void gnome_mdi_generic_child_set_label_func_full  (GnomeMDIGenericChild *child,
												   GnomeMDIChildLabelFunc func,
												   GtkCallbackMarshal marshal,
												   gpointer data,
												   GDestroyNotify notify);


G_END_DECLS

#endif /* GNOME_DISABLE_DEPRECATED */

#endif
