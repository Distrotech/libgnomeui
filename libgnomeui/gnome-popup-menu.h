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

#ifndef GNOME_POPUPMENU_H
#define GNOME_POPUPMENU_H

#include <libgnome/gnome-defs.h>
#include <gtk/gtkmenu.h>
#include <libgnomeui/gnome-app.h>
#include <libgnomeui/gnome-app-helper.h>


BEGIN_GNOME_DECLS


/* These routines are documented in gnome-popup-menu.c */

GtkWidget *gnome_popup_menu_new (GnomeUIInfo *uiinfo);
GtkWidget *gnome_popup_menu_new_with_accelgroup (GnomeUIInfo *uiinfo,
						 GtkAccelGroup *accelgroup);
GtkAccelGroup *gnome_popup_menu_get_accel_group(GtkMenu *menu);


void gnome_popup_menu_attach (GtkWidget *popup, GtkWidget *widget,
			      gpointer user_data);

void gnome_popup_menu_do_popup (GtkWidget *popup, GtkMenuPositionFunc pos_func, gpointer pos_data,
				GdkEventButton *event, gpointer user_data);

int gnome_popup_menu_do_popup_modal (GtkWidget *popup, GtkMenuPositionFunc pos_func, gpointer pos_data,
				     GdkEventButton *event, gpointer user_data);


END_GNOME_DECLS

#endif

