/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-mdi-child.c - implementation of an abstract class for MDI children

   Copyright (C) 1997, 1998 Free Software Foundation
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

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gnome-app.h"
#include "gnome-app-helper.h"
#include "gnome-mdi-child.h"
#include "gnome-mdi.h"
#include "gnome-mdiP.h"

#define GNOME_ENABLE_DEBUG

static void       gnome_mdi_child_class_init       (GnomeMDIChildClass *klass);
static void       gnome_mdi_child_init             (GnomeMDIChild *);
static void       gnome_mdi_child_destroy          (GtkObject *);
static void       gnome_mdi_child_finalize         (GtkObject *);

static GtkWidget *gnome_mdi_child_set_label        (GnomeMDIChild *, GtkWidget *, gpointer);
static GtkWidget *gnome_mdi_child_create_view      (GnomeMDIChild *);

static GtkObjectClass *parent_class = NULL;

guint gnome_mdi_child_get_type ()
{
	static guint mdi_child_type = 0;
  
	if (!mdi_child_type) {
		GtkTypeInfo mdi_child_info = {
			"GnomeMDIChild",
			sizeof (GnomeMDIChild),
			sizeof (GnomeMDIChildClass),
			(GtkClassInitFunc) gnome_mdi_child_class_init,
			(GtkObjectInitFunc) gnome_mdi_child_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL,
		};
    
		mdi_child_type = gtk_type_unique (gtk_object_get_type (), &mdi_child_info);
	}
  
	return mdi_child_type;
}

static void gnome_mdi_child_class_init (GnomeMDIChildClass *klass)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass*)klass;
  
	object_class->destroy = gnome_mdi_child_destroy;
	object_class->finalize = gnome_mdi_child_finalize;
  
	klass->create_view = NULL;
	klass->create_menus = NULL;
	klass->get_config_string = NULL;
	klass->set_label = gnome_mdi_child_set_label;

	parent_class = gtk_type_class (gtk_object_get_type ());
}

static void gnome_mdi_child_init (GnomeMDIChild *mdi_child)
{
	mdi_child->name = NULL;
	mdi_child->parent = NULL;
	mdi_child->views = NULL;

	mdi_child->behavior = GNOME_DOCK_ITEM_BEH_NORMAL;
	mdi_child->placement = GNOME_DOCK_TOP;
	mdi_child->band_num = 101;
	mdi_child->band_pos = 0;
	mdi_child->offset = 0;
}

static GtkWidget *gnome_mdi_child_create_view (GnomeMDIChild *child)
{
	if(GNOME_MDI_CHILD_CLASS(GTK_OBJECT(child)->klass)->create_view)
		return GNOME_MDI_CHILD_CLASS(GTK_OBJECT(child)->klass)->create_view(child, NULL);

	return NULL;
}

/* the default set_label function: returns a GtkLabel with child->name
 * if you provide your own, it should return a new widget if its old_label
 * parameter is NULL and modify and return the old widget otherwise. it
 * should (obviously) NOT call the parent class handler!
 */
static GtkWidget *gnome_mdi_child_set_label (GnomeMDIChild *child, GtkWidget *old_label, gpointer data)
{
#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeMDIChild: default set_label handler called!\n");
#endif

	if(old_label) {
		gtk_label_set_text(GTK_LABEL(old_label), child->name);
		return old_label;
	}
	else {
		GtkWidget *label;

		label = gtk_label_new(child->name);
		gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

		return label;
	}
}

static void gnome_mdi_child_finalize (GtkObject *obj)
{
	GnomeMDIChild *mdi_child;

#ifdef GNOME_ENABLE_DEBUG
	g_message("child finalization\n");
#endif

	mdi_child = GNOME_MDI_CHILD(obj);

	if(mdi_child->name)
		g_free(mdi_child->name);

	if(GTK_OBJECT_CLASS(parent_class)->finalize)
		(* GTK_OBJECT_CLASS(parent_class)->finalize)(obj);
}

