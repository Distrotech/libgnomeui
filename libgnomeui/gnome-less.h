/* gnome-less.h:
 * Copyright (C) 1998,2000 Free Software Foundation
 * All rights reserved.
 *
 * A simple GtkText wrapper with a scroll bar, convenience functions,
 * and a right-click popup menu. For displaying info to the user,
 * like a license agreement for example. OK, bad example. :-)
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
/*
  @NOTATION@
*/

#ifndef GNOME_LESS_H
#define GNOME_LESS_H


#include <gtk/gtk.h>
#include <stdio.h>

G_BEGIN_DECLS

typedef struct _GnomeLess        GnomeLess;
typedef struct _GnomeLessPrivate GnomeLessPrivate;
typedef struct _GnomeLessClass   GnomeLessClass;

#define GNOME_TYPE_LESS            (gnome_less_get_type ())
#define GNOME_LESS(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_LESS, GnomeLess))
#define GNOME_LESS_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_LESS, GnomeLessClass))
#define GNOME_IS_LESS(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_LESS))
#define GNOME_IS_LESS_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_LESS))
#define GNOME_LESS_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_LESS, GnomeLessClass))

struct _GnomeLess {
	GtkHBox hbox;

	/*< public >*/
	GtkTextView *text_view; 
	GtkTextBuffer *text_buffer;

	/*< private >*/
	GnomeLessPrivate *_priv;
};

struct _GnomeLessClass {
	GtkHBoxClass parent_class;
};

GtkType  gnome_less_get_type		(void) G_GNUC_CONST;

GtkWidget * gnome_less_new		(void);

/* Sugar new function for program output */
GtkWidget * gnome_less_new_fixed_font	(int columns);

/* Clear the text */
void     gnome_less_clear		(GnomeLess * gl);

/* Append stuff to the widget.  TRUE on success, FALSE
 * with errno set on failiure */
gboolean gnome_less_append_file		(GnomeLess * gl, const gchar * path);
gboolean gnome_less_append_command	(GnomeLess * gl, const gchar * command_line);
void     gnome_less_append_string	(GnomeLess * gl, const gchar * s);
gboolean gnome_less_append_filestream	(GnomeLess * gl, FILE * f);
gboolean gnome_less_append_fd		(GnomeLess * gl, int file_descriptor);

/* All these clear any existing text and show whatever you pass in. 
   When applicable, they return TRUE on success, FALSE and set errno 
   on failure. */
gboolean gnome_less_show_file		(GnomeLess * gl, const gchar * path);
gboolean gnome_less_show_command	(GnomeLess * gl, const gchar * command_line);
void     gnome_less_show_string		(GnomeLess * gl, const gchar * s);
gboolean gnome_less_show_filestream	(GnomeLess * gl, FILE * f);
gboolean gnome_less_show_fd		(GnomeLess * gl, int file_descriptor);

/* Write a file; returns FALSE and sets errno if either open
   or close fails on the file. write_file overwrites any existing file. */
gboolean gnome_less_write_file		(GnomeLess * gl, const gchar * path);
gboolean gnome_less_write_fd		(GnomeLess * gl, int fd);

/* Set an arbitrary font */
void     gnome_less_set_font_string	(GnomeLess * gl, const char * font);
/*
 * Whether to use a fixed font for any future showings. 
 * Recommended for anything that comes in columns, program code,
 * etc. Just loads a fixed font and calls set_font above.
 */
void     gnome_less_set_font_fixed	(GnomeLess * gl);
/* Sets the standard string */
void     gnome_less_set_font_standard	(GnomeLess * gl);
/* from a pango font description */
void     gnome_less_set_font_description(GnomeLess * gl,
					 const PangoFontDescription * font_desc);

void     gnome_less_set_width_columns	(GnomeLess * gl, int columns);
void     gnome_less_set_wrap_mode	(GnomeLess * gl, GtkWrapMode wrap_mode);

PangoFontDescription *gnome_less_get_font_description(GnomeLess * gl);

#ifndef GNOME_EXCLUDE_DEPRECATED
/* DEPRECATED */
/* Re-insert the text with the current font settings. */
void gnome_less_reshow         		(GnomeLess * gl);
void gnome_less_set_font		(GnomeLess * gl, GdkFont * font);
void gnome_less_set_fixed_font		(GnomeLess * gl, gboolean fixed);
#endif

G_END_DECLS
   
#endif /* GNOME_LESS_H */




