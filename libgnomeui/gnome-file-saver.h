/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GNOME GUI Library - gnome-file-saver.h
 * Copyright (C) 2000  Red Hat Inc.
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

#ifndef GNOME_FILE_SAVER_H
#define GNOME_FILE_SAVER_H


#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-gconf.h>

G_BEGIN_DECLS

#define GNOME_TYPE_FILE_SAVER            (gnome_file_saver_get_type())
#define GNOME_FILE_SAVER(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_FILE_SAVER, GnomeFileSaver))
#define GNOME_FILE_SAVER_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_FILE_SAVER, GnomeFileSaverClass))
#define GNOME_IS_FILE_SAVER(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_FILE_SAVER))
#define GNOME_IS_FILE_SAVER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_FILE_SAVER))
#define GNOME_FILE_SAVER_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_FILE_SAVER, GnomeFileSaverClass))

typedef struct _GnomeFileSaver        GnomeFileSaver;
typedef struct _GnomeFileSaverPrivate GnomeFileSaverPrivate;
typedef struct _GnomeFileSaverClass   GnomeFileSaverClass;

struct _GnomeFileSaver {
        GnomeDialog parent_instance;

	/*< private >*/
	GnomeFileSaverPrivate *_priv;
};

struct _GnomeFileSaverClass {
        GnomeDialogClass parent_class;
        
        void (* finished) (GnomeFileSaver *saver,
                           const gchar    *filename,
                           const gchar    *mime_type);
};

GtkType         gnome_file_saver_get_type     (void) G_GNUC_CONST;
GtkWidget*      gnome_file_saver_new          (const gchar    *title,
                                               const gchar    *saver_id);

void            gnome_file_saver_add_mime_type(GnomeFileSaver *file_saver,
                                               const gchar    *mime_type);

/* convenience wrapper; not language-binding friendly but also not
   required to be wrapped */
void            gnome_file_saver_add_mime_types(GnomeFileSaver *file_saver,
                                                const gchar    *mime_types[]);

G_END_DECLS

#endif
