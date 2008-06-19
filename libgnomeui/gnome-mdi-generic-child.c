/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-mdi-generic-child.c - implementation of a generic MDI child class.

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

#include <config.h>
#include <libgnome/gnome-macros.h>

#if !defined(GNOME_DISABLE_DEPRECATED_SOURCE) && !defined(GTK_DISABLE_DEPRECATED)

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <glib/gi18n-lib.h>

#include "gnome-mdi-generic-child.h"
#include "gnome-mdi-child.h"
#include "gnome-mdi.h"

static void       gnome_mdi_generic_child_class_init        (GnomeMDIGenericChildClass *klass);
static void       gnome_mdi_generic_child_instance_init     (GnomeMDIGenericChild *child);
static void       gnome_mdi_generic_child_destroy           (GtkObject *);

static GtkWidget  *gnome_mdi_generic_child_create_view      (GnomeMDIGenericChild *child);
static GList      *gnome_mdi_generic_child_create_menus     (GnomeMDIGenericChild *child,
							     GtkWidget            *view);
static gchar      *gnome_mdi_generic_child_get_config_string(GnomeMDIGenericChild *child);
static GtkWidget  *gnome_mdi_generic_child_set_label        (GnomeMDIGenericChild *child,
							     GtkWidget *old_label);

GNOME_CLASS_BOILERPLATE (GnomeMDIGenericChild, gnome_mdi_generic_child,
						 GnomeMDIChild, GNOME_TYPE_MDI_CHILD)

static void gnome_mdi_generic_child_class_init (GnomeMDIGenericChildClass *klass)
{
	GnomeMDIChildClass *mdi_child_klass;
	GtkObjectClass *object_klass;

	object_klass = GTK_OBJECT_CLASS(klass);
	mdi_child_klass = GNOME_MDI_CHILD_CLASS(klass);

	object_klass->destroy = gnome_mdi_generic_child_destroy;

	mdi_child_klass->create_view = (GnomeMDIChildViewCreator)gnome_mdi_generic_child_create_view;
	mdi_child_klass->create_menus = (GnomeMDIChildMenuCreator)gnome_mdi_generic_child_create_menus;
	mdi_child_klass->set_label = (GnomeMDIChildLabelFunc)gnome_mdi_generic_child_set_label;
	mdi_child_klass->get_config_string = (GnomeMDIChildConfigFunc)gnome_mdi_generic_child_get_config_string;
}

static void gnome_mdi_generic_child_instance_init (GnomeMDIGenericChild *child)
{
	child->create_view = NULL;
	child->create_menus = NULL;
	child->set_label = NULL;
	child->get_config_string = NULL;

	child->create_view_cbm = NULL;
	child->create_menus_cbm = NULL;
	child->set_label_cbm = NULL;
	child->get_config_string_cbm = NULL;

	child->create_view_dn = NULL;
	child->create_menus_dn = NULL;
	child->set_label_dn = NULL;
	child->get_config_string_dn = NULL;

	child->create_view_data = NULL;
	child->create_menus_data = NULL;
	child->set_label_data = NULL;
	child->get_config_string_data = NULL;
}

/**
 * gnome_mdi_generic_child_new:
 * @name: the name of this MDI child.
 * 
 * Creates a new mdi child, which has the ability to set view creators, etc
 * on an instance basis (rather than on a class basis like &GnomeMDIChild).
 *
 * After creation, you will need to set, at a minimum, the view creator
 * function.
 * 
 * Return value: A newly created &GnomeMDIGenericChild object.
 **/
GnomeMDIGenericChild *gnome_mdi_generic_child_new (const gchar *name)
{
	GnomeMDIGenericChild *child;

	child = g_object_new (GNOME_TYPE_MDI_GENERIC_CHILD, NULL);

	GNOME_MDI_CHILD(child)->name = g_strdup(name);

	return child;
}

