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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef GNOME_LESS_H
#define GNOME_LESS_H

#include "libgnome/gnome-defs.h"
#include <gtk/gtk.h>

BEGIN_GNOME_DECLS

typedef struct _GnomeLess GnomeLess;
typedef struct _GnomeLessClass GnomeLessClass;

#define GNOME_LESS(obj)        GTK_CHECK_CAST (obj, gnome_less_get_type (), GnomeLess)
#define GNOME_LESS_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gnome_less_get_type (), GnomeLessClass)
#define GNOME_IS_LESS(obj)       GTK_CHECK_TYPE (obj, gnome_less_get_type ())

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

guint gnome_less_get_type(void);

GtkWidget * gnome_less_new(void);

/* All these clear any existing text and show whatever you pass in. */
void gnome_less_show_file(GnomeLess * gl, const gchar * path);
void gnome_less_show_command(GnomeLess * gl,
			     const gchar * command_line);
void gnome_less_show_string(GnomeLess * gl, const gchar * s);
void gnome_less_show_filestream(GnomeLess * gl, FILE * f);

/* Use a fixed font for any future showings. 
   Recommended for anything that comes in columns, program code,
   etc. */
void gnome_less_fixed_font(GnomeLess * gl);

END_GNOME_DECLS
   
#endif /* GNOME_LESS_H */




