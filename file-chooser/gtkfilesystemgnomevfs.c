/* GTK - The GIMP Toolkit
 * gtkfilesystemgnomevfs.c: Implementation of GtkFileSystem for gnome-vfs
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
 *
 * portions come from eel-vfs-extensions.c:
 *
 *   Authors: Darin Adler <darin@eazel.com>
 *	      Pavel Cisler <pavel@eazel.com>
 *	      Mike Fleming  <mfleming@eazel.com>
 *            John Sullivan <sullivan@eazel.com>
 */

#include "gtkfilesystem.h"
#include "gtkfilesystemgnomevfs.h"

#include <libgnomevfs/gnome-vfs.h>
#include <gconf/gconf-client.h>

#include <stdlib.h>
#include <string.h>

typedef struct _GtkFileSystemGnomeVFSClass GtkFileSystemGnomeVFSClass;

#define GTK_FILE_SYSTEM_GNOME_VFS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_FILE_SYSTEM_GNOME_VFS, GtkFileSystemGnomeVFSClass))
#define GTK_IS_FILE_SYSTEM_GNOME_VFS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_FILE_SYSTEM_GNOME_VFS))
#define GTK_FILE_SYSTEM_GNOME_VFS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_FILE_SYSTEM_GNOME_VFS, GtkFileSystemGnomeVFSClass))

struct _GtkFileSystemGnomeVFSClass
{
  GObjectClass parent_class;
};

struct _GtkFileSystemGnomeVFS
{
  GObject parent_instance;

  GHashTable *folders;
  GSList *roots;
  GtkFileFolder *local_root;

  GConfClient *client;
  guint client_notify_id;
  GSList *bookmarks;

  guint locale_encoded_filenames : 1;
};

#define GTK_TYPE_FILE_FOLDER_GNOME_VFS             (gtk_file_folder_gnome_vfs_get_type ())
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
  gchar *uri;

  GnomeVFSAsyncHandle *async_handle;
  GnomeVFSMonitorHandle *monitor;

  GtkFileSystemGnomeVFS *system;

  GHashTable *children;
};

typedef struct _FolderChild FolderChild;

struct _FolderChild
{
  gchar *uri;
  GnomeVFSFileInfo *info;
};

/* GConf paths for the bookmarks; keep these two in sync */
#define BOOKMARKS_KEY  "/desktop/gnome/interface/bookmark_folders"
#define BOOKMARKS_PATH "/desktop/gnome/interface"

static void gtk_file_system_gnome_vfs_class_init (GtkFileSystemGnomeVFSClass *class);
static void gtk_file_system_gnome_vfs_iface_init (GtkFileSystemIface         *iface);
static void gtk_file_system_gnome_vfs_init       (GtkFileSystemGnomeVFS      *impl);
static void gtk_file_system_gnome_vfs_finalize   (GObject                    *object);

static GSList *       gtk_file_system_gnome_vfs_list_roots    (GtkFileSystem      *file_system);
static GtkFileInfo *  gtk_file_system_gnome_vfs_get_root_info (GtkFileSystem      *file_system,
							       const GtkFilePath  *path,
							       GtkFileInfoType     types,
							       GError            **error);
static GtkFileFolder *gtk_file_system_gnome_vfs_get_folder    (GtkFileSystem      *file_system,
							       const GtkFilePath  *path,
							       GtkFileInfoType     types,
							       GError            **error);
static gboolean       gtk_file_system_gnome_vfs_create_folder (GtkFileSystem      *file_system,
							       const GtkFilePath  *path,
							       GError            **error);
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

static gboolean       gtk_file_system_gnome_vfs_get_supports_bookmarks (GtkFileSystem *file_system);
static void           gtk_file_system_gnome_vfs_set_bookmarks          (GtkFileSystem *file_system,
									GSList        *bookmarks,
									GError       **error);
