/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GNOME GUI Library - gnome-recently-used.h
 * Copyright (C) 2000  Red Hat Inc.,
 * All rights reserved.
 *
 * Author: Havoc Pennington <hp@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
*/

#ifndef GNOME_RECENTLY_USED_H
#define GNOME_RECENTLY_USED_H


#include <libgnomeui/gnome-gconf.h>

G_BEGIN_DECLS

typedef struct _GnomeRecentDocument GnomeRecentDocument;

#define GNOME_TYPE_RECENTLY_USED            (gnome_recently_used_get_type())
#define GNOME_RECENTLY_USED(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_RECENTLY_USED, GnomeRecentlyUsed))
#define GNOME_RECENTLY_USED_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_RECENTLY_USED, GnomeRecentlyUsedClass))
#define GNOME_IS_RECENTLY_USED(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_RECENTLY_USED))
#define GNOME_IS_RECENTLY_USED_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_RECENTLY_USED))
#define GNOME_RECENTLY_USED_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_RECENTLY_USED, GnomeRecentlyUsedClass))

typedef struct _GnomeRecentlyUsed        GnomeRecentlyUsed;
typedef struct _GnomeRecentlyUsedPrivate GnomeRecentlyUsedPrivate;
typedef struct _GnomeRecentlyUsedClass   GnomeRecentlyUsedClass;

struct _GnomeRecentlyUsed {
        GtkObject parent_instance;

	/*< private >*/
	GnomeRecentlyUsedPrivate *_priv;
};

struct _GnomeRecentlyUsedClass {
        GtkObjectClass parent_class;

        void (* document_added) (GnomeRecentlyUsed* obj, GnomeRecentDocument *doc);
        void (* document_removed) (GnomeRecentlyUsed* obj, GnomeRecentDocument *doc);
        void (* document_changed) (GnomeRecentlyUsed* obj, GnomeRecentDocument *doc);
};

GtkType              gnome_recently_used_get_type         (void) G_GNUC_CONST;
GnomeRecentlyUsed*   gnome_recently_used_new              (void);
GnomeRecentlyUsed*   gnome_recently_used_new_app_specific (void);

void                 gnome_recently_used_add              (GnomeRecentlyUsed   *recently_used,
                                                           GnomeRecentDocument *doc);
void                 gnome_recently_used_remove           (GnomeRecentlyUsed   *recently_used,
                                                           GnomeRecentDocument *doc);
GSList*              gnome_recently_used_get_all          (GnomeRecentlyUsed   *recently_used);

void                 gnome_recently_used_document_changed (GnomeRecentlyUsed   *recently_used,
                                                           GnomeRecentDocument *doc);

/* convenience wrapper to allow ignoring the GnomeRecentDocument data
   type. string args can be NULL. auto-sets the app ID and creation time */
void                 gnome_recently_used_add_simple       (GnomeRecentlyUsed   *recently_used,
                                                           const gchar         *command,
                                                           const gchar         *menu_text,
                                                           const gchar         *menu_pixmap,
                                                           const gchar         *menu_hint,
                                                           const gchar         *filename,
                                                           const gchar         *mime_type);

GnomeRecentDocument* gnome_recent_document_new            (void);
GnomeRecentDocument* gnome_recent_document_ref            (GnomeRecentDocument *doc);
void                 gnome_recent_document_unref          (GnomeRecentDocument *doc);

/* Available args:
      "command" - command to run when menu item is selected
      "menu-text" - menu text to display
      "menu-pixmap" - pixmap filename to display
      "menu-hint" - menu hint to put in statusbar
      "filename" - document filename (maybe used if command isn't run)
      "mime-type" - mime type of the file
      "app" - application ID of the app creating the GnomeRecentDocument
*/
void                 gnome_recent_document_set            (GnomeRecentDocument *doc,
                                                           const gchar         *arg,
                                                           const gchar         *val);

/* can return NULL if the arg wasn't set for whatever reason */
const gchar*         gnome_recent_document_peek            (GnomeRecentDocument *doc,
							    const gchar         *arg);


GTime                gnome_recent_document_get_creation_time (GnomeRecentDocument *doc);


G_END_DECLS

#endif
