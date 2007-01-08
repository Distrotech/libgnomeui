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

/* #define this if you want the program to crash when a file system gets
 * finalized while async handles are still outstanding.
 */
#undef HANDLE_ME_HARDER

#include <config.h>

#include "gtkfilesystemgnomevfs.h"
#include <gtk/gtkversion.h>

#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
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

#define DESKTOP_GROUP "Desktop Entry"

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
static GType type_gtk_file_system_handle_gnome_vfs = 0;

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

  GHashTable *handles;

  guint execute_vfs_callbacks_idle_id;
  GSList *vfs_callbacks;

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
  guint finished_loading : 1;
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
static void gtk_file_system_gnome_vfs_dispose    (GObject                    *object);
static void gtk_file_system_gnome_vfs_finalize   (GObject                    *object);


#define GTK_TYPE_FILE_SYSTEM_HANDLE_GNOME_VFS             (gtk_file_system_handle_gnome_vfs_get_type())
#define GTK_FILE_SYSTEM_HANDLE_GNOME_VFS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_FILE_SYSTEM_HANDLE_GNOME_VFS, GtkFileSystemHandleGnomeVFS))
#define GTK_IS_FILE_SYSTEM_HANDLE_GNOME_VFS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_FILE_SYSTEM_HANDLE_GNOME_VFS))
#define GTK_FILE_SYSTEM_HANDLE_GNOME_VFS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_FILE_SYSTEM_HANDLE_GNOME_VFS, GtkFileSystemHandleGnomeVFSClass))
#define GTK_IS_FILE_SYSTEM_HANDLE_GNOME_VFS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_FILE_SYSTEM_HANDLE_GNOME_VFS))
#define GTK_FILE_SYSTEM_HANDLE_GNOME_VFS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_FILE_SYSTEM_HANDLE_GNOME_VFS, GtkFileSystemHandleGnomeVFSClass))

typedef struct _GtkFileSystemHandleGnomeVFS      GtkFileSystemHandleGnomeVFS;
typedef struct _GtkFileSystemHandleGnomeVFSClass GtkFileSystemHandleGnomeVFSClass;

struct _GtkFileSystemHandleGnomeVFS
{
  GtkFileSystemHandle parent_instance;

  GnomeVFSAsyncHandle *vfs_handle;

  gint callback_type;
  gpointer callback_data;
};

struct _GtkFileSystemHandleGnomeVFSClass
{
  GtkFileSystemHandleClass parent_class;
};



static GSList *             gtk_file_system_gnome_vfs_list_volumes        (GtkFileSystem     *file_system);
static GtkFileSystemVolume *gtk_file_system_gnome_vfs_get_volume_for_path (GtkFileSystem     *file_system,
									   const GtkFilePath *path);

static GtkFileSystemHandle *gtk_file_system_gnome_vfs_get_folder (GtkFileSystem                  *file_system,
								  const GtkFilePath              *path,
								  GtkFileInfoType                 types,
								  GtkFileSystemGetFolderCallback  callback,
								  gpointer                        data);
static GtkFileSystemHandle *gtk_file_system_gnome_vfs_get_info   (GtkFileSystem                  *file_system,
								  const GtkFilePath              *path,
								  GtkFileInfoType                 types,
								  GtkFileSystemGetInfoCallback    callback,
								  gpointer                        data);
static GtkFileSystemHandle *gtk_file_system_gnome_vfs_create_folder (GtkFileSystem                     *file_system,
								     const GtkFilePath                 *path,
								     GtkFileSystemCreateFolderCallback  callback,
								     gpointer                           data);
static void                 gtk_file_system_gnome_vfs_cancel_operation (GtkFileSystemHandle               *handle);

static void         gtk_file_system_gnome_vfs_volume_free             (GtkFileSystem       *file_system,
								       GtkFileSystemVolume *volume);
static GtkFilePath *gtk_file_system_gnome_vfs_volume_get_base_path    (GtkFileSystem       *file_system,
								       GtkFileSystemVolume *volume);
static gboolean     gtk_file_system_gnome_vfs_volume_get_is_mounted   (GtkFileSystem       *file_system,
								       GtkFileSystemVolume *volume);
static GtkFileSystemHandle *gtk_file_system_gnome_vfs_volume_mount    (GtkFileSystem                    *file_system,
								       GtkFileSystemVolume              *volume,
								       GtkFileSystemVolumeMountCallback  callback,
								       gpointer                          data);
static gchar *      gtk_file_system_gnome_vfs_volume_get_display_name (GtkFileSystem       *file_system,
								       GtkFileSystemVolume *volume);
static gchar *      gtk_file_system_gnome_vfs_volume_get_icon_name    (GtkFileSystem        *file_system,
								       GtkFileSystemVolume  *volume,
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

static GType gtk_file_system_handle_gnome_vfs_get_type (void);
static void  gtk_file_system_handle_gnome_vfs_class_init (GtkFileSystemHandleGnomeVFSClass *class);
static void  gtk_file_system_handle_gnome_vfs_init     (GtkFileSystemHandleGnomeVFS *handle);
static void  gtk_file_system_handle_gnome_vfs_finalize (GObject *object);
static GtkFileSystemHandleGnomeVFS *gtk_file_system_handle_gnome_vfs_new (GtkFileSystem *file_system);

static GtkFileInfo *gtk_file_folder_gnome_vfs_get_info      (GtkFileFolder      *folder,
							     const GtkFilePath  *path,
							     GError            **error);
static gboolean     gtk_file_folder_gnome_vfs_list_children (GtkFileFolder      *folder,
							     GSList            **children,
							     GError            **error);

static gboolean     gtk_file_folder_gnome_vfs_is_finished_loading (GtkFileFolder *folder);

static GtkFileInfo *           info_from_vfs_info (GtkFileSystemGnomeVFS  *system_vfs,
						   const gchar            *uri,
						   GnomeVFSFileInfo       *vfs_info,
						   GtkFileInfoType         types,
						   GError                **error);
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

static gboolean execute_vfs_callbacks_idle (gpointer data);
static void     execute_vfs_callbacks      (gpointer data);

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
					 const GtkFilePath *path);
static FolderChild *lookup_folder_child_from_uri (GtkFileFolder     *folder,
						  const char        *uri);

static GObjectClass *system_parent_class;
static GObjectClass *folder_parent_class;
static GObjectClass *handle_parent_class;

#define ITEMS_PER_LOCAL_NOTIFICATION 10000
#define ITEMS_PER_REMOTE_NOTIFICATION 100

/* The pointers we return for a GtkFileSystemVolume are opaque tokens; they are
 * really pointers to GnomeVFSDrive or GnomeVFSVolume objects.  We need an extra
 * token for the fake "Network Servers" volume.  So, we'll return the pointer to
 * this particular string.
 */
