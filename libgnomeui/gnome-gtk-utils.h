/* GNOME GUI Library
 * Copyright (C) Red Hat Software
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

#ifndef __GNOME_GTK_UTILS_H__
#define __GNOME_GTK_UTILS_H__

#include <gtk/gtkwidget.h>
#include <libgnome/gnome-defs.h>


BEGIN_GNOME_DECLS

/*
 *  Description: Creates a GtkLabel with 'labelstr' as its text.
 *               Creates a GtkHBox
 *               Inserts the GtkLabel and then 'child' into the GtkHBox
 *	         Returns the GtkHBox
*/
GtkWidget *gnome_build_labelled_widget(char *labelstr, GtkWidget *child);

/* same as above, but does widget+label instead of label+widget */
GtkWidget *gnome_build_widget_labelled(GtkWidget *child, const char *labelstr);

END_GNOME_DECLS

#endif
