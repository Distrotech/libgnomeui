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

#include <config.h>
#include <string.h>
#include <gtk/gtklabel.h>
#include <gtk/gtknotebook.h>
#include "gnome-properties.h"
#include "libgnome/gnome-config.h"
#include "libgnome/gnome-i18nP.h"

/* Create new property object from property descriptor. */
GnomePropertyObject *
gnome_property_object_new (GnomePropertyDescriptor *descriptor,
			   gpointer property_data_ptr)
{
	GnomePropertyObject *object;

	object = g_new0 (GnomePropertyObject, 1);

	object->descriptor = g_new0 (GnomePropertyDescriptor, 1);

	if (property_data_ptr)
		object->prop_data = property_data_ptr;
	else
		object->prop_data = g_malloc0 (descriptor->size);

	memcpy (object->descriptor, descriptor,
		sizeof (GnomePropertyDescriptor));

	gnome_property_object_load (object);

	return object;
}

/* Add new property object to the property box. */
void
gnome_property_object_register (GnomePropertyBox *property_box,
				GnomePropertyObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (property_box != NULL);
	g_return_if_fail (GNOME_IS_PROPERTY_BOX (property_box));

	g_return_if_fail (object->descriptor != NULL);
	g_return_if_fail (object->descriptor->init_func != NULL);

	if (!object->label) {
		object->label = gtk_label_new (gettext (object->descriptor->label));
		gtk_object_ref (GTK_OBJECT (object->label));
	}

	gtk_notebook_append_page
		(GTK_NOTEBOOK (property_box->notebook),
		 object->descriptor->init_func (object),
		 object->label);
}

/* Walk the list of property object and invoke callbacks. */

static void
_property_object_action (GnomePropertyObject *object, 
			 GnomePropertyAction action)
{
	switch (action) {
	case GNOME_PROPERTY_ACTION_APPLY:
		gnome_property_object_apply (object);
		break;
	case GNOME_PROPERTY_ACTION_UPDATE:
		gnome_property_object_update (object);
		break;
	case GNOME_PROPERTY_ACTION_LOAD:
		gnome_property_object_load (object);
		break;
	case GNOME_PROPERTY_ACTION_SAVE:
		gnome_property_object_save (object);
		break;
	case GNOME_PROPERTY_ACTION_LOAD_TEMP:
		gnome_property_object_load_temp (object);
		break;
	case GNOME_PROPERTY_ACTION_SAVE_TEMP:
		gnome_property_object_save_temp (object);
		break;
	case GNOME_PROPERTY_ACTION_DISCARD_TEMP:
		gnome_property_object_discard_temp (object);
		break;
	case GNOME_PROPERTY_ACTION_CHANGED:
		gnome_property_object_changed (object);
		break;
	}
}

void
gnome_property_object_list_walk (GList *property_object_list,
				 GnomePropertyAction action)
{
	GList *c, *d;
	
	for (c = property_object_list; c; c = c->next) {
		GnomePropertyObject *object = c->data;

		_property_object_action (object, action);

		for (d = object->object_list; d; d = d->next) {
			GnomePropertyObject *obj = d->data;

			_property_object_action (obj, action);
		}
	}
}

/* Implementation of property actions. */

void
gnome_property_object_apply (GnomePropertyObject *object)
{
	if (object->descriptor->apply_func)
		object->descriptor->apply_func (object);
}

void
gnome_property_object_update (GnomePropertyObject *object)
{
	if (object->descriptor->update_func)
		object->descriptor->update_func (object);
}

void
gnome_property_object_load (GnomePropertyObject *object)
{
	if (object->descriptor->load_func)
		object->descriptor->load_func (object);
}

void
gnome_property_object_save (GnomePropertyObject *object)
{
	if (object->descriptor->save_func)
		object->descriptor->save_func (object);

	gnome_config_sync ();
}

void
gnome_property_object_load_temp (GnomePropertyObject *object)
{
	gnome_property_object_discard_temp (object);

	object->temp_data = g_malloc0 (object->descriptor->size);

	memcpy (object->temp_data, object->prop_data,
		object->descriptor->size);

	if (object->descriptor->load_temp_func)
		object->descriptor->load_temp_func (object);
}

void
gnome_property_object_save_temp (GnomePropertyObject *object)
{
	gint copy = TRUE;

	if (object->descriptor->save_temp_func)
		copy = object->descriptor->save_temp_func (object);

	if (copy)
		memcpy (object->prop_data, object->temp_data,
			object->descriptor->size);
}

void
gnome_property_object_discard_temp (GnomePropertyObject *object)
{
	if (!object->temp_data)
		return;

	if (object->descriptor->discard_temp_func)
		object->descriptor->discard_temp_func (object);

	g_free (object->temp_data);
	object->temp_data = NULL;
}

void
gnome_property_object_changed (GnomePropertyObject *object)
{
	if (object->descriptor->changed_func)
		object->descriptor->changed_func (object);
}
