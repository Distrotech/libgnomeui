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

static void gtk_file_system_gnome_vfs_class_init   (GtkFileSystemGnomeVFSClass *class);
static void gtk_file_system_gnome_vfs_iface_init   (GtkFileSystemIface     *iface);
static void gtk_file_system_gnome_vfs_init         (GtkFileSystemGnomeVFS      *impl);
static void gtk_file_system_gnome_vfs_finalize     (GObject                *object);

static GSList *       gtk_file_system_gnome_vfs_list_roots    (GtkFileSystem    *file_system);
static GtkFileInfo *  gtk_file_system_gnome_vfs_get_root_info (GtkFileSystem    *file_system,
							       const gchar      *uri,
							       GtkFileInfoType   types,
							       GError          **error);
static GtkFileFolder *gtk_file_system_gnome_vfs_get_folder    (GtkFileSystem    *file_system,
							       const gchar      *uri,
							       GtkFileInfoType   types,
							       GError          **error);
static gboolean       gtk_file_system_gnome_vfs_create_folder (GtkFileSystem    *file_system,
							       const gchar      *uri,
							       GError          **error);
static gboolean       gtk_file_system_gnome_vfs_get_parent    (GtkFileSystem    *file_system,
							       const gchar      *text_uri,
							       gchar           **parent,
							       GError          **error);
static gchar *        gtk_file_system_gnome_vfs_make_uri      (GtkFileSystem    *file_system,
							       const gchar      *base_uri,
							       const gchar      *display_name,
							       GError          **error);
static gboolean       gtk_file_system_gnome_vfs_parse         (GtkFileSystem    *file_system,
							       const gchar      *base_uri,
							       const gchar      *str,
							       gchar           **folder,
							       gchar           **file_part,
							       GError          **error);

static GType gtk_file_folder_gnome_vfs_get_type   (void);
static void  gtk_file_folder_gnome_vfs_class_init (GtkFileFolderGnomeVFSClass *class);
static void  gtk_file_folder_gnome_vfs_iface_init (GtkFileFolderIface     *iface);
static void  gtk_file_folder_gnome_vfs_init       (GtkFileFolderGnomeVFS      *impl);
static void  gtk_file_folder_gnome_vfs_finalize   (GObject                *object);

static GtkFileInfo *gtk_file_folder_gnome_vfs_get_info      (GtkFileFolder  *folder,
							const gchar    *uri,
							GError        **error);
static gboolean     gtk_file_folder_gnome_vfs_list_children (GtkFileFolder  *folder,
							GSList        **children,
							GError        **error);

