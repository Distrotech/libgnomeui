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
#include <ftw.h>
#include "libgnome/gnome-util.h"
#include "gnome-pixmap.h"


static GnomeAppsMenu * gnome_apps_menu_new_empty(void);

/* Use a dotfile, so people won't try to edit manually
   unless they really know what they're doing. */
/* Alternatively, use "apps" to be consistent with share/apps? */

#define GNOME_APPS_MENU_DEFAULT_DIR ".Gnome Apps Menu"

#define BACKUP_EXTENSION "bak"

/*******************************
  Debugging stuff
  ********************************/

#define GNOME_ENABLE_DEBUG 1
#ifdef GNOME_ENABLE_DEBUG
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

  /* Should always have a vtable */
  if ( gam->vtable == NULL ) {
    g_warning("GnomeAppsMenu has no vtable");
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

void gnome_apps_menu_debug_print(GnomeAppsMenu * gam)
{
  gchar * pixmap, * name;
  
  g_return_if_fail(gam != NULL);

  gnome_apps_menu_setup(gam, &pixmap, &name, NULL, NULL);

  g_print("GnomeAppsMenu: %p; directory? %s; Name: %s; Ext: %s;\n  %s;\n"
	  "  Data: %p; Submenus: %p; Parent: %p Vtable: %p\n", 
	  gam, GNOME_APPS_MENU_IS_DIR(gam) ? "yes" : "no",
	  name, gam->extension, pixmap, gam->data, gam->submenus, 
	  gam->parent, gam->vtable );
}

/* Things that should always be true once the GnomeAppsMenu is
   initialized */
#define GNOME_APPS_MENU_INVARIANTS(x) g_assert( gnome_apps_menu_check(x) )

#define GNOME_APPS_MENU_DEBUG_PRINT(x) gnome_apps_menu_debug_print(x)

#else /* GNOME_ENABLE_DEBUG */
#define GNOME_APPS_MENU_INVARIANTS(x)
#define GNOME_APPS_MENU_DEBUG_PRINT(x)
#endif

/* Extract filename from a path, NULL if no filename */
gchar * g_extract_file(const gchar * path)
{
  int len, i;
  g_return_val_if_fail(path != NULL, NULL);

  i = len = strlen(path);

  while ( i >= 0 ) {
    --i;
    if (path[i] == PATH_SEP) break;
  }
  
  /* path ended in PATH_SEP */
  if ( i == len - 1) return NULL;
  /* i is indexing the separator, so return position one past */
  else {
    return g_strdup(&path[i+1]);
  }
}

/****************************************
  Class-handling stuff
  ***********************************/

/* If there are going to be more than a few classes, this should be
   a GTree or something. But I don't think there will be, so it's not
   worth it. Easy to change later anyway.*/
static GList * file_classes = NULL; /* list of above struct */
static GList * dir_classes = NULL;  /* same */

static GnomeAppsMenuClass * 
gnome_apps_menu_class_new( const gchar * extension,
			   GnomeAppsMenuLoadFunc load_func,
			   GnomeAppsMenuSetupFunc setup_func,
			   GnomeAppsMenuSaveFunc save_func )
{
  GnomeAppsMenuClass * c;

  g_return_val_if_fail(extension != NULL, NULL);
  
  c = g_new(GnomeAppsMenuClass, 1);
  
  c->extension = g_strdup(extension);
  c->load_func = load_func;
  c->setup_func = setup_func;
  c->save_func = save_func;

  return c;
}


void 
gnome_apps_menu_register_class( gboolean is_directory,
				const gchar * extension,
				GnomeAppsMenuLoadFunc load_func,
				GnomeAppsMenuSetupFunc setup_func,
				GnomeAppsMenuSaveFunc save_func )
{
  GnomeAppsMenuClass * c;

  /* This would make a mess. */
  g_return_if_fail ( strcmp(extension, BACKUP_EXTENSION) != 0 );

  c = gnome_apps_menu_class_new( extension, load_func, 
				 setup_func, save_func );

  if ( c == NULL ) {
    g_warning("AppsMenuClass registration failed");
    return;
  }
  
  /* use _append so that classes are prioritized in the order
     they were added. */

  if(is_directory) {
    dir_classes = g_list_append(dir_classes, c);
  }
  else {
    file_classes = g_list_append(file_classes, c);
  }
}


static void 
gnome_apps_menu_class_destroy( GnomeAppsMenuClass * c )
{
  g_return_if_fail ( c != NULL );
  
  g_free(c->extension);
  g_free(c);
}

static void clear_classes(void)
{
  GnomeAppsMenuClass * c;
  GList * first; 
  GList * classes = file_classes;

  while ( TRUE ) {    
    first = classes;
    while ( classes ) {
      c = (GnomeAppsMenuClass *)classes->data;
      gnome_apps_menu_class_destroy(c);
      classes = g_list_next(classes);
    }
    g_list_free(first);

    if (file_classes) {          /* Loop second time to clear
				      dir_classes */
      classes = dir_classes;
      file_classes = NULL;
    }
    else {
      dir_classes = NULL;        /* done. */
      break;
    }
  }
}


static GnomeAppsMenuClass * 
find_class_by_extension(const gchar * extension, 
			gboolean is_directory)
{
  GList * list;
  GnomeAppsMenuClass * c;

  g_return_val_if_fail(extension != NULL, NULL);

  list = is_directory ? dir_classes : file_classes;
  
  g_return_val_if_fail(list != NULL, NULL);

  while ( list ) {
    c = (GnomeAppsMenuClass *)list->data;
    g_assert ( c != NULL );

    if ( strcmp (extension, c->extension) == 0 ) {
      return c;
    }
    
    list = g_list_next(list);
  }

  return NULL; /* didn't find an appropriate extension */
}


/*******************************************
  Code to handle default classes 
  **********************************/

static void dentry_launch_cb(GtkWidget * menuitem, gpointer gam)
{
  gnome_desktop_entry_launch( (GnomeDesktopEntry *)
			      ((GnomeAppsMenu *)gam)->data );
}

static void 
dentry_setup_func( GnomeAppsMenu * gam, gchar ** pixmap_name, gchar ** name,
		   GtkSignalFunc * callback, gpointer * data )
{
  g_return_if_fail(gam != NULL);
  
  if (name) *name = 
	      ((GnomeDesktopEntry *)gam->data)->name;
  if (pixmap_name) *pixmap_name = 
		     ((GnomeDesktopEntry *)gam->data)->icon;

  if (callback) 
    *callback = GTK_SIGNAL_FUNC(dentry_launch_cb);

  if (data) 
    *data = gam;
}

static void dentry_save_func( GnomeAppsMenu * gam, 
			      const gchar * directory )
{
  gchar * filename = NULL;
  gchar * name;
  g_return_if_fail(gam != NULL);
  g_return_if_fail(directory != NULL);

  if ( ((GnomeDesktopEntry *)gam->data)->location ) {
    filename = g_extract_file( ((GnomeDesktopEntry *)gam->data)->location );
  }
  else if (GNOME_APPS_MENU_IS_DIR(gam)) {
    filename = g_strdup( "."GNOME_APPS_MENU_DENTRY_DIR_EXTENSION );
  }
  else {
    gnome_apps_menu_setup(gam, NULL, &name, NULL, NULL);
    if (name) {
      filename = g_copy_strings ( name, 
				  GNOME_APPS_MENU_DENTRY_EXTENSION, NULL);
    }
  }

  if (filename == NULL) {
    g_warning("Couldn't decide on a filename for this menu.");
    return;
  }

  g_free( ((GnomeDesktopEntry *)gam->data)->location ); /* free if it exists */

  ((GnomeDesktopEntry *)gam->data)->location = 
    g_concat_dir_and_file(directory, filename);

  g_free(filename);

  gnome_desktop_entry_save((GnomeDesktopEntry *)gam->data);
}

static void make_default_classes(void)
{
  static gboolean already_made = FALSE;

  if ( ! already_made ) {
    gnome_apps_menu_register_class( FALSE, 
				    GNOME_APPS_MENU_DENTRY_EXTENSION,
				    (GnomeAppsMenuLoadFunc)
				    gnome_desktop_entry_load,
				    dentry_setup_func,
				    dentry_save_func );
    gnome_apps_menu_register_class( TRUE, 
				    GNOME_APPS_MENU_DENTRY_DIR_EXTENSION,
				    (GnomeAppsMenuLoadFunc)
				    gnome_desktop_entry_load,
				    dentry_setup_func,
				    dentry_save_func );
    already_made = TRUE;
  }
}

/*****************************************
  Utility functions for handling files and directories. 
  *********************************************/

static void free_filename_list(gchar ** list)
{
  gchar ** start = list;

  g_return_if_fail(list != NULL);

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

static gboolean is_dotfile(const gchar * path)
{
  gint len;

  if (path[0] == '.') return TRUE;
  len = strlen(path);
  while ( len > 0 ) {
    if ( path[len] == PATH_SEP ) return FALSE; /* no dots */
    if ( path[len] == '.' ) break;
    --len;
  }
  if ( len == 0 ) return FALSE; /* no dots */
  /* len is indexing the . */
  if (path[len - 1] == PATH_SEP) return TRUE;
  else return FALSE;
}

static gchar * get_extension(const gchar * filename)
{
  gint len;
  const gchar * extension = NULL;

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
    return NULL;
  }
  else return g_strdup(extension);
}

/**********************************
  Code to load the GnomeAppsMenu from disk
  ************************************/

/* Different from _load because it's not recursive.
   "file" can't be a directory, it has to be a real file. */
static GnomeAppsMenu * 
gnome_apps_menu_new_from_file(const gchar * filename)
{
  GnomeAppsMenuClass * c;
  GnomeAppsMenu * gam;
  gchar * extension;
  gboolean is_directory;

  g_return_val_if_fail(filename != NULL, NULL);

  extension = get_extension(filename);

  /* The class find would fail anyway; this is just an optimization. */
  if ( strcmp(extension, BACKUP_EXTENSION) == 0 ) {
    g_free(extension);
    return NULL;
  }

  is_directory = is_dotfile(filename);

  c = find_class_by_extension(extension, is_directory);
  
  if (c == NULL) {
    g_free(extension);
    return NULL;
  }
  else {
    gam = gnome_apps_menu_new_empty();
  
    gam->extension = extension; /* memory now owned by gam */
    gam->is_directory = is_directory;
    gam->vtable = c;

    if (gam->vtable->load_func) {
      gam->data = (gam->vtable->load_func)(filename);
      if (gam->data == NULL) {
	gnome_apps_menu_destroy(gam);
	return NULL;
      }
    }
    else {
      gnome_apps_menu_destroy(gam);
      return NULL;
    }

    return gam;
  }
}

/* Get the directory info from this filename list */
static GnomeAppsMenu * load_directory_info(gchar ** filename_list)
{
  GnomeAppsMenu * dir_menu = NULL;

  while ( *filename_list ) {
    gchar * filename;
    
    filename = *filename_list;
    ++filename_list;

    /* ignore anything that's not a dotfile */
    if ( ! is_dotfile(filename) ) continue;

    dir_menu = gnome_apps_menu_new_from_file(filename);

    if (dir_menu == NULL) {
      /* Keep going in case another one works. */
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

  make_default_classes();

  filename_list_start = filename_list = get_filename_list(directory);

  if ( filename_list == NULL ) return NULL;

  root_apps_menu = load_directory_info(filename_list);

  if (root_apps_menu == NULL) {
    g_warning("No information for directory %s", directory);
    free_filename_list(filename_list_start);
    return NULL;
  }

  while (*filename_list) {
    GnomeAppsMenu * sub_apps_menu = NULL;
    gchar * filename;
    struct stat s;

    filename = *filename_list;
    ++filename_list;

    g_print("fn: %s\n", filename);

    /* Ignore all dotfiles -- they're meant to describe the 
       directory, not items in it. */
    if ( is_dotfile(filename) ) {
#ifdef GNOME_ENABLE_DEBUG
      g_print("Ignoring dotfile %s\n", filename);
#endif
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

      g_print("Dir: %s", filename);

      sub_apps_menu = gnome_apps_menu_load(filename);
      
      /* Recursive load failed for some reason. */
      if ( sub_apps_menu == NULL ) {
#ifdef GNOME_ENABLE_DEBUG
	g_print("Couldn't load %s\n", filename);
#endif
	continue;
      }      
    }

    /* Otherwise load the info. */
    else {
      sub_apps_menu = gnome_apps_menu_new_from_file(filename);

      if ( sub_apps_menu == NULL ) {
	/* didn't load it successfully, ignore it. */
#ifdef GNOME_ENABLE_DEBUG
	g_print("Couldn't load %s\n", filename);
#endif
	continue;
      }
    }

    gnome_apps_menu_append( root_apps_menu,
			    sub_apps_menu );      
    
  } /* while (there are files in the list) */

  /* Destroy menu if it's empty */
  if ( root_apps_menu->submenus == NULL && 
       GNOME_APPS_MENU_IS_DIR(root_apps_menu)) {
    gnome_apps_menu_destroy(root_apps_menu);
    root_apps_menu = NULL;
#ifdef GNOME_ENABLE_DEBUG
    g_print("Ignoring empty GnomeAppsMenu\n");
#endif
  }

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
 
  if (g_file_exists(s)) {
    gam = gnome_apps_menu_load(s);
  }
  else {
    /* Default to system menu */
    gam = gnome_apps_menu_load_system();
  }

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

/*****************************************************
  Save to disk
  ***************************************/

static gboolean known_class(const gchar * path, 
			    gboolean is_directory)
{
  gchar * extension;

  extension = get_extension(path);

  if ( extension == NULL ) return FALSE;

  if ( find_class_by_extension( extension, is_directory ) != NULL) {
    g_free(extension);
    return TRUE; 
  }
  else {
    g_free(extension);
    return FALSE;
  }
}

static gboolean is_backup(const gchar * path)
{
  int len = strlen(path);
  int dot_index = len - strlen("."BACKUP_EXTENSION);
  
  if ( dot_index < 0 ) return FALSE;

  return (strcmp("."BACKUP_EXTENSION, &path[dot_index]) == 0);
}

/* Non-recursively create directory if it doesn't exist, if it does
   exist backup known files in it and delete originals */
static gboolean prepare_directory(const gchar * directory)
{
  gchar * new_name;
  struct stat s;
  gchar ** filename_list, ** filename_list_start;
  const gchar * filename;

  /* If the directory doesn't exist just create it and return */
  if ( ! g_file_exists(directory) ) {
    if ( mkdir (directory, S_IRUSR | S_IWUSR | S_IXUSR ) == -1 ) {
      g_warning( "Couldn't create directory %s, error: %s", directory,
		 g_unix_error_string(errno) );
      return FALSE;
    }
    return TRUE;
  }
  
  filename_list_start = filename_list = get_filename_list(directory);

  if (filename_list == NULL) return FALSE;

  while ( *filename_list ) {
    filename = *filename_list;
    ++filename_list;

    g_print("fn: %s\n", filename);

    if ( stat(filename, &s) == -1 ) {
      /* ignore this file */
      continue;
    }

    /* Ignore directories, except if they're empty prune them. */  
    if ( S_ISDIR(s.st_mode) ) {
      /* How do you tell if a directory's empty? Wild shot in the
	 dark that it works like ls? */
      if ( s.st_nlink == 2 ) {
	unlink (filename);
      }
      continue;
    }

    /* dotfiles are directory info, non-dotfiles aren't. */
    if ( known_class ( filename, 
		       is_dotfile(filename) ) ) {

      new_name = g_copy_strings(filename, 
				"."BACKUP_EXTENSION, NULL);
    
      rename ( filename, new_name );
      g_free(new_name);
    }  
    else if ( is_backup(filename) ) {
      /* Delete old backups */
      unlink(filename);
    }
  }

  free_filename_list(filename_list_start);

  return TRUE;
}

gboolean gnome_apps_menu_save(GnomeAppsMenu * gam, 
			      const gchar * directory)
{
  GList * submenus;
  GnomeAppsMenu * sub;
  gchar * subdir, * full_subdir = NULL;  
  gboolean return_val = TRUE;

  g_return_val_if_fail(gam != NULL, FALSE);
  g_return_val_if_fail(directory != NULL, FALSE);

#ifdef GNOME_ENABLE_DEBUG
  g_print("**** \n Save got: \n");
  GNOME_APPS_MENU_DEBUG_PRINT(gam);
#endif

  /* Just ignore anything with no save function */
  if ( gam->vtable->save_func == NULL ) return TRUE;

  if ( GNOME_APPS_MENU_IS_DIR(gam) &&
       (gam->parent != NULL) ) {

    /* Get the name of this directory AppsMenu, so we know the name of
       the filesystem directory to store it and submenus in.
       The root menu goes in the passed-in directory. */

    gnome_apps_menu_setup(gam, NULL, &subdir, NULL, NULL);
  
    g_print("Got subdir %s\n", subdir);
    
    full_subdir = g_concat_dir_and_file(directory, subdir);
    
    g_print("Full: %s\n", full_subdir);  

    directory = full_subdir; /* alias */
  }

  if (! prepare_directory(directory) ) {
    g_free(full_subdir);
    return FALSE;
  }

  (gam->vtable->save_func)(gam, directory);

  submenus = gam->submenus;

  while (submenus) {
    sub = (GnomeAppsMenu *)submenus->data;

    if ( ! gnome_apps_menu_save(sub, directory) ) {
      return_val = FALSE; /* indicate error but keep going as much as
			     possible */
    }

    submenus = g_list_next(submenus);
  }

  /* Don't free subdir, stuff from _setup() doesn't belong
     to us */
  g_free(full_subdir); /* if non-null */

  return return_val;
}

gboolean gnome_apps_menu_save_default(GnomeAppsMenu * gam)
{
  gchar * s;
  gboolean return_val;
  
  s = gnome_util_home_file(GNOME_APPS_MENU_DEFAULT_DIR);
 
  return_val = gnome_apps_menu_save(gam, s);

  g_free(s);

  return return_val;
}

/**************************************
  Setup stuff
*****************************************/

void gnome_apps_menu_setup( GnomeAppsMenu * gam, gchar ** pixmap_name,
			    gchar ** name, GtkSignalFunc * callback,
			    gpointer * data )
{  
  g_return_if_fail(gam != NULL);

  if (gam->vtable->setup_func == NULL) {
    if (pixmap_name) *pixmap_name = NULL;
    if (name)        *name = NULL;
    if (callback)    *callback = NULL;
    if (data)        *data = NULL;
  }
  else {
    (gam->vtable->setup_func)(gam, pixmap_name, name, callback, data);
  }
}

/********************************************
  Functions for creating a GtkMenu[Item] from a GnomeAppsMenu
  *******************************************/

#define MENU_ICON_SIZE 20

GtkWidget * gtk_menu_item_new_from_apps_menu(GnomeAppsMenu * gam)
{
  GtkWidget * menuitem, * submenu;
  GtkWidget * pixmap;
  gchar * pixmap_name, * menuitem_name;
  GtkWidget *label, *hbox, *align;
  GtkSignalFunc callback;
  gpointer callback_data;

  g_return_val_if_fail(gam != NULL, NULL);


  gnome_apps_menu_setup( gam, &pixmap_name, &menuitem_name, 
			 &callback, &callback_data );

  
  if ( pixmap_name && g_file_exists(pixmap_name) ) {
    pixmap = gnome_pixmap_new_from_file_at_size (pixmap_name,
						 MENU_ICON_SIZE,
						 MENU_ICON_SIZE);
    /* For now gnome_pixmap_* don't ever return NULL, AFAICT. This is
       still a bug in the panel/menu.c code */
  }
  else pixmap = NULL;

  if (menuitem_name == NULL) menuitem_name = "";
    
  menuitem = gtk_menu_item_new();

  label = gtk_label_new (menuitem_name);
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
    
    if ( gam->submenus != NULL ) {
      submenu = gtk_menu_new_from_apps_menu(gam);
      if (submenu) {
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), submenu);
	gtk_widget_show(submenu);
      }
    }

  }
  /* Connect callback if it's not a directory */
  else {
    if (callback) {
      gtk_signal_connect ( GTK_OBJECT(menuitem), "activate",
			   callback,
			   callback_data );
    }
  }
  
  return menuitem;
}

GtkWidget * gtk_menu_new_from_apps_menu(GnomeAppsMenu * gam)
{
  GtkWidget * menu, * subitem;
  GList * submenus;
  gint items;
  
  g_return_val_if_fail(GNOME_APPS_MENU_IS_DIR(gam), NULL);

  menu = gtk_menu_new();    
  submenus = gam->submenus;
  items = 0;

  while ( submenus ) {
    
    subitem = 
      gtk_menu_item_new_from_apps_menu ( (GnomeAppsMenu *)(submenus->data));
    
    if (subitem) {
      gtk_menu_append(GTK_MENU(menu), subitem);
      gtk_widget_show(subitem);
      ++items;
    }
      
    submenus = g_list_next(submenus);
  }

  if (items > 0) return menu;
  else {
    gtk_widget_destroy(menu);
    return NULL;
  }
}


/****************************************************
  Remaining functions are for manipulating the apps_menu
  trees. Don't relate to config files. 
  *******************************************/

static GnomeAppsMenu * gnome_apps_menu_new_empty(void)
{
  GnomeAppsMenu * gam = g_new(GnomeAppsMenu, 1);

  gam->is_directory = FALSE;
  gam->extension = NULL;
  gam->data = NULL;
  gam->submenus = NULL;
  gam->parent = NULL;
  gam->in_destroy = FALSE;
  gam->vtable = NULL;
  
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
  gam->vtable = find_class_by_extension(extension, is_directory);

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

  /* Note that the same vtable is shared
     by all instances of the class, so it's 
     not destroyed. */
  
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
