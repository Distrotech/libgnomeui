/* This is the same as gnome-desktop/libgnome-desktop/libgnome/gnome-desktop-item.h.
 * We cannot use that motherfucker from libgnomeui/file-chooser because libgnomedesktop depends
 * on libgnomeui, not the other way around.  So I just cut&pasted that file and replaced "gnome"
 * with "sucky".
 */

/* -*- Mode: C; c-set-style: linux indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-ditem.h - GNOME Desktop File Representation 

   Copyright (C) 1999, 2000 Red Hat Inc.
   Copyright (C) 2001 Sid Vicious
   All rights reserved.

   This file is part of the Gnome Library.

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

#ifndef SUCKY_DITEM_H
#define SUCKY_DITEM_H

#include <glib.h>
#include <glib-object.h>

#include <gdk/gdk.h>
#include <libgnomeui/gnome-icon-theme.h>

G_BEGIN_DECLS

typedef enum {
	SUCKY_DESKTOP_ITEM_TYPE_NULL = 0 /* This means its NULL, that is, not
					   * set */,
	SUCKY_DESKTOP_ITEM_TYPE_OTHER /* This means it's not one of the below
					      strings types, and you must get the
					      Type attribute. */,

	/* These are the standard compliant types: */
	SUCKY_DESKTOP_ITEM_TYPE_APPLICATION,
	SUCKY_DESKTOP_ITEM_TYPE_LINK,
	SUCKY_DESKTOP_ITEM_TYPE_FSDEVICE,
	SUCKY_DESKTOP_ITEM_TYPE_MIME_TYPE,
	SUCKY_DESKTOP_ITEM_TYPE_DIRECTORY,
	SUCKY_DESKTOP_ITEM_TYPE_SERVICE,
	SUCKY_DESKTOP_ITEM_TYPE_SERVICE_TYPE
} SuckyDesktopItemType;

typedef enum {
        SUCKY_DESKTOP_ITEM_UNCHANGED = 0,
        SUCKY_DESKTOP_ITEM_CHANGED = 1,
        SUCKY_DESKTOP_ITEM_DISAPPEARED = 2
} SuckyDesktopItemStatus;

#define SUCKY_TYPE_DESKTOP_ITEM         (sucky_desktop_item_get_type ())
GType sucky_desktop_item_get_type       (void);

typedef struct _SuckyDesktopItem SuckyDesktopItem;

/* standard */
#define SUCKY_DESKTOP_ITEM_ENCODING	"Encoding" /* string */
#define SUCKY_DESKTOP_ITEM_VERSION	"Version"  /* numeric */
#define SUCKY_DESKTOP_ITEM_NAME		"Name" /* localestring */
#define SUCKY_DESKTOP_ITEM_GENERIC_NAME	"GenericName" /* localestring */
#define SUCKY_DESKTOP_ITEM_TYPE		"Type" /* string */
#define SUCKY_DESKTOP_ITEM_FILE_PATTERN "FilePattern" /* regexp(s) */
#define SUCKY_DESKTOP_ITEM_TRY_EXEC	"TryExec" /* string */
#define SUCKY_DESKTOP_ITEM_NO_DISPLAY	"NoDisplay" /* boolean */
#define SUCKY_DESKTOP_ITEM_COMMENT	"Comment" /* localestring */
#define SUCKY_DESKTOP_ITEM_EXEC		"Exec" /* string */
#define SUCKY_DESKTOP_ITEM_ACTIONS	"Actions" /* strings */
#define SUCKY_DESKTOP_ITEM_ICON		"Icon" /* string */
#define SUCKY_DESKTOP_ITEM_MINI_ICON	"MiniIcon" /* string */
#define SUCKY_DESKTOP_ITEM_HIDDEN	"Hidden" /* boolean */
#define SUCKY_DESKTOP_ITEM_PATH		"Path" /* string */
#define SUCKY_DESKTOP_ITEM_TERMINAL	"Terminal" /* boolean */
#define SUCKY_DESKTOP_ITEM_TERMINAL_OPTIONS "TerminalOptions" /* string */
#define SUCKY_DESKTOP_ITEM_SWALLOW_TITLE "SwallowTitle" /* string */
#define SUCKY_DESKTOP_ITEM_SWALLOW_EXEC	"SwallowExec" /* string */
#define SUCKY_DESKTOP_ITEM_MIME_TYPE	"MimeType" /* regexp(s) */
#define SUCKY_DESKTOP_ITEM_PATTERNS	"Patterns" /* regexp(s) */
#define SUCKY_DESKTOP_ITEM_DEFAULT_APP	"DefaultApp" /* string */
#define SUCKY_DESKTOP_ITEM_DEV		"Dev" /* string */
#define SUCKY_DESKTOP_ITEM_FS_TYPE	"FSType" /* string */
#define SUCKY_DESKTOP_ITEM_MOUNT_POINT	"MountPoint" /* string */
#define SUCKY_DESKTOP_ITEM_READ_ONLY	"ReadOnly" /* boolean */
#define SUCKY_DESKTOP_ITEM_UNMOUNT_ICON "UnmountIcon" /* string */
#define SUCKY_DESKTOP_ITEM_SORT_ORDER	"SortOrder" /* strings */
#define SUCKY_DESKTOP_ITEM_URL		"URL" /* string */
#define SUCKY_DESKTOP_ITEM_DOC_PATH	"X-GNOME-DocPath" /* string */

