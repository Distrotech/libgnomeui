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

#define gnome_widget_add_help(widget, help) \
	(gnome_widget_add_help_with_uidata((widget),(help),NULL, NULL))
void gnome_widget_add_help_with_uidata  (GtkWidget *widget,
					 const gchar *help,
					 GnomeUIInfo *menuinfo,
					 gpointer user_data);



END_GNOME_DECLS

#endif

