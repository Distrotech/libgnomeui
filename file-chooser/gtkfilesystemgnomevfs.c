/* GTK - The GIMP Toolkit
 * gtkfilesystemgnomevfs.c: Implementation of GtkFileSystem for gnome-vfs
 * Copyright (C) 2003, Novell, Inc.
 * Copyright (C) 2003, Red Hat, Inc.
 * Copyright (C) 1999, 2000 Eazel, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: Owen Taylor <otaylor@redhat.com>
 *          Federico Mena-Quintero <federico@ximian.com>
 *
 * portions come from eel-vfs-extensions.c:
 *
 *   Authors: Darin Adler <darin@eazel.com>
 *	      Pavel Cisler <pavel@eazel.com>
 *	      Mike Fleming  <mfleming@eazel.com>
 *            John Sullivan <sullivan@eazel.com>
 */

/* TODO:
 * - handle local only property (should go in gtkfilechooserdefault 
 * - Don't we need desktop file link support?
 * - Keeping all the gnome-vfs-infos in the hashtable looks wasteful, but otherwise we'll do to much i/o
 */

#include "gtkfilesystemgnomevfs.h"

#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-volume-monitor.h>
#include <libgnomevfs/gnome-vfs-drive.h>
#include <libgnomevfs/gnome-vfs-i18n.h>

#ifdef USE_GCONF
#include <gconf/gconf-client.h>
#endif

#include <libgnomeui/gnome-icon-lookup.h>

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BOOKMARKS_FILENAME ".gtk-bookmarks"
#define BOOKMARKS_TMP_FILENAME ".gtk-bookmarks-XXXXXX"

typedef struct _GtkFileSystemGnomeVFSClass GtkFileSystemGnomeVFSClass;

#define GTK_FILE_SYSTEM_GNOME_VFS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_FILE_SYSTEM_GNOME_VFS, GtkFileSystemGnomeVFSClass))
#define GTK_IS_FILE_SYSTEM_GNOME_VFS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_FILE_SYSTEM_GNOME_VFS))
#define GTK_FILE_SYSTEM_GNOME_VFS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_FILE_SYSTEM_GNOME_VFS, GtkFileSystemGnomeVFSClass))

static GType type_gtk_file_system_gnome_vfs = 0;
static GType type_gtk_file_folder_gnome_vfs = 0;

struct _GtkFileSystemGnomeVFSClass
{
  GObjectClass parent_class;
};

struct _GtkFileSystemGnomeVFS
{
  GObject parent_instance;

  GHashTable *folders;

  GnomeVFSVolumeMonitor *volume_monitor;

#ifdef USE_GCONF
  GConfClient *client;
  guint client_notify_id;
  GSList *bookmarks;
#endif

  char *desktop_uri;
  guint locale_encoded_filenames : 1;
};

#define GTK_TYPE_FILE_FOLDER_GNOME_VFS             (gtk_file_folder_gnome_vfs_get_type())
#define GTK_FILE_FOLDER_GNOME_VFS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_FILE_FOLDER_GNOME_VFS, GtkFileFolderGnomeVFS))
#define GTK_IS_FILE_FOLDER_GNOME_VFS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_FILE_FOLDER_GNOME_VFS))
#define GTK_FILE_FOLDER_GNOME_VFS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_FILE_FOLDER_GNOME_VFS, GtkFileFolderGnomeVFSClass))
#define GTK_IS_FILE_FOLDER_GNOME_VFS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_FILE_FOLDER_GNOME_VFS))
#define GTK_FILE_FOLDER_GNOME_VFS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_FILE_FOLDER_GNOME_VFS, GtkFileFolderGnomeVFSClass))

typedef struct _GtkFileFolderGnomeVFS      GtkFileFolderGnomeVFS;
typedef struct _GtkFileFolderGnomeVFSClass GtkFileFolderGnomeVFSClass;

struct _GtkFileFolderGnomeVFSClass
{
  GObjectClass parent_class;
};

struct _GtkFileFolderGnomeVFS
{
  GObject parent_instance;

  GtkFileInfoType types;
  gchar *uri; /* canonical */

  GnomeVFSAsyncHandle *async_handle;
  GnomeVFSMonitorHandle *monitor;

  GtkFileSystemGnomeVFS *system;

  GHashTable *children;
};

typedef struct _FolderChild FolderChild;

struct _FolderChild
{
  gchar *uri; /* canonical */
  GnomeVFSFileInfo *info;
};

#ifdef USE_GCONF
/* GConf paths for the bookmarks; keep these two in sync */
#define BOOKMARKS_KEY  "/desktop/gnome/interface/bookmark_folders"
#define BOOKMARKS_PATH "/desktop/gnome/interface"
#endif

static void gtk_file_system_gnome_vfs_class_init (GtkFileSystemGnomeVFSClass *class);
static void gtk_file_system_gnome_vfs_iface_init (GtkFileSystemIface         *iface);
static void gtk_file_system_gnome_vfs_init       (GtkFileSystemGnomeVFS      *impl);
static void gtk_file_system_gnome_vfs_finalize   (GObject                    *object);

static GSList *             gtk_file_system_gnome_vfs_list_volumes        (GtkFileSystem     *file_system);
static GtkFileSystemVolume *gtk_file_system_gnome_vfs_get_volume_for_path (GtkFileSystem     *file_system,
									   const GtkFilePath *path);

static GtkFileFolder *gtk_file_system_gnome_vfs_get_folder    (GtkFileSystem      *file_system,
							       const GtkFilePath  *path,
							       GtkFileInfoType     types,
							       GError            **error);
static gboolean       gtk_file_system_gnome_vfs_create_folder (GtkFileSystem      *file_system,
							       const GtkFilePath  *path,
							       GError            **error);

static void         gtk_file_system_gnome_vfs_volume_free             (GtkFileSystem       *file_system,
								       GtkFileSystemVolume *volume);
static GtkFilePath *gtk_file_system_gnome_vfs_volume_get_base_path    (GtkFileSystem       *file_system,
								       GtkFileSystemVolume *volume);
static gboolean     gtk_file_system_gnome_vfs_volume_get_is_mounted   (GtkFileSystem       *file_system,
								       GtkFileSystemVolume *volume);
static gboolean     gtk_file_system_gnome_vfs_volume_mount            (GtkFileSystem       *file_system,
								       GtkFileSystemVolume *volume,
								       GError             **error);
static gchar *      gtk_file_system_gnome_vfs_volume_get_display_name (GtkFileSystem       *file_system,
								       GtkFileSystemVolume *volume);
static GdkPixbuf *  gtk_file_system_gnome_vfs_volume_render_icon      (GtkFileSystem        *file_system,
								       GtkFileSystemVolume  *volume,
								       GtkWidget            *widget,
								       gint                  pixel_size,
								       GError              **error);

static gboolean       gtk_file_system_gnome_vfs_get_parent    (GtkFileSystem      *file_system,
							       const GtkFilePath  *path,
							       GtkFilePath       **parent,
							       GError            **error);
static GtkFilePath *  gtk_file_system_gnome_vfs_make_path     (GtkFileSystem      *file_system,
							       const GtkFilePath  *base_path,
							       const gchar        *display_name,
							       GError            **error);
static gboolean       gtk_file_system_gnome_vfs_parse         (GtkFileSystem      *file_system,
							       const GtkFilePath  *base_path,
							       const gchar        *str,
							       GtkFilePath       **folder,
							       gchar             **file_part,
							       GError            **error);

static gchar *      gtk_file_system_gnome_vfs_path_to_uri      (GtkFileSystem     *file_system,
								const GtkFilePath *path);
static gchar *      gtk_file_system_gnome_vfs_path_to_filename (GtkFileSystem     *file_system,
								const GtkFilePath *path);
static GtkFilePath *gtk_file_system_gnome_vfs_uri_to_path      (GtkFileSystem     *file_system,
								const gchar       *uri);
static GtkFilePath *gtk_file_system_gnome_vfs_filename_to_path (GtkFileSystem     *file_system,
								const gchar       *filename);

static GdkPixbuf *gtk_file_system_gnome_vfs_render_icon (GtkFileSystem     *file_system,
							 const GtkFilePath *path,
							 GtkWidget         *widget,
							 gint               pixel_size,
							 GError           **error);

static gboolean gtk_file_system_gnome_vfs_add_bookmark    (GtkFileSystem     *file_system,
							   const GtkFilePath *path,
							   GError           **error);
static gboolean gtk_file_system_gnome_vfs_remove_bookmark (GtkFileSystem     *file_system,
							   const GtkFilePath *path,
							   GError           **error);
static GSList * gtk_file_system_gnome_vfs_list_bookmarks  (GtkFileSystem *file_system);