/* The vfolder proposal */
#define SUCKY_DESKTOP_ITEM_CATEGORIES	"Categories" /* string */
#define SUCKY_DESKTOP_ITEM_ONLY_SHOW_IN	"OnlyShowIn" /* string */

typedef enum {
	/* Use the TryExec field to determine if this should be loaded */
        SUCKY_DESKTOP_ITEM_LOAD_ONLY_IF_EXISTS = 1<<0,
        SUCKY_DESKTOP_ITEM_LOAD_NO_TRANSLATIONS = 1<<1
} SuckyDesktopItemLoadFlags;

typedef enum {
	/* Never launch more instances even if the app can only
	 * handle one file and we have passed many */
        SUCKY_DESKTOP_ITEM_LAUNCH_ONLY_ONE = 1<<0,
	/* Use current directory instead of home directory */
        SUCKY_DESKTOP_ITEM_LAUNCH_USE_CURRENT_DIR = 1<<1,
	/* Append the list of URIs to the command if no Exec
	 * parameter is specified, instead of launching the 
	 * app without parameters. */
	SUCKY_DESKTOP_ITEM_LAUNCH_APPEND_URIS = 1<<2,
	/* Same as above but instead append local paths */
	SUCKY_DESKTOP_ITEM_LAUNCH_APPEND_PATHS = 1<<3
} SuckyDesktopItemLaunchFlags;

typedef enum {
	/* Don't check the kde directories */
        SUCKY_DESKTOP_ITEM_ICON_NO_KDE = 1<<0
} SuckyDesktopItemIconFlags;

typedef enum {
	SUCKY_DESKTOP_ITEM_ERROR_NO_FILENAME /* No filename set or given on save */,
	SUCKY_DESKTOP_ITEM_ERROR_UNKNOWN_ENCODING /* Unknown encoding of the file */,
	SUCKY_DESKTOP_ITEM_ERROR_CANNOT_OPEN /* Cannot open file */,
	SUCKY_DESKTOP_ITEM_ERROR_NO_EXEC_STRING /* Cannot launch due to no execute string */,
	SUCKY_DESKTOP_ITEM_ERROR_BAD_EXEC_STRING /* Cannot launch due to bad execute string */,
	SUCKY_DESKTOP_ITEM_ERROR_NO_URL /* No URL on a url entry*/,
	SUCKY_DESKTOP_ITEM_ERROR_NOT_LAUNCHABLE /* Not a launchable type of item */,
	SUCKY_DESKTOP_ITEM_ERROR_INVALID_TYPE /* Not of type application/x-gnome-app-info */
} SuckyDesktopItemError;

/* Note that functions can also return the G_FILE_ERROR_* errors */

#define SUCKY_DESKTOP_ITEM_ERROR sucky_desktop_item_error_quark ()
GQuark sucky_desktop_item_error_quark (void);

/* Returned item from new*() and copy() methods have a refcount of 1 */
SuckyDesktopItem *      sucky_desktop_item_new               (void);
SuckyDesktopItem *      sucky_desktop_item_new_from_file     (const char                 *file,
							      SuckyDesktopItemLoadFlags   flags,
							      GError                    **error);
SuckyDesktopItem *      sucky_desktop_item_new_from_uri      (const char                 *uri,
							      SuckyDesktopItemLoadFlags   flags,
							      GError                    **error);
SuckyDesktopItem *      sucky_desktop_item_new_from_string   (const char                 *uri,
							      const char                 *string,
							      gssize                      length,
							      SuckyDesktopItemLoadFlags   flags,
							      GError                    **error);
SuckyDesktopItem *      sucky_desktop_item_new_from_basename (const char                 *basename,
							      SuckyDesktopItemLoadFlags   flags,
							      GError                    **error);
SuckyDesktopItem *      sucky_desktop_item_copy              (const SuckyDesktopItem     *item);

/* if under is NULL save in original location */
gboolean                sucky_desktop_item_save              (SuckyDesktopItem           *item,
							      const char                 *under,
							      gboolean			  force,
							      GError                    **error);
SuckyDesktopItem *      sucky_desktop_item_ref               (SuckyDesktopItem           *item);
void                    sucky_desktop_item_unref             (SuckyDesktopItem           *item);
int			sucky_desktop_item_launch	     (const SuckyDesktopItem     *item,
							      GList                      *file_list,
							      SuckyDesktopItemLaunchFlags flags,
							      GError                    **error);
