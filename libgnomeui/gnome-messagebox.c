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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */

#include <config.h>
#include <stdarg.h>
#include "gnome-messagebox.h"
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-triggers.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-i18nP.h"
#include <string.h> /* for strcmp */
#include <gtk/gtk.h>
#include "libgnomeui/gnome-stock.h"
#include "libgnomeui/gnome-uidefs.h"
#include <libgnome/gnomelib-init2.h>

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

static GtkWidget*
gnome_pixmap_new_from_file_conditional(const gchar* filename)
{
        GdkPixbuf *pixbuf;
        GtkWidget *gpixmap;
        
        pixbuf = gdk_pixbuf_new_from_file(filename);

        if (pixbuf != NULL) {
                gpixmap = gnome_pixmap_new_from_pixbuf(pixbuf);
                gdk_pixbuf_unref(pixbuf);
                return gpixmap;
        } else {
                return NULL;
        }
}

/**
 * gnome_message_box_new:
 * @message: The message to be displayed.
 * @message_box_type: The type of the message
 * @...: A NULL terminated list of strings to use in each button.
 *
 * Creates a dialog box of type @message_box_type with @message.  A number
 * of buttons are inserted on it.  You can use the GNOME stock identifiers
 * to create gnome-stock-buttons.
 *
 * Returns a widget that has the dialog box.
 */
GtkWidget*
gnome_message_box_new (const gchar           *message,
		       const gchar           *message_box_type, ...)
{
	va_list ap;
	GnomeMessageBox *message_box;
	GtkWidget *hbox;
	GtkWidget *pixmap = NULL;
	GtkWidget *alignment;
	char *s;
	GtkStyle *style;
        const gchar* title_prefix = NULL;
        const gchar* appname;
        
	va_start (ap, message_box_type);
	
	message_box = gtk_type_new (gnome_message_box_get_type ());

	style = gtk_widget_get_style (GTK_WIDGET (message_box));

	/* Make noises, basically */
	gnome_triggers_vdo(message, message_box_type, NULL);

	if (strcmp(GNOME_MESSAGE_BOX_INFO, message_box_type) == 0)
	{
                title_prefix = _("Information");
		s = gnome_unconditional_pixmap_file("gnome-info.png");
		if (s) {
                        pixmap = gnome_pixmap_new_from_file_conditional(s);
                        g_free(s);
                }
	}
	else if (strcmp(GNOME_MESSAGE_BOX_WARNING, message_box_type) == 0)
	{
                title_prefix = _("Warning");
		s = gnome_unconditional_pixmap_file("gnome-warning.png");
		if (s) {
                        pixmap = gnome_pixmap_new_from_file_conditional(s);
                        g_free(s);
                }
	}
	else if (strcmp(GNOME_MESSAGE_BOX_ERROR, message_box_type) == 0)
	{
                title_prefix = _("Error");
		s = gnome_unconditional_pixmap_file("gnome-error.png");
		if (s) {
                        pixmap = gnome_pixmap_new_from_file_conditional(s);
                        g_free(s);
                }
	}
	else if (strcmp(GNOME_MESSAGE_BOX_QUESTION, message_box_type) == 0)
	{
                title_prefix = _("Question");
		s = gnome_unconditional_pixmap_file("gnome-question.png");
		if (s) {
                        pixmap = gnome_pixmap_new_from_file_conditional(s);
                        g_free(s);
                }
	}
	else
	{
                title_prefix = _("Message");
	}

        g_assert(title_prefix != NULL);
        s = NULL;
        appname = gnome_program_get_human_readable_name(gnome_program_get());
        if (appname) {
                s = g_strdup_printf("%s: %s", title_prefix, appname);
        }
        if (s) {
                gtk_window_set_title(GTK_WINDOW(message_box), s);
                g_free(s);
        } else {
                gtk_window_set_title(GTK_WINDOW(message_box), title_prefix);
        }
                              
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX(GNOME_DIALOG(message_box)->vbox),
			    hbox, TRUE, TRUE, 10);
	gtk_widget_show (hbox);

	if ( pixmap == NULL ) {
        	if (pixmap) gtk_widget_destroy(pixmap);
		s = gnome_unconditional_pixmap_file ("gnome-default-dlg.png");
         	if (s) {
			pixmap = gnome_pixmap_new_from_file_conditional(s);
                        g_free(s);
                } else
			pixmap = NULL;
	}
	if (pixmap) {
		gtk_box_pack_start (GTK_BOX(hbox), 
				    pixmap, FALSE, TRUE, 0);
		gtk_widget_show (pixmap);
	}

	message_box->label = gtk_label_new (message);
	gtk_label_set_justify (GTK_LABEL (message_box->label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_padding (GTK_MISC (message_box->label), GNOME_PAD, 0);
	gtk_box_pack_start (GTK_BOX (hbox), message_box->label, TRUE, TRUE, 0);
	gtk_widget_show (message_box->label);

	/* Add some extra space on the right to balance the pixmap */
	if (pixmap) {
		alignment = gtk_alignment_new (0., 0., 0., 0.);
		gtk_widget_set_usize (alignment, GNOME_PAD, -1);
		gtk_widget_show (alignment);
		
		gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 0);
	}
	
	while (TRUE) {
	  gchar * button_name;

	  button_name = va_arg (ap, gchar *);
	  
	  if (button_name == NULL) {
	    break;
	  }
	  
	  gnome_dialog_append_button ( GNOME_DIALOG(message_box), 
				       button_name);
	};
	
	va_end (ap);

	gnome_dialog_set_close ( GNOME_DIALOG(message_box),
				 TRUE );

	return GTK_WIDGET (message_box);
}

