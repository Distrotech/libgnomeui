/* GnomeFontSelector widget, by Elliot Lee.
   Derived from app/text_tool.c in: */
/* The GIMP -- an image manipulation program
 * Copyright (C) 1995, 1998 Spencer Kimball and Peter Mattis
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

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include "gnome-font-selector.h"
#include "libgnome/libgnomeP.h"
#include "libgnomeui/gnome-window-icon.h"
#include "gnome-app.h"
#include "gnome-app-helper.h"
#include "gdk/gdkkeysyms.h"

#define FONT_LIST_WIDTH  125
#define FONT_LIST_HEIGHT 200

#define PIXELS 0
#define POINTS 1

#define FOUNDRY   0
#define FAMILY    1
#define WEIGHT    2
#define SLANT     3
#define SET_WIDTH 4
#define SPACING   10

#define SUPERSAMPLE 3

struct _FontInfo
{
  gchar *family;         /* The font family this info struct describes. */
  gint *foundries;       /* An array of valid foundries. */
  gint *weights;         /* An array of valid weights. */
  gint *slants;          /* An array of valid slants. */
  gint *set_widths;      /* An array of valid set widths. */
  gint *spacings;        /* An array of valid spacings */
  gint **combos;         /* An array of valid combinations of the above 5 items */
  gint ncombos;          /* The number of elements in the "combos" array */
  GSList *fontnames;   /* An list of valid fontnames.
			 * This is used to make sure a family/foundry/weight/slant/set_width
			 *  combination is valid.
			 */
};

static void gnome_font_selector_class_init(GnomeFontSelectorClass *klass);
static void gnome_font_selector_init(GtkWidget *widget);
static void       text_resize_text_widget (GnomeFontSelector *);
static void       text_ok_callback        (GtkWidget *, gpointer);
static void       text_cancel_callback    (GtkWidget *, gpointer);
static gint       text_delete_callback    (GtkWidget *, GdkEvent *, gpointer);
static void       text_pixels_callback    (GtkWidget *, gpointer);
#if 0
/* Commented out until used.  */
static void       text_points_callback    (GtkWidget *, gpointer);
#endif
static void       text_foundry_callback   (GtkWidget *, gpointer);
static void       text_weight_callback    (GtkWidget *, gpointer);
static void       text_slant_callback     (GtkWidget *, gpointer);
static void       text_set_width_callback (GtkWidget *, gpointer);
static void       text_spacing_callback   (GtkWidget *, gpointer);
static gint       text_size_key_function  (GtkWidget *, GdkEventKey *, gpointer);
static void       text_font_item_update   (GtkWidget *, gpointer);
static void       text_validate_combo     (GnomeFontSelector *, gint);

static void       text_get_fonts          (GnomeFontSelectorClass *klass);
static void       text_insert_font        (FontInfo **, gint *, const gchar *);
static GSList*    text_insert_field       (GSList *, const gchar *, gint);
static gchar*     text_get_field          (const gchar *, gint);
static gint       text_field_to_index     (gchar **,
					   gint, const gchar *);
static gint       text_is_xlfd_font_name  (const gchar *);

static gchar *    text_get_xlfd           (gdouble, gint, const gchar *,
					   const gchar *, const gchar *,
					   const gchar *, const gchar *,
					   const gchar *);
static gint       text_load_font          (GnomeFontSelector *);

typedef GtkSignalFunc MenuItemCallback;
typedef struct {
  gchar *label;
  gint foo, bar;
  gpointer cb, cbdata, blah, baz;
  
} MenuItem;

static GnomeUIInfo size_metric_items[] =
{
  { GNOME_APP_UI_ITEM, "Pixels", "Use pixels as the unit of measurement",
    text_pixels_callback, NULL, NULL, GNOME_APP_PIXMAP_NONE,
    NULL, 0, 0, NULL},
  { GNOME_APP_UI_ITEM, "Points", "Use points as the unit of measurement",
    text_pixels_callback, NULL, NULL, GNOME_APP_PIXMAP_NONE,
    NULL, 0, 0, NULL},
  { GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL }
};

guint gnome_font_selector_get_type(void)
{
  static guint gnomefontsel_type = 0;
  if(!gnomefontsel_type) {
    GtkTypeInfo gnomefontsel_info = {
      "GnomeFontSelector",
      sizeof(GnomeFontSelector),
      sizeof(GnomeFontSelectorClass),
      (GtkClassInitFunc) gnome_font_selector_class_init,
      (GtkObjectInitFunc) gnome_font_selector_init,
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL,
    };
    gnomefontsel_type = gtk_type_unique(gtk_dialog_get_type(),
					&gnomefontsel_info);
  }
  return gnomefontsel_type;
}

static void
gnome_font_selector_class_init(GnomeFontSelectorClass *klass)
{
}

