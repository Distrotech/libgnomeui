/* Popup menus for GNOME
 * 
 * Copyright (C) 1998 Jonathan Blandford
 *
 * Authors: Jonathan Blandford <jrb@mit.edu>
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

#ifndef GNOME_POPUP_HELP_H
#define GNOME_POPUP_HELP_H

#include <libgnome/gnome-defs.h>
#include <libgnomeui/gnome-app.h>
#include <libgnomeui/gnome-app-helper.h>


BEGIN_GNOME_DECLS


/* This creates a popup menu on the specified menu with one entry.
 * The menu, invoked by pressing button three on the widget, will have
 * one entry entitled: "Help with this."  Selecting this entry
 * will bring up a tooltip with the help variable as its text.  In
 * addition, if the widget is a GtkEntry derivative or a GtkText
 * derivative, it will add cut/copy/paste to the list.  If help is
 * NULL, then it will _just_ add the cut copy paste.  Finally, if
 * menuinfo is non NULL, it will append the menu defined by it on the
 * end of the popup menu.  This function should be called everywhere.
 * If you would actually like a handle to the popup menu, call
 * gnome_popup_menu_get as normal. */
#define gnome_widget_add_help(widget, help) \
	(gnome_widget_add_help_with_uidata((widget),(help),NULL, NULL))
void gnome_widget_add_help_with_uidata  (GtkWidget *widget,
					 gchar *help,
					 GnomeUIInfo *menuinfo,
					 gpointer user_data);



END_GNOME_DECLS

#endif