/**
 * gnome_mdi_generic_child_set_view_creator:
 * @child: the mdi child object
 * @func: a function used to create views
 * @data: optional user data.
 * 
 * This function sets the function that is used to create new views for this
 * particular mdi child object.  The function should return a newly created
 * widget (the view).
 *
 * A &GnomeMDIGenericChild must have a view creator.
 **/
void gnome_mdi_generic_child_set_view_creator (GnomeMDIGenericChild *child,
											   GnomeMDIChildViewCreator func,
											   gpointer data)
{
        gnome_mdi_generic_child_set_view_creator_full(child,func,NULL,data,NULL);
}

/**
 * gnome_mdi_generic_child_set_view_creator_full:
 * @child: the mdi child object
 * @func: a function to create views (not used if @marshal != %NULL)
 * @marshal: a callback marshaller
 * @data: optional user data
 * @notify: a function used to free the user data.
 * 
 * Similar to gnome_mdi_generic_child_set_view_creator(), except that it gives
 * more control to the programmer.  If @marshal is not %NULL, then it will
 * be called instead of @func.
 *
 * The &GtkArg array passed to @marshal will be of length 2.  The first
 * element will be @child, and the second is the return value (a pointer to
 * a &GtkWidget).
 **/
void gnome_mdi_generic_child_set_view_creator_full (GnomeMDIGenericChild *child,
												    GnomeMDIChildViewCreator func,
													GtkCallbackMarshal marshal,
													gpointer data,
													GDestroyNotify notify)
{
	if(child->create_view_dn)
		child->create_view_dn(child->create_view_data);
	child->create_view = func;
	child->create_view_cbm = marshal;
	child->create_view_data = data;
	child->create_view_dn = notify;
}

/**
 * gnome_mdi_generic_child_set_menu_creator:
 * @child: the mdi child object
 * @func: a function to create a list of child specific menus
 * @data: optional user data
 * 
 * Sets the function used to create child specific menus.  The function
 * should return a &GList of the menus created.
 *
 * A &GnomeMDIGenericChild doesn't require a menu creator.
 **/
void gnome_mdi_generic_child_set_menu_creator (GnomeMDIGenericChild *child,
											   GnomeMDIChildMenuCreator func,
											   gpointer data)
{
	gnome_mdi_generic_child_set_menu_creator_full(child,func,NULL,data,NULL);
}

/**
 * gnome_mdi_generic_child_set_menu_creator_full:
 * @child: the mdi child object
 * @func: a menu creator function (not used if @marshal != %NULL)
 * @marshal: a callback marshaller
 * @data: optional user data
 * @notify: a destroy notify for the data
 * 
 * This function is similar to gnome_mdi_generic_child_set_menu_creator(),
 * but gives extra flexibility to the programmer, in the form of a a
 * destroy notify for the user data, and a callback marshaller.
 *
 * The &GtkArg array passed to @marshal is of length 3.  The first element
 * will be @child, the second will be a view of @child, and the third is the
 * return value (a pointer to the returned GList).
 **/
void gnome_mdi_generic_child_set_menu_creator_full (GnomeMDIGenericChild *child,
													GnomeMDIChildMenuCreator func,
													GtkCallbackMarshal marshal,
													gpointer data,
													GDestroyNotify notify)
{
	if(child->create_menus_dn)
		child->create_menus_dn(child->create_menus_data);
	child->create_menus = func;
	child->create_menus_cbm = marshal;
	child->create_menus_data = data;
	child->create_menus_dn = notify;
}

/**
 * gnome_mdi_generic_child_set_config_func:
 * @child: the mdi child object
 * @func: a function to set the config key for session saves
 * @data: optional user data
 * 
 * Sets the function used to get the config key used for session saves.
 *
 * A &GnomeMDIGenericChild doesn't require a config func.
 **/
