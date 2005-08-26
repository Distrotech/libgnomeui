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

#include <config.h>

#include "gtkfilesystemgnomevfs.h"
#include <gtk/gtkversion.h>

#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-volume-monitor.h>
#include <libgnomevfs/gnome-vfs-drive.h>
#include <glib/gi18n-lib.h>

#include <glib/gstdio.h>

#include <libgnomeui/gnome-icon-lookup.h>
#include <libgnomeui/gnome-authentication-manager-private.h>

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "sucky-desktop-item.h"

#undef PROFILE_FILE_CHOOSER
#ifdef PROFILE_FILE_CHOOSER
#define PROFILE_INDENT 4
static int profile_indent;

static void
profile_add_indent (int indent)
{
  profile_indent += indent;
  if (profile_indent < 0)
    g_error ("You screwed up your indentation");
}

static void
profile_log (const char *func, int indent, const char *msg1, const char *msg2)
{
  char *str;

  if (indent < 0)
    profile_add_indent (indent);

  if (profile_indent == 0)
    str = g_strdup_printf ("MARK: %s %s %s", func, msg1 ? msg1 : "", msg2 ? msg2 : "");
  else
    str = g_strdup_printf ("MARK: %*c %s %s %s", profile_indent - 1, ' ', func, msg1 ? msg1 : "", msg2 ? msg2 : "");

  access (str, F_OK);
  g_free (str);

  if (indent > 0)
    profile_add_indent (indent);
}

#define profile_start(x, y) profile_log (G_STRFUNC, PROFILE_INDENT, x, y)
#define profile_end(x, y) profile_log (G_STRFUNC, -PROFILE_INDENT, x, y)
#define profile_msg(x, y) profile_log (NULL, 0, x, y)
#else
#define profile_start(x, y)
#define profile_end(x, y)
#define profile_msg(x, y)
#endif

#define BOOKMARKS_FILENAME ".gtk-bookmarks"
#define BOOKMARKS_TMP_FILENAME ".gtk-bookmarks-XXXXXX"

typedef struct _GtkFileSystemGnomeVFSClass GtkFileSystemGnomeVFSClass;

#define GTK_FILE_SYSTEM_GNOME_VFS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_FILE_SYSTEM_GNOME_VFS, GtkFileSystemGnomeVFSClass))
#define GTK_IS_FILE_SYSTEM_GNOME_VFS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_FILE_SYSTEM_GNOME_VFS))
#define GTK_FILE_SYSTEM_GNOME_VFS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_FILE_SYSTEM_GNOME_VFS, GtkFileSystemGnomeVFSClass))

#ifdef G_OS_WIN32
#define gnome_authentication_manager_push_async()
#define gnome_authentication_manager_pop_async()
#define gnome_authentication_manager_push_sync()
#define gnome_authentication_manager_pop_sync()
#endif

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
  gulong volume_mounted_id;
  gulong volume_unmounted_id;
  gulong drive_connected_id;
  gulong drive_disconnected_id;

  char *desktop_uri;
  char *home_uri;

  /* For /afs and /net */
  struct stat afs_statbuf;
  struct stat net_statbuf;

  guint have_afs : 1;
  guint have_net : 1;

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

  GHashTable *children; /* NULL if destroyed */

  guint is_afs_or_net : 1;
};

typedef struct _FolderChild FolderChild;

struct _FolderChild
{
  gchar *uri; /* canonical */
  GnomeVFSFileInfo *info;
  guint reloaded : 1; /* used when reloading a folder */
};

/* When getting a folder, we launch two asynchronous processes, the monitor for
 * changes and the async directory load.  Unfortunately, the monitor will feed
 * us all the filenames.  So, to avoid duplicate emissions of the "files-added"
 * signal, we keep a "reloaded" flag in each FolderChild structure.
 *
 * If we re-get a live folder, we don't re-create its monitor, but we do restart
 * its async loader.  When we (re)read the information for a file, we turn on
 * its "reloaded" flag.  When the async load terminates, we purge out all the
 * structures which don't have the flag set, and emit "files-removed" signals
 * for them.
 */

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

static gboolean gtk_file_system_gnome_vfs_insert_bookmark (GtkFileSystem     *file_system,
							   const GtkFilePath *path,
							   gint               position,
							   GError           **error);
static gboolean gtk_file_system_gnome_vfs_remove_bookmark (GtkFileSystem     *file_system,
							   const GtkFilePath *path,
							   GError           **error);
static GSList * gtk_file_system_gnome_vfs_list_bookmarks  (GtkFileSystem *file_system);
static gchar  * gtk_file_system_gnome_vfs_get_bookmark_label (GtkFileSystem     *file_system,
							      const GtkFilePath *path);
static void     gtk_file_system_gnome_vfs_set_bookmark_label (GtkFileSystem     *file_system,
							      const GtkFilePath *path,
							      const gchar       *label);

static GType gtk_file_folder_gnome_vfs_get_type   (void);
static void  gtk_file_folder_gnome_vfs_class_init (GtkFileFolderGnomeVFSClass *class);
static void  gtk_file_folder_gnome_vfs_iface_init (GtkFileFolderIface         *iface);
static void  gtk_file_folder_gnome_vfs_init       (GtkFileFolderGnomeVFS      *impl);
static void  gtk_file_folder_gnome_vfs_finalize   (GObject                    *object);
static void  gtk_file_folder_gnome_vfs_dispose    (GObject                    *object);

static GtkFileInfo *gtk_file_folder_gnome_vfs_get_info      (GtkFileFolder      *folder,
							     const GtkFilePath  *path,
							     GError            **error);
static gboolean     gtk_file_folder_gnome_vfs_list_children (GtkFileFolder      *folder,
							     GSList            **children,
							     GError            **error);

static gboolean     gtk_file_folder_gnome_vfs_is_finished_loading (GtkFileFolder *folder);

static GtkFileInfo *           info_from_vfs_info (const gchar      *uri,
						   GnomeVFSFileInfo *vfs_info,
						   GtkFileInfoType   types,
						   GError          **error);
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

static gchar *make_child_uri (const gchar *base_uri,
			      const gchar *child_name,
			      GError     **error);

static FolderChild *folder_child_new (const char       *uri,
				      GnomeVFSFileInfo *info,
				      gboolean          reloaded);
static void         folder_child_free (FolderChild *child);

static void     set_vfs_error     (GnomeVFSResult   result,
				   const gchar     *uri,
				   GError         **error);
static gboolean has_valid_scheme  (const char      *uri);

static FolderChild *lookup_folder_child (GtkFileFolder     *folder,
					 const GtkFilePath *path,
					 GError           **error);
static FolderChild *lookup_folder_child_from_uri (GtkFileFolder     *folder,
						  const char        *uri,
						  GError           **error);

static GObjectClass *system_parent_class;
static GObjectClass *folder_parent_class;

