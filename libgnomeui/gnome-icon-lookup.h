/*
 * Copyright (C) 2002 Alexander Larsson <alexl@redhat.com>.
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef GNOME_ICON_LOOKUP_H
#define GNOME_ICON_LOOKUP_H

#include <libgnomeui/gnome-icon-theme.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include "gnome-thumbnail.h"

G_BEGIN_DECLS

typedef enum {
  GNOME_ICON_LOOKUP_FLAGS_NONE = 0,
  GNOME_ICON_LOOKUP_FLAGS_EMBEDDING_TEXT = 1<<0,
  GNOME_ICON_LOOKUP_FLAGS_SHOW_SMALL_IMAGES_AS_THEMSELVES = 1<<1,
} GnomeIconLookupFlags;

typedef enum {
  GNOME_ICON_LOOKUP_RESULT_FLAGS_NONE = 0,
  GNOME_ICON_LOOKUP_RESULT_FLAGS_THUMBNAIL = 1<<0,
} GnomeIconLookupResultFlags;


char *gnome_icon_lookup      (GnomeIconTheme             *icon_theme,
			      GnomeThumbnailFactory      *thumbnail_factory,
			      const char                 *file_uri,
			      const char                 *custom_icon,
			      GnomeVFSFileInfo           *file_info,
			      const char                 *mime_type,
			      GnomeIconLookupFlags        flags,
			      GnomeIconLookupResultFlags *result);
char *gnome_icon_lookup_sync (GnomeIconTheme             *icon_theme,
			      GnomeThumbnailFactory      *thumbnail_factory,
			      const char                 *file_uri,
			      const char                 *custom_icon,
			      GnomeIconLookupFlags        flags,
			      GnomeIconLookupResultFlags *result);


G_END_DECLS

#endif /* GNOME_ICON_LOOKUP_H */