void gnome_mdi_generic_child_set_config_func (GnomeMDIGenericChild *child,
											  GnomeMDIChildConfigFunc func,
											  gpointer data)
{
	gnome_mdi_generic_child_set_config_func_full(child,func,NULL,data,NULL);
}

/**
 * gnome_mdi_generic_child_set_config_func_full:
 * @child: the mdi child object
 * @func: a function (not used if @marshal != %NULL)
 * @marshal: a callback marshaller
 * @data: optional user data
 * @notify: a destroy notify for the user data
 * 
 * A function similar to gnome_mdi_generic_child_set_config_func(), except
 * it gives more control to the programmer.
 *
 * The &GtkArg array passed to @marshal is of length 2.  The first element is
 * @child, and the second is the return value (a pointer to a string).
 **/
void gnome_mdi_generic_child_set_config_func_full (GnomeMDIGenericChild *child,
												   GnomeMDIChildConfigFunc func,
												   GtkCallbackMarshal marshal,
												   gpointer data,
												   GDestroyNotify notify)
{
	if(child->get_config_string_dn)
		child->get_config_string_dn(child->get_config_string_data);
	child->get_config_string = func;
	child->get_config_string_cbm = marshal;
	child->get_config_string_data = data;
	child->get_config_string_dn = notify;
}

/**
 * gnome_mdi_generic_child_set_label_func:
 * @child: a mdi child object
 * @func: a function
 * @data: optional user data
 * 
 * Sets the function used to set (or modify) the label for @child.  The
 * first argument to @func is @child.  If a label exists, it will be passed
 * to @func as the second argument, otherwise, %NULL is passed.  The
 * function should return the modified label.
 *
 * A &GnomeMDIGenericChild doesn't require a label function.
 **/
void gnome_mdi_generic_child_set_label_func (GnomeMDIGenericChild *child,
											 GnomeMDIChildLabelFunc func,
											 gpointer data)
{
	gnome_mdi_generic_child_set_label_func_full(child,func,NULL,data,NULL);
}

/**
 * gnome_mdi_generic_child_set_label_func_full:
 * @child: the mdi child object
 * @func: a function (not used if @marshal != %NULL)
 * @marshal: a callback marshaller
 * @data: optional user data
 * @notify: a destroy notify for the data
 * 
 * Similar to gnome_mdi_generic_child_set_label_func(), except it gives more
 * flexibility to the programmer.
 *
 * The &GtkArg array passed to @marshal is of length 3.  The first argument
 * is @child, the second is the old widget (or %NULL), and the third is the
 * return value (a pointer to a &GtkWidget).
 **/
void gnome_mdi_generic_child_set_label_func_full (GnomeMDIGenericChild *child,
												  GnomeMDIChildLabelFunc func,
												  GtkCallbackMarshal marshal,
												  gpointer data,
												  GDestroyNotify notify)
{
	if(child->set_label_dn)
		child->set_label_dn(child->set_label_data);
	child->set_label = func;
	child->set_label_cbm = marshal;
	child->set_label_data = data;
	child->set_label_dn = notify;
}

static GtkWidget *gnome_mdi_generic_child_create_view (GnomeMDIGenericChild *child)
{
	if(!child->create_view && !child->create_view_cbm) {
		g_error("GnomeMDIGenericChild: No method for creating views was provided!");
		return NULL;
	}

	if(child->create_view_cbm) {
		GtkArg args[2];
		GtkWidget *ret = NULL;

		args[0].name = NULL;
		args[0].type = GNOME_TYPE_MDI_CHILD;
		GTK_VALUE_OBJECT(args[0]) = GTK_OBJECT(child);
		args[1].name = NULL;
		args[1].type = GTK_TYPE_WIDGET;
		GTK_VALUE_POINTER(args[1]) = &ret;
		child->create_view_cbm(NULL, child->create_view_data, 1, args);
		return ret;
	}
	else
		return child->create_view(GNOME_MDI_CHILD(child),
						 child->create_view_data);
}