static GType gtk_file_folder_gnome_vfs_get_type   (void);
static void  gtk_file_folder_gnome_vfs_class_init (GtkFileFolderGnomeVFSClass *class);
static void  gtk_file_folder_gnome_vfs_iface_init (GtkFileFolderIface         *iface);
static void  gtk_file_folder_gnome_vfs_init       (GtkFileFolderGnomeVFS      *impl);
static void  gtk_file_folder_gnome_vfs_finalize   (GObject                    *object);

static GtkFileInfo *gtk_file_folder_gnome_vfs_get_info      (GtkFileFolder      *folder,
							     const GtkFilePath  *path,
							     GError            **error);
static gboolean     gtk_file_folder_gnome_vfs_list_children (GtkFileFolder      *folder,
							     GSList            **children,
							     GError            **error);

static GtkFileInfo *           info_from_vfs_info (const gchar      *uri,
						   GnomeVFSFileInfo *vfs_info,
						   GtkFileInfoType   types);
static GnomeVFSFileInfoOptions get_options        (GtkFileInfoType   types);

static void volume_mount_unmount_cb (GnomeVFSVolumeMonitor *monitor,
				     GnomeVFSVolume        *volume,
				     GtkFileSystemGnomeVFS *system_vfs);
static void drive_connect_disconnect_cb (GnomeVFSVolumeMonitor *monitor,
					 GnomeVFSDrive         *drive,
					 GtkFileSystemGnomeVFS *system_vfs);

static void directory_load_callback (GnomeVFSAsyncHandle      *handle,
				     GnomeVFSResult            result,
				     GList                    *list,
				     guint                     entries_read,
				     gpointer                  user_data);
static void monitor_callback        (GnomeVFSMonitorHandle    *handle,
				     const gchar              *monitor_uri,
				     const gchar              *info_uri,
				     GnomeVFSMonitorEventType  event_type,
				     gpointer                  user_data);

#ifdef USE_GCONF
static void set_bookmarks_from_value (GtkFileSystemGnomeVFS *system_vfs,
				      GConfValue            *value,
				      gboolean               emit_changed);

static void client_notify_cb (GConfClient *client,
			      guint        cnxn_id,
			      GConfEntry  *entry,
			      gpointer     data);
#endif

static FolderChild *folder_child_new (const char       *uri,
				      GnomeVFSFileInfo *info);
static void         folder_child_free (FolderChild *child);

static void     set_vfs_error     (GnomeVFSResult   result,
				   const gchar     *uri,
				   GError         **error);
static gboolean has_valid_scheme  (const char      *uri);
static GnomeVFSFileInfo * lookup_vfs_info_in_folder (GtkFileFolder     *folder,
						     const GtkFilePath *path,
						     GError           **error);

static GObjectClass *system_parent_class;
static GObjectClass *folder_parent_class;

#define ITEMS_PER_NOTIFICATION 100

/*
 * GtkFileSystemGnomeVFS
 */
GType
gtk_file_system_gnome_vfs_get_type (void)
{
  return type_gtk_file_system_gnome_vfs;
}
/**
 * gtk_file_system_gnome_vfs_new:
 *
 * Creates a new #GtkFileSystemGnomeVFS object. #GtkFileSystemGnomeVFS
 * implements the #GtkFileSystem interface using the GNOME-VFS
 * library.
 *
 * Return value: the new #GtkFileSystemGnomeVFS object
 **/
GtkFileSystem *
gtk_file_system_gnome_vfs_new (void)
{
  GtkFileSystemGnomeVFS *system_vfs;

  system_vfs = g_object_new (GTK_TYPE_FILE_SYSTEM_GNOME_VFS, NULL);

  return GTK_FILE_SYSTEM (system_vfs);
}

static void
gtk_file_system_gnome_vfs_class_init (GtkFileSystemGnomeVFSClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  system_parent_class = g_type_class_peek_parent (class);

  gobject_class->finalize = gtk_file_system_gnome_vfs_finalize;
}

static void
gtk_file_system_gnome_vfs_iface_init (GtkFileSystemIface *iface)
{
  iface->list_volumes = gtk_file_system_gnome_vfs_list_volumes;
  iface->get_volume_for_path = gtk_file_system_gnome_vfs_get_volume_for_path;
  iface->get_folder = gtk_file_system_gnome_vfs_get_folder;
  iface->create_folder = gtk_file_system_gnome_vfs_create_folder;
  iface->volume_free = gtk_file_system_gnome_vfs_volume_free;
  iface->volume_get_base_path = gtk_file_system_gnome_vfs_volume_get_base_path;
  iface->volume_get_is_mounted = gtk_file_system_gnome_vfs_volume_get_is_mounted;
  iface->volume_mount = gtk_file_system_gnome_vfs_volume_mount;
  iface->volume_get_display_name = gtk_file_system_gnome_vfs_volume_get_display_name;
  iface->volume_render_icon = gtk_file_system_gnome_vfs_volume_render_icon;
  iface->get_parent = gtk_file_system_gnome_vfs_get_parent;
  iface->make_path = gtk_file_system_gnome_vfs_make_path;
  iface->parse = gtk_file_system_gnome_vfs_parse;
  iface->path_to_uri = gtk_file_system_gnome_vfs_path_to_uri;
  iface->path_to_filename = gtk_file_system_gnome_vfs_path_to_filename;
  iface->uri_to_path = gtk_file_system_gnome_vfs_uri_to_path;
  iface->filename_to_path = gtk_file_system_gnome_vfs_filename_to_path;
  iface->render_icon = gtk_file_system_gnome_vfs_render_icon;
  iface->add_bookmark = gtk_file_system_gnome_vfs_add_bookmark;
  iface->remove_bookmark = gtk_file_system_gnome_vfs_remove_bookmark;
  iface->list_bookmarks = gtk_file_system_gnome_vfs_list_bookmarks;
}

static void
gtk_file_system_gnome_vfs_init (GtkFileSystemGnomeVFS *system_vfs)
{
  char *name;
#ifdef USE_GCONF
  GConfValue *value;
#endif


  name = g_build_filename (g_get_home_dir (), "Desktop", NULL);
  system_vfs->desktop_uri = (char *)gtk_file_system_filename_to_path (GTK_FILE_SYSTEM (system_vfs),
								      name);
  g_free (name);
  system_vfs->locale_encoded_filenames = (getenv ("G_BROKEN_FILENAMES") != NULL);
  system_vfs->folders = g_hash_table_new (g_str_hash, g_str_equal);

#ifdef USE_GCONF
  system_vfs->client = gconf_client_get_default ();
  system_vfs->bookmarks = NULL;

  value = gconf_client_get (system_vfs->client, BOOKMARKS_KEY, NULL);
  set_bookmarks_from_value (system_vfs, value, FALSE);
  if (value)
    gconf_value_free (value);

  /* FIXME: use GError? */
  gconf_client_add_dir (system_vfs->client, BOOKMARKS_PATH, GCONF_CLIENT_PRELOAD_NONE, NULL);

  system_vfs->client_notify_id = gconf_client_notify_add (system_vfs->client,
							  BOOKMARKS_KEY,
							  client_notify_cb,
							  system_vfs,
							  NULL,
							  NULL);
#endif

  system_vfs->volume_monitor = gnome_vfs_volume_monitor_ref (gnome_vfs_get_volume_monitor ());
  g_signal_connect (system_vfs->volume_monitor, "volume-mounted",
		    G_CALLBACK (volume_mount_unmount_cb), system_vfs);
  g_signal_connect (system_vfs->volume_monitor, "volume-unmounted",
		    G_CALLBACK (volume_mount_unmount_cb), system_vfs);
  g_signal_connect (system_vfs->volume_monitor, "drive-connected",
		    G_CALLBACK (drive_connect_disconnect_cb), system_vfs);
  g_signal_connect (system_vfs->volume_monitor, "drive-disconnected",
		    G_CALLBACK (drive_connect_disconnect_cb), system_vfs);

}

static void
gtk_file_system_gnome_vfs_finalize (GObject *object)
{
  GtkFileSystemGnomeVFS *system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (object);

  g_free (system_vfs->desktop_uri);
  g_hash_table_destroy (system_vfs->folders);

#ifdef USE_GCONF
  gconf_client_notify_remove (system_vfs->client, system_vfs->client_notify_id);
  g_object_unref (system_vfs->client);
  gtk_file_paths_free (system_vfs->bookmarks);
#endif

  gnome_vfs_volume_monitor_unref (system_vfs->volume_monitor);

  /* XXX Assert ->folders should be empty
   */
  system_parent_class->finalize (object);
}

