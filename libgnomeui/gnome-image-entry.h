/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
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
/*
  @NOTATION@
 */

/* GnomeImageEntry widget - combo box with auto-saved history
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_IMAGE_ENTRY_H
#define GNOME_IMAGE_ENTRY_H


#include <libgnome/gnome-selector.h>
#include <libgnomeui/gnome-selector-client.h>


G_BEGIN_DECLS


#define GNOME_TYPE_IMAGE_ENTRY            (gnome_image_entry_get_type ())
#define GNOME_IMAGE_ENTRY(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_IMAGE_ENTRY, GnomeImageEntry))
#define GNOME_IMAGE_ENTRY_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_IMAGE_ENTRY, GnomeImageEntryClass))
#define GNOME_IS_IMAGE_ENTRY(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_IMAGE_ENTRY))
#define GNOME_IS_IMAGE_ENTRY_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_IMAGE_ENTRY))


typedef struct _GnomeImageEntry        GnomeImageEntry;
typedef struct _GnomeImageEntryPrivate GnomeImageEntryPrivate;
typedef struct _GnomeImageEntryClass   GnomeImageEntryClass;

struct _GnomeImageEntry {
	GnomeSelectorClient client;

	/*< private >*/
	GnomeImageEntryPrivate *_priv;
};

struct _GnomeImageEntryClass {
	GnomeSelectorClientClass parent_class;
};


GType        gnome_image_entry_get_type          (void) G_GNUC_CONST;

GtkWidget   *gnome_image_entry_new_icon_entry    (void);

GtkWidget   *gnome_image_entry_new_pixmap_entry  (guint                preview_x,
                                                  guint                preview_y);

GtkWidget   *gnome_image_entry_new_from_selector (GNOME_Selector       corba_selector,
						  Bonobo_UIContainer   uic);

G_END_DECLS

#endif