#define ITEMS_PER_LOCAL_NOTIFICATION 10000
#define ITEMS_PER_REMOTE_NOTIFICATION 100

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
  iface->insert_bookmark = gtk_file_system_gnome_vfs_insert_bookmark;
  iface->remove_bookmark = gtk_file_system_gnome_vfs_remove_bookmark;
  iface->list_bookmarks = gtk_file_system_gnome_vfs_list_bookmarks;
#if GTK_CHECK_VERSION(2,7,0)
  iface->get_bookmark_label = gtk_file_system_gnome_vfs_get_bookmark_label;
  iface->set_bookmark_label = gtk_file_system_gnome_vfs_set_bookmark_label;
#endif
}

static void
gtk_file_system_gnome_vfs_init (GtkFileSystemGnomeVFS *system_vfs)
{
  char *name;

  profile_start ("start", NULL);

  bindtextdomain (GETTEXT_PACKAGE, GNOMEUILOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET 
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
  name = g_build_filename (g_get_home_dir (), "Desktop", NULL);
  system_vfs->desktop_uri = (char *)gtk_file_system_filename_to_path (GTK_FILE_SYSTEM (system_vfs),
								      name);
  g_free (name);
  system_vfs->home_uri = (char *)gtk_file_system_filename_to_path (GTK_FILE_SYSTEM (system_vfs),
								   g_get_home_dir ());
  
  system_vfs->locale_encoded_filenames = (getenv ("G_BROKEN_FILENAMES") != NULL);
  system_vfs->folders = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  system_vfs->volume_monitor = gnome_vfs_get_volume_monitor ();
  system_vfs->volume_mounted_id =
    g_signal_connect_object (system_vfs->volume_monitor, "volume-mounted",
			     G_CALLBACK (volume_mount_unmount_cb), system_vfs, 0);
  system_vfs->volume_unmounted_id =
    g_signal_connect_object (system_vfs->volume_monitor, "volume-unmounted",
			     G_CALLBACK (volume_mount_unmount_cb), system_vfs, 0);
  system_vfs->drive_connected_id =
    g_signal_connect_object (system_vfs->volume_monitor, "drive-connected",
			     G_CALLBACK (drive_connect_disconnect_cb), system_vfs, 0);
  system_vfs->drive_disconnected_id =
    g_signal_connect_object (system_vfs->volume_monitor, "drive-disconnected",
			     G_CALLBACK (drive_connect_disconnect_cb), system_vfs, 0);

  /* Check for AFS */

  if (stat ("/afs", &system_vfs->afs_statbuf) == 0)
    system_vfs->have_afs = TRUE;
  else
    system_vfs->have_afs = FALSE;

  if (stat ("/net", &system_vfs->net_statbuf) == 0)
    system_vfs->have_net = TRUE;
  else
    system_vfs->have_net = FALSE;

  profile_end ("end", NULL);
}

static void
unref_folder (gpointer key, 
	      gpointer value,
	      gpointer data)
{
  g_object_unref (G_OBJECT (value));
}

static void
gtk_file_system_gnome_vfs_finalize (GObject *object)
{
  GtkFileSystemGnomeVFS *system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (object);

  g_free (system_vfs->desktop_uri);
  g_free (system_vfs->home_uri);
  g_hash_table_foreach (system_vfs->folders, unref_folder, NULL);
  g_hash_table_destroy (system_vfs->folders);

  g_signal_handler_disconnect (system_vfs->volume_monitor, system_vfs->volume_mounted_id);
  g_signal_handler_disconnect (system_vfs->volume_monitor, system_vfs->volume_unmounted_id);
  g_signal_handler_disconnect (system_vfs->volume_monitor, system_vfs->drive_connected_id);
  g_signal_handler_disconnect (system_vfs->volume_monitor, system_vfs->drive_disconnected_id);

  /* FIXME: See http://bugzilla.gnome.org/show_bug.cgi?id=151463
   * gnome_vfs_volume_monitor_unref (system_vfs->volume_monitor);
   */

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

  profile_start ("start", NULL);

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
  if (volume)
    result = g_slist_prepend (result, volume);

  profile_end ("end", NULL);

  return result;
}

static GtkFileSystemVolume *
gtk_file_system_gnome_vfs_get_volume_for_path (GtkFileSystem     *file_system,
					       const GtkFilePath *path)
{
  GtkFileSystemGnomeVFS *system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);
  GnomeVFSURI *uri;
  GnomeVFSVolume *volume;

  profile_start ("start", (char *) path);

  uri = gnome_vfs_uri_new (gtk_file_path_get_string (path));
  if (!uri)
    {
      profile_end ("end", (char *) path);
      return NULL;
    }

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

  profile_end ("end", (char *) path);

  return (GtkFileSystemVolume *) volume;
}


static void
unmark_all_fn  (gpointer  key,
		gpointer  value,
		gpointer  user_data)
{
  FolderChild *child;

  child = value;
  child->reloaded = FALSE;
}

static void
load_dir (GtkFileFolderGnomeVFS *folder_vfs)
{
  int num_items;
  GnomeVFSFileInfoOptions vfs_options;

  profile_start ("start", folder_vfs->uri);

  if (folder_vfs->async_handle)
    {
      gnome_vfs_async_cancel (folder_vfs->async_handle);
      g_hash_table_foreach (folder_vfs->children, unmark_all_fn, NULL);
    }

  gnome_authentication_manager_push_async ();

  if (g_str_has_prefix (folder_vfs->uri, "file:"))
    num_items = ITEMS_PER_LOCAL_NOTIFICATION;
  else
    num_items = ITEMS_PER_REMOTE_NOTIFICATION;

  if (folder_vfs->is_afs_or_net)
    vfs_options = GNOME_VFS_FILE_INFO_DEFAULT;
  else
    vfs_options = get_options (folder_vfs->types);

  gnome_vfs_async_load_directory (&folder_vfs->async_handle,
				  folder_vfs->uri,
				  get_options (folder_vfs->types),
				  num_items,
				  GNOME_VFS_PRIORITY_DEFAULT,
				  directory_load_callback, folder_vfs);
  gnome_authentication_manager_pop_async ();

  profile_end ("end", folder_vfs->uri);
}

static GnomeVFSFileInfo *
vfs_info_new_from_afs_or_net_folder (const char *basename)
{
  GnomeVFSFileInfo *vfs_info;

  vfs_info = gnome_vfs_file_info_new ();

  vfs_info->name = g_strdup (basename);
  vfs_info->valid_fields = GNOME_VFS_FILE_INFO_FIELDS_TYPE | GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE;
  vfs_info->type = GNOME_VFS_FILE_TYPE_DIRECTORY;
  vfs_info->mime_type = g_strdup ("x-directory/normal");

  return vfs_info;
}

