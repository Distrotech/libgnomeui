/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-mdi-generic-child.c - implementation of a generic MDI child class

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

#include "gnome-mdi-generic-child.h"
#include "gnome-mdi-child.h"
#include "gnome-mdi.h"

static void       gnome_mdi_generic_child_class_init        (GnomeMDIGenericChildClass *klass);
static void       gnome_mdi_generic_child_init              (GnomeMDIGenericChild *child);

static GtkWidget  *gnome_mdi_generic_child_create_view      (GnomeMDIGenericChild *child);
static GList      *gnome_mdi_generic_child_create_menus     (GnomeMDIGenericChild *child,
															 GtkWidget            *view);
static gchar      *gnome_mdi_generic_child_get_config_string(GnomeMDIGenericChild *child);
static GtkWidget  *gnome_mdi_generic_child_set_label        (GnomeMDIGenericChild *child,
															 GtkWidget *old_label);

static GnomeMDIChildClass *parent_class = NULL;

guint gnome_mdi_generic_child_get_type() {
	static guint mdi_gen_child_type = 0;
  
	if (!mdi_gen_child_type) {
		GtkTypeInfo mdi_gen_child_info = {
			"GnomeMDIGenericChild",
			sizeof (GnomeMDIGenericChild),
			sizeof (GnomeMDIGenericChildClass),
			(GtkClassInitFunc) gnome_mdi_generic_child_class_init,
			(GtkObjectInitFunc) gnome_mdi_generic_child_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL,
		};
    
		mdi_gen_child_type = gtk_type_unique (gnome_mdi_child_get_type (), &mdi_gen_child_info);
	}
  
	return mdi_gen_child_type;
}

static void gnome_mdi_generic_child_class_init(GnomeMDIGenericChildClass *klass) {
	GnomeMDIChildClass *mdi_child_klass;

	mdi_child_klass = GNOME_MDI_CHILD_CLASS(klass);

	mdi_child_klass->create_view = (GnomeMDIChildViewCreator)gnome_mdi_generic_child_create_view;
	mdi_child_klass->create_menus = (GnomeMDIChildMenuCreator)gnome_mdi_generic_child_create_menus;
	mdi_child_klass->set_label = (GnomeMDIChildLabelFunc)gnome_mdi_generic_child_set_label;
	mdi_child_klass->get_config_string = (GnomeMDIChildConfigFunc)gnome_mdi_generic_child_get_config_string;

	parent_class = gtk_type_class(gnome_mdi_child_get_type());
}

static void gnome_mdi_generic_child_init(GnomeMDIGenericChild *child) {
	child->create_view = NULL;
	child->create_menus = NULL;
	child->set_label = NULL;
	child->get_config_string = NULL;
	child->user_data;
}

GnomeMDIGenericChild *gnome_mdi_generic_child_new(gchar                    *name,
												  GnomeMDIChildViewCreator create_view,
												  GnomeMDIChildMenuCreator create_menus,
												  GnomeMDIChildConfigFunc  get_config_string,
												  GnomeMDIChildLabelFunc   set_label,
												  gpointer                 user_data) {
	GnomeMDIGenericChild *child;

	child = gtk_type_new(gnome_mdi_generic_child_get_type ());

	GNOME_MDI_CHILD(child)->name = g_strdup(name);
	child->create_view = create_view;
	child->create_menus = create_menus;
	child->get_config_string = get_config_string;
	child->set_label = set_label;
	
	child->user_data = user_data;

	return child;
}

void gnome_mdi_generic_child_set_data(GnomeMDIGenericChild *child,
									  gpointer user_data) {
	child->user_data = user_data;
}

gpointer gnome_mdi_generic_child_get_data(GnomeMDIGenericChild *child) {
	return child->user_data;
}

static GtkWidget *gnome_mdi_generic_child_create_view(GnomeMDIGenericChild *child) {
	if(child->create_view)
		return child->create_view(GNOME_MDI_CHILD(child));

	g_error("GnomeMDIGenericChild: No method for creating views was provided!");

	return NULL;
}

static GList *gnome_mdi_generic_child_create_menus(GnomeMDIGenericChild *child, GtkWidget *view) {
	if(child->create_menus)
		return child->create_menus(GNOME_MDI_CHILD(child), view);

	return NULL;
}

static gchar *gnome_mdi_generic_child_get_config_string(GnomeMDIGenericChild *child) {
	if(child->get_config_string)
		return child->get_config_string(GNOME_MDI_CHILD(child));

	return NULL;
}

static GtkWidget *gnome_mdi_generic_child_set_label(GnomeMDIGenericChild *child, GtkWidget *old_label) {
	if(child->set_label)
		return child->set_label(GNOME_MDI_CHILD(child), old_label);
	else
		return parent_class->set_label(GNOME_MDI_CHILD(child), old_label);
}
