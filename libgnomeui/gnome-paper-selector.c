/* GnomeSelectPaper widget
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Authors: Dirk Luetjens <dirk@luedi.oche.de>
 * Seth Alves <alves@hungry.com>
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
 * You should have received a copy of the GNU Library General
 * Public License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA
 * 02139, USA.
 *              
 */

#include <config.h>

#include <glib.h>
#include "libgnome/libgnomeP.h"
#include "gnome-paper-selector.h"

static void gnome_paper_selector_class_init (GnomePaperSelectorClass *class);
static void gnome_paper_selector_init       (GnomePaperSelector      *gspaper);

static GtkVBoxClass *parent_class;

guint
gnome_paper_selector_get_type (void)
{
  static guint select_paper_type = 0;
  
  if (!select_paper_type) {
    GtkTypeInfo select_paper_info = {
      "GnomePaperSelector",
      sizeof (GnomePaperSelector),
      sizeof (GnomePaperSelectorClass),
      (GtkClassInitFunc) gnome_paper_selector_class_init,
      (GtkObjectInitFunc) gnome_paper_selector_init,
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL
    };
    
    select_paper_type = gtk_type_unique (gtk_vbox_get_type (), &select_paper_info);
  }
  
  return select_paper_type;
}

static void
gnome_paper_selector_class_init (GnomePaperSelectorClass *class)
{
  GtkObjectClass *object_class;
  
  object_class = (GtkObjectClass *) class;
  parent_class = gtk_type_class (gtk_vbox_get_type ());

  paperinit();
}

static void 
paper_size_changed (GtkWidget *widget, gpointer data)
{
  GnomePaperSelector *gspaper;
  gfloat paper_width, paper_height;
  const struct paper *paper;

  gspaper = data;

  paper_width  = gtk_spin_button_get_value_as_float
	  (GTK_SPIN_BUTTON(gspaper->width));
  paper_height = gtk_spin_button_get_value_as_float
	  (GTK_SPIN_BUTTON(gspaper->height));
  
  paper = paperwithsize ((double)paper_width, (double)paper_height);
  
  gtk_signal_handler_block (GTK_OBJECT(GTK_COMBO(gspaper->paper)->entry), 
		  gspaper->paper_id);
  if (paper) {
    const char* paper_name = papername (paper);
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(gspaper->paper)->entry), paper_name); 
  }
  else {
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(gspaper->paper)->entry), "custom"); 
  }
  gtk_signal_handler_unblock (GTK_OBJECT(GTK_COMBO(gspaper->paper)->entry), 
		  gspaper->paper_id);
    
#ifdef DEBUG
  printf ("Paper Size changed to %f, %f\n", paper_width, paper_height);
#endif
}

static void 
unit_changed (GtkWidget *widget, gpointer data)
{
  GnomePaperSelector *gspaper;
  gchar *unit_name;
  
  gspaper = data;

  unit_name = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(gspaper->unit)->entry));
  gtk_label_set (GTK_LABEL(gspaper->unit_label), unit_name);
#ifdef DEBUG
  printf ("Unit changed to %s\n", unit_name);
#endif
}

