/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-mdi-generic-child.c - implementation of a generic MDI child class.

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

#include "config.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gnome-macros.h"
#include "gnome-mdi-generic-child.h"
#include "gnome-mdi-child.h"
#include "gnome-mdi.h"
#include "gnome-mdiP.h"

static void        gnome_mdi_generic_child_class_init        (GnomeMDIGenericChildClass *klass);
static void        gnome_mdi_generic_child_init              (GnomeMDIGenericChild *child);
static void        gnome_mdi_generic_child_destroy           (GtkObject *child);

static GtkWidget   *gnome_mdi_generic_child_create_view      (GnomeMDIChild *child);
static GList       *gnome_mdi_generic_child_create_menus     (GnomeMDIChild *child,
							      GtkWidget     *view);
static gchar       *gnome_mdi_generic_child_get_config_string(GnomeMDIChild *child);
static GtkWidget   *gnome_mdi_generic_child_set_label        (GnomeMDIChild *child,
							      GtkWidget     *old_label);

GNOME_CLASS_BOILERPLATE (GnomeMDIGenericChild, gnome_mdi_generic_child,
			 GnomeMDIChild, gnome_mdi_child);

static void
gnome_mdi_generic_child_class_init (GnomeMDIGenericChildClass *klass)
{
	GnomeMDIChildClass *mdi_child_klass;
	GtkObjectClass *object_klass;

	object_klass = GTK_OBJECT_CLASS(klass);
	mdi_child_klass = GNOME_MDI_CHILD_CLASS(klass);

	object_klass->destroy = gnome_mdi_generic_child_destroy;

	mdi_child_klass->create_view = gnome_mdi_generic_child_create_view;
	mdi_child_klass->create_menus = gnome_mdi_generic_child_create_menus;
	mdi_child_klass->set_label = gnome_mdi_generic_child_set_label;
	mdi_child_klass->get_config_string = gnome_mdi_generic_child_get_config_string;
}

