/* gnome-appsmenu.c: Copyright (C) 1998 Free Software Foundation
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

#include "gnome-appsmenu.h"
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include "libgnome/gnome-util.h"
#include "gnome-pixmap.h"

#ifndef G_DISABLE_CHECKS
static gboolean gnome_apps_menu_check(GnomeAppsMenu * gam)
{
  GnomeAppsMenu * sub;
  GList * submenus;

  if ( gam == NULL ) {g_warning("GnomeAppsMenu is null"); return FALSE;}

  /* Only directories have submenus. */
  if ( ! ( ((gam->submenus == NULL) && (!gam->is_directory)) || 
	   gam->is_directory ) ) {
    g_warning("Non-directory GnomeAppsMenu has submenus");
    return FALSE;
  }
    
  /* Should always be an extension. */
  if ( gam->extension == NULL ) {
    g_warning("GnomeAppsMenu has no extension");
    return FALSE; 
  }

  submenus = gam->submenus;
  
  while (submenus) {
    sub = (GnomeAppsMenu *)submenus->data;

    if ( ! gnome_apps_menu_check(sub) ) 
      {
	g_warning("Submenu failed validity check");
	return FALSE;
      }

    /* We should be the parent of any submenus. */
    if ( sub->parent != gam ) {
      g_warning("Submenu has wrong parent");
      return FALSE;
    }

    submenus = submenus->next;
  }

  return TRUE;
}

/* Things that should always be true once the GnomeAppsMenu is
   initialized */
#define GNOME_APPS_MENU_INVARIANTS(x) g_assert( gnome_apps_menu_check(x) )
#else
#define GNOME_APPS_MENU_INVARIANTS(x)
#endif

GnomeAppsMenu * gnome_apps_menu_new_empty(void);

/* Use a dotfile, so people won't try to edit manually
   unless they really know what they're doing. */
/* Alternatively, use "apps" to be consistent with share/apps? */

#define GNOME_APPS_MENU_DEFAULT_DIR ".Gnome Apps Menu"

typedef struct {
  gchar * extension;
  GnomeAppsMenuLoadFunc load_func;
  GnomeAppsMenuGtkMenuItemFunc menu_item_func;
} GnomeAppsMenuVariety;

/* If there are going to be more than a few varieties, this should be
   a GTree or something. But I don't think there will be, so it's not
   worth it. Easy to change later anyway.*/
static GList * file_varieties = NULL; /* list of above struct */
static GList * dir_varieties = NULL;  /* same */

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
gnome_apps_menu_register_variety( gboolean is_directory,
				  const gchar * extension,
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
  
  if(is_directory) {
    dir_varieties = g_list_append(dir_varieties, v);
  }
  else {
    file_varieties = g_list_append(file_varieties, v);
  }
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
  GList * first; 
  GList * varieties = file_varieties;

  while ( TRUE ) {    
    first = varieties;
    while ( varieties ) {
      v = (GnomeAppsMenuVariety *)varieties->data;
      gnome_apps_menu_variety_destroy(v);
      varieties = g_list_next(varieties);
    }
    g_list_free(first);

    if (file_varieties) {          /* Loop second time to clear
				      dir_varieties */
      varieties = dir_varieties;
      file_varieties = NULL;
    }
    else {
      dir_varieties = NULL;        /* done. */
      break;
    }
  }
}

static void dentry_launch_cb(GtkWidget * menuitem, gpointer gam)
{
  gnome_desktop_entry_launch( (GnomeDesktopEntry *)
			      ((GnomeAppsMenu *)gam)->data );
}

#define MENU_ICON_SIZE 20

