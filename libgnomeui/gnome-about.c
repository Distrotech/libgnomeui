/* GNOME GUI Library
 * Copyright (C) 1998 Horacio J. Peña
 * Based in gnome-about, copyright (C) 1997 Jay Painter 
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-i18n.h"
#include "gnome-about.h"
#include <gtk/gtk.h>

enum {
  CLICKED,
  LAST_SIGNAL
};

static void gnome_about_class_init (GnomeAboutClass *klass);
static void gnome_about_init       (GnomeAbout      *about);

static GtkWindowClass *parent_class;

guint
gnome_about_get_type ()
{
  static guint about_type = 0;

  if (!about_type)
    {
      GtkTypeInfo about_info =
      {
	"GnomeAbout",
	sizeof (GnomeAbout),
	sizeof (GnomeAboutClass),
	(GtkClassInitFunc) gnome_about_class_init,
	(GtkObjectInitFunc) gnome_about_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL,
      };

      about_type = gtk_type_unique (gtk_window_get_type (), &about_info);
    }

  return about_type;
}

void
gnome_about_button_clicked (GtkWidget   *button,
                            GtkWidget   *about)
{
	gtk_widget_destroy(about);
}

static void
gnome_about_class_init (GnomeAboutClass *klass)
{
}

static void
gnome_about_init (GnomeAbout *about)
{
}

GtkWidget* 
gnome_about_new (gchar	*title,
		gchar	*version,
		gchar   *copyright,
		gchar   **authors,
		gchar   *comments,
		gchar   *logo)
{
  GnomeAbout *about;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *vbox_labels;
  GtkWidget *vbox_authors;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *separator;
  GtkWidget *button;

  GtkWidget *pixmapwid;
  GdkPixmap *pixmap = NULL;
  GdkBitmap *mask;
  GtkStyle *style;

  gchar *s;

  about = gtk_type_new (gnome_about_get_type ());

  gtk_container_border_width (GTK_CONTAINER (about), 5);

  style = gtk_widget_get_style (GTK_WIDGET (about));

  gtk_window_set_title (GTK_WINDOW (about), _("About"));

  if(logo)
 	pixmap = gdk_pixmap_create_from_xpm (GTK_WIDGET (about)->window, 
					   &mask, 
					   &style->bg[GTK_STATE_NORMAL],
					   logo);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (about), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 10);
  gtk_widget_show (hbox);
  
  if (pixmap)
    {
      pixmapwid = gtk_pixmap_new (pixmap, mask);
      gtk_box_pack_start (GTK_BOX (hbox), pixmapwid, FALSE, TRUE, 0);
      gtk_widget_show (pixmapwid);
    }

  s = "";
  if(title) 
    	s = g_copy_strings (title, " ", version ? version : "", NULL);
  frame = gtk_frame_new (s);
  if(title) 
  	g_free(s);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
  gtk_box_pack_start_defaults (GTK_BOX (hbox), frame);
  gtk_widget_show (frame);

  vbox_labels = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox_labels);
  gtk_container_border_width (GTK_CONTAINER (vbox_labels), 5);
  gtk_widget_show (vbox_labels);

  if(copyright) 
    {
  	label = gtk_label_new (copyright);
  	gtk_box_pack_start_defaults (GTK_BOX (vbox_labels), label);
  	gtk_widget_show (label);
    }

  if (authors && authors[0]) 
    {
	frame = gtk_frame_new ( authors[1] ? _("Authors: \n") : _("Author: ") );
  	gtk_container_border_width (GTK_CONTAINER (frame), 10);
  	gtk_box_pack_start_defaults (GTK_BOX (vbox_labels), frame);
  	gtk_widget_show (frame);
    	
 	vbox_authors = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), vbox_authors);
  	gtk_widget_show (vbox_authors);
  
	while( *authors ) {
  		label = gtk_label_new ( *authors );
  		gtk_box_pack_start (GTK_BOX (vbox_authors), label, TRUE, TRUE, 0);
  		gtk_widget_show (label);
		authors++;
		}
  	}

  if(comments) 
    {
  	label = gtk_label_new (comments);
  	gtk_box_pack_start_defaults (GTK_BOX (vbox_labels), label);
  	gtk_widget_show (label);
    }
  	
  separator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE, 5);
  gtk_widget_show (separator);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_label ( _("OK") );
  GTK_WIDGET_SET_FLAGS (GTK_WIDGET (button), GTK_CAN_DEFAULT);
  gtk_widget_set_usize (GTK_WIDGET (button), 
			GNOME_ABOUT_BUTTON_WIDTH,
			GNOME_ABOUT_BUTTON_HEIGHT);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 0);
  gtk_widget_grab_default (GTK_WIDGET (button));
  gtk_widget_show (button);
  
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) gnome_about_button_clicked,
		      about);

  return GTK_WIDGET (about);
}
