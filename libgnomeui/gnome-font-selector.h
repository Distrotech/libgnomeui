/* GnomeFontSelector widget, by Elliot Lee.
   Derived from code in: */
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */

#ifndef __GNOME_FONT_SELECTOR_H__
#define __GNOME_FONT_SELECTOR_H__

#include <gtk/gtk.h>
#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

typedef struct _GnomeFontSelector GnomeFontSelector;
typedef struct _GnomeFontSelectorClass GnomeFontSelectorClass;

#define GNOME_TYPE_FONT_SELECTOR            (gnome_font_selector_get_type ())
#define GNOME_FONT_SELECTOR(obj)            (GTK_CHECK_CAST((obj), GNOME_TYPE_FONT_SELECTOR, GnomeFontSelector))
#define GNOME_FONT_SELECTOR_CLASS(klass)    (GTK_CHECK_CLASS_CAST((klass), GNOME_TYPE_FONT_SELECTOR, GnomeFontSelectorClass))
#define GNOME_IS_FONT_SELECTOR(obj)         (GTK_CHECK_TYPE((obj), GNOME_TYPE_FONT_SELECTOR))
#define GNOME_IS_FONT_SELECTOR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_FONT_SELECTOR))

typedef struct _FontInfo FontInfo; /* struct is defined in gnome-fontsel.c */

struct _GnomeFontSelectorClass
{
  GtkDialogClass parent_class;

  FontInfo **font_info;
  gint nfonts;

  GSList *foundries;
  GSList *weights;
  GSList *slants;
  GSList *set_widths;
  GSList *spacings;

  gchar **foundry_array;
  gchar **weight_array;
  gchar **slant_array;
  gchar **set_width_array;
  gchar **spacing_array;

  gint nfoundries;
  gint nweights;
  gint nslants;
  gint nset_widths;
  gint nspacings;

  gpointer text_options;
};

struct _GnomeFontSelector
{
  GtkDialog parent_object;

  GtkWidget *ok_button, *cancel_button;
  GtkWidget *main_vbox;
  GtkWidget *font_list;
  GtkWidget *size_menu;
  GtkWidget *size_text;
  GtkWidget *border_text;
  GtkWidget *text_frame;
  GtkWidget *the_text;
  GtkWidget *menus[5];
  GtkWidget *option_menus[5];
  GtkWidget **foundry_items;
  GtkWidget **weight_items;
  GtkWidget **slant_items;
  GtkWidget **set_width_items;
  GtkWidget **spacing_items;
  GtkWidget *antialias_toggle;
  GdkFont *font;
  gint font_index;
  gint size_type;
  gint foundry;
  gint weight;
  gint slant;
  gint set_width;
  gint spacing;
};

guint gnome_font_selector_get_type(void);
GtkWidget *gnome_font_selector_new(void);
/* You should free this retval up after you're done with it */
gchar *gnome_font_selector_get_selected(GnomeFontSelector *text_tool);
/* Basically runs a modal-dialog version of this, and returns
   the string that id's the selected font. */
gchar *gnome_font_select(void);
gchar *gnome_font_select_with_default(const gchar *);

END_GNOME_DECLS

#endif /* __GNOME_FONT_SELECTOR_H__ */




