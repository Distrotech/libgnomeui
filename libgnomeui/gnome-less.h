/* gnome-less.h: Copyright (C) 1998 Free Software Foundation
 *  A simple GtkText wrapper with a scroll bar, convenience functions,
 *   and a right-click popup menu. For displaying info to the user,
 *   like a license agreement for example. OK, bad example. :-)
 * Written by: Havoc Pennington
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

#ifndef GNOME_LESS_H
#define GNOME_LESS_H

#include "libgnome/gnome-defs.h"
#include <gtk/gtk.h>
#include <stdio.h>

BEGIN_GNOME_DECLS

typedef struct _GnomeLess GnomeLess;
typedef struct _GnomeLessClass GnomeLessClass;

#define GNOME_TYPE_LESS            (gnome_less_get_type ())
#define GNOME_LESS(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_LESS, GnomeLess))
#define GNOME_LESS_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_LESS, GnomeLessClass))
#define GNOME_IS_LESS(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_LESS))
#define GNOME_IS_LESS_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_LESS))

struct _GnomeLess {
  GtkVBox vbox;

  GtkText * text; 

  GtkMenu * popup;

  GdkFont * font;  /* font that goes to gtk_text_insert_text call,
		      can be NULL */
};

struct _GnomeLessClass {
  GtkVBoxClass parent_class;
};

guint gnome_less_get_type       (void);

GtkWidget * gnome_less_new      (void);

/* Clear the text */
void gnome_less_clear (GnomeLess * gl);

/* FIXME maybe add "append" equivalents to these */
/* All these clear any existing text and show whatever you pass in. 
   When applicable, they return TRUE on success, FALSE and set errno 
   on failure. */
gboolean gnome_less_show_file       (GnomeLess * gl, const gchar * path);
gboolean gnome_less_show_command    (GnomeLess * gl, const gchar * command_line);
void     gnome_less_show_string     (GnomeLess * gl, const gchar * s);
gboolean gnome_less_show_filestream (GnomeLess * gl, FILE * f);
gboolean gnome_less_show_fd         (GnomeLess * gl, int file_descriptor);

/* Write a file; returns FALSE and sets errno if either open
   or close fails on the file. write_file overwrites any existing file. */
gboolean gnome_less_write_file   (GnomeLess * gl, const gchar * path);
gboolean gnome_less_write_fd     (GnomeLess * gl, int fd);

/* Set an arbitrary font */
void gnome_less_set_font        (GnomeLess * gl, GdkFont * font);

/*
 * Whether to use a fixed font for any future showings. 
 * Recommended for anything that comes in columns, program code,
 * etc. Just loads a fixed font and calls set_font above.
 */
void gnome_less_set_fixed_font  (GnomeLess * gl, gboolean fixed);

/* Re-insert the text with the current font settings. */
void gnome_less_reshow          (GnomeLess * gl);

/* Replaced by the more versatile set_fixed_font - avoid. */
void gnome_less_fixed_font      (GnomeLess * gl);

END_GNOME_DECLS
   
#endif /* GNOME_LESS_H */




