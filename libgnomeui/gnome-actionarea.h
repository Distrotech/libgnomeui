/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef GNOME_ACTIONAREA_H
#define GNOME_ACTIONAREA_H

#include <gtk/gtkdialog.h>

typedef void (*GnomeActionCallback) (GtkWidget *, gpointer);

typedef struct {
  char *label;
  GnomeActionCallback callback;
  gpointer user_data;
  GtkWidget *widget;
} GnomeActionAreaItem;

/* NOTE: This interface is going to go away soon, or at least be
   changed. Don't use it. */

void gnome_build_action_area (GtkDialog           * dlg,
			      GnomeActionAreaItem * actions,
			      int                 num_actions,
			      int                 default_action);

#endif