static GSList *
gtk_file_system_gnome_vfs_list_volumes (GtkFileSystem *file_system)
{
  GtkFileSystemGnomeVFS *system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);
  GnomeVFSVolume *volume;
  GnomeVFSDrive *drive;
  GSList *result;
  GList *list;
  GList *l;

  result = NULL;

  /* User-visible drives */

  list = gnome_vfs_volume_monitor_get_connected_drives (system_vfs->volume_monitor);
  for (l = list; l; l = l->next)
    {
      drive = GNOME_VFS_DRIVE (l->data);

      if (gnome_vfs_drive_is_user_visible (drive))
	result = g_slist_prepend (result, drive);
      else
	gnome_vfs_drive_unref (drive);
    }

  g_list_free (list);

  /* User-visible volumes with no corresponding drives */

  list = gnome_vfs_volume_monitor_get_mounted_volumes (system_vfs->volume_monitor);
  for (l = list; l; l = l->next)
    {
      volume = GNOME_VFS_VOLUME (l->data);
      drive = gnome_vfs_volume_get_drive (volume);

      if (!drive && gnome_vfs_volume_is_user_visible (volume))
	{
	  gnome_vfs_drive_unref (drive);
	  result = g_slist_prepend (result, volume);
	}
      else
	{
	  gnome_vfs_drive_unref (drive);
	  gnome_vfs_volume_unref (volume);
	}
    }

  g_list_free (list);

  /* Done */

  result = g_slist_reverse (result);

  /* Add root, first in the list */
  volume = gnome_vfs_volume_monitor_get_volume_for_path (system_vfs->volume_monitor, "/");
  result = g_slist_prepend (result, volume);
  
  return result;
}

static GtkFileSystemVolume *
gtk_file_system_gnome_vfs_get_volume_for_path (GtkFileSystem     *file_system,
					       const GtkFilePath *path)
{
  GtkFileSystemGnomeVFS *system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);
  GnomeVFSURI *uri;
  GnomeVFSVolume *volume;

  uri = gnome_vfs_uri_new (gtk_file_path_get_string (path));
  if (!uri)
    return NULL;

  volume = NULL;

  if (strcmp (uri->method_string, "file") == 0) 
    while (uri)
      {
	const char *local_path;
	GnomeVFSURI *parent;
	
	local_path = gnome_vfs_uri_get_path (uri);
	volume = gnome_vfs_volume_monitor_get_volume_for_path (system_vfs->volume_monitor, local_path);
	
	if (volume == NULL)
	  break;
	
	if (gnome_vfs_volume_is_user_visible (volume))
	  break;
	
	parent = gnome_vfs_uri_get_parent (uri);
	gnome_vfs_uri_unref (uri);
	uri = parent;
      }
  
  if (uri)
    gnome_vfs_uri_unref (uri);

  return (GtkFileSystemVolume *) volume;
}


static gboolean
remove_all  (gpointer  key,
	     gpointer  value,
	     gpointer  user_data)
{
  return TRUE;
}

static void
load_dir (GtkFileFolderGnomeVFS *folder_vfs)
{
  if (folder_vfs->async_handle)
    gnome_vfs_async_cancel (folder_vfs->async_handle);

  g_hash_table_foreach_remove (folder_vfs->children,
			       remove_all, NULL);
  gnome_vfs_async_load_directory (&folder_vfs->async_handle,
				  folder_vfs->uri,
				  get_options (folder_vfs->types),
				  ITEMS_PER_NOTIFICATION,
				  GNOME_VFS_PRIORITY_DEFAULT,
				  directory_load_callback, folder_vfs);
}

static char *
make_uri_canonical (const char *uri)
{
  char *canonical;
  int len;
  
  canonical = gnome_vfs_make_uri_canonical (uri);

  /* Strip terminating / unless its foo:/ or foo:/// */
  len = strlen (canonical);
  if (len > 2 &&
      canonical[len-1] == '/' &&
      canonical[len-2] != '/' &&
      canonical[len-2] != ':')
    {
      canonical[len-1] = 0;
    }

  return canonical;
}


static GtkFileFolder *
gtk_file_system_gnome_vfs_get_folder (GtkFileSystem     *file_system,
				      const GtkFilePath *path,
				      GtkFileInfoType    types,
				      GError           **error)
{
  char *uri;
  GtkFilePath *parent_path;
  GtkFileSystemGnomeVFS *system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);
  GtkFileFolderGnomeVFS *folder_vfs;
  GnomeVFSMonitorHandle *monitor = NULL;
  GnomeVFSResult result;

  uri = make_uri_canonical (gtk_file_path_get_string (path));
  folder_vfs = g_hash_table_lookup (system_vfs->folders, uri);
  if (folder_vfs)
    {
      folder_vfs->types |= types;
      g_free (uri);
      return g_object_ref (folder_vfs);
    }

  if (!gtk_file_system_get_parent (file_system, path, &parent_path, error))
    {
      g_free (uri);
      return NULL;
    }

  folder_vfs = g_object_new (GTK_TYPE_FILE_FOLDER_GNOME_VFS, NULL);

  result = gnome_vfs_monitor_add (&monitor,
				  uri,
				  GNOME_VFS_MONITOR_DIRECTORY,
				  monitor_callback, folder_vfs);
  if (result != GNOME_VFS_OK && result != GNOME_VFS_ERROR_NOT_SUPPORTED)
    {
      g_free (uri);
      g_object_unref (folder_vfs);
      set_vfs_error (result, uri, error);
      return NULL;
    }
  folder_vfs->system = system_vfs;
  folder_vfs->uri = uri; /* takes ownership */
  folder_vfs->types = types;
  folder_vfs->monitor = monitor;
  folder_vfs->async_handle = NULL;
  folder_vfs->children = g_hash_table_new_full (g_str_hash,
						g_str_equal,
						NULL,
						(GDestroyNotify) folder_child_free);

  g_hash_table_insert (system_vfs->folders, folder_vfs->uri, folder_vfs);

  /* Make sure we exist as a child in the parent folder (if its loaded) */
  if (parent_path)
    {
      char *parent_uri;
      GtkFileFolderGnomeVFS *parent_folder;
      GSList *uris;

      parent_uri = make_uri_canonical (gtk_file_path_get_string (parent_path));
      parent_folder = g_hash_table_lookup (system_vfs->folders, parent_uri);
      g_free (parent_uri);
      gtk_file_path_free (parent_path);

      if (parent_folder &&
	  !g_hash_table_lookup (parent_folder->children, folder_vfs->uri))
	{
	  GnomeVFSFileInfo *vfs_info;
	  FolderChild *child;

	  vfs_info = gnome_vfs_file_info_new ();
	  result = gnome_vfs_get_file_info (folder_vfs->uri,
					    vfs_info,
					    get_options (parent_folder->types));
	  if (result != GNOME_VFS_OK)
	    {
	      gnome_vfs_file_info_unref (vfs_info);
	      g_object_unref (folder_vfs);
	      set_vfs_error (result, folder_vfs->uri, error);
	      return NULL;
	    }

	  child = folder_child_new (folder_vfs->uri, vfs_info);
	  gnome_vfs_file_info_unref (vfs_info);

	  g_hash_table_replace (parent_folder->children,
				child->uri, child);

	  uris = g_slist_prepend (NULL, child->uri);
	  g_signal_emit_by_name (parent_folder, "files-added", uris);
	  g_slist_free (uris);
	}
    }

  return GTK_FILE_FOLDER (folder_vfs);
}

static gboolean
gtk_file_system_gnome_vfs_create_folder (GtkFileSystem     *file_system,
					 const GtkFilePath *path,
					 GError           **error)
{
  const gchar *uri = gtk_file_path_get_string (path);
  GnomeVFSResult result;

  result = gnome_vfs_make_directory (uri,
				     GNOME_VFS_PERM_USER_ALL |
				     GNOME_VFS_PERM_GROUP_ALL |
				     GNOME_VFS_PERM_OTHER_READ);

  if (result != GNOME_VFS_OK)
    {
      set_vfs_error (result, uri, error);
      return FALSE;
    }

  return TRUE;
}

static void
gtk_file_system_gnome_vfs_volume_free (GtkFileSystem        *file_system,
				       GtkFileSystemVolume  *volume)
{
  if (GNOME_IS_VFS_DRIVE (volume))
    gnome_vfs_drive_unref (GNOME_VFS_DRIVE (volume));
  else if (GNOME_IS_VFS_VOLUME (volume))
    gnome_vfs_volume_unref (GNOME_VFS_VOLUME (volume));
  else
    g_warning ("%p is not a valid volume", volume);
}

static GtkFilePath *
gtk_file_system_gnome_vfs_volume_get_base_path (GtkFileSystem        *file_system,
						GtkFileSystemVolume  *volume)
{
  char *uri;

  if (GNOME_IS_VFS_DRIVE (volume))
    uri = gnome_vfs_drive_get_activation_uri (GNOME_VFS_DRIVE (volume));
  else if (GNOME_IS_VFS_VOLUME (volume))
    uri = gnome_vfs_volume_get_activation_uri (GNOME_VFS_VOLUME (volume));
  else
    {
      g_warning ("%p is not a valid volume", volume);
      return NULL;
    }

  return gtk_file_path_new_steal (uri);
}

