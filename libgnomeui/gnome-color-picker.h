/*
 * Copyright (C) 1998, 1999 Red Hat, Inc.
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */
/* Color picker button for GNOME
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_COLOR_PICKER_H
#define GNOME_COLOR_PICKER_H

#ifndef GNOME_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

/* The GnomeColorPicker widget is a simple color picker in a button.  The button displays a sample
 * of the currently selected color.  When the user clicks on the button, a color selection dialog
 * pops up.  The color picker emits the "color_set" signal when the color is set
 *
 * By default, the color picker does dithering when drawing the color sample box.  This can be
 * disabled for cases where it is useful to see the allocated color without dithering.
 */

#define GNOME_TYPE_COLOR_PICKER            (gnome_color_picker_get_type ())
#define GNOME_COLOR_PICKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_COLOR_PICKER, GnomeColorPicker))
#define GNOME_COLOR_PICKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_COLOR_PICKER, GnomeColorPickerClass))
#define GNOME_IS_COLOR_PICKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_COLOR_PICKER))
#define GNOME_IS_COLOR_PICKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_COLOR_PICKER))
#define GNOME_COLOR_PICKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_COLOR_PICKER, GnomeColorPickerClass))


typedef struct _GnomeColorPicker        GnomeColorPicker;
typedef struct _GnomeColorPickerPrivate GnomeColorPickerPrivate;
typedef struct _GnomeColorPickerClass   GnomeColorPickerClass;

struct _GnomeColorPicker {
	GtkButton button;

	/*< private >*/
	GnomeColorPickerPrivate *_priv;
};

struct _GnomeColorPickerClass {
	GtkButtonClass parent_class;

	/* Signal that is emitted when the color is set.  The rgba values
	 * are in the [0, 65535] range.  If you need a different color
	 * format, use the provided functions to get the values from the
	 * color picker.
	 */
        /*  (should be gushort, but Gtk can't marshal that.) */
	void (* color_set) (GnomeColorPicker *cp, guint r, guint g, guint b, guint a);

	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};

/* Standard Gtk function */
GType gnome_color_picker_get_type (void) G_GNUC_CONST;

/* Creates a new color picker widget */
GtkWidget *gnome_color_picker_new (void);

/* Set/get the color in the picker.  Values are in [0.0, 1.0] */
void gnome_color_picker_set_d (GnomeColorPicker *cp, gdouble r, gdouble g, gdouble b, gdouble a);
void gnome_color_picker_get_d (GnomeColorPicker *cp, gdouble *r, gdouble *g, gdouble *b, gdouble *a);

/* Set/get the color in the picker.  Values are in [0, 255] */
void gnome_color_picker_set_i8 (GnomeColorPicker *cp, guint8 r, guint8 g, guint8 b, guint8 a);
void gnome_color_picker_get_i8 (GnomeColorPicker *cp, guint8 *r, guint8 *g, guint8 *b, guint8 *a);

/* Set/get the color in the picker.  Values are in [0, 65535] */
void gnome_color_picker_set_i16 (GnomeColorPicker *cp, gushort r, gushort g, gushort b, gushort a);
void gnome_color_picker_get_i16 (GnomeColorPicker *cp, gushort *r, gushort *g, gushort *b, gushort *a);

/* Sets whether the picker should dither the color sample or just paint a solid rectangle */
void gnome_color_picker_set_dither (GnomeColorPicker *cp, gboolean dither);
gboolean gnome_color_picker_get_dither (GnomeColorPicker *cp);

/* Sets whether the picker should use the alpha channel or not */
void gnome_color_picker_set_use_alpha (GnomeColorPicker *cp, gboolean use_alpha);
gboolean gnome_color_picker_get_use_alpha (GnomeColorPicker *cp);

/* Sets the title for the color selection dialog */
void gnome_color_picker_set_title (GnomeColorPicker *cp, const gchar *title);
const char * gnome_color_picker_get_title (GnomeColorPicker *cp);

G_END_DECLS

#endif /* GNOME_DISABLE_DEPRECATED */

#endif /* GNOME_COLOR_PICKER_H */