static GSList *       gtk_file_system_gnome_vfs_list_bookmarks         (GtkFileSystem *file_system);

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

static void set_bookmarks_from_value (GtkFileSystemGnomeVFS *system_vfs,
				      GConfValue            *value,
				      gboolean               emit_changed);

static void client_notify_cb (GConfClient *client,
			      guint        cnxn_id,
			      GConfEntry  *entry,
			      gpointer     data);

static void  folder_child_free (FolderChild *child);

static void     set_vfs_error     (GnomeVFSResult   result,
				   const gchar     *uri,
				   GError         **error);
static gboolean has_valid_scheme  (const char      *uri);

static GObjectClass *system_parent_class;
static GObjectClass *folder_parent_class;

#define ITEMS_PER_NOTIFICATION 100

/*
 * GtkFileSystemGnomeVFS
 */
GType
gtk_file_system_gnome_vfs_get_type (void)
{
  static GType file_system_gnome_vfs_type = 0;

  if (!file_system_gnome_vfs_type)
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

      file_system_gnome_vfs_type = g_type_register_static (G_TYPE_OBJECT,
							   "GtkFileSystemGnomeVFS",
							   &file_system_gnome_vfs_info, 0);
      g_type_add_interface_static (file_system_gnome_vfs_type,
				   GTK_TYPE_FILE_SYSTEM,
				   &file_system_info);
    }

  return file_system_gnome_vfs_type;
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
  GtkFilePath *local_path;
  
  gnome_vfs_init ();
  
  system_vfs = g_object_new (GTK_TYPE_FILE_SYSTEM_GNOME_VFS, NULL);

  system_vfs->locale_encoded_filenames = (getenv ("G_BROKEN_FILENAMES") != NULL);

  system_vfs->folders = g_hash_table_new (g_str_hash, g_str_equal);

  /* Always display the file:/// root
   */
  local_path = gtk_file_path_new_dup ("file:///");
  system_vfs->local_root = gtk_file_system_get_folder (GTK_FILE_SYSTEM (system_vfs),
						       local_path,
						       GTK_FILE_INFO_ALL,
						       NULL);
  g_assert (system_vfs->local_root);
  gtk_file_path_free (local_path);
  
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
  iface->list_roots = gtk_file_system_gnome_vfs_list_roots;
  iface->get_folder = gtk_file_system_gnome_vfs_get_folder;
  iface->get_root_info = gtk_file_system_gnome_vfs_get_root_info;
  iface->create_folder = gtk_file_system_gnome_vfs_create_folder;
  iface->get_parent = gtk_file_system_gnome_vfs_get_parent;
  iface->make_path = gtk_file_system_gnome_vfs_make_path;
  iface->parse = gtk_file_system_gnome_vfs_parse;
  iface->path_to_uri = gtk_file_system_gnome_vfs_path_to_uri;
  iface->path_to_filename = gtk_file_system_gnome_vfs_path_to_filename;
  iface->uri_to_path = gtk_file_system_gnome_vfs_uri_to_path;
  iface->filename_to_path = gtk_file_system_gnome_vfs_filename_to_path;
  iface->get_supports_bookmarks = gtk_file_system_gnome_vfs_get_supports_bookmarks;
  iface->set_bookmarks = gtk_file_system_gnome_vfs_set_bookmarks;
  iface->list_bookmarks = gtk_file_system_gnome_vfs_list_bookmarks;
}

static void
gtk_file_system_gnome_vfs_init (GtkFileSystemGnomeVFS *system_vfs)
{
  GConfValue *value;

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
}

static void
gtk_file_system_gnome_vfs_finalize (GObject *object)
{
  GtkFileSystemGnomeVFS *system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (object);

  g_object_unref (system_vfs->local_root);
  g_hash_table_destroy (system_vfs->folders);

  gconf_client_notify_remove (system_vfs->client, system_vfs->client_notify_id);
  g_object_unref (system_vfs->client);
  gtk_file_paths_free (system_vfs->bookmarks);

  /* XXX Assert ->roots and ->folders should be empty
   */
  system_parent_class->finalize (object);
}

