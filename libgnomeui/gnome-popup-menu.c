/* Popup menus for GNOME
 * 
 * Copyright (C) 1998 Mark Crichton
 *
 * Authors: Mark Crichton <mcrichto@purdue.edu>
 *          Federico Mena <federico@nuclecu.unam.mx>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */

#include <config.h>
#include <gtk/gtk.h>
#include "gnome-popup-menu.h"

static GtkWidget* global_menushell_hack = NULL;
#define TOPLEVEL_MENUSHELL_KEY "gnome-popup-menu:toplevel_menushell_key"

/* This is our custom signal connection function for popup menu items -- see below for the
 * marshaller information.  We pass the original callback function as the data pointer for the
 * marshaller (uiinfo->moreinfo).
 */
static void
popup_connect_func (GnomeUIInfo *uiinfo, gchar *signal_name, GnomeUIBuilderData *uibdata)
{
	g_assert (uibdata->is_interp);

	gtk_object_set_data (GTK_OBJECT (uiinfo->widget), 
			     TOPLEVEL_MENUSHELL_KEY, 
			     global_menushell_hack);

	gtk_signal_connect_full (GTK_OBJECT (uiinfo->widget), signal_name,
				 NULL,
				 uibdata->relay_func,
				 uiinfo->moreinfo,
				 uibdata->destroy_func,
				 FALSE,
				 FALSE);
}

static GtkWidget* 
get_toplevel(GtkWidget* menuitem)
{
	return gtk_object_get_data (GTK_OBJECT (menuitem),
				    TOPLEVEL_MENUSHELL_KEY);
}

/* Our custom marshaller for menu items.  We need it so that it can extract the per-attachment
 * user_data pointer from the parent menu shell and pass it to the callback.  This overrides the
 * user-specified data from the GnomeUIInfo structures.
 */

typedef void (* ActivateFunc) (GtkObject *object, gpointer data);

static void
popup_marshal_func (GtkObject *object, gpointer data, guint n_args, GtkArg *args)
{
	ActivateFunc func;
	gpointer user_data;

	user_data = gtk_object_get_data (GTK_OBJECT (get_toplevel(GTK_WIDGET (object))),
					 "gnome_popup_menu_do_popup_user_data");

	gtk_object_set_data (GTK_OBJECT (get_toplevel(GTK_WIDGET (object))),
			     "gnome_popup_menu_active_item",
			     object);

	func = (ActivateFunc) data;
	if (func)
		(* func) (object, user_data);
}

/**
 * gnome_popup_menu_new_with_accelgroup
 * @uiinfo:
 * @accelgroup:
 *
 * Creates a popup menu out of the specified uiinfo array.  Use
 * gnome_popup_menu_do_popup() to pop the menu up, or attach it to a
 * window with gnome_popup_menu_attach().
 *
 * Returns a menu widget
 */
GtkWidget *
gnome_popup_menu_new_with_accelgroup (GnomeUIInfo *uiinfo,
				      GtkAccelGroup *accelgroup)
{
	GtkWidget *menu;
	GnomeUIBuilderData uibdata;
	gint length;

	g_return_val_if_fail (uiinfo != NULL, NULL);
	g_return_val_if_fail (accelgroup != NULL, NULL);

	/* We use our own callback marshaller so that it can fetch the
	 * popup user data from the popup menu and pass it on to the
	 * user-defined callbacks.
	 */

	uibdata.connect_func = popup_connect_func;
	uibdata.data = NULL;
	uibdata.is_interp = TRUE;
	uibdata.relay_func = popup_marshal_func;
	uibdata.destroy_func = NULL;

	for (length = 0; uiinfo[length].type != GNOME_APP_UI_ENDOFINFO; length++)
		if (uiinfo[length].type == GNOME_APP_UI_ITEM_CONFIGURABLE)
			gnome_app_ui_configure_configurable (uiinfo + length);

	menu = gtk_menu_new ();
	gtk_menu_set_accel_group (GTK_MENU (menu), accelgroup);
        global_menushell_hack = menu;
	gnome_app_fill_menu_custom (GTK_MENU_SHELL (menu), uiinfo,
				    &uibdata, accelgroup, FALSE, 0);
        global_menushell_hack = NULL;

	return menu;
}