static void
gnome_font_selector_init(GtkWidget *widget)
{
  GtkWidget *top_hbox;
  GtkWidget *right_vbox;
  GtkWidget *text_hbox;
  GtkWidget *list_box;
  GtkWidget *list_item;
  GtkWidget *size_opt_menu;
  GtkWidget *border_label;
  GtkWidget *menu_table;
  GtkWidget *menu_label;
  GtkWidget **menu_items[5];
  GtkWidget *alignment;
  int nmenu_items[5];
  char *menu_strs[5];
  char **menu_item_strs[5];
  int *font_infos[5];
  MenuItemCallback menu_callbacks[5];
  int i, j;
  FontInfo **font_info;
  int nfonts;
  GnomeFontSelectorClass *klass;
  GnomeFontSelector *text_tool;

  text_tool = GNOME_FONT_SELECTOR(widget);

  klass = GNOME_FONT_SELECTOR_CLASS(GTK_OBJECT(widget)->klass);
  text_get_fonts(klass);

  font_info = klass->font_info;
  nfonts = klass->nfonts;

  gnome_window_icon_set_from_default (GTK_WINDOW (text_tool));

  /* Create the shell and vertical & horizontal boxes */
  gtk_window_set_title (GTK_WINDOW (text_tool), "Font Selector");
  gtk_window_set_policy (GTK_WINDOW (text_tool), FALSE, TRUE, TRUE);
  gtk_window_set_position (GTK_WINDOW (text_tool), GTK_WIN_POS_MOUSE);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (text_tool), "delete_event",
		      GTK_SIGNAL_FUNC (text_delete_callback),
		      text_tool);

  text_tool->main_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (text_tool->main_vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (text_tool)->vbox), text_tool->main_vbox, TRUE, TRUE, 0);
  top_hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (text_tool->main_vbox), top_hbox, TRUE, TRUE, 0);

  /* Create the font listbox  */
  list_box = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (list_box, FONT_LIST_WIDTH, FONT_LIST_HEIGHT);
  gtk_box_pack_start (GTK_BOX (top_hbox), list_box, TRUE, TRUE, 0);
  text_tool->font_list = gtk_list_new ();
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (list_box), text_tool->font_list);
  gtk_list_set_selection_mode (GTK_LIST (text_tool->font_list), GTK_SELECTION_BROWSE);

  for (i = 0; i < nfonts; i++)
    {
      list_item = gtk_list_item_new_with_label (font_info[i]->family);
      gtk_object_set_data(GTK_OBJECT(list_item),
			  "GnomeFontSelector", (gpointer)text_tool);
      gtk_container_add (GTK_CONTAINER (text_tool->font_list), list_item);
      gtk_signal_connect (GTK_OBJECT (list_item), "select",
			  (GtkSignalFunc) text_font_item_update,
			  (gpointer) ((long) i));
      gtk_widget_show (list_item);
    }

  gtk_widget_show (text_tool->font_list);

  /* Create the box to hold options  */
  right_vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (top_hbox), right_vbox, TRUE, TRUE, 2);

  /* Create the text hbox, size text, and fonts size metric option menu */
  text_hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (right_vbox), text_hbox, FALSE, FALSE, 0);
  text_tool->size_text = gtk_entry_new ();
  gtk_widget_set_usize (text_tool->size_text, 75, 0);
  gtk_entry_set_text (GTK_ENTRY (text_tool->size_text), "50");
  gtk_signal_connect (GTK_OBJECT (text_tool->size_text), "key_press_event",
		      (GtkSignalFunc) text_size_key_function,
		      text_tool);
  gtk_box_pack_start (GTK_BOX (text_hbox), text_tool->size_text, TRUE, TRUE, 0);

  /* Create the size menu */
  text_tool->size_menu = gtk_menu_new();
  for(i = 0; size_metric_items[i].type != GNOME_APP_UI_ENDOFINFO; i++)
    {
      size_metric_items[i].widget = gtk_menu_item_new_with_label(size_metric_items[i].label);
      gtk_signal_connect(GTK_OBJECT(size_metric_items[i].widget),
			 "activate",
			 size_metric_items[i].moreinfo, text_tool);
      gtk_widget_show(size_metric_items[i].widget);
      gtk_menu_append(GTK_MENU(text_tool->size_menu),
		      size_metric_items[i].widget);
    }

  size_opt_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (text_hbox), size_opt_menu, FALSE, FALSE, 0);

  /* create the text entry widget */
  text_tool->text_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (text_tool->text_frame), GTK_SHADOW_NONE);
  gtk_box_pack_start (GTK_BOX (text_tool->main_vbox), text_tool->text_frame, FALSE, FALSE, 2);
  text_tool->the_text = gtk_entry_new ();
  gtk_entry_set_text(GTK_ENTRY(text_tool->the_text), "AaBbCcDdEeFfGgHh");
  gtk_entry_set_position(GTK_ENTRY(text_tool->the_text), 0);
  gtk_container_add (GTK_CONTAINER (text_tool->text_frame), text_tool->the_text);


  /* Allocate the arrays for the foundry, weight, slant, set_width and spacing menu items */
  text_tool->foundry_items = (GtkWidget **) g_malloc (sizeof (GtkWidget *) * klass->nfoundries);
  text_tool->weight_items = (GtkWidget **) g_malloc (sizeof (GtkWidget *) * klass->nweights);
  text_tool->slant_items = (GtkWidget **) g_malloc (sizeof (GtkWidget *) * klass->nslants);
  text_tool->set_width_items = (GtkWidget **) g_malloc (sizeof (GtkWidget *) * klass->nset_widths);
  text_tool->spacing_items = (GtkWidget **) g_malloc (sizeof (GtkWidget *) * klass->nspacings);

  menu_items[0] = text_tool->foundry_items;
  menu_items[1] = text_tool->weight_items;
  menu_items[2] = text_tool->slant_items;
  menu_items[3] = text_tool->set_width_items;
  menu_items[4] = text_tool->spacing_items;

  nmenu_items[0] = klass->nfoundries;
  nmenu_items[1] = klass->nweights;
  nmenu_items[2] = klass->nslants;
  nmenu_items[3] = klass->nset_widths;
  nmenu_items[4] = klass->nspacings;

  menu_strs[0] = "Foundry";
  menu_strs[1] = "Weight";
  menu_strs[2] = "Slant";
  menu_strs[3] = "Set width";
  menu_strs[4] = "Spacing";

  menu_item_strs[0] = klass->foundry_array;
  menu_item_strs[1] = klass->weight_array;
  menu_item_strs[2] = klass->slant_array;
  menu_item_strs[3] = klass->set_width_array;
  menu_item_strs[4] = klass->spacing_array;

  menu_callbacks[0] = GTK_SIGNAL_FUNC(text_foundry_callback);
  menu_callbacks[1] = GTK_SIGNAL_FUNC(text_weight_callback);
  menu_callbacks[2] = GTK_SIGNAL_FUNC(text_slant_callback);
  menu_callbacks[3] = GTK_SIGNAL_FUNC(text_set_width_callback);
  menu_callbacks[4] = GTK_SIGNAL_FUNC(text_spacing_callback);

  font_infos[0] = font_info[0]->foundries;
  font_infos[1] = font_info[0]->weights;
  font_infos[2] = font_info[0]->slants;
  font_infos[3] = font_info[0]->set_widths;
  font_infos[4] = font_info[0]->spacings;

  menu_table = gtk_table_new (6, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (right_vbox), menu_table, TRUE, TRUE, 0);

  /* Create the other menus */
  for (i = 0; i < 5; i++)
    {
      menu_label = gtk_label_new (menu_strs[i]);
      gtk_misc_set_alignment (GTK_MISC (menu_label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (menu_table), menu_label, 0, 1, i, i + 1,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 1);

      alignment = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
      gtk_table_attach (GTK_TABLE (menu_table), alignment, 1, 2, i, i + 1,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 1, 1);
      text_tool->menus[i] = gtk_menu_new ();

      for (j = 0; j < nmenu_items[i]; j++)
	{
	  menu_items[i][j] = gtk_menu_item_new_with_label (menu_item_strs[i][j]);
	  gtk_widget_set_sensitive (menu_items[i][j], font_infos[i][j]);

	  gtk_container_add (GTK_CONTAINER (text_tool->menus[i]), menu_items[i][j]);
	  gtk_object_set_data(GTK_OBJECT(menu_items[i][j]),
			      "GnomeFontSelector", text_tool);
	  gtk_signal_connect (GTK_OBJECT (menu_items[i][j]), "activate",
			      (GtkSignalFunc) menu_callbacks[i],
			      (gpointer) ((long) j));
	  gtk_widget_show (menu_items[i][j]);
	}

      text_tool->option_menus[i] = gtk_option_menu_new ();
      gtk_container_add (GTK_CONTAINER (alignment), text_tool->option_menus[i]);

      gtk_widget_show (menu_label);
      gtk_widget_show (text_tool->option_menus[i]);
      gtk_widget_show (alignment);

      gtk_option_menu_set_menu (GTK_OPTION_MENU (text_tool->option_menus[i]), text_tool->menus[i]);
    }

  /* Create the border text hbox, border text, and label  */
  border_label = gtk_label_new ("Border");
  gtk_misc_set_alignment (GTK_MISC (border_label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (menu_table), border_label, 0, 1, i, i + 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 1);
  alignment = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
  gtk_table_attach (GTK_TABLE (menu_table), alignment, 1, 2, i, i + 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 1, 1);
  text_tool->border_text = gtk_entry_new ();
  gtk_widget_set_usize (text_tool->border_text, 75, 25);
  gtk_entry_set_text (GTK_ENTRY (text_tool->border_text), "0");
  gtk_container_add (GTK_CONTAINER (alignment), text_tool->border_text);
  gtk_widget_show (alignment);

  /* Create the action area */
  text_tool->ok_button = gtk_button_new_with_label("OK");
  gtk_signal_connect(GTK_OBJECT(text_tool->ok_button),
		     "clicked", GTK_SIGNAL_FUNC(text_ok_callback),
		     text_tool);
  gtk_widget_show(text_tool->ok_button);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG(text_tool)->action_area),
		      text_tool->ok_button, TRUE, TRUE, 0);

  text_tool->cancel_button = gtk_button_new_with_label("Cancel");
  gtk_signal_connect(GTK_OBJECT(text_tool->cancel_button),
		     "clicked", GTK_SIGNAL_FUNC(text_cancel_callback),
		     text_tool);
  gtk_widget_show(text_tool->cancel_button);
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(text_tool)->action_area),
		      text_tool->cancel_button, TRUE, TRUE, 0);
 
  /* Show the widgets */
  gtk_widget_show (menu_table);
  gtk_widget_show (text_tool->size_text);
  gtk_widget_show (border_label);
  gtk_widget_show (text_tool->border_text);
  gtk_widget_show (size_opt_menu);
  gtk_widget_show (text_hbox);
  gtk_widget_show (list_box);
  gtk_widget_show (right_vbox);
  gtk_widget_show (top_hbox);
  gtk_widget_show (text_tool->the_text);
  gtk_widget_show (text_tool->text_frame);
  gtk_widget_show (text_tool->main_vbox);

  /* Post initialization */
  gtk_option_menu_set_menu (GTK_OPTION_MENU (size_opt_menu), text_tool->size_menu);

  if (nfonts)
    text_load_font (text_tool);
}

