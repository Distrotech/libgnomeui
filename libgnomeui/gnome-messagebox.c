/* GNOME GUI Library
 * Copyright (C) 1997 Jay Painter
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
#include "gnome-messagebox.h"
#include <string.h> /* for strcmp */
#include <gtk/gtk.h>

enum {
  CLICKED,
  LAST_SIGNAL
};

typedef void (*GnomeMessageBoxSignal1) (GtkObject *object,
				        gint       arg1,
				        gpointer   data);

static void gnome_messagebox_marshal_signal_1 (GtkObject         *object,
					       GtkSignalFunc      func,
					       gpointer           func_data,
					       GtkArg            *args);

static void gnome_messagebox_class_init (GnomeMessageBoxClass *klass);
static void gnome_messagebox_init       (GnomeMessageBox      *messagebox);

static GtkWindowClass *parent_class;
static gint messagebox_signals[LAST_SIGNAL] = { 0 };

guint
gnome_messagebox_get_type ()
{
  static guint messagebox_type = 0;

  if (!messagebox_type)
    {
      GtkTypeInfo messagebox_info =
      {
	"GnomeMessageBox",
	sizeof (GnomeMessageBox),
	sizeof (GnomeMessageBoxClass),
	(GtkClassInitFunc) gnome_messagebox_class_init,
	(GtkObjectInitFunc) gnome_messagebox_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL,
      };

      messagebox_type = gtk_type_unique (gtk_window_get_type (), &messagebox_info);
    }

  return messagebox_type;
}

static void
gnome_messagebox_class_init (GnomeMessageBoxClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkWindowClass *window_class;

  object_class = (GtkObjectClass*) klass;
  widget_class = (GtkWidgetClass*) klass;
  window_class = (GtkWindowClass*) klass;

  parent_class = gtk_type_class (gtk_window_get_type ());

  messagebox_signals[CLICKED] =
    gtk_signal_new ("clicked",
                    GTK_RUN_LAST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GnomeMessageBoxClass, clicked),
                    gnome_messagebox_marshal_signal_1,
                    GTK_TYPE_NONE, 1, GTK_TYPE_INT);

  gtk_object_class_add_signals (object_class, messagebox_signals, LAST_SIGNAL);

}

static void
gnome_messagebox_marshal_signal_1 (GtkObject      *object,
			           GtkSignalFunc   func,
			           gpointer        func_data,
			           GtkArg         *args)
{
  GnomeMessageBoxSignal1 rfunc;

  rfunc = (GnomeMessageBoxSignal1) func;

  (* rfunc) (object, GTK_VALUE_INT (args[0]), func_data);
}

static void
gnome_messagebox_init (GnomeMessageBox *messagebox)
{
  messagebox->modal = FALSE;
}

