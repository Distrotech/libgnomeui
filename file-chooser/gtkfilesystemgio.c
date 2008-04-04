/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2007 Carlos Garnacho
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Carlos Garnacho Parro  <carlos@imendio.com>
 */

#include <config.h>
#include <gio/gio.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <string.h>
#include "gtkfilesystemgio.h"

#define GTK_FILE_SYSTEM_GIO_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),   GTK_TYPE_FILE_SYSTEM_GIO, GtkFileSystemGioClass))
#define GTK_IS_FILE_SYSTEM_GIO_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),   GTK_TYPE_FILE_SYSTEM_GIO))
#define GTK_FILE_SYSTEM_GIO_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GTK_TYPE_FILE_SYSTEM_GIO, GtkFileSystemGioClass))

#define GTK_TYPE_FILE_SYSTEM_HANDLE_GIO  (gtk_file_system_handle_gio_get_type ())
#define GTK_FILE_SYSTEM_HANDLE_GIO(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_FILE_SYSTEM_HANDLE_GIO, GtkFileSystemHandleGio))

#define GTK_TYPE_FILE_FOLDER_GIO         (gtk_file_folder_gio_get_type ())
#define GTK_FILE_FOLDER_GIO(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_FILE_FOLDER_GIO, GtkFileFolderGio))

/* #define DEBUG_MODE */
#ifdef DEBUG_MODE
#define DEBUG(x) g_debug (x);
#else
#define DEBUG(x)
#endif

#define FILES_PER_QUERY 100

#define MODULE_IMPLEMENT_INTERFACE(TYPE_IFACE, iface_init)       { \
  const GInterfaceInfo g_implement_interface_info = { \
    (GInterfaceInitFunc) iface_init, NULL, NULL \
  }; \
  g_type_module_add_interface (type_module, g_define_type_id, TYPE_IFACE, &g_implement_interface_info); \
}

typedef struct GtkFileSystemGioClass       GtkFileSystemGioClass;
typedef struct GtkFileSystemHandleGioClass GtkFileSystemHandleGioClass;
typedef struct GtkFileSystemHandleGio      GtkFileSystemHandleGio;
typedef struct GtkFileFolderGioClass       GtkFileFolderGioClass;
typedef struct GtkFileFolderGio            GtkFileFolderGio;
typedef struct BookmarkEntry               BookmarkEntry;

struct GtkFileSystemGioClass
{
  GObjectClass parent_class;
};

struct GtkFileSystemGio
{
  GObject parent_instance;

  GVolumeMonitor *volume_monitor;

  /* This list contains elements that can be of type GDrive, GVolume and GMount */
  GSList *volumes;

  GCancellable *cancellable;
};

struct GtkFileSystemHandleGioClass
{
  GtkFileSystemHandleClass parent_class;
};

struct GtkFileSystemHandleGio
{
  GtkFileSystemHandle parent_instance;

  GCancellable *cancellable;
  guint source_id;
  gpointer callback;
  gpointer data;
  guint tried_mount : 1;
};

struct GtkFileFolderGioClass
{
  GObjectClass parent_class;
};

struct GtkFileFolderGio
{
  GObject parent_instance;

  GCancellable *cancellable;
  GFile *parent_file;
  GHashTable *children;
  GFileMonitor *directory_monitor;
  guint finished_loading : 1;
};

struct BookmarkEntry
{
  gchar *uri;
  gchar *label;
};

/* GtkFileSystemGio methods */
static void gtk_file_system_gio_iface_init     (GtkFileSystemIface     *iface);
static void gtk_file_system_gio_dispose        (GObject                *object);

/* GtkFileSystemHandleGio methods */
static GType gtk_file_system_handle_gio_get_type (void);
static void  gtk_file_system_handle_gio_finalize (GObject *object);

/* GtkFileSystem interface methods */
static GSList *              gtk_file_system_gio_list_volumes     (GtkFileSystem                  *file_system);
static GtkFileSystemVolume * gtk_file_system_gio_get_volume_for_path (GtkFileSystem               *file_system,
								      const GtkFilePath           *path);
static GtkFileSystemHandle * gtk_file_system_gio_get_folder       (GtkFileSystem                  *file_system,
								   const GtkFilePath              *path,
								   GtkFileInfoType                 types,
								   GtkFileSystemGetFolderCallback  callback,
								   gpointer                        data);

static GtkFileSystemHandle * gtk_file_system_gio_get_info         (GtkFileSystem                     *file_system,
								   const GtkFilePath                 *path,
								   GtkFileInfoType                    types,
								   GtkFileSystemGetInfoCallback       callback,
								   gpointer                           data);
static GtkFileSystemHandle * gtk_file_system_gio_create_folder    (GtkFileSystem                     *file_system,
								   const GtkFilePath                 *path,
								   GtkFileSystemCreateFolderCallback  callback,
								   gpointer                           data);
static void                  gtk_file_system_gio_cancel_operation (GtkFileSystemHandle               *handle);


static void                  gtk_file_system_gio_volume_free             (GtkFileSystem             *file_system,
									  GtkFileSystemVolume       *volume);
static GtkFilePath *         gtk_file_system_gio_volume_get_base_path    (GtkFileSystem             *file_system,
									  GtkFileSystemVolume       *volume);
static gboolean              gtk_file_system_gio_volume_get_is_mounted   (GtkFileSystem             *file_system,
									  GtkFileSystemVolume       *volume);
static GtkFileSystemHandle * gtk_file_system_gio_volume_mount            (GtkFileSystem                    *file_system,
									  GtkFileSystemVolume              *volume,
									  GtkFileSystemVolumeMountCallback  callback,
									  gpointer                          data);

static gchar *               gtk_file_system_gio_volume_get_display_name (GtkFileSystem             *file_system,
									  GtkFileSystemVolume       *volume);
static gchar *               gtk_file_system_gio_volume_get_icon_name    (GtkFileSystem             *file_system,
									  GtkFileSystemVolume       *file_system_volume,
									  GError                   **error);

static gboolean              gtk_file_system_gio_get_parent       (GtkFileSystem                  *file_system,
								   const GtkFilePath              *path,
								   GtkFilePath                   **parent,
								   GError                        **error);
static GtkFilePath *         gtk_file_system_gio_make_path        (GtkFileSystem                  *file_system,
								   const GtkFilePath              *base_path,
								   const gchar                    *display_name,
								   GError                        **error);
static gboolean              gtk_file_system_gio_parse            (GtkFileSystem                  *file_system,
								   const GtkFilePath              *base_path,
								   const gchar                    *str,
								   GtkFilePath                   **folder,
								   gchar                         **file_part,
								   GError                        **error);
static gchar *               gtk_file_system_gio_path_to_uri      (GtkFileSystem                  *file_system,
								   const GtkFilePath              *path);
static gchar *               gtk_file_system_gio_path_to_filename (GtkFileSystem                  *file_system,
								   const GtkFilePath              *path);
static GtkFilePath *         gtk_file_system_gio_uri_to_path      (GtkFileSystem                  *file_system,
								   const gchar                    *uri);
static GtkFilePath *         gtk_file_system_gio_filename_to_path (GtkFileSystem                  *file_system,
								   const gchar                    *filename);

static gboolean              gtk_file_system_gio_insert_bookmark  (GtkFileSystem                  *file_system,
								   const GtkFilePath              *path,
								   gint                            position,
								   GError                        **error);