static GSList *
gtk_file_system_gnome_vfs_list_roots (GtkFileSystem *file_system)
{
  GtkFileSystemGnomeVFS *system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);
  GSList *result = NULL;
  GSList *tmp_list;

  for (tmp_list = system_vfs->roots; tmp_list; tmp_list = tmp_list->next)
    {
      GtkFileFolderGnomeVFS *folder_vfs = tmp_list->data;
      result = g_slist_prepend (result, gtk_file_path_new_dup (folder_vfs->uri));
    }
  
  return result;
}

static GtkFileInfo *
gtk_file_system_gnome_vfs_get_root_info (GtkFileSystem     *file_system,
					 const GtkFilePath *path,
					 GtkFileInfoType    types,
					 GError           **error)
{
  const gchar *uri = gtk_file_path_get_string (path);
  GnomeVFSResult result;
  GnomeVFSFileInfo *vfs_info;
  GtkFileInfo *info = NULL;
  
  vfs_info = gnome_vfs_file_info_new ();

  result = gnome_vfs_get_file_info (uri, vfs_info, get_options (types));
  if (result != GNOME_VFS_OK)
    set_vfs_error (result, uri, error);
  else
    info = info_from_vfs_info (uri, vfs_info, types);

  gnome_vfs_file_info_unref (vfs_info);

  return info;
}

static void
ensure_types (GtkFileFolderGnomeVFS *folder_vfs,
	      GtkFileInfoType        types)
{
  if ((folder_vfs->types & types) != types)
    {
      folder_vfs->types |= types;

      if (folder_vfs->async_handle)
	gnome_vfs_async_cancel (folder_vfs->async_handle);
      
      gnome_vfs_async_load_directory (&folder_vfs->async_handle,
				      folder_vfs->uri,
				      get_options (folder_vfs->types),
				      ITEMS_PER_NOTIFICATION,
				      GNOME_VFS_PRIORITY_DEFAULT,
				      directory_load_callback, folder_vfs);

    }
}

