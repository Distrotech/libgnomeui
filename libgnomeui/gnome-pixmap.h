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
*/
/*
  @NOTATION@
*/

#ifndef GNOME_PIXMAP_H
#define GNOME_PIXMAP_H

#include <gtk/gtkmisc.h>
#include <gdk-pixbuf/gdk-pixbuf.h>



G_BEGIN_DECLS

typedef enum {
        /* update struct when adding enum values */
	GNOME_PIXMAP_SIMPLE, /* No alpha blending */
	GNOME_PIXMAP_COLOR   /* */
} GnomePixmapDrawMode;

#define GNOME_TYPE_PIXMAP            (gnome_pixmap_get_type ())
#define GNOME_PIXMAP(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_PIXMAP, GnomePixmap))
#define GNOME_PIXMAP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_PIXMAP, GnomePixmapClass))
#define GNOME_IS_PIXMAP(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_PIXMAP))
#define GNOME_IS_PIXMAP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_PIXMAP))
#define GNOME_PIXMAP_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_PIXMAP, GnomePixmapClass))


typedef struct _GnomePixmap GnomePixmap;
typedef struct _GnomePixmapClass GnomePixmapClass;

struct _GnomePixmap {
	GtkMisc misc;

        /* NOTE. In the old GnomePixmap, _lots_ of people were using GnomePixmap to
	 * load images, sucking out the pixmap field, and then not using the
	 * GnomePixmap as a widget at all. IF YOU DO THIS I WILL PERSONALLY
	 * KICK YOUR ASS. Use gdk_pixbuf_new_from_file(). Thank you.
	 * These are PRIVATE FIELDS which I will gratuitously change just to
	 * break your broken code.
	 *                          -  hp + jrb + quartic + Jesse Ventura + GW Bush 
	 */

	GdkPixbuf *provided_image;

        struct {
                GdkPixbuf *pixbuf;
                GdkBitmap *mask;
		gfloat saturation;
		gboolean pixelate;
        } provided[5]; /* the five states */

	GdkPixbuf *generated_scaled_image;
	GdkBitmap *generated_scaled_mask;
        
        struct {
                GdkPixbuf *pixbuf;
                GdkBitmap *mask;
        } generated[5]; /* the five states */
        
	gint width, height;
	gint alpha_threshold;

	GnomePixmapDrawMode mode : 2;
};


struct _GnomePixmapClass {
	GtkMiscClass parent_class;
};

guint           gnome_pixmap_get_type                (void) G_GNUC_CONST;
GtkWidget      *gnome_pixmap_new                     (void);
GtkWidget      *gnome_pixmap_new_from_file           (const gchar      *filename);
GtkWidget      *gnome_pixmap_new_from_file_at_size   (const gchar      *filename,
						      gint              width,
						      gint              height);
GtkWidget      *gnome_pixmap_new_from_xpm_d          (const gchar     **xpm_data);
GtkWidget      *gnome_pixmap_new_from_xpm_d_at_size  (const gchar     **xpm_data,
						      gint              width,
						      gint              height);
GtkWidget      *gnome_pixmap_new_from_pixbuf         (GdkPixbuf        *pixbuf);
GtkWidget      *gnome_pixmap_new_from_pixbuf_at_size (GdkPixbuf        *pixbuf,
						      gint              width,
						      gint              height);
void            gnome_pixmap_set_pixbuf_size         (GnomePixmap      *gpixmap,
						      gint              width,
						      gint              height);
void            gnome_pixmap_get_pixbuf_size         (GnomePixmap      *gpixmap,
						      gint             *width,
						      gint             *height);
void            gnome_pixmap_set_pixbuf              (GnomePixmap      *gpixmap,
						      GdkPixbuf        *pixbuf);
GdkPixbuf      *gnome_pixmap_get_pixbuf              (GnomePixmap      *gpixmap);
   
/* Sets the individual states, instead of generating them. */
void            gnome_pixmap_set_pixbuf_at_state     (GnomePixmap      *gpixmap,
						      GtkStateType      state,
						      GdkPixbuf        *pixbuf,
						      GdkBitmap        *mask);
void            gnome_pixmap_set_state_pixbufs       (GnomePixmap      *gpixmap,
                                                      GdkPixbuf        *pixbufs[5],
                                                      GdkBitmap        *masks[5]);
void            gnome_pixmap_set_draw_vals           (GnomePixmap      *gpixmap,
						      GtkStateType      state,
						      gfloat            saturation,
						      gboolean          pixelate);
void            gnome_pixmap_get_draw_vals           (GnomePixmap      *gpixmap,
						      GtkStateType      state,
						      gfloat           *saturation,
						      gboolean         *pixelate);
void            gnome_pixmap_set_draw_mode           (GnomePixmap      *gpixmap,
						      GnomePixmapDrawMode   mode);
GnomePixmapDrawMode gnome_pixmap_get_draw_mode       (GnomePixmap      *gpixmap);
void            gnome_pixmap_clear                   (GnomePixmap      *gpixmap);
void            gnome_pixmap_set_alpha_threshold     (GnomePixmap      *gpixmap,
						      gint              alpha_threshold);
gint            gnome_pixmap_get_alpha_threshold     (GnomePixmap      *gpixmap);


G_END_DECLS

#endif /* __GNOME_PIXMAP_H__ */