static gboolean              gtk_file_system_gio_remove_bookmark  (GtkFileSystem                  *file_system,
								   const GtkFilePath              *path,
								   GError                        **error);
static GSList *              gtk_file_system_gio_list_bookmarks   (GtkFileSystem                  *file_system);

static gchar *               gtk_file_system_gio_get_bookmark_label (GtkFileSystem                *file_system,
								     const GtkFilePath            *path);
static void                  gtk_file_system_gio_set_bookmark_label (GtkFileSystem                *file_system,
								     const GtkFilePath            *path,
								     const gchar                  *label);
static void                  query_info_callback                    (GObject      *source_object,
								     GAsyncResult *result,
								     gpointer      user_data);

/* GtkFileFolderGio methods */
static GType gtk_file_folder_gio_get_type       (void);
static void  gtk_file_folder_gio_iface_init     (GtkFileFolderIface     *iface);
static void  gtk_file_folder_gio_finalize       (GObject                *object);

/* GtkFileFolder implementation methods */
static GtkFileInfo *         gtk_file_folder_gio_get_info         (GtkFileFolder                  *folder,
								   const GtkFilePath              *path,
								   GError                        **error);
static gboolean              gtk_file_folder_gio_list_children    (GtkFileFolder                  *folder,
								   GSList                        **children,
								   GError                        **error);
static gboolean              gtk_file_folder_gio_is_finished_loading (GtkFileFolder               *folder);

/* The pointers we return for a GtkFileSystemVolume are opaque tokens; they are
 * really pointers to GDrive, GVolume or GMount objects.  We need an extra
 * token for the fake "File System" volume.  So, we'll return the pointer to
 * this particular string.
 */
static const char *root_volume_token = "File System";
#define IS_ROOT_VOLUME_TOKEN(volume) ((gpointer) (volume) == (gpointer) root_volume_token)

/* GtkFileSystem module methods */
void                         fs_module_init     (GTypeModule    *module);
void                         fs_module_exit     (void);
GtkFileSystem *              fs_module_create   (void);


G_DEFINE_DYNAMIC_TYPE_EXTENDED (GtkFileSystemGio,
				gtk_file_system_gio,
				G_TYPE_OBJECT,
				0,
				MODULE_IMPLEMENT_INTERFACE (GTK_TYPE_FILE_SYSTEM,
							    gtk_file_system_gio_iface_init))

G_DEFINE_DYNAMIC_TYPE (GtkFileSystemHandleGio,
		       gtk_file_system_handle_gio,
		       GTK_TYPE_FILE_SYSTEM_HANDLE)

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GtkFileFolderGio,
				gtk_file_folder_gio,
				G_TYPE_OBJECT,
				0,
				MODULE_IMPLEMENT_INTERFACE (GTK_TYPE_FILE_FOLDER,
							    gtk_file_folder_gio_iface_init))

/* GtkFileSystemGio methods */
static void
gtk_file_system_gio_class_init (GtkFileSystemGioClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = gtk_file_system_gio_dispose;
}

static void
gtk_file_system_gio_class_finalize (GtkFileSystemGioClass *class)
{
  DEBUG ("class_finalize");
}

static void
get_volumes_list (GtkFileSystemGio *file_system)
{
  GList *l, *ll;
  GList *drives;
  GList *volumes;
  GList *mounts;
  GDrive *drive;
  GVolume *volume;
  GMount *mount;

  if (file_system->volumes)
    {
      /* Remove the first one by hand */
      file_system->volumes = g_slist_remove (file_system->volumes, file_system->volumes->data);
      g_slist_foreach (file_system->volumes, (GFunc) g_object_unref, NULL);
      g_slist_free (file_system->volumes);
      file_system->volumes = NULL;
    }

  /* first go through all connected drives */
  drives = g_volume_monitor_get_connected_drives (file_system->volume_monitor);
  for (l = drives; l != NULL; l = l->next)
    {
      drive = l->data;

      volumes = g_drive_get_volumes (drive);
      if (volumes)
        {
          for (ll = volumes; ll != NULL; ll = ll->next)
            {
              volume = ll->data;
              mount = g_volume_get_mount (volume);
              if (mount)
                {
                  /* Show mounted volume */
                  file_system->volumes = g_slist_prepend (file_system->volumes, g_object_ref (mount));
                  g_object_unref (mount);
                }
              else
                {
                  /* Do show the unmounted volumes in the sidebar;
                   * this is so the user can mount it (in case automounting
                   * is off).
                   *
                   * Also, even if automounting is enabled, this gives a visual
                   * cue that the user should remember to yank out the media if
                   * he just unmounted it.
                   */
                  file_system->volumes = g_slist_prepend (file_system->volumes, g_object_ref (volume));
                }
              g_object_unref (volume);
            }
        }
      else
        {
          if (g_drive_is_media_removable (drive) && !g_drive_is_media_check_automatic (drive))
            {
              /* If the drive has no mountable volumes and we cannot detect media change.. we
               * display the drive in the sidebar so the user can manually poll the drive by
               * right clicking and selecting "Rescan..."
               *
               * This is mainly for drives like floppies where media detection doesn't
               * work.. but it's also for human beings who like to turn off media detection
               * in the OS to save battery juice.
               */

              file_system->volumes = g_slist_prepend (file_system->volumes, g_object_ref (drive));
            }
        }
      g_object_unref (drive);
    }
  g_list_free (drives);

  /* add all volumes that is not associated with a drive */
  volumes = g_volume_monitor_get_volumes (file_system->volume_monitor);
  for (l = volumes; l != NULL; l = l->next)
    {
      volume = l->data;
      drive = g_volume_get_drive (volume);
      if (drive)
        {
          g_object_unref (volume);
          g_object_unref (drive);
          continue;
        }
      mount = g_volume_get_mount (volume);
      if (mount)
        {
          /* show this mount */
          file_system->volumes = g_slist_prepend (file_system->volumes, g_object_ref (mount));
          g_object_unref (mount);
        }
      else
        {
          /* see comment above in why we add an icon for a volume */
          file_system->volumes = g_slist_prepend (file_system->volumes, g_object_ref (volume));
        }
      g_object_unref (volume);
    }
  g_list_free (volumes);

  /* add mounts that has no volume (/etc/mtab mounts, ftp, sftp,...) */
  mounts = g_volume_monitor_get_mounts (file_system->volume_monitor);
  for (l = mounts; l != NULL; l = l->next)
    {
      mount = l->data;
      volume = g_mount_get_volume (mount);
      if (volume)
        {
          g_object_unref (volume);
          g_object_unref (mount);
          continue;
        }

      /* show this mount */
      file_system->volumes = g_slist_prepend (file_system->volumes, g_object_ref (mount));
      g_object_unref (mount);
    }
  g_list_free (mounts);

  /* And finally, add the root volume to the front of the list */
  file_system->volumes = g_slist_prepend (file_system->volumes, (gpointer) root_volume_token);
}

static void
volumes_drives_changed (GVolumeMonitor *volume_monitor,
			GVolume        *volume,
			gpointer        user_data)
{
  GtkFileSystemGio *impl;

  impl = GTK_FILE_SYSTEM_GIO (user_data);
  gdk_threads_enter ();
  g_signal_emit_by_name (impl, "volumes-changed");
  gdk_threads_leave ();
}

static gchar *
get_bookmarks_filename (void)
{
  return g_build_filename (g_get_home_dir (),
			   ".gtk-bookmarks",
			   NULL);
}

