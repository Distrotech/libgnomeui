/* gtkcauldron.h - creates complex dialogs from a format string in a single line
 * Copyright (C) 1998 Paul Sheer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __GTK_CAULDRON_H__
#define __GTK_CAULDRON_H__

#include <gtk/gtkwidget.h>
#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#define GTK_CAULDRON_TOPLEVEL	(0x1L<<0)
#define GTK_CAULDRON_DIALOG		(0x1L<<1)
#define GTK_CAULDRON_POPUP		(0x1L<<2)

#define GTK_CAULDRON_SPACE_SHIFT	(3)
#define GTK_CAULDRON_SPACE_MASK	(0xFL<<GTK_CAULDRON_SPACE_SHIFT)

#define GTK_CAULDRON_SPACE1		(0x1L<<GTK_CAULDRON_SPACE_SHIFT)
#define GTK_CAULDRON_SPACE2		(0x2L<<GTK_CAULDRON_SPACE_SHIFT)
#define GTK_CAULDRON_SPACE3		(0x3L<<GTK_CAULDRON_SPACE_SHIFT)
#define GTK_CAULDRON_SPACE4		(0x4L<<GTK_CAULDRON_SPACE_SHIFT)
#define GTK_CAULDRON_SPACE5		(0x5L<<GTK_CAULDRON_SPACE_SHIFT)
#define GTK_CAULDRON_SPACE6		(0x6L<<GTK_CAULDRON_SPACE_SHIFT)
#define GTK_CAULDRON_SPACE7		(0x7L<<GTK_CAULDRON_SPACE_SHIFT)
#define GTK_CAULDRON_SPACE8		(0x8L<<GTK_CAULDRON_SPACE_SHIFT)
#define GTK_CAULDRON_SPACE9		(0x9L<<GTK_CAULDRON_SPACE_SHIFT)
#define GTK_CAULDRON_SPACE10		(0xAL<<GTK_CAULDRON_SPACE_SHIFT)
#define GTK_CAULDRON_SPACE11		(0xBL<<GTK_CAULDRON_SPACE_SHIFT)
#define GTK_CAULDRON_SPACE12		(0xCL<<GTK_CAULDRON_SPACE_SHIFT)
#define GTK_CAULDRON_SPACE13		(0xDL<<GTK_CAULDRON_SPACE_SHIFT)
#define GTK_CAULDRON_SPACE14		(0xEL<<GTK_CAULDRON_SPACE_SHIFT)
#define GTK_CAULDRON_SPACE15		(0xFL<<GTK_CAULDRON_SPACE_SHIFT)

#define GTK_CAULDRON_IGNOREESCAPE	(0x1L<<7)
#define GTK_CAULDRON_IGNOREENTER	(0x1L<<8)
#define GTK_CAULDRON_GRAB		(0x1L<<9)
#define GTK_CAULDRON_PARENT		(0x1L<<10)

typedef void (*GtkCauldronNextArgCallback) (gint cauldron_type, gpointer user_data, void *result);

/* int cauldron_type : */
enum {
    GTK_CAULDRON_TYPE_CHAR_P,
    GTK_CAULDRON_TYPE_CHAR_P_P,
    GTK_CAULDRON_TYPE_INT,
    GTK_CAULDRON_TYPE_INT_P,
    GTK_CAULDRON_TYPE_USERDATA_P,
    GTK_CAULDRON_TYPE_DOUBLE,
    GTK_CAULDRON_TYPE_DOUBLE_P,
    GTK_CAULDRON_TYPE_CALLBACK
};

extern gchar *GTK_CAULDRON_ENTER;
extern gchar *GTK_CAULDRON_ESCAPE;
#define GTK_CAULDRON_ERROR ((gchar *) (-1))

/* GTK_CAULDRON_TYPE_CALLBACK : */
typedef GtkWidget *(*GtkCauldronCustomCallback) (GtkWidget * widget, gpointer user_data);

/* if gtk_dialog_cauldron_parse() returns GTK_CAULDRON_ERROR */
gchar *gtk_dialog_cauldron_get_error (void);

/* for straight C usage */
gchar *gtk_dialog_cauldron (const gchar * title, glong options, ...);
/*
    i.e. either
	gchar *gtk_dialog_cauldron (const gchar * title, glong options, gchar *fmt, ...);
    or if GTK_CAULDRON_PARENT is given along in `options', then,
	gchar *gtk_dialog_cauldron (const gchar * title, glong options, GtkWindow *parent, gchar *fmt, ...);
*/

/* for interpreters */
gchar *gtk_dialog_cauldron_parse (const gchar * title, glong options,
				  const gchar * format,
				  GtkCauldronNextArgCallback next_arg,
				  gpointer user_data,
				  GtkWidget *parent);

END_GNOME_DECLS

#endif /* __GTK_CAULDRON_H__ */