static const char *network_servers_volume_token = "Network Servers";
#define IS_NETWORK_SERVERS_VOLUME_TOKEN(volume) ((gpointer) (volume) == (gpointer) network_servers_volume_token)

typedef void (* VfsIdleCallback) (gpointer data);

struct vfs_idle_callback
{
  VfsIdleCallback callback;
  gpointer callback_data;
};

static void queue_vfs_idle_callback (GtkFileSystemGnomeVFS *system_vfs, VfsIdleCallback callback, gpointer callback_data);


enum
{
  CALLBACK_GET_FOLDER = 1,
  CALLBACK_GET_FILE_INFO,
  CALLBACK_CREATE_FOLDER,
  CALLBACK_VOLUME_MOUNT
};

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

  gobject_class->dispose = gtk_file_system_gnome_vfs_dispose;
  gobject_class->finalize = gtk_file_system_gnome_vfs_finalize;
}

static void
gtk_file_system_gnome_vfs_iface_init (GtkFileSystemIface *iface)
{
  iface->list_volumes = gtk_file_system_gnome_vfs_list_volumes;
  iface->get_volume_for_path = gtk_file_system_gnome_vfs_get_volume_for_path;
  iface->get_folder = gtk_file_system_gnome_vfs_get_folder;
  iface->get_info = gtk_file_system_gnome_vfs_get_info;
  iface->create_folder = gtk_file_system_gnome_vfs_create_folder;
  iface->cancel_operation = gtk_file_system_gnome_vfs_cancel_operation;
  iface->volume_free = gtk_file_system_gnome_vfs_volume_free;
  iface->volume_get_base_path = gtk_file_system_gnome_vfs_volume_get_base_path;
  iface->volume_get_is_mounted = gtk_file_system_gnome_vfs_volume_get_is_mounted;
  iface->volume_mount = gtk_file_system_gnome_vfs_volume_mount;
  iface->volume_get_display_name = gtk_file_system_gnome_vfs_volume_get_display_name;
  iface->volume_get_icon_name = gtk_file_system_gnome_vfs_volume_get_icon_name;
  iface->get_parent = gtk_file_system_gnome_vfs_get_parent;
  iface->make_path = gtk_file_system_gnome_vfs_make_path;
  iface->parse = gtk_file_system_gnome_vfs_parse;
  iface->path_to_uri = gtk_file_system_gnome_vfs_path_to_uri;
  iface->path_to_filename = gtk_file_system_gnome_vfs_path_to_filename;
  iface->uri_to_path = gtk_file_system_gnome_vfs_uri_to_path;
  iface->filename_to_path = gtk_file_system_gnome_vfs_filename_to_path;
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

  profile_start ("gnome_vfs_get_volume_monitor start", NULL);
  system_vfs->volume_monitor = gnome_vfs_get_volume_monitor ();
  profile_end ("gnome_vfs_get_volume_monitor end", NULL);

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

  system_vfs->handles = g_hash_table_new (g_direct_hash, g_direct_equal);

  system_vfs->execute_vfs_callbacks_idle_id = 0;
  system_vfs->vfs_callbacks = NULL;

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
check_handle_fn (gpointer key, gpointer value, gpointer data)
{
  GtkFileSystemHandleGnomeVFS *handle;
  int *num_live_handles;

  handle = key;
  num_live_handles = data;

  (*num_live_handles)++;

  g_warning ("file_system_gnome_vfs=%p still has handle=%p at finalization which is %s!",
	     GTK_FILE_SYSTEM_HANDLE (handle)->file_system,
	     handle,
	     GTK_FILE_SYSTEM_HANDLE (handle)->cancelled ? "CANCELLED" : "NOT CANCELLED");
}

static void
check_handles_at_finalization (GtkFileSystemGnomeVFS *system_vfs)
{
  int num_live_handles;

  num_live_handles = 0;

  g_hash_table_foreach (system_vfs->handles, check_handle_fn, &num_live_handles);
#ifdef HANDLE_ME_HARDER
  g_assert (num_live_handles == 0);
#endif

  g_hash_table_destroy (system_vfs->handles);
  system_vfs->handles = NULL;
}

static void
handle_cancel_operation_fn (gpointer key, gpointer value, gpointer data)
{
  GtkFileSystemHandleGnomeVFS *handle;

  handle = key;

  if (handle->vfs_handle)
    gnome_vfs_async_cancel (handle->vfs_handle);
  handle->vfs_handle = NULL;
}

static void
gtk_file_system_gnome_vfs_dispose (GObject *object)
{
  GtkFileSystemGnomeVFS *system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (object);

  if (system_vfs->execute_vfs_callbacks_idle_id)
    {
      g_source_remove (system_vfs->execute_vfs_callbacks_idle_id);
      system_vfs->execute_vfs_callbacks_idle_id = 0;

      /* call pending callbacks */
      execute_vfs_callbacks (system_vfs);
    }

  /* cancel pending VFS operations */
  g_hash_table_foreach (system_vfs->handles, handle_cancel_operation_fn, NULL);

  system_parent_class->dispose (object);
}

static void
gtk_file_system_gnome_vfs_finalize (GObject *object)
{
  GtkFileSystemGnomeVFS *system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (object);

  check_handles_at_finalization (system_vfs);

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

  /* Network Servers */

  result = g_slist_prepend (result, (gpointer) network_servers_volume_token);

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
load_dir (GtkFileFolderGnomeVFS *folder_vfs)
{
  int num_items;

  profile_start ("start", folder_vfs->uri);

  if (folder_vfs->async_handle || folder_vfs->finished_loading)
    return;

  gnome_authentication_manager_push_async ();

  if (g_str_has_prefix (folder_vfs->uri, "file:"))
    num_items = ITEMS_PER_LOCAL_NOTIFICATION;
  else
    num_items = ITEMS_PER_REMOTE_NOTIFICATION;

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


static gboolean
uri_is_hostname_with_trailing_slash (const char *uri)
{
  char *p;

  p = strstr (uri, "://");
  if (p == NULL)
    {
      return FALSE;
    }

  p+=3;

  p = strstr (p, "/");
  if (p != NULL && *(p + 1) == '\0')
    {
      return TRUE;
    }

  return FALSE;
}

static char *
make_uri_canonical (const char *uri)
{
  char *canonical;
  int len;
  
  canonical = gnome_vfs_make_uri_canonical (uri);

  /* Strip terminating / unless its foo:/ or foo:///,
   * or foo://bar/ */
  len = strlen (canonical);
  if (len > 2 &&
      canonical[len-1] == '/' &&
      canonical[len-2] != '/' &&
      canonical[len-2] != ':' &&
      !uri_is_hostname_with_trailing_slash (canonical))
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
is_desktop_file_a_folder (GKeyFile *desktop_file)
{
  gboolean ret;
  gchar *type;

  type = g_key_file_get_value (desktop_file, DESKTOP_GROUP, "Type", NULL);
  if (!type)
    return FALSE;

  ret = !strncmp (type, "Link", 4) || !strncmp (type, "FSDevice", 8);
  g_free  (type);

  /* FIXME: do we have to get the link URI and figure out its type?  For now,
   * we'll just assume that it does link to a folder...
   */

  return ret;
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
  gboolean ret;
  GKeyFile *desktop_file;
  char *ditem_url;
  char *tmp;
  int tmpsize;

  ditem_url = NULL;

  desktop_file = g_key_file_new ();
  
  ret = gnome_vfs_read_entire_file (desktop_uri, &tmpsize, &tmp);
  if (ret != GNOME_VFS_OK)
    return NULL;

  ret = g_key_file_load_from_data (desktop_file, tmp, strlen(tmp),
				   G_KEY_FILE_KEEP_TRANSLATIONS, error);
  g_free (tmp);

  if (!ret)
    return NULL;

  if (!is_desktop_file_a_folder (desktop_file))
    {
      g_set_error (error,
		   GTK_FILE_SYSTEM_ERROR,
		   GTK_FILE_SYSTEM_ERROR_NOT_FOLDER,
		   _("%s is a link to something that is not a folder"),
		   desktop_uri);
      goto out;
    }

  ditem_url = g_key_file_get_value (desktop_file, DESKTOP_GROUP, "URL", NULL);
  if (ditem_url == NULL || strlen (ditem_url) == 0)
    {
      g_set_error (error,
		   GTK_FILE_SYSTEM_ERROR,
		   GTK_FILE_SYSTEM_ERROR_INVALID_URI,
		   _("%s is a link without a destination location"),
		   desktop_uri);
      goto out;
    }

 out:
  g_key_file_free (desktop_file);

  return ditem_url;
}


struct GetFolderData
{
  GtkFileSystemHandleGnomeVFS *handle;
  GtkFileSystemGetFolderCallback callback;
  gpointer callback_data;
  GtkFileFolderGnomeVFS *folder_vfs;
  GtkFileFolderGnomeVFS *parent_folder;
  GnomeVFSFileInfo *file_info;
  GnomeVFSURI *uri;
  char *vfs_uri;
  GtkFileInfoType types;
};

static void
get_folder_complete_operation (struct GetFolderData *op_data)
{
  GError *error = NULL;
  GnomeVFSMonitorHandle *monitor = NULL;
  GtkFileSystemGnomeVFS *system_vfs;
  GtkFileFolderGnomeVFS *folder_vfs;
  char *old_uri = op_data->vfs_uri;
  gboolean free_old_uri = FALSE;

  system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (GTK_FILE_SYSTEM_HANDLE (op_data->handle)->file_system);

  folder_vfs = g_hash_table_lookup (system_vfs->folders, op_data->vfs_uri);
  if (folder_vfs)
    {
      /* returned this cached folder */
      g_object_ref (folder_vfs);

      (* op_data->callback) (GTK_FILE_SYSTEM_HANDLE (op_data->handle),
			     GTK_FILE_FOLDER (folder_vfs), NULL,
			     op_data->callback_data);

      g_free (op_data->vfs_uri);
      goto out;
    }

  if (is_desktop_file (op_data->file_info))
    {
      char *ditem_uri = NULL;

      ditem_uri = get_desktop_link_uri (op_data->vfs_uri, &error);
      if (ditem_uri == NULL)
	{
	  /* Couldn't get desktop link uri */
	  (* op_data->callback) (GTK_FILE_SYSTEM_HANDLE (op_data->handle),
				 NULL, error, op_data->callback_data);

	  g_free (op_data->vfs_uri);

	  goto out;
	}

      free_old_uri = TRUE;
      op_data->vfs_uri = ditem_uri;
    }
  else if (op_data->file_info->type != GNOME_VFS_FILE_TYPE_DIRECTORY)
    {
      g_set_error (&error,
		   GTK_FILE_SYSTEM_ERROR,
		   GTK_FILE_SYSTEM_ERROR_NOT_FOLDER,
		   _("%s is not a folder"),
		   op_data->vfs_uri);

      (* op_data->callback) (GTK_FILE_SYSTEM_HANDLE (op_data->handle),
			     NULL, error, op_data->callback_data);

      g_free (op_data->vfs_uri);
      g_error_free (error);

      goto out;
    }

  /* Create the folder */
  folder_vfs = g_object_new (GTK_TYPE_FILE_FOLDER_GNOME_VFS, NULL);

  folder_vfs->is_afs_or_net = is_vfs_info_an_afs_folder (system_vfs,
						         op_data->file_info);
  if (!folder_vfs->is_afs_or_net)
    {
      GnomeVFSResult ret;
      char *tmp_uri;

      tmp_uri = gnome_vfs_uri_to_string (op_data->uri, 0);

      gnome_authentication_manager_push_sync ();
      ret = gnome_vfs_monitor_add (&monitor,
				   tmp_uri,
				   GNOME_VFS_MONITOR_DIRECTORY,
				   monitor_callback, folder_vfs);
      gnome_authentication_manager_pop_sync ();

      g_free (tmp_uri);

      if (ret != GNOME_VFS_OK && ret != GNOME_VFS_ERROR_NOT_SUPPORTED)
	{
	  char *uri;

	  uri = gnome_vfs_uri_to_string (op_data->uri, 0);
	  set_vfs_error (ret, uri, &error);
	  g_free (uri);

	  (* op_data->callback) (GTK_FILE_SYSTEM_HANDLE (op_data->handle), NULL,
				 error, op_data->callback_data);

	  g_free (op_data->vfs_uri);
	  g_object_unref (folder_vfs);

	  goto out;
	}
    }

  folder_vfs->system = system_vfs;
  folder_vfs->uri = op_data->vfs_uri; /* takes ownership */
  folder_vfs->types = op_data->types;
  folder_vfs->monitor = NULL;
  folder_vfs->async_handle = NULL;
  folder_vfs->finished_loading = 0;
  folder_vfs->children = g_hash_table_new_full (g_str_hash,
						g_str_equal,
						NULL,
						(GDestroyNotify) folder_child_free);

  g_hash_table_insert (system_vfs->folders, folder_vfs->uri, folder_vfs);

  if (op_data->parent_folder
      && !g_hash_table_lookup (op_data->parent_folder->children, old_uri))
    {
      GSList uris;
      FolderChild *child;

      /* Make sure this child is in the parent folder */
      child = folder_child_new (old_uri, op_data->file_info,
                                op_data->parent_folder->async_handle ? TRUE : FALSE);

      g_hash_table_insert (op_data->parent_folder->children, child->uri, child);

      uris.next = NULL;
      uris.data = gtk_file_path_new_steal (old_uri);
      g_signal_emit_by_name (op_data->parent_folder, "files-added", &uris);
    }

  folder_vfs->monitor = monitor;

  g_object_ref (folder_vfs);

  (* op_data->callback) (GTK_FILE_SYSTEM_HANDLE (op_data->handle),
			 GTK_FILE_FOLDER (folder_vfs), NULL,
			 op_data->callback_data);

  /* Now initiate a directory load */
  if (folder_vfs->is_afs_or_net)
    load_afs_dir (folder_vfs);
  else
    load_dir (folder_vfs);

  g_object_unref (folder_vfs);

out:
  if (op_data->parent_folder)
    g_object_unref (op_data->parent_folder);

  op_data->handle->callback_type = 0;
  op_data->handle->callback_data = NULL;

  if (free_old_uri)
    g_free (old_uri);

  g_object_unref (op_data->handle);
  g_free (op_data);
}

static void
get_folder_cached_callback (gpointer data)
{
  struct GetFolderData *op_data = data;

  (* op_data->callback) (GTK_FILE_SYSTEM_HANDLE (op_data->handle),
			 GTK_FILE_FOLDER (op_data->folder_vfs), NULL,
			 op_data->callback_data);

  g_object_unref (op_data->handle);
  g_free (op_data);
}

static void
get_folder_file_info_callback (GnomeVFSAsyncHandle *handle,
                               GList               *results,
                               gpointer             callback_data)
{
  GError *error = NULL;
  GnomeVFSGetFileInfoResult *result;
  struct GetFolderData *op_data = callback_data;
  GtkFileSystem *file_system;

  gdk_threads_enter ();

  g_assert (op_data->handle->vfs_handle == handle);
  g_assert (g_list_length (results) == 1);

  file_system = GTK_FILE_SYSTEM_HANDLE (op_data->handle)->file_system;
  g_object_ref (file_system);

  op_data->handle->vfs_handle = NULL;

  result = results->data;

  if (result->result != GNOME_VFS_OK)
    {
      char *uri;

      uri = gnome_vfs_uri_to_string (result->uri, 0);
      set_vfs_error (result->result, uri, &error);
      g_free (uri);

      (* op_data->callback) (GTK_FILE_SYSTEM_HANDLE (op_data->handle), NULL,
			     error, op_data->callback_data);

      g_error_free (error);

      if (op_data->parent_folder)
	g_object_unref (op_data->parent_folder);

      op_data->handle->callback_type = 0;
      op_data->handle->callback_data = NULL;

      g_object_unref (op_data->handle);
      g_free (op_data);

      goto out;
    }

  op_data->file_info = result->file_info;
  op_data->uri = result->uri;

  get_folder_complete_operation (op_data);

out:
  g_object_unref (file_system);

  gdk_threads_leave ();
}

static void
get_folder_vfs_info_cached_callback (gpointer data)
{
  GnomeVFSURI *uri;
  GnomeVFSFileInfo *file_info;
  struct GetFolderData *op_data = data;

  uri = op_data->uri;
  file_info = op_data->file_info;

  get_folder_complete_operation (op_data);

  gnome_vfs_uri_unref (uri);
  gnome_vfs_file_info_unref (file_info);
}

static GtkFileSystemHandle *
gtk_file_system_gnome_vfs_get_folder (GtkFileSystem                  *file_system,
				      const GtkFilePath              *path,
				      GtkFileInfoType                 types,
				      GtkFileSystemGetFolderCallback  callback,
				      gpointer                        data)
{
  char *uri;
  GList *uris = NULL;
  GtkFilePath *parent_path;
  GtkFileSystemGnomeVFS *system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);
  GtkFileFolderGnomeVFS *folder_vfs = NULL;
  GtkFileFolderGnomeVFS *parent_folder = NULL;
  GnomeVFSFileInfo *vfs_info;
  GnomeVFSFileInfoOptions options = 0;
  struct GetFolderData *op_data = NULL;

  profile_start ("start", (char *) path);

  uri = make_uri_canonical (gtk_file_path_get_string (path));
  folder_vfs = g_hash_table_lookup (system_vfs->folders, uri);
  if (folder_vfs)
    {
      folder_vfs->types |= types;

      g_free (uri);
      g_object_ref (folder_vfs);

      op_data = g_new0 (struct GetFolderData, 1);
      op_data->handle = gtk_file_system_handle_gnome_vfs_new (file_system);
      op_data->folder_vfs = folder_vfs;
      op_data->callback = callback;
      op_data->callback_data = data;

      queue_vfs_idle_callback (system_vfs, get_folder_cached_callback, op_data);

      /* We don't have to initiate a reload to make up for the added
       * types, since gnome-vfs loads all file info we need by
       * default (and get_options() explicitly requests the mime type).
       */

      profile_end ("returning cached folder", NULL);
      return GTK_FILE_SYSTEM_HANDLE (g_object_ref (op_data->handle));
    }

  if (!gtk_file_system_get_parent (file_system, path, &parent_path, NULL))
    {
      g_free (uri);
      profile_end ("returning NULL because of lack of parent", NULL);
      return NULL;
    }

  vfs_info = NULL;

  if (parent_path)
    {
      char *parent_uri;

      /* If the parent folder is loaded, make sure we exist in it */
      parent_uri = make_uri_canonical (gtk_file_path_get_string (parent_path));
      parent_folder = g_hash_table_lookup (system_vfs->folders, parent_uri);
      g_free (parent_uri);
      gtk_file_path_free (parent_path);

      if (parent_folder)
	{
	  FolderChild *child;

	  child = lookup_folder_child_from_uri (GTK_FILE_FOLDER (parent_folder), uri);

	  if (child)
	    {
	      vfs_info = child->info;
	      gnome_vfs_file_info_ref (vfs_info);
	      g_assert (vfs_info != NULL);
	    }
	}
    }

  op_data = g_new0 (struct GetFolderData, 1);
  op_data->handle = gtk_file_system_handle_gnome_vfs_new (file_system);
  op_data->types = types;
  op_data->vfs_uri = uri;
  op_data->callback = callback;
  op_data->callback_data = data;
  op_data->parent_folder = parent_folder;
  if (vfs_info)
    {
      op_data->file_info = gnome_vfs_file_info_new ();
      gnome_vfs_file_info_copy (op_data->file_info, vfs_info);
      op_data->uri = gnome_vfs_uri_new (uri);
    }
  else
    {
      op_data->file_info = NULL;
      op_data->uri = NULL;
    }

  if (parent_folder)
    {
      g_object_ref (parent_folder);
      options = get_options (parent_folder->types);
    }
  options |= get_options (GTK_FILE_INFO_IS_FOLDER);

  op_data->handle->callback_type = CALLBACK_GET_FOLDER;
  op_data->handle->callback_data = op_data;

  if (op_data->file_info)
    {
      queue_vfs_idle_callback (system_vfs, get_folder_vfs_info_cached_callback, op_data);
    }
  else
    {
      uris = g_list_append (uris, gnome_vfs_uri_new (uri));

      gnome_authentication_manager_push_async ();
      gnome_vfs_async_get_file_info (&op_data->handle->vfs_handle,
				     uris,
				     options,
				     GNOME_VFS_PRIORITY_DEFAULT,
				     get_folder_file_info_callback,
				     op_data);
      gnome_authentication_manager_pop_async ();

      gnome_vfs_uri_list_free (uris);
    }

  profile_end ("end", (char *) path);

  return GTK_FILE_SYSTEM_HANDLE (g_object_ref (op_data->handle));
}

struct GetFileInfoData
{
  GtkFileSystemHandleGnomeVFS *handle;
  GtkFileInfoType types;
  GtkFileSystemGetInfoCallback callback;
  gpointer callback_data;
};

static void
get_file_info_callback (GnomeVFSAsyncHandle *vfs_handle,
                        GList               *results,
			gpointer             data)
{
  gchar *uri = NULL;
  GError *error = NULL;
  GtkFileInfo *file_info = NULL;
  GnomeVFSGetFileInfoResult *result = results->data;
  struct GetFileInfoData *op_data = data;
  GtkFileSystem *file_system;

  gdk_threads_enter ();

  file_system = GTK_FILE_SYSTEM_HANDLE (op_data->handle)->file_system;
  g_object_ref (file_system);

  g_assert (op_data->handle->vfs_handle == vfs_handle);

  op_data->handle->vfs_handle = NULL;

  uri = gnome_vfs_uri_to_string (result->uri, 0);

  if (result->result != GNOME_VFS_OK)
    {
      set_vfs_error (result->result, uri, &error);

      (* op_data->callback) (GTK_FILE_SYSTEM_HANDLE (op_data->handle), NULL,
			     error, op_data->callback_data);

      goto out;
    }
  
  file_info = info_from_vfs_info (GTK_FILE_SYSTEM_GNOME_VFS (GTK_FILE_SYSTEM_HANDLE (op_data->handle)->file_system),
				  uri,
                                  result->file_info,
                                  op_data->types, &error);
  (* op_data->callback) (GTK_FILE_SYSTEM_HANDLE (op_data->handle), file_info,
			 error, op_data->callback_data);

out:
  g_free (uri);
  if (error)
    g_error_free (error);
  if (file_info)
    gtk_file_info_free (file_info);

  op_data->handle->callback_type = 0;
  op_data->handle->callback_data = NULL;

  g_object_unref (op_data->handle);
  g_free (op_data);

  g_object_unref (file_system);

  gdk_threads_leave ();
}

static GtkFileSystemHandle *
gtk_file_system_gnome_vfs_get_info (GtkFileSystem                 *file_system,
				    const GtkFilePath             *path,
				    GtkFileInfoType                types,
				    GtkFileSystemGetInfoCallback   callback,
				    gpointer                       data)
{
  GList uris;
  GtkFileSystemHandleGnomeVFS *handle;
  struct GetFileInfoData *op_data;
  char *uri;

  uri = make_uri_canonical (gtk_file_path_get_string (path));
  handle = gtk_file_system_handle_gnome_vfs_new (file_system);

  op_data = g_new0 (struct GetFileInfoData, 1);
  op_data->handle = g_object_ref (handle);
  op_data->types = types;
  op_data->callback = callback;
  op_data->callback_data = data;

  uris.prev = uris.next = NULL;
  uris.data = gnome_vfs_uri_new (uri);
  g_free (uri);

  op_data->handle->callback_type = CALLBACK_GET_FILE_INFO;
  op_data->handle->callback_data = op_data;

  gnome_authentication_manager_push_async ();
  gnome_vfs_async_get_file_info (&handle->vfs_handle,
				 &uris,
				 get_options (types),
				 GNOME_VFS_PRIORITY_DEFAULT,
				 get_file_info_callback,
				 op_data);
  gnome_authentication_manager_pop_async ();

  gnome_vfs_uri_unref (uris.data);

  return GTK_FILE_SYSTEM_HANDLE (handle);
}

struct CreateFolderData
{
  GtkFileSystemHandleGnomeVFS *handle;
  GtkFilePath *path;
  GtkFileSystemCreateFolderCallback callback;
  gpointer callback_data;
  gboolean error_reported;
};


static int
create_folder_progress_cb (GnomeVFSAsyncHandle      *vfs_handle,
			   GnomeVFSXferProgressInfo *progress_info,
			   gpointer                  data)
{
  int ret = 0;
  struct CreateFolderData *op_data = data;
  GtkFileSystem *file_system;

  gdk_threads_enter ();

  g_assert (op_data->handle->vfs_handle == vfs_handle);

  file_system = GTK_FILE_SYSTEM_HANDLE (op_data->handle)->file_system;
  g_object_ref (file_system);

  switch (progress_info->phase)
    {
      case GNOME_VFS_XFER_PHASE_COMPLETED:
	if (!op_data->error_reported)
	  (* op_data->callback) (GTK_FILE_SYSTEM_HANDLE (op_data->handle),
				 op_data->path, NULL, op_data->callback_data);

	break;

      default:
	switch (progress_info->status)
	  {
	    case GNOME_VFS_XFER_PROGRESS_STATUS_DUPLICATE:
	      ret = GNOME_VFS_XFER_ERROR_ACTION_ABORT;
	      /* fall through */
	    case GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR:
	      if (!op_data->error_reported)
	        {
		  GError *error = NULL;
	
		  set_vfs_error (progress_info->vfs_status,
				 gtk_file_path_get_string (op_data->path),
				 &error);

		  (* op_data->callback) (GTK_FILE_SYSTEM_HANDLE (op_data->handle),
					 op_data->path, error, op_data->callback_data);
		  g_error_free (error);

		  op_data->error_reported = TRUE;
		}

	      g_object_unref (file_system);

	      gdk_threads_leave ();

	      return ret;

	    case GNOME_VFS_XFER_PROGRESS_STATUS_OK:
	      /* not completed, don't free the handle and op_data yet */
	      gdk_threads_leave ();

	      g_object_unref (file_system);

	      return 0;

	    default:
	      break;
	  }
	break;
    }

  op_data->handle->callback_type = 0;
  op_data->handle->callback_data = NULL;

  g_object_unref (op_data->handle);
  gtk_file_path_free (op_data->path);
  g_free (op_data);

  g_object_unref (file_system);

  gdk_threads_leave ();

  return ret;
}

static GtkFileSystemHandle *
gtk_file_system_gnome_vfs_create_folder (GtkFileSystem                     *file_system,
					 const GtkFilePath                 *path,
					 GtkFileSystemCreateFolderCallback  callback,
					 gpointer                           data)
{
  char *uri;
  GList uris;
  struct CreateFolderData *op_data;

  op_data = g_new (struct CreateFolderData, 1);
  op_data->handle = gtk_file_system_handle_gnome_vfs_new (file_system);
  op_data->path = gtk_file_path_copy (path);
  op_data->callback = callback;
  op_data->callback_data = data;
  op_data->error_reported = FALSE;

  op_data->handle->callback_type = CALLBACK_CREATE_FOLDER;
  op_data->handle->callback_data = op_data;

  uri = make_uri_canonical (gtk_file_path_get_string (path));
  uris.prev = uris.next = NULL;
  uris.data = gnome_vfs_uri_new (uri);
  g_free (uri);

  gnome_authentication_manager_push_async ();
  gnome_vfs_async_xfer (&op_data->handle->vfs_handle,
			NULL,
			&uris,
			GNOME_VFS_XFER_NEW_UNIQUE_DIRECTORY,
			GNOME_VFS_XFER_ERROR_MODE_ABORT,
			GNOME_VFS_XFER_OVERWRITE_MODE_ABORT,
			GNOME_VFS_PRIORITY_DEFAULT,
			create_folder_progress_cb, op_data,
			NULL, NULL);

  gnome_vfs_uri_unref (uris.data);

  return GTK_FILE_SYSTEM_HANDLE (g_object_ref (op_data->handle));
}

static void
cancel_operation_callback (gpointer data)
{
  GtkFileSystemHandle *handle = GTK_FILE_SYSTEM_HANDLE (data);
  GtkFileSystemHandleGnomeVFS *handle_vfs = GTK_FILE_SYSTEM_HANDLE_GNOME_VFS (data);

  switch (handle_vfs->callback_type)
    {
      case CALLBACK_GET_FOLDER:
	{
	  struct GetFolderData *data = handle_vfs->callback_data;

	  (* data->callback) (handle, NULL, NULL, data->callback_data);

	  if (data->folder_vfs)
	    g_object_unref (data->folder_vfs);
	  if (data->parent_folder)
	    g_object_unref (data->parent_folder);
	  if (data->uri)
	    gnome_vfs_uri_unref (data->uri);
	  if (data->file_info)
	    gnome_vfs_file_info_unref (data->file_info);
	  g_free (data->vfs_uri);
	  g_free (data);
	  break;
	}

      case CALLBACK_GET_FILE_INFO:
	{
	  struct GetFileInfoData *data = handle_vfs->callback_data;

	  (* data->callback) (handle, NULL, NULL, data->callback_data);

	  g_free (data);
	  break;
	}

      case CALLBACK_CREATE_FOLDER:
	{
	  struct CreateFolderData *data = handle_vfs->callback_data;

	  if (!data->error_reported)
	    (* data->callback) (handle, NULL, NULL, data->callback_data);

	  gtk_file_path_free (data->path);
	  g_free (data);
	  break;
	}
    }

  handle_vfs->callback_type = 0;
  handle_vfs->callback_data = NULL;

  g_object_unref (handle);
}

static void
gtk_file_system_gnome_vfs_cancel_operation (GtkFileSystemHandle *handle)
{
  GtkFileSystemHandleGnomeVFS *handle_vfs = GTK_FILE_SYSTEM_HANDLE_GNOME_VFS (handle);

  if (handle->cancelled)
    return;

  if (handle_vfs->vfs_handle)
    {
      if (handle_vfs->vfs_handle)
        gnome_vfs_async_cancel (handle_vfs->vfs_handle);
      handle_vfs->vfs_handle = NULL;

      /* Volume mounts can't be cancelled right now... */
      if (handle_vfs->callback_type == CALLBACK_VOLUME_MOUNT)
	handle->cancelled = FALSE;
      else
        handle->cancelled = TRUE;

      queue_vfs_idle_callback (GTK_FILE_SYSTEM_GNOME_VFS (handle->file_system), cancel_operation_callback, handle);
    }
}

static void
gtk_file_system_gnome_vfs_volume_free (GtkFileSystem        *file_system,
				       GtkFileSystemVolume  *volume)
{
  if (IS_NETWORK_SERVERS_VOLUME_TOKEN (volume))
    /* do nothing */;
  else if (GNOME_IS_VFS_DRIVE (volume))
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

  if (IS_NETWORK_SERVERS_VOLUME_TOKEN (volume))
    return gtk_file_path_new_dup ("network:///");
  else if (GNOME_IS_VFS_DRIVE (volume)) {
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
  if (IS_NETWORK_SERVERS_VOLUME_TOKEN (volume))
    return TRUE;
  else if (GNOME_IS_VFS_DRIVE (volume))
    return gnome_vfs_drive_is_mounted (GNOME_VFS_DRIVE (volume));
  else if (GNOME_IS_VFS_VOLUME (volume))
    return gnome_vfs_volume_is_mounted (GNOME_VFS_VOLUME (volume));
  else
    {
      g_warning ("%p is not a valid volume", volume);
      return FALSE;
    }
}

struct VolumeMountData
{
  GtkFileSystemHandleGnomeVFS *handle;
  GtkFileSystemVolume *volume;
  GtkFileSystemVolumeMountCallback callback;
  gpointer callback_data;
};

/* Used from gnome_vfs_{volume,drive}_mount() */
static void
volume_mount_cb (gboolean succeeded,
		 char    *error,
		 char    *detailed_error,
		 gpointer data)
{
  GError *tmp_error = NULL;
  struct VolumeMountData *op_data = data;

  gdk_threads_enter ();

  if (!succeeded)
    {
      g_set_error (&tmp_error,
		   GTK_FILE_SYSTEM_ERROR,
		   GTK_FILE_SYSTEM_ERROR_FAILED,
		   "%s:\n%s",
		   error, detailed_error);
    }

  (* op_data->callback) (GTK_FILE_SYSTEM_HANDLE (op_data->handle),
			 op_data->volume,
			 tmp_error, op_data->callback_data);

  if (tmp_error)
    g_error_free (tmp_error);

  op_data->handle->callback_type = 0;
  op_data->handle->callback_data = NULL;

  g_object_unref (op_data->handle);
  g_object_unref (op_data->volume);
  g_free (op_data);

  gdk_threads_leave ();
}

static void
volume_mount_idle_callback (gpointer data)
{
  struct VolumeMountData *op_data = data;

  (* op_data->callback) (GTK_FILE_SYSTEM_HANDLE (op_data->handle),
			 op_data->volume, NULL, op_data->callback_data);

  g_object_unref (op_data->handle);
  g_object_unref (op_data->volume);
  g_free (op_data);
}

static GtkFileSystemHandle *
gtk_file_system_gnome_vfs_volume_mount (GtkFileSystem                    *file_system,
					GtkFileSystemVolume              *volume,
					GtkFileSystemVolumeMountCallback  callback,
					gpointer                          data)
{
  if (GNOME_IS_VFS_DRIVE (volume))
    {
      struct VolumeMountData *op_data;
      GtkFileSystemHandleGnomeVFS *handle;

      handle = gtk_file_system_handle_gnome_vfs_new (file_system);

      op_data = g_new0 (struct VolumeMountData, 1);
      op_data->handle = g_object_ref (handle);
      op_data->volume = g_object_ref (volume);
      op_data->callback = callback;
      op_data->callback_data = data;

      op_data->handle->callback_type = CALLBACK_VOLUME_MOUNT;
      op_data->handle->callback_data = op_data;

      gnome_authentication_manager_push_sync ();
      gnome_vfs_drive_mount (GNOME_VFS_DRIVE (volume), volume_mount_cb, op_data);
      gnome_authentication_manager_pop_sync ();

      return GTK_FILE_SYSTEM_HANDLE (op_data->handle);
    }
  else if (GNOME_IS_VFS_VOLUME (volume)
	   || (IS_NETWORK_SERVERS_VOLUME_TOKEN (volume)))
    {
      struct VolumeMountData *op_data;
      GtkFileSystemHandleGnomeVFS *handle;

      handle = gtk_file_system_handle_gnome_vfs_new (file_system);

      op_data = g_new0 (struct VolumeMountData, 1);
      op_data->handle = g_object_ref (handle);
      op_data->volume = g_object_ref (volume);
      op_data->callback = callback;
      op_data->callback_data = data;

      op_data->handle->callback_type = CALLBACK_VOLUME_MOUNT;
      op_data->handle->callback_data = op_data;

      queue_vfs_idle_callback (GTK_FILE_SYSTEM_GNOME_VFS (file_system), volume_mount_idle_callback, op_data);

      return GTK_FILE_SYSTEM_HANDLE (op_data->handle);
    }
  else
    {
      g_warning ("%p is not a valid volume", volume);
      return NULL;
    }

  return NULL;
}

static gchar *
gtk_file_system_gnome_vfs_volume_get_display_name (GtkFileSystem       *file_system,
						   GtkFileSystemVolume *volume)
{
  char *display_name, *uri;
  GnomeVFSVolume *mounted_volume;

  display_name = NULL;
  if (IS_NETWORK_SERVERS_VOLUME_TOKEN (volume))
    display_name = g_strdup (_("Network Servers"));
  else if (GNOME_IS_VFS_DRIVE (volume))
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

static gchar *
gtk_file_system_gnome_vfs_volume_get_icon_name (GtkFileSystem        *file_system,
					        GtkFileSystemVolume  *volume,
					        GError              **error)
{
  GtkFileSystemGnomeVFS *system_vfs;
  char *icon_name, *uri;
  GnomeVFSVolume *mounted_volume;

  profile_start ("start", NULL);

  system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);

  icon_name = NULL;
  if (IS_NETWORK_SERVERS_VOLUME_TOKEN (volume))
    icon_name = g_strdup ("gnome-fs-network");
  else if (GNOME_IS_VFS_DRIVE (volume))
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

  profile_end ("end", NULL);

  return icon_name;
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
      gchar *path_part, *path, *uri, *filesystem_path, *escaped;
      int len;

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
	      len = strlen (base_uri);
	      if (len != 0)
		{
		  escaped = gnome_vfs_escape_path_string (filesystem_path);

		  if (base_uri[len - 1] != '/')
		    {
		      char *base_dir;

		      base_dir = g_strconcat (base_uri, "/", NULL);
		      uri = gnome_vfs_uri_make_full_from_relative (base_dir, escaped);
		      g_free (base_dir);
		    }
		  else
		    uri = gnome_vfs_uri_make_full_from_relative (base_uri, escaped);

		  g_free (escaped);
		}
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
			      const char    *uri)
{
  GtkFileFolderGnomeVFS *folder_vfs = GTK_FILE_FOLDER_GNOME_VFS (folder);
  FolderChild *child;

  profile_start ("start", uri);

  child = g_hash_table_lookup (folder_vfs->children, uri);
  profile_end ("end", uri);

  return child;
}

static FolderChild *
lookup_folder_child (GtkFileFolder     *folder,
		     const GtkFilePath *path)
{
  char *uri;
  FolderChild *child;

  uri = make_uri_canonical (gtk_file_path_get_string (path));
  child = lookup_folder_child_from_uri (folder, uri);
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
      return NULL;
    }

  child = lookup_folder_child (folder, path);

  if (child)
    file_info = info_from_vfs_info (folder_vfs->system, uri, child->info, folder_vfs->types, error);
  else
    {
      char *uri;

      file_info = NULL;
      uri = gtk_file_system_path_to_uri (GTK_FILE_SYSTEM (folder_vfs->system), path);

      g_set_error (error,
		   GTK_FILE_SYSTEM_ERROR,
		   GTK_FILE_SYSTEM_ERROR_NONEXISTENT,
		   _("Error getting information for '%s'"),
		   uri);
      g_free (uri);
    }

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

  return folder_vfs->finished_loading;
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
info_from_vfs_info (GtkFileSystemGnomeVFS  *system_vfs,
		    const gchar            *uri,
		    GnomeVFSFileInfo       *vfs_info,
		    GtkFileInfoType         types,
		    GError                **error)
{
  GtkFileInfo *info = gtk_file_info_new ();
  gboolean is_desktop;
  GKeyFile *desktop_file = NULL;

  /* Desktop files are special.  They can override the name, type, icon, etc. */

  is_desktop = is_desktop_file (vfs_info);

  if (is_desktop)
    {
      gboolean ret;
      gchar *tmp;
      int tmpsize;

      ret = gnome_vfs_read_entire_file (uri, &tmpsize, &tmp);

      if (ret == GNOME_VFS_OK)
      {
        desktop_file = g_key_file_new ();
	ret = g_key_file_load_from_data (desktop_file, tmp, strlen(tmp),
                                       G_KEY_FILE_KEEP_TRANSLATIONS, error);
        g_free (tmp);
      }
    }

  /* Display name */

  if (types & GTK_FILE_INFO_DISPLAY_NAME)
    {
      if (is_desktop)
	{
	  char *name;

	  name = g_key_file_get_locale_string (desktop_file, DESKTOP_GROUP,
					       "Name", NULL, NULL);
	  if (name != NULL)
	    {
	      gtk_file_info_set_display_name (info, name);
	      g_free (name);
	    }
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
	is_hidden = g_key_file_get_boolean (desktop_file, DESKTOP_GROUP, "Hidden", NULL);
      else
	is_hidden = (vfs_info->name && vfs_info->name[0] == '.');

      gtk_file_info_set_is_hidden (info, is_hidden);
    }

  /* Folder */

  if (types & GTK_FILE_INFO_IS_FOLDER)
    {
      gboolean is_folder;

      if (is_desktop)
	is_folder = is_desktop_file_a_folder (desktop_file);
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

  if (types & GTK_FILE_INFO_ICON)
    {
      gchar *icon_name;
      GtkIconTheme *icon_theme = gtk_icon_theme_get_default (); /* FIXME: can't do better right now ... */

      if (types & GTK_FILE_INFO_MIME_TYPE && is_desktop)
        {
	  icon_name = g_key_file_get_value (desktop_file, DESKTOP_GROUP, "Icon", NULL);
	  gtk_file_info_set_icon_name (info, icon_name);
	  g_free (icon_name);
	}
      else
        {
          if (strcmp (uri, system_vfs->desktop_uri) == 0)
	    gtk_file_info_set_icon_name (info, "gnome-fs-desktop");
          else if (strcmp (uri, system_vfs->home_uri) == 0)
	    gtk_file_info_set_icon_name (info, "gnome-fs-home");
          else if (strcmp (uri, "trash:///") == 0)
	    gtk_file_info_set_icon_name (info, "gnome-fs-trash-empty");
          else if (vfs_info)
	    {
	      icon_name = gnome_icon_lookup (icon_theme,
				             NULL, uri, NULL,
				             vfs_info, vfs_info->mime_type,
				             GNOME_ICON_LOOKUP_FLAGS_NONE,
				             NULL);
	      gtk_file_info_set_icon_name (info, icon_name);
	      g_free (icon_name);
	    }
	}
    }

  gtk_file_info_set_modification_time (info, vfs_info->mtime);
  gtk_file_info_set_size (info, vfs_info->size);

  if (is_desktop)
    g_key_file_free (desktop_file);

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
      folder_vfs->finished_loading = 1;

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


/*
 * GtkFileSystemHandleGnomeVFS
 */
static GType
gtk_file_system_handle_gnome_vfs_get_type (void)
{
  return type_gtk_file_system_handle_gnome_vfs;
}

static void
gtk_file_system_handle_gnome_vfs_class_init (GtkFileSystemHandleGnomeVFSClass *class)
{
  GObjectClass *gobject_class;

  handle_parent_class = g_type_class_peek_parent (class);

  gobject_class = (GObjectClass *) class;

  gobject_class->finalize = gtk_file_system_handle_gnome_vfs_finalize;
}

static void
gtk_file_system_handle_gnome_vfs_init (GtkFileSystemHandleGnomeVFS *handle)
{
  handle->vfs_handle = NULL;
}

static void
gtk_file_system_handle_gnome_vfs_finalize (GObject *object)
{
  GtkFileSystemHandleGnomeVFS *handle;
  GtkFileSystemGnomeVFS *system_vfs;

  handle = GTK_FILE_SYSTEM_HANDLE_GNOME_VFS (object);

  system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (GTK_FILE_SYSTEM_HANDLE (handle)->file_system);

  g_assert (g_hash_table_lookup (system_vfs->handles, handle) != NULL);
  g_hash_table_remove (system_vfs->handles, handle);

  if (G_OBJECT_CLASS (handle_parent_class)->finalize)
    G_OBJECT_CLASS (handle_parent_class)->finalize (object);
}

static GtkFileSystemHandleGnomeVFS *
gtk_file_system_handle_gnome_vfs_new (GtkFileSystem *file_system)
{
  GtkFileSystemHandleGnomeVFS *handle;
  GtkFileSystemGnomeVFS *system_vfs;

  system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (file_system);

  handle = g_object_new (GTK_TYPE_FILE_SYSTEM_HANDLE_GNOME_VFS, NULL);
  GTK_FILE_SYSTEM_HANDLE (handle)->file_system = file_system;

  g_assert (g_hash_table_lookup (system_vfs->handles, handle) == NULL);
  g_hash_table_insert (system_vfs->handles, handle, handle);

  return handle;
}

/* some code for making callback calls from idle */
static void
execute_vfs_callbacks (gpointer data)
{
  GSList *l;
  gboolean unref_file_system = TRUE;
  GtkFileSystemGnomeVFS *system_vfs = GTK_FILE_SYSTEM_GNOME_VFS (data);

  if (!system_vfs->execute_vfs_callbacks_idle_id)
    unref_file_system = FALSE;
  else
    g_object_ref (system_vfs);

  for (l = system_vfs->vfs_callbacks; l; l = l->next)
    {
      struct vfs_idle_callback *cb = l->data;

      (* cb->callback) (cb->callback_data);

      g_free (cb);
    }

  g_slist_free (system_vfs->vfs_callbacks);
  system_vfs->vfs_callbacks = NULL;

  if (unref_file_system)
    g_object_unref (system_vfs);

  system_vfs->execute_vfs_callbacks_idle_id = 0;
}

static gboolean
execute_vfs_callbacks_idle (gpointer data)
{
  GDK_THREADS_ENTER ();

  execute_vfs_callbacks (data);

  GDK_THREADS_LEAVE ();

  return FALSE;
}

static void
queue_vfs_idle_callback (GtkFileSystemGnomeVFS *system_vfs,
			 VfsIdleCallback        callback,
			 gpointer               callback_data)
{
  struct vfs_idle_callback *cb;

  cb = g_new (struct vfs_idle_callback, 1);
  cb->callback = callback;
  cb->callback_data = callback_data;

  system_vfs->vfs_callbacks = g_slist_append (system_vfs->vfs_callbacks, cb);

  if (!system_vfs->execute_vfs_callbacks_idle_id)
    system_vfs->execute_vfs_callbacks_idle_id = g_idle_add (execute_vfs_callbacks_idle, system_vfs);
}

/* GtkFileSystem module calls */

void fs_module_init (GTypeModule    *module);
void fs_module_exit (void);
GtkFileSystem *fs_module_create (void);

void 
fs_module_init (GTypeModule    *module)
{
  profile_start ("start", "will call gnome_vfs_init()");

  gnome_vfs_init ();

  {
    const GTypeInfo file_system_gnome_vfs_info =
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
    const GInterfaceInfo file_system_info =
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
    const GTypeInfo file_folder_gnome_vfs_info =
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
    
    const GInterfaceInfo file_folder_info =
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

  {
    const GTypeInfo file_system_handle_gnome_vfs_info =
      {
	sizeof (GtkFileSystemHandleGnomeVFSClass),
	NULL,           /* base_init */ 
	NULL,           /* base_finalize */
	(GClassInitFunc) gtk_file_system_handle_gnome_vfs_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (GtkFileSystemHandleGnomeVFS),
	0,
	(GInstanceInitFunc) gtk_file_system_handle_gnome_vfs_init,
      };

    type_gtk_file_system_handle_gnome_vfs = g_type_module_register_type (module,
									 GTK_TYPE_FILE_SYSTEM_HANDLE,
									 "GtkFileSystemHandleGnomeVFS",
									 &file_system_handle_gnome_vfs_info, 0);
  }

  /* We ref the class so that the module won't get unloaded.  We need this
   * because gnome_icon_lookup() will cause GnomeIconTheme to get registered,
   * and we cannot unload that properly.  I'd prefer to do this via
   * g_module_make_resident(), but here we don't have access to the GModule.
   * See bug #139254 for reference. [federico]
   */

  g_type_class_ref (type_gtk_file_system_gnome_vfs);

  profile_end ("end", NULL);
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