static gboolean
gtk_file_system_gnome_vfs_volume_get_is_mounted (GtkFileSystem        *file_system,
						 GtkFileSystemVolume  *volume)
{
  if (GNOME_IS_VFS_DRIVE (volume))
    return gnome_vfs_drive_is_mounted (GNOME_VFS_DRIVE (volume));
  else if (GNOME_IS_VFS_VOLUME (volume))
    return gnome_vfs_volume_is_mounted (GNOME_VFS_VOLUME (volume));
  else
    {
      g_warning ("%p is not a valid volume", volume);
      return FALSE;
    }
}

struct mount_closure {
  GtkFileSystemGnomeVFS *system_vfs;
  GMainLoop *loop;
  gboolean succeeded;
  char *error;
  char *detailed_error;
};

/* Used from gnome_vfs_{volume,drive}_mount() */
static void
volume_mount_cb (gboolean succeeded,
		 char    *error,
		 char    *detailed_error,
		 gpointer data)
{
  struct mount_closure *closure;

  closure = data;

  closure->succeeded = succeeded;

  if (!succeeded)
    {
      closure->error = g_strdup (error);
      closure->detailed_error = g_strdup (detailed_error);
    }

  g_main_loop_quit (closure->loop);
}

static gboolean
gtk_file_system_gnome_vfs_volume_mount (GtkFileSystem        *file_system,
					GtkFileSystemVolume  *volume,
					GError              **error)
{
  GtkFileSystemGnomeVFS *system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);

  if (GNOME_IS_VFS_DRIVE (volume))
    {
      struct mount_closure closure;

      closure.system_vfs = system_vfs;
      closure.loop = g_main_loop_new (NULL, FALSE);
      gnome_vfs_drive_mount (GNOME_VFS_DRIVE (volume), volume_mount_cb, &closure);

      GDK_THREADS_LEAVE ();
      g_main_loop_run (closure.loop);
      GDK_THREADS_ENTER ();
      g_main_loop_unref (closure.loop);

      if (closure.succeeded)
	return TRUE;
      else
	{
	  g_set_error (error,
		       GTK_FILE_SYSTEM_ERROR,
		       GTK_FILE_SYSTEM_ERROR_FAILED,
		       "%s:\n%s",
		       closure.error,
		       closure.detailed_error);
	  g_free (closure.error);
	  g_free (closure.detailed_error);
	  return FALSE;
	}
    }
  else if (GNOME_IS_VFS_VOLUME (volume))
    return TRUE; /* It is already mounted */
  else
    {
      g_warning ("%p is not a valid volume", volume);
      return FALSE;
    }
}

static gchar *
gtk_file_system_gnome_vfs_volume_get_display_name (GtkFileSystem       *file_system,
						   GtkFileSystemVolume *volume)
{
  char *display_name, *uri;
  GnomeVFSVolume *mounted_volume;

  display_name = NULL;
  if (GNOME_IS_VFS_DRIVE (volume))
    {
      mounted_volume = gnome_vfs_drive_get_mounted_volume (GNOME_VFS_DRIVE (volume));
      if (mounted_volume)
	{
	  display_name = gnome_vfs_volume_get_display_name (mounted_volume);
	  gnome_vfs_volume_unref (mounted_volume);
	}
      else
	display_name = gnome_vfs_drive_get_display_name (GNOME_VFS_DRIVE (volume));
    }
  else if (GNOME_IS_VFS_VOLUME (volume))
    {
      uri = gnome_vfs_volume_get_activation_uri (GNOME_VFS_VOLUME (volume));
      if (strcmp (uri, "file:///") == 0)
	display_name = g_strdup (_("Filesystem"));
      else
	display_name = gnome_vfs_volume_get_display_name (GNOME_VFS_VOLUME (volume));
      g_free (uri);
    }
  else
    g_warning ("%p is not a valid volume", volume);

  return display_name;
}

typedef struct
{
  gint size;
  GdkPixbuf *pixbuf;
} IconCacheElement;

static void
icon_cache_element_free (IconCacheElement *element)
{
  if (element->pixbuf)
    g_object_unref (element->pixbuf);
  g_free (element);
}

static void
icon_theme_changed (GtkIconTheme *icon_theme)
{
  GHashTable *cache;

  /* Difference from the initial creation is that we don't
   * reconnect the signal
   */
  cache = g_hash_table_new_full (g_str_hash, g_str_equal,
				 (GDestroyNotify)g_free,
				 (GDestroyNotify)icon_cache_element_free);
  g_object_set_data_full (G_OBJECT (icon_theme), "gnome-vfs-gtk-file-icon-cache",
			  cache, (GDestroyNotify)g_hash_table_destroy);
}

static GdkPixbuf *
get_cached_icon (GtkWidget   *widget,
		 const gchar *name,
		 gint         pixel_size)
{
  GtkIconTheme *icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (widget));
  GHashTable *cache = g_object_get_data (G_OBJECT (icon_theme), "gnome-vfs-gtk-file-icon-cache");
  IconCacheElement *element;

  if (!cache)
    {
      cache = g_hash_table_new_full (g_str_hash, g_str_equal,
				     (GDestroyNotify)g_free,
				     (GDestroyNotify)icon_cache_element_free);

      g_object_set_data_full (G_OBJECT (icon_theme), "gnome-vfs-gtk-file-icon-cache",
			      cache, (GDestroyNotify)g_hash_table_destroy);
      g_signal_connect (icon_theme, "changed",
			G_CALLBACK (icon_theme_changed), NULL);
    }

  element = g_hash_table_lookup (cache, name);
  if (!element)
    {
      element = g_new0 (IconCacheElement, 1);
      g_hash_table_insert (cache, g_strdup (name), element);
    }

  if (element->size != pixel_size)
    {
      if (element->pixbuf)
	g_object_unref (element->pixbuf);
      element->size = pixel_size;
      element->pixbuf = gtk_icon_theme_load_icon (icon_theme, name,
						  pixel_size, 0, NULL);
    }

  return element->pixbuf ? g_object_ref (element->pixbuf) : NULL;
}

static GdkPixbuf *
gtk_file_system_gnome_vfs_volume_render_icon (GtkFileSystem        *file_system,
					      GtkFileSystemVolume  *volume,
					      GtkWidget            *widget,
					      gint                  pixel_size,
					      GError              **error)
{
  GtkFileSystemGnomeVFS *system_vfs;
  char *icon_name, *uri;
  GdkPixbuf *pixbuf;
  GnomeVFSVolume *mounted_volume;

  system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);

  icon_name = NULL;
  if (GNOME_IS_VFS_DRIVE (volume))
    {
      mounted_volume = gnome_vfs_drive_get_mounted_volume (GNOME_VFS_DRIVE (volume));
      if (mounted_volume)
	{
	  icon_name = gnome_vfs_volume_get_icon (mounted_volume);
	  gnome_vfs_volume_unref (mounted_volume);
	}
      else
	icon_name = gnome_vfs_drive_get_icon (GNOME_VFS_DRIVE (volume));
    }
  else if (GNOME_IS_VFS_VOLUME (volume))
    {
      uri = gnome_vfs_volume_get_activation_uri (GNOME_VFS_VOLUME (volume));
      if (strcmp (uri, "file:///") == 0)
	icon_name = g_strdup ("gnome-fs-blockdev");
      else if (strcmp (uri, system_vfs->desktop_uri) == 0)
	icon_name = g_strdup ("gnome-fs-desktop");
      else
	icon_name = gnome_vfs_volume_get_icon (GNOME_VFS_VOLUME (volume));
      g_free (uri);
    }
  else
    g_warning ("%p is not a valid volume", volume);

  if (icon_name)
    {
      pixbuf = get_cached_icon (widget, icon_name, pixel_size);
      g_free (icon_name);
    }
  else
    pixbuf = NULL;

  return pixbuf;
}

static gboolean
gtk_file_system_gnome_vfs_get_parent (GtkFileSystem     *file_system,
				      const GtkFilePath *path,
				      GtkFilePath      **parent,
				      GError           **error)
{
  GnomeVFSURI *uri = gnome_vfs_uri_new (gtk_file_path_get_string (path));
  GnomeVFSURI *parent_uri;

  if (uri == NULL)
    {
      set_vfs_error (GNOME_VFS_ERROR_INVALID_URI, gtk_file_path_get_string (path), error);
      return FALSE;
    }
  
  parent_uri = gnome_vfs_uri_get_parent (uri);
  if (parent_uri)
    {
      *parent = gtk_file_path_new_steal (gnome_vfs_uri_to_string (parent_uri, 0));
      gnome_vfs_uri_unref (parent_uri);
    }
  else
    *parent = NULL;

  gnome_vfs_uri_unref (uri);

  return TRUE;
}