static void set_widgets_from_paper (GnomePaperSelector *gspaper,
                                    const Paper *paper,
                                    const Unit *unit)
{
  const gchar *paper_name, *unit_name;
  double paper_width, paper_height;
  double margin_left, margin_top, margin_right, margin_bottom;
  GList *l;

  paper_name = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(gspaper->paper)->entry));
  unit_name = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(gspaper->unit)->entry));
  /*unit = gnome_unit_with_name (unit_name);*/

  paper_width = gnome_paper_convert (gnome_paper_pswidth (paper), unit);
  paper_height = gnome_paper_convert (gnome_paper_psheight (paper), unit);
  margin_left = gnome_paper_convert (gnome_paper_lmargin (paper), unit);
  margin_top = gnome_paper_convert (gnome_paper_tmargin (paper), unit);
  margin_right = gnome_paper_convert (gnome_paper_rmargin (paper), unit);
  margin_bottom=gnome_paper_convert (gnome_paper_bmargin (paper), unit);


  /***/
  gtk_signal_handler_block (GTK_OBJECT(gspaper->width), gspaper->width_id);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(gspaper->width),
                             (gfloat) paper_width);
  gtk_signal_handler_unblock (GTK_OBJECT(gspaper->width), gspaper->width_id);
  /***/
  gtk_signal_handler_block (GTK_OBJECT(gspaper->height), gspaper->height_id);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(gspaper->height),
                             (gfloat) paper_height);
  gtk_signal_handler_unblock (GTK_OBJECT(gspaper->height),
                              gspaper->height_id);
  /***/
  gtk_signal_handler_block (GTK_OBJECT(gspaper->lmargin), gspaper->lmargin_id);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (gspaper->lmargin),
                             (gfloat) margin_left);
  gtk_signal_handler_unblock (GTK_OBJECT(gspaper->lmargin),
                              gspaper->lmargin_id);
  /***/
  gtk_signal_handler_block (GTK_OBJECT(gspaper->tmargin), gspaper->tmargin_id);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (gspaper->tmargin),
                             (gfloat) margin_top);
  gtk_signal_handler_unblock (GTK_OBJECT(gspaper->tmargin),
                              gspaper->tmargin_id);
  /***/
  gtk_signal_handler_block (GTK_OBJECT(gspaper->rmargin), gspaper->rmargin_id);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (gspaper->rmargin),
                             (gfloat) margin_right);
  gtk_signal_handler_unblock (GTK_OBJECT(gspaper->rmargin),
                              gspaper->rmargin_id);
  /***/
  gtk_signal_handler_block (GTK_OBJECT(gspaper->bmargin), gspaper->bmargin_id);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (gspaper->bmargin),
                             (gfloat) margin_bottom);
  gtk_signal_handler_unblock (GTK_OBJECT(gspaper->bmargin),
                              gspaper->bmargin_id);


  /*#ifdef DEBUG*/
  printf ("Selected Paper: %s (%f, %f) "
          "margins: (%f, %f, %f, %f) "
          "units: %s\n",
          paper_name, paper_width, paper_height,
          margin_left, margin_top, margin_right, margin_bottom,
          unit_name);
  /*#endif*/
}


static void 
paper_size_system (GtkWidget *widget, gpointer data)
{
  GnomePaperSelector *gspaper;
  const struct paper *paper;
  gchar *paper_name;
  double paper_width, paper_height;

  gspaper = data;
  
#ifdef DEBUG
  printf ("env : %s\n", (char*)getenv("PAPERSIZE"));
#endif
  paper_name = systempapername();
  if (paper_name) {
    paper = paperinfo (paper_name);

    if (paper) {
      paper_width = paperpswidth (paper);
      paper_height = paperpsheight (paper);
      
      gtk_signal_handler_block (GTK_OBJECT(GTK_COMBO(gspaper->paper)->entry), 
		      gspaper->paper_id);
      gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(gspaper->paper)->entry), 
		      paper_name); 
      gtk_signal_handler_unblock(GTK_OBJECT(GTK_COMBO(gspaper->paper)->entry), 
		      gspaper->paper_id);

      gtk_signal_handler_block (GTK_OBJECT(gspaper->width), gspaper->width_id);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(gspaper->width), 
		      (gfloat) paper_width);
      gtk_signal_handler_unblock (GTK_OBJECT(gspaper->width), 
		      gspaper->width_id);
      
      gtk_signal_handler_block (GTK_OBJECT(gspaper->height), 
		      gspaper->height_id);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(gspaper->height), 
		      (gfloat) paper_height);
      gtk_signal_handler_unblock (GTK_OBJECT(gspaper->height), 
		      gspaper->height_id);
      
#ifdef DEBUG
      printf ("Selected Paper: %s (%f, %f)\n", paper_name, paper_width, paper_height);
#endif
    }
#ifdef DEBUG
    else 
      printf ("Paper %s not known to the system\n", paper_name);
#endif
  }
}

