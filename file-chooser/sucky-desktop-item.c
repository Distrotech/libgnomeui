/* This is the same as gnome-desktop/libgnome-desktop/gnome-desktop-item.c.
 * We cannot use that motherfucker from libgnomeui/file-chooser because libgnomedesktop depends
 * on libgnomeui, not the other way around.  So I just cut&pasted that file and replaced "gnome"
 * with "sucky".
 */

/* -*- Mode: C; c-set-style: linux indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-desktop-item.c - GNOME Desktop File Representation 

   Copyright (C) 1999, 2000 Red Hat Inc.
   Copyright (C) 2001 Sid Vicious
   All rights reserved.

   This file is part of the Gnome Library.

   Developed by Elliot Lee <sopwith@redhat.com> and Sid Vicious
   
   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
   
   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */
/*
  @NOTATION@
 */

#include "config.h"

#include <limits.h>
#include <ctype.h>
#include <stdio.h>
#include <glib.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include <libgnome/gnome-util.h>
#include <libgnome/gnome-exec.h>
#include <libgnome/gnome-url.h>
#include <locale.h>
#include <popt.h>

#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "sucky-desktop-item.h"

#ifdef HAVE_STARTUP_NOTIFICATION
#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#endif

#define sure_string(s) ((s)!=NULL?(s):"")

extern char **environ;

struct _SuckyDesktopItem {
	int refcount;

	/* all languages used */
	GList *languages;

	SuckyDesktopItemType type;
	
	/* `modified' means that the ditem has been
	 * modified since the last save. */
	gboolean modified;

	/* Keys of the main section only */
	GList *keys;

	GList *sections;

	/* This includes ALL keys, including
	 * other sections, separated by '/' */
	GHashTable *main_hash;

	char *location;

	time_t mtime;

	guint32 launch_time;
};

/* If mtime is set to this, set_location won't update mtime,
 * this is to be used internally only. */
#define DONT_UPDATE_MTIME ((time_t)-2)

typedef struct {
	char *name;
	GList *keys;
} Section;

typedef enum {
	ENCODING_UNKNOWN,
	ENCODING_UTF8,
	ENCODING_LEGACY_MIXED
} Encoding;

/*
 * GnomeVFS reading utils, that look like the libc buffered io stuff
 */

#define READ_BUF_SIZE (32 * 1024)

typedef struct {
	GnomeVFSHandle *handle;
	char *uri;
	char *buf;
	gboolean buf_needs_free;
	gboolean past_first_read;
	gboolean eof;
	gsize size;
	gsize pos;
} ReadBuf;

static SuckyDesktopItem *ditem_load (ReadBuf           *rb,
				     gboolean           no_translations,
				     GError           **error);
static gboolean          ditem_save (SuckyDesktopItem  *item,
				     const char        *uri,
				     GError           **error);

static int
readbuf_getc (ReadBuf *rb)
{
	if (rb->eof)
		return EOF;

	if (rb->size == 0 ||
	    rb->pos == rb->size) {
		GnomeVFSFileSize bytes_read;

		/* FIXME: handle errors other than EOF */
		if (rb->handle == NULL
		    || gnome_vfs_read (rb->handle,
				       rb->buf,
				       READ_BUF_SIZE,
				       &bytes_read) != GNOME_VFS_OK) {
			bytes_read = 0;
		}

		if (bytes_read == 0) {
			rb->eof = TRUE;
			return EOF;
		}

		if (rb->size != 0)
			rb->past_first_read = TRUE;
		rb->size = bytes_read;
		rb->pos = 0;

	}

	return (guchar) rb->buf[rb->pos++];
}

/* Note, does not include the trailing \n */
static char *
readbuf_gets (char *buf, gsize bufsize, ReadBuf *rb)
{
	int c;
	gsize pos;

	g_return_val_if_fail (buf != NULL, NULL);
	g_return_val_if_fail (rb != NULL, NULL);

	pos = 0;
	buf[0] = '\0';

	do {
		c = readbuf_getc (rb);
		if (c == EOF || c == '\n')
			break;
		buf[pos++] = c;
	} while (pos < bufsize-1);

	if (c == EOF && pos == 0)
		return NULL;

	buf[pos++] = '\0';

	return buf;
}

static ReadBuf *
readbuf_open (const char *uri, GError **error)
{
	GnomeVFSResult result;
	GnomeVFSHandle *handle;
	ReadBuf *rb;

	g_return_val_if_fail (uri != NULL, NULL);

	result = gnome_vfs_open (&handle, uri,
				 GNOME_VFS_OPEN_READ);
	if (result != GNOME_VFS_OK) {
		g_set_error (error,
			     /* FIXME: better errors */
			     SUCKY_DESKTOP_ITEM_ERROR,
			     SUCKY_DESKTOP_ITEM_ERROR_CANNOT_OPEN,
			     _("Error reading file '%s': %s"),
			     uri, gnome_vfs_result_to_string (result));
		return NULL;
	}

	rb = g_new0 (ReadBuf, 1);
	rb->handle = handle;
	rb->uri = g_strdup (uri);
	rb->buf = g_malloc (READ_BUF_SIZE);
	rb->buf_needs_free = TRUE;
	/* rb->past_first_read = FALSE; */
	/* rb->eof = FALSE; */
	/* rb->size = 0; */
	/* rb->pos = 0; */

	return rb;
}

static ReadBuf *
readbuf_new_from_string (const char *uri, const char *string, gssize length)
{
	ReadBuf *rb;

	g_return_val_if_fail (string != NULL, NULL);
	g_return_val_if_fail (length >= 0, NULL);

	rb = g_new0 (ReadBuf, 1);
	/* rb->handle = NULL; */
	rb->uri = g_strdup (uri);
	rb->buf = (char *) string;
	/* rb->buf_needs_free = FALSE; */
	/* rb->past_first_read = FALSE; */
	/* rb->eof = FALSE; */
	rb->size = length;
	/* rb->pos = 0; */

	return rb;
}

static gboolean
readbuf_rewind (ReadBuf *rb, GError **error)
{
	GnomeVFSResult result;

	rb->eof = FALSE;
	rb->pos = 0;

	if (!rb->past_first_read)
		return TRUE;

	rb->size = 0;

	if (rb->handle) {
		result = gnome_vfs_seek (
				rb->handle, GNOME_VFS_SEEK_START, 0);
		if (result == GNOME_VFS_OK)
			return TRUE;

		gnome_vfs_close (rb->handle);
		rb->handle = NULL;
	}

	result = gnome_vfs_open (
			&rb->handle, rb->uri, GNOME_VFS_OPEN_READ);
	if (result != GNOME_VFS_OK) {
		g_set_error (
			error, SUCKY_DESKTOP_ITEM_ERROR,
			SUCKY_DESKTOP_ITEM_ERROR_CANNOT_OPEN,
			_("Error rewinding file '%s': %s"),
			rb->uri, gnome_vfs_result_to_string (result));

		return FALSE;
	}

	return TRUE;
}

static void
readbuf_close (ReadBuf *rb)
{
	if (rb->handle != NULL)
		gnome_vfs_close (rb->handle);
	g_free (rb->uri);
	if (rb->buf_needs_free)
		g_free (rb->buf);
	g_free (rb);
}

static SuckyDesktopItemType
type_from_string (const char *type)
{
	if (!type)
		return SUCKY_DESKTOP_ITEM_TYPE_NULL;

	switch (type [0]) {
	case 'A':
		if (!strcmp (type, "Application"))
			return SUCKY_DESKTOP_ITEM_TYPE_APPLICATION;
		break;
	case 'L':
		if (!strcmp (type, "Link"))
			return SUCKY_DESKTOP_ITEM_TYPE_LINK;
		break;
	case 'F':
		if (!strcmp (type, "FSDevice"))
			return SUCKY_DESKTOP_ITEM_TYPE_FSDEVICE;
		break;
	case 'M':
		if (!strcmp (type, "MimeType"))
			return SUCKY_DESKTOP_ITEM_TYPE_MIME_TYPE;
		break;
	case 'D':
		if (!strcmp (type, "Directory"))
			return SUCKY_DESKTOP_ITEM_TYPE_DIRECTORY;
		break;
	case 'S':
		if (!strcmp (type, "Service"))
			return SUCKY_DESKTOP_ITEM_TYPE_SERVICE;

		else if (!strcmp (type, "ServiceType"))
			return SUCKY_DESKTOP_ITEM_TYPE_SERVICE_TYPE;
		break;
	default:
		break;
	}

	return SUCKY_DESKTOP_ITEM_TYPE_OTHER;
}
#if 0
static void
init_i18n (void) {
	static gboolean initialized = FALSE;
	
	if (!initialized) {
		bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
		bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
		initialized = TRUE;
	}
}
#endif
/**
 * sucky_desktop_item_new:
 *
 * Creates a SuckyDesktopItem object. The reference count on the returned value is set to '1'.
 *
 * Returns: The new SuckyDesktopItem
 */
SuckyDesktopItem *
sucky_desktop_item_new (void)
{
	SuckyDesktopItem *retval;
#if 0
	init_i18n ();
#endif
	retval = g_new0 (SuckyDesktopItem, 1);

	retval->refcount++;

	retval->main_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
						   (GDestroyNotify) g_free,
						   (GDestroyNotify) g_free);
	
	/* These are guaranteed to be set */
	sucky_desktop_item_set_string (retval,
				       SUCKY_DESKTOP_ITEM_NAME,
				       _("No name"));
	sucky_desktop_item_set_string (retval,
				       SUCKY_DESKTOP_ITEM_ENCODING,
				       "UTF-8");
	sucky_desktop_item_set_string (retval,
				       SUCKY_DESKTOP_ITEM_VERSION,
				       "1.0");

	retval->launch_time = 0;

	return retval;
}

static Section *
dup_section (Section *sec)
{
	GList *li;
	Section *retval = g_new0 (Section, 1);

	retval->name = g_strdup (sec->name);

	retval->keys = g_list_copy (sec->keys);
	for (li = retval->keys; li != NULL; li = li->next)
		li->data = g_strdup (li->data);

	return retval;
}

static void
copy_string_hash (gpointer key, gpointer value, gpointer user_data)
{
	GHashTable *copy = user_data;
	g_hash_table_replace (copy,
			      g_strdup (key),
			      g_strdup (value));
}


/**
 * sucky_desktop_item_copy:
 * @item: The item to be copied
 *
 * Creates a copy of a SuckyDesktopItem.  The new copy has a refcount of 1.
 * Note: Section stack is NOT copied.
 *
 * Returns: The new copy 
 */
SuckyDesktopItem *
sucky_desktop_item_copy (const SuckyDesktopItem *item)
{
	GList *li;
	SuckyDesktopItem *retval;

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (item->refcount > 0, NULL);

	retval = sucky_desktop_item_new ();

	retval->type = item->type;
	retval->modified = item->modified;
	retval->location = g_strdup (item->location);
	retval->mtime = item->mtime;
	retval->launch_time = item->launch_time;

	/* Languages */
	retval->languages = g_list_copy (item->languages);
	for (li = retval->languages; li != NULL; li = li->next)
		li->data = g_strdup (li->data);	

	/* Keys */
	retval->keys = g_list_copy (item->keys);
	for (li = retval->keys; li != NULL; li = li->next)
		li->data = g_strdup (li->data);

	/* Sections */
	retval->sections = g_list_copy (item->sections);
	for (li = retval->sections; li != NULL; li = li->next)
		li->data = dup_section (li->data);

	retval->main_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
						   (GDestroyNotify) g_free,
						   (GDestroyNotify) g_free);

	g_hash_table_foreach (item->main_hash,
			      copy_string_hash,
			      retval->main_hash);

	return retval;
}

static void
read_sort_order (SuckyDesktopItem *item, const char *dir)
{
	char *file;
	char buf[BUFSIZ];
	GString *str;
	ReadBuf *rb;

	file = g_build_filename (dir, ".order", NULL);
	rb = readbuf_open (file, NULL);
	g_free (file);

	if (rb == NULL)
		return;

	str = NULL;
	while (readbuf_gets (buf, sizeof (buf), rb) != NULL) {
		if (str == NULL)
			str = g_string_new (buf);
		else
			g_string_append (str, buf);
		g_string_append_c (str, ';');
	}
	readbuf_close (rb);
	if (str != NULL) {
		sucky_desktop_item_set_string (item, SUCKY_DESKTOP_ITEM_SORT_ORDER,
					       str->str);
		g_string_free (str, TRUE);
	}
}

static SuckyDesktopItem *
make_fake_directory (const char *dir)
{
	SuckyDesktopItem *item;
	char *file;

	item = sucky_desktop_item_new ();
	sucky_desktop_item_set_entry_type (item,
					   SUCKY_DESKTOP_ITEM_TYPE_DIRECTORY);
	file = g_build_filename (dir, ".directory", NULL);
	item->mtime = DONT_UPDATE_MTIME; /* it doesn't exist, we know that */
	sucky_desktop_item_set_location (item, file);
	item->mtime = 0;
	g_free (file);

	read_sort_order (item, dir);

	return item;
}

/**
 * sucky_desktop_item_new_from_file:
 * @file: The filename or directory path to load the SuckyDesktopItem from
 * @flags: Flags to influence the loading process
 *
 * This function loads 'file' and turns it into a SuckyDesktopItem.
 *
 * Returns: The newly loaded item.
 */
SuckyDesktopItem *
sucky_desktop_item_new_from_file (const char *file,
				  SuckyDesktopItemLoadFlags flags,
				  GError **error)
{
	SuckyDesktopItem *retval;
	char *uri;

	g_return_val_if_fail (file != NULL, NULL);

	if (g_path_is_absolute (file)) {
		uri = gnome_vfs_get_uri_from_local_path (file);
	} else {
		char *cur = g_get_current_dir ();
		char *full = g_build_filename (cur, file, NULL);
		g_free (cur);
		uri = gnome_vfs_get_uri_from_local_path (full);
		g_free (full);
	}
	retval = sucky_desktop_item_new_from_uri (uri, flags, error);

	g_free (uri);

	return retval;
}