/**
 * gnome_font_selector_new:
 *
 * Returns: a newly created font selector widget
 */
GtkWidget *
gnome_font_selector_new(void)
{
  return gtk_type_new(gnome_font_selector_get_type());
}

static void
text_resize_text_widget (GnomeFontSelector *text_tool)
{
  GtkStyle *style;

  style = gtk_style_new ();
  gdk_font_unref (style->font);
  style->font = text_tool->font;
  gdk_font_ref (style->font);

  gtk_widget_set_style (text_tool->the_text, style);
  gtk_style_unref(style);
}

static void
text_ok_callback (GtkWidget *w,
		  gpointer   client_data)
{
  GnomeFontSelector * text_tool;

  text_tool = GNOME_FONT_SELECTOR(client_data);

  if (GTK_WIDGET_VISIBLE (text_tool))
    gtk_widget_hide (GTK_WIDGET(text_tool));

}

static gint
text_delete_callback (GtkWidget *w,
		      GdkEvent  *e,
		      gpointer   client_data)
{
  text_cancel_callback (w, client_data);
  
  return FALSE;
}

static void
text_cancel_callback (GtkWidget *w,
		      gpointer   client_data)
{
  GnomeFontSelector * text_tool;

  text_tool = GNOME_FONT_SELECTOR(client_data);

  if (GTK_WIDGET_VISIBLE (text_tool))
    gtk_widget_hide (GTK_WIDGET(text_tool));
}

static void
text_font_item_update (GtkWidget *w,
		       gpointer   data)
{
  FontInfo *font;
  int old_index;
  int i, index;
  GnomeFontSelector *the_text_tool;
  GnomeFontSelectorClass *klass;
  FontInfo **font_info;

  /*  Is this font being selected?  */
  if (w->state != GTK_STATE_SELECTED)
    return;

  the_text_tool = GNOME_FONT_SELECTOR(gtk_object_get_data(GTK_OBJECT(w),
							  "GnomeFontSelector"));
  klass = GNOME_FONT_SELECTOR_CLASS(GTK_OBJECT(the_text_tool)->klass);
  font_info = klass->font_info;

  index = (long) data;

  old_index = the_text_tool->font_index;
  the_text_tool->font_index = index;

  if (!text_load_font (the_text_tool))
    {
      the_text_tool->font_index = old_index;
      return;
    }

  font = font_info[the_text_tool->font_index];

  if (the_text_tool->foundry && !font->foundries[the_text_tool->foundry])
    {
      the_text_tool->foundry = 0;
      gtk_option_menu_set_history (GTK_OPTION_MENU (the_text_tool->option_menus[0]), 0);
    }
  if (the_text_tool->weight && !font->weights[the_text_tool->weight])
    {
      the_text_tool->weight = 0;
      gtk_option_menu_set_history (GTK_OPTION_MENU (the_text_tool->option_menus[1]), 0);
    }
  if (the_text_tool->slant && !font->slants[the_text_tool->slant])
    {
      the_text_tool->slant = 0;
      gtk_option_menu_set_history (GTK_OPTION_MENU (the_text_tool->option_menus[2]), 0);
    }
  if (the_text_tool->set_width && !font->set_widths[the_text_tool->set_width])
    {
      the_text_tool->set_width = 0;
      gtk_option_menu_set_history (GTK_OPTION_MENU (the_text_tool->option_menus[3]), 0);
    }
  if (the_text_tool->spacing && !font->spacings[the_text_tool->spacing])
    {
      the_text_tool->spacing = 0;
      gtk_option_menu_set_history (GTK_OPTION_MENU (the_text_tool->option_menus[4]), 0);
    }

  for (i = 0; i < klass->nfoundries; i++)
    gtk_widget_set_sensitive (the_text_tool->foundry_items[i], font->foundries[i]);
  for (i = 0; i < klass->nweights; i++)
    gtk_widget_set_sensitive (the_text_tool->weight_items[i], font->weights[i]);
  for (i = 0; i < klass->nslants; i++)
    gtk_widget_set_sensitive (the_text_tool->slant_items[i], font->slants[i]);
  for (i = 0; i < klass->nset_widths; i++)
    gtk_widget_set_sensitive (the_text_tool->set_width_items[i], font->set_widths[i]);
  for (i = 0; i < klass->nspacings; i++)
    gtk_widget_set_sensitive (the_text_tool->spacing_items[i], font->spacings[i]);
}

