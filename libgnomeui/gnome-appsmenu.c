/* gnome-appsmenu.c: Copyright (C) 1998 Havoc Pennington
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

#include "gnome-appsmenu.h"
#include <string.h>

/* Use a dotfile, so people won't try to edit manually
   unless they really know what they're doing. */
/* Alternatively, use "apps" to be consistent with share/apps? */

#define GNOME_APPS_MENU_DEFAULT_DIR ".Gnome Apps Menu"

typedef struct {
  gchar * extension;
  GnomeAppsMenuLoadFunc load_func;
  GnomeAppsMenuGtkMenuItemFunc menu_item_func;
} GnomeAppsMenuVariety;

static GList * varieties = NULL; /* list of above struct */

static GnomeAppsMenuVariety * 
gnome_apps_menu_variety_new( const gchar * extension,
			     GnomeAppsMenuLoadFunc load_func,
			     GnomeAppsMenuGtkMenuItemFunc menu_item_func )
{
  GnomeAppsMenuVariety * v;

  g_return_val_if_fail(extension != NULL, NULL);
  g_return_val_if_fail(load_func != NULL, NULL);
  /* OK to have no menu_item_func */
  
  v = g_new(GnomeAppsMenuVariety, 1);
  
  v->extension = g_strdup(extension);
  v->load_func = load_func;
  v->menu_item_func = menu_item_func;

  return v;
}


void 
gnome_apps_menu_register_variety( const gchar * extension,
				  GnomeAppsMenuLoadFunc load_func,
				  GnomeAppsMenuGtkMenuItemFunc menu_item_func )
{

  GnomeAppsMenuVariety * v;

  v = gnome_apps_menu_variety_new(extension, load_func, 
				  menu_item_func);

  if ( v == NULL ) {
    g_warning("AppsMenuVariety registration failed");
    return;
  }

  g_list_append(varieties, v);
}


static void 
gnome_apps_menu_variety_destroy( GnomeAppsMenuVariety * v )
{
  g_return_if_fail ( v != NULL );
  
  g_free(v->extension);
  g_free(v);
}

static void clear_varieties(void)
{
  GnomeAppsMenuVariety * v;
  GList * first = varieties;

  while ( varieties ) {
    v = (GnomeAppsMenuVariety *)varieties->data;
    gnome_apps_menu_variety_destroy(v);
    varieties = g_list_next(varieties);
  }
  g_list_free(first);
  varieties = NULL;
}

static void make_default_varieties(void)
{
  gnome_apps_menu_register_variety( GNOME_APPS_MENU_DENTRY_EXTENSION,
				    gnome_desktop_entry_load,
				    NULL /* FIXME */ );
}

static GnomeAppsMenuLoadFunc get_func(const gchar * filename,
				      /* load_func if TRUE,
					 menu_item_func if FALSE */
				      gboolean load_func)
{
  GList * list;
  GnomeAppsMenuVariety * v;

  list = varieties;

  while ( list ) {
    v = (GnomeAppsMenuVariety *)list->data;
    g_assert ( v != NULL );

    if ( strstr(filename, 
		v->extension) ) {
      if (load_func) {
	return v->load_func;
      }
      else {
	return v->menu_item_func;
      }
    }
    
    list = g_list_next(list);
  }

  return NULL; /* didn't find an appropriate extension */
}

/* FIXME these two will be based on the panel/menu.c code */

GnomeAppsMenu * gnome_apps_menu_load(const gchar * directory)
{
  GnomeAppsMenuLoadFunc load_func;
  g_warning("Not implemented\n");
}


GnomeAppsMenu * gnome_apps_menu_load_default(void)
{
  gchar * s;

  s = gnome_util_home_file(GNOME_APPS_MENU_DEFAULT_DIR);
 
  gnome_apps_menu_load(s);

  g_free(s);
}



GnomeAppsMenu * gnome_apps_menu_load_system(void);
{
  gchar * s;

  s = gnome_datadir_file("apps");
 
  gnome_apps_menu_load(s);

  g_free(s);
}

GtkWidget * gtk_menu_new_from_apps_menu(GnomeAppsMenu * gam)
{
  GnomeAppsMenuGtkMenuItemFunc menu_item_func;
  g_warning("Not implemented\n");
}

/****************************************************
  Remaining functions are for manipulating the apps_menu
  trees. Don't relate to config files. 
  *******************************************/

GnomeAppsMenu * gnome_apps_menu_new(void)
{
  GnomeAppsMenu * gam = g_new(GnomeAppsMenu, 1);

  gam->extension = NULL;
  gam->data = NULL;
  gam->submenus = NULL;
  gam->in_destroy = FALSE;
}

