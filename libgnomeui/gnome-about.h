/* GNOME GUI Library
 * Copyright (C) 1998 Horacio J. Peña
 * Based in gnome-about, copyright (C) 1997 Jay Painter
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
#ifndef __GNOME_ABOUT_H__
#define __GNOME_ABOUT_H__

#include <gdk/gdk.h>
#include <gtk/gtkwindow.h>

BEGIN_GNOME_DECLS

#define GNOME_ABOUT(obj)        GTK_CHECK_CAST (obj, gnome_about_get_type (), GnomeAbout)
#define GNOME_ABOUT_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gnome_about_get_type (), GnomeAboutClass)
#define GNOME_IS_ABOUT(obj)       GTK_CHECK_TYPE (obj, gnome_about_get_type ())

typedef struct _GnomeAbout        GnomeAbout;
typedef struct _GnomeAboutClass   GnomeAboutClass;
typedef struct _GnomeAboutButton  GnomeAboutButton;

#define GNOME_ABOUT_BUTTON_WIDTH 100
#define GNOME_ABOUT_BUTTON_HEIGHT 40

struct _GnomeAbout
{
  GtkWindow window;
};

struct _GnomeAboutClass
{
  GtkWindowClass parent_class;

};


guint      gnome_about_get_type       (void);
GtkWidget* gnome_about_new            (gchar	*title,
					gchar	*version,
					gchar	*copyright,
					gchar	**authors,
					gchar	*comments,
					gchar	*logo);

END_GNOME_DECLS

#endif /* __GNOME_ABOUT_H__ */