static void
gnome_mdi_generic_child_init (GnomeMDIGenericChild *child)
{
	child->priv = g_new0(GnomeMDIGenericChildPrivate, 1);

	child->priv->create_view = NULL;
	child->priv->create_menus = NULL;
	child->priv->set_label = NULL;
	child->priv->get_config_string = NULL;

	child->priv->create_view_cbm = NULL;
	child->priv->create_menus_cbm = NULL;
	child->priv->set_label_cbm = NULL;
	child->priv->get_config_string_cbm = NULL;

	child->priv->create_view_dn = NULL;
	child->priv->create_menus_dn = NULL;
	child->priv->set_label_dn = NULL;
	child->priv->get_config_string_dn = NULL;

	child->priv->create_view_data = NULL;
	child->priv->create_menus_data = NULL;
	child->priv->set_label_data = NULL;
	child->priv->get_config_string_data = NULL;
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
GnomeMDIGenericChild *
gnome_mdi_generic_child_new (const gchar *name)
{
	GnomeMDIGenericChild *child;

	child = gtk_type_new(gnome_mdi_generic_child_get_type ());

	GNOME_MDI_CHILD(child)->priv->name = g_strdup(name);

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
void
gnome_mdi_generic_child_set_view_creator (GnomeMDIGenericChild *child,
					  GnomeMDIChildViewCreator func,
					  gpointer data)
{
	g_return_if_fail(child != NULL);
	g_return_if_fail(GNOME_IS_MDI_GENERIC_CHILD(child));

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
void
gnome_mdi_generic_child_set_view_creator_full (GnomeMDIGenericChild *child,
					       GnomeMDIChildViewCreator func,
					       GtkCallbackMarshal marshal,
					       gpointer data,
					       GtkDestroyNotify notify)
{
	g_return_if_fail(child != NULL);
	g_return_if_fail(GNOME_IS_MDI_GENERIC_CHILD(child));

	if(child->priv->create_view_dn)
		child->priv->create_view_dn(child->priv->create_view_data);
	child->priv->create_view = func;
	child->priv->create_view_cbm = marshal;
	child->priv->create_view_data = data;
	child->priv->create_view_dn = notify;
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
void
gnome_mdi_generic_child_set_menu_creator (GnomeMDIGenericChild *child,
										  GnomeMDIChildMenuCreator func,
										  gpointer data)
{
	g_return_if_fail(child != NULL);
	g_return_if_fail(GNOME_IS_MDI_GENERIC_CHILD(child));

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
void
gnome_mdi_generic_child_set_menu_creator_full (GnomeMDIGenericChild *child,
											   GnomeMDIChildMenuCreator func,
											   GtkCallbackMarshal marshal,
											   gpointer data,
											   GtkDestroyNotify notify)
{
	g_return_if_fail(child != NULL);
	g_return_if_fail(GNOME_IS_MDI_GENERIC_CHILD(child));

	if(child->priv->create_menus_dn)
		child->priv->create_menus_dn(child->priv->create_menus_data);
	child->priv->create_menus = func;
	child->priv->create_menus_cbm = marshal;
	child->priv->create_menus_data = data;
	child->priv->create_menus_dn = notify;
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
void
gnome_mdi_generic_child_set_config_func (GnomeMDIGenericChild *child,
										 GnomeMDIChildConfigFunc func,
										 gpointer data)
{
	g_return_if_fail(child != NULL);
	g_return_if_fail(GNOME_IS_MDI_GENERIC_CHILD(child));

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
void
gnome_mdi_generic_child_set_config_func_full (GnomeMDIGenericChild *child,
											  GnomeMDIChildConfigFunc func,
											  GtkCallbackMarshal marshal,
											  gpointer data,
											  GtkDestroyNotify notify)
{
	g_return_if_fail(child != NULL);
	g_return_if_fail(GNOME_IS_MDI_GENERIC_CHILD(child));

	if(child->priv->get_config_string_dn)
		child->priv->get_config_string_dn(child->priv->get_config_string_data);
	child->priv->get_config_string = func;
	child->priv->get_config_string_cbm = marshal;
	child->priv->get_config_string_data = data;
	child->priv->get_config_string_dn = notify;
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
void
gnome_mdi_generic_child_set_label_func (GnomeMDIGenericChild *child,
					GnomeMDIChildLabelFunc func,
					gpointer data)
{
	g_return_if_fail(child != NULL);
	g_return_if_fail(GNOME_IS_MDI_GENERIC_CHILD(child));

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
void
gnome_mdi_generic_child_set_label_func_full (GnomeMDIGenericChild *child,
					     GnomeMDIChildLabelFunc func,
					     GtkCallbackMarshal marshal,
					     gpointer data,
					     GtkDestroyNotify notify)
{
	g_return_if_fail(child != NULL);
	g_return_if_fail(GNOME_IS_MDI_GENERIC_CHILD(child));

	if(child->priv->set_label_dn)
		child->priv->set_label_dn(child->priv->set_label_data);
	child->priv->set_label = func;
	child->priv->set_label_cbm = marshal;
	child->priv->set_label_data = data;
	child->priv->set_label_dn = notify;
}

static GtkWidget *
gnome_mdi_generic_child_create_view (GnomeMDIChild *_child)
{
	GnomeMDIGenericChild *child = GNOME_MDI_GENERIC_CHILD (_child);

	if(!child->priv->create_view && !child->priv->create_view_cbm) {
		g_error("GnomeMDIGenericChild: No method for creating views was provided!");
		return NULL;
	}

	if(child->priv->create_view_cbm) {
		GtkArg args[2];
		GtkWidget *ret = NULL;

		args[0].name = NULL;
		args[0].type = gnome_mdi_child_get_type();
		GTK_VALUE_OBJECT(args[0]) = GTK_OBJECT(child);
		args[1].name = NULL;
		args[1].type = gtk_widget_get_type();
		GTK_VALUE_POINTER(args[1]) = &ret;
		child->priv->create_view_cbm(NULL, child->priv->create_view_data, 1, args);
		return ret;
	}
	else
		return child->priv->create_view(GNOME_MDI_CHILD(child),
								  child->priv->create_view_data);
}

static GList *
gnome_mdi_generic_child_create_menus(GnomeMDIChild *_child,
				     GtkWidget *view)
{
	GnomeMDIGenericChild *child = GNOME_MDI_GENERIC_CHILD (_child);

	if(!child->priv->create_menus && !child->priv->create_menus_cbm)
		return GNOME_CALL_PARENT_HANDLER (GNOME_MDI_CHILD_CLASS,
						  create_menus,
						  (GNOME_MDI_CHILD (child),
						   view));

	if (child->priv->create_menus_cbm) {
		GtkArg args[3];
		GList *ret = NULL;
		
		args[0].name = NULL;
		args[0].type = gnome_mdi_child_get_type();
		GTK_VALUE_OBJECT(args[0]) = GTK_OBJECT(child);
		args[1].name = NULL;
		args[1].type = gtk_widget_get_type();
		GTK_VALUE_OBJECT(args[1]) = GTK_OBJECT(view);
		args[2].name = NULL;
		args[2].type = GTK_TYPE_POINTER; /* should we have a boxed type? */
		GTK_VALUE_POINTER(args[2]) = &ret;
		child->priv->create_menus_cbm(NULL, child->priv->create_menus_data, 2, args);
		return ret;
	}
	else
		return child->priv->create_menus(GNOME_MDI_CHILD(child), view,
										 child->priv->create_menus_data);
}

static gchar *
gnome_mdi_generic_child_get_config_string (GnomeMDIChild *_child)
{
	GnomeMDIGenericChild *child = GNOME_MDI_GENERIC_CHILD (_child);

	if(!child->priv->get_config_string && !child->priv->get_config_string_cbm)
		return GNOME_CALL_PARENT_HANDLER (GNOME_MDI_CHILD_CLASS,
						  get_config_string,
						  (GNOME_MDI_CHILD (child)));

	if(child->priv->get_config_string_cbm) {
		GtkArg args[2];
		gchar *ret = NULL;
		
		args[0].name = NULL;
		args[0].type = gnome_mdi_child_get_type();
		GTK_VALUE_OBJECT(args[0]) = GTK_OBJECT(child);
		args[1].name = NULL;
		args[1].type = GTK_TYPE_STRING;
		GTK_VALUE_POINTER(args[1]) = &ret;
		child->priv->get_config_string_cbm(NULL, child->priv->get_config_string_data,
										   1, args);
		return ret;
	}
	else
		return child->priv->get_config_string(GNOME_MDI_CHILD(child),
											  child->priv->get_config_string_data);
}

static GtkWidget *
gnome_mdi_generic_child_set_label (GnomeMDIChild *_child,
				   GtkWidget *old_label)
{
	GnomeMDIGenericChild *child = GNOME_MDI_GENERIC_CHILD (_child);

	if(!child->priv->set_label && !child->priv->set_label_cbm)
		return GNOME_CALL_PARENT_HANDLER (GNOME_MDI_CHILD_CLASS,
						  set_label,
						  (GNOME_MDI_CHILD (child),
						   old_label));

	if(child->priv->set_label_cbm) {
		GtkArg args[3];
		GtkWidget *ret = NULL;

		args[0].name = NULL;
		args[0].type = gnome_mdi_child_get_type();
		GTK_VALUE_OBJECT(args[0]) = GTK_OBJECT(child);
		args[1].name = NULL;
		args[1].type = gtk_widget_get_type();
		GTK_VALUE_OBJECT(args[0]) = GTK_OBJECT(old_label);
		args[2].name = NULL;
		args[2].type = gtk_widget_get_type();
		GTK_VALUE_POINTER(args[2]) = &ret;
		child->priv->set_label_cbm(NULL, child->priv->set_label_data, 2, args);
		return ret;
	}
	else
		return child->priv->set_label(GNOME_MDI_CHILD(child), old_label,
									  child->priv->set_label_data);
}

static void
gnome_mdi_generic_child_destroy (GtkObject *obj)
{
	GnomeMDIGenericChild *child = GNOME_MDI_GENERIC_CHILD(obj);

	/* remember, destroy can be run multiple times! */

	if(child->priv->create_view_dn)
		child->priv->create_view_dn(child->priv->create_view_data);
	child->priv->create_view_dn = NULL;
	if(child->priv->create_menus_dn)
		child->priv->create_menus_dn(child->priv->create_menus_data);
	child->priv->create_menus_dn = NULL;
	if(child->priv->get_config_string_dn)
		child->priv->get_config_string_dn(child->priv->get_config_string_data);
	child->priv->get_config_string_dn = NULL;
	if(child->priv->set_label_dn)
		child->priv->set_label_dn(child->priv->set_label_data);
	child->priv->set_label_dn = NULL;
	
	GNOME_CALL_PARENT_HANDLER (GTK_OBJECT_CLASS, destroy, (obj));
}