static GtkWidget * gtk_menu_item_new_from_dentry (GnomeAppsMenu * gam)
{
  GtkWidget * menuitem;
  GtkWidget * pixmap;
  gchar * pixmap_name, * menu_name;
  GtkWidget * subdir, *subitem;
  GList * submenus;
  GtkWidget *label, *hbox, *align;

  g_return_val_if_fail(gam != NULL, NULL);

  menu_name = ((GnomeDesktopEntry *)gam->data)->name;
  pixmap_name = ((GnomeDesktopEntry *)gam->data)->icon;

  if ( pixmap_name && g_file_exists(pixmap_name) ) {
    pixmap = gnome_pixmap_new_from_file_at_size (pixmap_name,
						 MENU_ICON_SIZE,
						 MENU_ICON_SIZE);
    /* For now gnome_pixmap_* don't ever return NULL, AFAICT. This is
       still a bug in the panel/menu.c code */
  }
  else pixmap = NULL;
  
  menuitem = gtk_menu_item_new();

  label = gtk_label_new (menu_name);
  gtk_misc_set_alignment (GTK_MISC(label), 0.0, 0.5);
	
  hbox = gtk_hbox_new (FALSE, 0);
	
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_border_width (GTK_CONTAINER (align), 1);

  gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, FALSE, 0);
  
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
  gtk_container_add (GTK_CONTAINER (menuitem), hbox);
  
  if (pixmap) gtk_container_add (GTK_CONTAINER (align), pixmap);
  gtk_widget_set_usize (align, 22, 16);

  /* At this point panel/menu.c saves the pixmap in a list,
     and it is not quickly clear to me why. I need to look 
     at it better and consider doing the same. */
  
  gtk_widget_show (align);
  gtk_widget_show (hbox);
  gtk_widget_show (label);
  if (pixmap) gtk_widget_show (pixmap);
      
  if ( GNOME_APPS_MENU_IS_DIR(gam) ) {

    subdir = gtk_menu_new();
    gtk_widget_show(subdir);

    gtk_menu_item_set_submenu ( GTK_MENU_ITEM(menuitem), subdir );
    
    submenus = gam->submenus;

    while ( submenus ) {
      
      subitem = gtk_menu_item_new_from_dentry ( (GnomeAppsMenu *)(submenus->data));
      if (subitem) {
	gtk_menu_append(GTK_MENU(subdir), subitem);
	gtk_widget_show(subitem);
      }

      submenus = g_slist_next(submenus);
    }
  }
  else {
    /* Connect callback if it's not a directory */
    gtk_signal_connect ( GTK_OBJECT(menuitem), "activate",
			 GTK_SIGNAL_FUNC(dentry_launch_cb),
			 gam );
  }

  return menuitem;
}

static void make_default_varieties(void)
{
  static gboolean already_made = FALSE;

  if ( ! already_made ) {
    gnome_apps_menu_register_variety( FALSE, 
				      GNOME_APPS_MENU_DENTRY_EXTENSION,
				      gnome_desktop_entry_load,
				      gtk_menu_item_new_from_dentry );
    gnome_apps_menu_register_variety( TRUE, "directory",
				      gnome_desktop_entry_load,
				      gtk_menu_item_new_from_dentry );
    already_made = TRUE;
  }
}

static GnomeAppsMenuVariety * 
find_variety(GnomeAppsMenu * gam)
{
  GList * list;
  GnomeAppsMenuVariety * v;

  g_return_val_if_fail(gam != NULL, NULL);
  g_return_val_if_fail(gam->extension != NULL, NULL);

  list = GNOME_APPS_MENU_IS_DIR(gam) ? dir_varieties : file_varieties;
  
  g_return_val_if_fail(list != NULL, NULL);

  while ( list ) {
    v = (GnomeAppsMenuVariety *)list->data;
    g_assert ( v != NULL );

    g_print("Comparing %s with\n %s\n", gam->extension, v->extension);

    if ( strcmp (gam->extension, v->extension) == 0 ) {
      return v;
    }
    
    list = g_list_next(list);
  }

  return NULL; /* didn't find an appropriate extension */
}


static GnomeAppsMenuLoadFunc 
get_load_func_and_init_apps_menu( const gchar * filename,
				  GnomeAppsMenu * dest )
{
  GnomeAppsMenuVariety * v;
  gint len;
  gchar * extension = NULL;

  g_return_val_if_fail(filename != NULL, NULL);
  g_return_val_if_fail(dest != NULL, NULL);

  /* Extract extension from filename */

  len = strlen(filename);

  while ( len >= 0 ) {
    --len; /* always skip last character, because if
	      the filename ends in . it doesn't count. */
    if (filename[len] == '.') {
      extension = &filename[len + 1]; /* without period */
      break;
    }
  }
  
  if ( extension == NULL ) {
    g_warning("gnome-appsmenu: file %s has no extension", filename);
    return NULL;
  }

  /* set extension */
  dest->extension = g_strdup(extension);

  v = find_variety(dest);

  if ( v ) {
    /* return function. */
    return v->load_func;
  }
  else return NULL;
}

static void free_filename_list(gchar ** list)
{
  gchar ** start = list;
  while ( *list ) {
    g_free ( *list );
    ++list;
  }
  g_free(start);
}


