/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-mdi.h - definition of a Gnome MDI object

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

#ifndef __GNOME_MDI_H__
#define __GNOME_MDI_H__

#ifndef GNOME_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gnome-app.h"
#include "gnome-app-helper.h"
#include "gnome-mdi-child.h"

G_BEGIN_DECLS

#define GNOME_TYPE_MDI            (gnome_mdi_get_type ())
#define GNOME_MDI(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_MDI, GnomeMDI))
#define GNOME_MDI_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_MDI, GnomeMDIClass))
#define GNOME_IS_MDI(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_MDI))
#define GNOME_IS_MDI_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_MDI))
#define GNOME_MDI_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_MDI, GnomeMDIClass))

typedef struct _GnomeMDI        GnomeMDI;
typedef struct _GnomeMDIClass   GnomeMDIClass;

typedef enum {
	GNOME_MDI_NOTEBOOK,
	GNOME_MDI_TOPLEVEL,
	GNOME_MDI_MODAL,
	GNOME_MDI_DEFAULT_MODE = 42
} GnomeMDIMode;

struct _GnomeMDI {
	GtkObject object;

	GnomeMDIMode mode;

	GtkPositionType tab_pos;

	guint signal_id;
	guint in_drag : 1;

	gchar *appname, *title;

	GnomeUIInfo *menu_template;
	GnomeUIInfo *toolbar_template;

    /* probably only one of these would do, but... redundancy rules ;) */
	GnomeMDIChild *active_child;
	GtkWidget *active_view;  
	GnomeApp *active_window;

	GList *windows;     /* toplevel windows - GnomeApp widgets */
	GList *children;    /* children - GnomeMDIChild objects*/

	GSList *registered; /* see comment for gnome_mdi_(un)register() functions below for an explanation. */
	
    /* paths for insertion of mdi_child specific menus and mdi_child list menu via
       gnome-app-helper routines */
	gchar *child_menu_path; 
	gchar *child_list_path;

	gpointer reserved;
};

struct _GnomeMDIClass {
	GtkObjectClass parent_class;

	gint        (*add_child)(GnomeMDI *, GnomeMDIChild *); 
	gint        (*remove_child)(GnomeMDI *, GnomeMDIChild *); 
	gint        (*add_view)(GnomeMDI *, GtkWidget *); 
	gint        (*remove_view)(GnomeMDI *, GtkWidget *); 
	void        (*child_changed)(GnomeMDI *, GnomeMDIChild *);
	void        (*view_changed)(GnomeMDI *, GtkWidget *);
	void        (*app_created)(GnomeMDI *, GnomeApp *);
};

/*
 * description of GnomeMDI signals:
 *
 * gint add_child(GnomeMDI *, GnomeMDIChild *)
 * gint add_view(GnomeMDI *, GtkWidget *)
 *   are called before actually adding a mdi_child or a view to the MDI. if the handler returns
 *   TRUE, the action proceeds otherwise the mdi_child or view are not added.
 *
 * gint remove_child(GnomeMDI *, GnomeMDIChild *)
 * gint remove_view(GnomeMDI *, GtkWidget *)
 *   are called before removing mdi_child or view. the handler should return true if the object
 *   is to be removed from MDI
 *
 * void child_changed(GnomeMDI *, GnomeMDIChild *)
 *   gets called each time when active child is changed with the second argument
 *   pointing to the old child. mdi->active_view and mdi->active_child still already
 *   hold the new values
 *
 * void view_changed(GnomeMDI *, GtkWidget *)
 *   is emitted whenever a view is changed, regardless of it being the view of the same child as
 *   the old view or not. the second argument points to the old view, mdi->active_view and
 *   mdi->active_child hold the new values. if the child has also been changed, this signal is
 *   emitted after the child_changed signal.
 * 
 * void app_created(GnomeMDI *, GnomeApp *)
 *   is called with each newly created GnomeApp to allow the MDI user to customize it (add a
 *   statusbar, toolbars or menubar if the method with GnomeUIInfo templates is not sufficient,
 *   etc.).
 *   no contents may be set since GnomeMDI uses them for storing either a view of a child
 *   or a notebook
 */