GtkWidget*
gnome_messagebox_new (gchar           *message,
		      gchar           *messagebox_type,
		      gchar           *button1,
		      gchar           *button2,
		      gchar           *button3)
{
  GnomeMessageBox *messagebox;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *separator;

  GtkWidget *pixmapwid;
  GdkPixmap *pixmap;
  GdkBitmap *mask;                 
  GtkStyle *style;

  messagebox = gtk_type_new (gnome_messagebox_get_type ());

  gtk_window_set_policy (GTK_WINDOW (messagebox), FALSE, FALSE, FALSE);
  gtk_widget_set_usize (GTK_WIDGET (messagebox), GNOME_MESSAGEBOX_WIDTH,
			GNOME_MESSAGEBOX_HEIGHT);
  gtk_container_border_width (GTK_CONTAINER (messagebox), 
			      GNOME_MESSAGEBOX_BORDER_WIDTH);

  style = gtk_widget_get_style (GTK_WIDGET (messagebox));

  if (strcmp(GNOME_MESSAGEBOX_INFO, messagebox_type) == 0)
    {
      gtk_window_set_title (GTK_WINDOW (messagebox), "Information");
      pixmap = gdk_pixmap_create_from_xpm (GTK_WIDGET (messagebox)->window, 
					   &mask, 
					   &style->bg[GTK_STATE_NORMAL],
					   "bomb.xpm");
    }
  else if (strcmp(GNOME_MESSAGEBOX_WARNING, messagebox_type) == 0)
    {
      gtk_window_set_title (GTK_WINDOW (messagebox), "Warning");
      pixmap = gdk_pixmap_create_from_xpm (GTK_WIDGET (messagebox)->window,
					   &mask, 
					   &style->bg[GTK_STATE_NORMAL],
					   "bomb.xpm");
    }
  else if (strcmp(GNOME_MESSAGEBOX_ERROR, messagebox_type) == 0)
    {
      gtk_window_set_title (GTK_WINDOW (messagebox), "Error");
      pixmap = gdk_pixmap_create_from_xpm (GTK_WIDGET (messagebox)->window, 
					   &mask, 
					   &style->bg[GTK_STATE_NORMAL],
					   "bomb.xpm");
    }
  else if (strcmp(GNOME_MESSAGEBOX_QUESTION, messagebox_type) == 0)
    {
      gtk_window_set_title (GTK_WINDOW (messagebox), "Question");
      pixmap = gdk_pixmap_create_from_xpm (GTK_WIDGET (messagebox)->window, 
					   &mask, 
					   &style->bg[GTK_STATE_NORMAL],
					   "bomb.xpm");
    }
  else
    {
      gtk_window_set_title (GTK_WINDOW (messagebox), "");
      pixmap = NULL;
    }

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (messagebox), vbox);
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

  label = gtk_label_new (message);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  separator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE,
		      GNOME_MESSAGEBOX_BORDER_WIDTH);
  gtk_widget_show (separator);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  gtk_widget_show (hbox);

  if (button1)
    messagebox->button1 = gtk_button_new_with_label (button1);
  else
    messagebox->button1 = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (GTK_WIDGET (messagebox->button1), GTK_CAN_DEFAULT);
  gtk_widget_set_usize (GTK_WIDGET (messagebox->button1), 
			GNOME_MESSAGEBOX_BUTTON_WIDTH,
			GNOME_MESSAGEBOX_BUTTON_HEIGHT);
  gtk_box_pack_start (GTK_BOX (hbox), messagebox->button1, TRUE, FALSE, 0);
  gtk_widget_grab_default (GTK_WIDGET (messagebox->button1));
  gtk_widget_show (messagebox->button1);
  
  gtk_signal_connect (GTK_OBJECT (messagebox->button1), "clicked",
		      (GtkSignalFunc) gnome_messagebox_button_clicked,
		      messagebox);

  if (button2)
    {
      messagebox->button2 = gtk_button_new_with_label (button2);
      GTK_WIDGET_SET_FLAGS (messagebox->button2, GTK_CAN_DEFAULT);
      gtk_widget_set_usize (GTK_WIDGET (messagebox->button2), 
			    GNOME_MESSAGEBOX_BUTTON_WIDTH,
			    GNOME_MESSAGEBOX_BUTTON_HEIGHT);
      gtk_box_pack_start (GTK_BOX (hbox), messagebox->button2, TRUE, FALSE, 0);
      gtk_widget_show (messagebox->button2);
      
      gtk_signal_connect (GTK_OBJECT (messagebox->button2), "clicked",
			  (GtkSignalFunc) gnome_messagebox_button_clicked,
			  messagebox);
    }

  if (button3)
    {
      messagebox->button3 = gtk_button_new_with_label (button3);
      GTK_WIDGET_SET_FLAGS (messagebox->button3, GTK_CAN_DEFAULT);
      gtk_widget_set_usize (GTK_WIDGET (messagebox->button3), 
			    GNOME_MESSAGEBOX_BUTTON_WIDTH,
			    GNOME_MESSAGEBOX_BUTTON_HEIGHT);
      gtk_box_pack_start (GTK_BOX (hbox), messagebox->button3, TRUE, FALSE, 0);
      gtk_widget_show (messagebox->button3);
      
      gtk_signal_connect (GTK_OBJECT (messagebox->button3), "clicked",
			  (GtkSignalFunc) gnome_messagebox_button_clicked,
			  messagebox);
    }

  return GTK_WIDGET (messagebox);
}

void
gnome_messagebox_set_modal (GnomeMessageBox     *messagebox)
{
  messagebox->modal = TRUE;
  gtk_grab_add (GTK_WIDGET (messagebox));
}

void
gnome_messagebox_set_default (GnomeMessageBox     *messagebox,
                              gint                button)
{
  switch (button)
    {
    case 1:
      if (messagebox->button1)
        gtk_widget_grab_default (GTK_WIDGET (messagebox->button1));
      break;
    case 2:
      if (messagebox->button2)
        gtk_widget_grab_default (GTK_WIDGET (messagebox->button2));
      break;
    case 3:
      if (messagebox->button3)
        gtk_widget_grab_default (GTK_WIDGET (messagebox->button3));
      break;
    default:
      break;
    }
}

void
gnome_messagebox_button_clicked (GtkWidget   *button, 
			         GtkWidget   *messagebox)
{
  if (button == GNOME_MESSAGEBOX (messagebox)->button1)
    gtk_signal_emit (GTK_OBJECT (messagebox), messagebox_signals[CLICKED], 1);
  else if (button == GNOME_MESSAGEBOX (messagebox)->button2)
    gtk_signal_emit (GTK_OBJECT (messagebox), messagebox_signals[CLICKED], 2);
  else
     gtk_signal_emit (GTK_OBJECT (messagebox), messagebox_signals[CLICKED], 3);

  if (GNOME_MESSAGEBOX (messagebox)->modal)
    gtk_grab_remove (GTK_WIDGET (messagebox));
 
  gtk_widget_destroy (messagebox);
}
