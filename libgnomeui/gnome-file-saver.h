/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GNOME GUI Library - gnome-file-saver.h
 * Copyright (C) 2000  Red Hat Inc.,
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

#ifndef GNOME_FILE_SAVER_H
#define GNOME_FILE_SAVER_H

#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-gconf.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GNOME_TYPE_FILE_SAVER (gnome_file_saver_get_type())
#define GNOME_FILE_SAVER(obj)  (GTK_CHECK_CAST ((obj), GNOME_TYPE_FILE_SAVER, GnomeFileSaver))
#define GNOME_FILE_SAVER_CLASS(klass)  (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_FILE_SAVER, GnomeFileSaverClass))
#define GNOME_IS_FILE_SAVER(obj)  (GTK_CHECK_TYPE ((obj), GNOME_TYPE_FILE_SAVER))
#define GNOME_IS_FILE_SAVER_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_FILE_SAVER))

typedef struct _GnomeFileSaver GnomeFileSaver;
typedef struct _GnomeFileSaverClass GnomeFileSaverClass;

struct _GnomeFileSaver {
        GnomeDialog parent_instance;
        
        /* all private, don't even think about it */

        GtkWidget *entry;
        GtkWidget *option;
        GtkWidget *menu;
        
        GConfClient *conf;
        guint        conf_notify;

        GSList      *locations;
};

struct _GnomeFileSaverClass {
        GnomeDialogClass parent_class;
        
        void (* filename_chosen) (GnomeFileSaver *saver,
                                  const gchar    *filename);
};

GtkType         gnome_file_saver_get_type     (void);
GnomeFileSaver* gnome_file_saver_new          (void);
gchar*          gnome_file_saver_get_filename (GnomeFileSaver *saver);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
