/* GNOME GUI Library
 * Copyright (C) 1995-1998 Jay Painter
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
#ifndef __GNOME_MESSAGE_BOX_H__
#define __GNOME_MESSAGE_BOX_H__


#include <gdk/gdk.h>
#include <gtk/gtkwindow.h>
#include <libgnome/gnome-defs.h>


#define GNOME_MESSAGE_BOX_WIDTH  425
#define GNOME_MESSAGE_BOX_HEIGHT 125
#define GNOME_MESSAGE_BOX_BORDER_WIDTH 5

#define GNOME_MESSAGE_BOX_BUTTON_WIDTH 100
#define GNOME_MESSAGE_BOX_BUTTON_HEIGHT 40


BEGIN_GNOME_DECLS

#define GNOME_MESSAGE_BOX(obj)        GTK_CHECK_CAST (obj, gnome_message_box_get_type (), GnomeMessageBox)
#define GNOME_MESSAGE_BOX_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gnome_message_box_get_type (), GnomeMessageBoxClass)
#define GNOME_IS_MESSAGE_BOX(obj)       GTK_CHECK_TYPE (obj, gnome_message_box_get_type ())


#define GNOME_MESSAGE_BOX_INFO      "info"
#define GNOME_MESSAGE_BOX_WARNING   "warning"
#define GNOME_MESSAGE_BOX_ERROR     "error"
#define GNOME_MESSAGE_BOX_QUESTION  "question"
#define GNOME_MESSAGE_BOX_GENERIC   "generic"


typedef struct _GnomeMessageBox        GnomeMessageBox;
typedef struct _GnomeMessageBoxClass   GnomeMessageBoxClass;
typedef struct _GnomeMessageBoxButton  GnomeMessageBoxButton;

struct _GnomeMessageBox
{
  GtkWindow window;

  GList *buttons;
  int modal : 1;
};

struct _GnomeMessageBoxClass
{
  GtkWindowClass parent_class;

  void (* clicked)  (GnomeMessageBox *messagebox, gint button);
};


guint      gnome_message_box_get_type       (void);
GtkWidget* gnome_message_box_new            (gchar           *message,
					     gchar           *messagebox_type,
					     ...);
void       gnome_message_box_set_modal      (GnomeMessageBox *messagebox);
void       gnome_message_box_set_default    (GnomeMessageBox *messagebox,
					     gint            button);

END_GNOME_DECLS

#endif /* __GNOME_MESSAGE_BOX_H__ */