static void 
paper_changed (GtkWidget *widget, gpointer data)
{
  GnomePaperSelector *gspaper;
  const struct paper *paper;
  gchar *paper_name;
  double paper_width, paper_height;

  gspaper = data;
  
  paper_name = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(gspaper->paper)->entry));
  paper = paperinfo (paper_name);

  paper_width = paperpswidth (paper);
  paper_height = paperpsheight (paper);

  gtk_signal_handler_block (GTK_OBJECT(gspaper->width), gspaper->width_id);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(gspaper->width), 
		  (gfloat) paper_width);
  gtk_signal_handler_unblock (GTK_OBJECT(gspaper->width), gspaper->width_id);

  gtk_signal_handler_block (GTK_OBJECT(gspaper->height), gspaper->height_id);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(gspaper->height), 
		  (gfloat) paper_height);
  gtk_signal_handler_unblock (GTK_OBJECT(gspaper->height), gspaper->height_id);

#ifdef DBEUG
  printf ("Selected Paper: %s (%f, %f)\n", paper_name, paper_width, paper_height);
#endif
}

static void
gnome_paper_selector_init (GnomePaperSelector *gspaper)
{
  GtkWidget *table, *label, *button;
  GtkTooltips *tooltips;
  GtkAdjustment *adj;
  const Paper *paper;
  const Unit *unit;
  const char *paper_name, *unitname;
  gint i,j;

  tooltips = gtk_tooltips_new();
  gtk_tooltips_enable (tooltips);

   table = gtk_table_new (8, 3, FALSE);
   gtk_container_add (GTK_CONTAINER(gspaper), table);

    /* create the paper entry */
   label = gtk_label_new ("Paper:");
   gtk_widget_show (label);
   /*gtk_misc_set_alignment (GTK_MISC(label), 0.0, 0.5);*/
   gtk_table_attach_defaults (GTK_TABLE(table), label, 0, 1, 0, 1);

   gspaper->paper = gtk_combo_new();
   gtk_combo_set_popdown_strings (GTK_COMBO(gspaper->paper), gnome_paper_name_list());
   gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(gspaper->paper)->entry), FALSE);
   gspaper->paper_id = gtk_signal_connect (GTK_OBJECT(GTK_COMBO(gspaper->paper)->entry), "changed", 
   	(GtkSignalFunc) paper_changed, gspaper);
   gtk_table_attach_defaults (GTK_TABLE (table), gspaper->paper, 1, 3, 0, 1);
   gtk_widget_show (gspaper->paper);

   /* create the width entry */
   label = gtk_label_new ("Width:");
   gtk_widget_show (label);
   /*gtk_misc_set_alignment (GTK_MISC(label), 0.0, 0.5);*/
   gtk_table_attach_defaults (GTK_TABLE(table), label, 0, 1, 1, 2);

   adj = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, G_MAXFLOAT, 1.0, 5.0, 0.0);
   gspaper->width = gtk_spin_button_new(adj, 5.0, 1);
   gspaper->width_id = gtk_signal_connect (GTK_OBJECT(gspaper->width), "changed", 							                                          (GtkSignalFunc) paper_size_changed,
                                          gspaper);
  gtk_table_attach_defaults (GTK_TABLE (table), gspaper->width, 1, 2, 1, 2);
  gtk_widget_show (gspaper->width);

  /* create the height entry */
  label = gtk_label_new ("Height:");
  gtk_widget_show (label);
  /*gtk_misc_set_alignment (GTK_MISC(label), 0.0, 0.5);*/
  gtk_table_attach_defaults (GTK_TABLE(table), label, 0, 1, 2, 3);

  adj = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, G_MAXFLOAT, 1.0,
                                              5.0, 0.0);
  gspaper->height = gtk_spin_button_new(adj, 5.0, 1);
  gspaper->height_id = gtk_signal_connect (GTK_OBJECT(gspaper->height),
                                           "changed",
                                           (GtkSignalFunc) paper_size_changed,
                                           gspaper);
  gtk_table_attach_defaults (GTK_TABLE (table), gspaper->height, 1, 2, 2, 3);
  gtk_widget_show (gspaper->height);

  /* create the left margin entry */
  label = gtk_label_new ("Left Margin:");
  gtk_widget_show (label);
  /*gtk_misc_set_alignment (GTK_MISC(label), 0.0, 0.5);*/
  gtk_table_attach_defaults (GTK_TABLE(table), label, 0, 1, 3, 4);

  adj = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, G_MAXFLOAT, 1.0,
                                              5.0, 0.0);
  gspaper->lmargin = gtk_spin_button_new(adj, 5.0, 1);
  gspaper->lmargin_id = gtk_signal_connect (GTK_OBJECT(gspaper->lmargin),
                                            "changed",
                                            (GtkSignalFunc) paper_size_changed,
                                            gspaper);
  gtk_table_attach_defaults (GTK_TABLE (table), gspaper->lmargin, 1, 2, 3, 4);
  gtk_widget_show (gspaper->lmargin);


  /* create the top margin entry */
  label = gtk_label_new ("Top Margin:");
  gtk_widget_show (label);
  /*gtk_misc_set_alignment (GTK_MISC(label), 0.0, 0.5);*/
  gtk_table_attach_defaults (GTK_TABLE(table), label, 0, 1, 4, 5);

  adj = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, G_MAXFLOAT, 1.0,
                                              5.0, 0.0);
  gspaper->tmargin = gtk_spin_button_new(adj, 5.0, 1);
  gtk_signal_connect (GTK_OBJECT(GTK_COMBO(gspaper->unit)->entry), "changed",
                      (GtkSignalFunc) unit_changed,
                      gspaper);
  gtk_table_attach_defaults (GTK_TABLE (table), gspaper->unit, 2, 3, 1, 2);
  gtk_widget_show (gspaper->unit);

  gspaper->unit_label =
    gtk_label_new ((gchar*)g_list_data(gnome_unit_name_list()));
  /*gtk_misc_set_alignment (GTK_MISC(gspaper->unit_label), 0.0, 0.5);*/
  gtk_table_attach_defaults (GTK_TABLE (table),
                             gspaper->unit_label, 2, 3, 2, 3);
  gtk_widget_show (gspaper->unit_label);

  /* buttons for the default settings */
  button = gtk_button_new_with_label ("Set System Paper Size");
  gtk_signal_connect (GTK_OBJECT(button), "clicked",
                      (GtkSignalFunc) paper_size_system,
                      gspaper);
  gtk_container_add (GTK_CONTAINER (gspaper), button);
  gtk_tooltips_set_tip(tooltips, button,
                       "Set the papersize to the value found in $PAPERCONF. "
                       "If this variable isnet, look into the file "
                       "specified by $PAPERSIZE. If this fails, too, look "
                       "into the default file /etc/paperconf else fallback "
                       "to the builtin value", NULL);

  gtk_widget_show (button);

  /* update the width and height entries */
  paper_size_system (NULL, gspaper);
  paper_changed (NULL, gspaper);

  /* show the whole thing */
  gtk_table_set_col_spacings (GTK_TABLE(table), 10);
  /*gtk_table_set_row_spacings (GTK_TABLE(table), 10);*/

  gtk_table_set_row_spacing (GTK_TABLE(table), 0, 20);
  gtk_table_set_row_spacing (GTK_TABLE(table), 1, 10);
  gtk_table_set_row_spacing (GTK_TABLE(table), 2, 20);

  gtk_widget_show (table);
}	

GtkWidget *
gnome_select_paper_new (void)
{
  return GTK_WIDGET ( gtk_type_new (gnome_paper_selector_get_type ()));
}

gchar *gnome_paper_selector_get_name (GnomePaperSelector *gspaper)
{
  gchar *paper_name;

  paper_name = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(gspaper->paper)->entry));

  return paper_name;
}

gfloat gnome_paper_selector_get_width (GnomePaperSelector *gspaper)
{
  gfloat paper_width;

  paper_width = gtk_spin_button_get_value_as_float
	  (GTK_SPIN_BUTTON(gspaper->width));

  return paper_width;
}

gfloat gnome_paper_selector_get_height (GnomePaperSelector *gspaper)
{
  gfloat paper_height;

  paper_height = gtk_spin_button_get_value_as_float
	  (GTK_SPIN_BUTTON(gspaper->height));

  return paper_height;
}
