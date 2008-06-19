/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-mdi-child.h - definition of an abstract MDI child class.

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
*/

#ifndef __GNOME_MDI_CHILD_H__
#define __GNOME_MDI_CHILD_H__

#ifndef GNOME_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "libgnomeui/gnome-app.h"
#include "libgnomeui/gnome-app-helper.h"

G_BEGIN_DECLS

#define GNOME_TYPE_MDI_CHILD            (gnome_mdi_child_get_type ())
#define GNOME_MDI_CHILD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_MDI_CHILD, GnomeMDIChild))
#define GNOME_MDI_CHILD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_MDI_CHILD, GnomeMDIChildClass))
#define GNOME_IS_MDI_CHILD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_MDI_CHILD))
#define GNOME_IS_MDI_CHILD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_MDI_CHILD))
#define GNOME_MDI_CHILD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_MDI_CHILD, GnomeMDIChildClass))

typedef struct _GnomeMDIChild        GnomeMDIChild;
typedef struct _GnomeMDIChildClass   GnomeMDIChildClass;

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

	gpointer reserved;
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

GType         gnome_mdi_child_get_type         (void);

GtkWidget     *gnome_mdi_child_add_view        (GnomeMDIChild *mdi_child);

void          gnome_mdi_child_remove_view      (GnomeMDIChild *mdi_child, GtkWidget *view);

void          gnome_mdi_child_set_name         (GnomeMDIChild *mdi_child, const gchar *name);

void          gnome_mdi_child_set_menu_template(GnomeMDIChild *mdi_child, GnomeUIInfo *menu_tmpl);

G_END_DECLS

#endif /* GNOME_DISABLE_DEPRECATED */

#endif /* __GNOME_MDI_CHILD_H__ */