static GtkFileFolder *
gtk_file_system_gnome_vfs_get_folder (GtkFileSystem     *file_system,
				      const GtkFilePath *path,
				      GtkFileInfoType    types,
				      GError           **error)
{
  const gchar *uri = gtk_file_path_get_string (path);
  GtkFilePath *parent_path;
  GtkFileSystemGnomeVFS *system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);
  GtkFileFolderGnomeVFS *folder_vfs;
  GnomeVFSMonitorHandle *monitor = NULL;
  GnomeVFSAsyncHandle *async_handle;
  GnomeVFSResult result;

  folder_vfs = g_hash_table_lookup (system_vfs->folders, uri);
  if (folder_vfs)
    {
      ensure_types (folder_vfs, types);
      return g_object_ref (folder_vfs);
    }

  if (!gtk_file_system_get_parent (file_system, path, &parent_path, error))
    return FALSE;

  folder_vfs = g_object_new (GTK_TYPE_FILE_FOLDER_GNOME_VFS, NULL);
  
  result = gnome_vfs_monitor_add (&monitor,
				  uri,
				  GNOME_VFS_MONITOR_DIRECTORY,
				  monitor_callback, folder_vfs);
  if (result != GNOME_VFS_OK && result != GNOME_VFS_ERROR_NOT_SUPPORTED)
    {
      g_object_unref (folder_vfs);
      set_vfs_error (result, uri, error);
      return NULL;
    }

  gnome_vfs_async_load_directory (&async_handle,
				  uri,
				  get_options (types),
				  ITEMS_PER_NOTIFICATION,
				  GNOME_VFS_PRIORITY_DEFAULT,
				  directory_load_callback, folder_vfs);

  folder_vfs->system = system_vfs;
  folder_vfs->uri = g_strdup (uri);
  folder_vfs->types = types;
  folder_vfs->monitor = monitor;
  folder_vfs->async_handle = async_handle;
  folder_vfs->children = g_hash_table_new_full (g_str_hash,
						g_str_equal,
						NULL,
						(GDestroyNotify) folder_child_free);

  g_hash_table_insert (system_vfs->folders, folder_vfs->uri, folder_vfs);

  if (parent_path)
    {
      const gchar *parent_uri;
      GtkFileFolderGnomeVFS *parent_folder;
      GSList *uris;
      
      parent_uri = gtk_file_path_get_string (parent_path);
      parent_folder = g_hash_table_lookup (system_vfs->folders, parent_uri);
      gtk_file_path_free (parent_path);

      if (parent_folder &&
	  !g_hash_table_lookup (parent_folder->children, uri))
	{
	  GnomeVFSFileInfo *vfs_info;
	  FolderChild *child;

	  vfs_info = gnome_vfs_file_info_new ();
	  result = gnome_vfs_get_file_info (uri, vfs_info, get_options (parent_folder->types));
	  if (result != GNOME_VFS_OK)
	    {
	      gnome_vfs_file_info_unref (vfs_info);
	      set_vfs_error (result, uri, error);
	      return FALSE;
	    }
	  
	  child = g_new (FolderChild, 1);
	  child->uri = g_strdup (uri);
	  
	  child->info = vfs_info;
	  gnome_vfs_file_info_ref (child->info);
	  
	  g_hash_table_replace (parent_folder->children,
				child->uri, child);

	  uris = g_slist_prepend (NULL, child->uri);
	  g_signal_emit_by_name (parent_folder, "files-added", uris);
	  g_slist_free (uris);
	}
    }
  else
    {
      GSList *tmp_list;
      gboolean found = FALSE;

      for (tmp_list = system_vfs->roots; tmp_list; tmp_list = tmp_list->next)
	{
	  GtkFileFolderGnomeVFS *folder_vfs = tmp_list->data;
	  if (strcmp (folder_vfs->uri, uri) == 0)
	    {
	      found = TRUE;
	      break;
	    }
	}

      if (!found)
	{
	  system_vfs->roots = g_slist_prepend (system_vfs->roots, folder_vfs);
	  g_signal_emit_by_name (system_vfs, "roots-changed");
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
				     (GNOME_VFS_PERM_USER_ALL |
				      GNOME_VFS_PERM_GROUP_ALL |
				      GNOME_VFS_PERM_OTHER_READ |
				      GNOME_VFS_PERM_OTHER_EXEC));
  
  if (result != GNOME_VFS_OK)
    {
      set_vfs_error (result, uri, error);
      return FALSE;
    }

  return TRUE;
}

static gboolean
gtk_file_system_gnome_vfs_get_parent (GtkFileSystem     *file_system,
				      const GtkFilePath *path,
				      GtkFilePath      **parent,
				      GError           **error)
{
  GnomeVFSURI *uri = gnome_vfs_uri_new (gtk_file_path_get_string (path));
  GnomeVFSURI *parent_uri;

  g_return_val_if_fail (uri != NULL, FALSE);
  
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

  child_name = g_filename_to_utf8 (display_name, -1, NULL, NULL, &tmp_error);
  
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

static gboolean
gtk_file_system_gnome_vfs_get_supports_bookmarks (GtkFileSystem *file_system)
{
  return TRUE;
}

static void
gtk_file_system_gnome_vfs_set_bookmarks (GtkFileSystem *file_system,
					 GSList        *bookmarks,
					 GError       **error)
{
  GtkFileSystemGnomeVFS *system_vfs;
  GSList *list, *l;;

  system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);

  list = NULL;

  for (l = bookmarks; l; l = l->next)
    {
      GtkFilePath *path;
      const char *str;

      path = l->data;
      str = gtk_file_path_get_string (path);

      list = g_slist_prepend (list, (gpointer) str);
    }

  list = g_slist_reverse (list);

  if (!gconf_client_set_list (system_vfs->client, BOOKMARKS_KEY,
			      GCONF_VALUE_STRING,
			      list,
			      error))
    goto out;

  gtk_file_paths_free (system_vfs->bookmarks);
  system_vfs->bookmarks = gtk_file_paths_copy (bookmarks);

 out:

  g_slist_free (list);
}

