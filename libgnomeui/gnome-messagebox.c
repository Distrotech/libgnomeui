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
#include "libgnome/gnome-defs.h"
#include "gnome-messagebox.h"
#include <string.h> /* for strcmp */
#include <gtk/gtk.h>
#include "libgnomeui/gnome-stock.h"

/* FIXME: define more globally.  */
#define GNOME_PAD 10


/* Library must use dgettext, not gettext.  */
#ifdef ENABLE_NLS
#    include <libintl.h>
#    define _(String) dgettext (PACKAGE, String)
#    ifdef gettext_noop
#        define N_(String) gettext_noop (String)
#    else
#        define N_(String) (String)
#    endif
#else
/* Stubs that do something close enough.  */
#    define textdomain(String) (String)
#    define gettext(String) (String)
#    define dgettext(Domain,Message) (Message)
#    define dcgettext(Domain,Message,Type) (Message)
#    define bindtextdomain(Domain,Directory) (Domain)
#    define _(String) (String)
#    define N_(String) (String)
#endif


enum {
	CLICKED,
	LAST_SIGNAL
};

typedef void (*GnomeMessageBoxSignal1) (GtkObject *object,
				        gint       arg1,
				        gpointer   data);

static void gnome_message_box_marshal_signal_1 (GtkObject         *object,
						GtkSignalFunc      func,
						gpointer           func_data,
						GtkArg            *args);

static void gnome_message_box_class_init (GnomeMessageBoxClass *klass);
static void gnome_message_box_init       (GnomeMessageBox      *messagebox);

static void gnome_message_box_button_clicked (GtkWidget   *button, 
					      GtkWidget   *messagebox);

static GtkWindowClass *parent_class;
static gint message_box_signals[LAST_SIGNAL] = { 0 };

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

		message_box_type = gtk_type_unique (gtk_window_get_type (), &message_box_info);
	}

	return message_box_type;
}

static void
gnome_message_box_class_init (GnomeMessageBoxClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	GtkWindowClass *window_class;

	object_class = (GtkObjectClass*) klass;
	widget_class = (GtkWidgetClass*) klass;
	window_class = (GtkWindowClass*) klass;

	parent_class = gtk_type_class (gtk_window_get_type ());

	message_box_signals[CLICKED] =
		gtk_signal_new ("clicked",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeMessageBoxClass, clicked),
				gnome_message_box_marshal_signal_1,
				GTK_TYPE_NONE, 1, GTK_TYPE_INT);

	gtk_object_class_add_signals (object_class, message_box_signals, LAST_SIGNAL);

}

static void
gnome_message_box_marshal_signal_1 (GtkObject      *object,
			           GtkSignalFunc   func,
			           gpointer        func_data,
			           GtkArg         *args)
{
	GnomeMessageBoxSignal1 rfunc;

	rfunc = (GnomeMessageBoxSignal1) func;

	(* rfunc) (object, GTK_VALUE_INT (args[0]), func_data);
}

static void
gnome_message_box_init (GnomeMessageBox *message_box)
{
	message_box->modal = FALSE;
}