static gchar *
make_child_uri (const    gchar *base_uri,
		const    gchar *child_name,
		GError **error)
{
  GnomeVFSURI *uri = gnome_vfs_uri_new (base_uri);
  GnomeVFSURI *child_uri;
  gchar *result;

  /* TODO: Can't this happen due to invalid input from the user? */
  g_return_val_if_fail (uri != NULL, NULL);

  child_uri = gnome_vfs_uri_append_file_name (uri, child_name);

  result = gnome_vfs_uri_to_string (child_uri, 0);

  gnome_vfs_uri_unref (uri);
  gnome_vfs_uri_unref (child_uri);

  return result;
}

static GtkFilePath *
gtk_file_system_gnome_vfs_make_path (GtkFileSystem     *file_system,
				     const GtkFilePath *base_path,
				     const gchar       *display_name,
				     GError           **error)
{
  gchar *child_name;
  gchar *result;
  GError *tmp_error = NULL;

  child_name = g_filename_from_utf8 (display_name, -1, NULL, NULL, &tmp_error);

  if (!child_name)
    {
      g_set_error (error,
		   GTK_FILE_SYSTEM_ERROR,
		   GTK_FILE_SYSTEM_ERROR_BAD_FILENAME,
		   "%s",
		   tmp_error->message);
      g_error_free (tmp_error);

      return NULL;
    }

  result = make_child_uri (gtk_file_path_get_string (base_path),
			   child_name, error);

  g_free (child_name);

  return gtk_file_path_new_steal (result);
}

static gchar *
path_from_input (GtkFileSystemGnomeVFS *system_vfs,
		 const gchar           *str,
		 GError               **error)
{
  if (system_vfs->locale_encoded_filenames)
    {
      GError *tmp_error = NULL;
      gchar *path = g_locale_from_utf8 (str, -1, NULL, NULL, &tmp_error);
      if (!path)
	{
	  g_set_error (error,
		       GTK_FILE_SYSTEM_ERROR,
		       GTK_FILE_SYSTEM_ERROR_BAD_FILENAME,
		       "%s",
		       tmp_error->message);
	  g_error_free (tmp_error);

	  return NULL;
	}

      return path;
    }
  else
    return g_strdup (str);
}

/* Very roughly based on eel_make_uri_from_input()
 */
static gboolean
gtk_file_system_gnome_vfs_parse (GtkFileSystem     *file_system,
				 const GtkFilePath *base_path,
				 const gchar       *str,
				 GtkFilePath      **folder,
				 gchar            **file_part,
				 GError           **error)
{
  GtkFileSystemGnomeVFS *system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);
  const gchar *base_uri = gtk_file_path_get_string (base_path);
  gchar *stripped;
  gchar *last_slash;
  gboolean result = FALSE;

  /* Strip off leading whitespaces (since they can't be part of a valid
   * uri). Strip off trailing whitespaces too because filenames with
   * trailing whitespace are crack (We want to avoid letting the
   * user accidentally create such filenames for the save dialog.)
   */
  stripped = g_strstrip (g_strdup (str));

  last_slash = strrchr (stripped, '/');
  if (!last_slash)
    {
      *folder = gtk_file_path_copy (base_path);
      *file_part = g_strdup (stripped);
      result = TRUE;
    }
  else if (has_valid_scheme (stripped))
    {
      gchar *scheme;
      gchar *host_name;
      gchar *path;
      gchar *file;
      gchar *host_and_path;
      gchar *host_and_path_escaped;

      gchar *colon = strchr (stripped, ':');

      scheme = g_strndup (stripped, colon + 1 - stripped);

      if (*(colon + 1) != '/' ||                         /* http:www.gnome.org/asdf */
	  (*(colon + 1) == '/' && *(colon + 2) != '/'))  /* file:/asdf */
	{
	  gchar *first_slash = strchr (colon + 1, '/');

	  host_name = g_strndup (colon + 1, first_slash - (colon + 1));

	  if (first_slash == last_slash)
	    path = g_strdup ("/");
	  else
	    path = g_strndup (first_slash, last_slash - first_slash);

	  file = g_strdup (last_slash + 1);
	}
      else if (*(colon + 1) == '/' &&
	       *(colon + 2) == '/')
	{
	  /* http://www.gnome.org
	   * http://www.gnome.org/foo/bar
	   */

	  gchar *first_slash = strchr (colon + 3, '/');
	  if (!first_slash)
	    {
	      host_name = g_strdup (colon + 3);
	      path = g_strdup ("/");
	      file = g_strdup ("");
	    }
	  else
	    {
	      host_name = g_strndup (colon + 3, first_slash - (colon + 3));

	      if (first_slash == last_slash)
		path = g_strdup ("/");
	      else
		path = g_strndup (first_slash, last_slash - first_slash);

	      file = g_strdup (last_slash + 1);
	    }
	}

      host_and_path = g_strconcat (host_name, path, NULL);
      host_and_path_escaped = gnome_vfs_escape_host_and_path_string (host_and_path);
      *folder = gtk_file_path_new_steal (g_strconcat (scheme, "//", host_and_path_escaped, NULL));
      *file_part = file;
      result = TRUE;

      g_free (scheme);
      g_free (host_name);
      g_free (path);
      g_free (host_and_path);
      g_free (host_and_path_escaped);

      result = TRUE;
    }
  else
    {
      gchar *path_part, *path, *uri, *filesystem_path, *escaped, *base_dir;

      if (last_slash == stripped)
	path_part = g_strdup ("/");
      else
	path_part = g_strndup (stripped, last_slash - stripped);

      uri = NULL;

      filesystem_path = path_from_input (system_vfs, path_part, error);
      g_free (path_part);

      if (filesystem_path)
	{
	  switch (filesystem_path[0])
	    {
	    case '/':
	      uri = gnome_vfs_get_uri_from_local_path (filesystem_path);
	      break;
	    case '~':
	      /* deliberately falling into default case on fail */
	      path = gnome_vfs_expand_initial_tilde (filesystem_path);
	      if (*path == '/')
		{
		  uri = gnome_vfs_get_uri_from_local_path (path);
		  g_free (path);
		  break;
		}
	      g_free (path);
	      /* don't insert break here, read above comment */
	    default:
	      escaped = gnome_vfs_escape_path_string (filesystem_path);
	      base_dir = g_strconcat (base_uri, "/", NULL);
	      uri = gnome_vfs_uri_make_full_from_relative (base_dir, escaped);
	      g_free (base_dir);

	      g_free (escaped);
	    }

	  g_free (filesystem_path);
	}

      if (uri)
	{
	  *file_part = g_strdup (last_slash + 1);
	  *folder = gtk_file_path_new_steal (uri);

	  result = TRUE;
	}

    }

  g_free (stripped);

  return result;
}

static gchar *
gtk_file_system_gnome_vfs_path_to_uri (GtkFileSystem     *file_system,
				       const GtkFilePath *path)
{
  return g_strdup (gtk_file_path_get_string (path));
}

static gchar *
gtk_file_system_gnome_vfs_path_to_filename (GtkFileSystem     *file_system,
					    const GtkFilePath *path)
{
  const gchar *uri = gtk_file_path_get_string (path);

  return gnome_vfs_get_local_path_from_uri (uri);
}

static GtkFilePath *
gtk_file_system_gnome_vfs_uri_to_path (GtkFileSystem *file_system,
				       const gchar   *uri)
{
  return gtk_file_path_new_dup (uri);
}

static GtkFilePath *
gtk_file_system_gnome_vfs_filename_to_path (GtkFileSystem *file_system,
					    const gchar   *filename)
{
  gchar *uri = gnome_vfs_get_uri_from_local_path (filename);
  if (uri)
    return gtk_file_path_new_steal (uri);
  else
    return NULL;
}


static GnomeVFSFileInfo *
get_vfs_info (GtkFileSystem     *file_system,
	      const GtkFilePath *path,
	      GtkFileInfoType    types)
{
  const char *parent_uri, *uri;
  char *canonical_uri;
  GtkFilePath *parent_path;
  GtkFileFolderGnomeVFS *folder_vfs;
  GnomeVFSFileInfo *info;
  GtkFileSystemGnomeVFS *system_vfs;

  system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);

  if (!gtk_file_system_get_parent (file_system, path, &parent_path, NULL))
    return NULL;
  
  parent_uri = gtk_file_path_get_string (parent_path);
  canonical_uri = make_uri_canonical (parent_uri);
  folder_vfs = g_hash_table_lookup (system_vfs->folders, canonical_uri);
  g_free (canonical_uri);
  if (folder_vfs &&
      (folder_vfs->types & types) == types)
    {
      info = lookup_vfs_info_in_folder (GTK_FILE_FOLDER (folder_vfs),
					path,
					NULL);
      gnome_vfs_file_info_ref (info);
    }
  else
    {
      info = gnome_vfs_file_info_new ();
      uri = gtk_file_path_get_string (path);
      gnome_vfs_get_file_info (uri, info, get_options (types));
    }

  gtk_file_path_free (parent_path);
  
  return info;
  
}

