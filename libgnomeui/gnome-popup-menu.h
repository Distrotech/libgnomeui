/* My vague and ugly attempt at adding popup menus to GnomeUI */
/* By: Mark Crichton <mcrichto@purdue.edu> */
/* Written under the heavy infulence of whatever was playing
 * in my CDROM drive at the time... */
/* First pass: July 8, 1998 */
/* gnome-popup-menu.h
 * 
 * Copyright (C) Mark Crichton
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


#ifndef __GNOME_POPUPMENU_H__
#define __GNOME_POPUPMENU_H__

#include <gtk/gtk.h>
#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

void 
gnome_app_create_popup_menus(GnomeApp * app, 
			     GtkWidget * child, 
			     GnomeUIInfo * uiinfo, 
			     gpointer handler);

void
gnome_app_create_popup_menus_custom(GnomeApp * app,
				    GtkWidget * child,
				    GnomeUIInfo * uiinfo,
       				    gpointer handler,
			            GnomeUIBuilderData * xinfo);

END_GNOME_DECLS

#endif

