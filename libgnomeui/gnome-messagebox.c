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

#ifndef GNOME_DISABLE_DEPRECATED_SOURCE

#include <stdarg.h>

/* Must be before all other gnome includes!! */
#include "gnome-i18nP.h"

#include "gnome-messagebox.h"

#include <libgnome/gnome-triggers.h>
#include <libgnome/gnome-util.h>
#include <string.h> /* for strcmp */
#include <gtk/gtk.h>
#include <libgnomeui/gnome-uidefs.h>
#include <libgnome/libgnome.h>

#define GNOME_MESSAGE_BOX_WIDTH  425
#define GNOME_MESSAGE_BOX_HEIGHT 125

struct _GnomeMessageBoxPrivate {
	/* not used currently, if something is added
	 * make sure to update _init and finalize */
	int dummy;
};

static void gnome_message_box_class_init (GnomeMessageBoxClass *klass);
static void gnome_message_box_init       (GnomeMessageBox      *messagebox);
static void gnome_message_box_destroy    (GtkObject            *object);
static void gnome_message_box_finalize   (GObject              *object);

static GnomeDialogClass *parent_class;

guint
gnome_message_box_get_type (void)
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
			NULL,
			NULL,
			NULL
		};

		message_box_type = gtk_type_unique (gnome_dialog_get_type (), &message_box_info);
	}

	return message_box_type;
}

static void
gnome_message_box_class_init (GnomeMessageBoxClass *klass)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = (GtkObjectClass *)klass;
	gobject_class = (GObjectClass *)klass;
	parent_class = gtk_type_class (gnome_dialog_get_type ());

	object_class->destroy = gnome_message_box_destroy;
	gobject_class->finalize = gnome_message_box_finalize;
}

static void
gnome_message_box_init (GnomeMessageBox *message_box)
{
	/*
 	message_box->_priv = g_new0(GnomeMessageBoxPrivate, 1); 
	*/
}

static void
gnome_message_box_destroy(GtkObject *object)
{
	/* remember, destroy can be run multiple times! */
	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gnome_message_box_finalize(GObject *object)
{
	/*
	GnomeMessageBox *mbox = GNOME_MESSAGE_BOX(object);

	g_free(mbox->_priv);
	mbox->_priv = NULL;
	*/

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
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
 *
 * Returns:
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
	GtkStyle *style;
        const gchar* title_prefix = NULL;
        const gchar* appname;
	gint i = 0;

	g_return_if_fail (messagebox != NULL);
	g_return_if_fail (GNOME_IS_MESSAGE_BOX (messagebox));
	g_return_if_fail (message != NULL);
	g_return_if_fail (message_box_type != NULL);

	style = gtk_widget_get_style (GTK_WIDGET (messagebox));

	/* Make noises, basically */
	gnome_triggers_vdo(message, message_box_type, NULL);

	if (strcmp(GNOME_MESSAGE_BOX_INFO, message_box_type) == 0)
	{
                title_prefix = _("Information");
		s = GNOMEUIPIXMAPDIR "/gnome-info.png";
		if (s) {
                        pixmap = gtk_image_new_from_file (s);
                        g_free(s);
                }
	}
	else if (strcmp(GNOME_MESSAGE_BOX_WARNING, message_box_type) == 0)
	{
                title_prefix = _("Warning");
		s = GNOMEUIPIXMAPDIR "/gnome-warning.png";
		if (s) {
                        pixmap = gtk_image_new_from_file (s);
                        g_free(s);
                }
	}
	else if (strcmp(GNOME_MESSAGE_BOX_ERROR, message_box_type) == 0)
	{
                title_prefix = _("Error");
		s = GNOMEUIPIXMAPDIR "/gnome-error.png";
		if (s) {
                        pixmap = gtk_image_new_from_file (s);
                        g_free(s);
                }
	}
	else if (strcmp(GNOME_MESSAGE_BOX_QUESTION, message_box_type) == 0)
	{
                title_prefix = _("Question");
		s = GNOMEUIPIXMAPDIR "/gnome-question.png";
		if (s) {
                        pixmap = gtk_image_new_from_file (s);
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
                s = g_strdup_printf("%s (%s)", title_prefix, appname);
        }
        if (s) {
                gtk_window_set_title(GTK_WINDOW(messagebox), s);
                g_free(s);
        } else {
                gtk_window_set_title(GTK_WINDOW(messagebox), title_prefix);
        }

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX(GNOME_DIALOG(messagebox)->vbox),
			    hbox, TRUE, TRUE, 10);
	gtk_widget_show (hbox);

	if (pixmap == NULL) {
        	if (pixmap) gtk_widget_destroy(pixmap);
		s = GNOMEUIPIXMAPDIR "/gnome-default-dlg.png";
         	if (s) {
			pixmap = gtk_image_new_from_file (s);
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
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_misc_set_padding (GTK_MISC (label), GNOME_PAD, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	gtk_widget_show (label);

	/* Add some extra space on the right to balance the pixmap */
	if (pixmap) {
		alignment = gtk_alignment_new (0., 0., 0., 0.);
		gtk_widget_set_usize (alignment, GNOME_PAD, -1);
		gtk_widget_show (alignment);
		
		gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 0);
	}

	if (buttons) {
		while (buttons[i]) {
			gnome_dialog_append_button (GNOME_DIALOG (messagebox), 
						    buttons[i]);
			i++;
		};
	}

	if(GNOME_DIALOG(messagebox)->buttons)
		gtk_widget_grab_focus(
			g_list_last (GNOME_DIALOG (messagebox)->buttons)->data);
	
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
	
	message_box = gtk_type_new (gnome_message_box_get_type ());

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

	gtk_widget_grab_focus(
		g_list_last (GNOME_DIALOG (message_box)->buttons)->data);

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

	message_box = gtk_type_new (gnome_message_box_get_type ());

	gnome_message_box_construct (message_box, message,
				     message_box_type, buttons);

	return GTK_WIDGET (message_box);
}

#endif /* GNOME_DISABLE_DEPRECATED_SOURCE */