static char *
get_dirname (const char *uri)
{
	GnomeVFSURI *vfsuri = gnome_vfs_uri_new (uri);
	char *dirname;

	if (vfsuri == NULL)
		return NULL;
	dirname = gnome_vfs_uri_extract_dirname (vfsuri);
	gnome_vfs_uri_unref (vfsuri);
	return dirname;
}

/**
 * sucky_desktop_item_new_from_uri:
 * @uri: GnomeVFSURI to load the SuckyDesktopItem from
 * @flags: Flags to influence the loading process
 *
 * This function loads 'uri' and turns it into a SuckyDesktopItem.
 *
 * Returns: The newly loaded item.
 */
SuckyDesktopItem *
sucky_desktop_item_new_from_uri (const char *uri,
				 SuckyDesktopItemLoadFlags flags,
				 GError **error)
{
	SuckyDesktopItem *retval;
	char *subfn, *dir;
	GnomeVFSFileInfo *info;
	time_t mtime = 0;
	ReadBuf *rb;
	GnomeVFSResult result;

	g_return_val_if_fail (uri != NULL, NULL);

	info = gnome_vfs_file_info_new ();

	result = gnome_vfs_get_file_info (uri, info,
					  GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	if (result != GNOME_VFS_OK) {
		g_set_error (error,
			     /* FIXME: better errors */
			     SUCKY_DESKTOP_ITEM_ERROR,
			     SUCKY_DESKTOP_ITEM_ERROR_CANNOT_OPEN,
			     _("Error reading file '%s': %s"),
			     uri, gnome_vfs_result_to_string (result));

		gnome_vfs_file_info_unref (info);

		return NULL;
	}
	
	if (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_TYPE &&
	    info->type != GNOME_VFS_FILE_TYPE_REGULAR &&
	    info->type != GNOME_VFS_FILE_TYPE_DIRECTORY) {
		g_set_error (error,
			     /* FIXME: better errors */
			     SUCKY_DESKTOP_ITEM_ERROR,
			     SUCKY_DESKTOP_ITEM_ERROR_INVALID_TYPE,
			     _("File '%s' is not a regular file or directory."),
			     uri);

		gnome_vfs_file_info_unref (info);

		return NULL;
	}	    	
	
	if (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MTIME)
		mtime = info->mtime;
	else
		mtime = 0;

	if (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_TYPE &&
	    info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
		subfn = g_build_filename (uri, ".directory", NULL);
		gnome_vfs_file_info_clear (info);
		if (gnome_vfs_get_file_info (subfn, info,
					     GNOME_VFS_FILE_INFO_FOLLOW_LINKS) != GNOME_VFS_OK) {
			gnome_vfs_file_info_unref (info);
			g_free (subfn);

			if (flags & SUCKY_DESKTOP_ITEM_LOAD_ONLY_IF_EXISTS) {
				return NULL;
			} else {
				return make_fake_directory (uri);
			}
		}

		if (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MTIME)
			mtime = info->mtime;
		else
			mtime = 0;
	} else {
		subfn = g_strdup (uri);
	}

	gnome_vfs_file_info_unref (info);

	rb = readbuf_open (subfn, error);
	
	if (rb == NULL) {
		g_free (subfn);
		return NULL;
	}

	retval = ditem_load (rb,
			     (flags & SUCKY_DESKTOP_ITEM_LOAD_NO_TRANSLATIONS) != 0,
			     error);

	if (retval == NULL) {
		g_free (subfn);
		return NULL;
	}

	if (flags & SUCKY_DESKTOP_ITEM_LOAD_ONLY_IF_EXISTS &&
	    ! sucky_desktop_item_exists (retval)) {
		sucky_desktop_item_unref (retval);
		g_free (subfn);
		return NULL;
	}

	retval->mtime = DONT_UPDATE_MTIME;
	sucky_desktop_item_set_location (retval, subfn);
	retval->mtime = mtime;

	dir = get_dirname (retval->location);
	if (dir != NULL) {
		read_sort_order (retval, dir);
		g_free (dir);
	}

	g_free (subfn);

	return retval;
}

/**
 * sucky_desktop_item_new_from_string:
 * @string: string to load the SuckyDesktopItem from
 * @length: length of string, or -1 to use strlen
 * @flags: Flags to influence the loading process
 * @error: place to put errors
 *
 * This function turns the contents of the string into a SuckyDesktopItem.
 *
 * Returns: The newly loaded item.
 */
SuckyDesktopItem *
sucky_desktop_item_new_from_string (const char *uri,
				    const char *string,
				    gssize length,
				    SuckyDesktopItemLoadFlags flags,
				    GError **error)
{
	SuckyDesktopItem *retval;
	ReadBuf *rb;

	g_return_val_if_fail (string != NULL, NULL);
	g_return_val_if_fail (length >= -1, NULL);

	rb = readbuf_new_from_string (uri, string, length);

	retval = ditem_load (rb,
			     (flags & SUCKY_DESKTOP_ITEM_LOAD_NO_TRANSLATIONS) != 0,
			     error);

	if (retval == NULL) {
		return NULL;
	}
	
	/* FIXME: Sort order? */

	return retval;
}

static char *
lookup_desktop_file_in_data_dir (const char *desktop_file,
                                 const char *data_dir)
{
	char *path;

	path = g_build_filename (data_dir, "applications", desktop_file, NULL);
	if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
		g_free (path);
		return NULL;
	}
	return path;
}

static char *
file_from_basename (const char *basename)
{
	const char * const *system_data_dirs;
	const char         *user_data_dir;
	char               *retval;
	int                 i;

	user_data_dir = g_get_user_data_dir ();
	system_data_dirs = g_get_system_data_dirs ();

	if ((retval = lookup_desktop_file_in_data_dir (basename, user_data_dir))) {
		return retval;
	}
	for (i = 0; system_data_dirs[i]; i++) {
		if ((retval = lookup_desktop_file_in_data_dir (basename, system_data_dirs[i]))) {
			return retval;
		}
	}
	return NULL;
}

/**
 * sucky_desktop_item_new_from_basename:
 * @basename: The basename of the SuckyDesktopItem to load.
 * @flags: Flags to influence the loading process
 *
 * This function loads 'basename' from a system data directory and 
 * returns its SuckyDesktopItem. 
 *
 * Returns: The newly loaded item.
 */
SuckyDesktopItem *
sucky_desktop_item_new_from_basename (const char *basename,
                                      SuckyDesktopItemLoadFlags flags,
                                      GError **error)
{
	SuckyDesktopItem *retval;
	char *file;

	g_return_val_if_fail (basename != NULL, NULL);

	if (!(file = file_from_basename (basename))) {
		g_set_error (error,
			     SUCKY_DESKTOP_ITEM_ERROR,
			     SUCKY_DESKTOP_ITEM_ERROR_CANNOT_OPEN,
			     _("Error cannot find file id '%s'"),
			     basename);
		return NULL;
	}
				   
	retval = sucky_desktop_item_new_from_file (file, flags, error);
	g_free (file);
	
	return retval;
}

/**
 * sucky_desktop_item_save:
 * @item: A desktop item
 * @under: A new uri (location) for this #SuckyDesktopItem
 * @force: Save even if it wasn't modified
 * @error: #GError return
 *
 * Writes the specified item to disk.  If the 'under' is NULL, the original
 * location is used.  It sets the location of this entry to point to the
 * new location.
 *
 * Returns: boolean. %TRUE if the file was saved, %FALSE otherwise
 */
gboolean
sucky_desktop_item_save (SuckyDesktopItem *item,
			 const char *under,
			 gboolean force,
			 GError **error)
{
	const char *uri;

	if (under == NULL &&
	    ! force &&
	    ! item->modified)
		return TRUE;
	
	if (under == NULL)
		uri = item->location;
	else 
		uri = under;

	if (uri == NULL) {
		g_set_error (error,
			     SUCKY_DESKTOP_ITEM_ERROR,
			     SUCKY_DESKTOP_ITEM_ERROR_NO_FILENAME,
			     _("No filename to save to"));
		return FALSE;
	}

	if ( ! ditem_save (item, uri, error))
		return FALSE;

	item->modified = FALSE;
	item->mtime = time (NULL);

	return TRUE;
}

/**
 * sucky_desktop_item_ref:
 * @item: A desktop item
 *
 * Description: Increases the reference count of the specified item.
 *
 * Returns: the newly referenced @item
 */
SuckyDesktopItem *
sucky_desktop_item_ref (SuckyDesktopItem *item)
{
	g_return_val_if_fail (item != NULL, NULL);

	item->refcount++;

	return item;
}

static void
free_section (gpointer data, gpointer user_data)
{
	Section *section = data;

	g_free (section->name);
	section->name = NULL;

	g_list_foreach (section->keys, (GFunc)g_free, NULL);
	g_list_free (section->keys);
	section->keys = NULL;

	g_free (section);
}

/**
 * sucky_desktop_item_unref:
 * @item: A desktop item
 *
 * Decreases the reference count of the specified item, and destroys the item if there are no more references left.
 */
void
sucky_desktop_item_unref (SuckyDesktopItem *item)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);

	item->refcount--;

	if(item->refcount != 0)
		return;

	g_list_foreach (item->languages, (GFunc)g_free, NULL);
	g_list_free (item->languages);
	item->languages = NULL;

	g_list_foreach (item->keys, (GFunc)g_free, NULL);
	g_list_free (item->keys);
	item->keys = NULL;

	g_list_foreach (item->sections, free_section, NULL);
	g_list_free (item->sections);
	item->sections = NULL;

	g_hash_table_destroy (item->main_hash);
	item->main_hash = NULL;

	g_free (item->location);
	item->location = NULL;

	g_free (item);
}

static Section *
find_section (SuckyDesktopItem *item, const char *section)
{
	GList *li;
	Section *sec;

	if (section == NULL)
		return NULL;
	if (strcmp (section, "Desktop Entry") == 0)
		return NULL;

	for (li = item->sections; li != NULL; li = li->next) {
		sec = li->data;
		if (strcmp (sec->name, section) == 0)
			return sec;
	}

	sec = g_new0 (Section, 1);
	sec->name = g_strdup (section);
	sec->keys = NULL;

	item->sections = g_list_append (item->sections, sec);

	/* Don't mark the item modified, this is just an empty section,
	 * it won't be saved even */

	return sec;
}

static Section *
section_from_key (SuckyDesktopItem *item, const char *key)
{
	char *p;
	char *name;
	Section *sec;

	if (key == NULL)
		return NULL;

	p = strchr (key, '/');
	if (p == NULL)
		return NULL;

	name = g_strndup (key, p - key);

	sec = find_section (item, name);

	g_free (name);

	return sec;
}

static const char *
key_basename (const char *key)
{
	char *p = strrchr (key, '/');
	if (p != NULL)
		return p+1;
	else
		return key;
}


static const char *
lookup (const SuckyDesktopItem *item, const char *key)
{
	return g_hash_table_lookup (item->main_hash, key);
}

static const char *
lookup_locale (const SuckyDesktopItem *item, const char *key, const char *locale)
{
	if (locale == NULL ||
	    strcmp (locale, "C") == 0) {
		return lookup (item, key);
	} else {
		const char *ret;
		char *full = g_strdup_printf ("%s[%s]", key, locale);
		ret = lookup (item, full);
		g_free (full);
		return ret;
	}
}

static const char *
lookup_best_locale (const SuckyDesktopItem *item, const char *key)
{
	const char * const *langs_pointer;
	int                 i;

	langs_pointer = g_get_language_names ();
	for (i = 0; langs_pointer[i] != NULL; i++) {
		const char *ret = NULL;

		ret = lookup_locale (item, key, langs_pointer[i]);
		if (ret != NULL)
			return ret;
	}

	return NULL;
}

static void
set (SuckyDesktopItem *item, const char *key, const char *value)
{
	Section *sec = section_from_key (item, key);

	if (sec != NULL) {
		if (value != NULL) {
			if (g_hash_table_lookup (item->main_hash, key) == NULL)
				sec->keys = g_list_append
					(sec->keys,
					 g_strdup (key_basename (key)));

			g_hash_table_replace (item->main_hash,
					      g_strdup (key),
					      g_strdup (value));
		} else {
			GList *list = g_list_find_custom
				(sec->keys, key_basename (key),
				 (GCompareFunc)strcmp);
			if (list != NULL) {
				g_free (list->data);
				sec->keys =
					g_list_delete_link (sec->keys, list);
			}
			g_hash_table_remove (item->main_hash, key);
		}
	} else {
		if (value != NULL) {
			if (g_hash_table_lookup (item->main_hash, key) == NULL)
				item->keys = g_list_append (item->keys,
							    g_strdup (key));

			g_hash_table_replace (item->main_hash, 
					      g_strdup (key),
					      g_strdup (value));
		} else {
			GList *list = g_list_find_custom
				(item->keys, key, (GCompareFunc)strcmp);
			if (list != NULL) {
				g_free (list->data);
				item->keys =
					g_list_delete_link (item->keys, list);
			}
			g_hash_table_remove (item->main_hash, key);
		}
	}
	item->modified = TRUE;
}

static void
set_locale (SuckyDesktopItem *item, const char *key,
	    const char *locale, const char *value)
{
	if (locale == NULL ||
	    strcmp (locale, "C") == 0) {
		set (item, key, value);
	} else {
		char *full = g_strdup_printf ("%s[%s]", key, locale);
		set (item, full, value);
		g_free (full);
	}
}

static char **
list_to_vector (GSList *list)
{
	int len = g_slist_length (list);
	char **argv;
	int i;
	GSList *li;

	argv = g_new0 (char *, len+1);

	for (i = 0, li = list;
	     li != NULL;
	     li = li->next, i++) {
		argv[i] = g_strdup (li->data);
	}
	argv[i] = NULL;

	return argv;
}

