/* gnome-appsmenu.h: Copyright (C) 1998 Free Software Foundation
 * Written by: Havoc Pennington
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
   imagine this will be rooted in .gnome/AppsMenu or somewhere.

   Directories have .directory files in them; these are handled by
   default. Other such file types can be registered, and anything
   starting with . will be taken to be one.  Only one directory
   dotfile will be used per directory, the first one found with a
   known name. So really there should only be one.

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
   icewm files will be ignored.  */

#include <gtk/gtk.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-dentry.h"

BEGIN_GNOME_DECLS

/* fixme, this should be (or even is?) defined elsewhere,
   like gnome-dentry.h, and renamed appropriately */
#define GNOME_APPS_MENU_DENTRY_EXTENSION "desktop"

typedef struct _GnomeAppsMenu GnomeAppsMenu;

struct _GnomeAppsMenu {
  guint is_directory : 1; /* Whether it's a directory. If it is, the
			     extension will really be the whole file
			     name (except the period)
			     (e.g. ".directory" for filename,
			     "directory" extension) */
  gchar * extension; /* The file extension, without the period.
			This implies the type of the data field. */
  gpointer data;     /* Data loaded from the file */
  GList * submenus;  /* NULL if there are none. If is_directory is 
			FALSE, this must be NULL */
  GnomeAppsMenu * parent;
  guint in_destroy : 1;
};

#define GNOME_APPS_MENU_IS_DIR(x) \
(((GnomeAppsMenu *)x)->is_directory)

GnomeAppsMenu * gnome_apps_menu_new(gboolean is_directory,
				    const gchar * extension,
				    gpointer data);

/* Recursively destroy the menu, calling g_free on 
   ALL non-null fields; the AppsMenu owns anything you 
   put in it. */
void gnome_apps_menu_destroy(GnomeAppsMenu *); 

/* Manipulate apps menu directories - it's basically going to 
   be an error to call these on non-directories */
void gnome_apps_menu_append(GnomeAppsMenu * dir,
			    GnomeAppsMenu * sub);
void gnome_apps_menu_prepend(GnomeAppsMenu * dir,
			     GnomeAppsMenu * sub);
/* Removing does not destroy! */
void gnome_apps_menu_remove(GnomeAppsMenu * dir,
			    GnomeAppsMenu * sub);
void gnome_apps_menu_insert_after(GnomeAppsMenu * dir,
				  GnomeAppsMenu * sub,
				  GnomeAppsMenu * after_me);
/* Recursive foreach. For non-recursive, just use the GList */
void gnome_apps_menu_foreach(GnomeAppsMenu * dir,
                             GFunc func, gpointer data);

/* The load func takes a filename as argument and returns 
   whatever should go in the data field, above.
   Should return NULL if it fails. */

typedef gpointer (*GnomeAppsMenuLoadFunc)(const gchar *);

/* The menuitem-creating function can be NULL, if you don't
   want to create menuitems in gtk_menu_new_from_apps_menu()
   This function should create the menu item and attach any
   callbacks. It should NOT be recursive, i.e. just create 
   the item for this GnomeAppsMenu, don't worry about the 
   submenus. */

typedef GtkWidget * (*GnomeAppsMenuGtkMenuItemFunc)(GnomeAppsMenu *);
       
/* The desktop entry variety is already registered by default,
   with gnome_desktop_entry_load () as LoadFunc,
   and GNOME_APPS_MENU_DENTRY_EXTENSION as extension,
   and a private function to create menuitems. 

   .directory files are also registered by default. */

void 
gnome_apps_menu_register_variety( gboolean is_directory,
                                  const gchar * extension,
                                  GnomeAppsMenuLoadFunc load_func,
                                  GnomeAppsMenuGtkMenuItemFunc menu_item_func );

/* Load a GnomeAppsMenu, with `directory' as root.  */

GnomeAppsMenu * gnome_apps_menu_load(const gchar * directory);

/* Load the user's default apps menu.  (window managers, panel,
   etc. should use the default for the main "start" menu) */

GnomeAppsMenu * gnome_apps_menu_load_default(void);

/* Load the systemwide menu, e.g. share/apps */

GnomeAppsMenu * gnome_apps_menu_load_system(void);

/* Create a GtkMenu or MenuItem from a GnomeAppsMenu, complete with
   callbacks. Creating an item recursively creates any submenus.
   Creating a menu is the same as iterating over gam->submenus and
   appending the result of gtk_menu_item_new_from_apps_menu() for each
   submenu to the menu. i.e. it's the same as creating an item, except
   that there's no item created for the root AppsMenu.  Confusing -
   just look at the code. :) */

GtkWidget * gtk_menu_new_from_apps_menu(GnomeAppsMenu * gam);
GtkWidget * gtk_menu_item_new_from_apps_menu(GnomeAppsMenu * gam);

/* Functions to save GnomeAppsMenu to be added. */

END_GNOME_DECLS
   
#endif
