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
#ifndef __GNOME_DIALOG_H__
#define __GNOME_DIALOG_H__


#include <gdk/gdk.h>
#include <gtk/gtkwindow.h>
#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#define GNOME_DIALOG(obj)        GTK_CHECK_CAST (obj, gnome_dialog_get_type (), GnomeDialog)
#define GNOME_DIALOG_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gnome_dialog_get_type (), GnomeDialogClass)
#define GNOME_IS_DIALOG(obj)       GTK_CHECK_TYPE (obj, gnome_dialog_get_type ())

typedef struct _GnomeDialog        GnomeDialog;
typedef struct _GnomeDialogClass   GnomeDialogClass;
typedef struct _GnomeDialogButton  GnomeDialogButton;

struct _GnomeDialog
{
  GtkWindow window;
  GtkWidget * vbox;
  GtkWidget * action_area; /* A button box, not an hbox */

  GList *buttons;
  int modal : 1;
  int self_destruct : 1;
};

struct _GnomeDialogClass
{
  GtkWindowClass parent_class;

  void (* clicked)  (GnomeDialog *dialog, gint button);
};


guint      gnome_dialog_get_type       (void);

/* Arguments: Title and button names, then NULL */
GtkWidget* gnome_dialog_new            (const gchar * title,
					...);
void       gnome_dialog_set_modal      (GnomeDialog *dialog);
void       gnome_dialog_set_default    (GnomeDialog *dialog,
					gint         button);
void       gnome_dialog_set_sensitive  (GnomeDialog *dialog,
					gint         button,
					gboolean     setting);
/* Whether to destroy after emitting clicked signal - 
   default is FALSE */
void       gnome_dialog_set_destroy    (GnomeDialog * dialog,
					gboolean self_destruct);

/* Use of append_buttons is discouraged, it's really
   meant for subclasses. */
void       gnome_dialog_append_buttons (GnomeDialog * dialog,
					const gchar * first,
					...);

END_GNOME_DECLS

#endif /* __GNOME_DIALOG_H__ */
