/* WARNING ____ IMMATURE API ____ liable to change */

/* gnome-properties.h - Properties interface.

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

#ifndef __GNOME_PROPERTIES_H__
#define __GNOME_PROPERTIES_H__

#include <libgnome/gnome-defs.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-propertybox.h>

BEGIN_GNOME_DECLS

typedef struct _GnomePropertyObject GnomePropertyObject;
typedef struct _GnomePropertyDescriptor GnomePropertyDescriptor;

typedef enum {
	GNOME_PROPERTY_ACTION_APPLY = 1,
	GNOME_PROPERTY_ACTION_UPDATE,
	GNOME_PROPERTY_ACTION_LOAD,
	GNOME_PROPERTY_ACTION_SAVE,
	GNOME_PROPERTY_ACTION_LOAD_TEMP,
	GNOME_PROPERTY_ACTION_SAVE_TEMP,
	GNOME_PROPERTY_ACTION_DISCARD_TEMP,
	GNOME_PROPERTY_ACTION_CHANGED
} GnomePropertyAction;

struct _GnomePropertyDescriptor {
	guint size;
	const gchar *label;
	GtkWidget * (*init_func) (GnomePropertyObject *);
	void (*apply_func) (GnomePropertyObject *);
	void (*update_func) (GnomePropertyObject *);
	void (*load_func) (GnomePropertyObject *);
	void (*save_func) (GnomePropertyObject *);
	void (*load_temp_func) (GnomePropertyObject *);
	gint (*save_temp_func) (GnomePropertyObject *);
	void (*discard_temp_func) (GnomePropertyObject *);
	void (*changed_func) (GnomePropertyObject *);
	GList *next;
};

struct _GnomePropertyObject {
	GtkWidget *label;
	GnomePropertyDescriptor *descriptor;
	gpointer prop_data, temp_data, user_data;
	GList *object_list;
};

/* Create new property object from property descriptor. */
GnomePropertyObject *
gnome_property_object_new (GnomePropertyDescriptor *descriptor,
			   gpointer property_data_ptr);

/* Add new property object to the property box. */
void
gnome_property_object_register (GnomePropertyBox *property_box,
				GnomePropertyObject *object);

/* Walk the list of property object and invoke callbacks. */
void
gnome_property_object_list_walk (GList *property_object_list,
				 GnomePropertyAction action);

/* Implementation of property actions. */
void gnome_property_object_apply (GnomePropertyObject *object);
void gnome_property_object_update (GnomePropertyObject *object);
void gnome_property_object_load (GnomePropertyObject *object);
void gnome_property_object_save (GnomePropertyObject *object);
void gnome_property_object_load_temp (GnomePropertyObject *object);
void gnome_property_object_save_temp (GnomePropertyObject *object);
void gnome_property_object_discard_temp (GnomePropertyObject *object);
void gnome_property_object_changed (GnomePropertyObject *object);

END_GNOME_DECLS

#endif
