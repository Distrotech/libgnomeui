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

#include <config.h>
#include "gnome-properties.h"
#include "gnome-property-entries.h"
#include "libgnome/gnome-i18nP.h"
#include "gnome-uidefs.h"
#include "gnome-color-picker.h"

#include <gdk/gdkprivate.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkfontsel.h>
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtktable.h>

/* =======================================================================
 * Font properties object.
 * =======================================================================
 */

typedef struct
{
	GdkFont **font_ptr;
	gchar **font_name_ptr;
	GdkFont *font;
	gchar *font_name;
} FontProperties;

typedef struct
{
	GtkWidget *entry;
	const gchar *label;
	GnomePropertyObject *object;
} FontCbData;

/* Properties callbacks. */

static void
_property_entry_font_load_temp (GnomePropertyObject *object)
{
	FontProperties *prop_ptr = object->prop_data;
	FontProperties *temp_ptr = object->temp_data;

	temp_ptr->font_name_ptr = &temp_ptr->font_name;
	temp_ptr->font_ptr = &temp_ptr->font;

	temp_ptr->font_name = g_strdup (*(prop_ptr->font_name_ptr));
	temp_ptr->font = *(prop_ptr->font_ptr);
	
	if (temp_ptr->font)
		gdk_font_ref (temp_ptr->font);
}

static gint
_property_entry_font_save_temp (GnomePropertyObject *object)
{
	FontProperties *prop_ptr = object->prop_data;
	FontProperties *temp_ptr = object->temp_data;

	g_free (*(prop_ptr->font_name_ptr));

	if (*(prop_ptr->font_ptr))
		gdk_font_unref (*(prop_ptr->font_ptr));

	*(prop_ptr->font_name_ptr) = g_strdup (temp_ptr->font_name);
	*(prop_ptr->font_ptr) = temp_ptr->font;

	if (*(prop_ptr->font_ptr))
		gdk_font_ref (*(prop_ptr->font_ptr));

	return FALSE;
}

static void
_property_entry_font_discard_temp (GnomePropertyObject *object)
{
	FontProperties *temp_ptr = object->temp_data;

	g_free (temp_ptr->font_name);

	if (temp_ptr->font)
		gdk_font_unref (temp_ptr->font);
}

/* The font was changed. */

static gint
_property_entry_font_apply (GtkFontSelectionDialog *fontsel)
{
	FontCbData *cb_data;
	FontProperties *temp_ptr;
	gchar *font_name;
	GdkFont *font;

	font_name = gtk_font_selection_dialog_get_font_name (fontsel);

	if (font_name == NULL) {
		gdk_beep ();
		return FALSE;
	}

	/* Make sure it is a valid font. */

	font = gdk_font_load (font_name);
	
	if (!font) {
		g_free (font_name);
		gdk_beep ();
		return FALSE;
	}

	cb_data = gtk_object_get_data (GTK_OBJECT (fontsel), "cb_data");
	temp_ptr = cb_data->object->temp_data;

	g_free (*(temp_ptr->font_name_ptr));
	if (*(temp_ptr->font_ptr))
		gdk_font_unref ((*temp_ptr->font_ptr));
	
	*(temp_ptr->font_name_ptr) = g_strdup (font_name);
	*(temp_ptr->font_ptr) = font;

	gtk_entry_set_text (GTK_ENTRY (cb_data->entry), font_name);

	gnome_property_object_changed (cb_data->object->user_data);

	return TRUE;
}

/* Callbacks for the Font selection dialog box. */

static void
_property_entry_font_cancel_cb (GtkWidget *widget, GtkWidget *fontsel)
{
	gtk_widget_destroy (fontsel);
}

static void
_property_entry_font_ok_cb (GtkWidget *widget, GtkWidget *fontsel)
{
	if (_property_entry_font_apply (GTK_FONT_SELECTION_DIALOG (fontsel)))
		gtk_widget_destroy (fontsel);
}

/* Font name entry was changed. */

static void
_property_entry_font_entry_cb (GtkWidget *widget, FontCbData *cb_data)
{
	GtkWidget *entry = cb_data->entry;
	FontProperties *temp_ptr = cb_data->object->temp_data;
	gchar *font_name;
	GdkFont *font;

	font_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

	/* Make sure it is a valid font. */

	font = gdk_font_load (font_name);
	
	if (!font) {
		g_free (font_name);
		gdk_beep ();
		return;
	}

	g_free (*(temp_ptr->font_name_ptr));
	
	if (*(temp_ptr->font_ptr))
		gdk_font_unref (*(temp_ptr->font_ptr));

	*(temp_ptr->font_name_ptr) = font_name;
	*(temp_ptr->font_ptr) = font;

	gnome_property_object_changed (cb_data->object->user_data);
}

/* Select button was presssed. */