int			sucky_desktop_item_launch_with_env   (const SuckyDesktopItem     *item,
							      GList                      *file_list,
							      SuckyDesktopItemLaunchFlags flags,
							      char                      **envp,
							      GError                    **error);

int                     sucky_desktop_item_launch_on_screen  (const SuckyDesktopItem       *item,
							      GList                        *file_list,
							      SuckyDesktopItemLaunchFlags   flags,
							      GdkScreen                    *screen,
							      int                           workspace,
							      GError                      **error);

/* A list of files or urls dropped onto an icon This is the output
 * of gnome_vfs_uri_list_parse */
int                     sucky_desktop_item_drop_uri_list     (const SuckyDesktopItem     *item,
							      const char                 *uri_list,
							      SuckyDesktopItemLaunchFlags flags,
							      GError                    **error);

int                     sucky_desktop_item_drop_uri_list_with_env    (const SuckyDesktopItem     *item,
								      const char                 *uri_list,
								      SuckyDesktopItemLaunchFlags flags,
								      char                      **envp,
								      GError                    **error);

gboolean                sucky_desktop_item_exists            (const SuckyDesktopItem     *item);

SuckyDesktopItemType	sucky_desktop_item_get_entry_type    (const SuckyDesktopItem	 *item);
/* You could also just use the set_string on the TYPE argument */
void			sucky_desktop_item_set_entry_type    (SuckyDesktopItem		 *item,
							      SuckyDesktopItemType	  type);

/* Get current location on disk */
const char *            sucky_desktop_item_get_location      (const SuckyDesktopItem     *item);
void                    sucky_desktop_item_set_location      (SuckyDesktopItem           *item,
							      const char                 *location);
void                    sucky_desktop_item_set_location_file (SuckyDesktopItem           *item,
							      const char                 *file);
SuckyDesktopItemStatus  sucky_desktop_item_get_file_status   (const SuckyDesktopItem     *item);

/*
 * Get the icon, this is not as simple as getting the Icon attr as it actually tries to find
 * it and returns %NULL if it can't
 */
char *                  sucky_desktop_item_get_icon          (const SuckyDesktopItem     *item,
							      GnomeIconTheme             *icon_theme);

char *                  sucky_desktop_item_find_icon         (GnomeIconTheme             *icon_theme,
							      const char                 *icon,
							      /* size is only a suggestion */
							      int                         desired_size,
							      int                         flags);


/*
 * Reading/Writing different sections, NULL is the standard section
 */
gboolean                sucky_desktop_item_attr_exists       (const SuckyDesktopItem     *item,
							      const char                 *attr);

/*
 * String type
 */
const char *            sucky_desktop_item_get_string        (const SuckyDesktopItem     *item,
							      const char		 *attr);

void                    sucky_desktop_item_set_string        (SuckyDesktopItem           *item,
							      const char		 *attr,
							      const char                 *value);

const char *            sucky_desktop_item_get_attr_locale   (const SuckyDesktopItem     *item,
							      const char		 *attr);

/*
 * LocaleString type
 */
const char *            sucky_desktop_item_get_localestring  (const SuckyDesktopItem     *item,
							      const char		 *attr);
const char *            sucky_desktop_item_get_localestring_lang (const SuckyDesktopItem *item,
								  const char		 *attr,
								  const char             *language);
/* use g_list_free only */
GList *                 sucky_desktop_item_get_languages     (const SuckyDesktopItem     *item,
							      const char		 *attr);

void                    sucky_desktop_item_set_localestring  (SuckyDesktopItem           *item,
							      const char		 *attr,
							      const char                 *value);
void                    sucky_desktop_item_set_localestring_lang  (SuckyDesktopItem      *item,
								   const char		 *attr,
								   const char		 *language,
								   const char            *value);
void                    sucky_desktop_item_clear_localestring(SuckyDesktopItem           *item,
							      const char		 *attr);

/*
 * Strings, Regexps types
 */

/* use sucky_desktop_item_free_string_list */
char **                 sucky_desktop_item_get_strings       (const SuckyDesktopItem     *item,
							      const char		 *attr);

void			sucky_desktop_item_set_strings       (SuckyDesktopItem           *item,
							      const char                 *attr,
							      char                      **strings);

/*
 * Boolean type
 */
gboolean                sucky_desktop_item_get_boolean       (const SuckyDesktopItem     *item,
							      const char		 *attr);

void                    sucky_desktop_item_set_boolean       (SuckyDesktopItem           *item,
							      const char		 *attr,
							      gboolean                    value);

/*
 * Xserver time of user action that caused the application launch to start.
 */
void                    sucky_desktop_item_set_launch_time   (SuckyDesktopItem           *item,
							      guint32                     timestamp);

/*
 * Clearing attributes
 */
#define                 sucky_desktop_item_clear_attr(item,attr) \
				sucky_desktop_item_set_string(item,attr,NULL)
void			sucky_desktop_item_clear_section     (SuckyDesktopItem           *item,
							      const char                 *section);

G_END_DECLS

#endif /* SUCKY_DITEM_H */