static GSList *
make_args (GList *files)
{
	GSList *list = NULL;
	GList *li;

	for (li = files; li != NULL; li = li->next) {
		GnomeVFSURI *uri;
		const char *file = li->data;
		if (file == NULL)
			continue;;
		uri = gnome_vfs_uri_new (file);
		if (uri)
			list = g_slist_prepend (list, uri);
	}

	return g_slist_reverse (list);
}

static void
free_args (GSList *list)
{
	GSList *li;

	for (li = list; li != NULL; li = li->next) {
		GnomeVFSURI *uri = li->data;
		li->data = NULL;
		gnome_vfs_uri_unref (uri);
	}
	g_slist_free (list);
}

static char *
escape_single_quotes (const char *s,
		      gboolean in_single_quotes,
		      gboolean in_double_quotes)
{
	const char *p;
	GString *gs;
	const char *pre = "";
	const char *post = "";

	if ( ! in_single_quotes && ! in_double_quotes) {
		pre = "'";
		post = "'";
	} else if ( ! in_single_quotes && in_double_quotes) {
		pre = "\"'";
		post = "'\"";
	}

	if (strchr (s, '\'') == NULL) {
		return g_strconcat (pre, s, post, NULL);
	}

	gs = g_string_new (pre);

	for (p = s; *p != '\0'; p++) {
		if (*p == '\'')
			g_string_append (gs, "'\\''");
		else
			g_string_append_c (gs, *p);
	}

	g_string_append (gs, post);

	return g_string_free (gs, FALSE);
}

typedef enum {
	URI_TO_STRING,
	URI_TO_LOCAL_PATH,
	URI_TO_LOCAL_DIRNAME,
	URI_TO_LOCAL_BASENAME
} ConversionType;

static char *
convert_uri (GnomeVFSURI    *uri,
	     ConversionType  conversion)
{
	char *uri_str;
	char *local_path;
	char *retval = NULL;

	uri_str = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

	if (conversion == URI_TO_STRING)
		return uri_str;

	local_path = gnome_vfs_get_local_path_from_uri (uri_str);
	g_free (uri_str);

	if (!local_path)
		return NULL;

	switch (conversion) {
	case URI_TO_LOCAL_PATH:
		retval = local_path;
		break;
	case URI_TO_LOCAL_DIRNAME:
		retval = g_path_get_dirname (local_path);
		g_free (local_path);
		break;
	case URI_TO_LOCAL_BASENAME:
		retval = g_path_get_basename (local_path);
		g_free (local_path);
		break;
	default:
		g_assert_not_reached ();
	}

	return retval;
}

typedef enum {
	ADDED_NONE = 0,
	ADDED_SINGLE,
	ADDED_ALL
} AddedStatus;

static AddedStatus
append_all_converted (GString        *str,
		      ConversionType  conversion,
		      GSList         *args,
		      gboolean        in_single_quotes,
		      gboolean        in_double_quotes,
		      AddedStatus     added_status)
{
	GSList *l;

	for (l = args; l; l = l->next) {
		char *converted;
		char *escaped;

		if (!(converted = convert_uri (l->data, conversion)))
			continue;

		g_string_append (str, " ");

		escaped = escape_single_quotes (converted,
						in_single_quotes,
						in_double_quotes);
		g_string_append (str, escaped);

		g_free (escaped);
		g_free (converted);
	}

	return ADDED_ALL;
}

static AddedStatus
append_first_converted (GString         *str,
			ConversionType   conversion,
			GSList         **arg_ptr,
			gboolean         in_single_quotes,
			gboolean         in_double_quotes,
			AddedStatus      added_status)
{
	GSList *l;
	char   *converted = NULL;
	char   *escaped;

	for (l = *arg_ptr; l; l = l->next) {
		if ((converted = convert_uri (l->data, conversion)))
			break;

		*arg_ptr = l->next;
	}

	if (!converted)
		return added_status;

	escaped = escape_single_quotes (converted, in_single_quotes, in_double_quotes);
	g_string_append (str, escaped);
	g_free (escaped);
	g_free (converted);

	return added_status != ADDED_ALL ? ADDED_SINGLE : added_status;
}

static gboolean
do_percent_subst (const SuckyDesktopItem  *item,
		  const char              *arg,
		  GString                 *str,
		  gboolean                 in_single_quotes,
		  gboolean                 in_double_quotes,
		  GSList                  *args,
		  GSList                 **arg_ptr,
		  AddedStatus             *added_status)
{
	char *esc;
	const char *cs;

	if (arg[0] != '%' || arg[1] == '\0') {
		return FALSE;
	}

	switch (arg[1]) {
	case '%':
		g_string_append_c (str, '%');
		break;
	case 'U':
		*added_status = append_all_converted (str,
						      URI_TO_STRING,
						      args,
						      in_single_quotes,
						      in_double_quotes,
						      *added_status);
		break;
	case 'F':
		*added_status = append_all_converted (str,
						      URI_TO_LOCAL_PATH,
						      args,
						      in_single_quotes,
						      in_double_quotes,
						      *added_status);
		break;
	case 'N':
		*added_status = append_all_converted (str,
						      URI_TO_LOCAL_BASENAME,
						      args,
						      in_single_quotes,
						      in_double_quotes,
						      *added_status);
		break;
	case 'D':
		*added_status = append_all_converted (str,
						      URI_TO_LOCAL_DIRNAME,
						      args,
						      in_single_quotes,
						      in_double_quotes,
						      *added_status);
		break;
	case 'f':
		*added_status = append_first_converted (str,
							URI_TO_LOCAL_PATH,
							arg_ptr,
							in_single_quotes,
							in_double_quotes,
							*added_status);
		break;
	case 'u':
		*added_status = append_first_converted (str,
							URI_TO_STRING,
							arg_ptr,
							in_single_quotes,
							in_double_quotes,
							*added_status);
		break;
	case 'd':
		*added_status = append_first_converted (str,
							URI_TO_LOCAL_DIRNAME,
							arg_ptr,
							in_single_quotes,
							in_double_quotes,
							*added_status);
		break;
	case 'n':
		*added_status = append_first_converted (str,
							URI_TO_LOCAL_BASENAME,
							arg_ptr,
							in_single_quotes,
							in_double_quotes,
							*added_status);
		break;
	case 'm':
		/* Note: v0.9.4 of the spec says this is deprecated and
		   just replace with icon name but KDE replaces with --icon iconname */
		cs = sucky_desktop_item_get_string (item, SUCKY_DESKTOP_ITEM_MINI_ICON);
		if (cs != NULL) {
			g_string_append (str, "--miniicon=");
			esc = escape_single_quotes (cs, in_single_quotes, in_double_quotes);
			g_string_append (str, esc);
		}
		break;
	case 'i':
		/* Note: v0.9.4 of the spec says just replace with icon name
		   but KDE replaces with --icon iconname */
		cs = sucky_desktop_item_get_string (item, SUCKY_DESKTOP_ITEM_ICON);
		if (cs != NULL) {
			g_string_append (str, "--icon=");
			esc = escape_single_quotes (cs, in_single_quotes, in_double_quotes);
			g_string_append (str, esc);
		}
		break;
	case 'c':
		/* Note: v0.9.4 of the spec says comment, but kde uses Name=, so use
		   Name= since no gnome app is using this anyhow */
		cs = sucky_desktop_item_get_localestring (item, SUCKY_DESKTOP_ITEM_NAME);
		if (cs != NULL) {
			esc = escape_single_quotes (cs, in_single_quotes, in_double_quotes);
			g_string_append (str, esc);
			g_free (esc);
		}
		break;
	case 'k':
		/* Note: v0.9.4 of the spec says name but means filename */
		if (item->location != NULL) {
			esc = escape_single_quotes (item->location, in_single_quotes, in_double_quotes);
			g_string_append (str, esc);
			g_free (esc);
		}
		break;
	case 'v':
		cs = sucky_desktop_item_get_localestring (item, SUCKY_DESKTOP_ITEM_DEV);
		if (cs != NULL) {
			esc = escape_single_quotes (cs, in_single_quotes, in_double_quotes);
			g_string_append (str, esc);
			g_free (esc);
		}
		break;
	default:
		/* Maintain special characters - e.g. "%20" */
		if (g_ascii_isdigit (arg [1])) 
			g_string_append_c (str, '%');
		return FALSE;
		break;
	}

	return TRUE;
}

static char *
expand_string (const SuckyDesktopItem  *item,
	       const char              *s,
	       GSList                  *args,
	       GSList                 **arg_ptr,
	       AddedStatus             *added_status)
{
	const char *p;
	gboolean escape = FALSE;
	gboolean single_quot = FALSE;
	gboolean double_quot = FALSE;
	GString *gs = g_string_new (NULL);

	for (p = s; *p != '\0'; p++) {
		if (escape) {
			escape = FALSE;
			g_string_append_c (gs, *p);
		} else if (*p == '\\') {
			if ( ! single_quot)
				escape = TRUE;
			g_string_append_c (gs, *p);
		} else if (*p == '\'') {
			g_string_append_c (gs, *p);
			if ( ! single_quot && ! double_quot) {
				single_quot = TRUE;
			} else if (single_quot) {
				single_quot = FALSE;
			}
		} else if (*p == '"') {
			g_string_append_c (gs, *p);
			if ( ! single_quot && ! double_quot) {
				double_quot = TRUE;
			} else if (double_quot) {
				double_quot = FALSE;
			}
		} else if (*p == '%') {
			if (do_percent_subst (item, p, gs,
					      single_quot, double_quot,
					      args, arg_ptr,
					      added_status)) {
				p++;
			}
		} else {
			g_string_append_c (gs, *p);
		}
	}
	return g_string_free (gs, FALSE);
}

#ifdef HAVE_STARTUP_NOTIFICATION
static void
sn_error_trap_push (SnDisplay *display,
		    Display   *xdisplay)
{
	gdk_error_trap_push ();
}

static void
sn_error_trap_pop (SnDisplay *display,
		   Display   *xdisplay)
{
	gdk_error_trap_pop ();
}

static char **
make_spawn_environment_for_sn_context (SnLauncherContext *sn_context,
				       char             **envp)
{
	char **retval = NULL;
	int    i, j;
	int    desktop_startup_id_len;

	if (envp == NULL)
		envp = environ;
	
	for (i = 0; envp[i]; i++)
		;

	retval = g_new (char *, i + 2);

	desktop_startup_id_len = strlen ("DESKTOP_STARTUP_ID");
	
	for (i = 0, j = 0; envp[i]; i++) {
		if (strncmp (envp[i], "DESKTOP_STARTUP_ID", desktop_startup_id_len) != 0)
		{
			retval[j] = g_strdup (envp[i]);
			++j;
	        }
	}

	retval[j] = g_strdup_printf ("DESKTOP_STARTUP_ID=%s",
				     sn_launcher_context_get_startup_id (sn_context));
	++j;
	retval[j] = NULL;

	return retval;
}

/* This should be fairly long, as it's confusing to users if a startup
 * ends when it shouldn't (it appears that the startup failed, and
 * they have to relaunch the app). Also the timeout only matters when
 * there are bugs and apps don't end their own startup sequence.
 *
 * This timeout is a "last resort" timeout that ignores whether the
 * startup sequence has shown activity or not.  Metacity and the
 * tasklist have smarter, and correspondingly able-to-be-shorter
 * timeouts. The reason our timeout is dumb is that we don't monitor
 * the sequence (don't use an SnMonitorContext)
 */
#define STARTUP_TIMEOUT_LENGTH (30 /* seconds */ * 1000)

typedef struct
{
	GdkScreen *screen;
	GSList *contexts;
	guint timeout_id;
} StartupTimeoutData;

static void
free_startup_timeout (void *data)
{
	StartupTimeoutData *std = data;

	g_slist_foreach (std->contexts,
			 (GFunc) sn_launcher_context_unref,
			 NULL);
	g_slist_free (std->contexts);

	if (std->timeout_id != 0) {
		g_source_remove (std->timeout_id);
		std->timeout_id = 0;
	}

	g_free (std);
}

static gboolean
startup_timeout (void *data)
{
	StartupTimeoutData *std = data;
	GSList *tmp;
	GTimeVal now;
	int min_timeout;

	min_timeout = STARTUP_TIMEOUT_LENGTH;
	
	g_get_current_time (&now);
	
	tmp = std->contexts;
	while (tmp != NULL) {
		SnLauncherContext *sn_context = tmp->data;
		GSList *next = tmp->next;
		long tv_sec, tv_usec;
		double elapsed;
		
		sn_launcher_context_get_last_active_time (sn_context,
							  &tv_sec, &tv_usec);

		elapsed =
			((((double)now.tv_sec - tv_sec) * G_USEC_PER_SEC +
			  (now.tv_usec - tv_usec))) / 1000.0;

		if (elapsed >= STARTUP_TIMEOUT_LENGTH) {
			std->contexts = g_slist_remove (std->contexts,
							sn_context);
			sn_launcher_context_complete (sn_context);
			sn_launcher_context_unref (sn_context);
		} else {
			min_timeout = MIN (min_timeout, (STARTUP_TIMEOUT_LENGTH - elapsed));
		}
		
		tmp = next;
	}

	if (std->contexts == NULL) {
		std->timeout_id = 0;
	} else {
		std->timeout_id = g_timeout_add (min_timeout,
						 startup_timeout,
						 std);
	}

	/* always remove this one, but we may have reinstalled another one. */
	return FALSE;
}

