/* WARNING ____ IMMATURE API ____ liable to change */

/* gnome-property-entries.h - Property entries.

   Copyright (C) 1998 Martin Baulig

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License
   as published by the Free Software Foundation; either version 2, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.
*/

#ifndef __GNOME_PROPERTY_ENTRIES_H__
#define __GNOME_PROPERTY_ENTRIES_H__

#include <libgnomeui/gnome-properties.h>

BEGIN_GNOME_DECLS

/* Font properties. */
GtkWidget *
gnome_property_entry_font (GnomePropertyObject *object, const gchar *label,
			   gchar **font_name_ptr, GdkFont **font_ptr);

/* Color properties. */
GtkWidget *
gnome_property_entry_colors (GnomePropertyObject *object, const gchar *label,
			     gint num_colors, gint columns, gint *table_pos,
			     GdkColor *colors, const gchar *texts []);

END_GNOME_DECLS

#endif
