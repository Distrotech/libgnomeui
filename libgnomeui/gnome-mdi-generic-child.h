/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-mdi-generic-child.h - definition of a Gnome MDI generic child class

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

   Author: Jaka Mocnik <jaka.mocnik@kiss.uni-lj.si>
*/

#ifndef __GNOME_MDI_GENERIC_CHILD_H__
#define __GNOME_MDI_GENERIC_CHILD_H__

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gnome-mdi-child.h"

BEGIN_GNOME_DECLS

#define GNOME_MDI_GENERIC_CHILD(obj)          GTK_CHECK_CAST (obj, gnome_mdi_generic_child_get_type (), GnomeMDIGenericChild)
#define GNOME_MDI_GENERIC_CHILD_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gnome_mdi_generic_child_get_type (), GnomeMDIGenericChildClass)
#define GNOME_IS_GENERIC_MDI_CHILD(obj)       GTK_CHECK_TYPE (obj, gnome_mdi_generic_child_get_type ())

typedef struct _GnomeMDIGenericChild       GnomeMDIGenericChild;
typedef struct _GnomeMDIGenericChildClass  GnomeMDIGenericChildClass;

struct _GnomeMDIGenericChild {
	GnomeMDIChild mdi_child;

  gpointer user_data;

  /* if any of these are set they override the virtual functions
     in GnomeMDIChildClass. create_view is mandatory, as no default
     handler is provided, others may be NULL */
	GnomeMDIChildViewCreator create_view;
	GnomeMDIChildMenuCreator create_menus;
	GnomeMDIChildConfigFunc  get_config_string;
	GnomeMDIChildLabelFunc   set_label;
};

struct _GnomeMDIGenericChildClass {
	GnomeMDIChildClass parent_class;
};

guint                gnome_mdi_generic_child_get_type (void);

GnomeMDIGenericChild *gnome_mdi_generic_child_new(gchar *name,
                                                  GnomeMDIChildViewCreator create_view,
                                                  GnomeMDIChildMenuCreator create_menus,
                                                  GnomeMDIChildConfigFunc  get_config_string,
                                                  GnomeMDIChildLabelFunc   set_label,
                                                  gpointer                 user_data);

gpointer             gnome_mdi_generic_child_get_data(GnomeMDIGenericChild *child);
void                 gnome_mdi_generic_child_set_data(GnomeMDIGenericChild *child,
                                                      gpointer             data);

END_GNOME_DECLS

#endif
