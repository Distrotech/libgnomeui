/* gnome-icon-sel.h: Copyright (C) 1998 Free Software Foundation
 *  For selecting an icon.
 * Written by: Havoc Pennington, based on John Ellis's code.
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

#ifndef GNOME_ICON_SEL_H
#define GNOME_ICON_SEL_H

#include "libgnome/gnome-defs.h"
#include <gtk/gtk.h>

BEGIN_GNOME_DECLS

typedef struct _GnomeIconSelection GnomeIconSelection;
typedef struct _GnomeIconSelectionClass GnomeIconSelectionClass;

#define GNOME_TYPE_ICON_SELECTION            (gnome_icon_selection_get_type ())
#define GNOME_ICON_SELECTION(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_ICON_SELECTION, GnomeIconSelection))
#define GNOME_ICON_SELECTION_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ICON_SELECTION, GnomeIconSelectionClass))
#define GNOME_IS_ICON_SELECTION(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_ICON_SELECTION))
#define GNOME_IS_ICON_SELECTION_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ICON_SELECTION))

struct _GnomeIconSelection {
  GtkVBox vbox;

  GtkWidget * box;

  GtkWidget * gil;

  GList * file_list;
  
  int stop_loading; /* a flag set to stop the loading of images in midprocess */
};

struct _GnomeIconSelectionClass {
  GtkVBoxClass parent_class;
};

guint gnome_icon_selection_get_type       (void);

GtkWidget * gnome_icon_selection_new      (void);

/* Add default Gnome icon directories */
void  gnome_icon_selection_add_defaults   (GnomeIconSelection * gis);

/* Add icons from this directory */
void  gnome_icon_selection_add_directory  (GnomeIconSelection * gis,
					   const gchar * dir);

/* Loads and displays the icons that were added using gnome_icon_selection_add_* */
void  gnome_icon_selection_show_icons  (GnomeIconSelection * gis);

/* Clear all icons (even the non shown ones if not_shown is set)*/
void  gnome_icon_selection_clear          (GnomeIconSelection * gis,
					   gboolean not_shown);

/* if (full_path) return the whole filename, otherwise just the 
   last component */
const gchar * 
gnome_icon_selection_get_icon             (GnomeIconSelection * gis,
					   gboolean full_path);

/* Filename is only the last part, not the full path */
void  gnome_icon_selection_select_icon    (GnomeIconSelection * gis,
					   const gchar * filename);

/* Stop the loading of images when we are in the loop in show_icons */
void  gnome_icon_selection_stop_loading   (GnomeIconSelection * gis);

END_GNOME_DECLS
   
#endif /* GNOME_ICON_SEL_H */