static void gnome_mdi_child_destroy (GtkObject *obj)
{
	GnomeMDIChild *mdi_child;

#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeMDIChild: destroying!\n");
#endif

	mdi_child = GNOME_MDI_CHILD(obj);

	while(mdi_child->views)
		gnome_mdi_child_remove_view(mdi_child, GTK_WIDGET(mdi_child->views->data));

	if(GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(obj);
}

/**
 * gnome_mdi_child_add_view:
 * @mdi_child: A pointer to a GnomeMDIChild object.
 * 
 * Description:
 * Creates a new view of a child (a GtkWidget) adds it to the list
 * of the views and returns a pointer to it. Virtual function
 * that has to be specified for classes derived from GnomeMDIChild
 * is used to create the new view.
 * 
 * Return value:
 * A pointer to the new view.
 **/
GtkWidget *gnome_mdi_child_add_view (GnomeMDIChild *mdi_child)
{
	GtkWidget *view = NULL;

	view = gnome_mdi_child_create_view(mdi_child);

	if(view) {
		mdi_child->views = g_list_append(mdi_child->views, view);

		gtk_object_set_data(GTK_OBJECT(view), GNOME_MDI_CHILD_KEY, mdi_child);
	}

	return view;
}

/**
 * gnome_mdi_child_remove_view:
 * @mdi_child: A pointer to a GnomeMDIChild object.
 * @view: View to be removed.
 * 
 * Description:
 * Removes view @view from the list of @mdi_child's views and
 * unrefs it.
 **/
void gnome_mdi_child_remove_view(GnomeMDIChild *mdi_child, GtkWidget *view)
{
	mdi_child->views = g_list_remove(mdi_child->views, view);

	gtk_widget_unref(view);
}

/**
 * gnome_mdi_child_set_name:
 * @mdi_child: A pointer to a GnomeMDIChild object.
 * @name: String containing the new name for the child.
 * 
 * Description:
 * Changes name of @mdi_child to @name. @name is duplicated and stored
 * in @mdi_child. If @mdi_child has already been added to GnomeMDI,
 * it also takes care of updating it.
 **/
void gnome_mdi_child_set_name(GnomeMDIChild *mdi_child, const gchar *name)
{
	gchar *old_name = mdi_child->name;

	if(mdi_child->parent)
		gnome_mdi_child_list_remove(GNOME_MDI(mdi_child->parent), mdi_child);

	mdi_child->name = (gchar *)g_strdup(name);

	if(old_name)
		g_free(old_name);

	if(mdi_child->parent) {
		gnome_mdi_child_list_add(GNOME_MDI(mdi_child->parent), mdi_child);
		gnome_mdi_update_child(GNOME_MDI(mdi_child->parent), mdi_child);
	}
}

/**
 * gnome_mdi_child_set_menu_template:
 * @mdi_child: A pointer to a GnomeMDIChild object.
 * @menu_tmpl: A GnomeUIInfo array describing the child specific menus.
 * 
 * Description:
 * Sets the template for menus that are added and removed when differrent
 * children get activated. This way, each child can modify the MDI menubar
 * to suit its needs. If no template is set, the create_menus virtual
 * function will be used for creating these menus (it has to return a
 * GList of menu items). If no such function is specified, the menubar will
 * be unchanged by MDI children, but one can still modify the menubar
 * using handlers for GnomeMDI view_changed or child_changed signal.
 **/
void gnome_mdi_child_set_menu_template (GnomeMDIChild *mdi_child, GnomeUIInfo *menu_tmpl)
{
	mdi_child->menu_template = menu_tmpl;
}

/**
 * gnome_mdi_child_set_toolbar_template:
 * @mdi_child: A pointer to a GnomeMDIChild object.
 * @toolbar_tmpl: A GnomeUIInfo array describing the child specific toolbar.
 * 
 * Description:
 * Sets the template for a toolbar that is added and removed when different
 * children get activated. If no template is specified, one can still add
 * toolbars utilizing the GnomeMDI view_changed or child_changed signal.
 **/
void gnome_mdi_child_set_toolbar_template (GnomeMDIChild *mdi_child, GnomeUIInfo *toolbar_tmpl)
{
	mdi_child->toolbar_template = toolbar_tmpl;
}

/**
 * gnome_mdi_child_set_toolbar_position:
 * @mdi_child: A pointer to a GnomeMDIChild object.
 * @behavior: GnomeDockItem's behavior for the toolbar
 * @placement: what area of the dock the toolbar should be placed in.
 * @band_num: what band to place toolbar in.
 * @band_pos: the toolbar's position in its band.
 * @offset: offset from the previous GnomeDockItem in the band.
 * 
 * Description:
 * Sets the default values for adding the child's toolbar to a GnomeApp.
 * These values are used by MDI's default view_changed handler and
 * gnome_mdi_child_add_toolbar() function.
 **/
void gnome_mdi_child_set_toolbar_position(GnomeMDIChild *mdi_child,
										  GnomeDockItemBehavior behavior,
										  GnomeDockPlacement placement,
										  gint band_num, gint band_pos,
										  gint offset)
{
	mdi_child->behavior = behavior;
	mdi_child->placement = placement;
	mdi_child->band_num = band_num;
	mdi_child->band_pos = band_pos;
	mdi_child->offset = offset;
}

/**
 * gnome_mdi_child_add_toolbar:
 * @mdi_child: A pointer to a GnomeMDIChild object.
 * @app: A pointer to a GnomeApp that contains the view of the @mdi_child
 * @toolbar: The toolbar to be added.
 *
 * Description:
 * Adds toolbar @toolbar to the GnomeApp @app using the default placement
 * values stored in @mdi_child.
 **/
void gnome_mdi_child_add_toolbar(GnomeMDIChild *mdi_child, GnomeApp *app,
								 GtkToolbar *toolbar)
{
	gnome_app_add_toolbar(app, toolbar, GNOME_MDI_CHILD_TOOLBAR_NAME,
						  mdi_child->behavior, mdi_child->placement,
						  mdi_child->band_num, mdi_child->band_pos,
						  mdi_child->offset);
}