static void
load_afs_dir (GtkFileFolderGnomeVFS *folder_vfs)
{
  GDir *dir;
  char *pathname;
  char *hostname;
  const char *basename;
  GSList *added_uris;
  GSList *changed_uris;

  g_assert (folder_vfs->is_afs_or_net);

  pathname = g_filename_from_uri (folder_vfs->uri, &hostname, NULL);
  g_assert (pathname != NULL); /* Must be already valid */
  g_assert (hostname == NULL); /* Must be a local path */

  dir = g_dir_open (pathname, 0, NULL);
  if (!dir)
    return;

  added_uris = changed_uris = NULL;

  while ((basename = g_dir_read_name (dir)) != NULL)
    {
      char *uri;
      FolderChild *child;
      GnomeVFSFileInfo *vfs_info;

      uri = make_child_uri (folder_vfs->uri, basename, NULL);
      if (!uri)
	continue;

      vfs_info = vfs_info_new_from_afs_or_net_folder (basename);

      child = g_hash_table_lookup (folder_vfs->children, uri);
      if (child)
	{
	  gnome_vfs_file_info_unref (child->info);

	  child->info = vfs_info;
	  gnome_vfs_file_info_ref (vfs_info);

	  changed_uris = g_slist_prepend (changed_uris, child->uri);
	}
      else
	{
	  child = folder_child_new (uri, vfs_info, FALSE);
	  g_hash_table_insert (folder_vfs->children, child->uri, child);

	  added_uris = g_slist_prepend (added_uris, child->uri);
	}

      gnome_vfs_file_info_unref (vfs_info);
      g_free (uri);
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

/* Returns whether the MIME type of a vfs_info is "application/x-desktop" */
static gboolean
is_desktop_file (GnomeVFSFileInfo *vfs_info)
{
  return ((vfs_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE) != 0
	  && strcmp (gnome_vfs_file_info_get_mime_type (vfs_info), "application/x-desktop") == 0);
}

/* Returns whether a .desktop file indicates a link to a folder */
static gboolean
is_desktop_file_a_folder (SuckyDesktopItem *ditem)
{
  SuckyDesktopItemType ditem_type;

  ditem_type = sucky_desktop_item_get_entry_type (ditem);

  if (ditem_type != SUCKY_DESKTOP_ITEM_TYPE_LINK)
    return FALSE;

  /* FIXME: do we have to get the link URI and figure out its type?  For now,
   * we'll just assume that it does link to a folder...
   */

  return TRUE;
}

/* Checks whether a vfs_info matches what we know about the AFS directories on
 * the system.
 */
static gboolean
is_vfs_info_an_afs_folder (GtkFileSystemGnomeVFS *system_vfs,
			   GnomeVFSFileInfo      *vfs_info)
{
  return (GNOME_VFS_FILE_INFO_LOCAL (vfs_info)
	  && (vfs_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_DEVICE) != 0
	  && (vfs_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_INODE) != 0
	  && ((system_vfs->have_afs
	       && system_vfs->afs_statbuf.st_dev == vfs_info->device
	       && system_vfs->afs_statbuf.st_ino == vfs_info->inode)
	      || (system_vfs->have_net
		  && system_vfs->net_statbuf.st_dev == vfs_info->device
		  && system_vfs->net_statbuf.st_ino == vfs_info->inode)));
}

/* Returns the URL attribute for a .desktop file of type Link */
static char *
get_desktop_link_uri (const char *desktop_uri,
		      GError    **error)
{
  SuckyDesktopItem *ditem;
  const char *ditem_url;
  char *ret_uri;

  ditem = sucky_desktop_item_new_from_uri (desktop_uri, 0, error);
  if (ditem == NULL)
    return NULL;

  ret_uri = NULL;

  if (!is_desktop_file_a_folder (ditem))
    {
      g_set_error (error,
		   GTK_FILE_SYSTEM_ERROR,
		   GTK_FILE_SYSTEM_ERROR_NOT_FOLDER,
		   _("%s is a link to something that is not a folder"),
		   desktop_uri);
      goto out;
    }

  ditem_url = sucky_desktop_item_get_string (ditem, SUCKY_DESKTOP_ITEM_URL);
  if (ditem_url == NULL || strlen (ditem_url) == 0)
    {
      g_set_error (error,
		   GTK_FILE_SYSTEM_ERROR,
		   GTK_FILE_SYSTEM_ERROR_INVALID_URI,
		   _("%s is a link without a destination location"),
		   desktop_uri);
      goto out;
    }

  ret_uri = g_strdup (ditem_url);

 out:

  sucky_desktop_item_unref (ditem);
  return ret_uri;
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
  GnomeVFSMonitorHandle *monitor;
  GnomeVFSResult result;
  GnomeVFSFileInfo *vfs_info;

  profile_start ("start", (char *) path);

  uri = make_uri_canonical (gtk_file_path_get_string (path));
  folder_vfs = g_hash_table_lookup (system_vfs->folders, uri);
  if (folder_vfs)
    {
      folder_vfs->types |= types;
      g_free (uri);
      profile_end ("returning cached folder", NULL);
      return g_object_ref (folder_vfs);
    }

  if (!gtk_file_system_get_parent (file_system, path, &parent_path, error))
    {
      g_free (uri);
      profile_end ("returning NULL because of lack of parent", NULL);
      return NULL;
    }

  vfs_info = NULL;

  if (parent_path)
    {
      char *parent_uri;
      GtkFileFolderGnomeVFS *parent_folder;

      /* If the parent folder is loaded, make sure we exist in it */
      parent_uri = make_uri_canonical (gtk_file_path_get_string (parent_path));
      parent_folder = g_hash_table_lookup (system_vfs->folders, parent_uri);
      g_free (parent_uri);
      gtk_file_path_free (parent_path);

      if (parent_folder)
	{
	  FolderChild *child;

	  child = lookup_folder_child_from_uri (GTK_FILE_FOLDER (parent_folder), uri, error);
	  if (!child)
	    {
	      g_free (uri);
	      profile_end ("returning NULL because lookup_folder_child_from_uri() failed", NULL);
	      return NULL;
	    }

	  vfs_info = child->info;
	  gnome_vfs_file_info_ref (vfs_info);
	  g_assert (vfs_info != NULL);
	}
    }

  if (!vfs_info)
    {
      vfs_info = gnome_vfs_file_info_new ();
      gnome_authentication_manager_push_sync ();
      result = gnome_vfs_get_file_info (uri, vfs_info, get_options (GTK_FILE_INFO_IS_FOLDER));
      gnome_authentication_manager_pop_sync ();

      if (result != GNOME_VFS_OK)
	{
	  set_vfs_error (result, uri, error);
	  gnome_vfs_file_info_unref (vfs_info);
	  g_free (uri);
	  profile_end ("returning NULL because couldn't get file info", NULL);
	  return NULL;
	}
    }

  if (is_desktop_file (vfs_info))
    {
      char *ditem_uri;

      ditem_uri = get_desktop_link_uri (uri, error);
      if (ditem_uri == NULL)
	{
	  g_free (uri);
	  gnome_vfs_file_info_unref (vfs_info);
	  profile_end ("returning NULL because couldn't get desktop link uri", NULL);
	  return NULL;
	}

      g_free (uri);
      uri = ditem_uri;
    }
  else if (vfs_info->type != GNOME_VFS_FILE_TYPE_DIRECTORY)
    {
      g_set_error (error,
		   GTK_FILE_SYSTEM_ERROR,
		   GTK_FILE_SYSTEM_ERROR_NOT_FOLDER,
		   _("%s is not a folder"),
		   uri);
      g_free (uri);
      gnome_vfs_file_info_unref (vfs_info);
      profile_end ("returning NULL because not a folder", NULL);
      return NULL;
    }

  folder_vfs = g_object_new (GTK_TYPE_FILE_FOLDER_GNOME_VFS, NULL);

  folder_vfs->is_afs_or_net = is_vfs_info_an_afs_folder (system_vfs, vfs_info);

  gnome_vfs_file_info_unref (vfs_info);
  vfs_info = NULL;

  monitor = NULL;

  if (!folder_vfs->is_afs_or_net)
    {
      gnome_authentication_manager_push_sync ();
      result = gnome_vfs_monitor_add (&monitor,
				      uri,
				      GNOME_VFS_MONITOR_DIRECTORY,
				      monitor_callback, folder_vfs);
      gnome_authentication_manager_pop_sync ();
      if (result != GNOME_VFS_OK && result != GNOME_VFS_ERROR_NOT_SUPPORTED)
	{
	  g_free (uri);
	  g_object_unref (folder_vfs);
	  set_vfs_error (result, uri, error);
	  profile_end ("returning NULL because couldn't add monitor", NULL);
	  return NULL;
	}
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

  profile_end ("end", (char *) path);

  return GTK_FILE_FOLDER (folder_vfs);
}

static gboolean
gtk_file_system_gnome_vfs_create_folder (GtkFileSystem     *file_system,
					 const GtkFilePath *path,
					 GError           **error)
{
  const gchar *uri = gtk_file_path_get_string (path);
  GnomeVFSResult result;

  gnome_authentication_manager_push_sync ();
  result = gnome_vfs_make_directory (uri,
				     GNOME_VFS_PERM_USER_ALL |
				     GNOME_VFS_PERM_GROUP_ALL |
				     GNOME_VFS_PERM_OTHER_READ);
  gnome_authentication_manager_pop_sync ();

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

  if (GNOME_IS_VFS_DRIVE (volume)) {
    GnomeVFSVolume *vfs_volume;

    /* So... the drive may have grown a volume (from mounting it from
     * the file chooser) which may have a different activation_uri
     * than the drive since it may be mounted anywhere. In fact, the
     * activation_uri is in the drive is only a guess (from
     * e.g. /etc/fstab) at best.. at worst, the activation_uri in the
     * drive may be completely bogus (if the drive was picked up from
     * something different than e.g. /etc/fstab)..
     *
     * Quick solution here is to check if the drive has got a volume,
     * and if so, use that activation_uri.
     */

    vfs_volume = gnome_vfs_drive_get_mounted_volume (GNOME_VFS_DRIVE (volume));
    if (vfs_volume != NULL)
      {
        uri = gnome_vfs_volume_get_activation_uri (vfs_volume);
	gnome_vfs_volume_unref (vfs_volume);
      } 
    else
      {
        uri = gnome_vfs_drive_get_activation_uri (GNOME_VFS_DRIVE (volume));
      }
  } else if (GNOME_IS_VFS_VOLUME (volume))
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

  gdk_threads_enter ();
  
  closure = data;

  closure->succeeded = succeeded;

  if (!succeeded)
    {
      closure->error = g_strdup (error);
      closure->detailed_error = g_strdup (detailed_error);
    }

  g_main_loop_quit (closure->loop);

  gdk_threads_leave ();
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
      gnome_authentication_manager_push_sync ();
      gnome_vfs_drive_mount (GNOME_VFS_DRIVE (volume), volume_mount_cb, &closure);
      gnome_authentication_manager_pop_sync ();

      gdk_threads_leave ();
      g_main_loop_run (closure.loop);
      gdk_threads_enter ();
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
	display_name = g_strdup (_("File System"));
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
		 gint         pixel_size,
		 GError     **error)
{
  GtkIconTheme *icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (widget));
  GHashTable *cache = g_object_get_data (G_OBJECT (icon_theme), "gnome-vfs-gtk-file-icon-cache");
  IconCacheElement *element;

  profile_start ("start", name);

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

      /* Normally, the names are raw icon names, not filenames.  If they happen
       * to be filenames, then they likely came from a .desktop file
       */
      if (g_path_is_absolute (name))
	element->pixbuf = gdk_pixbuf_new_from_file_at_size (name, pixel_size, pixel_size, error);
      else
	element->pixbuf = gtk_icon_theme_load_icon (icon_theme, name, pixel_size, 0, error);
    }

  profile_end ("end", name);

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

  profile_start ("start", NULL);

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
	icon_name = g_strdup ("gnome-dev-harddisk");
      else if (strcmp (uri, system_vfs->desktop_uri) == 0)
	icon_name = g_strdup ("gnome-fs-desktop");
      else if (strcmp (uri, system_vfs->home_uri) == 0)
	icon_name = g_strdup ("gnome-fs-home");
      else
	icon_name = gnome_vfs_volume_get_icon (GNOME_VFS_VOLUME (volume));
      g_free (uri);
    }
  else
    g_warning ("%p is not a valid volume", volume);

  if (icon_name)
    {
      pixbuf = get_cached_icon (widget, icon_name, pixel_size, error);
      g_free (icon_name);
    }
  else
    pixbuf = NULL;

  profile_end ("end", NULL);

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
   * uri).
   */
  stripped = g_strchug (g_strdup (str));

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
      gboolean complete_hostname = TRUE;

      gchar *colon = strchr (stripped, ':');

      scheme = g_strndup (stripped, colon + 1 - stripped);

      if (*(colon + 1) != '/' ||                         /* http:www.gnome.org/asdf */
	  (*(colon + 1) == '/' && *(colon + 2) != '/'))  /* file:/asdf */
	{
	  gchar *first_slash = strchr (colon + 1, '/');

	  host_name = g_strndup (colon + 1, first_slash - (colon + 1));

	  if (first_slash == colon + 1)
	    complete_hostname = FALSE;

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
	      complete_hostname = FALSE;
	      host_name = g_strdup (colon + 3);
	      path = g_strdup ("");
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

      if (complete_hostname)
	{
	  *folder = gtk_file_path_new_steal (g_strconcat (scheme, "//", host_and_path_escaped, NULL));
	  *file_part = file;
	  result = TRUE;
	}
      else
	{
	  /* Don't switch the folder until we have seen the full hostname.
           * Otherwise, the auth dialog will come up for every single character
           * of the hostname being typed in. 
           */
	  *folder = gtk_file_path_copy (base_path);
	  *file_part = g_strdup (stripped);
	  result = TRUE;
	}

      g_free (scheme);
      g_free (host_name);
      g_free (path);
      g_free (host_and_path);
      g_free (host_and_path_escaped);
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
	  if (g_path_is_absolute (filesystem_path))
	      uri = gnome_vfs_get_uri_from_local_path (filesystem_path);
	  else switch (filesystem_path[0])
	    {
#ifndef G_OS_WIN32
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
#endif
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
  gchar *canonical;

  canonical = make_uri_canonical (uri);
  return gtk_file_path_new_steal (canonical);
}

static GtkFilePath *
gtk_file_system_gnome_vfs_filename_to_path (GtkFileSystem *file_system,
					    const gchar   *filename)
{
  gchar *uri;
  
  if (!filename [0])
	  return NULL;

  uri = gnome_vfs_get_uri_from_local_path (filename);
  if (uri)
    {
      gchar *canonical;

      canonical = make_uri_canonical (uri);
      g_free (uri);

      return gtk_file_path_new_steal (canonical);
    }
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

  profile_start ("start", (char *) path);

  system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);

  if (!gtk_file_system_get_parent (file_system, path, &parent_path, NULL))
    {
      profile_end ("end with no parent", (char *) path);
      return NULL;
    }
  
  parent_uri = gtk_file_path_get_string (parent_path);
  info = NULL;
  if (parent_uri != NULL)
    {
      canonical_uri = make_uri_canonical (parent_uri);
      folder_vfs = g_hash_table_lookup (system_vfs->folders, canonical_uri);
      g_free (canonical_uri);
      if (folder_vfs &&
	  (folder_vfs->types & types) == types)
	{
	  FolderChild *child;
	  child = lookup_folder_child (GTK_FILE_FOLDER (folder_vfs), path, NULL);
	  if (child)
	    {
	      info = child->info;
	      gnome_vfs_file_info_ref (info);
	    }
	}
    }
  
  if (info == NULL)
    {
      info = gnome_vfs_file_info_new ();
      uri = gtk_file_path_get_string (path);
      gnome_authentication_manager_push_sync ();
      gnome_vfs_get_file_info (uri, info, get_options (types));
      gnome_authentication_manager_pop_sync ();
    }
  
  gtk_file_path_free (parent_path);

  profile_end ("end", (char *) path);

  return info;
  
}