static GList *gnome_mdi_generic_child_create_menus(GnomeMDIGenericChild *child, GtkWidget *view)
{
	if(!child->create_menus && !child->create_menus_cbm)
		return NULL;

	if (child->create_menus_cbm) {
		GtkArg args[3];
		GList *ret = NULL;
		
		args[0].name = NULL;
		args[0].type = GNOME_TYPE_MDI_CHILD;
		GTK_VALUE_OBJECT(args[0]) = GTK_OBJECT(child);
		args[1].name = NULL;
		args[1].type = GTK_TYPE_WIDGET;
		GTK_VALUE_OBJECT(args[0]) = GTK_OBJECT(view);
		args[2].name = NULL;
		args[2].type = GTK_TYPE_POINTER; /* should we have a boxed type? */
		GTK_VALUE_POINTER(args[2]) = &ret;
		child->create_menus_cbm(NULL, child->create_menus_data, 2, args);
		return ret;
	}
	else
		return child->create_menus(GNOME_MDI_CHILD(child), view,
						  child->create_menus_data);
}

static gchar *gnome_mdi_generic_child_get_config_string (GnomeMDIGenericChild *child)
{
	if(!child->get_config_string && !child->get_config_string_cbm)
		return NULL;

	if(child->get_config_string_cbm) {
		GtkArg args[2];
		gchar *ret = NULL;
		
		args[0].name = NULL;
		args[0].type = GNOME_TYPE_MDI_CHILD;
		GTK_VALUE_OBJECT(args[0]) = GTK_OBJECT(child);
		args[1].name = NULL;
		args[1].type = GTK_TYPE_STRING;
		GTK_VALUE_POINTER(args[1]) = &ret;
		child->get_config_string_cbm(NULL, child->get_config_string_data,
									 1, args);
		return ret;
	}
	else
		return child->get_config_string(GNOME_MDI_CHILD(child),
						       child->get_config_string_data);
}

static GtkWidget *gnome_mdi_generic_child_set_label (GnomeMDIGenericChild *child, GtkWidget *old_label)
{
	if(!child->set_label && !child->set_label_cbm)
		return parent_class->set_label(GNOME_MDI_CHILD(child), old_label, NULL);

	if(child->set_label_cbm) {
		GtkArg args[3];
		GtkWidget *ret = NULL;

		args[0].name = NULL;
		args[0].type = GNOME_TYPE_MDI_CHILD;
		GTK_VALUE_OBJECT(args[0]) = GTK_OBJECT(child);
		args[1].name = NULL;
		args[1].type = GTK_TYPE_WIDGET;
		GTK_VALUE_OBJECT(args[0]) = GTK_OBJECT(old_label);
		args[2].name = NULL;
		args[2].type = GTK_TYPE_WIDGET;
		GTK_VALUE_POINTER(args[2]) = &ret;
		child->set_label_cbm(NULL, child->set_label_data, 2, args);
		return ret;
	}
	else
		return child->set_label(GNOME_MDI_CHILD(child), old_label,
					       child->set_label_data);
}

static void gnome_mdi_generic_child_destroy (GtkObject *obj)
{
	GnomeMDIGenericChild *child = GNOME_MDI_GENERIC_CHILD(obj);

	if(child->create_view_dn)
		child->create_view_dn(child->create_view_data);
	child->create_view_dn = NULL;
	if(child->create_menus_dn)
		child->create_menus_dn(child->create_menus_data);
	child->create_menus_dn = NULL;
	if(child->get_config_string_dn)
		child->get_config_string_dn(child->get_config_string_data);
	child->get_config_string_dn = NULL;
	if(child->set_label_dn)
		child->set_label_dn(child->set_label_data);
	child->set_label_dn = NULL;
	
	if(GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(obj);
}

#endif /* !defined(GNOME_DISABLE_DEPRECATED_SOURCE) && !defined(GTK_DISABLE_DEPRECATED) */
