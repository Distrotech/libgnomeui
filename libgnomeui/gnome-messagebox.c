/* GNOME GUI Library
 * Copyright (C) 1997, 1998 Jay Painter
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

#include <config.h>
#include <stdarg.h>
#include "gnome-messagebox.h"
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-i18nP.h"
#include <string.h> /* for strcmp */
#include <gtk/gtk.h>
#include "libgnomeui/gnome-stock.h"
#include "libgnomeui/gnome-uidefs.h"

#define GNOME_MESSAGE_BOX_WIDTH  425
#define GNOME_MESSAGE_BOX_HEIGHT 125


static void gnome_message_box_class_init (GnomeMessageBoxClass *klass);
static void gnome_message_box_init       (GnomeMessageBox      *messagebox);

static GnomeDialogClass *parent_class;

guint
gnome_message_box_get_type ()
{
	static guint message_box_type = 0;

	if (!message_box_type)
	{
		GtkTypeInfo message_box_info =
		{
			"GnomeMessageBox",
			sizeof (GnomeMessageBox),
			sizeof (GnomeMessageBoxClass),
			(GtkClassInitFunc) gnome_message_box_class_init,
			(GtkObjectInitFunc) gnome_message_box_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL,
		};

		message_box_type = gtk_type_unique (gnome_dialog_get_type (), &message_box_info);
	}

	return message_box_type;
}

static void
gnome_message_box_class_init (GnomeMessageBoxClass *klass)
{
	parent_class = gtk_type_class (gnome_dialog_get_type ());
}

static void
gnome_message_box_init (GnomeMessageBox *message_box)
{
  
}


GtkWidget*
gnome_message_box_new (const gchar           *message,
		       const gchar           *message_box_type, ...)
{
	va_list ap;
	GnomeMessageBox *message_box;
	GtkWidget *label, *hbox;
	GtkWidget *pixmap = NULL;
	char *s;
	GtkStyle *style;

	va_start (ap, message_box_type);
	
	message_box = gtk_type_new (gnome_message_box_get_type ());

	style = gtk_widget_get_style (GTK_WIDGET (message_box));

	if (strcmp(GNOME_MESSAGE_BOX_INFO, message_box_type) == 0)
	{
		gtk_window_set_title (GTK_WINDOW (message_box), _("Information"));
		s = gnome_pixmap_file("gnome-info.png");
		if (s) pixmap = gnome_pixmap_new_from_file(s);
	}
	else if (strcmp(GNOME_MESSAGE_BOX_WARNING, message_box_type) == 0)
	{
		gtk_window_set_title (GTK_WINDOW (message_box), _("Warning"));
		s = gnome_pixmap_file("gnome-warning.png");
		if (s) pixmap = gnome_pixmap_new_from_file(s);
	}
	else if (strcmp(GNOME_MESSAGE_BOX_ERROR, message_box_type) == 0)
	{
		gtk_window_set_title (GTK_WINDOW (message_box), _("Error"));
		s = gnome_pixmap_file("gnome-error");
		if (s) pixmap = gnome_pixmap_new_from_file(s);
	}
	else if (strcmp(GNOME_MESSAGE_BOX_QUESTION, message_box_type) == 0)
	{
		gtk_window_set_title (GTK_WINDOW (message_box), _("Question"));
		s = gnome_pixmap_file("gnome-question.png");
		if (s) pixmap = gnome_pixmap_new_from_file(s);
	}
	else
	{
		gtk_window_set_title (GTK_WINDOW (message_box), _("Message"));
	}

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX(GNOME_DIALOG(message_box)->vbox),
			    hbox, TRUE, TRUE, 10);
	gtk_widget_show (hbox);

	if ( (pixmap == NULL) ||
	     (GNOME_PIXMAP(pixmap)->pixmap == NULL) ) {
        	if (pixmap) gtk_widget_destroy(pixmap);
		s = gnome_pixmap_file("gnome-default.png");
         	if (s)
			pixmap = gnome_pixmap_new_from_file(s);
		else
			pixmap = NULL;
	}
	if (pixmap) {
		gtk_box_pack_start (GTK_BOX(hbox), 
				    pixmap, FALSE, TRUE, 0);
		gtk_widget_show (pixmap);
	}

	label = gtk_label_new (message);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	gtk_widget_show (label);

	while (TRUE) {
	  gchar * button_name;

	  button_name = va_arg (ap, gchar *);
	  
	  if (button_name == NULL) {
	    break;
	  }
	  
	  gnome_dialog_append_buttons( GNOME_DIALOG(message_box), 
				       button_name, 
				       NULL );
	};
	
	va_end (ap);

	gnome_dialog_set_destroy ( GNOME_DIALOG(message_box),
				   TRUE );

	return GTK_WIDGET (message_box);
}

/* These two here for backwards compatibility */

void
gnome_message_box_set_modal (GnomeMessageBox     *message_box)
{
  gnome_dialog_set_modal(GNOME_DIALOG(message_box));
}

void
gnome_message_box_set_default (GnomeMessageBox     *message_box,
                              gint                button)
{
  gnome_dialog_set_default(GNOME_DIALOG(message_box), button);
}



