/* gnome-appsmenu.h: Copyright (C) 1998 Havoc Pennington
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

#ifndef GNOME_APPSMENU_H
#define GNOME_APPSMENU_H

/******* Rough draft, and no implementation anyway. don't use...  I
  just want to try this scheme. Will implement it as soon as I get a
  chance... comments welcome as always. 
  It's almost what the panel does now, so that code should be 
  recyclable. */

/* 
   This file defines a standard API for loading, saving, and
   displaying "start menu"-type applications menus.

   Menus are saved as directories containing .desktop entries. Each
   directory is a submenu. This is just like share/apps is now. I
   imagine this will be rooted in .gnome/AppsMenu or somewhere. Each
   directory also contains a desktop entry for itself, in case users
   want to customize folder icons or whatever.

   Applications can register their own types of apps menu items by
   providing an extension other than .desktop, and a function to load
   their type. Once a menu item type is registered, subsequent calls
   to gnome_apps_menu_load() will load entries of that type.

   Entries with unknown extensions are ignored, so all apps
   should be able to coexist just fine. Menus will always
   have only the entries relevant to the app being used.

   The extension should involve the name of the app,
   to prevent collisions.  

   So, for example, icewm might register a type ending in
   ".icewm-desktop", and a function to load these files.  The files
   might store special icewm info, like an "Exit icewm" menu item.
   When icewm calls gnome_apps_menu_load it will get a menu
   containing these items; when another app calls it, the
   icewm files will be ignored.
*/

#include <gtk/gtk.h>
#include "libgnome/gnome-defs.h"

BEGIN_GNOME_DECLS

/* fixme, this should be (or even is?) defined elsewhere,
   like gnome-dentry.h, and renamed appropriately */
#define GNOME_APPS_MENU_DENTRY_EXTENSION "desktop"

/* By default:
   extension = GNOME_APPS_MENU_DENTRY_EXTENSION
   data is a GnomeDesktopEntry
   */

typedef struct {
  gchar * extension; /* The file extension, without the period.
			This implies the type of the data field. */
  gpointer data;     /* Data loaded from the file */
  GList * submenus;  /* NULL if there are none */
} GnomeAppsMenu;

#define GNOME_APPS_MENU_IS_DENTRY(x) \
(strcmp ( (gchar *)(((GnomeAppsMenu *)x)->extension), \
	  GNOME_APPS_MENU_DENTRY_EXTENSION ) == 0)

/* The load func takes a filename as argument and returns 
   whatever should go in the data field, above */

typedef gpointer (GnomeAppsMenuLoadFunc *)(gchar *);

/* The menuitem-creating function can be NULL, if you don't
   want to create menuitems in gtk_menu_new_from_apps_menu()
   This function should create the menu item and attach any
   callbacks. */

typedef GtkWidget * (GnomeAppsMenuGtkMenuItemFunc *)(GnomeAppsMenu *);

typedef struct {
  gchar * extension;
  GnomeAppsMenuLoadFunc load_func;
  GnomeAppsMenuGtkMenuItemFunc menu_item_func;
} GnomeAppsMenuVariety;
       
/* The desktop entry variety is already registered by default,
   with gnome_desktop_entry_load () as LoadFunc,
   and GNOME_APPS_MENU_DENTRY_EXTENSION as extension,
   and a private function to create menuitems. */

void gnome_apps_menu_register_variety(GnomeAppsMenuVariety * v);

/* Load a GnomeAppsMenu, with `directory' as root.  If directory ==
   NULL load the user's default apps menu.  (window managers, panel,
   etc. should use the default for the main "start" menu) */

GnomeAppsMenu * gnome_apps_menu_load(gchar * directory);

/* Create a GtkMenu from a GnomeAppsMenu, complete with
   callbacks */

GtkWidget * gtk_menu_new_from_apps_menu(GnomeAppsMenu * gam);

/* Functions to change and save GnomeAppsMenu to be added. */

END_GNOME_DECLS
   
#endif