static void
gtk_file_system_gio_init (GtkFileSystemGio *impl)
{
  GFile *bookmarks_file;
  gchar *path;

  DEBUG ("init");

  impl->volume_monitor = g_volume_monitor_get ();

  path = get_bookmarks_filename ();
  bookmarks_file = g_file_new_for_path (path);
  g_object_unref (bookmarks_file);
  g_free (path);

  g_signal_connect (impl->volume_monitor, "mount-added",
		    G_CALLBACK (volumes_drives_changed), impl);
  g_signal_connect (impl->volume_monitor, "mount-removed",
		    G_CALLBACK (volumes_drives_changed), impl);
  g_signal_connect (impl->volume_monitor, "mount-changed",
		    G_CALLBACK (volumes_drives_changed), impl);
  g_signal_connect (impl->volume_monitor, "volume-added",
		    G_CALLBACK (volumes_drives_changed), impl);
  g_signal_connect (impl->volume_monitor, "volume-removed",
		    G_CALLBACK (volumes_drives_changed), impl);
  g_signal_connect (impl->volume_monitor, "volume-changed",
		    G_CALLBACK (volumes_drives_changed), impl);
  g_signal_connect (impl->volume_monitor, "drive-connected",
		    G_CALLBACK (volumes_drives_changed), impl);
  g_signal_connect (impl->volume_monitor, "drive-disconnected",
		    G_CALLBACK (volumes_drives_changed), impl);
  g_signal_connect (impl->volume_monitor, "drive-changed",
		    G_CALLBACK (volumes_drives_changed), impl);

  /* This cancellable will be used for cancelling ongoing
   * enumerator_next_files operations when the filesystem
   * is being disposed
   */
  impl->cancellable = g_cancellable_new ();
}

static void
gtk_file_system_gio_dispose (GObject *object)
{
  GtkFileSystemGio *impl;

  DEBUG ("dispose");

  impl = GTK_FILE_SYSTEM_GIO (object);

  if (impl->cancellable)
    {
      g_cancellable_cancel (impl->cancellable);
      g_object_unref (impl->cancellable);
      impl->cancellable = NULL;
    }

  if (impl->volumes)
    {
      /* Remove the first one by hand */
      impl->volumes = g_slist_remove (impl->volumes, impl->volumes->data);
      g_slist_foreach (impl->volumes, (GFunc) g_object_unref, NULL);
      g_slist_free (impl->volumes);
      impl->volumes = NULL;
    }

  if (impl->volume_monitor)
    g_object_unref (impl->volume_monitor);

  G_OBJECT_CLASS (gtk_file_system_gio_parent_class)->dispose (object);
}

/* GtkFileSystemHandleGio methods */
static void
gtk_file_system_handle_gio_class_init (GtkFileSystemHandleGioClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = gtk_file_system_handle_gio_finalize;
}

static void
gtk_file_system_handle_gio_class_finalize (GtkFileSystemHandleGioClass *class)
{
}

static void
gtk_file_system_handle_gio_init (GtkFileSystemHandleGio *impl)
{
}

static void
gtk_file_system_handle_gio_finalize (GObject *object)
{
  GtkFileSystemHandleGio *handle;

  DEBUG ("handle finalize");

  handle = GTK_FILE_SYSTEM_HANDLE_GIO (object);

  if (handle->cancellable)
    g_object_unref (handle->cancellable);

  G_OBJECT_CLASS (gtk_file_system_handle_gio_parent_class)->finalize (object);
}

/* GtkFileSystem interface implementation */
static void
gtk_file_system_gio_iface_init (GtkFileSystemIface *iface)
{
  DEBUG ("iface_init");

  iface->list_volumes = gtk_file_system_gio_list_volumes;
  iface->get_volume_for_path = gtk_file_system_gio_get_volume_for_path;
  iface->get_folder = gtk_file_system_gio_get_folder;
  iface->get_info = gtk_file_system_gio_get_info;
  iface->create_folder = gtk_file_system_gio_create_folder;
  iface->cancel_operation = gtk_file_system_gio_cancel_operation;
  iface->volume_free = gtk_file_system_gio_volume_free;
  iface->volume_get_base_path = gtk_file_system_gio_volume_get_base_path;
  iface->volume_get_is_mounted = gtk_file_system_gio_volume_get_is_mounted;
  iface->volume_mount = gtk_file_system_gio_volume_mount;
  iface->volume_get_display_name = gtk_file_system_gio_volume_get_display_name;
  iface->volume_get_icon_name = gtk_file_system_gio_volume_get_icon_name;
  iface->get_parent = gtk_file_system_gio_get_parent;
  iface->make_path = gtk_file_system_gio_make_path;
  iface->parse = gtk_file_system_gio_parse;
  iface->path_to_uri = gtk_file_system_gio_path_to_uri;
  iface->path_to_filename = gtk_file_system_gio_path_to_filename;
  iface->uri_to_path = gtk_file_system_gio_uri_to_path;
  iface->filename_to_path = gtk_file_system_gio_filename_to_path;
  iface->insert_bookmark = gtk_file_system_gio_insert_bookmark;
  iface->remove_bookmark = gtk_file_system_gio_remove_bookmark;
  iface->list_bookmarks = gtk_file_system_gio_list_bookmarks;
  iface->get_bookmark_label = gtk_file_system_gio_get_bookmark_label;
  iface->set_bookmark_label = gtk_file_system_gio_set_bookmark_label;
}

static GFile *
get_file_from_path (const GtkFilePath *path)
{
  const gchar *uri;

  uri = gtk_file_path_get_string (path);
  return g_file_new_for_uri (uri);
}

static GtkFilePath *
get_path_from_file (GFile *file)
{
  gchar *uri;

  uri = g_file_get_uri (file);
  return gtk_file_path_new_steal (uri);
}

static GSList *
gtk_file_system_gio_list_volumes (GtkFileSystem *file_system)
{
  GtkFileSystemGio *file_system_gio;
  GSList *list;

  DEBUG ("list_volumes");

  get_volumes_list (GTK_FILE_SYSTEM_GIO (file_system));

  file_system_gio = GTK_FILE_SYSTEM_GIO (file_system);
  list = g_slist_copy (file_system_gio->volumes);
  /* Don't try to ref the "File System" one */
  g_slist_foreach (list->next, (GFunc) g_object_ref, NULL);

  return list;
}

static GtkFileSystemVolume *
gtk_file_system_gio_get_volume_for_path (GtkFileSystem     *file_system,
					 const GtkFilePath *path)
{
  GtkFileSystemGio *file_system_gio;
  GFile *file;
  GMount *mount;
  GSList *list;
  const char *uri;

  DEBUG ("get_volume_for_path");

  uri = gtk_file_path_get_string (path);
  if (strcmp (uri, "file:///") == 0)
    return (GtkFileSystemVolume *) root_volume_token;

  file_system_gio = GTK_FILE_SYSTEM_GIO (file_system);
  file = g_file_new_for_uri (uri);

  g_return_val_if_fail (file != NULL, NULL);

  /* Skip the first item on the list! */
  for (list = file_system_gio->volumes->next; list; list = list->next)
    {
      if (g_type_is_a (G_OBJECT_TYPE (list->data), G_TYPE_MOUNT))
        {
          GFile *root;

          mount = list->data;
          root = g_mount_get_root (mount);
          if (g_file_has_prefix (file, root))
            {
              mount = list->data;
              break;
            }
          g_object_unref (root);

          mount = NULL;
        }

    }

  g_object_unref (file);

  if (mount)
    return (GtkFileSystemVolume *) g_object_ref (mount);

  return NULL;
}