static void
_property_entry_font_select_cb (GtkWidget *widget, FontCbData *cb_data)
{
	FontProperties *temp_ptr = cb_data->object->temp_data;
	GtkFontSelectionDialog *fontsel;

	fontsel = GTK_FONT_SELECTION_DIALOG
		(gtk_font_selection_dialog_new (_(cb_data->label)));

	gtk_window_set_modal (GTK_WINDOW (fontsel), TRUE);
	
	if (*(temp_ptr->font_name_ptr))
		gtk_font_selection_dialog_set_font_name
			(fontsel, *(temp_ptr->font_name_ptr));

	gtk_signal_connect (GTK_OBJECT (fontsel->ok_button),
			    "pressed", _property_entry_font_ok_cb,
			    (gpointer) fontsel);

	gtk_signal_connect (GTK_OBJECT (fontsel->cancel_button),
			    "pressed", _property_entry_font_cancel_cb,
			    (gpointer) fontsel);

	gtk_object_set_data (GTK_OBJECT (fontsel), "cb_data", cb_data);

	gtk_widget_show (GTK_WIDGET (fontsel));
}

GtkWidget *
gnome_property_entry_font (GnomePropertyObject *object, const gchar *label,
			   gchar **font_name_ptr, GdkFont **font_ptr)
{
	GtkWidget *table, *entry, *button;
	GnomePropertyDescriptor *descriptor;
	GnomePropertyObject *entry_object;
	FontProperties *prop_ptr;
	FontCbData *cb_data;

	table = gtk_table_new (1, 4, TRUE);
	gtk_table_set_col_spacings (GTK_TABLE (table), GNOME_PAD << 2);

	entry = gtk_entry_new ();
	if (*font_name_ptr)
		gtk_entry_set_text (GTK_ENTRY (entry), *font_name_ptr);

	cb_data = g_new0 (FontCbData, 1);
	cb_data->entry = entry;
	cb_data->label = label;

	gtk_signal_connect (GTK_OBJECT (entry), "activate",
			    _property_entry_font_entry_cb, cb_data);

	gtk_table_attach_defaults (GTK_TABLE (table), entry, 0, 3, 0, 1);

	button = gtk_button_new_with_label (_("Select"));

	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    _property_entry_font_select_cb, cb_data);

	gtk_table_attach_defaults (GTK_TABLE (table), button, 3, 4, 0, 1);

	descriptor = g_new0 (GnomePropertyDescriptor, 1);
	descriptor->size = sizeof (FontProperties);

	descriptor->load_temp_func = _property_entry_font_load_temp;
	descriptor->save_temp_func = _property_entry_font_save_temp;
	descriptor->discard_temp_func = _property_entry_font_discard_temp;

	prop_ptr = g_new0 (FontProperties, 1);
	prop_ptr->font_name_ptr = font_name_ptr;
	prop_ptr->font_ptr = font_ptr;

	entry_object = gnome_property_object_new (descriptor, prop_ptr);
	entry_object->user_data = object;

	cb_data->object = entry_object;

	object->object_list = g_list_append (object->object_list, entry_object);

	g_free (descriptor);

	return table;
}

/* =======================================================================
 * Font properties object.
 * =======================================================================
 */

typedef struct
{
	GdkColor *color;
	GnomePropertyObject *object;
} ColorsCbData;

static void
_property_entry_colors_changed_cb (GnomeColorPicker *cp, gint r, gint g,
				   gint b, gint a, ColorsCbData *cb_data)
{
	gushort red, green, blue, alpha;
	
	gnome_color_picker_get_i16 (cp, &red, &green, &blue, &alpha);

	cb_data->color->red   = red;
	cb_data->color->green = green;
	cb_data->color->blue  = blue;

	gnome_property_object_changed (cb_data->object);
}

GtkWidget *
gnome_property_entry_colors (GnomePropertyObject *object, const gchar *label,
			     gint num_colors, gint columns, gint *table_pos,
			     GdkColor *colors, const gchar *texts [])
{
	GtkWidget *frame, *table;
	gint rows, i;

	frame = gtk_frame_new (_(label));

	rows = (num_colors < columns) ? 2 :
		((num_colors+columns-1) / columns) << 1;

	table = gtk_table_new (rows, columns, TRUE);
	gtk_table_set_col_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (table), GNOME_PAD_SMALL);

	for (i = 0; i < num_colors; i++) {
		ColorsCbData *cb_data;
		GtkWidget *label, *cp;
		gint row, col;

		label = gtk_label_new (_(texts [i]));
		cp = gnome_color_picker_new ();
		gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (cp),
					    colors [i].red,
					    colors [i].green,
					    colors [i].blue,
					    0);

		cb_data = g_new0 (ColorsCbData, 1);
		cb_data->color = &colors [i];
		cb_data->object = object;

		gtk_signal_connect (GTK_OBJECT (cp), "color_set", 
				    _property_entry_colors_changed_cb,
				    cb_data);

		col = table_pos ? table_pos [i] : i;
		row = (col / columns) << 1;
		col %= columns;

		gtk_table_attach (GTK_TABLE (table), cp,
				  col, col+1, row, row+1,
				  GTK_FILL, GTK_SHRINK, 0, 0);
		gtk_table_attach (GTK_TABLE (table), label,
				  col, col+1, row+1, row+2,
				  GTK_EXPAND, GTK_FILL, 0, 0);
	}

	gtk_container_add (GTK_CONTAINER (frame), table);
	
	return frame;
}