static GdkPixbuf *
get_icon_from_desktop_file (const char   *desktop_uri,
			    GtkWidget    *widget,
			    gint          pixel_size,
			    GError      **error)
{
  SuckyDesktopItem *ditem;
  const char *ditem_icon_name;
  GdkPixbuf *ret_pixbuf;

  ret_pixbuf = NULL;

  ditem = sucky_desktop_item_new_from_uri (desktop_uri, 0, error);
  if (ditem == NULL)
    goto out;

  ditem_icon_name = sucky_desktop_item_get_string (ditem, SUCKY_DESKTOP_ITEM_ICON);
  if (ditem_icon_name == NULL || strlen (ditem_icon_name) == 0)
    goto out;

  ret_pixbuf = get_cached_icon (widget, ditem_icon_name, pixel_size, error);

 out:

  if (ditem)
    sucky_desktop_item_unref (ditem);

  return ret_pixbuf;
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
  GnomeVFSFileInfo *vfs_info;

  profile_start ("start", (char *) path);

  system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);

  pixbuf = NULL;
  icon_name = NULL;

  vfs_info = get_vfs_info (file_system, path, GTK_FILE_INFO_MIME_TYPE);
  uri = gtk_file_path_get_string (path);

  if (vfs_info && is_desktop_file (vfs_info))
    {
      pixbuf = get_icon_from_desktop_file (uri, widget, pixel_size, error);
      gnome_vfs_file_info_unref (vfs_info);
      profile_end ("end with icon from desktop file", (char *) path);
      return pixbuf;
    }

  if (strcmp (uri, system_vfs->desktop_uri) == 0)
    icon_name = g_strdup ("gnome-fs-desktop");
  else if (strcmp (uri, system_vfs->home_uri) == 0)
    icon_name = g_strdup ("gnome-fs-home");
  else if (strcmp (uri, "trash:///") == 0)
    icon_name = g_strdup ("gnome-fs-trash-empty");
  else if (vfs_info)
    icon_name = gnome_icon_lookup (icon_theme,
				   NULL,
				   uri,
				   NULL,
				   vfs_info,
				   vfs_info->mime_type,
				   GNOME_ICON_LOOKUP_FLAGS_NONE,
				   NULL);
  if (icon_name)
    {
      pixbuf = get_cached_icon (widget, icon_name, pixel_size, error);
      g_free (icon_name);
    }
  else
    pixbuf = NULL; /* FIXME: in this case, we are missing GError information */

  if (vfs_info)
    gnome_vfs_file_info_unref (vfs_info);

  profile_end ("end", (char *) path);

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
  gboolean result = FALSE;

  filename = bookmark_get_filename (FALSE);
  *bookmarks = NULL;

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

      if (g_rename (tmp_filename, filename) == -1)
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
    g_unlink (tmp_filename); /* again, not much error checking we can do here */

 out:

  g_free (filename);
  g_free (tmp_filename);

  return result;
}