static void
enumerator_files_callback (GObject      *source_object,
			   GAsyncResult *result,
			   gpointer      user_data)
{
  GFileEnumerator *enumerator;
  GtkFileFolderGio *folder;
  GError *error = NULL;
  GSList *added_files = NULL;
  GList *files, *f;

  folder = GTK_FILE_FOLDER_GIO (user_data);
  enumerator = G_FILE_ENUMERATOR (source_object);
  files = g_file_enumerator_next_files_finish (enumerator, result, &error);

  if (!files)
    {
      /* There's no way to spread the error up to the filechooser, if any */
      g_file_enumerator_close_async (enumerator,
				     G_PRIORITY_DEFAULT,
				     NULL, NULL, NULL);

      folder->finished_loading = TRUE;
      gdk_threads_enter ();
      g_signal_emit_by_name (folder, "finished-loading", 0);
      gdk_threads_leave ();
      g_object_unref (folder);
      return;
    }

  for (f = files; f; f = f->next)
    {
      GFileInfo *info;
      GFile *child_file;

      info = f->data;
      child_file = g_file_resolve_relative_path (folder->parent_file, g_file_info_get_name (info));
      g_hash_table_insert (folder->children, g_file_get_uri (child_file), info);
      added_files = g_slist_prepend (added_files, get_path_from_file (child_file));

      g_object_unref (child_file);
    }

  g_file_enumerator_next_files_async (enumerator, FILES_PER_QUERY,
				      G_PRIORITY_DEFAULT,
				      folder->cancellable,
				      enumerator_files_callback,
				      folder);

  gdk_threads_enter ();
  g_signal_emit_by_name (folder, "files-added", added_files);
  gdk_threads_leave ();
  g_slist_foreach (added_files, (GFunc) g_free, NULL);
  g_slist_free (added_files);

  g_list_free (files);
}

static void
directory_monitor_changed (GFileMonitor      *monitor,
			   GFile             *file,
			   GFile             *other_file,
			   GFileMonitorEvent  event,
			   gpointer           data)
{
  GtkFileFolder *folder;
  GSList *files;

  folder = GTK_FILE_FOLDER (data);
  files = g_slist_prepend (NULL, get_path_from_file (file));

  switch (event)
    {
    case G_FILE_MONITOR_EVENT_CREATED:
      gdk_threads_enter ();
      g_signal_emit_by_name (folder, "files-added", files);
      gdk_threads_leave ();
      break;
    case G_FILE_MONITOR_EVENT_DELETED:
      gdk_threads_enter ();
      g_signal_emit_by_name (folder, "files-removed", files);
      gdk_threads_leave ();
      break;
    default:
      break;
    }
}

static void
enumerate_children_callback (GObject      *source_object,
			     GAsyncResult *result,
			     gpointer      user_data)
{
  GtkFileSystemGio *file_system;
  GtkFileSystemHandleGio *handle;
  GtkFileFolderGio *folder = NULL;
  GFileEnumerator *enumerator;
  GFile *file;
  GError *error = NULL;

  file = G_FILE (source_object);
  handle = GTK_FILE_SYSTEM_HANDLE_GIO (user_data);
  file_system = GTK_FILE_SYSTEM_GIO (GTK_FILE_SYSTEM_HANDLE (handle)->file_system);
  enumerator = g_file_enumerate_children_finish (file, result, &error);

  if (enumerator)
    {
      folder = g_object_new (GTK_TYPE_FILE_FOLDER_GIO, NULL);
      folder->cancellable = g_object_ref (file_system->cancellable);
      folder->parent_file = g_object_ref (file);
      folder->children = g_hash_table_new_full (g_str_hash, g_str_equal,
						(GDestroyNotify) g_free,
						(GDestroyNotify) g_object_unref);
      folder->finished_loading = FALSE;

      folder->directory_monitor = g_file_monitor_directory (file, G_FILE_MONITOR_NONE, NULL, &error);

      if (error)
	g_warning (error->message);
      else
	g_signal_connect (folder->directory_monitor, "changed",
			  G_CALLBACK (directory_monitor_changed), folder);

      g_file_enumerator_next_files_async (enumerator, FILES_PER_QUERY,
					  G_PRIORITY_DEFAULT,
					  folder->cancellable,
					  enumerator_files_callback,
					  g_object_ref (folder));
      g_object_unref (enumerator);
    }

  gdk_threads_enter ();
  ((GtkFileSystemGetFolderCallback) handle->callback) (GTK_FILE_SYSTEM_HANDLE (handle),
						       GTK_FILE_FOLDER (folder),
						       error, handle->data);
  gdk_threads_leave ();
}

static GtkFileSystemHandle *
gtk_file_system_gio_get_folder (GtkFileSystem                  *file_system,
				const GtkFilePath              *path,
				GtkFileInfoType                 types,
				GtkFileSystemGetFolderCallback  callback,
				gpointer                        data)
{
  GtkFileSystemHandleGio *handle;
  GFile *file;

  DEBUG ("get_folder");

  file = get_file_from_path (path);

  g_return_val_if_fail (file != NULL, NULL);

  handle = g_object_new (GTK_TYPE_FILE_SYSTEM_HANDLE_GIO, NULL);
  GTK_FILE_SYSTEM_HANDLE (handle)->file_system = file_system;
  handle->cancellable = g_cancellable_new ();
  handle->callback = callback;
  handle->data = data;

  g_file_enumerate_children_async (file, "standard,time,thumbnail::*", 0, 0,
				   handle->cancellable,
				   enumerate_children_callback,
				   handle);
  g_object_unref (file);

  return GTK_FILE_SYSTEM_HANDLE (handle);
}

static gchar *
get_icon_for_special_directory (GFile *file)
{
  GFile *special_file;
  gboolean equal;

  /* check for root directory */
  special_file = g_file_get_parent (file);

  if (!special_file)
    return "gnome-dev-harddisk";

  g_object_unref (special_file);

  /* check for desktop */
  special_file = g_file_new_for_path (g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP));
  equal = g_file_equal (file, special_file);
  g_object_unref (special_file);

  if (equal)
    return "gnome-fs-desktop";

  /* check for home */
  special_file = g_file_new_for_path (g_get_home_dir ());
  equal = g_file_equal (file, special_file);
  g_object_unref (special_file);

  if (equal)
    return "gnome-fs-home";

  return NULL;
}

static gchar *
get_icon_string (GIcon *icon)
{
  gchar *name = NULL;

  if (!icon)
    return NULL;

  if (G_IS_THEMED_ICON (icon))
    {
      const gchar * const *names;

      /* FIXME: choose between names */
      names = g_themed_icon_get_names (G_THEMED_ICON (icon));

      if (names)
	name = g_strdup (names [0]);
    }
  else if (G_IS_FILE_ICON (icon))
    {
      GFile *icon_file;

      icon_file = g_file_icon_get_file (G_FILE_ICON (icon));

      if (icon_file)
	name = g_file_get_path (icon_file);
    }

  return name;
}

