/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 1997, 1998, 1999, 2000 Red Hat Software, The Free
 * Software Foundation, Miguel de Icaza, Federico Menu, Chris Toshok.
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  */
/*
  @NOTATION@
 */

/*
 * Originally by Elliot Lee,
 *
 * improvements and rearrangement by Miguel,
 * and I don't know what you other people did :)
 *
 * Even more changes by Federico Mena.
 *
 * Toolbar separators and configurable relief by Andrew Veliath.
 *
 * Modified by Ettore Perazzoli to support GnomeDock.
 *
 */

#include "config.h"
#include "gnome-macros.h"

#include <string.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-program.h>

#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-util.h>
#include <libgnome/gnome-config.h>
#include <libgnome/gnome-preferences.h>
#include "gnome-app-helper.h"
#include "gnome-uidefs.h"
#include "gnome-dock.h"
#include "gnome-init.h"
#include "gnome-helpsys.h"
#include "gnome-window.h"

#include "gnome-app.h"

#define LAYOUT_CONFIG_PATH      "Placement/Dock"

struct _GnomeAppPrivate
{
	int dummy;
	/* Nothing right now, needs to get filled with the private things */
	/* XXX: When stuff is added, uncomment the allocation in the
	 * gnome_app_init function! */
};


static void gnome_app_class_init  (GnomeAppClass *class);
static void gnome_app_init        (GnomeApp      *app);
static void gnome_app_destroy     (GtkObject     *object);
static void gnome_app_finalize    (GObject       *object);
static void gnome_app_show        (GtkWidget     *widget);
static void gnome_app_get_property   (GObject       *object,
				   guint          param_id,
				   GValue        *value,
				   GParamSpec    *pspec);
static void gnome_app_set_property   (GObject       *object,
				   guint          param_id,
				   const GValue  *value,
				   GParamSpec    *pspec);

static gchar *read_layout_config  (GnomeApp      *app);
static void   write_layout_config (GnomeApp      *app,
				   GnomeDockLayout *layout);
static void   layout_changed      (GtkWidget     *widget,
				   gpointer       data);

/* define _get_type and parent_class */
GNOME_CLASS_BOILERPLATE (GnomeApp, gnome_app,
			 GtkWindow, gtk_window)

static gchar *
read_layout_config (GnomeApp *app)
{
	gchar *s;

	gnome_config_push_prefix (app->prefix);
	s = gnome_config_get_string (LAYOUT_CONFIG_PATH);
	gnome_config_pop_prefix ();

	return s;
}

static void
write_layout_config (GnomeApp *app, GnomeDockLayout *layout)
{
	gchar *s;

	s = gnome_dock_layout_create_string (layout);
	gnome_config_push_prefix (app->prefix);
	gnome_config_set_string (LAYOUT_CONFIG_PATH, s);
	gnome_config_pop_prefix ();
	gnome_config_sync ();

	g_free (s);
}

enum {
	PROP_0,
	PROP_APP_ID
};

static void
gnome_app_class_init (GnomeAppClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;

	gobject_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;

	gobject_class->finalize = gnome_app_finalize;
	gobject_class->set_property = gnome_app_set_property;
	gobject_class->get_property = gnome_app_get_property;

	object_class->destroy = gnome_app_destroy;

	widget_class->show = gnome_app_show;

	g_object_class_install_property (gobject_class,
				      PROP_APP_ID,
				      g_param_spec_string ("app_id",
							   _("App ID"),
							   _("The application ID string"),
							   NULL,
							   (G_PARAM_READABLE |
							    G_PARAM_WRITABLE)));
}

static void
gnome_app_set_property (GObject       *object,
		     guint          param_id,
		     const GValue  *value,
		     GParamSpec    *pspec)
{
	GnomeApp *app = GNOME_APP (object);

	switch(param_id) {
	case PROP_APP_ID:
		g_free (app->name);
		app->name = g_value_dup_string (value);
		g_free (app->prefix);
		app->prefix = g_strconcat("/", app->name, "/", NULL);
		break;
	}
}

static void
gnome_app_get_property (GObject       *object,
		     guint          param_id,
		     GValue        *value,
		     GParamSpec    *pspec)
{
	GnomeApp *app = GNOME_APP (object);

	switch(param_id) {
	case PROP_APP_ID:
		g_value_set_string (value, app->name);
		break;
	}
}