static void
add_startup_timeout (GdkScreen         *screen,
		     SnLauncherContext *sn_context)
{
	StartupTimeoutData *data;

	data = g_object_get_data (G_OBJECT (screen), "gnome-startup-data");
	if (data == NULL) {
		data = g_new (StartupTimeoutData, 1);
		data->screen = screen;
		data->contexts = NULL;
		data->timeout_id = 0;
		
		g_object_set_data_full (G_OBJECT (screen), "gnome-startup-data",
					data, free_startup_timeout);		
	}

	sn_launcher_context_ref (sn_context);
	data->contexts = g_slist_prepend (data->contexts, sn_context);
	
	if (data->timeout_id == 0) {
		data->timeout_id = g_timeout_add (STARTUP_TIMEOUT_LENGTH,
						  startup_timeout,
						  data);		
	}
}
#endif /* HAVE_STARTUP_NOTIFICATION */

static inline char *
stringify_uris (GSList *args)
{
	GString *str;

	str = g_string_new (NULL);

	append_all_converted (str, URI_TO_STRING, args, FALSE, FALSE, ADDED_NONE);

	return g_string_free (str, FALSE);
}

static inline char *
stringify_files (GSList *args)
{
	GString *str;

	str = g_string_new (NULL);

	append_all_converted (str, URI_TO_LOCAL_PATH, args, FALSE, FALSE, ADDED_NONE);

	return g_string_free (str, FALSE);
}

static char **
make_environment_for_screen (GdkScreen  *screen,
			     char      **envp)
{
	char **retval = NULL;
	char  *display_name;
	int    display_index = -1;
	int    i, env_len;

	g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

	if (envp == NULL)
		envp = environ;

	for (env_len = 0; envp [env_len]; env_len++)
		if (strncmp (envp [env_len], "DISPLAY", strlen ("DISPLAY")) == 0)
			display_index = env_len;

	retval = g_new (char *, env_len + 1);
	retval [env_len] = NULL;

	display_name = gdk_screen_make_display_name (screen);

	for (i = 0; i < env_len; i++)
		if (i == display_index)
			retval [i] = g_strconcat ("DISPLAY=", display_name, NULL);
		else
			retval [i] = g_strdup (envp[i]);

	g_assert (i == env_len);

	g_free (display_name);

	return retval;
}

static int
ditem_execute (const SuckyDesktopItem *item,
	       const char *exec,
	       GList *file_list,
	       GdkScreen *screen,
	       int workspace,
               char **envp,
	       gboolean launch_only_one,
	       gboolean use_current_dir,
	       gboolean append_uris,
	       gboolean append_paths,
	       GError **error)
{
	char **free_me = NULL;
	char **real_argv;
	int i, ret;
	char **term_argv = NULL;
	int term_argc = 0;
	GSList *vector_list;
	GSList *args, *arg_ptr;
	AddedStatus added_status;
	const char *working_dir = NULL;
	char **temp_argv = NULL;
	int temp_argc = 0;
	char *new_exec, *uris, *temp;
	char *exec_locale;
	int launched = 0;
#ifdef HAVE_STARTUP_NOTIFICATION
	SnLauncherContext *sn_context;
	SnDisplay *sn_display;
	const char *startup_class;
#endif
	
	g_return_val_if_fail (item, -1);

	if (!use_current_dir)
		working_dir = g_get_home_dir ();

	if (sucky_desktop_item_get_boolean (item, SUCKY_DESKTOP_ITEM_TERMINAL)) {
		const char *options =
			sucky_desktop_item_get_string (item, SUCKY_DESKTOP_ITEM_TERMINAL_OPTIONS);

		if (options != NULL) {
			g_shell_parse_argv (options,
					    &term_argc,
					    &term_argv,
					    NULL /* error */);
			/* ignore errors */
		}

		gnome_prepend_terminal_to_vector (&term_argc, &term_argv);
	}

	args = make_args (file_list);
	arg_ptr = make_args (file_list);

#ifdef HAVE_STARTUP_NOTIFICATION
	sn_display = sn_display_new (gdk_display,
				     sn_error_trap_push,
				     sn_error_trap_pop);

	
	/* Only initiate notification if desktop file supports it.
	 * (we could avoid setting up the SnLauncherContext if we aren't going
	 * to initiate, but why bother)
	 */

	startup_class = sucky_desktop_item_get_string (item,
						       "StartupWMClass");
	if (startup_class ||
	    sucky_desktop_item_get_boolean (item, "StartupNotify")) {
		const char *name;
		const char *icon;

		sn_context = sn_launcher_context_new (sn_display,
						      screen ? gdk_screen_get_number (screen) :
						      DefaultScreen (gdk_display));
		
		name = sucky_desktop_item_get_localestring (item,
							    SUCKY_DESKTOP_ITEM_NAME);

		if (name == NULL)
			name = sucky_desktop_item_get_localestring (item,
								    SUCKY_DESKTOP_ITEM_GENERIC_NAME);
		
		if (name != NULL) {
			char *description;
			
			sn_launcher_context_set_name (sn_context, name);
			
			description = g_strdup_printf (_("Starting %s"), name);
			
			sn_launcher_context_set_description (sn_context, description);
			
			g_free (description);
		}
		
		icon = sucky_desktop_item_get_string (item,
						      SUCKY_DESKTOP_ITEM_ICON);
		
		if (icon != NULL)
			sn_launcher_context_set_icon_name (sn_context, icon);
		
		sn_launcher_context_set_workspace (sn_context, workspace);

		if (startup_class != NULL)
			sn_launcher_context_set_wmclass (sn_context,
							 startup_class);
	} else {
		sn_context = NULL;
	}
#endif

	if (screen) {
		envp = make_environment_for_screen (screen, envp);
		if (free_me)
			g_strfreev (free_me);
		free_me = envp;
	}
	
	exec_locale = g_filename_from_utf8 (exec, -1, NULL, NULL, NULL);
	
	if (exec_locale == NULL) {
		exec_locale = g_strdup ("");
	}	
	
	do {
		added_status = ADDED_NONE;
		new_exec = expand_string (item,
					  exec_locale,
					  args, &arg_ptr, &added_status);

		if (launched == 0 && added_status == ADDED_NONE && append_uris) {
			uris = stringify_uris (args);
			temp = g_strconcat (new_exec, " ", uris, NULL);
			g_free (uris);
			g_free (new_exec);
			new_exec = temp;
			added_status = ADDED_ALL;
		}

		/* append_uris and append_paths are mutually exlusive */
		if (launched == 0 && added_status == ADDED_NONE && append_paths) {
			uris = stringify_files (args);
			temp = g_strconcat (new_exec, " ", uris, NULL);
			g_free (uris);
			g_free (new_exec);
			new_exec = temp;
			added_status = ADDED_ALL;
		}

		if (launched > 0 && added_status == ADDED_NONE) {
			g_free (new_exec);
			break;
		}

		if ( ! g_shell_parse_argv (new_exec,
					   &temp_argc, &temp_argv, error)) {
			/* The error now comes from g_shell_parse_argv */
			g_free (new_exec);
			ret = -1;
			break;
		}
		g_free (new_exec);

		vector_list = NULL;
		for(i = 0; i < term_argc; i++)
			vector_list = g_slist_append (vector_list,
						      g_strdup (term_argv[i]));

		for(i = 0; i < temp_argc; i++)
			vector_list = g_slist_append (vector_list,
						      g_strdup (temp_argv[i]));

		g_strfreev (temp_argv);

		real_argv = list_to_vector (vector_list);
		g_slist_foreach (vector_list, (GFunc)g_free, NULL);
		g_slist_free (vector_list);

#ifdef HAVE_STARTUP_NOTIFICATION
		if (sn_context != NULL &&
		    !sn_launcher_context_get_initiated (sn_context)) {
			guint32 launch_time;

			/* This means that we always use the first real_argv[0]
			 * we select for the "binary name", but it's probably
			 * OK to do that. Binary name isn't super-important
			 * anyway, and we can't initiate twice, and we
			 * must initiate prior to fork/exec.
			 */
			
			sn_launcher_context_set_binary_name (sn_context,
							     real_argv[0]);
			
			if (item->launch_time > 0)
				launch_time = item->launch_time;
			else
				launch_time = gtk_get_current_event_time ();

			sn_launcher_context_initiate (sn_context,
						      g_get_prgname () ? g_get_prgname () : "unknown",
						      real_argv[0],
						      launch_time);

			/* Don't allow accidental reuse of same timestamp */
			((SuckyDesktopItem *)item)->launch_time = 0;

			envp = make_spawn_environment_for_sn_context (sn_context, envp);
			if (free_me)
				g_strfreev (free_me);
			free_me = envp;
		}
#endif
		
		
		if ( ! g_spawn_async (working_dir,
				      real_argv,
				      envp,
				      G_SPAWN_SEARCH_PATH /* flags */,
				      NULL, /* child_setup_func */
				      NULL, /* child_setup_func_data */
				      &ret /* child_pid */,
				      error)) {
			/* The error was set for us,
			 * we just can't launch this thingie */
			ret = -1;
			g_strfreev (real_argv);
			break;
		}
		launched ++;

		g_strfreev (real_argv);

		if (arg_ptr != NULL)
			arg_ptr = arg_ptr->next;

	/* rinse, repeat until we run out of arguments (That
	 * is if we were adding singles anyway) */
	} while (added_status == ADDED_SINGLE &&
		 arg_ptr != NULL &&
		 ! launch_only_one);

	g_free (exec_locale);
#ifdef HAVE_STARTUP_NOTIFICATION
	if (sn_context != NULL) {
		if (ret < 0)
			sn_launcher_context_complete (sn_context); /* end sequence */
		else
			add_startup_timeout (screen ? screen :
					     gdk_display_get_default_screen (gdk_display_get_default ()),
					     sn_context);
		sn_launcher_context_unref (sn_context);
	}
	
	sn_display_unref (sn_display);
#endif /* HAVE_STARTUP_NOTIFICATION */
	
	free_args (args);
	
	if (term_argv)
		g_strfreev (term_argv);

	if (free_me)
		g_strfreev (free_me);

	return ret;
}

/* strip any trailing &, return FALSE if bad things happen and
   we end up with an empty string */
static gboolean
strip_the_amp (char *exec)
{
	size_t exec_len;

	g_strstrip (exec);
	if (*exec == '\0')
		return FALSE;

	exec_len = strlen (exec);
	/* kill any trailing '&' */
	if (exec[exec_len-1] == '&') {
		exec[exec_len-1] = '\0';
		g_strchomp (exec);
	}

	/* can't exactly launch an empty thing */
	if (*exec == '\0')
		return FALSE;

	return TRUE;
}


static int
sucky_desktop_item_launch_on_screen_with_env (
		const SuckyDesktopItem       *item,
		GList                        *file_list,
		SuckyDesktopItemLaunchFlags   flags,
		GdkScreen                    *screen,
		int                           workspace,
		char                        **envp,
		GError                      **error)
{
	const char *exec;
	char *the_exec;
	int ret;

	exec = sucky_desktop_item_get_string (item, SUCKY_DESKTOP_ITEM_EXEC);
	/* This is a URL, so launch it as a url */
	if (item->type == SUCKY_DESKTOP_ITEM_TYPE_LINK) {
		const char *url;
		url = sucky_desktop_item_get_string (item, SUCKY_DESKTOP_ITEM_URL);
		if (url && url[0] != '\0') {
			if (gnome_url_show (url, error))
				return 0;
			else
				return -1;
		/* Gnome panel used to put this in Exec */
		} else if (exec && exec[0] != '\0') {
			if (gnome_url_show (exec, error))
				return 0;
			else
				return -1;
		} else {
			g_set_error (error,
				     SUCKY_DESKTOP_ITEM_ERROR,
				     SUCKY_DESKTOP_ITEM_ERROR_NO_URL,
				     _("No URL to launch"));
			return -1;
		}
	}

	/* check the type, if there is one set */
	if (item->type != SUCKY_DESKTOP_ITEM_TYPE_APPLICATION) {
		g_set_error (error,
			     SUCKY_DESKTOP_ITEM_ERROR,
			     SUCKY_DESKTOP_ITEM_ERROR_NOT_LAUNCHABLE,
			     _("Not a launchable item"));
		return -1;
	}


	if (exec == NULL ||
	    exec[0] == '\0') {
		g_set_error (error,
			     SUCKY_DESKTOP_ITEM_ERROR,
			     SUCKY_DESKTOP_ITEM_ERROR_NO_EXEC_STRING,
			     _("No command (Exec) to launch"));
		return -1;
	}


	/* make a new copy and get rid of spaces */
	the_exec = g_alloca (strlen (exec) + 1);
	strcpy (the_exec, exec);

	if ( ! strip_the_amp (the_exec)) {
		g_set_error (error,
			     SUCKY_DESKTOP_ITEM_ERROR,
			     SUCKY_DESKTOP_ITEM_ERROR_BAD_EXEC_STRING,
			     _("Bad command (Exec) to launch"));
		return -1;
	}

	ret = ditem_execute (item, the_exec, file_list, screen, workspace, envp,
			     (flags & SUCKY_DESKTOP_ITEM_LAUNCH_ONLY_ONE),
			     (flags & SUCKY_DESKTOP_ITEM_LAUNCH_USE_CURRENT_DIR),
			     (flags & SUCKY_DESKTOP_ITEM_LAUNCH_APPEND_URIS),
			     (flags & SUCKY_DESKTOP_ITEM_LAUNCH_APPEND_PATHS),
			     error);

	return ret;
}

/**
 * sucky_desktop_item_launch:
 * @item: A desktop item
 * @file_list:  Files/URIs to launch this item with, can be %NULL
 * @flags: FIXME
 * @error: FIXME
 *
 * This function runs the program listed in the specified 'item',
 * optionally appending additional arguments to its command line.  It uses
 * #g_shell_parse_argv to parse the the exec string into a vector which is
 * then passed to #g_spawn_async for execution. This can return all
 * the errors from GnomeURL, #g_shell_parse_argv and #g_spawn_async,
 * in addition to it's own.  The files are
 * only added if the entry defines one of the standard % strings in it's
 * Exec field.
 *
 * Returns: The the pid of the process spawned.  If more then one
 * process was spawned the last pid is returned.  On error -1
 * is returned and @error is set.
 */