static void
text_pixels_callback (GtkWidget *w,
		      gpointer   client_data)
{
  GnomeFontSelector *text_tool;
  int old_value;

  text_tool = GNOME_FONT_SELECTOR(client_data);

  old_value = text_tool->size_type;
  text_tool->size_type = PIXELS;

  if (!text_load_font (text_tool))
    text_tool->size_type = old_value;
}

#if 0
/* Commented out until used.  Is this useful?  */
static void
text_points_callback (GtkWidget *w,
		      gpointer   client_data)
{
  GnomeFontSelector *text_tool;
  int old_value;

  text_tool = GNOME_FONT_SELECTOR(client_data);

  old_value = text_tool->size_type;
  text_tool->size_type = POINTS;

  if (!text_load_font (text_tool))
    text_tool->size_type = old_value;
}
#endif

static void
text_foundry_callback (GtkWidget *w,
		       gpointer   client_data)
{
  int old_value;
  GnomeFontSelector *the_text_tool;

  the_text_tool = GNOME_FONT_SELECTOR(gtk_object_get_data(GTK_OBJECT(w),
							  "GnomeFontSelector"));
  old_value = the_text_tool->foundry;
  the_text_tool->foundry = (long) client_data;
  text_validate_combo (the_text_tool, 0);

  if (!text_load_font (the_text_tool))
    the_text_tool->foundry = old_value;
}

static void
text_weight_callback (GtkWidget *w,
		      gpointer   client_data)
{
  int old_value;
  GnomeFontSelector *the_text_tool;

  the_text_tool = GNOME_FONT_SELECTOR(gtk_object_get_data(GTK_OBJECT(w),
							  "GnomeFontSelector"));

  old_value = the_text_tool->weight;
  the_text_tool->weight = (long) client_data;
  text_validate_combo (the_text_tool, 1);

  if (!text_load_font (the_text_tool))
    the_text_tool->weight = old_value;
}

static void
text_slant_callback (GtkWidget *w,
		     gpointer   client_data)
{
  int old_value;
  GnomeFontSelector *the_text_tool;

  the_text_tool = GNOME_FONT_SELECTOR(gtk_object_get_data(GTK_OBJECT(w),
							  "GnomeFontSelector"));

  old_value = the_text_tool->slant;
  the_text_tool->slant = (long) client_data;
  text_validate_combo (the_text_tool, 2);

  if (!text_load_font (the_text_tool))
    the_text_tool->slant = old_value;
}

static void
text_set_width_callback (GtkWidget *w,
			 gpointer   client_data)
{
  int old_value;
  GnomeFontSelector *the_text_tool;

  the_text_tool = GNOME_FONT_SELECTOR(gtk_object_get_data(GTK_OBJECT(w),
							  "GnomeFontSelector"));

  old_value = the_text_tool->set_width;
  the_text_tool->set_width = (long) client_data;
  text_validate_combo (the_text_tool, 3);

  if (!text_load_font (the_text_tool))
    the_text_tool->set_width = old_value;
}

static void
text_spacing_callback (GtkWidget *w,
		       gpointer   client_data)
{
  int old_value;
  GnomeFontSelector *the_text_tool;

  the_text_tool = GNOME_FONT_SELECTOR(gtk_object_get_data(GTK_OBJECT(w),
							  "GnomeFontSelector"));

  old_value = the_text_tool->spacing;
  the_text_tool->spacing = (long) client_data;
  text_validate_combo (the_text_tool, 4);

  if (!text_load_font (the_text_tool))
    the_text_tool->spacing = old_value;
}

static gint
text_size_key_function (GtkWidget   *w,
			GdkEventKey *event,
			gpointer     data)
{
  GnomeFontSelector *text_tool;
  gchar buffer[16];
  gint old_value;

  text_tool = (GnomeFontSelector *) data;

  if (event->keyval == GDK_Return)
    {
      gtk_signal_emit_stop_by_name (GTK_OBJECT (w), "key_press_event");

      old_value = atoi (gtk_entry_get_text (GTK_ENTRY (text_tool->size_text)));
      if (!text_load_font (text_tool))
	{
	  g_snprintf (buffer, sizeof(buffer), "%d", old_value);
	  gtk_entry_set_text (GTK_ENTRY (text_tool->size_text), buffer);
	}
      return TRUE;
    }

  return FALSE;
}