static GtkFileInfo *           info_from_vfs_info (GnomeVFSFileInfo *vfs_info,
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

static void  folder_child_free (FolderChild *child);

static void     set_vfs_error     (GnomeVFSResult   result,
				   const gchar     *uri,
				   GError         **error);
static gboolean has_valid_scheme  (const char      *uri);

GObjectClass *system_parent_class;
GObjectClass *folder_parent_class;

#define ITEMS_PER_NOTIFICATION 100

/*
 * GtkFileSystemGnomeVFS
 */
GType
_gtk_file_system_gnome_vfs_get_type (void)
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
 * _gtk_file_system_gnome_vfs_new:
 * 
 * Creates a new #GtkFileSystemGnomeVFS object. #GtkFileSystemGnomeVFS
 * implements the #GtkFileSystem interface using the GNOME-VFS
 * library.
 * 
 * Return value: the new #GtkFileSystemGnomeVFS object
 **/
GtkFileSystem *
_gtk_file_system_gnome_vfs_new (void)
{
  GtkFileSystemGnomeVFS *system_vfs;
  
  gnome_vfs_init ();
  
  system_vfs = g_object_new (GTK_TYPE_FILE_SYSTEM_GNOME_VFS, NULL);

  system_vfs->locale_encoded_filenames = (getenv ("G_BROKEN_FILENAMES") != NULL);

  system_vfs->folders = g_hash_table_new (g_str_hash, g_str_equal);

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
gtk_file_system_gnome_vfs_iface_init   (GtkFileSystemIface *iface)
{
  iface->list_roots = gtk_file_system_gnome_vfs_list_roots;
  iface->get_folder = gtk_file_system_gnome_vfs_get_folder;
  iface->get_root_info = gtk_file_system_gnome_vfs_get_root_info;
  iface->create_folder = gtk_file_system_gnome_vfs_create_folder;
  iface->get_parent = gtk_file_system_gnome_vfs_get_parent;
  iface->make_uri = gtk_file_system_gnome_vfs_make_uri;
  iface->parse = gtk_file_system_gnome_vfs_parse;
}

static void
gtk_file_system_gnome_vfs_init (GtkFileSystemGnomeVFS *system_gnome_vfs)
{
}

static void
gtk_file_system_gnome_vfs_finalize (GObject *object)
{
  folder_parent_class->finalize (object);
}

static GSList *
gtk_file_system_gnome_vfs_list_roots (GtkFileSystem *file_system)
{
  return g_slist_append (NULL, g_strdup ("file:///"));
}

static GtkFileInfo *
gtk_file_system_gnome_vfs_get_root_info (GtkFileSystem    *file_system,
					 const gchar      *uri,
					 GtkFileInfoType   types,
					 GError          **error)
{
  GnomeVFSResult result;
  GnomeVFSFileInfo *vfs_info;
  GtkFileInfo *info = NULL;
  
  g_return_val_if_fail (strcmp (uri, "file:///") == 0, NULL);

  vfs_info = gnome_vfs_file_info_new ();

  result = gnome_vfs_get_file_info (uri, vfs_info, get_options (types));
  if (result != GNOME_VFS_OK)
    set_vfs_error (result, uri, error);
  else
    info = info_from_vfs_info (vfs_info, types);

  gnome_vfs_file_info_unref (vfs_info);

  return info;
}

static void
ensure_types (GtkFileFolderGnomeVFS *folder_vfs,
	      GtkFileInfoType        types)
{
}


static GtkFileFolder *
gtk_file_system_gnome_vfs_get_folder (GtkFileSystem    *file_system,
				      const gchar      *uri,
				      GtkFileInfoType   types,
				      GError          **error)
{
  GtkFileSystemGnomeVFS *system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);
  GtkFileFolderGnomeVFS *folder_vfs;
  GnomeVFSMonitorHandle *monitor;
  GnomeVFSAsyncHandle *async_handle;
  GnomeVFSResult result;
  gchar *parent_uri;

  folder_vfs = g_hash_table_lookup (system_vfs->folders, uri);
  if (folder_vfs)
    {
      ensure_types (folder_vfs, types);
      return g_object_ref (folder_vfs);
    }

  if (!gtk_file_system_get_parent (file_system, uri, &parent_uri, error))
    return FALSE;

  if (parent_uri)
    {
      GtkFileFolderGnomeVFS *parent_folder;
      GSList *uris;
      
      parent_folder = g_hash_table_lookup (system_vfs->folders, parent_uri);
      g_free (parent_uri);

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
	  
	  g_hash_table_insert (parent_folder->children,
			       child->uri, child);

	  uris = g_slist_prepend (NULL, child->uri);
	  g_signal_emit_by_name (parent_folder, "files_added", uris);
	  g_slist_free (uris);
	}
    }

  folder_vfs = g_object_new (GTK_TYPE_FILE_FOLDER_GNOME_VFS, NULL);
  
  result = gnome_vfs_monitor_add (&monitor,
				  uri,
				  GNOME_VFS_MONITOR_DIRECTORY,
				  monitor_callback, folder_vfs);
  if (result != GNOME_VFS_OK)
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

  return GTK_FILE_FOLDER (folder_vfs);
}

static gboolean
gtk_file_system_gnome_vfs_create_folder (GtkFileSystem    *file_system,
					 const gchar      *uri,
					 GError          **error)
{
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
gtk_file_system_gnome_vfs_get_parent (GtkFileSystem    *file_system,
				      const gchar      *text_uri,
				      gchar           **parent,
				      GError          **error)
{
  GnomeVFSURI *uri = gnome_vfs_uri_new (text_uri);
  GnomeVFSURI *parent_uri;
  
  if (!uri)
    {
      g_set_error (error,
		   GTK_FILE_SYSTEM_ERROR,
		   GTK_FILE_SYSTEM_ERROR_INVALID_URI,
		   "Invalid URI '%s'", text_uri);
      return FALSE;
    }

  parent_uri = gnome_vfs_uri_get_parent (uri);
  if (parent_uri)
    {
      *parent = gnome_vfs_uri_to_string (parent_uri, 0);
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
  
  if (!uri)
    {
      g_set_error (error,
		   GTK_FILE_SYSTEM_ERROR,
		   GTK_FILE_SYSTEM_ERROR_INVALID_URI,
		   "Invalid URI '%s'", base_uri);
      return NULL;
    }

  child_uri = gnome_vfs_uri_append_file_name (uri, child_name);

  result = gnome_vfs_uri_to_string (child_uri, 0);
  
  gnome_vfs_uri_unref (uri);
  gnome_vfs_uri_unref (child_uri);

  return result;
}

static gchar *
gtk_file_system_gnome_vfs_make_uri (GtkFileSystem *file_system,
				    const gchar   *base_uri,
				    const gchar   *display_name,
				    GError       **error)
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

  result = make_child_uri (base_uri, child_name, error);
  
  g_free (child_name);

  return result;
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
gtk_file_system_gnome_vfs_parse (GtkFileSystem  *file_system,
				 const gchar    *base_uri,
				 const gchar    *str,
				 gchar         **folder,
				 gchar         **file_part,
				 GError        **error)
{
  GtkFileSystemGnomeVFS *system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);
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
      *folder = g_strdup (base_uri);
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
      *folder = g_strconcat (scheme, "//", host_and_path_escaped, NULL);
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
	  *folder = uri;

	  result = TRUE;
	}

    }
      
  g_free (stripped);
  
  return result;
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

  if (folder_vfs->uri)
    g_hash_table_remove (folder_vfs->system->folders, folder_vfs->uri);
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
gtk_file_folder_gnome_vfs_get_info (GtkFileFolder  *folder,
				    const gchar    *uri,
				    GError        **error)
{
  GtkFileFolderGnomeVFS *folder_vfs = GTK_FILE_FOLDER_GNOME_VFS (folder);
  FolderChild *child;

  child = g_hash_table_lookup (folder_vfs->children, uri);
  if (!child)
    {
      g_set_error (error,
		   GTK_FILE_SYSTEM_ERROR,
		   GTK_FILE_SYSTEM_ERROR_NONEXISTANT,
		   "'%s' does not exist",
		   uri);

      return FALSE;
    }
  
  return info_from_vfs_info (child->info, folder_vfs->types);
}