static GdkPixbuf *
gtk_file_system_gnome_vfs_render_icon (GtkFileSystem     *file_system,
				       const GtkFilePath *path,
				       GtkWidget         *widget,
				       gint               pixel_size,
				       GError           **error)
{
  GtkIconTheme *icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (widget));
  GtkFileSystemGnomeVFS *system_vfs;
  const char *uri;
  GdkPixbuf *pixbuf;
  char *icon_name;
  GnomeVFSFileInfo *info;

  system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);

  pixbuf = NULL;

  info = get_vfs_info (file_system, path, GTK_FILE_INFO_MIME_TYPE);
  uri = gtk_file_path_get_string (path);
  if (strcmp (uri, system_vfs->desktop_uri) == 0)
    icon_name = g_strdup ("gnome-fs-desktop");
  else  
    icon_name = gnome_icon_lookup (icon_theme,
				   NULL,
				   uri,
				   NULL,
				   info,
				   info->mime_type,
				   GNOME_ICON_LOOKUP_FLAGS_NONE,
				   NULL);
  if (icon_name)
    {
      pixbuf = get_cached_icon (widget, icon_name, pixel_size);
      g_free (icon_name);
    }
  else
    pixbuf = NULL;

  return pixbuf;
}

static void
bookmark_list_free (GSList *list)
{
  GSList *l;

  for (l = list; l; l = l->next)
    g_free (l->data);

  g_slist_free (list);
}

static char *
bookmark_get_filename (gboolean tmp_file)
{
  char *filename;

  filename = g_build_filename (g_get_home_dir (),
			       tmp_file ? BOOKMARKS_TMP_FILENAME : BOOKMARKS_FILENAME,
			       NULL);
  g_assert (filename != NULL);
  return filename;
}

static gboolean
bookmark_list_read (GSList **bookmarks, GError **error)
{
  gchar *filename;
  gchar *contents;
  gboolean result;

  filename = bookmark_get_filename (FALSE);
  *bookmarks = NULL;
  result = FALSE;

  if (g_file_get_contents (filename, &contents, NULL, error))
    {
      gchar **lines = g_strsplit (contents, "\n", -1);
      int i;
      GHashTable *table;

      table = g_hash_table_new (g_str_hash, g_str_equal);

      for (i = 0; lines[i]; i++)
	{
	  if (lines[i][0] && !g_hash_table_lookup (table, lines[i]))
	    {
	      *bookmarks = g_slist_prepend (*bookmarks, g_strdup (lines[i]));
	      g_hash_table_insert (table, lines[i], lines[i]);
	    }
	}

      g_free (contents);
      g_hash_table_destroy (table);
      g_strfreev (lines);

      *bookmarks = g_slist_reverse (*bookmarks);
      result = TRUE;
    }

  g_free (filename);

  return result;
}

static gboolean
bookmark_list_write (GSList *bookmarks, GError **error)
{
  char *tmp_filename;
  char *filename;
  gboolean result = TRUE;
  FILE *file;
  int fd;
  int saved_errno;

  /* First, write a temporary file */

  tmp_filename = bookmark_get_filename (TRUE);
  filename = bookmark_get_filename (FALSE);

  fd = g_mkstemp (tmp_filename);
  if (fd == -1)
    {
      saved_errno = errno;
      goto io_error;
    }

  if ((file = fdopen (fd, "w")) != NULL)
    {
      GSList *l;

      for (l = bookmarks; l; l = l->next)
	if (fputs (l->data, file) == EOF
	    || fputs ("\n", file) == EOF)
	  {
	    saved_errno = errno;
	    goto io_error;
	  }

      if (fclose (file) == EOF)
	{
	  saved_errno = errno;
	  goto io_error;
	}

      if (rename (tmp_filename, filename) == -1)
	{
	  saved_errno = errno;
	  goto io_error;
	}

      result = TRUE;
      goto out;
    }
  else
    {
      saved_errno = errno;

      /* fdopen() failed, so we can't do much error checking here anyway */
      close (fd);
    }

 io_error:

  g_set_error (error,
	       GTK_FILE_SYSTEM_ERROR,
	       GTK_FILE_SYSTEM_ERROR_FAILED,
	       _("Bookmark saving failed (%s)"),
	       g_strerror (saved_errno));
  result = FALSE;

  if (fd != -1)
    unlink (tmp_filename); /* again, not much error checking we can do here */

 out:

  g_free (filename);
  g_free (tmp_filename);

  return result;
}

static gboolean
gtk_file_system_gnome_vfs_add_bookmark (GtkFileSystem     *file_system,
					const GtkFilePath *path,
					GError           **error)
{
  GSList *bookmarks;
  GSList *l;
  char *uri;
  gboolean result;

  if (!bookmark_list_read (&bookmarks, error))
    return FALSE;

  result = FALSE;

  uri = gtk_file_system_path_to_uri (file_system, path);

  for (l = bookmarks; l; l = l->next)
    {
      const char *bookmark;

      bookmark = l->data;
      if (strcmp (bookmark, uri) == 0)
	break;
    }

  if (!l)
    {
      bookmarks = g_slist_append (bookmarks, g_strdup (uri));
      if (bookmark_list_write (bookmarks, error))
	{
	  result = TRUE;
	  g_signal_emit_by_name (file_system, "bookmarks-changed", 0);
	}
    }

  g_free (uri);
  bookmark_list_free (bookmarks);

  return result;
}

static gboolean
gtk_file_system_gnome_vfs_remove_bookmark (GtkFileSystem     *file_system,
					   const GtkFilePath *path,
					   GError           **error)
{
  GSList *bookmarks;
  char *uri;
  GSList *l;
  gboolean result;

  if (!bookmark_list_read (&bookmarks, error))
    return FALSE;

  result = FALSE;

  uri = gtk_file_system_path_to_uri (file_system, path);

  for (l = bookmarks; l; l = l->next)
    {
      const char *bookmark;

      bookmark = l->data;
      if (strcmp (bookmark, uri) == 0)
	break;
    }

  if (l)
    {
      g_free (l->data);
      bookmarks = g_slist_remove_link (bookmarks, l);
      g_slist_free_1 (l);

      if (bookmark_list_write (bookmarks, error))
	result = TRUE;
    }
  else
    result = TRUE;

  g_free (uri);
  bookmark_list_free (bookmarks);

  if (result)
    g_signal_emit_by_name (file_system, "bookmarks-changed", 0);

  return result;
}

static GSList *
gtk_file_system_gnome_vfs_list_bookmarks (GtkFileSystem *file_system)
{
  GSList *bookmarks;
  GSList *result;
  GSList *l;

  if (!bookmark_list_read (&bookmarks, NULL))
    return NULL;

  result = NULL;

  for (l = bookmarks; l; l = l->next)
    {
      const char *name;

      name = l->data;
      result = g_slist_prepend (result, gtk_file_system_uri_to_path (file_system, name));
    }

  bookmark_list_free (bookmarks);

  result = g_slist_reverse (result);
  return result;
}

#ifdef USE_GCONF
static gboolean
gtk_file_system_gnome_vfs_add_bookmark (GtkFileSystem     *file_system,
					const GtkFilePath *path,
					GError           **error)
{
  GtkFileSystemGnomeVFS *system_vfs;
  GSList *list, *l;
  gboolean result;

  system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);

  list = NULL;

  for (l = system_vfs->bookmarks; l; l = l->next)
    {
      GtkFilePath *p;
      const char *str;

      p = l->data;
      str = gtk_file_path_get_string (p);

      list = g_slist_prepend (list, (gpointer) str);
    }

  list = g_slist_prepend (list, (gpointer) gtk_file_path_get_string (path));
  list = g_slist_reverse (list);

  result = gconf_client_set_list (system_vfs->client, BOOKMARKS_KEY,
				  GCONF_VALUE_STRING,
				  list,
				  error);

  g_slist_free (list);

  return result;
}

static gboolean
gtk_file_system_gnome_vfs_remove_bookmark (GtkFileSystem     *file_system,
					   const GtkFilePath *path,
					   GError           **error)
{
  GtkFileSystemGnomeVFS *system_vfs;
  GSList *list, *l;
  gboolean found;
  gboolean result;

  system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);

  list = NULL;
  found = FALSE;

  for (l = system_vfs->bookmarks; l; l = l->next)
    {
      GtkFilePath *p;

      p = l->data;
      if (gtk_file_path_compare (path, p) != 0)
	{
	  const char *str;

	  str = gtk_file_path_get_string (p);
	  list = g_slist_prepend (list, (gpointer) str);
	}
      else
	found = TRUE;
    }

  if (found)
    {
      list = g_slist_reverse (list);

      result = gconf_client_set_list (system_vfs->client, BOOKMARKS_KEY,
				      GCONF_VALUE_STRING,
				      list,
				      error);
    }
  else
    {
      g_set_error (error,
		   GTK_FILE_SYSTEM_ERROR,
		   GTK_FILE_SYSTEM_ERROR_FAILED,
		   "Path %s is not in bookmarks",
		   gtk_file_path_get_string (path));
      result = FALSE;
    }

  g_slist_free (list);

  return result;
}