static void
text_validate_combo (GnomeFontSelector *text_tool,
		     gint       which)
{
  FontInfo *font;
  int which_val;
  int new_combo[5];
  int best_combo[5];
  int best_matches;
  int matches;
  int i;
  GnomeFontSelectorClass *klass;
  FontInfo **font_info;

  klass = GNOME_FONT_SELECTOR_CLASS(GTK_OBJECT(text_tool)->klass);
  font_info = klass->font_info;

  font = font_info[text_tool->font_index];

  switch (which)
    {
    case 0:
      which_val = text_tool->foundry;
      break;
    case 1:
      which_val = text_tool->weight;
      break;
    case 2:
      which_val = text_tool->slant;
      break;
    case 3:
      which_val = text_tool->set_width;
      break;
    case 4:
      which_val = text_tool->spacing;
      break;
    default:
      which_val = 0;
      break;
    }

  best_matches = -1;
  best_combo[0] = 0;
  best_combo[1] = 0;
  best_combo[2] = 0;
  best_combo[3] = 0;
  best_combo[4] = 0;

  for (i = 0; i < font->ncombos; i++)
    {
      /* we must match the which field */
      if (font->combos[i][which] == which_val)
	{
	  matches = 0;
	  new_combo[0] = 0;
	  new_combo[1] = 0;
	  new_combo[2] = 0;
	  new_combo[3] = 0;
	  new_combo[4] = 0;

	  if ((text_tool->foundry == 0) || (text_tool->foundry == font->combos[i][0]))
	    {
	      matches++;
	      if (text_tool->foundry)
		new_combo[0] = font->combos[i][0];
	    }
	  if ((text_tool->weight == 0) || (text_tool->weight == font->combos[i][1]))
	    {
	      matches++;
	      if (text_tool->weight)
		new_combo[1] = font->combos[i][1];
	    }
	  if ((text_tool->slant == 0) || (text_tool->slant == font->combos[i][2]))
	    {
	      matches++;
	      if (text_tool->slant)
		new_combo[2] = font->combos[i][2];
	    }
	  if ((text_tool->set_width == 0) || (text_tool->set_width == font->combos[i][3]))
	    {
	      matches++;
	      if (text_tool->set_width)
		new_combo[3] = font->combos[i][3];
	    }
	  if ((text_tool->spacing == 0) || (text_tool->spacing == font->combos[i][4]))
	    {
	      matches++;
	      if (text_tool->spacing)
		new_combo[4] = font->combos[i][4];
	    }

	  /* if we get all 5 matches simply return */
	  if (matches == 5)
	    return;

	  if (matches > best_matches)
	    {
	      best_matches = matches;
	      best_combo[0] = new_combo[0];
	      best_combo[1] = new_combo[1];
	      best_combo[2] = new_combo[2];
	      best_combo[3] = new_combo[3];
	      best_combo[4] = new_combo[4];
	    }
	}
    }

  if (best_matches > -1)
    {
      if (text_tool->foundry != best_combo[0])
	{
	  text_tool->foundry = best_combo[0];
	  if (which != 0)
	    gtk_option_menu_set_history (GTK_OPTION_MENU (text_tool->option_menus[0]), text_tool->foundry);
	}
      if (text_tool->weight != best_combo[1])
	{
	  text_tool->weight = best_combo[1];
	  if (which != 1)
	    gtk_option_menu_set_history (GTK_OPTION_MENU (text_tool->option_menus[1]), text_tool->weight);
	}
      if (text_tool->slant != best_combo[2])
	{
	  text_tool->slant = best_combo[2];
	  if (which != 2)
	    gtk_option_menu_set_history (GTK_OPTION_MENU (text_tool->option_menus[2]), text_tool->slant);
	}
      if (text_tool->set_width != best_combo[3])
	{
	  text_tool->set_width = best_combo[3];
	  if (which != 3)
	    gtk_option_menu_set_history (GTK_OPTION_MENU (text_tool->option_menus[3]), text_tool->set_width);
	}
      if (text_tool->spacing != best_combo[4])
	{
	  text_tool->spacing = best_combo[4];
	  if (which != 4)
	    gtk_option_menu_set_history (GTK_OPTION_MENU (text_tool->option_menus[4]), text_tool->spacing);
	}
    }
}

static void
text_get_fonts (GnomeFontSelectorClass *klass)
{
  gchar **fontnames;
  gchar *fontname;
  gchar *field;
  GSList * temp_list;
  gint num_fonts;
  gint index;
  gint i, j;

  if(klass->font_info)
    return;

  /* construct a valid font pattern */

  fontnames = XListFonts (GDK_DISPLAY(), "-*-*-*-*-*-*-0-0-75-75-*-0-*-*", 32767, &num_fonts);

  /* the maximum size of the table is the number of font names returned */
  klass->font_info = g_malloc (sizeof (FontInfo**) * num_fonts);

  /* insert the fontnames into a table */
  klass->nfonts = 0;
  for (i = 0; i < num_fonts; i++)
    if (text_is_xlfd_font_name (fontnames[i]))
      {
	text_insert_font (klass->font_info, &klass->nfonts, fontnames[i]);

	klass->foundries = text_insert_field (klass->foundries, fontnames[i], FOUNDRY);
	klass->weights = text_insert_field (klass->weights, fontnames[i], WEIGHT);
	klass->slants = text_insert_field (klass->slants, fontnames[i], SLANT);
	klass->set_widths = text_insert_field (klass->set_widths, fontnames[i], SET_WIDTH);
	klass->spacings = text_insert_field (klass->spacings, fontnames[i], SPACING);
      }

  XFreeFontNames (fontnames);
#define list_length g_slist_length
  klass->nfoundries = list_length (klass->foundries) + 1;
  klass->nweights = list_length (klass->weights) + 1;
  klass->nslants = list_length (klass->slants) + 1;
  klass->nset_widths = list_length (klass->set_widths) + 1;
  klass->nspacings = list_length (klass->spacings) + 1;

  klass->foundry_array = g_malloc (sizeof (char*) * klass->nfoundries);
  klass->weight_array = g_malloc (sizeof (char*) * klass->nweights);
  klass->slant_array = g_malloc (sizeof (char*) * klass->nslants);
  klass->set_width_array = g_malloc (sizeof (char*) * klass->nset_widths);
  klass->spacing_array = g_malloc (sizeof (char*) * klass->nspacings);

  i = 1;
  temp_list = klass->foundries;
  while (temp_list)
    {
      klass->foundry_array[i++] = temp_list->data;
      temp_list = temp_list->next;
    }

  i = 1;
  temp_list = klass->weights;
  while (temp_list)
    {
      klass->weight_array[i++] = temp_list->data;
      temp_list = temp_list->next;
    }

  i = 1;
  temp_list = klass->slants;
  while (temp_list)
    {
      klass->slant_array[i++] = temp_list->data;
      temp_list = temp_list->next;
    }

  i = 1;
  temp_list = klass->set_widths;
  while (temp_list)
    {
      klass->set_width_array[i++] = temp_list->data;
      temp_list = temp_list->next;
    }

  i = 1;
  temp_list = klass->spacings;
  while (temp_list)
    {
      klass->spacing_array[i++] = temp_list->data;
      temp_list = temp_list->next;
    }

  klass->foundry_array[0] = "*";
  klass->weight_array[0] = "*";
  klass->slant_array[0] = "*";
  klass->set_width_array[0] = "*";
  klass->spacing_array[0] = "*";

  for (i = 0; i < klass->nfonts; i++)
    {
      klass->font_info[i]->foundries = g_malloc (sizeof (int) * klass->nfoundries);
      klass->font_info[i]->weights = g_malloc (sizeof (int) * klass->nweights);
      klass->font_info[i]->slants = g_malloc (sizeof (int) * klass->nslants);
      klass->font_info[i]->set_widths = g_malloc (sizeof (int) * klass->nset_widths);
      klass->font_info[i]->spacings = g_malloc (sizeof (int) * klass->nspacings);
      klass->font_info[i]->ncombos = list_length (klass->font_info[i]->fontnames);
      klass->font_info[i]->combos = g_malloc (sizeof (int*) * klass->font_info[i]->ncombos);

      for (j = 0; j < klass->nfoundries; j++)
	klass->font_info[i]->foundries[j] = 0;
      for (j = 0; j < klass->nweights; j++)
	klass->font_info[i]->weights[j] = 0;
      for (j = 0; j < klass->nslants; j++)
	klass->font_info[i]->slants[j] = 0;
      for (j = 0; j < klass->nset_widths; j++)
	klass->font_info[i]->set_widths[j] = 0;
      for (j = 0; j < klass->nspacings; j++)
	klass->font_info[i]->spacings[j] = 0;

      klass->font_info[i]->foundries[0] = 1;
      klass->font_info[i]->weights[0] = 1;
      klass->font_info[i]->slants[0] = 1;
      klass->font_info[i]->set_widths[0] = 1;
      klass->font_info[i]->spacings[0] = 1;

      j = 0;
      temp_list = klass->font_info[i]->fontnames;
      while (temp_list)
	{
	  fontname = temp_list->data;
	  temp_list = temp_list->next;

	  klass->font_info[i]->combos[j] = g_malloc (sizeof (int) * 5);

	  field = text_get_field (fontname, FOUNDRY);
	  index = text_field_to_index (klass->foundry_array, klass->nfoundries, field);
	  klass->font_info[i]->foundries[index] = 1;
	  klass->font_info[i]->combos[j][0] = index;
	  g_free (field);

	  field = text_get_field (fontname, WEIGHT);
	  index = text_field_to_index (klass->weight_array, klass->nweights, field);
	  klass->font_info[i]->weights[index] = 1;
	  klass->font_info[i]->combos[j][1] = index;
	  g_free (field);

	  field = text_get_field (fontname, SLANT);
	  index = text_field_to_index (klass->slant_array, klass->nslants, field);
	  klass->font_info[i]->slants[index] = 1;
	  klass->font_info[i]->combos[j][2] = index;
	  g_free (field);

	  field = text_get_field (fontname, SET_WIDTH);
	  index = text_field_to_index (klass->set_width_array, klass->nset_widths, field);
	  klass->font_info[i]->set_widths[index] = 1;
	  klass->font_info[i]->combos[j][3] = index;
	  g_free (field);

	  field = text_get_field (fontname, SPACING);
	  index = text_field_to_index (klass->spacing_array, klass->nspacings, field);
	  klass->font_info[i]->spacings[index] = 1;
	  klass->font_info[i]->combos[j][4] = index;
	  g_free (field);

	  j += 1;
	}
    }
}

