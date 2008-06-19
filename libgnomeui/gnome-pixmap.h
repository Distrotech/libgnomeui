/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*-

   Copyright (C) 1999, 2000 Red Hat, Inc.
   All rights reserved.
    
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
   Boston, MA 02111-1307, USA.

   GnomePixmap Developers: Havoc Pennington, Jonathan Blandford

   Though this is hardly GnomePixmap anymore :)
   If you use this API in new applications, you will be strangled to death,
   please use GtkImage, it's much nicer and cooler and this just uses it anyway

     -George
*/
/*
  @NOTATION@
*/

#ifndef GNOME_PIXMAP_H
#define GNOME_PIXMAP_H

#ifndef GNOME_DISABLE_DEPRECATED

#include <gtk/gtk.h>


G_BEGIN_DECLS

#define GNOME_TYPE_PIXMAP            (gnome_pixmap_get_type ())
#define GNOME_PIXMAP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_PIXMAP, GnomePixmap))
#define GNOME_PIXMAP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_PIXMAP, GnomePixmapClass))
#define GNOME_IS_PIXMAP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_PIXMAP))
#define GNOME_IS_PIXMAP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_PIXMAP))
#define GNOME_PIXMAP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_PIXMAP, GnomePixmapClass))

/* Note:
 * You should use GtkImage if you can, this is just a compatibility wrapper to get
 * old code to compile */

typedef struct _GnomePixmap        GnomePixmap;
typedef struct _GnomePixmapClass   GnomePixmapClass;
typedef struct _GnomePixmapPrivate GnomePixmapPrivate;

struct _GnomePixmap {
	GtkImage parent;

	GnomePixmapPrivate *_priv;
};


struct _GnomePixmapClass {
	GtkImageClass parent_class;
};

GType           gnome_pixmap_get_type                (void) G_GNUC_CONST;

GtkWidget      *gnome_pixmap_new_from_file           (const gchar      *filename);
GtkWidget      *gnome_pixmap_new_from_file_at_size   (const gchar      *filename,
						      gint              width,
						      gint              height);
GtkWidget      *gnome_pixmap_new_from_xpm_d          (const gchar     **xpm_data);
GtkWidget      *gnome_pixmap_new_from_xpm_d_at_size  (const gchar     **xpm_data,
						      gint              width,
						      gint              height);
GtkWidget      *gnome_pixmap_new_from_gnome_pixmap   (GnomePixmap      *gpixmap);

void            gnome_pixmap_load_file               (GnomePixmap      *gpixmap,
						      const char       *filename);
void            gnome_pixmap_load_file_at_size       (GnomePixmap      *gpixmap,
						      const char       *filename,
						      int               width,
						      int               height);
void            gnome_pixmap_load_xpm_d              (GnomePixmap      *gpixmap,
						      const char      **xpm_data);
void            gnome_pixmap_load_xpm_d_at_size      (GnomePixmap      *gpixmap,
						      const char      **xpm_data,
						      int               width,
						      int               height);


G_END_DECLS

#endif /* GNOME_DISABLE_DEPRECATED */

#endif /* __GNOME_PIXMAP_H__ */