static void
gnome_app_init (GnomeApp *app)
{
	const char *str = NULL;
	GValue value = { 0, };

	app->_priv = NULL;
	/* XXX: when there is some private stuff enable this
	app->_priv = g_new0(GnomeAppPrivate, 1);
	*/

	app->name = NULL;
	app->prefix = NULL;

	app->accel_group = gtk_accel_group_new ();
	gtk_window_add_accel_group (GTK_WINDOW (app), app->accel_group);
	
	app->vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (app), app->vbox);

	app->dock = gnome_dock_new ();
	gtk_box_pack_start (GTK_BOX (app->vbox), app->dock,
			    TRUE, TRUE, 0);

	gtk_signal_connect (GTK_OBJECT (app->dock),
			    "layout_changed",
			    GTK_SIGNAL_FUNC (layout_changed),
			    (gpointer) app);

	app->layout = gnome_dock_layout_new ();

	app->enable_layout_config = TRUE;
	g_value_init (&value, G_TYPE_STRING);
	g_object_get_property (G_OBJECT (gnome_program_get ()),
			       LIBGNOMEUI_PARAM_DEFAULT_ICON, &value);
	str = g_value_get_string (&value);
	if (str != NULL)
		gnome_window_set_icon_from_file (GTK_WINDOW (app), str, FALSE);
	g_value_unset (&value);
}

static void
gnome_app_show (GtkWidget *widget)
{
	GnomeApp *app;

	app = GNOME_APP (widget);

	if (app->layout != NULL) {
		if (app->enable_layout_config) {
			gchar *s;

			/* Override the layout with the user's saved
			   configuration.  */
			s = read_layout_config (app);
			gnome_dock_layout_parse_string (app->layout, s);
			g_free (s);
		}

		gnome_dock_add_from_layout (GNOME_DOCK (app->dock),
					    app->layout);

		if (app->enable_layout_config)
			write_layout_config (app, app->layout);

		gtk_object_unref (GTK_OBJECT (app->layout));
		app->layout = NULL;
	}
			
	gtk_widget_show (app->vbox);
	gtk_widget_show (app->dock);

	GNOME_CALL_PARENT_HANDLER (GTK_WIDGET_CLASS, show, (widget));
}

static void
layout_changed (GtkWidget *w, gpointer data)
{
	GnomeApp *app;

	g_return_if_fail (GNOME_IS_APP (data));
	g_return_if_fail (GNOME_IS_DOCK (w));

	app = GNOME_APP (data);

	if (app->enable_layout_config) {
		GnomeDockLayout *layout;

		layout = gnome_dock_get_layout (GNOME_DOCK (app->dock));
		write_layout_config (app, layout);
		gtk_object_unref (GTK_OBJECT (layout));
	}
}

/**
 * gnome_app_new
 * @appname: Name of program, using in file names and paths.
 * @title: Window title for application.
 *
 * Description:
 * Create a new (empty) application window.  You must specify the
 * application's name (used internally as an identifier).
 * @title can be left as NULL, in which case the window's title will
 * not be set.
 *
 * Returns: Pointer to new GNOME app object.
 **/

GtkWidget *
gnome_app_new (const gchar *appname, const gchar *title)
{
	GtkObject *app;

	g_return_val_if_fail (appname != NULL, NULL);

	if (title != NULL) {
		app = g_object_new (gnome_app_get_type (),
				    "app_id", appname,
				    NULL);
		gtk_object_set (GTK_OBJECT (app),
				"title", title,
				NULL);
	} else {
		app = g_object_new (gnome_app_get_type (),
				    "app_id", appname,
				    NULL);
	}
	return GTK_WIDGET (app);
}

/**
 * gnome_app_construct
 * @app: Pointer to newly-created GNOME app object.
 * @appname: Name of program, using in file names and paths.
 * @title: Window title for application.
 *
 * Description:
 * Constructor for language bindings; you don't normally need this.
 **/