int
sucky_desktop_item_launch (const SuckyDesktopItem       *item,
			   GList                        *file_list,
			   SuckyDesktopItemLaunchFlags   flags,
			   GError                      **error)
{
	return sucky_desktop_item_launch_on_screen_with_env (
			item, file_list, flags, NULL, -1, NULL, error);
}

/**
 * sucky_desktop_item_launch_with_env:
 * @item: A desktop item
 * @file_list:  Files/URIs to launch this item with, can be %NULL
 * @flags: FIXME
 * @envp: child's environment, or %NULL to inherit parent's
 * @error: FIXME
 *
 * See sucky_desktop_item_launch for a full description. This function
 * additionally passes an environment vector for the child process
 * which is to be launched.
 *
 * Returns: The the pid of the process spawned.  If more then one
 * process was spawned the last pid is returned.  On error -1
 * is returned and @error is set.
 */
int
sucky_desktop_item_launch_with_env (const SuckyDesktopItem       *item,
				    GList                        *file_list,
				    SuckyDesktopItemLaunchFlags   flags,
				    char                        **envp,
				    GError                      **error)
{
	return sucky_desktop_item_launch_on_screen_with_env (
			item, file_list, flags,
			NULL, -1, envp, error);
}

/**
 * sucky_desktop_item_launch_on_screen:
 * @item: A desktop item
 * @file_list:  Files/URIs to launch this item with, can be %NULL
 * @flags: FIXME
 * @screen: the %GdkScreen on which the application should be launched
 * @workspace: the workspace on which the app should be launched (-1 for current)
 * @error: FIXME
 *
 * See sucky_desktop_item_launch for a full description. This function
 * additionally attempts to launch the application on a given screen
 * and workspace.
 *
 * Returns: The the pid of the process spawned.  If more then one
 * process was spawned the last pid is returned.  On error -1
 * is returned and @error is set.
 */
int
sucky_desktop_item_launch_on_screen (const SuckyDesktopItem       *item,
				     GList                        *file_list,
				     SuckyDesktopItemLaunchFlags   flags,
				     GdkScreen                    *screen,
				     int                           workspace,
				     GError                      **error)
{
	return sucky_desktop_item_launch_on_screen_with_env (
			item, file_list, flags,
			screen, workspace, NULL, error);
}

/**
 * sucky_desktop_item_drop_uri_list:
 * @item: A desktop item
 * @uri_list: text as gotten from a text/uri-list
 * @flags: FIXME
 * @error: FIXME
 *
 * A list of files or urls dropped onto an icon, the proper (Url or File)
 * exec is run you can pass directly string that you got as the
 * text/uri-list.  This just parses the list and calls 
 *
 * Returns: The value returned by #gnome_execute_async() upon execution of
 * the specified item or -1 on error.  If multiple instances are run, the
 * return of the last one is returned.
 */
int
sucky_desktop_item_drop_uri_list (const SuckyDesktopItem *item,
				  const char *uri_list,
				  SuckyDesktopItemLaunchFlags flags,
				  GError **error)
{
	GList *li;
	int ret;
	GList *list = gnome_vfs_uri_list_parse (uri_list);

	for (li = list; li != NULL; li = li->next) {
		GnomeVFSURI *uri = li->data;
		li->data = gnome_vfs_uri_to_string (uri, 0 /* hide_options */);
		gnome_vfs_uri_unref (uri);
	}

	ret = sucky_desktop_item_launch (item, list, flags, error);

	g_list_foreach (list, (GFunc)g_free, NULL);
	g_list_free (list);

	return ret;
}

/**
* sucky_desktop_item_drop_uri_list_with_env:
* @item: A desktop item
* @uri_list: text as gotten from a text/uri-list
* @flags: FIXME
* @envp: child's environment
* @error: FIXME
*
* See sucky_desktop_item_drop_uri_list for a full description. This function
* additionally passes an environment vector for the child process
* which is to be launched.
*
* Returns: The value returned by #gnome_execute_async() upon execution of
* the specified item or -1 on error.  If multiple instances are run, the
* return of the last one is returned.
*/
int
sucky_desktop_item_drop_uri_list_with_env (const SuckyDesktopItem *item,
					   const char *uri_list,
					   SuckyDesktopItemLaunchFlags flags,
					   char                        **envp,
					   GError **error)
{
	GList *li;
	int ret;
	GList *list = gnome_vfs_uri_list_parse (uri_list);

	for (li = list; li != NULL; li = li->next) {
		GnomeVFSURI *uri = li->data;
		li->data = gnome_vfs_uri_to_string (uri, 0 /* hide_options */);
		gnome_vfs_uri_unref (uri);
	}

	ret =  sucky_desktop_item_launch_with_env (
			item, list, flags, envp, error);

	g_list_foreach (list, (GFunc)g_free, NULL);
	g_list_free (list);

	return ret;
}

static gboolean
exec_exists (const char *exec)
{
	if (g_path_is_absolute (exec)) {
		if (access (exec, X_OK) == 0)
			return TRUE;
		else
			return FALSE;
	} else {
		char *tryme;

		tryme = g_find_program_in_path (exec);
		if (tryme != NULL) {
			g_free (tryme);
			return TRUE;
		}
		return FALSE;
	}
}

/**
 * sucky_desktop_item_exists:
 * @item: A desktop item
 *
 * Attempt to figure out if the program that can be executed by this item
 * actually exists.  First it tries the TryExec attribute to see if that
 * contains a program that is in the path.  Then if there is no such
 * attribute, it tries the first word of the Exec attribute.
 *
 * Returns: A boolean, %TRUE if it exists, %FALSE otherwise.
 */
gboolean
sucky_desktop_item_exists (const SuckyDesktopItem *item)
{
	const char *try_exec;
	const char *exec;

	g_return_val_if_fail (item != NULL, FALSE);

	try_exec = lookup (item, SUCKY_DESKTOP_ITEM_TRY_EXEC);

	if (try_exec != NULL &&
	    ! exec_exists (try_exec)) {
		return FALSE;
	}

	if (item->type == SUCKY_DESKTOP_ITEM_TYPE_APPLICATION) {
		int argc;
		char **argv;
		const char *exe;

		exec = lookup (item, SUCKY_DESKTOP_ITEM_EXEC);
		if (exec == NULL)
			return FALSE;

		if ( ! g_shell_parse_argv (exec, &argc, &argv, NULL))
			return FALSE;

		if (argc < 1) {
			g_strfreev (argv);
			return FALSE;
		}

		exe = argv[0];

		if ( ! exec_exists (exe)) {
			g_strfreev (argv);
			return FALSE;
		}
		g_strfreev (argv);
	}

	return TRUE;
}

/**
 * sucky_desktop_item_get_entry_type:
 * @item: A desktop item
 *
 * Gets the type attribute (the 'Type' field) of the item.  This should
 * usually be 'Application' for an application, but it can be 'Directory'
 * for a directory description.  There are other types available as well.
 * The type usually indicates how the desktop item should be handeled and
 * how the 'Exec' field should be handeled.
 *
 * Returns: The type of the specified 'item'. The returned
 * memory remains owned by the SuckyDesktopItem and should not be freed.
 */
SuckyDesktopItemType
sucky_desktop_item_get_entry_type (const SuckyDesktopItem *item)
{
	g_return_val_if_fail (item != NULL, 0);
	g_return_val_if_fail (item->refcount > 0, 0);

	return item->type;
}

void
sucky_desktop_item_set_entry_type (SuckyDesktopItem *item,
				   SuckyDesktopItemType type)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);

	item->type = type;

	switch (type) {
	case SUCKY_DESKTOP_ITEM_TYPE_NULL:
		set (item, SUCKY_DESKTOP_ITEM_TYPE, NULL);
		break;
	case SUCKY_DESKTOP_ITEM_TYPE_APPLICATION:
		set (item, SUCKY_DESKTOP_ITEM_TYPE, "Application");
		break;
	case SUCKY_DESKTOP_ITEM_TYPE_LINK:
		set (item, SUCKY_DESKTOP_ITEM_TYPE, "Link");
		break;
	case SUCKY_DESKTOP_ITEM_TYPE_FSDEVICE:
		set (item, SUCKY_DESKTOP_ITEM_TYPE, "FSDevice");
		break;
	case SUCKY_DESKTOP_ITEM_TYPE_MIME_TYPE:
		set (item, SUCKY_DESKTOP_ITEM_TYPE, "MimeType");
		break;
	case SUCKY_DESKTOP_ITEM_TYPE_DIRECTORY:
		set (item, SUCKY_DESKTOP_ITEM_TYPE, "Directory");
		break;
	case SUCKY_DESKTOP_ITEM_TYPE_SERVICE:
		set (item, SUCKY_DESKTOP_ITEM_TYPE, "Service");
		break;
	case SUCKY_DESKTOP_ITEM_TYPE_SERVICE_TYPE:
		set (item, SUCKY_DESKTOP_ITEM_TYPE, "ServiceType");
		break;
	default:
		break;
	}
}



/**
 * sucky_desktop_item_get_file_status:
 * @item: A desktop item
 *
 * This function checks the modification time of the on-disk file to
 * see if it is more recent than the in-memory data.
 *
 * Returns: An enum value that specifies whether the item has changed since being loaded.
 */
SuckyDesktopItemStatus
sucky_desktop_item_get_file_status (const SuckyDesktopItem *item)
{
	SuckyDesktopItemStatus retval;
	GnomeVFSFileInfo *info;

	g_return_val_if_fail (item != NULL, SUCKY_DESKTOP_ITEM_DISAPPEARED);
	g_return_val_if_fail (item->refcount > 0, SUCKY_DESKTOP_ITEM_DISAPPEARED);

	info = gnome_vfs_file_info_new ();
	
	if (item->location == NULL ||
	    gnome_vfs_get_file_info (item->location, info,
				     GNOME_VFS_FILE_INFO_FOLLOW_LINKS) != GNOME_VFS_OK) {
		gnome_vfs_file_info_unref (info);
		return SUCKY_DESKTOP_ITEM_DISAPPEARED;
	}

	retval = SUCKY_DESKTOP_ITEM_UNCHANGED;
	if ((info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MTIME) &&
	    info->mtime > item->mtime)
		retval = SUCKY_DESKTOP_ITEM_CHANGED;

	gnome_vfs_file_info_unref (info);
	
	return retval;
}

static char *kde_icondir = NULL;
static GSList *hicolor_kde_48 = NULL;
static GSList *hicolor_kde_32 = NULL;
static GSList *hicolor_kde_22 = NULL;
static GSList *hicolor_kde_16 = NULL;
/* XXX: maybe we don't care about locolor
static GSList *locolor_kde_48 = NULL;
static GSList *locolor_kde_32 = NULL;
static GSList *locolor_kde_22 = NULL;
static GSList *locolor_kde_16 = NULL;
*/

static GSList *
add_dirs (GSList *list, const char *dirname)
{
	DIR *dir;
	struct dirent *dent;

	dir = opendir (dirname);
	if (dir == NULL)
		return list;

	list = g_slist_prepend (list, g_strdup (dirname));

	while ((dent = readdir (dir)) != NULL) {
		char *full;

		/* skip hidden and self/parent references */
		if (dent->d_name[0] == '.')
			continue;

		full = g_build_filename (dirname, dent->d_name, NULL);
		if (g_file_test (full, G_FILE_TEST_IS_DIR)) {
			list = g_slist_prepend (list, full);
			list = add_dirs (list, full);
		} else {
			g_free (full);
		}
	}
	closedir (dir);

	return list;
}

