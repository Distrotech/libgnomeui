/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-mdi-child.h - definition of a Gnome MDI Child object

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

#ifndef __GNOME_MDI_CHILD_H__
#define __GNOME_MDI_CHILD_H__

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "libgnomeui/gnome-app-helper.h"

BEGIN_GNOME_DECLS

#define GNOME_MDI_CHILD(obj)          GTK_CHECK_CAST (obj, gnome_mdi_child_get_type (), GnomeMDIChild)
#define GNOME_MDI_CHILD_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gnome_mdi_child_get_type (), GnomeMDIChildClass)
#define GNOME_IS_MDI_CHILD(obj)       GTK_CHECK_TYPE (obj, gnome_mdi_child_get_type ())

typedef struct _GnomeMDIChild       GnomeMDIChild;
typedef struct _GnomeMDIChildClass  GnomeMDIChildClass;

struct _GnomeMDIChild
{
	GtkObject object;

	GtkObject *parent;

	gchar *name;

	GList *views;

	GnomeUIInfo *menu_template;
};

struct _GnomeMDIChildClass
{
	GtkObjectClass parent_class;

	GtkWidget * (*create_view)(GnomeMDIChild *); 
	GList     * (*create_menus)(GnomeMDIChild *, GtkWidget *); 
	gchar     * (*get_config_string)(GnomeMDIChild *);
};

guint         gnome_mdi_child_get_type         (void);

GnomeMDIChild *gnome_mdi_child_new             (void);

GtkWidget     *gnome_mdi_child_add_view        (GnomeMDIChild *);
void          gnome_mdi_child_remove_view      (GnomeMDIChild *, GtkWidget *view);

void          gnome_mdi_child_set_name         (GnomeMDIChild *, gchar *);

void          gnome_mdi_child_set_menu_template(GnomeMDIChild *, GnomeUIInfo *);

END_GNOME_DECLS

#endif /* __GNOME_MDI_CHILD_H__ */
