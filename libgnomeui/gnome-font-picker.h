/* GNOME font picker button.
 * Copyright (C) 1998 David Abilleira Freijeiro <odaf@nexo.es>
 * All rights reserved
 * Based on gnome-color-picker by Federico Mena <federico@nuclecu.unam.mx>
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

#ifndef GNOME_FONT_PICKER_H
#define GNOME_FONT_PICKER_H

#include <gtk/gtkbutton.h>


G_BEGIN_DECLS

/* GnomeFontPicker is a button widget that allow user to select a font.
 */

/* Button Mode or What to show */
typedef enum {
    GNOME_FONT_PICKER_MODE_PIXMAP,
    GNOME_FONT_PICKER_MODE_FONT_INFO,
    GNOME_FONT_PICKER_MODE_USER_WIDGET,
    GNOME_FONT_PICKER_MODE_UNKNOWN
} GnomeFontPickerMode;
        

#define GNOME_TYPE_FONT_PICKER            (gnome_font_picker_get_type ())
#define GNOME_FONT_PICKER(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_FONT_PICKER, GnomeFontPicker))
#define GNOME_FONT_PICKER_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_FONT_PICKER, GnomeFontPickerClass))
#define GNOME_IS_FONT_PICKER(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_FONT_PICKER))
#define GNOME_IS_FONT_PICKER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_FONT_PICKER))
#define GNOME_FONT_PICKER_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_FONT_PICKER, GnomeFontPickerClass))

typedef struct _GnomeFontPicker        GnomeFontPicker;
typedef struct _GnomeFontPickerPrivate GnomeFontPickerPrivate;
typedef struct _GnomeFontPickerClass   GnomeFontPickerClass;

struct _GnomeFontPicker {
        GtkButton button;
    
	GtkWidget     *font_dialog;
        GtkWidget     *inside;
        GtkWidget     *font_label, *vsep, *size_label;

	/*< private >*/
	GnomeFontPickerPrivate *_priv;
};

struct _GnomeFontPickerClass {
	GtkButtonClass parent_class;

	/* font_set signal is emitted when font is choosed */
	void (* font_set) (GnomeFontPicker *gfp, gchar *font_name);
};


/* Standard Gtk function */
GtkType gnome_font_picker_get_type (void) G_GNUC_CONST;

/* Creates a new font picker widget */
GtkWidget *gnome_font_picker_new (void);

/* Sets the title for the font selection dialog */
void       gnome_font_picker_set_title       (GnomeFontPicker *gfp,
					      const gchar *title);
const gchar * gnome_font_picker_get_title    (GnomeFontPicker *gfp);

/* Button mode */
GnomeFontPickerMode
           gnome_font_picker_get_mode        (GnomeFontPicker *gfp);

void       gnome_font_picker_set_mode        (GnomeFontPicker *gfp,
                                              GnomeFontPickerMode mode);
/* With  GNOME_FONT_PICKER_MODE_FONT_INFO */
/* If use_font_in_label is true, font name will be writen using font choosed by user and
 using size passed to this function*/
void       gnome_font_picker_fi_set_use_font_in_label (GnomeFontPicker *gfp,
                                                       gboolean use_font_in_label,
                                                       gint size);

void       gnome_font_picker_fi_set_show_size (GnomeFontPicker *gfp,
                                               gboolean show_size);

/* With GNOME_FONT_PICKER_MODE_USER_WIDGET */
void       gnome_font_picker_uw_set_widget    (GnomeFontPicker *gfp,
                                               GtkWidget       *widget);

/* Functions to interface with GtkFontSelectionDialog */
const gchar* gnome_font_picker_get_font_name  (GnomeFontPicker *gfp);

GdkFont*   gnome_font_picker_get_font	      (GnomeFontPicker *gfp);

gboolean   gnome_font_picker_set_font_name    (GnomeFontPicker *gfp,
                                               const gchar     *fontname);

const gchar* gnome_font_picker_get_preview_text (GnomeFontPicker *gfp);

void	   gnome_font_picker_set_preview_text (GnomeFontPicker *gfp,
                                               const gchar     *text);


G_END_DECLS

    
#endif /* GNOME_FONT_PICKER_H */