/* GFunc for the g_list_foreach() call  */
static void gnome_apps_menu_destroy_one ( gpointer menu, 
					  gpointer data )
{
  g_return_if_fail ( menu != NULL );
  gnome_apps_menu_destroy((GnomeAppsMenu *)menu);
}

void gnome_apps_menu_destroy(GnomeAppsMenu * gam) 
{
  g_return_if_fail ( gam != NULL );

  gam->in_destroy = TRUE;

  if ( gam->parent ) {
    gnome_apps_menu_remove(gam->parent, gam);
  }

  g_free ( gam->extension ); gam->extension = NULL;
  g_free ( gam->data ); gam->extension = NULL;

  if ( gam->submenus ) {
    g_list_foreach ( gam->submenus,
		     gnome_apps_menu_destroy_one, NULL );

    g_list_free ( gam->submenus );
    gam->submenus = NULL;
  }
  
  g_free (gam);
}

void gnome_apps_menu_append(GnomeAppsMenu * dir,
			    GnomeAppsMenu * sub)
{
  g_return_if_fail ( dir != NULL );
  g_return_if_fail ( sub != NULL );
  g_return_if_fail ( GNOME_APPS_MENU_IS_DIR(dir) );
  g_return_if_fail ( sub->parent == NULL );
  g_return_if_fail ( ! dir->in_destroy );
  g_return_if_fail ( ! sub->in_destroy );
  
  sub->parent = dir;

  dir->submenus = 
    g_list_append( dir->submenus,
		   sub );
}


void gnome_apps_menu_prepend(GnomeAppsMenu * dir,
			     GnomeAppsMenu * sub)
{
  g_return_if_fail ( dir != NULL );
  g_return_if_fail ( sub != NULL );
  g_return_if_fail ( GNOME_APPS_MENU_IS_DIR(dir) );
  g_return_if_fail ( sub->parent == NULL );
  g_return_if_fail ( ! dir->in_destroy );
  g_return_if_fail ( ! sub->in_destroy );
  
  sub->parent = dir;

  dir->submenus = 
    g_list_prepend( dir->submenus,
		    sub );
}

void gnome_apps_menu_remove(GnomeAppsMenu * dir,
			    GnomeAppsMenu * sub)
{
  g_return_if_fail ( dir != NULL );
  g_return_if_fail ( sub != NULL );
  g_return_if_fail ( dir->submenus != NULL );

  /* If the parent folder is being destroyed, 
     we don't want to bother removing this item from it.
     In fact, doing so might cause weird stuff to happen.
     */

  if ( ! dir->in_destroy ) {
     dir->submenus = 
       g_slist_remove ( dir->submenus,
			sub );
  }

  /* Set submenu's parent to NULL */
  sub->parent = NULL;
}

void gnome_apps_menu_insert_after(GnomeAppsMenu * dir,
				  GnomeAppsMenu * sub,
				  GnomeAppsMenu * after_me)
{
  GList * where;

  /* OK, a little too defensive. :) 
     These are no-ops if debugging is off, right? */
  g_return_if_fail ( dir != NULL );
  g_return_if_fail ( sub != NULL );
  g_return_if_fail ( after_me != NULL );
  g_return_if_fail ( GNOME_APPS_MENU_IS_DIR(dir) );
  g_return_if_fail ( sub->parent == NULL );
  g_return_if_fail ( after_me->parent == dir->parent );
  g_return_if_fail ( ! dir->in_destroy );
  g_return_if_fail ( ! sub->in_destroy );
  g_return_if_fail ( ! after_me->in_destroy );

  where = g_list_find(dir->submenus, after_me);
  
  if ( where == NULL ) {
    g_warning("Attempt to insert after nonexistent folder!");
    return;
  }

  sub->parent = dir;
 
  /* after_me should be at 0 in `where', so we put our menu at 1 */
  g_list_insert(where, sub, 1);
}

void gnome_apps_menu_foreach(GnomeAppsMenu * dir,
			     GFunc func, gpointer data)
{
  GList * submenus;
  GnomeAppsMenu * gam;
  
  g_return_if_fail ( dir != NULL );
  g_return_if_fail ( func != NULL );

  /* Note that dir doesn't really have to be a directory,
     and on most recursive calls it isn't, but there's no 
     real reason why you'd want to call the function
     on a non-directory, of course. */
  
  (* func)(dir, data);

  submenus = dir->submenus;

  while (submenus) {
    gam = (GnomeAppsMenu *)submenus->data;
    
    /* Recursively descend */
    if ( gam->submenus ) {
      gnome_apps_menu_foreach(gam, func, data);
    }

    submenus = g_list_next(submenus);
  }
}
    