static void
list_children_foreach (gpointer key,
		       gpointer value,
		       gpointer user_data)
{
  FolderChild *child = value;
  GSList **list = user_data;

  *list = g_slist_prepend (*list, g_strdup (child->uri));
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

  if (types & GTK_FILE_INFO_MIME_TYPE)
    {
      options |= GNOME_VFS_FILE_INFO_GET_MIME_TYPE;
    }

  return options;
}

static GtkFileInfo *
info_from_vfs_info (GnomeVFSFileInfo *vfs_info,
		    GtkFileInfoType   types)
{
  GtkFileInfo *info = gtk_file_info_new ();

  if (types & GTK_FILE_INFO_DISPLAY_NAME)
    {
      gchar *display_name = g_filename_to_utf8 (vfs_info->name, -1, NULL, NULL, NULL);
      if (!display_name)
	display_name = g_strescape (vfs_info->name, NULL);
      
      gtk_file_info_set_display_name (info, display_name);
      
      g_free (display_name);
    }
  
  gtk_file_info_set_is_hidden (info, vfs_info->name[0] == '.');
  gtk_file_info_set_is_folder (info, vfs_info->type == GNOME_VFS_FILE_TYPE_DIRECTORY);

  if (types & GTK_FILE_INFO_MIME_TYPE)
    {
      gtk_file_info_set_mime_type (info, vfs_info->mime_type);
    }

  gtk_file_info_set_modification_time (info, vfs_info->mtime);
  gtk_file_info_set_size (info, vfs_info->size);
  
  if (types & GTK_FILE_INFO_ICON)
    {
      /* NOT YET IMPLEMENTED */
    }

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
      errcode = GTK_FILE_SYSTEM_ERROR_NONEXISTANT;
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
  GSList *uris = NULL;

  for (tmp_list = list; tmp_list; tmp_list = tmp_list->next)
    {
      GnomeVFSFileInfo *vfs_info;
      gchar *uri;

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

	  g_hash_table_replace (folder_vfs->children,
				child->uri, child);
	  
	  uris = g_slist_prepend (uris, child->uri);
	}
    }

  if (uris)
    {
      g_signal_emit_by_name (folder_vfs, "files_added", uris);
      g_slist_free (uris);
    }
}

static void
monitor_callback (GnomeVFSMonitorHandle   *handle,
		  const gchar             *monitor_uri,
		  const gchar             *info_uri,
		  GnomeVFSMonitorEventType event_type,
		  gpointer                 user_data)
{
  GtkFileFolderGnomeVFS *folder_vfs = user_data;

  switch (event_type)
    {
    case GNOME_VFS_MONITOR_EVENT_CHANGED:
      break;
    case GNOME_VFS_MONITOR_EVENT_DELETED:
      break;
    case GNOME_VFS_MONITOR_EVENT_STARTEXECUTING:
      break;
    case GNOME_VFS_MONITOR_EVENT_STOPEXECUTING:
      break;
    case GNOME_VFS_MONITOR_EVENT_CREATED:
      break;
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

	if (!is_valid_scheme_character (*p)) {
		return FALSE;
	}

	do {
		p++;
	} while (is_valid_scheme_character (*p));

	return *p == ':';
}
