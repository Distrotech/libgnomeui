/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-mdi-child.c - implementation of the base class for MDI children

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

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gnome-app.h"
#include "gnome-app-helper.h"
#include "gnome-mdi-child.h"
#include "gnome-mdi.h"

static void       gnome_mdi_child_class_init       (GnomeMDIChildClass *klass);
static void       gnome_mdi_child_init             (GnomeMDIChild *);
static void       gnome_mdi_child_destroy          (GtkObject *);
static GtkWidget *gnome_mdi_child_set_label        (GnomeMDIChild *, GtkWidget *, gpointer);
static GtkWidget *gnome_mdi_child_create_view      (GnomeMDIChild *);

/* declare the functions from gnome-mdi.c that we need but are not public */
void child_list_menu_remove_item(GnomeMDI *, GnomeMDIChild *);
void child_list_menu_add_item(GnomeMDI *, GnomeMDIChild *);

static GtkObjectClass *parent_class = NULL;

guint gnome_mdi_child_get_type () {
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

static void gnome_mdi_child_class_init (GnomeMDIChildClass *klass) {
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass*)klass;
  
	object_class->destroy = gnome_mdi_child_destroy;
  
	klass->create_view = NULL;
	klass->create_menus = NULL;
	klass->get_config_string = NULL;
	klass->set_label = gnome_mdi_child_set_label;

	parent_class = gtk_type_class (gtk_object_get_type ());
}

static void gnome_mdi_child_init (GnomeMDIChild *mdi_child) {
	mdi_child->name = NULL;
	mdi_child->parent = NULL;
	mdi_child->views = NULL;
}

/* the default set_label function: returns a GtkLabel with child->name
 * if you provide your own, it should return a new widget if its old_label
 * parameter is NULL and modify and return the old widget otherwise. it
 * should (obviously) NOT call the parent class handler!
 */
static GtkWidget *gnome_mdi_child_set_label(GnomeMDIChild *child, GtkWidget *old_label, gpointer data) {
#ifdef GNOME_ENABLE_DEBUG
	printf("GnomeMDIChild: default set_label handler called!\n");
#endif

	if(old_label) {
		gtk_label_set(GTK_LABEL(old_label), child->name);
		return old_label;
	}
	else
		return gtk_label_new(child->name);
}

static void gnome_mdi_child_destroy(GtkObject *obj) {
	GnomeMDIChild *mdi_child;

#ifdef GNOME_ENABLE_DEBUG
	printf("GnomeMDIChild: destroying!\n");
#endif

	mdi_child = GNOME_MDI_CHILD(obj);

	while(mdi_child->views)
		gnome_mdi_child_remove_view(mdi_child, GTK_WIDGET(mdi_child->views->data));

	if(mdi_child->name)
		g_free(mdi_child->name);

	if(GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(GTK_OBJECT(mdi_child));
}

GtkWidget *gnome_mdi_child_add_view(GnomeMDIChild *mdi_child) {
	GtkWidget *view = NULL;

	view = gnome_mdi_child_create_view(mdi_child);

	if(view) {
		mdi_child->views = g_list_append(mdi_child->views, view);

		gtk_object_set_data(GTK_OBJECT(view), "GnomeMDIChild", mdi_child);

		gtk_widget_ref(view);
	}

	return view;
}

void gnome_mdi_child_remove_view(GnomeMDIChild *mdi_child, GtkWidget *view) {
	mdi_child->views = g_list_remove(mdi_child->views, view);

	gtk_widget_destroy(view);
}

void gnome_mdi_child_set_name(GnomeMDIChild *mdi_child, gchar *name) {
	gchar *old_name = mdi_child->name;

	if(mdi_child->parent)
		child_list_menu_remove_item(GNOME_MDI(mdi_child->parent), mdi_child);

	mdi_child->name = (gchar *)g_strdup(name);

	if(old_name)
		g_free(old_name);

	if(mdi_child->parent) {
		child_list_menu_add_item(GNOME_MDI(mdi_child->parent), mdi_child);
		gnome_mdi_update_child(GNOME_MDI(mdi_child->parent), mdi_child);
	}
}

void gnome_mdi_child_set_menu_template(GnomeMDIChild *mdi_child, GnomeUIInfo *menu_tmpl) {
	mdi_child->menu_template = menu_tmpl;
}

static GtkWidget *gnome_mdi_child_create_view(GnomeMDIChild *child) {
	if(GNOME_MDI_CHILD_CLASS(GTK_OBJECT(child)->klass)->create_view)
		return GNOME_MDI_CHILD_CLASS(GTK_OBJECT(child)->klass)->create_view(child, NULL);

	return NULL;
}
