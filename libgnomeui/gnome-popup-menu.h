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

#ifndef GNOME_POPUPMENU_H
#define GNOME_POPUPMENU_H

#include <libgnome/gnome-defs.h>
#include <gtk/gtkmenu.h>
#include <libgnomeui/gnome-app.h>
#include <libgnomeui/gnome-app-helper.h>


BEGIN_GNOME_DECLS


/* Creates a popup menu out of the specified uiinfo.  This should be popped up using
 * gnome_popup_menu_do_popup(), or attached to a widget using gnome_popup_menu_attach().
 */
GtkWidget *gnome_popup_menu_new (GnomeUIInfo *uiinfo);

/* Attaches the specified popup menu to the specified widget.  The menu can then be activated by
 * pressing mouse button 3 over the widget.  When a menu item callback is invoked, the specified
 * user_data will be passed to it.
 *
 * This function requires the widget to have its own window (i.e. GTK_WIDGET_NO_WINDOW (widget) ==
 * FALSE), This function will try to set the GDK_BUTTON_PRESS_MASK flag on the widget's event mask
 * if it does not have it yet; if this is the case, then the widget must not be realized for it to
 * work.
 *
 * The popup menu can be attached to different widgets at the same time.  A reference count is kept
 * on the popup menu; when all the widgets it is attached to are destroyed, the popup menu will be
 * destroyed as well.
 */
void gnome_popup_menu_attach (GtkWidget *popup, GtkWidget *widget, gpointer user_data);

/* You can use this function to pop up a menu.  When a menu item callback is invoked, the specified
 * user_data will be passed to it.
 *
 * The pos_func and pos_data parameters are the same as for gtk_menu_popup(), i.e. you can use them
 * to specify a function to position the menu explicitly.  If you want the default position (near
 * the mouse), pass NULL for these parameters.
 *
 * The event parameter is needed to figure out the mouse button that activated the menu and the time
 * at which this happened.  If you pass in NULL, then no button and the current time will be used as
 * defaults.
 */
void gnome_popup_menu_do_popup (GtkWidget *popup, GtkMenuPositionFunc pos_func, gpointer pos_data,
				GdkEventButton *event, gpointer user_data);

/* Same as above, but runs the popup menu modally and returns the index of the selected item, or -1
 * if none.
 */
int gnome_popup_menu_do_popup_modal (GtkWidget *popup, GtkMenuPositionFunc pos_func, gpointer pos_data,
				     GdkEventButton *event, gpointer user_data);


END_GNOME_DECLS

#endif

