/* GnomeFontSel widget, by Elliot Lee. Largely derived from: */
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
#ifndef __GNOME_FONT_SELECTOR_H__
#define __GNOME_FONT_SELECTOR_H__

#include <gtk/gtk.h>

BEGIN_GNOME_DECLS

#define GNOME_FONT_SELECTOR(obj) \
   GTK_CHECK_CAST(obj, gnome_font_selector_get_type(), GnomeFontSelector)
#define GNOME_FONT_SELECTOR_CLASS(class) \
     GTK_CHECK_CAST_CLASS(class, gnome_font_selector_get_type(), \
			  GnomeFontSelectorClass)
#define GNOME_IS_FONT_SELECTOR(obj)      GTK_CHECK_TYPE(obj, gnome_font_selector_get_type())

typedef struct _GnomeFontSelector GnomeFontSelector;
typedef struct _GnomeFontSelectorClass GnomeFontSelectorClass;
typedef struct _FontInfo FontInfo; /* struct is defined in gnome-fontsel.c */

struct _GnomeFontSelectorClass
{
  GtkDialogClass parent_class;

  FontInfo **font_info;
  int nfonts;

  GSList *foundries;
  GSList *weights;
  GSList *slants;
  GSList *set_widths;
  GSList *spacings;

  char **foundry_array;
  char **weight_array;
  char **slant_array;
  char **set_width_array;
  char **spacing_array;

  int nfoundries;
  int nweights;
  int nslants;
  int nset_widths;
  int nspacings;

  void *text_options;
};

struct _GnomeFontSelector
{
  GtkDialog parent_object;
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
  int font_index;
  int size_type;
  int foundry;
  int weight;
  int slant;
  int set_width;
  int spacing;
  gpointer gdisp_ptr;
};

guint gnome_font_selector_get_type(void);

END_GNOME_DECLS

#endif /* __TEXT_TOOL_H__ */
