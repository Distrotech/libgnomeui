/* Popup menus for GNOME
 * 
 * Copyright (C) 1998 Mark Crichton
 * All rights reserved.
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
/*
  @NOTATION@
*/

#include <config.h>
#include <gtk/gtk.h>
#include "gnome-popup-menu.h"

static GtkWidget* global_menushell_hack = NULL;
#define TOPLEVEL_MENUSHELL_KEY "gnome-popup-menu:toplevel_menushell_key"

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

typedef void (* ActivateFunc) (GtkObject *object, gpointer data, GtkWidget *for_widget);

static void
popup_marshal_func (GtkObject *object, gpointer data, guint n_args, GtkArg *args)
{
	ActivateFunc func;
	gpointer user_data;
	GtkObject *tl;
	GtkWidget *for_widget;

	tl = GTK_OBJECT (get_toplevel(GTK_WIDGET (object)));
	user_data = gtk_object_get_data (tl, "gnome_popup_menu_do_popup_user_data");
	for_widget = gtk_object_get_data (tl, "gnome_popup_menu_do_popup_for_widget");

	gtk_object_set_data (GTK_OBJECT (get_toplevel(GTK_WIDGET (object))),
			     "gnome_popup_menu_active_item",
			     object);

	func = (ActivateFunc) data;
	if (func)
		(* func) (object, user_data, for_widget);
}

/* This is our custom signal connection function for popup menu items -- see below for the
 * marshaller information.  We pass the original callback function as the data pointer for the
 * marshaller (uiinfo->moreinfo).
 */
static void
popup_connect_func (GnomeUIInfo *uiinfo, const char *signal_name, GnomeUIBuilderData *uibdata)
{
	g_assert (uibdata->is_interp);

	gtk_object_set_data (GTK_OBJECT (uiinfo->widget), 
			     TOPLEVEL_MENUSHELL_KEY, 
			     global_menushell_hack);

	gtk_signal_connect (GTK_OBJECT (uiinfo->widget),
			    signal_name,
			    GTK_SIGNAL_FUNC (popup_marshal_func),
			    uiinfo->moreinfo);
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
	GtkAccelGroup *my_accelgroup;

	/* We use our own callback marshaller so that it can fetch the
	 * popup user data from the popup menu and pass it on to the
	 * user-defined callbacks.
	 */

	menu = gtk_menu_new ();

	if(accelgroup)
	  my_accelgroup = accelgroup;
	else
	  my_accelgroup = gtk_accel_group_new();
	gtk_menu_set_accel_group (GTK_MENU (menu), my_accelgroup);
	if(!accelgroup)
	  g_object_unref (G_OBJECT (my_accelgroup));

	gnome_popup_menu_append (menu, uiinfo);

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
  return gnome_popup_menu_new_with_accelgroup(uiinfo, NULL);
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
real_popup_button_pressed (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GtkWidget *popup;
	gpointer user_data;

	popup = data;

	/*
	 * Fetch the attachment user data from the widget and install
	 * it in the popup menu -- it will be used by the callback
	 * mashaller to pass the data to the callbacks.
	 *
	 */

	user_data = gtk_object_get_data (GTK_OBJECT (widget), "gnome_popup_menu_attach_user_data");

	gnome_popup_menu_do_popup (popup, NULL, NULL, event, user_data, widget);

	return TRUE;
}

static gint
popup_button_pressed (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	if (event->button != 3)
		return FALSE;

	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "button_press_event");

	return real_popup_button_pressed (widget, event, data);
}

static gint
relay_popup_button_pressed (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
        GtkWidget *new_widget = NULL;

	if (event->button != 3)
		return FALSE;

	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "button_press_event");

	if (GTK_IS_CONTAINER(widget))
	  {

	    do {
	      GList *children;

	      for (children = gtk_container_children(GTK_CONTAINER(widget)), new_widget = NULL;
		   children; children = children->next)
		{
		  GtkWidget *cur;
		  
		  cur = (GtkWidget *)children->data;
		  
		  if(! GTK_WIDGET_NO_WINDOW(cur) )
		    continue;
		  
		  if(cur->allocation.x <= event->x
		     && cur->allocation.y <= event->y
		     && (cur->allocation.x + cur->allocation.width) > event->x
		     && (cur->allocation.y + cur->allocation.height) > event->y
		     && gtk_object_get_data (GTK_OBJECT(cur), "gnome_popup_menu_nowindow"))
		    {
		      new_widget = cur;
		      break;
		    }
		}
	      if(new_widget)
		widget = new_widget;
	      else
		break;
	    } while(widget && GTK_IS_CONTAINER (widget) && GTK_WIDGET_NO_WINDOW(widget));

	    if(!widget || !gtk_object_get_data (GTK_OBJECT(widget), "gnome_popup_menu"))
	      {
		return TRUE;
	      }
	  }
	else
	  new_widget = widget;

	return real_popup_button_pressed (new_widget, event, data);
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
 *
 * Under the current implementation, setting a popup menu for a NO_WINDOW widget and then reparenting
 * that widget will cause Bad Things to happen.
 */