static void
text_insert_font (FontInfo    **table,
		  gint        *ntable,
		  const gchar *fontname)
{
  FontInfo *temp_info;
  gchar *family;
  gint lower, upper;
  gint middle, cmp;

  /* insert a fontname into a table */
  family = text_get_field (fontname, FAMILY);
  if (!family)
    return;

  lower = 0;
  if (*ntable > 0)
    {
      /* Do a binary search to determine if we have already encountered
       *  a font from this family.
       */
      upper = *ntable;
      while (lower < upper)
	{
	  middle = (lower + upper) >> 1;

	  cmp = strcmp (family, table[middle]->family);
	  if (cmp == 0)
	    {
	      table[middle]->fontnames = g_slist_prepend (table[middle]->fontnames, g_strdup (fontname));
	      return;
	    }
	  else if (cmp < 0)
	    upper = middle;
	  else if (cmp > 0)
	    lower = middle+1;
	}
    }

  /* Add another entry to the table for this new font family */
  table[*ntable] = g_malloc (sizeof (FontInfo));
  table[*ntable]->family = family;
  table[*ntable]->foundries = NULL;
  table[*ntable]->weights = NULL;
  table[*ntable]->slants = NULL;
  table[*ntable]->set_widths = NULL;
  table[*ntable]->fontnames = NULL;
  table[*ntable]->fontnames = g_slist_prepend (table[*ntable]->fontnames, g_strdup (fontname));
  (*ntable)++;

  /* Quickly insert the entry into the table in sorted order
   *  using a modification of insertion sort and the knowledge
   *  that the entries proper position in the table was determined
   *  above in the binary search and is contained in the "lower"
   *  variable.
   */
  if (*ntable > 1)
    {
      temp_info = table[*ntable - 1];

      upper = *ntable - 1;
      while (lower != upper)
	{
	  table[upper] = table[upper-1];
	  upper -= 1;
	}

      table[lower] = temp_info;
    }
}

static GSList *
text_insert_field (GSList      *list,
		   const gchar *fontname,
		   gint        field_num)
{
  GSList *temp_list;
  GSList *prev_list;
  GSList *new_list;
  gchar *field;
  gint cmp;

  field = text_get_field (fontname, field_num);
  if (!field)
    return list;

  temp_list = list;
  prev_list = NULL;

  while (temp_list)
    {
      cmp = strcmp (field, (gchar*) temp_list->data);
      if (cmp == 0)
	{
	  g_free (field);
	  return list;
	}
      else if (cmp < 0)
	{
	  new_list = g_slist_alloc ();
	  new_list->data = field;
	  new_list->next = temp_list;
	  if (prev_list)
	    {
	      prev_list->next = new_list;
	      return list;
	    }
	  else
	    return new_list;
	}
      else
	{
	  prev_list = temp_list;
	  temp_list = temp_list->next;
	}
    }

  new_list = g_slist_alloc ();
  new_list->data = field;
  new_list->next = NULL;
  if (prev_list)
    {
      prev_list->next = new_list;
      return list;
    }
  else
    return new_list;
}

