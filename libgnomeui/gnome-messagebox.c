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

static void gnome_messagebox_marshal_signal_1 (GtkObject         *object,
					       GtkSignalFunc      func,
					       gpointer           func_data,
					       GtkArg            *args);

static void gnome_messagebox_class_init (GnomeMessageBoxClass *klass);
static void gnome_messagebox_init       (GnomeMessageBox      *messagebox);

static void gnome_messagebox_button_clicked (GtkWidget   *button, 
					     GtkWidget   *messagebox);

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
		      gchar           *messagebox_type, ...)
{
	va_list ap;
	GnomeMessageBox *messagebox;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *separator;

	GtkWidget *pixmapwid;
	GdkPixmap *pixmap;
	GdkBitmap *mask;                 
	GtkStyle *style;

	va_start (ap, messagebox_type);
	
	messagebox = gtk_type_new (gnome_messagebox_get_type ());

	gtk_window_set_policy (GTK_WINDOW (messagebox), FALSE, FALSE, FALSE);
	gtk_widget_set_usize (GTK_WIDGET (messagebox), GNOME_MESSAGEBOX_WIDTH,
			      GNOME_MESSAGEBOX_HEIGHT);
	gtk_container_border_width (GTK_CONTAINER (messagebox), 
				    GNOME_MESSAGEBOX_BORDER_WIDTH);

	style = gtk_widget_get_style (GTK_WIDGET (messagebox));

	if (strcmp(GNOME_MESSAGEBOX_INFO, messagebox_type) == 0)
	{
		gtk_window_set_title (GTK_WINDOW (messagebox), _("Information"));
		pixmap = gdk_pixmap_create_from_xpm (GTK_WIDGET (messagebox)->window, 
						     &mask, 
						     &style->bg[GTK_STATE_NORMAL],
						     "bomb.xpm");
	}
	else if (strcmp(GNOME_MESSAGEBOX_WARNING, messagebox_type) == 0)
	{
		gtk_window_set_title (GTK_WINDOW (messagebox), _("Warning"));
		pixmap = gdk_pixmap_create_from_xpm (GTK_WIDGET (messagebox)->window,
						     &mask, 
						     &style->bg[GTK_STATE_NORMAL],
						     "bomb.xpm");
	}
	else if (strcmp(GNOME_MESSAGEBOX_ERROR, messagebox_type) == 0)
	{
		gtk_window_set_title (GTK_WINDOW (messagebox), _("Error"));
		pixmap = gdk_pixmap_create_from_xpm (GTK_WIDGET (messagebox)->window, 
						     &mask, 
						     &style->bg[GTK_STATE_NORMAL],
						     "bomb.xpm");
	}
	else if (strcmp(GNOME_MESSAGEBOX_QUESTION, messagebox_type) == 0)
	{
		gtk_window_set_title (GTK_WINDOW (messagebox), _("Question"));
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

	messagebox->buttons = NULL;
	for (;;) {
		GtkWidget *button;
		char *text = va_arg (ap, char *);

		if (text == NULL)
			break;

		button = gnome_stock_or_ordinary_button (text);
		GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
		gtk_widget_set_usize (button, GNOME_MESSAGEBOX_BUTTON_WIDTH,
				      GNOME_MESSAGEBOX_BUTTON_HEIGHT);
		gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 0);
		gtk_widget_grab_default (button);
		gtk_widget_show (button);
		
		gtk_signal_connect (GTK_OBJECT (button), "clicked",
				    (GtkSignalFunc) gnome_messagebox_button_clicked,
				    messagebox);

		messagebox->buttons = g_list_append (messagebox->buttons, button);
	}

	va_end (ap);

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
	GList *list = g_list_nth (messagebox->buttons, button);

	if (list && list->data)
		gtk_widget_grab_default (GTK_WIDGET (list->data));
}

static void
gnome_messagebox_button_clicked (GtkWidget   *button, 
			         GtkWidget   *messagebox)
{
	GList *list = GNOME_MESSAGEBOX (messagebox)->buttons;
	int which = 0;

	while (list){
		if (list->data == button)
			gtk_signal_emit (GTK_OBJECT (messagebox), messagebox_signals[CLICKED], which);	
		list = list->next;
		which ++;
	}

	g_list_free (GNOME_MESSAGEBOX (messagebox)->buttons);
	gtk_widget_destroy (messagebox);
}
