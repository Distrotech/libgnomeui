/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-mdi-child.h - definition of an abstract MDI child class.

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

#include "libgnomeui/gnome-app.h"
#include "libgnomeui/gnome-app-helper.h"

BEGIN_GNOME_DECLS

#define GNOME_MDI_CHILD(obj)          GTK_CHECK_CAST (obj, gnome_mdi_child_get_type (), GnomeMDIChild)
#define GNOME_MDI_CHILD_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gnome_mdi_child_get_type (), GnomeMDIChildClass)
#define GNOME_IS_MDI_CHILD(obj)       GTK_CHECK_TYPE (obj, gnome_mdi_child_get_type ())

typedef struct _GnomeMDIChild       GnomeMDIChild;
typedef struct _GnomeMDIChildClass  GnomeMDIChildClass;

/* GnomeMDIChild
 * is an abstract class. In order to use it, you have to either derive a
 * new class from it and set the proper virtual functions in its parent
 * GnomeMDIChildClass structure or use the GnomeMDIGenericChild class
 * that allows to specify the relevant functions on a per-instance basis
 * and can directly be used with GnomeMDI.
 */
struct _GnomeMDIChild
{
	GtkObject object;

	GtkObject *parent;

	gchar *name;

	GList *views;

	GnomeUIInfo *menu_template;
};

typedef GtkWidget *(*GnomeMDIChildViewCreator) (GnomeMDIChild *, gpointer);
typedef GList     *(*GnomeMDIChildMenuCreator) (GnomeMDIChild *, GtkWidget *, gpointer);
typedef gchar     *(*GnomeMDIChildConfigFunc)  (GnomeMDIChild *, gpointer);
typedef GtkWidget *(*GnomeMDIChildLabelFunc)   (GnomeMDIChild *, GtkWidget *, gpointer);

/* note that if you override the set_label virtual function, it should return
 * a new widget if its GtkWidget* parameter is NULL and modify and return the
 * old widget otherwise.
 * (see gnome-mdi-child.c/gnome_mdi_child_set_book_label() for an example).
 */

struct _GnomeMDIChildClass
{
	GtkObjectClass parent_class;

	/* these make no sense as signals, so we'll make them "virtual" functions */
	GnomeMDIChildViewCreator create_view;
	GnomeMDIChildMenuCreator create_menus;
	GnomeMDIChildConfigFunc  get_config_string;
	GnomeMDIChildLabelFunc   set_label;
};

guint         gnome_mdi_child_get_type         (void);

GtkWidget     *gnome_mdi_child_add_view        (GnomeMDIChild *);

void          gnome_mdi_child_remove_view      (GnomeMDIChild *, GtkWidget *view);

void          gnome_mdi_child_set_name         (GnomeMDIChild *, gchar *);

void          gnome_mdi_child_set_menu_template(GnomeMDIChild *, GnomeUIInfo *);

END_GNOME_DECLS

#endif /* __GNOME_MDI_CHILD_H__ */
