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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>
#include "gnome-popup-menu.h"


/* This is our custom signal connection function for popup menu items -- see below for the
 * marshaller information.  We pass the original callback function as the data pointer for the
 * marshaller (uiinfo->moreinfo).
 */
static void
popup_connect_func (GnomeUIInfo *uiinfo, gchar *signal_name, GnomeUIBuilderData *uibdata)
{
	g_assert (uibdata->is_interp);

	gtk_signal_connect_full (GTK_OBJECT (uiinfo->widget), signal_name,
				 NULL,
				 uibdata->relay_func,
				 uiinfo->moreinfo,
				 uibdata->destroy_func,
				 FALSE,
				 FALSE);
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

	func = (ActivateFunc) data;
	user_data = gtk_object_get_data (GTK_OBJECT (GTK_WIDGET (object)->parent), "gnome_popup_menu_do_popup_user_data");

	(* func) (object, user_data);
}

GtkWidget *
gnome_popup_menu_new (GnomeUIInfo *uiinfo)
{
	GtkWidget *menu;
	GnomeUIBuilderData uibdata;

	g_return_val_if_fail (uiinfo != NULL, NULL);

	/* We use our own callback marshaller so that it can fetch the popup user data
	 * from the popup menu and pass it on to the user-defined callbacks.
	 */

	uibdata.connect_func = popup_connect_func;
	uibdata.data = NULL;
	uibdata.is_interp = TRUE;
	uibdata.relay_func = popup_marshal_func;
	uibdata.destroy_func = NULL;

	menu = gtk_menu_new ();
	gnome_app_fill_menu_custom (GTK_MENU_SHELL (menu), uiinfo, &uibdata, NULL, FALSE, TRUE, 0);

	return menu;
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

void
gnome_popup_menu_attach (GtkWidget *popup, GtkWidget *widget, gpointer user_data)
{
	int event_mask;

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

void
gnome_popup_menu_do_popup (GtkWidget *popup, GtkMenuPositionFunc pos_func, gpointer pos_data,
			   GdkEventButton *event, gpointer user_data)
{
	g_return_if_fail (popup != NULL);
	g_return_if_fail (GTK_IS_WIDGET (popup));
	g_return_if_fail (event != NULL);

	/* Store the user data in the menu for when a callback is activated -- if it is a
	 * Gnome-generated menu, then the user data will be passed on to callbacks by our custom
	 * marshaller.
	 */

	gtk_object_set_data (GTK_OBJECT (popup), "gnome_popup_menu_do_popup_user_data", user_data);

	gtk_menu_popup (GTK_MENU (popup), NULL, NULL, pos_func, pos_data, event->button, event->time);
}