static GSList *
gtk_file_system_gnome_vfs_list_bookmarks (GtkFileSystem *file_system)
{
  GtkFileSystemGnomeVFS *system_vfs;

  system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);

  return gtk_file_paths_copy (system_vfs->bookmarks);
}
#endif

/*
 * GtkFileFolderGnomeVFS
 */
static GType
gtk_file_folder_gnome_vfs_get_type (void)
{
  return type_gtk_file_folder_gnome_vfs;
}

static void
gtk_file_folder_gnome_vfs_class_init (GtkFileFolderGnomeVFSClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  folder_parent_class = g_type_class_peek_parent (class);

  gobject_class->finalize = gtk_file_folder_gnome_vfs_finalize;
}

static void
gtk_file_folder_gnome_vfs_iface_init (GtkFileFolderIface *iface)
{
  iface->get_info = gtk_file_folder_gnome_vfs_get_info;
  iface->list_children = gtk_file_folder_gnome_vfs_list_children;
}

static void
gtk_file_folder_gnome_vfs_init (GtkFileFolderGnomeVFS *impl)
{
}

static void
gtk_file_folder_gnome_vfs_finalize (GObject *object)
{
  GtkFileFolderGnomeVFS *folder_vfs = GTK_FILE_FOLDER_GNOME_VFS (object);
  GtkFileSystemGnomeVFS *system_vfs = folder_vfs->system;

  if (folder_vfs->uri)
    g_hash_table_remove (system_vfs->folders, folder_vfs->uri);
  if (folder_vfs->async_handle)
    gnome_vfs_async_cancel (folder_vfs->async_handle);
  if (folder_vfs->monitor)
    gnome_vfs_monitor_cancel (folder_vfs->monitor);
  if (folder_vfs->children)
    g_hash_table_destroy (folder_vfs->children);

  g_free (folder_vfs->uri);

  folder_parent_class->finalize (object);
}


static GnomeVFSFileInfo *
lookup_vfs_info_in_folder (GtkFileFolder     *folder,
			   const GtkFilePath *path,
			   GError           **error)
{
  GtkFileFolderGnomeVFS *folder_vfs = GTK_FILE_FOLDER_GNOME_VFS (folder);
  const gchar *uri = gtk_file_path_get_string (path);
  FolderChild *child;

  child = g_hash_table_lookup (folder_vfs->children, uri);
  if (!child)
    {
      /* We may not have loaded the file's information yet.
       */
      GnomeVFSFileInfo *vfs_info;
      GnomeVFSResult result;
      
      vfs_info = gnome_vfs_file_info_new ();
      result = gnome_vfs_get_file_info (uri, vfs_info, get_options (folder_vfs->types));
      
      if (result != GNOME_VFS_OK)
	set_vfs_error (result, uri, error);
      else
	{
	  GSList *uris;
	  
	  child = folder_child_new (uri, vfs_info);
	  g_hash_table_replace (folder_vfs->children, child->uri, child);
	  
	  uris = g_slist_append (NULL, (char *) uri);
	  g_signal_emit_by_name (folder_vfs, "files-added", uris);
	  g_slist_free (uris);
	}
      
      gnome_vfs_file_info_unref (vfs_info);
    }
  
  return child->info;
}
static GtkFileInfo *
gtk_file_folder_gnome_vfs_get_info (GtkFileFolder     *folder,
				    const GtkFilePath *path,
				    GError           **error)
{
  GtkFileFolderGnomeVFS *folder_vfs = GTK_FILE_FOLDER_GNOME_VFS (folder);
  const gchar *uri = gtk_file_path_get_string (path);
  GnomeVFSFileInfo *info;

  info = lookup_vfs_info_in_folder (folder, path, error);
  if (info)
    return info_from_vfs_info (uri, info, folder_vfs->types);
  else
    return NULL;
}

static void
list_children_foreach (gpointer key,
		       gpointer value,
		       gpointer user_data)
{
  FolderChild *child = value;
  GSList **list = user_data;

  *list = g_slist_prepend (*list, gtk_file_path_new_dup (child->uri));
}

static gboolean
gtk_file_folder_gnome_vfs_list_children (GtkFileFolder  *folder,
					 GSList        **children,
					 GError        **error)
{
  GtkFileFolderGnomeVFS *folder_vfs = GTK_FILE_FOLDER_GNOME_VFS (folder);

  load_dir (folder_vfs);
  
  *children = NULL;

  g_hash_table_foreach (folder_vfs->children, list_children_foreach, children);

  *children = g_slist_reverse (*children);

  return TRUE;
}


static GnomeVFSFileInfoOptions
get_options (GtkFileInfoType types)
{
  GnomeVFSFileInfoOptions options = GNOME_VFS_FILE_INFO_FOLLOW_LINKS;
#if 0
  if ((types & GTK_FILE_INFO_MIME_TYPE) || (types & GTK_FILE_INFO_ICON))
#else
  if (types & GTK_FILE_INFO_MIME_TYPE)
#endif
    {
      options |= GNOME_VFS_FILE_INFO_GET_MIME_TYPE;
    }

  return options;
}

static GtkFileInfo *
info_from_vfs_info (const gchar      *uri,
		    GnomeVFSFileInfo *vfs_info,
		    GtkFileInfoType   types)
{
  GtkFileInfo *info = gtk_file_info_new ();

  if (types & GTK_FILE_INFO_DISPLAY_NAME)
    {
      if (!vfs_info->name || strcmp (vfs_info->name, "/") == 0)
	{
	  if (strcmp (uri, "file:///") == 0)
	    gtk_file_info_set_display_name (info, "/");
	  else
	    gtk_file_info_set_display_name (info, uri);
	}
      else
	{
	  gchar *display_name = g_filename_to_utf8 (vfs_info->name, -1, NULL, NULL, NULL);
	  if (!display_name)
	    display_name = g_strescape (vfs_info->name, NULL);

	  gtk_file_info_set_display_name (info, display_name);

	  g_free (display_name);
	}
    }

  gtk_file_info_set_is_hidden (info, vfs_info->name && vfs_info->name[0] == '.');
  gtk_file_info_set_is_folder (info, vfs_info->type == GNOME_VFS_FILE_TYPE_DIRECTORY);

#if 0
  if ((types & GTK_FILE_INFO_MIME_TYPE) || (types & GTK_FILE_INFO_ICON))
#else
  if (types & GTK_FILE_INFO_MIME_TYPE)
#endif
    {
      gtk_file_info_set_mime_type (info, vfs_info->mime_type);
    }

  gtk_file_info_set_modification_time (info, vfs_info->mtime);
  gtk_file_info_set_size (info, vfs_info->size);

  return info;
}

static FolderChild *
folder_child_new (const char       *uri,
		  GnomeVFSFileInfo *info)
{
  FolderChild *child;

  child = g_new (FolderChild, 1);
  child->uri = g_strdup (uri);
  child->info = info;
  gnome_vfs_file_info_ref (child->info);

  return child;
}

static void
folder_child_free (FolderChild *child)
{
  g_free (child->uri);
  if (child->info)
    gnome_vfs_file_info_unref (child->info);

  g_free (child);
}

static void
set_vfs_error (GnomeVFSResult result,
	       const gchar   *uri,
	       GError       **error)
{
  GtkFileSystemError errcode = GTK_FILE_SYSTEM_ERROR_FAILED;

  switch (result)
    {
    case GNOME_VFS_OK:
      g_assert_not_reached ();
      break;
    case GNOME_VFS_ERROR_NOT_FOUND:
      errcode = GTK_FILE_SYSTEM_ERROR_NONEXISTENT;
      break;
    case GNOME_VFS_ERROR_BAD_PARAMETERS:
    case GNOME_VFS_ERROR_IO:
    case GNOME_VFS_ERROR_INVALID_URI:
      errcode = GTK_FILE_SYSTEM_ERROR_INVALID_URI;
      break;
    case GNOME_VFS_ERROR_NOT_A_DIRECTORY:
      errcode = GTK_FILE_SYSTEM_ERROR_NOT_FOLDER;
      break;
    default:
      break;
    }

  if (uri)
    {
      g_set_error (error,
		   GTK_FILE_SYSTEM_ERROR,
		   errcode,
		   "error accessing '%s': %s",
		   uri,
		   gnome_vfs_result_to_string (result));
    }
  else
    {
      g_set_error (error,
		   GTK_FILE_SYSTEM_ERROR,
		   errcode,
		   "VFS error: %s",
		   gnome_vfs_result_to_string (result));
    }
}