GtkWidget*
gnome_message_box_new (gchar           *message,
		      gchar           *message_box_type, ...)
{
	va_list ap;
	GnomeMessageBox *message_box;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *separator;

	GtkWidget *pixmapwid;
	GdkPixmap *pixmap;
	GdkBitmap *mask;                 
	GtkStyle *style;

	va_start (ap, message_box_type);
	
	message_box = gtk_type_new (gnome_message_box_get_type ());

	gtk_window_set_policy (GTK_WINDOW (message_box), FALSE, FALSE, FALSE);
	gtk_widget_set_usize (GTK_WIDGET (message_box), GNOME_MESSAGE_BOX_WIDTH,
			      GNOME_MESSAGE_BOX_HEIGHT);
	gtk_container_border_width (GTK_CONTAINER (message_box), 
				    GNOME_MESSAGE_BOX_BORDER_WIDTH);

	style = gtk_widget_get_style (GTK_WIDGET (message_box));

	if (strcmp(GNOME_MESSAGE_BOX_INFO, message_box_type) == 0)
	{
		gtk_window_set_title (GTK_WINDOW (message_box), _("Information"));
		pixmap = gdk_pixmap_create_from_xpm (GTK_WIDGET (message_box)->window, 
						     &mask, 
						     &style->bg[GTK_STATE_NORMAL],
						     "bomb.xpm");
	}
	else if (strcmp(GNOME_MESSAGE_BOX_WARNING, message_box_type) == 0)
	{
		gtk_window_set_title (GTK_WINDOW (message_box), _("Warning"));
		pixmap = gdk_pixmap_create_from_xpm (GTK_WIDGET (message_box)->window,
						     &mask, 
						     &style->bg[GTK_STATE_NORMAL],
						     "bomb.xpm");
	}
	else if (strcmp(GNOME_MESSAGE_BOX_ERROR, message_box_type) == 0)
	{
		gtk_window_set_title (GTK_WINDOW (message_box), _("Error"));
		pixmap = gdk_pixmap_create_from_xpm (GTK_WIDGET (message_box)->window, 
						     &mask, 
						     &style->bg[GTK_STATE_NORMAL],
						     "bomb.xpm");
	}
	else if (strcmp(GNOME_MESSAGE_BOX_QUESTION, message_box_type) == 0)
	{
		gtk_window_set_title (GTK_WINDOW (message_box), _("Question"));
		pixmap = gdk_pixmap_create_from_xpm (GTK_WIDGET (message_box)->window, 
						     &mask, 
						     &style->bg[GTK_STATE_NORMAL],
						     "bomb.xpm");
	}
	else
	{
		gtk_window_set_title (GTK_WINDOW (message_box), "");
		pixmap = NULL;
	}

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (message_box), vbox);
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
			    GNOME_MESSAGE_BOX_BORDER_WIDTH);
	gtk_widget_show (separator);

	hbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox),
				   GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbox), GNOME_PAD);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
	gtk_widget_show (hbox);

	message_box->buttons = NULL;
	for (;;) {
		GtkWidget *button;
		char *text = va_arg (ap, char *);

		if (text == NULL)
			break;

		button = gnome_stock_or_ordinary_button (text);
		GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
		gtk_widget_set_usize (button, GNOME_MESSAGE_BOX_BUTTON_WIDTH,
				      GNOME_MESSAGE_BOX_BUTTON_HEIGHT);
		gtk_container_add (GTK_CONTAINER (hbox), button);
		gtk_widget_grab_default (button);
		gtk_widget_show (button);

		gtk_signal_connect (GTK_OBJECT (button), "clicked",
				    (GtkSignalFunc) gnome_message_box_button_clicked,
				    message_box);

		message_box->buttons = g_list_append (message_box->buttons, button);
	}

	va_end (ap);

	return GTK_WIDGET (message_box);
}

void
gnome_message_box_set_modal (GnomeMessageBox     *message_box)
{
	message_box->modal = TRUE;
	gtk_grab_add (GTK_WIDGET (message_box));
}

void
gnome_message_box_set_default (GnomeMessageBox     *message_box,
                              gint                button)
{
	GList *list = g_list_nth (message_box->buttons, button);

	if (list && list->data)
		gtk_widget_grab_default (GTK_WIDGET (list->data));
}

static void
gnome_message_box_button_clicked (GtkWidget   *button, 
			         GtkWidget   *message_box)
{
	GList *list = GNOME_MESSAGE_BOX (message_box)->buttons;
	int which = 0;

	while (list){
		if (list->data == button)
			gtk_signal_emit (GTK_OBJECT (message_box), message_box_signals[CLICKED], which);	
		list = list->next;
		which ++;
	}

	g_list_free (GNOME_MESSAGE_BOX (message_box)->buttons);
	gtk_widget_destroy (message_box);
}