static gchar ** get_filename_list(const gchar * directory)
{
  gint max_filenames = 100;
  gchar ** filename_list = NULL;
  struct dirent * dir_entry = NULL;
  DIR * dir = NULL;
  gint next = 0;

  dir = opendir(directory);

  if (dir == NULL) {
    g_warning("gnome-appsmenu: error on directory %s: %s",
	      directory, g_unix_error_string(errno));
    return NULL;
  }

  filename_list = g_malloc(sizeof(gchar *) * max_filenames);

  while ( (dir_entry = readdir(dir)) != NULL) {
    gchar * thisfile;

    thisfile = dir_entry->d_name;

    /* Skip . and .. */

    if ((thisfile[0] == '.' && thisfile[1] == '\0') ||
	(thisfile[0] == '.' && thisfile[1] == '.' && 
	 thisfile[2] == '\0')) {
      continue;
    }

    filename_list[next] = g_concat_dir_and_file(directory, thisfile);
    ++next;
    if ( next > max_filenames ) {
      filename_list = g_realloc(filename_list, sizeof(gchar *)*100);
      max_filenames += 100;
    }
  }
  filename_list[next] = NULL;
  return filename_list;
}

static gboolean is_dotfile(const gchar * filename)
{
  gint len;
  if (filename[0] == '.') return TRUE;
  len = strlen(filename);
  while ( len > 0 ) {
    if (filename[len] == '.') break;
    --len;
  }
  if ( len == 0 ) return FALSE; /* no dots */
  /* len is indexing the . */
  if (filename[len - 1] == '/') return TRUE;
  else return FALSE;
}

/* Get the directory info from this filename list */
static GnomeAppsMenu * load_directory_info(gchar ** filename_list)
{
  GnomeAppsMenu * dir_menu = NULL;

  while ( *filename_list ) {
    GnomeAppsMenuLoadFunc load_func = NULL;
    gchar * filename;
    
    filename = *filename_list;
    ++filename_list;

    /* ignore anything that's not a dotfile */
    if ( ! is_dotfile(filename) ) continue;

    dir_menu = gnome_apps_menu_new_empty();
    dir_menu->is_directory = TRUE;
    load_func = get_load_func_and_init_apps_menu( filename, 
						  dir_menu ); 
      
    if ( load_func != NULL ) {
      dir_menu->data = load_func(filename);
    }
      
    /* if load_func == NULL this will be true too, 
       because of the default initialization of AppsMenu 
       in _new() */
    /* This situation means the file type was unrecognized,
       perhaps belonging to another app, so we want 
       to ignore this file. */
    if ( dir_menu->data == NULL ) {
      gnome_apps_menu_destroy(dir_menu);
      dir_menu = NULL;
      continue;
    }

    break; /* If we haven't continued, we have what we wanted.  All
	    dotfiles after the first are ignored, (unless we didn't
	    know how to load the first.) */
  }

  return dir_menu;
}


/* Function based on panel code, Miguel de Icaza and Federico Mena 
   (The lame parts are probably mine. :) */

GnomeAppsMenu * gnome_apps_menu_load(const gchar * directory)
{
  GnomeAppsMenu * root_apps_menu = NULL;
  gchar ** filename_list;
  gchar ** filename_list_start;

  make_default_varieties();

  filename_list_start = filename_list = get_filename_list(directory);

  if ( filename_list == NULL ) return NULL;

#ifdef DEBUG
  while ( *filename_list ) {
    g_print("%s\n", *filename_list);
    ++filename_list;
  }
  filename_list = filename_list_start;
#endif

  root_apps_menu = load_directory_info(filename_list);

  if (root_apps_menu == NULL) {
    g_warning("No information for directory %s", directory);
    free_filename_list(filename_list_start);
    return NULL;
  }

  while (*filename_list) {
    GnomeAppsMenuLoadFunc load_func = NULL;
    GnomeAppsMenu * sub_apps_menu = NULL;
    gchar * filename;
    struct stat s;

    filename = *filename_list;
    ++filename_list;

    /* Ignore all dotfiles -- they're meant to describe the 
       directory, not items in it. */
    if ( is_dotfile(filename) ) {
      continue;
    }

    /* Be sure we can read the file */
    if (stat (filename, &s) == -1) {
      g_warning("gnome-appsmenu: error on file %s: %s\n", 
		filename, g_unix_error_string(errno));
      continue;
    }

    /* If it's a directory, recursively descend into it */
    if (S_ISDIR (s.st_mode)) {

      sub_apps_menu = gnome_apps_menu_load(filename);
      
      /* Recursive load failed for some reason. */
      if ( sub_apps_menu == NULL ) {
	continue;
      }      
    }

    /* Otherwise load the info. */
    else {
      sub_apps_menu = gnome_apps_menu_new_empty();
      load_func = get_load_func_and_init_apps_menu( filename, 
						    sub_apps_menu ); 
      
      if ( load_func != NULL ) {
	sub_apps_menu->data = load_func(filename);
      }
      
      /* if load_func == NULL this will be true too, 
	 because of the default initialization of AppsMenu 
	 in _new() */
      /* This situation means the file type was unrecognized,
	 perhaps belonging to another app, so we want 
	 to ignore this file. */
      if ( sub_apps_menu->data == NULL ) {
	gnome_apps_menu_destroy(sub_apps_menu);
	sub_apps_menu = NULL;
	continue;
      }
    }

    gnome_apps_menu_append( root_apps_menu,
			    sub_apps_menu );      
    
  } /* while (there are files in the list) */

  /* The panel code destroyed the directory menu if it was empty,
     may want to do that.

     if ( root_apps_menu->submenus == NULL && 
     GNOME_APPS_MENU_IS_DIR(root_apps_menu)) {
     gnome_apps_menu_destroy(root_apps_menu);
     return NULL;
     }
     */

  free_filename_list(filename_list_start);
  
  if(root_apps_menu) GNOME_APPS_MENU_INVARIANTS(root_apps_menu);

  /* Could be NULL, note */
  return root_apps_menu;
}