static gchar*
text_get_field (const gchar *fontname,
		gint   field_num)
{
  const gchar *t1, *t2;
  gchar *field;

  /* we assume this is a valid fontname...that is, it has 14 fields */

  t1 = fontname;
  while (*t1 && (field_num >= 0))
    if (*t1++ == '-')
      field_num--;

  t2 = t1;
  while (*t2 && (*t2 != '-'))
    t2++;

  if (t1 != t2)
    {
      field = g_malloc (1 + (long) t2 - (long) t1);
      strncpy (field, t1, (long) t2 - (long) t1);
      field[(long) t2 - (long) t1] = 0;
      return field;
    }

  return g_strdup ("(nil)");
}

static int
text_field_to_index (gchar **table,
		     gint  ntable,
		     const gchar  *field)
{
  gint i;

  for (i = 0; i < ntable; i++)
    if (strcmp (field, table[i]) == 0)
      return i;

  return -1;
}

static gint
text_is_xlfd_font_name (const gchar *fontname)
{
  gint i;

  i = 0;
  while (*fontname)
    if (*fontname++ == '-')
      i++;

  return (i == 14);
}

static gchar *
text_get_xlfd (gdouble     size,
	       gint        size_type,
	       const gchar *foundry,
	       const gchar *family,
	       const gchar *weight,
	       const gchar *slant,
	       const gchar *set_width,
	       const gchar *spacing)
{
  gchar pixel_size[12], point_size[12];

  if (size > 0)
    {
      switch (size_type)
	{
	case PIXELS:
	  g_snprintf (pixel_size, sizeof(pixel_size), "%d", (int) size);
	  g_snprintf (point_size, sizeof(point_size), "*");
	  break;
	case POINTS:
	  g_snprintf (pixel_size, sizeof(pixel_size), "*");
	  g_snprintf (point_size, sizeof(point_size), "%d", (int) (size * 10));
	  break;
	}

      return g_strconcat("-", foundry,
			    "-", family,
			    "-", weight,
			    "-", slant,
			    "-", set_width,
			    "-*-", pixel_size,
			      "-", point_size,
			    "-75-75-", spacing,
			    "-*-*-*", NULL);
    }
  else
    return NULL;
}

static gint
text_load_font (GnomeFontSelector *text_tool)
{
  GdkFont *font;
  gchar *fontname;
  gdouble size;
  gchar *size_text;
  gchar *foundry_str;
  gchar *family_str;
  gchar *weight_str;
  gchar *slant_str;
  gchar *set_width_str;
  gchar *spacing_str;
  GnomeFontSelectorClass *klass;

  klass = GNOME_FONT_SELECTOR_CLASS(GTK_OBJECT(text_tool)->klass);

  size_text = gtk_entry_get_text (GTK_ENTRY (text_tool->size_text));
  size = atof (size_text);

  foundry_str = klass->foundry_array[text_tool->foundry];
  if (strcmp (foundry_str, "(nil)") == 0)
    foundry_str = "";
  family_str = klass->font_info[text_tool->font_index]->family;
  weight_str = klass->weight_array[text_tool->weight];
  if (strcmp (weight_str, "(nil)") == 0)
    weight_str = "";
  slant_str = klass->slant_array[text_tool->slant];
  if (strcmp (slant_str, "(nil)") == 0)
    slant_str = "";
  set_width_str = klass->set_width_array[text_tool->set_width];
  if (strcmp (set_width_str, "(nil)") == 0)
    set_width_str = "";
  spacing_str = klass->spacing_array[text_tool->spacing];
  if (strcmp (spacing_str, "(nil)") == 0)
    spacing_str = "";

  if ((fontname = text_get_xlfd (size, text_tool->size_type, foundry_str, family_str,
		     weight_str, slant_str, set_width_str, spacing_str)))
    {
      font = gdk_font_load (fontname);
      g_free(fontname);
      if (font)
	{
	  if (text_tool->font)
	    gdk_font_unref (text_tool->font);
	  text_tool->font = font;
	  text_resize_text_widget (text_tool);
	  return TRUE;
	}
    }

  return FALSE;
}


/**
 * gnome_font_selector_get_selected:
 * @text_tool: a font selector
 *
 * Returns the name of the font currently selected.  The value returned
 * is allocated with g_malloc.  The font name is in the format expected
 * by Gdk.
 */
gchar *
gnome_font_selector_get_selected (GnomeFontSelector *text_tool)
{
  gdouble size;
  gchar *size_text;
  gchar *foundry_str;
  gchar *family_str;
  gchar *weight_str;
  gchar *slant_str;
  gchar *set_width_str;
  gchar *spacing_str;
  GnomeFontSelectorClass *klass;

  klass = GNOME_FONT_SELECTOR_CLASS(GTK_OBJECT(text_tool)->klass);

  size_text = gtk_entry_get_text (GTK_ENTRY (text_tool->size_text));
  size = atof (size_text);

  foundry_str = klass->foundry_array[text_tool->foundry];
  if (strcmp (foundry_str, "(nil)") == 0)
    foundry_str = "";
  family_str = klass->font_info[text_tool->font_index]->family;
  weight_str = klass->weight_array[text_tool->weight];
  if (strcmp (weight_str, "(nil)") == 0)
    weight_str = "";
  slant_str = klass->slant_array[text_tool->slant];
  if (strcmp (slant_str, "(nil)") == 0)
    slant_str = "";
  set_width_str = klass->set_width_array[text_tool->set_width];
  if (strcmp (set_width_str, "(nil)") == 0)
    set_width_str = "";
  spacing_str = klass->spacing_array[text_tool->spacing];
  if (strcmp (spacing_str, "(nil)") == 0)
    spacing_str = "";

  return text_get_xlfd (size, text_tool->size_type, foundry_str,
			family_str, weight_str, slant_str,
			set_width_str, spacing_str);
}

static void
gnome_font_select_quit(GtkWidget *widget,
		       gpointer user_data)
{
  gtk_main_quit();
  gtk_object_set_data(GTK_OBJECT(user_data),
		      "gnome_font_select_quit widget",
		      widget);
}

/**
 * gnome_font_select:
 *
 * Pops up a font selector and lets the user choose the font.
 *
 * Returns: A g_malloc()ed string with the X font name that was selected
 */