static GtkFileInfo *
translate_file_info (GFile     *file,
		     GFileInfo *file_info)
{
  GtkFileInfo *info;
  gboolean is_folder;
  GTimeVal mtime;
  const gchar *thumbnail_path;

  info = gtk_file_info_new ();
  is_folder = (g_file_info_get_file_type (file_info) == G_FILE_TYPE_DIRECTORY);
  g_file_info_get_modification_time (file_info, &mtime);

  gtk_file_info_set_display_name (info, g_file_info_get_display_name (file_info));
  gtk_file_info_set_is_folder (info, is_folder);
  gtk_file_info_set_is_hidden (info, g_file_info_get_is_hidden (file_info));
  gtk_file_info_set_mime_type (info, g_file_info_get_content_type (file_info));
  gtk_file_info_set_modification_time (info, mtime.tv_sec);
  gtk_file_info_set_size (info, g_file_info_get_size (file_info));

  thumbnail_path = g_file_info_get_attribute_byte_string (file_info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH);

  if (thumbnail_path)
    gtk_file_info_set_icon_name (info, thumbnail_path);
  else
    {
      const gchar *icon_name;

      icon_name = get_icon_for_special_directory (file);
      if (icon_name)
        {
          gtk_file_info_set_icon_name (info, icon_name);
	}
      else
        {
          GIcon *icon;
          gchar *name;

	  icon = g_file_info_get_icon (file_info);
	  name = get_icon_string (icon);
	  gtk_file_info_set_icon_name (info, name);

	  g_free (name);
	}
    }

  return info;
}

static void
mount_async_callback (GObject      *source_object,
		      GAsyncResult *result,
		      gpointer      user_data)
{
  GtkFileSystemHandleGio *handle;
  GError *error = NULL;
  GFile *file;

  DEBUG ("mount_async_callback");

  file = G_FILE (source_object);
  handle = GTK_FILE_SYSTEM_HANDLE_GIO (user_data);
  if (g_file_mount_enclosing_volume_finish (file, result, &error))
    {
      g_file_query_info_async (file, "standard,time,thumbnail::*", 0, 0,
			       handle->cancellable,
			       query_info_callback,
			       handle);
    }
  else
    {
      gdk_threads_enter ();
      ((GtkFileSystemGetInfoCallback) handle->callback) (GTK_FILE_SYSTEM_HANDLE (handle),
							 NULL, error, handle->data);
      gdk_threads_leave ();
    }
}

static void
query_info_callback (GObject      *source_object,
		     GAsyncResult *result,
		     gpointer      user_data)
{
  GtkFileSystemHandleGio *handle;
  GtkFileInfo *info = NULL;
  GError *error = NULL;
  GFileInfo *file_info;
  GFile *file;

  DEBUG ("query_info_callback");

  file = G_FILE (source_object);
  handle = GTK_FILE_SYSTEM_HANDLE_GIO (user_data);
  file_info = g_file_query_info_finish (file, result, &error);

  if (file_info)
    {
      info = translate_file_info (file, file_info);
      g_object_unref (file_info);
    }
  else if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_MOUNTED) && !handle->tried_mount)
    {
      /* If it's not mounted, try to mount it ourselves */
      g_error_free (error);
      handle->tried_mount = TRUE;
      g_file_mount_enclosing_volume (file, G_MOUNT_MOUNT_NONE, NULL,
				     handle->cancellable,
				     mount_async_callback,
				     handle);
      return;
    }

  gdk_threads_enter ();
  ((GtkFileSystemGetInfoCallback) handle->callback) (GTK_FILE_SYSTEM_HANDLE (handle),
						     info, error, handle->data);
  gdk_threads_leave ();

  if (info)
    gtk_file_info_free (info);
}

static GtkFileSystemHandle *
gtk_file_system_gio_get_info (GtkFileSystem                *file_system,
			      const GtkFilePath            *path,
			      GtkFileInfoType               types,
			      GtkFileSystemGetInfoCallback  callback,
			      gpointer                      data)
{
  GtkFileSystemHandleGio *handle;
  GFile *file;

  DEBUG ("get_info");

  file = get_file_from_path (path);

  g_return_val_if_fail (file != NULL, NULL);

  handle = g_object_new (GTK_TYPE_FILE_SYSTEM_HANDLE_GIO, NULL);
  GTK_FILE_SYSTEM_HANDLE (handle)->file_system = file_system;
  handle->cancellable = g_cancellable_new ();
  handle->callback = callback;
  handle->data = data;
  handle->tried_mount = FALSE;

  g_file_query_info_async (file, "standard,time,thumbnail::*", 0, 0,
			   handle->cancellable,
			   query_info_callback,
			   handle);

  g_object_unref (file);

  return GTK_FILE_SYSTEM_HANDLE (g_object_ref (handle));
}

typedef struct
{
  GtkFilePath *path;
  GtkFileSystemHandleGio *handle;
} CreateFolderData;

static gboolean
create_folder_callback (gpointer data)
{
  GtkFileSystemHandleGio *handle;
  CreateFolderData *idle_data;
  GError *error = NULL;
  GFile *file;

  idle_data = (CreateFolderData *) data;
  handle = idle_data->handle;
  file = get_file_from_path (idle_data->path);

  g_file_make_directory (file, handle->cancellable, &error);

  ((GtkFileSystemCreateFolderCallback) handle->callback) (GTK_FILE_SYSTEM_HANDLE (handle),
							  idle_data->path, error, handle->data);
  g_object_unref (file);
  gtk_file_path_free (idle_data->path);
  g_slice_free (CreateFolderData, idle_data);

  return FALSE;
}

static GtkFileSystemHandle *
gtk_file_system_gio_create_folder (GtkFileSystem                     *file_system,
				   const GtkFilePath                 *path,
				   GtkFileSystemCreateFolderCallback  callback,
				   gpointer                           data)
{
  GtkFileSystemHandleGio *handle;
  CreateFolderData *idle_data;

  DEBUG ("create_folder");

  /* FIXME: make_directory() doesn't seem to have async version */

  handle = g_object_new (GTK_TYPE_FILE_SYSTEM_HANDLE_GIO, NULL);
  GTK_FILE_SYSTEM_HANDLE (handle)->file_system = file_system;
  handle->cancellable = g_cancellable_new ();
  handle->callback = callback;
  handle->data = data;

  idle_data = g_slice_new (CreateFolderData);
  idle_data->path = gtk_file_path_copy (path);
  idle_data->handle = handle;

  handle->source_id = gdk_threads_add_idle (create_folder_callback, idle_data);

  return GTK_FILE_SYSTEM_HANDLE (handle);
}

static void
gtk_file_system_gio_cancel_operation (GtkFileSystemHandle *handle)
{
  GtkFileSystemHandleGio *handle_gio;

  DEBUG ("cancel_operation");

  handle_gio = GTK_FILE_SYSTEM_HANDLE_GIO (handle);

  if (handle_gio->cancellable)
    {
      g_cancellable_cancel (handle_gio->cancellable);
      g_object_unref (handle_gio->cancellable);
      handle_gio->cancellable = NULL;
    }

  if (handle_gio->source_id)
    {
      /* This is only for functions without async option */
      g_source_remove (handle_gio->source_id);
      handle_gio->source_id = 0;
    }
}

static void
gtk_file_system_gio_volume_free (GtkFileSystem       *file_system,
				 GtkFileSystemVolume *volume)
{
  DEBUG ("volume_free");
  if (IS_ROOT_VOLUME_TOKEN(volume))
    return;

  g_object_unref (G_OBJECT (volume));
}