static GSList *
gtk_file_system_gnome_vfs_list_bookmarks (GtkFileSystem *file_system)
{
  GtkFileSystemGnomeVFS *system_vfs;

  system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);

  return gtk_file_paths_copy (system_vfs->bookmarks);
}

/*
 * GtkFileFolderGnomeVFS
 */
static GType
gtk_file_folder_gnome_vfs_get_type (void)
{
  static GType file_folder_gnome_vfs_type = 0;

  if (!file_folder_gnome_vfs_type)
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

      file_folder_gnome_vfs_type = g_type_register_static (G_TYPE_OBJECT,
							   "GtkFileFolderGnomeVFS",
							   &file_folder_gnome_vfs_info, 0);
      g_type_add_interface_static (file_folder_gnome_vfs_type,
				   GTK_TYPE_FILE_FOLDER,
				   &file_folder_info);
    }

  return file_folder_gnome_vfs_type;
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

  if (system_vfs)
    {
      GSList *tmp_list;
      
      for (tmp_list = system_vfs->roots; tmp_list; tmp_list = tmp_list->next)
	{
	  if (tmp_list->data == object)
	    {
	      system_vfs->roots = g_slist_delete_link (system_vfs->roots, tmp_list);
	      g_signal_emit_by_name (system_vfs, "roots-changed");
	    }
	}
    }
  
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

static GtkFileInfo *
gtk_file_folder_gnome_vfs_get_info (GtkFileFolder     *folder,
				    const GtkFilePath *path,
				    GError           **error)
{
  GtkFileFolderGnomeVFS *folder_vfs = GTK_FILE_FOLDER_GNOME_VFS (folder);
  const gchar *uri = gtk_file_path_get_string (path);
  FolderChild *child;
  
  child = g_hash_table_lookup (folder_vfs->children, uri);
  if (!child)
    {
      g_set_error (error,
		   GTK_FILE_SYSTEM_ERROR,
		   GTK_FILE_SYSTEM_ERROR_NONEXISTENT,
		   "'%s' does not exist",
		   uri);

      return FALSE;
    }
  
  return info_from_vfs_info (uri, child->info, folder_vfs->types);
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

  *children = NULL;

  g_hash_table_foreach (folder_vfs->children, list_children_foreach, children);

  *children = g_slist_reverse (*children);
  
  return TRUE;
}


static GnomeVFSFileInfoOptions
get_options (GtkFileInfoType types)
{
  GnomeVFSFileInfoOptions options = GNOME_VFS_FILE_INFO_FOLLOW_LINKS;

  if ((types & GTK_FILE_INFO_MIME_TYPE) || (types & GTK_FILE_INFO_ICON))
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

  if ((types & GTK_FILE_INFO_MIME_TYPE) || (types & GTK_FILE_INFO_ICON))
    {
      gtk_file_info_set_mime_type (info, vfs_info->mime_type);
    }

  gtk_file_info_set_modification_time (info, vfs_info->mtime);
  gtk_file_info_set_size (info, vfs_info->size);
  
  return info;
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
	  FolderChild *child = g_new (FolderChild, 1);
	  child->uri = uri;
	  child->info = vfs_info;
	  gnome_vfs_file_info_ref (child->info);

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
	    FolderChild *child = g_new (FolderChild, 1);
	    child->uri = g_strdup (info_uri);
	    child->info = vfs_info;
	    gnome_vfs_file_info_ref (child->info);
	    
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