GnomeAppsMenu * gnome_apps_menu_load_default(void)
{
  gchar * s;
  GnomeAppsMenu * gam;

  s = gnome_util_home_file(GNOME_APPS_MENU_DEFAULT_DIR);
 
  gam = gnome_apps_menu_load(s);

  g_free(s);

  return gam;
}



GnomeAppsMenu * gnome_apps_menu_load_system(void)
{
  gchar * s;
  GnomeAppsMenu * gam;

  s = gnome_datadir_file("apps");
 
  gam = gnome_apps_menu_load(s);

  g_free(s);

  return gam;
}

GnomeAppsMenuGtkMenuItemFunc
get_menu_item_func(GnomeAppsMenu * gam)
{
  GnomeAppsMenuVariety * v;

  g_return_val_if_fail(gam != NULL, NULL);
  g_assert(gam->extension != NULL);

  v = find_variety(gam);
  if (v) {
    return v->menu_item_func;
  }
  else return NULL;
}

GtkWidget * gtk_menu_item_new_from_apps_menu(GnomeAppsMenu * gam)
{
  GnomeAppsMenuGtkMenuItemFunc menu_item_func;

  g_return_val_if_fail(gam != NULL, NULL);

  menu_item_func = get_menu_item_func(gam);
  
  if (menu_item_func == NULL) {
    /* This kind of thing shouldn't show up on menus, because
       it has no function to make it show up.*/
    return NULL;
  }
  
  return menu_item_func(gam);
}

GtkWidget * gtk_menu_new_from_apps_menu(GnomeAppsMenu * gam)
{
  GtkWidget * root_menu;
  GtkWidget * menuitem;

  g_return_val_if_fail(gam != NULL, NULL);    

  root_menu = gtk_menu_new();

  menuitem = gtk_menu_item_new_from_apps_menu(gam);
  
  if (menuitem != NULL) {
    gtk_menu_append( GTK_MENU(root_menu), menuitem );
    gtk_widget_show(menuitem);
  }

  return root_menu;
}

/****************************************************
  Remaining functions are for manipulating the apps_menu
  trees. Don't relate to config files. 
  *******************************************/

GnomeAppsMenu * gnome_apps_menu_new_empty(void)
{
  GnomeAppsMenu * gam = g_new(GnomeAppsMenu, 1);

  gam->is_directory = FALSE;
  gam->extension = NULL;
  gam->data = NULL;
  gam->submenus = NULL;
  gam->parent = NULL;
  gam->in_destroy = FALSE;
  
  return gam;
}

GnomeAppsMenu * gnome_apps_menu_new(gboolean is_directory,
				    const gchar * extension,
				    gpointer data)
{
  GnomeAppsMenu * gam;
  
  gam = gnome_apps_menu_new_empty();
  gam->is_directory = is_directory;
  gam->extension = g_strdup(extension);
  gam->data = data;

  GNOME_APPS_MENU_INVARIANTS(gam);

  return gam;
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

  GNOME_APPS_MENU_INVARIANTS(dir);
  GNOME_APPS_MENU_INVARIANTS(sub);
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

  GNOME_APPS_MENU_INVARIANTS(dir);
  GNOME_APPS_MENU_INVARIANTS(sub);
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
       g_list_remove ( dir->submenus,
		       sub );
  }

  /* Set submenu's parent to NULL */
  sub->parent = NULL;

  GNOME_APPS_MENU_INVARIANTS(dir);
  GNOME_APPS_MENU_INVARIANTS(sub);
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

  GNOME_APPS_MENU_INVARIANTS(dir);
  GNOME_APPS_MENU_INVARIANTS(sub);
  GNOME_APPS_MENU_INVARIANTS(after_me);
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

  GNOME_APPS_MENU_INVARIANTS(dir);

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