gchar *
gnome_font_select(void)
{
  GnomeFontSelector *font_sel;
  gchar *retval = NULL;
  font_sel = GNOME_FONT_SELECTOR(gnome_font_selector_new());

  gtk_signal_connect(GTK_OBJECT(font_sel->ok_button),
		     "clicked", GTK_SIGNAL_FUNC(gnome_font_select_quit),
		     font_sel);

  gtk_signal_connect(GTK_OBJECT(font_sel->cancel_button),
		     "clicked", GTK_SIGNAL_FUNC(gnome_font_select_quit),
		     font_sel);

  gtk_signal_connect(GTK_OBJECT(font_sel),
		     "delete_event",
		     GTK_SIGNAL_FUNC(gnome_font_select_quit),
		     font_sel);

  gtk_widget_show(GTK_WIDGET(font_sel));
  gtk_grab_add(GTK_WIDGET(font_sel));
  gtk_main();
  gtk_widget_hide(GTK_WIDGET(font_sel));
  gtk_grab_remove(GTK_WIDGET(font_sel));

  if(gtk_object_get_data(GTK_OBJECT(font_sel),
			 "gnome_font_select_quit widget")
     == font_sel->ok_button)
    retval = gnome_font_selector_get_selected(font_sel);

  gtk_widget_destroy(GTK_WIDGET(font_sel));

  return retval;
}

/**
 * gnome_font_select:
 * @default_font: The font chosen by default.
 *
 * Pops up a font selector and lets the user choose the font.  
 *
 * Returns: A g_malloc()ed string with the X font name that was selected
 */
gchar *gnome_font_select_with_default(const gchar *default_font)
{
  GnomeFontSelector *text_tool;
  gchar *retval = NULL;
  text_tool = GNOME_FONT_SELECTOR(gnome_font_selector_new());

  /* If it is a valid font name ... */
  if (default_font && text_is_xlfd_font_name (default_font)) {
    char *foundry, *family, *weight, *slant, *set_width, *spacing;
    char *pixel_size, *point_size;
    int old_value, old_size, i;

    GnomeFontSelectorClass *klass =
      GNOME_FONT_SELECTOR_CLASS (GTK_OBJECT (text_tool)->klass);

    foundry	= text_get_field (default_font, FOUNDRY);
    family	= text_get_field (default_font, FAMILY);
    weight	= text_get_field (default_font, WEIGHT);
    slant	= text_get_field (default_font, SLANT);
    set_width	= text_get_field (default_font, SET_WIDTH);
    pixel_size	= text_get_field (default_font, 6);
    point_size	= text_get_field (default_font, 7);
    spacing	= text_get_field (default_font, SPACING);

    /* set family. */

    old_value	= text_tool->font_index;
    
    for (i = 0; i < klass->nfonts; i++)
      if (!strcmp (family, klass->font_info[i]->family)) {
	text_tool->font_index = i;
	break;
      }

    if (!text_load_font (text_tool))
      text_tool->font_index = old_value;

    gtk_list_select_item (GTK_LIST (text_tool->font_list), text_tool->font_index);

    /* set foundry. */

    old_value	= text_tool->foundry;

    text_tool->foundry = text_field_to_index
      (klass->foundry_array, klass->nfoundries, foundry);
    
    text_validate_combo (text_tool, 0);

    if (!text_load_font (text_tool))
      text_tool->foundry = old_value;

    gtk_option_menu_set_history
      (GTK_OPTION_MENU (text_tool->option_menus [0]), text_tool->foundry);

    /* set weight. */

    old_value	= text_tool->weight;

    text_tool->weight = text_field_to_index
      (klass->weight_array, klass->nweights, weight);

    text_validate_combo (text_tool, 1);

    if (!text_load_font (text_tool))
      text_tool->weight = old_value;

    gtk_option_menu_set_history
      (GTK_OPTION_MENU (text_tool->option_menus [1]), text_tool->weight);

    /* set slant. */

    old_value	= text_tool->slant;

    text_tool->slant = text_field_to_index
      (klass->slant_array, klass->nslants, slant);

    text_validate_combo (text_tool, 2);

    if (!text_load_font (text_tool))
      text_tool->slant = old_value;

    gtk_option_menu_set_history
      (GTK_OPTION_MENU (text_tool->option_menus [2]), text_tool->slant);

    /* set width. */

    old_value	= text_tool->set_width;

    text_tool->set_width = text_field_to_index
      (klass->set_width_array, klass->nset_widths, set_width);

    text_validate_combo (text_tool, 3);

    if (!text_load_font (text_tool))
      text_tool->set_width = old_value;

    gtk_option_menu_set_history
      (GTK_OPTION_MENU (text_tool->option_menus [3]), text_tool->set_width);

    /* set spacing. */

    old_value	= text_tool->spacing;

    text_tool->spacing = text_field_to_index
      (klass->spacing_array, klass->nspacings, spacing);

    text_validate_combo (text_tool, 4);

    if (!text_load_font (text_tool))
      text_tool->spacing = old_value;

    gtk_option_menu_set_history
      (GTK_OPTION_MENU (text_tool->option_menus [4]), text_tool->spacing);

    /* first we try to set the pixel size and then the point size. */

    old_value	= text_tool->size_type;
    old_size	= atoi (gtk_entry_get_text
			(GTK_ENTRY (text_tool->size_text)));

    if (strcmp (pixel_size, "*") && strcmp (pixel_size, "(nil)")) {
      text_tool->size_type = PIXELS;
      
      gtk_entry_set_text (GTK_ENTRY (text_tool->size_text), pixel_size);
      
      if (!text_load_font (text_tool)) {
	char buffer [BUFSIZ];
	
	text_tool->size_type = old_value;
	
	g_snprintf (buffer, sizeof(buffer), "%d", old_size);
	gtk_entry_set_text (GTK_ENTRY (text_tool->size_text), buffer);
      }
    }
  }

  gtk_signal_connect(GTK_OBJECT(text_tool->ok_button),
		     "clicked", GTK_SIGNAL_FUNC(gnome_font_select_quit),
		     text_tool);

  gtk_signal_connect(GTK_OBJECT(text_tool->cancel_button),
		     "clicked", GTK_SIGNAL_FUNC(gnome_font_select_quit),
		     text_tool);

  gtk_signal_connect(GTK_OBJECT(text_tool),
		     "delete_event",
		     GTK_SIGNAL_FUNC(gnome_font_select_quit),
		     text_tool);

  gtk_widget_show(GTK_WIDGET(text_tool));
  gtk_grab_add(GTK_WIDGET(text_tool));
  gtk_main();
  gtk_widget_hide(GTK_WIDGET(text_tool));
  gtk_grab_remove(GTK_WIDGET(text_tool));

  if(gtk_object_get_data(GTK_OBJECT(text_tool),
			 "gnome_font_select_quit widget")
     == text_tool->ok_button)
    retval = gnome_font_selector_get_selected(text_tool);

  gtk_widget_destroy(GTK_WIDGET(text_tool));

  return retval;
}
