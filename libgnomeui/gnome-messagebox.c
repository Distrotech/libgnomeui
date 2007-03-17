/* GNOME GUI Library
 * Copyright (C) 1997, 1998 Jay Painter
 * All rights reserved.
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
/*
  @NOTATION@
*/

#include <config.h>
#include <libgnome/gnome-macros.h>

#ifndef GNOME_DISABLE_DEPRECATED_SOURCE

#include <stdarg.h>

/* Must be before all other gnome includes!! */
#include <glib/gi18n-lib.h>

#include "gnome-messagebox.h"

#include <libgnome/gnome-triggers.h>
#include <libgnome/gnome-util.h>
#include <string.h> /* for strcmp */
#include <gtk/gtk.h>
#include <libgnomeui/gnome-uidefs.h>

#define GNOME_MESSAGE_BOX_WIDTH  425
#define GNOME_MESSAGE_BOX_HEIGHT 125

struct _GnomeMessageBoxPrivate {
	/* not used currently, if something is added
	 * make sure to update _init and finalize */
	int dummy;
};

GNOME_CLASS_BOILERPLATE (GnomeMessageBox, gnome_message_box,
			 GnomeDialog, GNOME_TYPE_DIALOG)

static void
gnome_message_box_class_init (GnomeMessageBoxClass *klass)
{
	/*
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *)klass;
	*/
}

static void
gnome_message_box_instance_init (GnomeMessageBox *message_box)
{
	/*
 	message_box->_priv = g_new0(GnomeMessageBoxPrivate, 1); 
	*/
}

/**
 * gnome_message_box_construct:
 * @messagebox: The message box to construct
 * @message: The message to be displayed.
 * @message_box_type: The type of the message
 * @buttons: a NULL terminated array with the buttons to insert.
 *
 * For language bindings or subclassing, from C use #gnome_message_box_new or
 * #gnome_message_box_newv
 */
void
gnome_message_box_construct (GnomeMessageBox       *messagebox,
			     const gchar           *message,
			     const gchar           *message_box_type,
			     const gchar          **buttons)
{
	GtkWidget *hbox;
	GtkWidget *pixmap = NULL;
	GtkWidget *alignment;
	GtkWidget *label;
	char *s;
        const gchar* title_prefix = NULL;
        const gchar* appname;

	g_return_if_fail (messagebox != NULL);
	g_return_if_fail (GNOME_IS_MESSAGE_BOX (messagebox));
	g_return_if_fail (message != NULL);
	g_return_if_fail (message_box_type != NULL);

	atk_object_set_role (gtk_widget_get_accessible (GTK_WIDGET (messagebox)), ATK_ROLE_ALERT);

	/* Make noises, basically */
	gnome_triggers_vdo(message, message_box_type, NULL);

	if (strcmp(GNOME_MESSAGE_BOX_INFO, message_box_type) == 0)
	{
                title_prefix = _("Information");
		pixmap = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
	}
	else if (strcmp(GNOME_MESSAGE_BOX_WARNING, message_box_type) == 0)
	{
                title_prefix = _("Warning");
		pixmap = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
		
	}
	else if (strcmp(GNOME_MESSAGE_BOX_ERROR, message_box_type) == 0)
	{
                title_prefix = _("Error");
		pixmap = gtk_image_new_from_stock (GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);

	}
	else if (strcmp(GNOME_MESSAGE_BOX_QUESTION, message_box_type) == 0)
	{
                title_prefix = _("Question");
		pixmap = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
	}
	else
	{
                title_prefix = _("Message");
	}

        g_assert(title_prefix != NULL);
        s = NULL;
        appname = gnome_program_get_human_readable_name(gnome_program_get());
        if (appname) {
                s = g_strdup_printf("%s (%s)", title_prefix, appname);
        }

	gnome_dialog_constructv (GNOME_DIALOG (messagebox), s ? s : title_prefix, buttons);
	g_free (s);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX(GNOME_DIALOG(messagebox)->vbox),
			    hbox, TRUE, TRUE, 10);
	gtk_widget_show (hbox);

	if (pixmap) {
		gtk_box_pack_start (GTK_BOX(hbox), 
				    pixmap, FALSE, TRUE, 0);
		gtk_widget_show (pixmap);
	}

	label = gtk_label_new (message);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	gtk_widget_show (label);

	/* Add some extra space on the right to balance the pixmap */
	if (pixmap) {
		alignment = gtk_alignment_new (0., 0., 0., 0.);
		gtk_widget_set_size_request (alignment, GNOME_PAD, -1);
		gtk_widget_show (alignment);
		
		gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 0);
	}
	
	gnome_dialog_set_close (GNOME_DIALOG (messagebox),
				TRUE );
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

	g_return_val_if_fail (message != NULL, NULL);
	g_return_val_if_fail (message_box_type != NULL, NULL);
        
	va_start (ap, message_box_type);
	
	message_box = g_object_new (GNOME_TYPE_MESSAGE_BOX, NULL);

	gnome_message_box_construct (message_box, message,
				     message_box_type, NULL);
	
	/* we need to add buttons by hand here */
	while (TRUE) {
		gchar * button_name;

		button_name = va_arg (ap, gchar *);

		if (button_name == NULL) {
			break;
		}

		gnome_dialog_append_button ( GNOME_DIALOG(message_box), 
					     button_name);
	}
	
	va_end (ap);

	if (GNOME_DIALOG (message_box)->buttons) {
		gtk_widget_grab_focus(
			g_list_last (GNOME_DIALOG (message_box)->buttons)->data);
	}

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
 * The buttons array can be NULL if you wish to add buttons yourself later.
 *
 * Returns a widget that has the dialog box.
 */
GtkWidget*
gnome_message_box_newv (const gchar           *message,
		        const gchar           *message_box_type,
			const gchar 	     **buttons)
{
	GnomeMessageBox *message_box;

	g_return_val_if_fail (message != NULL, NULL);
	g_return_val_if_fail (message_box_type != NULL, NULL);

	message_box = g_object_new (GNOME_TYPE_MESSAGE_BOX, NULL);

	gnome_message_box_construct (message_box, message,
				     message_box_type, buttons);

	return GTK_WIDGET (message_box);
}

#endif /* GNOME_DISABLE_DEPRECATED_SOURCE */