void 
gnome_app_construct (GnomeApp *app, const gchar *appname, const gchar *title)
{
	g_return_if_fail (appname != NULL);

	if (title != NULL) {
		g_object_set (G_OBJECT (app),
			      "app_id", appname,
			      NULL);
		gtk_object_set (GTK_OBJECT (app),
				"title", title,
				NULL);
	} else {
		g_object_set (G_OBJECT (app),
			      "app_id", appname,
			      NULL);
	}
}

static void
gnome_app_destroy (GtkObject *object)
{
	GnomeApp *app;

	/* remember, destroy can be run multiple times! */
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_APP (object));

	app = GNOME_APP (object);

	g_free (app->name);
	app->name = NULL;
	g_free (app->prefix);
	app->prefix = NULL;

	if (app->accel_group != NULL) {
		gtk_accel_group_unref (app->accel_group);
		app->accel_group = NULL;
	}

	if (app->layout != NULL) {
		gtk_object_unref (GTK_OBJECT (app->layout));
		app->layout = NULL;
	}

	GNOME_CALL_PARENT_HANDLER (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gnome_app_finalize (GObject *object)
{
	GnomeApp *app;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_APP (object));

	app = GNOME_APP (object);

	/* Free private data */
	g_free(app->_priv);
	app->_priv = NULL;

	GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, finalize, (object));
}

/* Callback for an app's contents' parent_set signal.  We set the app->contents
 * to NULL and detach the signal.
 */
static void
contents_parent_set (GtkWidget *widget, GtkWidget *previous_parent, gpointer data)
{
	GnomeApp *app;

	app = GNOME_APP (data);

	g_assert (previous_parent == app->dock);

	gtk_signal_disconnect_by_func (GTK_OBJECT (widget),
				       GTK_SIGNAL_FUNC (contents_parent_set),
				       data);

	app->contents = NULL;
}

/**
 * gnome_app_set_contents
 * @app: Pointer to GNOME app object.
 * @contents: Widget to be application content area.
 *
 * Description:
 * Sets the content area of the GNOME app's main window.
 **/
void
gnome_app_set_contents (GnomeApp *app, GtkWidget *contents)
{
	GtkWidget *new_contents;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP(app));
	g_return_if_fail (app->dock != NULL);
	g_return_if_fail (contents != NULL);
	g_return_if_fail (GTK_IS_WIDGET (contents));

	gnome_dock_set_client_area (GNOME_DOCK (app->dock), contents);

	/* Re-fetch it in case it did not change */
	new_contents = gnome_dock_get_client_area (GNOME_DOCK (app->dock));

	if (new_contents == contents && new_contents != app->contents) {
		gtk_widget_show (new_contents);
		gtk_signal_connect (GTK_OBJECT (new_contents), "parent_set",
				    GTK_SIGNAL_FUNC (contents_parent_set),
				    app);

		app->contents = new_contents;
	}
}

/**
 * gnome_app_set_menus
 * @app: Pointer to GNOME app object.
 * @menubar: Menu bar widget for main app window.
 *
 * Description:
 * Sets the menu bar of the application window.
 **/

void
gnome_app_set_menus (GnomeApp *app, GtkMenuBar *menubar)
{
	GtkWidget *dock_item;
	GtkAccelGroup *ag;
	GnomeDockItemBehavior behavior;

	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));
	g_return_if_fail(app->menubar == NULL);
	g_return_if_fail(menubar != NULL);
	g_return_if_fail(GTK_IS_MENU_BAR(menubar));

	behavior = (GNOME_DOCK_ITEM_BEH_EXCLUSIVE
		    | GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL);
	
	if (!gnome_preferences_get_menubar_detachable())
		behavior |= GNOME_DOCK_ITEM_BEH_LOCKED;

	dock_item = gnome_dock_item_new (GNOME_APP_MENUBAR_NAME,
					 behavior);
	gtk_container_add (GTK_CONTAINER (dock_item), GTK_WIDGET (menubar));

	app->menubar = GTK_WIDGET (menubar);

	/* To have menubar relief agree with the toolbar (and have the relief
	 * outside of smaller handles), substitute the dock item's relief for
	 * the menubar's relief, but don't change the size of the menubar in
	 * the process. */
#ifdef FIXME
	gtk_menu_bar_set_shadow_type (GTK_MENU_BAR (app->menubar),
				      GTK_SHADOW_NONE);