static gboolean
gtk_file_system_gnome_vfs_insert_bookmark (GtkFileSystem     *file_system,
					   const GtkFilePath *path,
					   gint               position,
					   GError           **error)
{
  GSList *bookmarks;
  int num_bookmarks;
  GSList *l;
  char *uri;
  gboolean result;
  GError *err;

  profile_start ("start", (char *) path);

  err = NULL;
  if (!bookmark_list_read (&bookmarks, &err) && err->code != G_FILE_ERROR_NOENT)
    {
      g_propagate_error (error, err);
      g_error_free (err);
      profile_end ("end; could not read bookmarks list", NULL);
      return FALSE;
    }

  num_bookmarks = g_slist_length (bookmarks);
  g_return_val_if_fail (position >= -1 && position <= num_bookmarks, FALSE);

  result = FALSE;

  uri = gtk_file_system_gnome_vfs_path_to_uri (file_system, path);

  for (l = bookmarks; l; l = l->next)
    {
      gchar *bookmark, *space;

      bookmark = l->data;

      space = strchr (bookmark, ' ');
      if (space)
	*space = '\0';
      if (strcmp (bookmark, uri) != 0)
	{
	  if (space)
	    *space = ' ';
	}
      else
	{
	  g_set_error (error,
		       GTK_FILE_SYSTEM_ERROR,
		       GTK_FILE_SYSTEM_ERROR_ALREADY_EXISTS,
		       "%s already exists in the bookmarks list",
		       uri);
	  goto out;
	}
    }

  bookmarks = g_slist_insert (bookmarks, g_strdup (uri), position);
  if (bookmark_list_write (bookmarks, error))
    {
      result = TRUE;
      g_signal_emit_by_name (file_system, "bookmarks-changed", 0);
    }

 out:

  g_free (uri);
  bookmark_list_free (bookmarks);

  profile_end ("end", (char *) path);

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

  profile_start ("start", (char *) path);

  if (!bookmark_list_read (&bookmarks, error))
    {
      profile_end ("end, could not read bookmarks list", NULL);
      return FALSE;
    }

  result = FALSE;

  uri = gtk_file_system_path_to_uri (file_system, path);

  for (l = bookmarks; l; l = l->next)
    {
      gchar *bookmark, *space;

      bookmark = l->data;
      space = strchr (bookmark, ' ');
      if (space)
	*space = '\0';

      if (strcmp (bookmark, uri) != 0)
	{
	  if (space)
	    *space = ' ';
	}
      else
	{
	  g_free (l->data);
	  bookmarks = g_slist_remove_link (bookmarks, l);
	  g_slist_free_1 (l);

	  if (bookmark_list_write (bookmarks, error))
	    {
	      result = TRUE;
	      g_signal_emit_by_name (file_system, "bookmarks-changed", 0);
	    }

	  goto out;
	}
    }

  g_set_error (error,
	       GTK_FILE_SYSTEM_ERROR,
	       GTK_FILE_SYSTEM_ERROR_NONEXISTENT,
	       "%s does not exist in the bookmarks list",
	       uri);

 out:

  g_free (uri);
  bookmark_list_free (bookmarks);

  profile_end ("end", (char *) path);

  return result;
}