/**
 * gnome_message_box_newv:
 * @message: The message to be displayed.
 * @message_box_type: The type of the message
 * @buttons: a NULL terminated array with the buttons to insert.
 *
 * Creates a dialog box of type @message_box_type with @message.  A number
 * of buttons are inserted on it, the messages come from the @buttons array.
 * You can use the GNOME stock identifiers to create gnome-stock-buttons.
 *
 * Returns a widget that has the dialog box.
 */
GtkWidget*
gnome_message_box_newv (const gchar           *message,
		        const gchar           *message_box_type,
			const gchar 	     **buttons)
{
	GnomeMessageBox *message_box;
	GtkWidget *label, *hbox;
	GtkWidget *pixmap = NULL;
	char *s;
	GtkStyle *style;
	gint i = 0;

	message_box = gtk_type_new (gnome_message_box_get_type ());

	style = gtk_widget_get_style (GTK_WIDGET (message_box));

	/* Make noises, basically */
	gnome_triggers_vdo(message, message_box_type, NULL);

	if (strcmp(GNOME_MESSAGE_BOX_INFO, message_box_type) == 0)
	{
		gtk_window_set_title (GTK_WINDOW (message_box), _("Information"));
		s = gnome_pixmap_file("gnome-info.png");
		if (s) {
                        pixmap = gnome_pixmap_new_from_file_conditional(s);
                        g_free(s);
                }
	}
	else if (strcmp(GNOME_MESSAGE_BOX_WARNING, message_box_type) == 0)
	{
		gtk_window_set_title (GTK_WINDOW (message_box), _("Warning"));
		s = gnome_pixmap_file("gnome-warning.png");
		if (s) {
                        pixmap = gnome_pixmap_new_from_file_conditional(s);
                        g_free(s);
                }
	}
	else if (strcmp(GNOME_MESSAGE_BOX_ERROR, message_box_type) == 0)
	{
		gtk_window_set_title (GTK_WINDOW (message_box), _("Error"));
		s = gnome_pixmap_file("gnome-error");
		if (s) {
                        pixmap = gnome_pixmap_new_from_file_conditional(s);
                        g_free(s);
                }
	}
	else if (strcmp(GNOME_MESSAGE_BOX_QUESTION, message_box_type) == 0)
	{
		gtk_window_set_title (GTK_WINDOW (message_box), _("Question"));
		s = gnome_pixmap_file("gnome-question.png");
		if (s) {
                        pixmap = gnome_pixmap_new_from_file_conditional(s);
                        g_free(s);
                }
	}
	else
	{
		gtk_window_set_title (GTK_WINDOW (message_box), _("Message"));
	}

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX(GNOME_DIALOG(message_box)->vbox),
			    hbox, TRUE, TRUE, 10);
	gtk_widget_show (hbox);

	if ( pixmap == NULL) {
        	if (pixmap) gtk_widget_destroy(pixmap);
		s = gnome_pixmap_file("gnome-default.png");
         	if (s) {
			pixmap = gnome_pixmap_new_from_file_conditional(s);
                        g_free(s);
                } else
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

	while (buttons[i]) {
	  gnome_dialog_append_button ( GNOME_DIALOG(message_box), 
				       buttons[i]);
	  i++;
	};
	
	gnome_dialog_set_close ( GNOME_DIALOG(message_box),
				 TRUE );

	return GTK_WIDGET (message_box);
}

GtkWidget *
gnome_message_box_get_label (GnomeMessageBox *messagebox)
{
	g_return_val_if_fail (messagebox != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_MESSAGE_BOX (messagebox), NULL);

	return messagebox->label;
}