#endif
	if (gnome_preferences_get_menubar_relief ()) {
		guint border_width;
		
		gtk_container_set_border_width (GTK_CONTAINER (dock_item), 2);
		border_width = GTK_CONTAINER (app->menubar)->border_width;
		if (border_width >= 2)
			border_width -= 2;
		gtk_container_set_border_width (GTK_CONTAINER (app->menubar),
						border_width);
	} else
		gnome_dock_item_set_shadow_type (GNOME_DOCK_ITEM (dock_item),
						 GTK_SHADOW_NONE);

	if (app->layout != NULL)
		gnome_dock_layout_add_item (app->layout,
					    GNOME_DOCK_ITEM (dock_item),
					    GNOME_DOCK_TOP,
					    0, 0, 0);
	else
		gnome_dock_add_item(GNOME_DOCK(app->dock),
				    GNOME_DOCK_ITEM (dock_item),
				    GNOME_DOCK_TOP,
				    0, 0, 0, TRUE);

	gtk_widget_show (GTK_WIDGET (menubar));
	gtk_widget_show (GTK_WIDGET (dock_item));

	ag = gtk_object_get_data(GTK_OBJECT(app), "GtkAccelGroup");
	if (ag && !g_slist_find(gtk_accel_groups_from_object (GTK_OBJECT (app)), ag))
	        gtk_window_add_accel_group(GTK_WINDOW(app), ag);
}


/**
 * gnome_app_set_statusbar
 * @app: Pointer to GNOME app object
 * @statusbar: Statusbar widget for main app window
 *
 * Description:
 * Sets the status bar of the application window.
 **/

void
gnome_app_set_statusbar (GnomeApp *app,
		         GtkWidget *statusbar)
{
	GtkWidget *hbox;

	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));
	g_return_if_fail(statusbar != NULL);
	g_return_if_fail(app->statusbar == NULL);

	app->statusbar = GTK_WIDGET(statusbar);
	gtk_widget_show(app->statusbar);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 0);
	gtk_box_pack_start(GTK_BOX(hbox), statusbar, TRUE, TRUE, 0);
	gtk_widget_show(hbox);

	gtk_box_pack_start (GTK_BOX (app->vbox), hbox, FALSE, FALSE, 0);
}

/**
 * gnome_app_set_statusbar_custom
 * @app: Pointer to GNOME app object
 * @container: container widget containing the statusbar
 * @statusbar: Statusbar widget for main app window
 *
 * Description:
 * Sets the status bar of the application window, but use @container
 * as its container.
 *
 **/

void
gnome_app_set_statusbar_custom (GnomeApp *app,
				GtkWidget *container,
				GtkWidget *statusbar)
{
	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));
	g_return_if_fail(container != NULL);
	g_return_if_fail(GTK_IS_CONTAINER(container));
	g_return_if_fail(statusbar != NULL);
	g_return_if_fail(app->statusbar == NULL);

	app->statusbar = GTK_WIDGET(statusbar);

	gtk_box_pack_start (GTK_BOX (app->vbox), container, FALSE, FALSE, 0);
}

/**
 * gnome_app_add_toolbar:
 * @app: A &GnomeApp widget
 * @toolbar: Toolbar to be added to @app's dock
 * @name: Name for the dock item that will contain @toolbar
 * @behavior: Behavior for the new dock item
 * @placement: Placement for the new dock item
 * @band_num: Number of the band where the dock item should be placed
 * @band_position: Position of the new dock item in band @band_num
 * @offset: Offset from the previous dock item in the band; if there is
 * no previous item, offset from the beginning of the band.
 *
 * Create a new &GnomeDockItem widget containing @toolbar, and add it
 * to @app's dock with the specified layout information.  Notice that,
 * if automatic layout configuration is enabled, the layout is
 * overridden by the saved configuration, if any.
 **/