static void
init_kde_dirs (void)
{
	char *dirname;

	if (kde_icondir == NULL)
		return;

#define ADD_DIRS(color,size) \
	dirname = g_build_filename (kde_icondir, #color,	\
				    #size "x" #size , NULL);	\
	color ## _kde_ ## size = add_dirs (NULL, dirname);	\
	g_free (dirname);

	ADD_DIRS (hicolor, 48);
	ADD_DIRS (hicolor, 32);
	ADD_DIRS (hicolor, 22);
	ADD_DIRS (hicolor, 16);

/* XXX: maybe we don't care about locolor
	ADD_DIRS (locolor, 48);
	ADD_DIRS (locolor, 32);
	ADD_DIRS (locolor, 22);
	*/

#undef ADD_DIRS
}

static GSList *
get_kde_dirs (int size)
{
	GSList *list = NULL;

	if (kde_icondir == NULL)
		return NULL;

	if (size > 32) {
		/* 48-inf */
		list = g_slist_concat (g_slist_copy (hicolor_kde_48),
				       g_slist_copy (hicolor_kde_32));
		list = g_slist_concat (list,
				       g_slist_copy (hicolor_kde_22));
		list = g_slist_concat (list,
				       g_slist_copy (hicolor_kde_16));
	} else if (size > 22) {
		/* 23-32 */
		list = g_slist_concat (g_slist_copy (hicolor_kde_32),
				       g_slist_copy (hicolor_kde_48));
		list = g_slist_concat (list,
				       g_slist_copy (hicolor_kde_22));
		list = g_slist_concat (list,
				       g_slist_copy (hicolor_kde_16));
	} else if (size > 16) {
		/* 17-22 */
		list = g_slist_concat (g_slist_copy (hicolor_kde_22),
				       g_slist_copy (hicolor_kde_32));
		list = g_slist_concat (list,
				       g_slist_copy (hicolor_kde_48));
		list = g_slist_concat (list,
				       g_slist_copy (hicolor_kde_16));
	} else {
		/* 1-16 */
		list = g_slist_concat (g_slist_copy (hicolor_kde_16),
				       g_slist_copy (hicolor_kde_22));
		list = g_slist_concat (list,
				       g_slist_copy (hicolor_kde_32));
		list = g_slist_concat (list,
				       g_slist_copy (hicolor_kde_48));
	}

	list = g_slist_append (list, kde_icondir);

	return list;
}

/* similar function is in panel/main.c */
static void
find_kde_directory (void)
{
	int i;
	const char *kdedir;
	char *try_prefixes[] = {
		"/usr",
		"/opt/kde",
		"/opt/kde2",
		"/usr/local",
		"/kde",
		"/kde2",
		NULL
	};

	if (kde_icondir != NULL)
		return;

	kdedir = g_getenv ("KDEDIR");

	if (kdedir != NULL) {
		kde_icondir = g_build_filename (kdedir, "share", "icons", NULL);
		init_kde_dirs ();
		return;
	}

	/* if what configure gave us works use that */
	if (g_file_test (KDE_ICONDIR, G_FILE_TEST_IS_DIR)) {
		kde_icondir = g_strdup (KDE_ICONDIR);
		init_kde_dirs ();
		return;
	}

	for (i = 0; try_prefixes[i] != NULL; i++) {
		char *try;
		try = g_build_filename (try_prefixes[i], "share", "applnk", NULL);
		if (g_file_test (try, G_FILE_TEST_IS_DIR)) {
			g_free (try);
			kde_icondir = g_build_filename (try_prefixes[i], "share", "icons", NULL);
			init_kde_dirs ();
			return;
		}
		g_free (try);
	}

	/* absolute fallback, these don't exist, but we're out of options
	   here */
	kde_icondir = g_strdup (KDE_ICONDIR);

	init_kde_dirs ();
}

/**
 * sucky_desktop_item_find_icon:
 * @icon: icon name, something you'd get out of the Icon key
 * @desired_size: FIXME
 * @flags: FIXME
 *
 * Description:  This function goes and looks for the icon file.  If the icon
 * is not an absolute filename, this will look for it in the standard places.
 * If it can't find the icon, it will return %NULL
 *
 * Returns: A newly allocated string
 */
char *
sucky_desktop_item_find_icon (GnomeIconTheme *icon_theme,
			      const char *icon,
			      int desired_size,
			      int flags)
{
	char *full = NULL;

	if (icon == NULL || strcmp(icon,"") == 0) {
		return NULL;
	} else if (g_path_is_absolute (icon)) {
		if (g_file_test (icon, G_FILE_TEST_EXISTS)) {
			return g_strdup (icon);
		} else {
			return NULL;
		}
	} else {
		char *icon_no_extension;
		char *p;

		if (icon_theme == NULL) {
			icon_theme = gnome_icon_theme_new ();
		} else {
			g_object_ref (icon_theme);
		}

		
		icon_no_extension = g_strdup (icon);
		p = strrchr (icon_no_extension, '.');
		if (p &&
		    (strcmp (p, ".png") == 0 ||
		     strcmp (p, ".xpm") == 0 ||
		     strcmp (p, ".svg") == 0)) {
		    *p = 0;
		}

		full = gnome_icon_theme_lookup_icon (icon_theme,
						     icon_no_extension,
						     desired_size,
						     NULL, NULL);
		
		g_object_unref (icon_theme);
		
		g_free (icon_no_extension);
	}

	if (full == NULL) { /* Fall back on old Gnome/KDE code */
		GSList *kde_dirs = NULL;
		GSList *li;
		const char *exts[] = { ".png", ".xpm", NULL };
		const char *no_exts[] = { "", NULL };
		const char **check_exts;

		full = gnome_program_locate_file (NULL,
						  GNOME_FILE_DOMAIN_PIXMAP,
						  icon,
						  TRUE /* only_if_exists */,
						  NULL /* ret_locations */);
		if (full == NULL)
			full = gnome_program_locate_file (NULL,
							  GNOME_FILE_DOMAIN_APP_PIXMAP,
							  icon,
							  TRUE /* only_if_exists */,
							  NULL /* ret_locations */);

		/* if we found something or no kde, then just return now */
		if (full != NULL ||
		    flags & SUCKY_DESKTOP_ITEM_ICON_NO_KDE)
			return full;

		/* If there is an extention don't add any extensions */
		if (strchr (icon, '.') != NULL) {
			check_exts = no_exts;
		} else {
			check_exts = exts;
		}

		find_kde_directory ();

		kde_dirs = get_kde_dirs (desired_size);

		for (li = kde_dirs; full == NULL && li != NULL; li = li->next) {
			int i;
			for (i = 0; full == NULL && check_exts[i] != NULL; i++) {
				full = g_strconcat (li->data, G_DIR_SEPARATOR_S, icon,
						    check_exts[i], NULL);
				if ( ! g_file_test (full, G_FILE_TEST_EXISTS)) {
					g_free (full);
					full = NULL;
				}
			}
		}

		g_slist_free (kde_dirs);

	}
	    return full;
	    
}

/**
 * sucky_desktop_item_get_icon:
 * @item: A desktop item
 *
 * Description:  This function goes and looks for the icon file.  If the icon
 * is not set as an absolute filename, this will look for it in the standard places.
 * If it can't find the icon, it will return %NULL
 *
 * Returns: A newly allocated string
 */
char *
sucky_desktop_item_get_icon (const SuckyDesktopItem *item,
			     GnomeIconTheme *icon_theme)
{
	/* maybe this function should be deprecated in favour of find icon
	 * -George */
	const char *icon;

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (item->refcount > 0, NULL);

	icon = sucky_desktop_item_get_string (item, SUCKY_DESKTOP_ITEM_ICON);

	return sucky_desktop_item_find_icon (icon_theme, icon,
					     48 /* desired_size */,
					     0 /* flags */);
}

/**
 * sucky_desktop_item_get_location:
 * @item: A desktop item
 *
 * Returns: The file location associated with 'item'.
 *
 */
const char *
sucky_desktop_item_get_location (const SuckyDesktopItem *item)
{
	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (item->refcount > 0, NULL);

	return item->location;
}

/**
 * sucky_desktop_item_set_location:
 * @item: A desktop item
 * @location: A uri string specifying the file location of this particular item.
 *
 * Set's the 'location' uri of this item.
 *
 * Returns:
 */
void
sucky_desktop_item_set_location (SuckyDesktopItem *item, const char *location)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);

	if (item->location != NULL &&
	    location != NULL &&
	    strcmp (item->location, location) == 0)
		return;

	g_free (item->location);
	item->location = g_strdup (location);

	/* This is ugly, but useful internally */
	if (item->mtime != DONT_UPDATE_MTIME) {
		item->mtime = 0;

		if (item->location) {
			GnomeVFSFileInfo *info;
			GnomeVFSResult    res;

			info = gnome_vfs_file_info_new ();

			res = gnome_vfs_get_file_info (item->location, info,
						       GNOME_VFS_FILE_INFO_FOLLOW_LINKS);

			if (res == GNOME_VFS_OK &&
			    info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MTIME)
				item->mtime = info->mtime;

			gnome_vfs_file_info_unref (info);
		}
	}

	/* Make sure that save actually saves */
	item->modified = TRUE;
}

/**
 * sucky_desktop_item_set_location_file:
 * @item: A desktop item
 * @file: A local filename specifying the file location of this particular item.
 *
 * Set's the 'location' uri of this item to the given @file.
 *
 * Returns:
 */
void
sucky_desktop_item_set_location_file (SuckyDesktopItem *item, const char *file)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);

	if (file != NULL) {
		char *uri;
		uri = gnome_vfs_get_uri_from_local_path (file);
		sucky_desktop_item_set_location (item, uri);
		g_free (uri);
	} else {
		sucky_desktop_item_set_location (item, NULL);
	}
}

/*
 * Reading/Writing different sections, NULL is the standard section
 */

gboolean
sucky_desktop_item_attr_exists (const SuckyDesktopItem *item,
				const char *attr)
{
	g_return_val_if_fail (item != NULL, FALSE);
	g_return_val_if_fail (item->refcount > 0, FALSE);
	g_return_val_if_fail (attr != NULL, FALSE);

	return lookup (item, attr) != NULL;
}

/*
 * String type
 */
const char *
sucky_desktop_item_get_string (const SuckyDesktopItem *item,
			       const char *attr)
{
	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (item->refcount > 0, NULL);
	g_return_val_if_fail (attr != NULL, NULL);

	return lookup (item, attr);
}

void
sucky_desktop_item_set_string (SuckyDesktopItem *item,
			       const char *attr,
			       const char *value)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);
	g_return_if_fail (attr != NULL);

	set (item, attr, value);

	if (strcmp (attr, SUCKY_DESKTOP_ITEM_TYPE) == 0)
		item->type = type_from_string (value);
}

/*
 * LocaleString type
 */
const char *
sucky_desktop_item_get_localestring (const SuckyDesktopItem *item,
				     const char *attr)
{
	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (item->refcount > 0, NULL);
	g_return_val_if_fail (attr != NULL, NULL);

	return lookup_best_locale (item, attr);
}

const char *
sucky_desktop_item_get_localestring_lang (const SuckyDesktopItem *item,
					  const char *attr,
					  const char *language)
{
	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (item->refcount > 0, NULL);
	g_return_val_if_fail (attr != NULL, NULL);

	return lookup_locale (item, attr, language);
}

/**
 * sucky_desktop_item_get_string_locale:
 * @item: A desktop item
 * @attr: An attribute name
 *
 * Returns the current locale that is used for the given attribute.
 * This might not be the same for all attributes. For example, if your
 * locale is "en_US.ISO8859-1" but attribute FOO only has "en_US" then
 * that would be returned for attr = "FOO". If attribute BAR has
 * "en_US.ISO8859-1" then that would be returned for "BAR".
 *
 * Returns: a string equal to the current locale or NULL
 * if the attribute is invalid or there is no matching locale.
 */
const char *
sucky_desktop_item_get_attr_locale (const SuckyDesktopItem *item,
				    const char             *attr)
{
	const char * const *langs_pointer;
	int                 i;

	langs_pointer = g_get_language_names ();
	for (i = 0; langs_pointer[i] != NULL; i++) {
		const char *value = NULL;

		value = lookup_locale (item, attr, langs_pointer[i]);
		if (value)
			return langs_pointer[i];
	}

	return NULL;
}

GList *
sucky_desktop_item_get_languages (const SuckyDesktopItem *item,
				  const char *attr)
{
	GList *li;
	GList *list = NULL;

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (item->refcount > 0, NULL);

	for (li = item->languages; li != NULL; li = li->next) {
		char *language = li->data;
		if (attr == NULL ||
		    lookup_locale (item, attr, language) != NULL) {
			list = g_list_prepend (list, language);
		}
	}

	return g_list_reverse (list);
}

static const char *
get_language (void)
{
	const char * const *langs_pointer;
	int                 i;

	langs_pointer = g_get_language_names ();
	for (i = 0; langs_pointer[i] != NULL; i++) {
		/* find first without encoding  */
		if (strchr (langs_pointer[i], '.') == NULL) {
			return langs_pointer[i]; 
		}
	}
	return NULL;
}

void
sucky_desktop_item_set_localestring (SuckyDesktopItem *item,
				     const char *attr,
				     const char *value)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);
	g_return_if_fail (attr != NULL);

	set_locale (item, attr, get_language (), value);
}

void
sucky_desktop_item_set_localestring_lang (SuckyDesktopItem *item,
					  const char *attr,
					  const char *language,
					  const char *value)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);
	g_return_if_fail (attr != NULL);

	set_locale (item, attr, language, value);
}

void
sucky_desktop_item_clear_localestring (SuckyDesktopItem *item,
				       const char *attr)
{
	GList *l;

	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);
	g_return_if_fail (attr != NULL);

	for (l = item->languages; l != NULL; l = l->next)
		set_locale (item, attr, l->data, NULL);

	set (item, attr, NULL);
}

/*
 * Strings, Regexps types
 */

char **
sucky_desktop_item_get_strings (const SuckyDesktopItem *item,
				const char *attr)
{
	const char *value;

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (item->refcount > 0, NULL);
	g_return_val_if_fail (attr != NULL, NULL);

	value = lookup (item, attr);
	if (value == NULL)
		return NULL;

	/* FIXME: there's no way to escape semicolons apparently */
	return g_strsplit (value, ";", -1);
}

void
sucky_desktop_item_set_strings (SuckyDesktopItem *item,
				const char *attr,
				char **strings)
{
	char *str, *str2;

	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);
	g_return_if_fail (attr != NULL);

	str = g_strjoinv (";", strings);
	str2 = g_strconcat (str, ";", NULL);
	/* FIXME: there's no way to escape semicolons apparently */
	set (item, attr, str2);
	g_free (str);
	g_free (str2);
}

/*
 * Boolean type
 */
gboolean
sucky_desktop_item_get_boolean (const SuckyDesktopItem *item,
				const char *attr)
{
	const char *value;

	g_return_val_if_fail (item != NULL, FALSE);
	g_return_val_if_fail (item->refcount > 0, FALSE);
	g_return_val_if_fail (attr != NULL, FALSE);

	value = lookup (item, attr);
	if (value == NULL)
		return FALSE;

	return (value[0] == 'T' ||
		value[0] == 't' ||
		value[0] == 'Y' ||
		value[0] == 'y' ||
		atoi (value) != 0);
}

void
sucky_desktop_item_set_boolean (SuckyDesktopItem *item,
				const char *attr,
				gboolean value)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);
	g_return_if_fail (attr != NULL);

	set (item, attr, value ? "true" : "false");
}

void
sucky_desktop_item_set_launch_time (SuckyDesktopItem *item,
				    guint32           timestamp)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (timestamp > 0);

	item->launch_time = timestamp;
}

/*
 * Clearing attributes
 */
void
sucky_desktop_item_clear_section (SuckyDesktopItem *item,
				  const char *section)
{
	Section *sec;
	GList *li;

	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);

	sec = find_section (item, section);

	if (sec == NULL) {
		for (li = item->keys; li != NULL; li = li->next) {
			g_hash_table_remove (item->main_hash, li->data);
			g_free (li->data);
			li->data = NULL;
		}
		g_list_free (item->keys);
		item->keys = NULL;
	} else {
		for (li = sec->keys; li != NULL; li = li->next) {
			char *key = li->data;
			char *full = g_strdup_printf ("%s/%s",
						      sec->name, key);
			g_hash_table_remove (item->main_hash, full);
			g_free (full);
			g_free (key);
			li->data = NULL;
		}
		g_list_free (sec->keys);
		sec->keys = NULL;
	}
	item->modified = TRUE;
}

