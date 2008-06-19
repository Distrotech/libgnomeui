/* GNOME GUI Library
 * Copyright (C) 1995-1998 Jay Painter
 * All rights reserved.
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
#ifndef __GNOME_MESSAGE_BOX_H__
#define __GNOME_MESSAGE_BOX_H__

#ifndef GNOME_DISABLE_DEPRECATED

#include "gnome-dialog.h"

G_BEGIN_DECLS

#define GNOME_TYPE_MESSAGE_BOX            (gnome_message_box_get_type ())
#define GNOME_MESSAGE_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_MESSAGE_BOX, GnomeMessageBox))
#define GNOME_MESSAGE_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_MESSAGE_BOX, GnomeMessageBoxClass))
#define GNOME_IS_MESSAGE_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_MESSAGE_BOX))
#define GNOME_IS_MESSAGE_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_MESSAGE_BOX))
#define GNOME_MESSAGE_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_MESSAGE_BOX, GnomeMessageBoxClass))


#define GNOME_MESSAGE_BOX_INFO      "info"
#define GNOME_MESSAGE_BOX_WARNING   "warning"
#define GNOME_MESSAGE_BOX_ERROR     "error"
#define GNOME_MESSAGE_BOX_QUESTION  "question"
#define GNOME_MESSAGE_BOX_GENERIC   "generic"


typedef struct _GnomeMessageBox        GnomeMessageBox;
typedef struct _GnomeMessageBoxPrivate GnomeMessageBoxPrivate;
typedef struct _GnomeMessageBoxClass   GnomeMessageBoxClass;
typedef struct _GnomeMessageBoxButton  GnomeMessageBoxButton;

/**
 * GnomeMessageBox:
 * @dialog: A #GnomeDialog widget holding the contents of the message box.
 */
struct _GnomeMessageBox
{
  /*< public >*/
  GnomeDialog dialog;
  /*< private >*/
  GnomeMessageBoxPrivate *_priv;
};

struct _GnomeMessageBoxClass
{
  GnomeDialogClass parent_class;
};


GType      gnome_message_box_get_type   (void) G_GNUC_CONST;
GtkWidget* gnome_message_box_new        (const gchar           *message,
					 const gchar          *message_box_type,
					 ...);

GtkWidget* gnome_message_box_newv       (const gchar           *message,
					 const gchar          *message_box_type,
					 const gchar          **buttons);

void        gnome_message_box_construct  (GnomeMessageBox      *messagebox,
					  const gchar          *message,
					  const gchar         *message_box_type,
					  const gchar         **buttons);

G_END_DECLS

#endif /* GNOME_DISABLE_DEPRECATED */

#endif /* __GNOME_MESSAGE_BOX_H__ */