static GSList *
gtk_file_system_gnome_vfs_list_bookmarks (GtkFileSystem *file_system)
{
  GSList *bookmarks;
  GSList *result;
  GSList *l;

  profile_start ("start", NULL);

  if (!bookmark_list_read (&bookmarks, NULL))
    {
      profile_end ("end, could not read bookmarks list", NULL);
      return NULL;
    }

  result = NULL;

  for (l = bookmarks; l; l = l->next)
    {
      gchar *bookmark, *space;

      bookmark = l->data;
      space = strchr (bookmark, ' ');
      if (space)
	*space = '\0';
      result = g_slist_prepend (result, gtk_file_system_uri_to_path (file_system, bookmark));
    }

  bookmark_list_free (bookmarks);

  result = g_slist_reverse (result);

  profile_end ("end", NULL);

  return result;
}

static gchar *
gtk_file_system_gnome_vfs_get_bookmark_label (GtkFileSystem     *file_system,
					      const GtkFilePath *path)
{
  GSList *bookmarks;
  gchar *label;
  GSList *l;
  gchar *bookmark, *space, *uri;

  profile_start ("start", (char *) path);
  
  if (!bookmark_list_read (&bookmarks, NULL))
    {
      profile_end ("end, could not read bookmarks list", NULL);
      return NULL;
    }

  uri = gtk_file_system_path_to_uri (file_system, path);

  label = NULL;
  for (l = bookmarks; l && !label; l = l->next) 
    {
      bookmark = l->data;
      space = strchr (bookmark, ' ');
      if (!space)
	continue;

      *space = '\0';

      if (strcmp (uri, bookmark) == 0)
	label = g_strdup (space + 1);
    }

  g_free (uri);
  bookmark_list_free (bookmarks);

  profile_end ("end", (char *) path);

  return label;
}

static void
gtk_file_system_gnome_vfs_set_bookmark_label (GtkFileSystem     *file_system,
					      const GtkFilePath *path,
					      const gchar       *label)
{
  GSList *bookmarks;
  GSList *l;
  gchar *bookmark, *space, *uri;
  gboolean found;

  profile_start ("start", (char *) path);

  if (!bookmark_list_read (&bookmarks, NULL))
    {
      profile_end ("end, could not read bookmarks list", NULL);
      return;
    }

  uri = gtk_file_system_path_to_uri (file_system, path);

  found = FALSE;
  for (l = bookmarks; l && !found; l = l->next) 
    {
      bookmark = l->data;
      space = strchr (bookmark, ' ');
      if (space)
	*space = '\0';

      if (strcmp (bookmark, uri) != 0)
	{
	  if (space)
	    *space = ' ';
	}
      else
	{
	  g_free (bookmark);
	  
	  if (label && *label)
	    l->data = g_strdup_printf ("%s %s", uri, label);
	  else
	    l->data = g_strdup (uri);

	  found = TRUE;
	  break;
	}
    }

  if (found)
    {
      if (bookmark_list_write (bookmarks, NULL))
	g_signal_emit_by_name (file_system, "bookmarks-changed", 0);
    }
  
  g_free (uri);
  bookmark_list_free (bookmarks);

  profile_end ("end", NULL);
}

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
  gobject_class->dispose = gtk_file_folder_gnome_vfs_dispose;
}

static void
gtk_file_folder_gnome_vfs_iface_init (GtkFileFolderIface *iface)
{
  iface->get_info = gtk_file_folder_gnome_vfs_get_info;
  iface->list_children = gtk_file_folder_gnome_vfs_list_children;
  iface->is_finished_loading = gtk_file_folder_gnome_vfs_is_finished_loading;
}

static void
gtk_file_folder_gnome_vfs_init (GtkFileFolderGnomeVFS *impl)
{
}

static gboolean
unref_at_idle (GObject *object)
{
  g_object_unref (object);
  return FALSE;
}

static void
gtk_file_folder_gnome_vfs_dispose (GObject *object)
{
  GtkFileFolderGnomeVFS *folder_vfs = GTK_FILE_FOLDER_GNOME_VFS (object);
  GtkFileSystemGnomeVFS *system_vfs = folder_vfs->system;
  gboolean was_destroyed;

  was_destroyed = folder_vfs->children == NULL;
  
  if (folder_vfs->uri)
    g_hash_table_remove (system_vfs->folders, folder_vfs->uri);
  folder_vfs->uri = NULL;
  
  if (folder_vfs->async_handle)
    gnome_vfs_async_cancel (folder_vfs->async_handle);
  folder_vfs->async_handle = NULL;
  
  if (folder_vfs->monitor)
    gnome_vfs_monitor_cancel (folder_vfs->monitor);
  folder_vfs->monitor = NULL;
  
  if (folder_vfs->children)
    g_hash_table_destroy (folder_vfs->children);
  folder_vfs->children = NULL;

  if (!was_destroyed)
    {
      /* The folder is now marked as destroyed, we
       * free it at idle time to avoid races with the
       * cancelled async job. See gnome_vfs_async_cancel()
       */
      g_object_ref (object);
      g_idle_add ((GSourceFunc)unref_at_idle, object);
    }
}


static void
gtk_file_folder_gnome_vfs_finalize (GObject *object)
{
  folder_parent_class->finalize (object);
}