/* Callback used when a volume gets mounted or unmounted */
static void
volume_mount_unmount_cb (GnomeVFSVolumeMonitor *monitor,
			 GnomeVFSVolume        *volume,
			 GtkFileSystemGnomeVFS *system_vfs)
{
  g_signal_emit_by_name (system_vfs, "volumes-changed");
}

/* Callback used when a drive gets connected or disconnected */
static void
drive_connect_disconnect_cb (GnomeVFSVolumeMonitor *monitor,
			     GnomeVFSDrive         *drive,
			     GtkFileSystemGnomeVFS *system_vfs)
{
  g_signal_emit_by_name (system_vfs, "volumes-changed");
}

static void
directory_load_callback (GnomeVFSAsyncHandle *handle,
			 GnomeVFSResult       result,
			 GList               *list,
			 guint                entries_read,
			 gpointer             user_data)
{
  GtkFileFolderGnomeVFS *folder_vfs = user_data;
  GList *tmp_list;
  GSList *added_uris = NULL;
  GSList *changed_uris = NULL;

  for (tmp_list = list; tmp_list; tmp_list = tmp_list->next)
    {
      GnomeVFSFileInfo *vfs_info;
      gchar *uri;
      gboolean new = FALSE;

      vfs_info = tmp_list->data;
      if (strcmp (vfs_info->name, ".") == 0 ||
	  strcmp (vfs_info->name, "..") == 0)
	continue;

      uri = make_child_uri (folder_vfs->uri, vfs_info->name, NULL);

      if (uri)
	{
	  FolderChild *child;

	  child = folder_child_new (uri, vfs_info);

	  if (!g_hash_table_lookup (folder_vfs->children, child->uri))
	    new = TRUE;

	  g_hash_table_replace (folder_vfs->children,
				child->uri, child);

	  if (new)
	    added_uris = g_slist_prepend (added_uris, child->uri);
	  else
	    changed_uris = g_slist_prepend (changed_uris, child->uri);
	}
    }

  if (added_uris)
    {
      g_signal_emit_by_name (folder_vfs, "files-added", added_uris);
      g_slist_free (added_uris);
    }
  if (changed_uris)
    {
      g_signal_emit_by_name (folder_vfs, "files-changed", changed_uris);
      g_slist_free (changed_uris);
    }

  if (result != GNOME_VFS_OK)
    folder_vfs->async_handle = NULL;
}

static void
monitor_callback (GnomeVFSMonitorHandle   *handle,
		  const gchar             *monitor_uri,
		  const gchar             *info_uri,
		  GnomeVFSMonitorEventType event_type,
		  gpointer                 user_data)
{
  GtkFileFolderGnomeVFS *folder_vfs = user_data;
  GSList *uris;

  switch (event_type)
    {
    case GNOME_VFS_MONITOR_EVENT_CHANGED:
    case GNOME_VFS_MONITOR_EVENT_CREATED:
      {
	GnomeVFSResult result;
	GnomeVFSFileInfo *vfs_info;

	vfs_info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (info_uri, vfs_info, get_options (folder_vfs->types));

	if (result == GNOME_VFS_OK)
	  {
	    FolderChild *child;

	    child = folder_child_new (info_uri, vfs_info);
	    gnome_vfs_file_info_unref (vfs_info);

	    g_hash_table_replace (folder_vfs->children,
				  child->uri, child);

	    uris = g_slist_prepend (NULL, (gchar *)info_uri);
	    if (event_type == GNOME_VFS_MONITOR_EVENT_CHANGED)
	      g_signal_emit_by_name (folder_vfs, "files-changed", uris);
	    else
	      g_signal_emit_by_name (folder_vfs, "files-added", uris);
	    g_slist_free (uris);
	  }
	else
	  gnome_vfs_file_info_unref (vfs_info);
      }
      break;
    case GNOME_VFS_MONITOR_EVENT_DELETED:
      g_hash_table_remove (folder_vfs->children, info_uri);

      uris = g_slist_prepend (NULL, (gchar *)info_uri);
      g_signal_emit_by_name (folder_vfs, "files-removed", uris);
      g_slist_free (uris);
      break;
    case GNOME_VFS_MONITOR_EVENT_STARTEXECUTING:
    case GNOME_VFS_MONITOR_EVENT_STOPEXECUTING:
    case GNOME_VFS_MONITOR_EVENT_METADATA_CHANGED:
      break;
    }
}

static gboolean
is_valid_scheme_character (char c)
{
  return g_ascii_isalnum (c) || c == '+' || c == '-' || c == '.';
}

static gboolean
has_valid_scheme (const char *uri)
{
  const char *p;

  p = uri;

  if (!is_valid_scheme_character (*p))
    return FALSE;

  do
    p++;
  while (is_valid_scheme_character (*p));

  return *p == ':';
}

#ifdef USE_GCONF
/* Sets the bookmarks list from a GConfValue */
static void
set_bookmarks_from_value (GtkFileSystemGnomeVFS *system_vfs,
			  GConfValue            *value,
			  gboolean               emit_changed)
{
  gtk_file_paths_free (system_vfs->bookmarks);
  system_vfs->bookmarks = NULL;

  if (value && gconf_value_get_list_type (value) == GCONF_VALUE_STRING)
    {
      GSList *list, *l;

      list = gconf_value_get_list (value);

      for (l = list; l; l = l->next)
	{
	  GConfValue *v;
	  const char *uri;
	  GtkFilePath *path;

	  v = l->data;
	  uri = gconf_value_get_string (v);
	  path = gtk_file_system_uri_to_path (GTK_FILE_SYSTEM (system_vfs), uri);

	  if (!path)
	    continue;

	  system_vfs->bookmarks = g_slist_prepend (system_vfs->bookmarks, path);
	}

      system_vfs->bookmarks = g_slist_reverse (system_vfs->bookmarks);
    }

  if (emit_changed)
    g_signal_emit_by_name (system_vfs, "bookmarks-changed");
}

static void
client_notify_cb (GConfClient *client,
		  guint        cnxn_id,
		  GConfEntry  *entry,
		  gpointer     data)
{
  GtkFileSystemGnomeVFS *system_vfs;
  GConfValue *value;

  system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (data);

  if (strcmp (gconf_entry_get_key (entry), BOOKMARKS_KEY) != 0)
    return;

  value = gconf_entry_get_value (entry);
  set_bookmarks_from_value (system_vfs, value, TRUE);
}
#endif

/* GtkFileSystem module calls */

void fs_module_init (GTypeModule    *module);
void fs_module_exit (void);
GtkFileSystem *fs_module_create (void);

void 
fs_module_init (GTypeModule    *module)
{

  gnome_vfs_init ();

  {
    static const GTypeInfo file_system_gnome_vfs_info =
      {
	sizeof (GtkFileSystemGnomeVFSClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) gtk_file_system_gnome_vfs_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GtkFileSystemGnomeVFS),
	0,		/* n_preallocs */
	(GInstanceInitFunc) gtk_file_system_gnome_vfs_init,
      };
    static const GInterfaceInfo file_system_info =
      {
	(GInterfaceInitFunc) gtk_file_system_gnome_vfs_iface_init, /* interface_init */
	NULL,			                              /* interface_finalize */
	NULL			                              /* interface_data */
      };
    
    
    type_gtk_file_system_gnome_vfs = 
      g_type_module_register_type (module,
				   G_TYPE_OBJECT,
				   "GtkFileSystemGnomeVFS",
				   &file_system_gnome_vfs_info, 0);

    g_type_module_add_interface (module,
				 GTK_TYPE_FILE_SYSTEM_GNOME_VFS,
				 GTK_TYPE_FILE_SYSTEM,
				 &file_system_info);
  }

  {
    static const GTypeInfo file_folder_gnome_vfs_info =
      {
	sizeof (GtkFileFolderGnomeVFSClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) gtk_file_folder_gnome_vfs_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GtkFileFolderGnomeVFS),
	0,		/* n_preallocs */
	(GInstanceInitFunc) gtk_file_folder_gnome_vfs_init,
      };
    
    static const GInterfaceInfo file_folder_info =
      {
	(GInterfaceInitFunc) gtk_file_folder_gnome_vfs_iface_init, /* interface_init */
	NULL,			                              /* interface_finalize */
	NULL			                              /* interface_data */
      };
    
    type_gtk_file_folder_gnome_vfs = g_type_module_register_type (module,
								  G_TYPE_OBJECT,
								  "GtkFileFolderGnomeVFS",
								  &file_folder_gnome_vfs_info, 0);
    g_type_module_add_interface (module,
				 type_gtk_file_folder_gnome_vfs,
				 GTK_TYPE_FILE_FOLDER,
				 &file_folder_info);
  }
}

void 
fs_module_exit (void)
{
}

GtkFileSystem *     
fs_module_create (void)
{
  return gtk_file_system_gnome_vfs_new ();
}