/**
 * gnome_popup_menu_new
 * @uiinfo:
 *
 * This function behaves just like
 * gnome_popup_menu_new_with_accelgroup(), except that it creates an
 * accelgroup for you and attaches it to the menu object.  Use
 * gnome_popup_menu_get_accel_group() to get the accelgroup that is
 * created.
 *
 * Returns a menu widget
 *
 */
GtkWidget *
gnome_popup_menu_new (GnomeUIInfo *uiinfo)
{
	GtkAccelGroup *accelgroup;
	GtkWidget *menu;

	accelgroup = gtk_accel_group_new();
  
	menu = gnome_popup_menu_new_with_accelgroup(uiinfo, accelgroup);

	gtk_accel_group_unref (accelgroup);

	return menu;
}

/**
 * gnome_popup_menu_get_accel_group
 * @menu: 
 *
 * Returns the accelgroup associated with the specified GtkMenu.  This
 * is the accelgroup that was created by gnome_popup_menu_new().  If
 * you want to specify the accelgroup that the popup menu accelerators
 * use, then use gnome_popup_menu_new_with_accelgroup().
 */
GtkAccelGroup *
gnome_popup_menu_get_accel_group(GtkMenu *menu)
{
	g_return_val_if_fail (menu != NULL, NULL);
        g_return_val_if_fail (GTK_IS_MENU (menu), NULL);

#if GTK_CHECK_VERSION(1,2,1)
        return gtk_menu_get_accel_group (menu);
#else
	return NULL;
#endif
}

/* Callback used when a button is pressed in a widget attached to a popup menu.  It decides whether
 * the menu should be popped up and does the appropriate stuff.
 */
static gint
popup_button_pressed (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GtkWidget *popup;
	gpointer user_data;

	if (event->button != 3)
		return FALSE;

	popup = data;

	/* Fetch the attachment user data from the widget and install it in the popup menu -- it
	 * will be used by the callback mashaller to pass the data to the callbacks.
	 */

	user_data = gtk_object_get_data (GTK_OBJECT (widget), "gnome_popup_menu_attach_user_data");

	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "button_press_event");

	gnome_popup_menu_do_popup (popup, NULL, NULL, event, user_data);
	return TRUE;
}

/* Callback used to unref the popup menu when the widget it is attached to gets destroyed */
static void
popup_attach_widget_destroyed (GtkWidget *widget, gpointer data)
{
	GtkWidget *popup;

	popup = data;

	gtk_object_unref (GTK_OBJECT (popup));
}

/**
 * gnome_popup_menu_attach:
 * @popup:
 * @widget:
 * @user_data:
 *
 * Attaches the specified popup menu to the specified widget.  The
 * menu can then be activated by pressing mouse button 3 over the
 * widget.  When a menu item callback is invoked, the specified
 * user_data will be passed to it.
 *
 * This function requires the widget to have its own window
 * (i.e. GTK_WIDGET_NO_WINDOW (widget) == FALSE), This function will
 * try to set the GDK_BUTTON_PRESS_MASK flag on the widget's event
 * mask if it does not have it yet; if this is the case, then the
 * widget must not be realized for it to work.
 *
 * The popup menu can be attached to different widgets at the same
 * time.  A reference count is kept on the popup menu; when all the
 * widgets it is attached to are destroyed, the popup menu will be
 * destroyed as well.
 */
void
gnome_popup_menu_attach (GtkWidget *popup, GtkWidget *widget,
			 gpointer user_data)
{
	g_return_if_fail (popup != NULL);
	g_return_if_fail (GTK_IS_MENU (popup));
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

	/* Ref/sink the popup menu so that we take "ownership" of it */

	gtk_object_ref (GTK_OBJECT (popup));
	gtk_object_sink (GTK_OBJECT (popup));

	/* Store the user data pointer in the widget -- we will use it later when the menu has to be
	 * invoked.
	 */

	gtk_object_set_data (GTK_OBJECT (widget), "gnome_popup_menu_attach_user_data", user_data);

	/* Prepare the widget to accept button presses -- the proper assertions will be
	 * shouted by gtk_widget_set_events().
	 */

	gtk_widget_add_events (widget, GDK_BUTTON_PRESS_MASK);

	gtk_signal_connect (GTK_OBJECT (widget), "button_press_event",
			    (GtkSignalFunc) popup_button_pressed,
			    popup);

	/* This callback will unref the popup menu when the widget it is attached to gets destroyed. */

	gtk_signal_connect (GTK_OBJECT (widget), "destroy",
			    (GtkSignalFunc) popup_attach_widget_destroyed,
			    popup);
}