/* The URI must be canonicalized already */
static FolderChild *
lookup_folder_child_from_uri (GtkFileFolder *folder,
			      const char    *uri,
			      GError       **error)
{
  GtkFileFolderGnomeVFS *folder_vfs = GTK_FILE_FOLDER_GNOME_VFS (folder);
  FolderChild *child;
  GnomeVFSFileInfo *vfs_info;
  GnomeVFSResult result;

  profile_start ("start", uri);

  child = g_hash_table_lookup (folder_vfs->children, uri);
  if (child)
    {
      profile_end ("end, returning cached child", uri);
      return child;
    }

  /* We may not have loaded the file's information yet.
   */

  vfs_info = gnome_vfs_file_info_new ();
  gnome_authentication_manager_push_sync ();
  result = gnome_vfs_get_file_info (uri, vfs_info, get_options (folder_vfs->types));
  gnome_authentication_manager_pop_sync ();

  if (result != GNOME_VFS_OK)
    {
      set_vfs_error (result, uri, error);
      gnome_vfs_file_info_unref (vfs_info);
      profile_end ("end, could not get file info", uri);
      return NULL;
    }
  else
    {
      GSList *uris;

      child = folder_child_new (uri, vfs_info, folder_vfs->async_handle ? TRUE : FALSE);
      gnome_vfs_file_info_unref (vfs_info);
      g_hash_table_replace (folder_vfs->children, child->uri, child);

      uris = g_slist_append (NULL, (char *) uri);
      g_signal_emit_by_name (folder_vfs, "files-added", uris);
      g_slist_free (uris);

      profile_end ("end", uri);

      return child;
    }
}

static FolderChild *
lookup_folder_child (GtkFileFolder     *folder,
		     const GtkFilePath *path,
		     GError           **error)
{
  char *uri;
  FolderChild *child;

  uri = make_uri_canonical (gtk_file_path_get_string (path));
  child = lookup_folder_child_from_uri (folder, uri, error);
  g_free (uri);

  return child;
}

static GtkFileInfo *
gtk_file_folder_gnome_vfs_get_info (GtkFileFolder     *folder,
				    const GtkFilePath *path,
				    GError           **error)
{
  GtkFileFolderGnomeVFS *folder_vfs = GTK_FILE_FOLDER_GNOME_VFS (folder);
  const gchar *uri = gtk_file_path_get_string (path);
  FolderChild *child;
  GtkFileInfo *file_info;

  profile_start ("start", (char *) path);

  if (!path)
    {
      GnomeVFSURI *vfs_uri;
      GnomeVFSResult result;
      GnomeVFSFileInfo *info;

      vfs_uri = gnome_vfs_uri_new (folder_vfs->uri);
      g_assert (vfs_uri != NULL);

      g_return_val_if_fail (!gnome_vfs_uri_has_parent (vfs_uri), NULL);
      gnome_vfs_uri_unref (vfs_uri);

      info = gnome_vfs_file_info_new ();
      gnome_authentication_manager_push_sync ();
      result = gnome_vfs_get_file_info (folder_vfs->uri, info, get_options (GTK_FILE_INFO_ALL));
      gnome_authentication_manager_pop_sync ();
      if (result != GNOME_VFS_OK)
	{
	  file_info = NULL;
	  set_vfs_error (result, folder_vfs->uri, error);
	}
      else
	file_info = info_from_vfs_info (folder_vfs->uri, info, GTK_FILE_INFO_ALL, error);

      gnome_vfs_file_info_unref (info);

      profile_end ("end for non-child info", (char *) path);

      return file_info;
    }

  child = lookup_folder_child (folder, path, error);

  if (child)
    file_info = info_from_vfs_info (uri, child->info, folder_vfs->types, error);
  else
    file_info = NULL;

  profile_end ("end", (char *) path);

  return file_info;
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

  profile_start ("start", folder_vfs->uri);

  if (folder_vfs->is_afs_or_net)
    load_afs_dir (folder_vfs);
  else
    load_dir (folder_vfs);
  
  *children = NULL;

  g_hash_table_foreach (folder_vfs->children, list_children_foreach, children);

  profile_end ("end", folder_vfs->uri);

  return TRUE;
}

static gboolean
gtk_file_folder_gnome_vfs_is_finished_loading (GtkFileFolder *folder)
{
  GtkFileFolderGnomeVFS *folder_vfs = GTK_FILE_FOLDER_GNOME_VFS (folder);

  return (folder_vfs->async_handle == NULL);
}


static GnomeVFSFileInfoOptions
get_options (GtkFileInfoType types)
{
  GnomeVFSFileInfoOptions options = GNOME_VFS_FILE_INFO_FOLLOW_LINKS;

  /* We need the MIME type regardless of what was requested, because we need to
   * be able to distinguish .desktop files (application/x-desktop).  FIXME: can
   * we get away with passing GNOME_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE?
   */
  options |= GNOME_VFS_FILE_INFO_GET_MIME_TYPE;

  return options;
}

static GtkFileInfo *
info_from_vfs_info (const gchar      *uri,
		    GnomeVFSFileInfo *vfs_info,
		    GtkFileInfoType   types,
		    GError          **error)
{
  GtkFileInfo *info = gtk_file_info_new ();
  gboolean is_desktop;
  SuckyDesktopItem *ditem;

  /* Desktop files are special.  They can override the name, type, icon, etc. */

  is_desktop = is_desktop_file (vfs_info);

  ditem = NULL; /* shut up GCC */

  if (is_desktop)
    {
      ditem = sucky_desktop_item_new_from_uri (uri, 0, error);
      if (ditem == NULL)
	return NULL;
    }

  /* Display name */

  if (types & GTK_FILE_INFO_DISPLAY_NAME)
    {
      if (is_desktop)
	{
	  const char *name;

	  name = sucky_desktop_item_get_localestring (ditem, SUCKY_DESKTOP_ITEM_NAME);
	  if (name != NULL)
	    gtk_file_info_set_display_name (info, name);
	  else
	    goto fetch_name_from_uri;
	}
      else if (!vfs_info->name || strcmp (vfs_info->name, "/") == 0)
	{
	  if (strcmp (uri, "file:///") == 0)
	    gtk_file_info_set_display_name (info, "/");
	  else
	    gtk_file_info_set_display_name (info, uri);
	}
      else
	{
	  char *local_file;
	  char *display_name;

	fetch_name_from_uri:

	  local_file = gnome_vfs_get_local_path_from_uri (uri);
	  if (local_file != NULL)
	    {
	      display_name = g_filename_display_basename (local_file);
	      g_free (local_file);
	    }
	  else
	    {
	      display_name = g_filename_display_name (vfs_info->name);
	    }

	  gtk_file_info_set_display_name (info, display_name);

	  g_free (display_name);
	}
    }

  /* Hidden */

  if (types & GTK_FILE_INFO_IS_HIDDEN)
    {
      gboolean is_hidden;

      if (is_desktop)
	is_hidden = sucky_desktop_item_get_boolean (ditem, SUCKY_DESKTOP_ITEM_HIDDEN);
      else
	is_hidden = (vfs_info->name && vfs_info->name[0] == '.');

      gtk_file_info_set_is_hidden (info, is_hidden);
    }

  /* Folder */

  if (types & GTK_FILE_INFO_IS_FOLDER)
    {
      gboolean is_folder;

      if (is_desktop)
	is_folder = is_desktop_file_a_folder (ditem);
      else
	is_folder = (vfs_info->type == GNOME_VFS_FILE_TYPE_DIRECTORY);

      gtk_file_info_set_is_folder (info, is_folder);
    }

  if (types & GTK_FILE_INFO_MIME_TYPE)
    {
      const char *mime_type;

      if (is_desktop)
	mime_type = "application/x-desktop"; /* FIXME: do we have to get the link URI and figure out its type? */
      else
	mime_type = vfs_info->mime_type;

      gtk_file_info_set_mime_type (info, mime_type);
    }

  gtk_file_info_set_modification_time (info, vfs_info->mtime);
  gtk_file_info_set_size (info, vfs_info->size);

  if (is_desktop)
    sucky_desktop_item_unref (ditem);

  return info;
}