void
gnome_popup_menu_attach (GtkWidget *popup, GtkWidget *widget,
			 gpointer user_data)
{
        GtkWidget *ev_widget;

	g_return_if_fail (popup != NULL);
	g_return_if_fail (GTK_IS_MENU (popup));
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

	if(gtk_object_get_data (GTK_OBJECT (widget), "gnome_popup_menu"))
	  return;

	gtk_object_set_data (GTK_OBJECT (widget), "gnome_popup_menu", popup);

	/* This operation can fail if someone is trying to set a popup on e.g. an uncontained label, so we do it first. */
	for(ev_widget = widget; ev_widget && GTK_WIDGET_NO_WINDOW(ev_widget); ev_widget = ev_widget->parent)
	  {
	    gtk_object_set_data (GTK_OBJECT (ev_widget), "gnome_popup_menu_nowindow", GUINT_TO_POINTER(1));
	  }

	g_return_if_fail (ev_widget);

	/* Ref/sink the popup menu so that we take "ownership" of it */

	gtk_object_ref (GTK_OBJECT (popup));
	gtk_object_sink (GTK_OBJECT (popup));

	/* Store the user data pointer in the widget -- we will use it later when the menu has to be
	 * invoked.
	 */

	gtk_object_set_data (GTK_OBJECT (widget), "gnome_popup_menu_attach_user_data", user_data);
	gtk_object_set_data (GTK_OBJECT (widget), "gnome_popup_menu", user_data);

	/* Prepare the widget to accept button presses -- the proper assertions will be
	 * shouted by gtk_widget_set_events().
	 */

	gtk_widget_add_events (ev_widget, GDK_BUTTON_PRESS_MASK);

	gtk_signal_connect (GTK_OBJECT (widget), "button_press_event",
			    GTK_SIGNAL_FUNC (popup_button_pressed), popup);

	if (ev_widget != widget)
	  gtk_signal_connect_while_alive (GTK_OBJECT (ev_widget), "button_press_event",
					  (GtkSignalFunc) relay_popup_button_pressed,
					  popup, GTK_OBJECT(widget));

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
			   GdkEventButton *event, gpointer user_data, GtkWidget *for_widget)
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
	gtk_object_set_data (GTK_OBJECT (popup), "gnome_popup_menu_do_popup_for_widget", for_widget);

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
				 GdkEventButton *event, gpointer user_data, GtkWidget *for_widget)
{
	guint id;
	guint button;
	guint32 timestamp;

	g_return_val_if_fail (popup != NULL, -1);
	g_return_val_if_fail (GTK_IS_WIDGET (popup), -1);

	/* Connect to the deactivation signal to be able to quit our modal main loop */

	id = gtk_signal_connect (GTK_OBJECT (popup), "deactivate",
				 (GtkSignalFunc) menu_shell_deactivated,
				 NULL);

	gtk_object_set_data (GTK_OBJECT (popup), "gnome_popup_menu_active_item", NULL);
	gtk_object_set_data (GTK_OBJECT (popup), "gnome_popup_menu_do_popup_user_data", user_data);
	gtk_object_set_data (GTK_OBJECT (popup), "gnome_popup_menu_do_popup_for_widget", for_widget);

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

void
gnome_popup_menu_append (GtkWidget *popup, GnomeUIInfo *uiinfo)
{
	GnomeUIBuilderData uibdata;
	gint length;

	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = popup_connect_func;
	uibdata.data = NULL;
	uibdata.is_interp = TRUE;
	uibdata.relay_func = NULL;
	uibdata.destroy_func = NULL;

	for (length = 0; uiinfo[length].type != GNOME_APP_UI_ENDOFINFO; length++)
		if (uiinfo[length].type == GNOME_APP_UI_ITEM_CONFIGURABLE)
			gnome_app_ui_configure_configurable (uiinfo + length);

        global_menushell_hack = popup;
	gnome_app_fill_menu_custom (GTK_MENU_SHELL (popup), uiinfo,
				    &uibdata, gtk_menu_get_accel_group(GTK_MENU(popup)), FALSE, 0);
        global_menushell_hack = NULL;
}


/**
 * gnome_gtk_widget_add_popup_items:
 * @widget: The widget to append popup menu items for
 * @uiinfo: The array holding information on the menu items
 * @user_data: The user_data to pass to the callbacks for the menu items
 *
 * This creates a new popup menu for the widget if none exists, and
 * then adds the items in 'uiinfo' to the widget's popup menu.
 */
void
gnome_gtk_widget_add_popup_items (GtkWidget *widget, GnomeUIInfo *uiinfo, gpointer user_data)
{
  GtkWidget *prev_popup;

  prev_popup = gtk_object_get_data (GTK_OBJECT (widget), "gnome_popup_menu");

  if(!prev_popup)
    {
      prev_popup = gnome_popup_menu_new(uiinfo);
      gnome_popup_menu_attach(prev_popup, widget, user_data);
    }
  else
    gnome_popup_menu_append(prev_popup, uiinfo);
}