static GtkFilePath *
gtk_file_system_gio_volume_get_base_path (GtkFileSystem       *file_system,
					  GtkFileSystemVolume *file_system_volume)
{
  GFile *root;
  GtkFilePath *path;

  DEBUG ("volume_get_base_path");

  if (IS_ROOT_VOLUME_TOKEN (file_system_volume))
    return gtk_file_path_new_dup ("file:///");

  path = NULL;

  if (g_type_is_a (G_OBJECT_TYPE (file_system_volume), G_TYPE_MOUNT))
    {
      GMount *mount = G_MOUNT (file_system_volume);

      root = g_mount_get_root (mount);
      path = get_path_from_file (root);
      g_object_unref (root);
    }
  else if (g_type_is_a (G_OBJECT_TYPE (file_system_volume), G_TYPE_VOLUME))
    {
      GMount *mount;
      GVolume *volume = G_VOLUME (file_system_volume);

      mount = g_volume_get_mount (volume);

      if (mount)
        {
          root = g_mount_get_root (mount);
          path = get_path_from_file (root);
          g_object_unref (mount);
        }
    }

  return path;
}

static gboolean
gtk_file_system_gio_volume_get_is_mounted (GtkFileSystem       *file_system,
					   GtkFileSystemVolume *file_system_volume)
{
  gboolean mounted;

  DEBUG ("volume_get_is_mounted");

  if (IS_ROOT_VOLUME_TOKEN (file_system_volume))
    return TRUE;

  mounted = FALSE;

  if (g_type_is_a (G_OBJECT_TYPE (file_system_volume), G_TYPE_MOUNT))
    mounted = TRUE;
  else if (g_type_is_a (G_OBJECT_TYPE (file_system_volume), G_TYPE_VOLUME))
    {
      GMount *mount;
      GVolume *volume = G_VOLUME (file_system_volume);

      mount = g_volume_get_mount (volume);

      if (mount)
        {
          mounted = TRUE;
          g_object_unref (mount);
        }
    }

  return mounted;
}

static void
volume_mount_cb (GObject *source_object,
		 GAsyncResult *res,
		 gpointer user_data)
{
  GError *error = NULL;
  GtkFileSystemHandleGio *handle;

  handle = GTK_FILE_SYSTEM_HANDLE_GIO (user_data);

  g_volume_mount_finish (G_VOLUME (source_object), res, &error);

  gdk_threads_enter ();
  ((GtkFileSystemVolumeMountCallback) handle->callback) (GTK_FILE_SYSTEM_HANDLE (handle),
                                                         (GtkFileSystemVolume *) source_object,
                                                         error, handle->data);
  gdk_threads_leave ();

  if (error)
    g_error_free (error);
}

static void
drive_poll_for_media_cb (GObject *source_object,
                         GAsyncResult *res,
                         gpointer user_data)
{
  GError *error = NULL;
  GtkFileSystemHandleGio *handle;

  handle = GTK_FILE_SYSTEM_HANDLE_GIO (user_data);

  g_drive_poll_for_media_finish (G_DRIVE (source_object), res, &error);

  gdk_threads_enter ();
  ((GtkFileSystemVolumeMountCallback) handle->callback) (GTK_FILE_SYSTEM_HANDLE (handle),
                                                         (GtkFileSystemVolume *) source_object,
                                                         error, handle->data);
  gdk_threads_leave ();

  if (error)
    g_error_free (error);
}

GtkFileSystemHandle *
gtk_file_system_gio_volume_mount (GtkFileSystem                    *file_system,
				  GtkFileSystemVolume              *file_system_volume,
				  GtkFileSystemVolumeMountCallback  callback,
				  gpointer                          data)
{
  GtkFileSystemHandleGio *handle;

  DEBUG ("volume_mount");

  handle = g_object_new (GTK_TYPE_FILE_SYSTEM_HANDLE_GIO, NULL);
  GTK_FILE_SYSTEM_HANDLE (handle)->file_system = file_system;
  handle->cancellable = g_cancellable_new ();
  handle->callback = callback;
  handle->data = data;

  if (g_type_is_a (G_OBJECT_TYPE (file_system_volume), G_TYPE_DRIVE))
    {
      GDrive *drive = G_DRIVE (file_system_volume);

      /* this path happens for drives that are not polled by the OS and where the last media
       * check indicated that no media was available. So the thing to do here is to
       * invoke poll_for_media() on the drive
       */
      g_drive_poll_for_media (drive, handle->cancellable, drive_poll_for_media_cb, handle);
    }
  else
    {
      GVolume *volume = G_VOLUME (file_system_volume);
      GMountOperation *mount_op;

      mount_op = g_mount_operation_new ();
      g_volume_mount (volume, 0, mount_op, handle->cancellable, volume_mount_cb, handle);
      g_object_unref (mount_op);
    }

  return GTK_FILE_SYSTEM_HANDLE (handle);
}

static gchar *
gtk_file_system_gio_volume_get_display_name (GtkFileSystem       *file_system,
					     GtkFileSystemVolume *file_system_volume)
{
  gchar *name = NULL;

  DEBUG ("volume_get_display_name");

  if (IS_ROOT_VOLUME_TOKEN (file_system_volume))
    return g_strdup (_("File System"));

  if (g_type_is_a (G_OBJECT_TYPE (file_system_volume), G_TYPE_DRIVE))
    {
      GDrive *drive = G_DRIVE (file_system_volume);
      name = g_drive_get_name (drive);
    }
  else if (g_type_is_a (G_OBJECT_TYPE (file_system_volume), G_TYPE_VOLUME))
    {
      GVolume *volume = G_VOLUME (file_system_volume);
      name = g_volume_get_name (volume);
    }
  else if (g_type_is_a (G_OBJECT_TYPE (file_system_volume), G_TYPE_MOUNT))
    {
      GMount *mount = G_MOUNT (file_system_volume);
      name = g_mount_get_name (mount);
    }

  return name;
}

static gchar *
gtk_file_system_gio_volume_get_icon_name (GtkFileSystem        *file_system,
					  GtkFileSystemVolume  *file_system_volume,
					  GError              **error)
{
  char *name;
  GIcon *icon = NULL;

  DEBUG ("volume_get_icon_name");

  if (IS_ROOT_VOLUME_TOKEN (file_system_volume))
    return g_strdup ("gnome-dev-harddisk");

  if (g_type_is_a (G_OBJECT_TYPE (file_system_volume), G_TYPE_DRIVE))
    {
      GDrive *drive = G_DRIVE (file_system_volume);
      icon = g_drive_get_icon (drive);
    }
  else if (g_type_is_a (G_OBJECT_TYPE (file_system_volume), G_TYPE_VOLUME))
    {
      GVolume *volume = G_VOLUME (file_system_volume);
      icon = g_volume_get_icon (volume);
    }
  else if (g_type_is_a (G_OBJECT_TYPE (file_system_volume), G_TYPE_MOUNT))
    {
      GMount *mount = G_MOUNT (file_system_volume);
      const char *icon_name;
      GFile *file;

      file = g_mount_get_root (mount);
      icon_name = get_icon_for_special_directory (file);
      g_object_unref (file);

      if (icon_name)
        return g_strdup (icon_name);

      icon = g_mount_get_icon (mount);
    }

  name = get_icon_string (icon);

  return name;
}

static gboolean
gtk_file_system_gio_get_parent (GtkFileSystem      *file_system,
				const GtkFilePath  *path,
				GtkFilePath       **parent,
				GError            **error)
{
  GFile *file, *parent_file;

  DEBUG ("get_parent");
  file = get_file_from_path (path);
  parent_file = g_file_get_parent (file);

  if (parent_file)
    {
      *parent = get_path_from_file (parent_file);
      g_object_unref (parent_file);
    }
  else
    *parent = NULL;

  g_object_unref (file);

  return TRUE;
}