static FolderChild *
folder_child_new (const char       *uri,
		  GnomeVFSFileInfo *info,
		  gboolean          reloaded)
{
  FolderChild *child;

  child = g_new (FolderChild, 1);
  child->uri = g_strdup (uri);
  child->info = info;
  child->reloaded = reloaded ? TRUE : FALSE;
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
  gdk_threads_enter ();
  g_signal_emit_by_name (system_vfs, "volumes-changed");
  gdk_threads_leave ();
}

/* Callback used when a drive gets connected or disconnected */
static void
drive_connect_disconnect_cb (GnomeVFSVolumeMonitor *monitor,
			     GnomeVFSDrive         *drive,
			     GtkFileSystemGnomeVFS *system_vfs)
{
  gdk_threads_enter ();
  g_signal_emit_by_name (system_vfs, "volumes-changed");
  gdk_threads_leave ();
}

struct purge_closure
{
  GSList *removed_uris;
};

/* Used from g_hash_table_foreach() in folder_purge_and_unmark() */
static gboolean
purge_fn (gpointer key,
	  gpointer value,
	  gpointer data)
{
  struct purge_closure *closure;
  FolderChild *child;

  child = value;
  closure = data;

  if (child->reloaded)
    {
      child->reloaded = FALSE;
      return FALSE;
    }
  else
    {
      closure->removed_uris = g_slist_prepend (closure->removed_uris, child->uri);

      if (child->info)
	gnome_vfs_file_info_unref (child->info);

      g_free (child);

      return TRUE;
    }
}

/* Purges out all the files that don't have the "reloaded" flag set */
static void
folder_purge_and_unmark (GtkFileFolderGnomeVFS *folder_vfs)
{
  struct purge_closure closure;

  closure.removed_uris = NULL;
  g_hash_table_foreach_steal (folder_vfs->children, purge_fn, &closure);

  if (closure.removed_uris)
    {
      GSList *l;

      closure.removed_uris = g_slist_reverse (closure.removed_uris);
      g_signal_emit_by_name (folder_vfs, "files-removed", closure.removed_uris);
      
      for (l = closure.removed_uris; l; l = l->next)
	{
	  char *uri;

	  uri = l->data;
	  g_free (uri);
	}

      g_slist_free (closure.removed_uris);
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

  profile_start ("start", NULL);

  /* This is called from an idle, we need to protect is as such */
  gdk_threads_enter ();

  /* See if this folder is already destroyed.
   * This can happen if we cancel in another thread while
   * directory_load_callback is just being called
   */
  if (folder_vfs->children == NULL)
    {
      profile_end ("end", NULL);
      gdk_threads_leave ();
      return;
    }
  
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
	  FolderChild *child;

	  child = g_hash_table_lookup (folder_vfs->children, uri);
	  if (child)
	    {
	      child->reloaded = TRUE;

	      if (child->info)
		gnome_vfs_file_info_unref (child->info);

	      child->info = vfs_info;
	      gnome_vfs_file_info_ref (child->info);

	      changed_uris = g_slist_prepend (changed_uris, child->uri);
	    }
	  else
	    {
	      child = folder_child_new (uri, vfs_info, TRUE);
	      g_hash_table_insert (folder_vfs->children, child->uri, child);

	      added_uris = g_slist_prepend (added_uris, child->uri);
	    }

	  g_free (uri);
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
    {
      folder_vfs->async_handle = NULL;

      g_signal_emit_by_name (folder_vfs, "finished-loading");
      folder_purge_and_unmark (folder_vfs);
    }

  gdk_threads_leave ();

  profile_end ("end", NULL);
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

  profile_start ("start", info_uri);

  /* This is called from an idle, we need to protect is as such */
  gdk_threads_enter ();

  /* Check if destroyed */
  if (folder_vfs->children == NULL)
    {
      gdk_threads_leave ();
      profile_end ("end", info_uri);
      return;
    }
  
  switch (event_type)
    {
    case GNOME_VFS_MONITOR_EVENT_CHANGED:
    case GNOME_VFS_MONITOR_EVENT_CREATED:
      {
	GnomeVFSResult result;
	GnomeVFSFileInfo *vfs_info;

	vfs_info = gnome_vfs_file_info_new ();

	gnome_authentication_manager_push_sync ();
	result = gnome_vfs_get_file_info (info_uri, vfs_info, get_options (folder_vfs->types));
	gnome_authentication_manager_pop_sync ();

	if (result == GNOME_VFS_OK)
	  {
	    FolderChild *child;

	    child = g_hash_table_lookup (folder_vfs->children, info_uri);

	    if (child)
	      {
		if (folder_vfs->async_handle)
		  child->reloaded = TRUE;

		if (child->info)
		  gnome_vfs_file_info_unref (child->info);

		child->info = vfs_info;
	      }
	    else
	      {
		child = folder_child_new (info_uri, vfs_info, folder_vfs->async_handle ? TRUE : FALSE);
		gnome_vfs_file_info_unref (vfs_info);
		g_hash_table_insert (folder_vfs->children, child->uri, child);
	      }

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

  gdk_threads_leave ();

  profile_end ("end", info_uri);
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

  /* We ref the class so that the module won't get unloaded.  We need this
   * because gnome_icon_lookup() will cause GnomeIconTheme to get registered,
   * and we cannot unload that properly.  I'd prefer to do this via
   * g_module_make_resident(), but here we don't have access to the GModule.
   * See bug #139254 for reference. [federico]
   */

  g_type_class_ref (type_gtk_file_system_gnome_vfs);
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