void
gnome_app_add_toolbar (GnomeApp *app,
		       GtkToolbar *toolbar,
		       const gchar *name,
		       GnomeDockItemBehavior behavior,
		       GnomeDockPlacement placement,
		       gint band_num,
		       gint band_position,
		       gint offset)
{

	GtkWidget *dock_item;
	GtkAccelGroup *ag;

	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));
	g_return_if_fail(toolbar != NULL);

	dock_item = gnome_dock_item_new (name, behavior);

	gtk_container_set_border_width (GTK_CONTAINER (toolbar), 1);
	gtk_container_add (GTK_CONTAINER (dock_item), GTK_WIDGET (toolbar));

	if(app->layout)
		gnome_dock_layout_add_item (app->layout,
					    GNOME_DOCK_ITEM (dock_item),
					    placement,
					    band_num,
					    band_position,
					    offset);
	else
		gnome_dock_add_item (GNOME_DOCK(app->dock),
				     GNOME_DOCK_ITEM (dock_item),
				     placement,
				     band_num,
				     band_position,
				     offset,
				     TRUE);


	gnome_app_setup_toolbar(toolbar, GNOME_DOCK_ITEM(dock_item));
	
	gtk_widget_show (GTK_WIDGET (toolbar));
	gtk_widget_show (GTK_WIDGET (dock_item));

	ag = gtk_object_get_data(GTK_OBJECT(app), "GtkAccelGroup");
	if (ag && !g_slist_find(gtk_accel_groups_from_object (GTK_OBJECT (app)), ag))
	        gtk_window_add_accel_group(GTK_WINDOW(app), ag);
}

/**
 * gnome_app_set_toolbar
 * @app: Pointer to GNOME app object.
 * @toolbar: Toolbar widget for main app window.
 *
 * Description:
 * Sets the main toolbar of the application window.
 **/

void
gnome_app_set_toolbar (GnomeApp *app,
		       GtkToolbar *toolbar)
{
	GnomeDockItemBehavior behavior;

	/* Making dock items containing toolbars use
	   `GNOME_DOCK_ITEM_BEH_EXCLUSIVE' is not really a
	   requirement.  We only do this for backwards compatibility.  */
	behavior = GNOME_DOCK_ITEM_BEH_EXCLUSIVE;
	
	if (!gnome_preferences_get_toolbar_detachable())
		behavior |= GNOME_DOCK_ITEM_BEH_LOCKED;
	
	gnome_app_add_toolbar (app, toolbar,
			       GNOME_APP_TOOLBAR_NAME,
			       behavior,
			       GNOME_DOCK_TOP,
			       1, 0, 0);
}


/**
 * gnome_app_add_dock_item:
 * @app: A &GnomeApp widget
 * @item: Dock item to be added to @app's dock.
 * @placement: Placement for the dock item
 * @band_num: Number of the band where the dock item should be placed
 * @band_position: Position of the dock item in band @band_num
 * @offset: Offset from the previous dock item in the band; if there is
 * no previous item, offset from the beginning of the band.
 * 
 * Add @item according to the specified layout information.  Notice
 * that, if automatic layout configuration is enabled, the layout is
 * overridden by the saved configuration, if any.
 **/
void
gnome_app_add_dock_item (GnomeApp *app,
			 GnomeDockItem *item,
			 GnomeDockPlacement placement,
			 gint band_num,
			 gint band_position,
			 gint offset)
{
	if (app->layout)
		gnome_dock_layout_add_item (app->layout,
					    GNOME_DOCK_ITEM (item),
					    placement,
					    band_num,
					    band_position,
					    offset);
	else
		gnome_dock_add_item (GNOME_DOCK(app->dock),
				     GNOME_DOCK_ITEM( item),
				     placement,
				     band_num,
				     band_position,
				     offset,
				     FALSE);

	gtk_signal_emit_by_name (GTK_OBJECT (app->dock),
				 "layout_changed",
				 (gpointer) app);
}

/**
 * gnome_app_add_docked:
 * @app: A &GnomeApp widget
 * @widget: Widget to be added to the &GnomeApp
 * @name: Name for the new dock item
 * @behavior: Behavior for the new dock item
 * @placement: Placement for the new dock item
 * @band_num: Number of the band where the dock item should be placed
 * @band_position: Position of the new dock item in band @band_num
 * @offset: Offset from the previous dock item in the band; if there is
 * no previous item, offset from the beginning of the band.
 *
 * Returns: The dock item used to contain the widget.
 * 
 * Add @widget as a dock item according to the specified layout
 * information.  Notice that, if automatic layout configuration is
 * enabled, the layout is overridden by the saved configuration, if
 * any.
 **/