GType          gnome_mdi_get_type            (void);

GtkObject     *gnome_mdi_new                (const gchar *appname, const gchar *title);

void          gnome_mdi_set_mode            (GnomeMDI *mdi, GnomeMDIMode mode);

/* setting the menu and toolbar stuff */
void          gnome_mdi_set_menubar_template(GnomeMDI *mdi, GnomeUIInfo *menu_tmpl);
void          gnome_mdi_set_toolbar_template(GnomeMDI *mdi, GnomeUIInfo *tbar_tmpl);
void          gnome_mdi_set_child_menu_path (GnomeMDI *mdi, const gchar *path);
void          gnome_mdi_set_child_list_path (GnomeMDI *mdi, const gchar *path);

/* manipulating views */
gint          gnome_mdi_add_view            (GnomeMDI *mdi, GnomeMDIChild *child);
gint          gnome_mdi_add_toplevel_view   (GnomeMDI *mdi, GnomeMDIChild *child);
gint          gnome_mdi_remove_view         (GnomeMDI *mdi, GtkWidget *view, gint force);

GtkWidget     *gnome_mdi_get_active_view    (GnomeMDI *mdi);
void          gnome_mdi_set_active_view     (GnomeMDI *mdi, GtkWidget *view);

/* manipulating children */
gint          gnome_mdi_add_child           (GnomeMDI *mdi, GnomeMDIChild *child);
gint          gnome_mdi_remove_child        (GnomeMDI *mdi, GnomeMDIChild *child, gint force);
gint          gnome_mdi_remove_all          (GnomeMDI *mdi, gint force);

void          gnome_mdi_open_toplevel       (GnomeMDI *mdi);

void          gnome_mdi_update_child        (GnomeMDI *mdi, GnomeMDIChild *child);

GnomeMDIChild *gnome_mdi_get_active_child   (GnomeMDI *mdi);
GnomeMDIChild *gnome_mdi_find_child         (GnomeMDI *mdi, const gchar *name);

GnomeApp      *gnome_mdi_get_active_window  (GnomeMDI *mdi);

/*
 * the following two functions are here to make life easier if an application
 * creates objects (like opening a window) that should "keep the application
 * alive" even if there are no MDI windows open. any such windows should be
 * registered with the MDI: as long as there is a window registered, the MDI
 * will not destroy itself (even if the last of its windows is closed). on the
 * other hand, closing the last MDI window when no objects are registered
 * with the MDI will result in MDI being gtk_object_destroy()ed.
 */
void          gnome_mdi_register            (GnomeMDI *mdi, GtkObject *object);
void          gnome_mdi_unregister          (GnomeMDI *mdi, GtkObject *object);

/*
 * convenience functions for retrieveing GnomeMDIChild and GnomeApp
 * objects associated with a particular view and for retrieveing the
 * visible view of a certain GnomeApp.
 */
GnomeApp      *gnome_mdi_get_app_from_view    (GtkWidget *view);
GnomeMDIChild *gnome_mdi_get_child_from_view  (GtkWidget *view);
GtkWidget     *gnome_mdi_get_view_from_window (GnomeMDI *mdi, GnomeApp *app);

/* the following functions are used to obtain pointers to the GnomeUIInfo
 * structures for a specified MDI GnomeApp widget. this might be useful for
 * enabling/disabling menu items (via ui_info[i]->widget) when certain events
 * happen or selecting the default menuitem in a radio item group. these
 * GnomeUIInfo structures are exact copies of the template GnomeUIInfo trees
 * and are non-NULL only if templates are used for menu/toolbar creation.
 */
GnomeUIInfo   *gnome_mdi_get_menubar_info     (GnomeApp *app);
GnomeUIInfo   *gnome_mdi_get_toolbar_info     (GnomeApp *app);
GnomeUIInfo   *gnome_mdi_get_child_menu_info  (GnomeApp *app);

G_END_DECLS

#endif /* GNOME_DISABLE_DEPRECATED */

#endif /* __GNOME_MDI_H__ */