/**
 * gnome_popup_menu_do_popup:
 * @popup:
 * @pos_func:
 * @pos_data:
 * @event:
 * @user_data:
 *
 *
 * You can use this function to pop up a menu.  When a menu item *
 * callback is invoked, the specified user_data will be passed to it.
 *
 * The pos_func and pos_data parameters are the same as for
 * gtk_menu_popup(), i.e. you can use them to specify a function to
 * position the menu explicitly.  If you want the default position
 * (near the mouse), pass NULL for these parameters.
 *
 * The event parameter is needed to figure out the mouse button that
 * activated the menu and the time at which this happened.  If you
 * pass in NULL, then no button and the current time will be used as
 * defaults.
 */
void
gnome_popup_menu_do_popup (GtkWidget *popup, GtkMenuPositionFunc pos_func, gpointer pos_data,
			   GdkEventButton *event, gpointer user_data)
{
	guint button;
	guint32 timestamp;

	g_return_if_fail (popup != NULL);
	g_return_if_fail (GTK_IS_WIDGET (popup));

	/* Store the user data in the menu for when a callback is activated -- if it is a
	 * Gnome-generated menu, then the user data will be passed on to callbacks by our custom
	 * marshaller.
	 */

	gtk_object_set_data (GTK_OBJECT (popup), "gnome_popup_menu_do_popup_user_data", user_data);

	if (event) {
		button = event->button;
		timestamp = event->time;
	} else {
		button = 0;
		timestamp = GDK_CURRENT_TIME;
	}

	gtk_menu_popup (GTK_MENU (popup), NULL, NULL, pos_func, pos_data, button, timestamp);
}

/* Convenience callback to exit the main loop of a modal popup menu when it is deactivated*/
static void
menu_shell_deactivated (GtkMenuShell *menu_shell, gpointer data)
{
	gtk_main_quit ();
}

/* Returns the index of the active item in the specified menu, or -1 if none is active */
static int
get_active_index (GtkMenu *menu)
{
	GList *l;
	GtkWidget *active;
	int i;

	active = gtk_object_get_data (GTK_OBJECT (menu), "gnome_popup_menu_active_item");

	for (i = 0, l = GTK_MENU_SHELL (menu)->children; l; l = l->next, i++)
		if (active == l->data)
			return i;

	return -1;
}

/**
 * gnome_popup_menu_do_popup_modal:
 * @popup:
 * @pos_func:
 * @pos_data:
 * @event:
 * @user_data:
 * 
 * Same as above, but runs the popup menu modally and returns the
 * index of the selected item, or -1 if none.
 */
int
gnome_popup_menu_do_popup_modal (GtkWidget *popup, GtkMenuPositionFunc pos_func, gpointer pos_data,
				 GdkEventButton *event, gpointer user_data)
{
	guint id;
	guint button;
	guint32 timestamp;

	g_return_val_if_fail (popup != NULL, -1);
	g_return_val_if_fail (GTK_IS_WIDGET (popup), -1);

	gtk_object_set_data (GTK_OBJECT (popup), "gnome_popup_menu_do_popup_user_data", user_data);

	/* Connect to the deactivation signal to be able to quit our modal main loop */

	id = gtk_signal_connect (GTK_OBJECT (popup), "deactivate",
				 (GtkSignalFunc) menu_shell_deactivated,
				 NULL);

	gtk_object_set_data (GTK_OBJECT (popup), "gnome_popup_menu_active_item", NULL);

	if (event) {
		button = event->button;
		timestamp = event->time;
	} else {
		button = 0;
		timestamp = GDK_CURRENT_TIME;
	}

	gtk_menu_popup (GTK_MENU (popup), NULL, NULL, pos_func, pos_data, button, timestamp);
	gtk_grab_add (popup);
	gtk_main ();
	gtk_grab_remove (popup);

	gtk_signal_disconnect (GTK_OBJECT (popup), id);

	return get_active_index (GTK_MENU (popup));
}