/************************************************************
 * Parser:                                                  *
 ************************************************************/

static gboolean G_GNUC_CONST
standard_is_boolean (const char * key)
{
	static GHashTable *bools = NULL;

	if (bools == NULL) {
		bools = g_hash_table_new (g_str_hash, g_str_equal);
		g_hash_table_insert (bools,
				     SUCKY_DESKTOP_ITEM_NO_DISPLAY,
				     SUCKY_DESKTOP_ITEM_NO_DISPLAY);
		g_hash_table_insert (bools,
				     SUCKY_DESKTOP_ITEM_HIDDEN,
				     SUCKY_DESKTOP_ITEM_HIDDEN);
		g_hash_table_insert (bools,
				     SUCKY_DESKTOP_ITEM_TERMINAL,
				     SUCKY_DESKTOP_ITEM_TERMINAL);
		g_hash_table_insert (bools,
				     SUCKY_DESKTOP_ITEM_READ_ONLY,
				     SUCKY_DESKTOP_ITEM_READ_ONLY);
	}

	return g_hash_table_lookup (bools, key) != NULL;
}

static gboolean G_GNUC_CONST
standard_is_strings (const char *key)
{
	static GHashTable *strings = NULL;

	if (strings == NULL) {
		strings = g_hash_table_new (g_str_hash, g_str_equal);
		g_hash_table_insert (strings,
				     SUCKY_DESKTOP_ITEM_FILE_PATTERN,
				     SUCKY_DESKTOP_ITEM_FILE_PATTERN);
		g_hash_table_insert (strings,
				     SUCKY_DESKTOP_ITEM_ACTIONS,
				     SUCKY_DESKTOP_ITEM_ACTIONS);
		g_hash_table_insert (strings,
				     SUCKY_DESKTOP_ITEM_MIME_TYPE,
				     SUCKY_DESKTOP_ITEM_MIME_TYPE);
		g_hash_table_insert (strings,
				     SUCKY_DESKTOP_ITEM_PATTERNS,
				     SUCKY_DESKTOP_ITEM_PATTERNS);
		g_hash_table_insert (strings,
				     SUCKY_DESKTOP_ITEM_SORT_ORDER,
				     SUCKY_DESKTOP_ITEM_SORT_ORDER);
	}

	return g_hash_table_lookup (strings, key) != NULL;
}

/* If no need to cannonize, returns NULL */
static char *
cannonize (const char *key, const char *value)
{
	if (standard_is_boolean (key)) {
		if (value[0] == 'T' ||
		    value[0] == 't' ||
		    value[0] == 'Y' ||
		    value[0] == 'y' ||
		    atoi (value) != 0) {
			return g_strdup ("true");
		} else {
			return g_strdup ("false");
		}
	} else if (standard_is_strings (key)) {
		int len = strlen (value);
		if (len == 0 || value[len-1] != ';') {
			return g_strconcat (value, ";", NULL);
		}
	}
	/* XXX: Perhaps we should canonize numeric values as well, but this
	 * has caused some subtle problems before so it needs to be done
	 * carefully if at all */
	return NULL;
}


static char *
decode_string_and_dup (const char *s)
{
	char *p = g_malloc (strlen (s) + 1);
	char *q = p;

	do {
		if (*s == '\\'){
			switch (*(++s)){
			case 's':
				*p++ = ' ';
				break;
			case 't':
				*p++ = '\t';
				break;
			case 'n':
				*p++ = '\n';
				break;
			case '\\':
				*p++ = '\\';
				break;
			case 'r':
				*p++ = '\r';
				break;
			default:
				*p++ = '\\';
				*p++ = *s;
				break;
			}
		} else {
			*p++ = *s;
		}
	} while (*s++);

	return q;
}

static char *
escape_string_and_dup (const char *s)
{
	char *return_value, *p;
	const char *q;
	int len = 0;

	if (s == NULL)
		return g_strdup("");
	
	q = s;
	while (*q){
		len++;
		if (strchr ("\n\r\t\\", *q) != NULL)
			len++;
		q++;
	}
	return_value = p = (char *) g_malloc (len + 1);
	do {
		switch (*s){
		case '\t':
			*p++ = '\\';
			*p++ = 't';
			break;
		case '\n':
			*p++ = '\\';
			*p++ = 'n';
			break;
		case '\r':
			*p++ = '\\';
			*p++ = 'r';
			break;
		case '\\':
			*p++ = '\\';
			*p++ = '\\';
			break;
		default:
			*p++ = *s;
		}
	} while (*s++);
	return return_value;
}

static gboolean
check_locale (const char *locale)
{
	GIConv cd = g_iconv_open ("UTF-8", locale);
	if ((GIConv)-1 == cd)
		return FALSE;
	g_iconv_close (cd);
	return TRUE;
}

static void
insert_locales (GHashTable *encodings, char *enc, ...)
{
	va_list args;
	char *s;

	va_start (args, enc);
	for (;;) {
		s = va_arg (args, char *);
		if (s == NULL)
			break;
		g_hash_table_insert (encodings, s, enc);
	}
	va_end (args);
}

/* make a standard conversion table from the desktop standard spec */
static GHashTable *
init_encodings (void)
{
	GHashTable *encodings = g_hash_table_new (g_str_hash, g_str_equal);

	/* "C" is plain ascii */
	insert_locales (encodings, "ASCII", "C", NULL);

	insert_locales (encodings, "ARMSCII-8", "by", NULL);
	insert_locales (encodings, "BIG5", "zh_TW", NULL);
	insert_locales (encodings, "CP1251", "be", "bg", NULL);
	if (check_locale ("EUC-CN")) {
		insert_locales (encodings, "EUC-CN", "zh_CN", NULL);
	} else {
		insert_locales (encodings, "GB2312", "zh_CN", NULL);
	}
	insert_locales (encodings, "EUC-JP", "ja", NULL);
	insert_locales (encodings, "EUC-KR", "ko", NULL);
	/*insert_locales (encodings, "GEORGIAN-ACADEMY", NULL);*/
	insert_locales (encodings, "GEORGIAN-PS", "ka", NULL);
	insert_locales (encodings, "ISO-8859-1", "br", "ca", "da", "de", "en", "es", "eu", "fi", "fr", "gl", "it", "nl", "wa", "no", "pt", "pt", "sv", NULL);
	insert_locales (encodings, "ISO-8859-2", "cs", "hr", "hu", "pl", "ro", "sk", "sl", "sq", "sr", NULL);
	insert_locales (encodings, "ISO-8859-3", "eo", NULL);
	insert_locales (encodings, "ISO-8859-5", "mk", "sp", NULL);
	insert_locales (encodings, "ISO-8859-7", "el", NULL);
	insert_locales (encodings, "ISO-8859-9", "tr", NULL);
	insert_locales (encodings, "ISO-8859-13", "lt", "lv", "mi", NULL);
	insert_locales (encodings, "ISO-8859-14", "ga", "cy", NULL);
	insert_locales (encodings, "ISO-8859-15", "et", NULL);
	insert_locales (encodings, "KOI8-R", "ru", NULL);
	insert_locales (encodings, "KOI8-U", "uk", NULL);
	if (check_locale ("TCVN-5712")) {
		insert_locales (encodings, "TCVN-5712", "vi", NULL);
	} else {
		insert_locales (encodings, "TCVN", "vi", NULL);
	}
	insert_locales (encodings, "TIS-620", "th", NULL);
	/*insert_locales (encodings, "VISCII", NULL);*/

	return encodings;
}

static const char *
get_encoding_from_locale (const char *locale)
{
	char lang[3];
	const char *encoding;
	static GHashTable *encodings = NULL;

	if (locale == NULL)
		return NULL;

	/* if locale includes encoding, use it */
	encoding = strchr (locale, '.');
	if (encoding != NULL) {
		return encoding+1;
	}

	if (encodings == NULL)
		encodings = init_encodings ();

	/* first try the entire locale (at this point ll_CC) */
	encoding = g_hash_table_lookup (encodings, locale);
	if (encoding != NULL)
		return encoding;

	/* Try just the language */
	strncpy (lang, locale, 2);
	lang[2] = '\0';
	return g_hash_table_lookup (encodings, lang);
}

static Encoding
get_encoding (ReadBuf *rb)
{
	gboolean old_kde = FALSE;
	char     buf [BUFSIZ];
	gboolean all_valid_utf8 = TRUE;

	while (readbuf_gets (buf, sizeof (buf), rb) != NULL) {
		if (strncmp (SUCKY_DESKTOP_ITEM_ENCODING,
			     buf,
			     strlen (SUCKY_DESKTOP_ITEM_ENCODING)) == 0) {
			char *p = &buf[strlen (SUCKY_DESKTOP_ITEM_ENCODING)];
			if (*p == ' ')
				p++;
			if (*p != '=')
				continue;
			p++;
			if (*p == ' ')
				p++;
			if (strcmp (p, "UTF-8") == 0) {
				return ENCODING_UTF8;
			} else if (strcmp (p, "Legacy-Mixed") == 0) {
				return ENCODING_LEGACY_MIXED;
			} else {
				/* According to the spec we're not supposed
				 * to read a file like this */
				return ENCODING_UNKNOWN;
			}
		} else if (strcmp ("[KDE Desktop Entry]", buf) == 0) {
			old_kde = TRUE;
			/* don't break yet, we still want to support
			 * Encoding even here */
		}
		if (all_valid_utf8 && ! g_utf8_validate (buf, -1, NULL))
			all_valid_utf8 = FALSE;
	}

	if (old_kde)
		return ENCODING_LEGACY_MIXED;

	/* try to guess by location */
	if (rb->uri != NULL && strstr (rb->uri, "gnome/apps/") != NULL) {
		/* old gnome */
		return ENCODING_LEGACY_MIXED;
	}

	/* A dilemma, new KDE files are in UTF-8 but have no Encoding
	 * info, at this time we really can't tell.  The best thing to
	 * do right now is to just assume UTF-8 if the whole file
	 * validates as utf8 I suppose */

	if (all_valid_utf8)
		return ENCODING_UTF8;
	else
		return ENCODING_LEGACY_MIXED;
}

static char *
decode_string (const char *value, Encoding encoding, const char *locale)
{
	char *retval = NULL;

	/* if legacy mixed, then convert */
	if (locale != NULL && encoding == ENCODING_LEGACY_MIXED) {
		const char *char_encoding = get_encoding_from_locale (locale);
		char *utf8_string;
		if (char_encoding == NULL)
			return NULL;
		if (strcmp (char_encoding, "ASCII") == 0) {
			return decode_string_and_dup (value);
		}
		utf8_string = g_convert (value, -1, "UTF-8", char_encoding,
					NULL, NULL, NULL);
		if (utf8_string == NULL)
			return NULL;
		retval = decode_string_and_dup (utf8_string);
		g_free (utf8_string);
		return retval;
	/* if utf8, then validate */
	} else if (locale != NULL && encoding == ENCODING_UTF8) {
		if ( ! g_utf8_validate (value, -1, NULL))
			/* invalid utf8, ignore this key */
			return NULL;
		return decode_string_and_dup (value);
	} else {
		/* Meaning this is not a localized string */
		return decode_string_and_dup (value);
	}
}

static char *
snarf_locale_from_key (const char *key)
{
	const char *brace;
	char *locale, *p;

	brace = strchr (key, '[');
	if (brace == NULL)
		return NULL;

	locale = g_strdup (brace + 1);
	if (*locale == '\0') {
		g_free (locale);
		return NULL;
	}
	p = strchr (locale, ']');
	if (p == NULL) {
		g_free (locale);
		return NULL;
	}
	*p = '\0';
	return locale;
}

static void
insert_key (SuckyDesktopItem *item,
	    Section *cur_section,
	    Encoding encoding,
	    const char *key,
	    const char *value,
	    gboolean old_kde,
	    gboolean no_translations)
{
	char *k;
	char *val;
	/* we always store everything in UTF-8 */
	if (cur_section == NULL &&
	    strcmp (key, SUCKY_DESKTOP_ITEM_ENCODING) == 0) {
		k = g_strdup (key);
		val = g_strdup ("UTF-8");
	} else {
		char *locale = snarf_locale_from_key (key);
		/* If we're ignoring translations */
		if (no_translations && locale != NULL) {
			g_free (locale);
			return;
		}
		val = decode_string (value, encoding, locale);

		/* Ignore this key, it's whacked */
		if (val == NULL) {
			g_free (locale);
			return;
		}
		
		g_strchomp (val);

		/* For old KDE entries, we can also split by a comma
		 * on sort order, so convert to semicolons */
		if (old_kde &&
		    cur_section == NULL &&
		    strcmp (key, SUCKY_DESKTOP_ITEM_SORT_ORDER) == 0 &&
		    strchr (val, ';') == NULL) {
			int i;
			for (i = 0; val[i] != '\0'; i++) {
				if (val[i] == ',')
					val[i] = ';';
			}
		}

		/* Check some types, not perfect, but catches a lot
		 * of things */
		if (cur_section == NULL) {
			char *cannon = cannonize (key, val);
			if (cannon != NULL) {
				g_free (val);
				val = cannon;
			}
		}

		k = g_strdup (key);

		/* Take care of the language part */
		if (locale != NULL &&
		    strcmp (locale, "C") == 0) {
			char *p;
			/* Whack C locale */
			p = strchr (k, '[');
			*p = '\0';
			g_free (locale);
		} else if (locale != NULL) {
			char *p, *brace;

			/* Whack the encoding part */
			p = strchr (locale, '.');
			if (p != NULL)
				*p = '\0';
				
			if (g_list_find_custom (item->languages, locale,
						(GCompareFunc)strcmp) == NULL) {
				item->languages = g_list_prepend
					(item->languages, locale);
			} else {
				g_free (locale);
			}

			/* Whack encoding from encoding in the key */ 
			brace = strchr (k, '[');
			p = strchr (brace, '.');
			if (p != NULL) {
				*p = ']';
				*(p+1) = '\0';
			}
		}
	}


	if (cur_section == NULL) {
		/* only add to list if we haven't seen it before */
		if (g_hash_table_lookup (item->main_hash, k) == NULL) {
			item->keys = g_list_prepend (item->keys,
						     g_strdup (k));
		}
		/* later duplicates override earlier ones */
		g_hash_table_replace (item->main_hash, k, val);
	} else {
		char *full = g_strdup_printf
			("%s/%s",
			 cur_section->name, k);
		/* only add to list if we haven't seen it before */
		if (g_hash_table_lookup (item->main_hash, full) == NULL) {
			cur_section->keys =
				g_list_prepend (cur_section->keys, k);
		}
		/* later duplicates override earlier ones */
		g_hash_table_replace (item->main_hash,
				      full, val);
	}
}