GtkWidget *
gnome_app_add_docked (GnomeApp *app,
		      GtkWidget *widget,
		      const gchar *name,
		      GnomeDockItemBehavior behavior,
		      GnomeDockPlacement placement,
		      gint band_num,
		      gint band_position,
		      gint offset)
{
	GtkWidget *item;

	item = gnome_dock_item_new (name, behavior);
	gtk_container_add (GTK_CONTAINER (item), widget);
	gnome_app_add_dock_item (app, GNOME_DOCK_ITEM (item),
				 placement, band_num, band_position, offset);

	return item;
}

/**
 * gnome_app_enable_layout_config:
 * @app: A &GnomeApp widget
 * @enable: Boolean specifying whether automatic configuration saving
 * is enabled
 * 
 * Specify whether @app should automatically save the dock's
 * layout configuration via gnome-config whenever it changes or not.
 **/
void
gnome_app_enable_layout_config (GnomeApp *app, gboolean enable)
{
	app->enable_layout_config = enable;
}

/**
 * gnome_app_get_dock_item_by_name:
 * @app: A &GnomeApp widget
 * @name: Name of the dock item to retrieve
 * 
 * Retrieve the dock item whose name matches @name.
 * 
 * Return value: The retrieved dock item.
 **/
GnomeDockItem *
gnome_app_get_dock_item_by_name (GnomeApp *app,
				 const gchar *name)
{
	GnomeDockItem *item;

	item = gnome_dock_get_item_by_name (GNOME_DOCK (app->dock), name,
					    NULL, NULL, NULL, NULL);

	if (item == NULL && app->layout != NULL) {
		GnomeDockLayoutItem *i;

		i = gnome_dock_layout_get_item_by_name (app->layout,
							name);
		if (i == NULL)
			return NULL;

		return i->item;
	} else {
		return item;
	}
}

/**
 * gnome_app_get_dock:
 * @app: A &GnomeApp widget
 * 
 * Retrieves the &GnomeDock widget contained in the &GnomeApp.
 * 
 * Returns: The &GnomeDock widget.
 **/
GnomeDock *
gnome_app_get_dock (GnomeApp *app)
{
	return GNOME_DOCK (app->dock);
}

static void
gnome_app_set_help_view_orientation(GtkWidget *dock_item, GtkOrientation new_orientation, GnomeHelpView *help_view)
{
	/* 2000-09-01: Something wrong with new_orientation:
	   It is always 0. IH */
	gnome_help_view_set_orientation(help_view, 
					GNOME_DOCK_ITEM (dock_item)->orientation);
}

/**
 * gnome_app_add_help_view:
 * @app: A &GnomeApp widget
 * @help_view: A &GnomeHelpView widget
 *
 * Sets the &GnomeHelpView widget for a particular &GnomeApp toplevel.
 * If 'help_view' is NULL, creates a help view using reasonable default settings.
 */
void
gnome_app_set_help_view (GnomeApp *app, GtkWidget *help_view)
{
	GtkWidget *item;
	gpointer td;
	g_return_if_fail(GNOME_IS_APP(app));

	td = gtk_object_get_data((GtkObject *)app, GNOME_APP_HELP_VIEW_NAME);

	if(td)
		return;

	if(help_view == NULL)
		help_view = gnome_help_view_new(GTK_WIDGET(app),
						GNOME_HELP_POPUP,
						G_PRIORITY_LOW);

	gtk_widget_show(help_view);

	item = gnome_dock_item_new (GNOME_APP_HELP_VIEW_NAME, 
				    GNOME_DOCK_ITEM_BEH_EXCLUSIVE);
	gtk_container_add (GTK_CONTAINER (item), help_view);
	gnome_app_add_dock_item (app, GNOME_DOCK_ITEM (item),
				 GNOME_DOCK_BOTTOM, 0, 0, 0);
	gtk_signal_connect_while_alive(GTK_OBJECT(item), "orientation_changed",
				       GTK_SIGNAL_FUNC(gnome_app_set_help_view_orientation), 
				       help_view, GTK_OBJECT(help_view));
}