static GtkFilePath *
gtk_file_system_gio_make_path (GtkFileSystem      *file_system,
			       const GtkFilePath  *base_path,
			       const gchar        *display_name,
			       GError            **error)
{
  GFile *base_path_file, *file;
  GtkFilePath *path = NULL;

  DEBUG ("make_path");

  /* FIXME: should check for dir separator */

  base_path_file = get_file_from_path (base_path);
  file = g_file_get_child_for_display_name (base_path_file, display_name, error);
  g_object_unref (base_path_file);

  if (file)
    {
      path = get_path_from_file (file);
      g_object_unref (file);
    }

  return path;
}

static gboolean
gtk_file_system_gio_parse (GtkFileSystem     *file_system,
			   const GtkFilePath *base_path,
			   const gchar       *str,
			   GtkFilePath      **folder,
			   gchar            **file_part,
			   GError           **error)
{
  GFile *base_path_file, *file;
  gboolean result = FALSE;
  gboolean is_dir = FALSE;
  gchar *last_slash = NULL;

  DEBUG ("parse");

  if (str && *str)
    is_dir = (str [strlen (str) - 1] == G_DIR_SEPARATOR);

  last_slash = strrchr (str, G_DIR_SEPARATOR);
  base_path_file = get_file_from_path (base_path);

  if (str[0] == '~')
    file = g_file_parse_name (str);
  else
    file = g_file_resolve_relative_path (base_path_file, str);

  if (g_file_equal (base_path_file, file))
    {
      /* this is when user types '.', could be the
       * beginning of a hidden file, ./ or ../
       */
      *folder = get_path_from_file (file);
      *file_part = g_strdup (str);
      result = TRUE;
    }
  else if (is_dir)
    {
      /* it's a dir, or at least it ends with the dir separator */
      *folder = get_path_from_file (file);
      *file_part = g_strdup ("");
      result = TRUE;
    }
  else
    {
      GFile *parent_file;

      parent_file = g_file_get_parent (file);

      if (!parent_file)
	{
	  g_set_error (error,
		       GTK_FILE_SYSTEM_ERROR,
		       GTK_FILE_SYSTEM_ERROR_NONEXISTENT,
		       "Could not get parent file");
	  *folder = NULL;
	  *file_part = NULL;
	}
      else
	{
	  *folder = get_path_from_file (parent_file);
	  g_object_unref (parent_file);

	  result = TRUE;

	  if (last_slash)
	    *file_part = g_strdup (last_slash + 1);
	  else
	    *file_part = g_strdup (str);
	}
    }

  g_object_unref (base_path_file);
  g_object_unref (file);

  return result;
}

static gchar *
gtk_file_system_gio_path_to_uri (GtkFileSystem     *file_system,
				 const GtkFilePath *path)
{
  GFile *file;
  gchar *uri;

  DEBUG ("path_to_uri");

  file = get_file_from_path (path);
  uri = g_file_get_uri (file);
  g_object_unref (file);

  return uri;
}

static gchar *
gtk_file_system_gio_path_to_filename (GtkFileSystem     *file_system,
				      const GtkFilePath *path)
{
  GFile *file;
  gchar *path_str;

  DEBUG ("path_to_filename");

  file = get_file_from_path (path);
  path_str = g_file_get_path (file);
  g_object_unref (file);

  return path_str;
}

static GtkFilePath *
gtk_file_system_gio_uri_to_path (GtkFileSystem *file_system,
				 const gchar   *uri)
{
  GFile *file;
  GtkFilePath *path;

  DEBUG ("uri_to_path");

  /* leave to GFile the task of canonicalizing
   * the uri in order to create the GtkFilePath
   */
  file = g_file_new_for_uri (uri);
  path = get_path_from_file (file);
  g_object_unref (file);

  return path;
}

static GtkFilePath *
gtk_file_system_gio_filename_to_path (GtkFileSystem *file_system,
				      const gchar   *filename)
{
  GFile *file;
  GtkFilePath *path;

  DEBUG ("filename_to_path");
  file = g_file_new_for_path (filename);
  path = get_path_from_file (file);
  g_object_unref (file);

  return path;
}

static GList *
read_bookmarks_file (void)
{
  gchar *filename, *contents;
  gchar **lines, *space;
  GError *error = NULL;
  GList *bookmarks = NULL;
  GFile *file;
  gint i;

  filename = get_bookmarks_filename ();
  file = g_file_new_for_path (filename);
  g_free (filename);

  if (!g_file_load_contents (file, NULL, &contents,
			     NULL, NULL, &error))
    {
      if (error)
	{
	  g_critical (error->message);
	  g_error_free (error);
	}

      return NULL;
    }

  lines = g_strsplit (contents, "\n", -1);

  for (i = 0; lines[i]; i++)
    {
      BookmarkEntry *entry;

      if (!*lines[i])
	continue;

      entry = g_slice_new0 (BookmarkEntry);

      if ((space = strchr (lines[i], ' ')) != NULL)
	{
	  space[0] = '\0';
	  entry->label = g_strdup (space + 1);
	}

      entry->uri = g_strdup (lines[i]);
      bookmarks = g_list_prepend (bookmarks, entry);
    }

  g_strfreev (lines);
  g_free (contents);
  g_object_unref (file);

  return bookmarks;
}

static void
save_bookmarks_file (GList *bookmarks)
{
  GError *error = NULL;
  gchar *filename;
  GString *contents;
  GList *elem;
  GFile *file;

  /* read_bookmarks_file returns the list reversed
   * in order to just prepend elements in list_bookmarks,
   * so to keep the same order we have to reverse it here
   */
  bookmarks = g_list_reverse (bookmarks);

  filename = get_bookmarks_filename ();
  file = g_file_new_for_path (filename);
  g_free (filename);

  contents = g_string_new ("");

  for (elem = bookmarks; elem; elem = elem->next)
    {
      BookmarkEntry *entry = elem->data;

      g_string_append (contents, entry->uri);

      if (entry->label)
	g_string_append_printf (contents, " %s", entry->label);

      g_string_append_c (contents, '\n');
    }

  if (!g_file_replace_contents (file, contents->str,
				strlen (contents->str),
				NULL, FALSE, 0, NULL,
				NULL, &error))
    {
      g_critical (error->message);
      g_error_free (error);
    }

  g_object_unref (file);
  g_string_free (contents, TRUE);
}

static void
free_bookmark_entry (BookmarkEntry *entry)
{
  g_free (entry->uri);
  g_free (entry->label);
  g_slice_free (BookmarkEntry, entry);
}

static void
free_bookmarks (GList *bookmarks)
{
  g_list_foreach (bookmarks, (GFunc) free_bookmark_entry, NULL);
  g_list_free (bookmarks);
}

static gboolean
gtk_file_system_gio_insert_bookmark (GtkFileSystem      *file_system,
				     const GtkFilePath  *path,
				     gint                position,
				     GError            **error)
{
  GList *bookmarks, *elem;
  BookmarkEntry *entry;
  gboolean result = TRUE;
  gchar *uri;

  bookmarks = read_bookmarks_file ();
  uri = gtk_file_system_gio_path_to_uri (file_system, path);

  for (elem = bookmarks; elem; elem = elem->next)
    {
      entry = elem->data;

      if (strcmp (uri, entry->uri) == 0)
	{
	  /* uh oh, found the same entry */
	  result = FALSE;
	  break;
	}
    }

  if (!result)
    {
      g_set_error (error,
		   GTK_FILE_SYSTEM_ERROR,
		   GTK_FILE_SYSTEM_ERROR_ALREADY_EXISTS,
		   "%s already exists in the bookmarks list",
		   uri);
      g_free (uri);
      return FALSE;
    }

  entry = g_slice_new0 (BookmarkEntry);
  entry->uri = uri;

  bookmarks = g_list_insert (bookmarks, entry, position);
  save_bookmarks_file (bookmarks);
  free_bookmarks (bookmarks);

  g_signal_emit_by_name (file_system, "bookmarks-changed", 0);

  return TRUE;
}

