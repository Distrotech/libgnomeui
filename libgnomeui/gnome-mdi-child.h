/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-mdi-child.h - definition of an abstract MDI child class.

   Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
   All rights reserved

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

#ifndef __GNOME_MDI_CHILD_H__
#define __GNOME_MDI_CHILD_H__

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <libgnomeui/gnome-app.h>
#include <libgnomeui/gnome-app-helper.h>

G_BEGIN_DECLS

#define GNOME_TYPE_MDI_CHILD            (gnome_mdi_child_get_type ())
#define GNOME_MDI_CHILD(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_MDI_CHILD, GnomeMDIChild))
#define GNOME_MDI_CHILD_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_MDI_CHILD, GnomeMDIChildClass))
#define GNOME_IS_MDI_CHILD(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_MDI_CHILD))
#define GNOME_IS_MDI_CHILD_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_MDI_CHILD))
#define GNOME_MDI_CHILD_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_MDI_CHILD, GnomeMDIChildClass))

typedef struct _GnomeMDIChild        GnomeMDIChild;
typedef struct _GnomeMDIChildClass   GnomeMDIChildClass;

typedef struct _GnomeMDIChildPrivate GnomeMDIChildPrivate;

/* GnomeMDIChild
 * is an abstract class. In order to use it, you have to either derive a
 * new class from it and set the proper virtual functions in its parent
 * GnomeMDIChildClass structure or use the GnomeMDIGenericChild class
 * that allows to specify the relevant functions on a per-instance instead
 * on a per-class basis and can directly be used with GnomeMDI.
 */
struct _GnomeMDIChild
{
	GtkObject object;

	GnomeMDIChildPrivate *priv;
};

/* note that if you override the set_label virtual function, it should return
 * a new widget if its GtkWidget* parameter is NULL and modify and return the
 * old widget otherwise.
 */
struct _GnomeMDIChildClass
{
	GtkObjectClass parent_class;

	/* these make no sense as signals, so we'll make them "virtual" functions */
	/* these should correspond to the typedefs in gnome-mdi-generic-child,
	 * except that they should lack the data argument */
	GtkWidget * (* create_view)       (GnomeMDIChild *);
	GList *     (* create_menus)      (GnomeMDIChild *, GtkWidget *);
	char *      (* get_config_string) (GnomeMDIChild *);
	GtkWidget * (* set_label)         (GnomeMDIChild *, GtkWidget *);
};

GtkType      gnome_mdi_child_get_type   (void) G_GNUC_CONST;
GtkWidget   *gnome_mdi_child_add_view   (GnomeMDIChild *mdi_child);
void         gnome_mdi_child_remove_view(GnomeMDIChild *mdi_child, GtkWidget *view);

const gchar *gnome_mdi_child_get_name   (GnomeMDIChild *mdi_child);
void         gnome_mdi_child_set_name   (GnomeMDIChild *mdi_child, const gchar *name);
const GList *gnome_mdi_child_get_views  (GnomeMDIChild *mdi_child);
 
void         gnome_mdi_child_set_menu_template(GnomeMDIChild *mdi_child, GnomeUIInfo *menu_tmpl);
void         gnome_mdi_child_set_toolbar_template(GnomeMDIChild *mdi_child, GnomeUIInfo *toolbar_tmpl);
void         gnome_mdi_child_set_toolbar_position(GnomeMDIChild *mdi_child,
												  GnomeDockItemBehavior behavior,
												  GnomeDockPlacement placement,
												  gint band_num, gint band_pos,
												  gint offset);


G_END_DECLS

#endif /* __GNOME_MDI_CHILD_H__ */