static void
setup_type (SuckyDesktopItem *item, const char *uri)
{
	const char *type = g_hash_table_lookup (item->main_hash,
						SUCKY_DESKTOP_ITEM_TYPE);
	if (type == NULL && uri != NULL) {
		char *base = g_path_get_basename (uri);
		if (base != NULL &&
		    strcmp (base, ".directory") == 0) {
			/* This gotta be a directory */
			g_hash_table_replace (item->main_hash,
					      g_strdup (SUCKY_DESKTOP_ITEM_TYPE), 
					      g_strdup ("Directory"));
			item->keys = g_list_prepend
				(item->keys, g_strdup (SUCKY_DESKTOP_ITEM_TYPE));
			item->type = SUCKY_DESKTOP_ITEM_TYPE_DIRECTORY;
		} else {
			item->type = SUCKY_DESKTOP_ITEM_TYPE_NULL;
		}
		g_free (base);
	} else {
		item->type = type_from_string (type);
	}
}

/* fallback to find something suitable for C locale */
static char *
try_english_key (SuckyDesktopItem *item, const char *key)
{
	char *str;
	char *locales[] = { "en_US", "en_GB", "en_AU", "en", NULL };
	int i;

	str = NULL;
	for (i = 0; locales[i] != NULL && str == NULL; i++) {
		str = g_strdup (lookup_locale (item, key, locales[i]));
	}
	if (str != NULL) {
		/* We need a 7-bit ascii string, so whack all
		 * above 127 chars */
		guchar *p;
		for (p = (guchar *)str; *p != '\0'; p++) {
			if (*p > 127)
				*p = '?';
		}
	}
	return str;
}


static void
sanitize (SuckyDesktopItem *item, const char *uri)
{
	const char *type;

	type = lookup (item, SUCKY_DESKTOP_ITEM_TYPE);

	/* understand old gnome style url exec thingies */
	if (type != NULL && strcmp (type, "URL") == 0) {
		const char *exec = lookup (item, SUCKY_DESKTOP_ITEM_EXEC);
		set (item, SUCKY_DESKTOP_ITEM_TYPE, "Link");
		if (exec != NULL) {
			/* Note, this must be in this order */
			set (item, SUCKY_DESKTOP_ITEM_URL, exec);
			set (item, SUCKY_DESKTOP_ITEM_EXEC, NULL);
		}
	}

	/* we make sure we have Name, Encoding and Version */
	if (lookup (item, SUCKY_DESKTOP_ITEM_NAME) == NULL) {
		char *name = try_english_key (item, SUCKY_DESKTOP_ITEM_NAME);
		/* If no name, use the basename */
		if (name == NULL && uri != NULL)
			name = g_path_get_basename (uri);
		/* If no uri either, use same default as sucky_desktop_item_new */
		if (name == NULL)
			name = g_strdup (_("No name"));
		g_hash_table_replace (item->main_hash,
				      g_strdup (SUCKY_DESKTOP_ITEM_NAME), 
				      name);
		item->keys = g_list_prepend
			(item->keys, g_strdup (SUCKY_DESKTOP_ITEM_NAME));
	}
	if (lookup (item, SUCKY_DESKTOP_ITEM_ENCODING) == NULL) {
		/* We store everything in UTF-8 so write that down */
		g_hash_table_replace (item->main_hash,
				      g_strdup (SUCKY_DESKTOP_ITEM_ENCODING), 
				      g_strdup ("UTF-8"));
		item->keys = g_list_prepend
			(item->keys, g_strdup (SUCKY_DESKTOP_ITEM_ENCODING));
	}
	if (lookup (item, SUCKY_DESKTOP_ITEM_VERSION) == NULL) {
		/* this is the version that we follow, so write it down */
		g_hash_table_replace (item->main_hash,
				      g_strdup (SUCKY_DESKTOP_ITEM_VERSION), 
				      g_strdup ("1.0"));
		item->keys = g_list_prepend
			(item->keys, g_strdup (SUCKY_DESKTOP_ITEM_VERSION));
	}
}

enum {
	FirstBrace,
	OnSecHeader,
	IgnoreToEOL,
	IgnoreToEOLFirst,
	KeyDef,
	KeyDefOnKey,
	KeyValue
};

static SuckyDesktopItem *
ditem_load (ReadBuf *rb,
	    gboolean no_translations,
	    GError **error)
{
	int state;
	char CharBuffer [1024];
	char *next = CharBuffer;
	int c;
	Encoding encoding;
	SuckyDesktopItem *item;
	Section *cur_section = NULL;
	char *key = NULL;
	gboolean old_kde = FALSE;

	encoding = get_encoding (rb);
	if (encoding == ENCODING_UNKNOWN) {
		readbuf_close (rb);
		/* spec says, don't read this file */
		g_set_error (error,
			     SUCKY_DESKTOP_ITEM_ERROR,
			     SUCKY_DESKTOP_ITEM_ERROR_UNKNOWN_ENCODING,
			     _("Unknown encoding of: %s"),
			     rb->uri);
		return NULL;
	}

	/* Rewind since get_encoding goes through the file */
	if (! readbuf_rewind (rb, error)) {
		readbuf_close (rb);
		/* spec says, don't read this file */
		return NULL;
	}

	item = sucky_desktop_item_new ();
	item->modified = FALSE;

	/* Note: location and mtime are filled in by the new_from_file
	 * function since it has those values */

#define OVERFLOW (next == &CharBuffer [sizeof(CharBuffer)-1])

	state = FirstBrace;
	while ((c = readbuf_getc (rb)) != EOF) {
		if (c == '\r')		/* Ignore Carriage Return */
			continue;
		
		switch (state) {

		case OnSecHeader:
			if (c == ']' || OVERFLOW) {
				*next = '\0';
				next = CharBuffer;

				/* keys were inserted in reverse */
				if (cur_section != NULL &&
				    cur_section->keys != NULL) {
					cur_section->keys = g_list_reverse
						(cur_section->keys);
				}
				if (strcmp (CharBuffer,
					    "KDE Desktop Entry") == 0) {
					/* Main section */
					cur_section = NULL;
					old_kde = TRUE;
				} else if (strcmp (CharBuffer,
						   "Desktop Entry") == 0) {
					/* Main section */
					cur_section = NULL;
				} else {
					cur_section = g_new0 (Section, 1);
					cur_section->name =
						g_strdup (CharBuffer);
					cur_section->keys = NULL;
					item->sections = g_list_prepend
						(item->sections, cur_section);
				}
				state = IgnoreToEOL;
			} else if (c == '[') {
				/* FIXME: probably error out instead of ignoring this */
			} else {
				*next++ = c;
			}
			break;

		case IgnoreToEOL:
		case IgnoreToEOLFirst:
			if (c == '\n'){
				if (state == IgnoreToEOLFirst)
					state = FirstBrace;
				else
					state = KeyDef;
				next = CharBuffer;
			}
			break;

		case FirstBrace:
		case KeyDef:
		case KeyDefOnKey:
			if (c == '#') {
				if (state == FirstBrace)
					state = IgnoreToEOLFirst;
				else
					state = IgnoreToEOL;
				break;
			}

			if (c == '[' && state != KeyDefOnKey){
				state = OnSecHeader;
				next = CharBuffer;
				g_free (key);
				key = NULL;
				break;
			}
			/* On first pass, don't allow dangling keys */
			if (state == FirstBrace)
				break;
	    
			if ((c == ' ' && state != KeyDefOnKey) || c == '\t')
				break;
	    
			if (c == '\n' || OVERFLOW) { /* Abort Definition */
				next = CharBuffer;
				state = KeyDef;
				break;
			}
	    
			if (c == '=' || OVERFLOW){
				*next = '\0';

				g_free (key);
				key = g_strdup (CharBuffer);
				state = KeyValue;
				next = CharBuffer;
			} else {
				*next++ = c;
				state = KeyDefOnKey;
			}
			break;

		case KeyValue:
			if (OVERFLOW || c == '\n'){
				*next = '\0';

				insert_key (item, cur_section, encoding,
					    key, CharBuffer, old_kde,
					    no_translations);

				g_free (key);
				key = NULL;

				state = (c == '\n') ? KeyDef : IgnoreToEOL;
				next = CharBuffer;
			} else {
				*next++ = c;
			}
			break;

		} /* switch */
	
	} /* while ((c = getc_unlocked (f)) != EOF) */
	if (c == EOF && state == KeyValue) {
		*next = '\0';

		insert_key (item, cur_section, encoding,
			    key, CharBuffer, old_kde,
			    no_translations);

		g_free (key);
		key = NULL;
	}

#undef OVERFLOW

	/* keys were inserted in reverse */
	if (cur_section != NULL &&
	    cur_section->keys != NULL) {
		cur_section->keys = g_list_reverse (cur_section->keys);
	}
	/* keys were inserted in reverse */
	item->keys = g_list_reverse (item->keys);
	/* sections were inserted in reverse */
	item->sections = g_list_reverse (item->sections);

	/* sanitize some things */
	sanitize (item, rb->uri);

	/* make sure that we set up the type */
	setup_type (item, rb->uri);

	readbuf_close (rb);

	return item;
}

static void vfs_printf (GnomeVFSHandle *handle,
			const char *format, ...) G_GNUC_PRINTF (2, 3);

static void
vfs_printf (GnomeVFSHandle *handle, const char *format, ...)
{
    va_list args;
    gchar *s;
    GnomeVFSFileSize bytes_written;

    va_start (args, format);
    s = g_strdup_vprintf (format, args);
    va_end (args);

    /* FIXME: what about errors */
    gnome_vfs_write (handle, s, strlen (s), &bytes_written);
    g_free (s);
}

static void 
dump_section (SuckyDesktopItem *item, GnomeVFSHandle *handle, Section *section)
{
	GList *li;

	vfs_printf (handle, "[%s]\n", section->name);
	for (li = section->keys; li != NULL; li = li->next) {
		const char *key = li->data;
		char *full = g_strdup_printf ("%s/%s", section->name, key);
		const char *value = g_hash_table_lookup (item->main_hash, full);
		if (value != NULL) {
			char *val = escape_string_and_dup (value);
			vfs_printf (handle, "%s=%s\n", key, val);
			g_free (val);
		}
		g_free (full);
	}
}

static gboolean
ditem_save (SuckyDesktopItem *item, const char *uri, GError **error)
{
	GList *li;
	GnomeVFSHandle *handle;
	GnomeVFSResult result;

	handle = NULL;
	result = gnome_vfs_open (&handle, uri, GNOME_VFS_OPEN_WRITE);
	if (result == GNOME_VFS_ERROR_NOT_FOUND) {
		result = gnome_vfs_create (&handle, uri, GNOME_VFS_OPEN_WRITE, TRUE, GNOME_VFS_PERM_USER_ALL);
	} else if (result == GNOME_VFS_OK) {
		result = gnome_vfs_truncate_handle (handle, 0);
	}

	if (result != GNOME_VFS_OK) {
		g_set_error (error,
			     /* FIXME: better errors */
			     SUCKY_DESKTOP_ITEM_ERROR,
			     SUCKY_DESKTOP_ITEM_ERROR_CANNOT_OPEN,
			     _("Error writing file '%s': %s"),
			     uri, gnome_vfs_result_to_string (result));

		return FALSE;
	}

	vfs_printf (handle, "[Desktop Entry]\n");
	for (li = item->keys; li != NULL; li = li->next) {
		const char *key = li->data;
		const char *value = g_hash_table_lookup (item->main_hash, key);
		if (value != NULL) {
			char *val = escape_string_and_dup (value);
			vfs_printf (handle, "%s=%s\n", key, val);
			g_free (val);
		}
	}

	if (item->sections != NULL)
		vfs_printf (handle, "\n");

	for (li = item->sections; li != NULL; li = li->next) {
		Section *section = li->data;

		/* Don't write empty sections */
		if (section->keys == NULL)
			continue;

		dump_section (item, handle, section);

		if (li->next != NULL)
			vfs_printf (handle, "\n");
	}

	gnome_vfs_close (handle);

	return TRUE;
}

static gpointer
_sucky_desktop_item_copy (gpointer boxed)
{
	return sucky_desktop_item_copy (boxed);
}

static void
_sucky_desktop_item_free (gpointer boxed)
{
	sucky_desktop_item_unref (boxed);
}

GType
sucky_desktop_item_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		type = g_boxed_type_register_static ("SuckyDesktopItem",
						     _sucky_desktop_item_copy,
						     _sucky_desktop_item_free);
	}

	return type;
}

GQuark
sucky_desktop_item_error_quark (void)
{
	static GQuark q = 0;
	if (q == 0)
		q = g_quark_from_static_string ("gnome-desktop-item-error-quark");

	return q;
}