static gboolean
gtk_file_system_gio_remove_bookmark (GtkFileSystem      *file_system,
				     const GtkFilePath  *path,
				     GError            **error)
{
  GList *bookmarks;
  gboolean result = FALSE;
  GList *elem;
  gchar *uri;

  bookmarks = read_bookmarks_file ();

  if (!bookmarks)
    return FALSE;

  uri = gtk_file_system_gio_path_to_uri (file_system, path);

  for (elem = bookmarks; elem; elem = elem->next)
    {
      BookmarkEntry *entry = (BookmarkEntry *) elem->data;

      if (strcmp (uri, entry->uri) != 0)
	continue;

      result = TRUE;
      bookmarks = g_list_remove (bookmarks, entry);
      free_bookmark_entry (entry);
    }

  if (!result)
    {
      g_set_error (error,
		   GTK_FILE_SYSTEM_ERROR,
		   GTK_FILE_SYSTEM_ERROR_NONEXISTENT,
		   "%s does not exist in the bookmarks list",
		   uri);
      return FALSE;
    }

  save_bookmarks_file (bookmarks);
  free_bookmarks (bookmarks);
  g_free (uri);

  g_signal_emit_by_name (file_system, "bookmarks-changed", 0);

  return TRUE;
}

static GSList *
gtk_file_system_gio_list_bookmarks (GtkFileSystem *file_system)
{
  GList *bookmarks, *elem;
  GSList *list = NULL;

  DEBUG ("list_bookmarks");
  bookmarks = read_bookmarks_file ();

  for (elem = bookmarks; elem; elem = elem->next)
    {
      BookmarkEntry *entry = elem->data;
      list = g_slist_prepend (list, g_strdup (entry->uri));
    }

  free_bookmarks (bookmarks);

  return list;
}

static gchar *
gtk_file_system_gio_get_bookmark_label (GtkFileSystem     *file_system,
					const GtkFilePath *path)
{
  GList *bookmarks, *elem;
  gchar *uri, *label = NULL;

  DEBUG ("get_bookmark_label");

  bookmarks = read_bookmarks_file ();
  uri = gtk_file_system_gio_path_to_uri (file_system, path);

  for (elem = bookmarks; elem; elem = elem->next)
    {
      BookmarkEntry *entry = elem->data;

      if (strcmp (uri, entry->uri) == 0)
	{
	  label = g_strdup (entry->label);
	  break;
	}
    }

  free_bookmarks (bookmarks);
  g_free (uri);

  return label;
}

static void
gtk_file_system_gio_set_bookmark_label (GtkFileSystem     *file_system,
					const GtkFilePath *path,
					const gchar       *label)
{
  GList *bookmarks, *elem;
  gboolean changed = FALSE;
  gchar *uri;

  DEBUG ("set_bookmark_label");

  bookmarks = read_bookmarks_file ();
  uri = gtk_file_system_gio_path_to_uri (file_system, path);

  for (elem = bookmarks; elem; elem = elem->next)
    {
      BookmarkEntry *entry = elem->data;

      if (strcmp (uri, entry->uri) == 0)
	{
	  g_free (entry->label);
	  entry->label = g_strdup (label);
	  changed = TRUE;

	  break;
	}
    }

  save_bookmarks_file (bookmarks);
  free_bookmarks (bookmarks);

  if (changed)
    g_signal_emit_by_name (file_system, "bookmarks-changed", 0);

  g_free (uri);
}

/* GtkFileFolder methods */
static void
gtk_file_folder_gio_class_init (GtkFileFolderGioClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = gtk_file_folder_gio_finalize;
}

static void
gtk_file_folder_gio_iface_init (GtkFileFolderIface *iface)
{
  iface->get_info = gtk_file_folder_gio_get_info;
  iface->list_children = gtk_file_folder_gio_list_children;
  iface->is_finished_loading = gtk_file_folder_gio_is_finished_loading;
}

static void
gtk_file_folder_gio_init (GtkFileFolderGio *impl)
{
}

static void
gtk_file_folder_gio_class_finalize (GtkFileFolderGioClass *class)
{
}

static void
gtk_file_folder_gio_finalize (GObject *object)
{
  GtkFileFolderGio *folder = GTK_FILE_FOLDER_GIO (object);

  DEBUG ("folder_finalize");

  g_object_unref (folder->parent_file);

  if (folder->directory_monitor)
    g_object_unref (folder->directory_monitor);

  if (folder->cancellable)
    g_object_unref (folder->cancellable);

  g_hash_table_unref (folder->children);
  G_OBJECT_CLASS (gtk_file_folder_gio_parent_class)->finalize (object);
}

/* GtkFileFolder implementation methods */
static GtkFileInfo *
gtk_file_folder_gio_get_info (GtkFileFolder      *folder,
			      const GtkFilePath  *path,
			      GError            **error)
{
  GtkFileFolderGio *folder_gio;
  GFileInfo *file_info;
  const char *uri;

  DEBUG ("folder_get_info");

  folder_gio = GTK_FILE_FOLDER_GIO (folder);
  uri = gtk_file_path_get_string (path);
  file_info = g_hash_table_lookup (folder_gio->children,
				   uri);
  if (file_info)
    {
      GtkFileInfo *info;
      GFile *file;

      file = g_file_new_for_uri (uri);
      info = translate_file_info (file, file_info);
      g_object_unref (file);

      return info;
    }

  return NULL;
}

static gboolean
gtk_file_folder_gio_list_children (GtkFileFolder  *folder,
				   GSList        **children,
				   GError        **error)
{
  GtkFileFolderGio *folder_gio;
  GList *list, *elem;

  DEBUG ("list_children");

  folder_gio = GTK_FILE_FOLDER_GIO (folder);
  list = g_hash_table_get_keys (folder_gio->children);

  for (elem = list; elem; elem = elem->next)
    *children = g_slist_prepend (*children, gtk_file_path_new_dup (elem->data));

  return TRUE;
}

static gboolean
gtk_file_folder_gio_is_finished_loading (GtkFileFolder *folder)
{
  GtkFileFolderGio *folder_gio;

  DEBUG ("is_finished_loading");
  folder_gio = GTK_FILE_FOLDER_GIO (folder);

  return folder_gio->finished_loading;
}

GtkFileSystem*
gtk_file_system_gio_new (void)
{
  return g_object_new (GTK_TYPE_FILE_SYSTEM_GIO, NULL);
}

/* GtkFileSystem module methods */
void
fs_module_init (GTypeModule *module)
{
  /* these are defined by the G_DEFINE_BLAH macros */
  gtk_file_system_gio_register_type (module);
  gtk_file_system_handle_gio_register_type (module);
  gtk_file_folder_gio_register_type (module);
}

void
fs_module_exit (void)
{
}

GtkFileSystem *
fs_module_create (void)
{
  return gtk_file_system_gio_new ();
}
