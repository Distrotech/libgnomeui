/* gnome-propertybox.c - Property dialog box.

   Copyright (C) 1998 Tom Tromey

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License
   as published by the Free Software Foundation; either version 2, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

/* Note that the property box is constructed so that we could later
   change how the buttons work.  For instance, we could put an Apply
   button inside each page; this kind of Apply button would only
   affect the current page.  Please do not change the API in a way
   that would violate this goal.  */

#include <config.h>

#include <gtk/gtk.h>
#include <gnome.h>

/* Library must use dgettext, not gettext.  */
#ifdef ENABLE_NLS
#    include <libintl.h>
#    undef _
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


/* FIXME: define more globally.  */
#define GNOME_PAD 10


enum
{
	APPLY,
	HELP,
	LAST_SIGNAL
};

typedef void (*GnomePropertyBoxSignal) (GtkObject *object,
					gint arg, gpointer data);

static void gnome_property_box_class_init (GnomePropertyBoxClass *klass);
static void gnome_property_box_init (GnomePropertyBox *property_box);
static void gnome_property_box_marshal_signal (GtkObject *object,
					       GtkSignalFunc func,
					       gpointer func_data,
					       GtkArg *args);
static void gnome_property_box_destroy (GtkObject *object);

static void global_apply (GtkObject *button, GnomePropertyBox *property_box);
static void help (GtkObject *button, GnomePropertyBox *property_box);
static void apply_and_close (GtkObject *button,
			     GnomePropertyBox *property_box);
static void just_close (GtkObject *button, GnomePropertyBox *property_box);


static GtkWindowClass *parent_class = NULL;

static gint property_box_signals[LAST_SIGNAL] = { 0 };

guint
gnome_property_box_get_type (void)
{
	static guint property_box_type = 0;

	if (! property_box_type) {
		GtkTypeInfo property_box_info = {
			"GnomePropertyBox",
			sizeof (GnomePropertyBox),
			sizeof (GnomePropertyBoxClass),
			(GtkClassInitFunc) gnome_property_box_class_init,
			(GtkObjectInitFunc) gnome_property_box_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		property_box_type = gtk_type_unique (gtk_window_get_type (),
						     &property_box_info);
	}

	return property_box_type;
}

static void
gnome_property_box_class_init (GnomePropertyBoxClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	GtkWindowClass *window_class;

	object_class = (GtkObjectClass*) klass;
	widget_class = (GtkWidgetClass*) klass;
	window_class = (GtkWindowClass*) klass;

	object_class->destroy = gnome_property_box_destroy;

	parent_class = gtk_type_class (gtk_window_get_type ());

	property_box_signals[APPLY] =
		gtk_signal_new ("apply",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomePropertyBoxClass,
						   apply),
				gnome_property_box_marshal_signal,
				GTK_TYPE_NONE, 1, GTK_TYPE_INT);
	property_box_signals[HELP] =
		gtk_signal_new ("help",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomePropertyBoxClass,
						   help),
				gnome_property_box_marshal_signal,
				GTK_TYPE_NONE, 1, GTK_TYPE_INT);

	gtk_object_class_add_signals (object_class, property_box_signals,
				      LAST_SIGNAL);
}

static void
gnome_property_box_marshal_signal (GtkObject *object,
				   GtkSignalFunc func,
				   gpointer func_data,
				   GtkArg *args)
{
	GnomePropertyBoxSignal rfunc;

	rfunc = (GnomePropertyBoxSignal) func;
	(* rfunc) (object, GTK_VALUE_INT (args[0]), func_data);
}

static void
gnome_property_box_init (GnomePropertyBox *property_box)
{
	GtkWidget *vbox, *hbox, *bf;

	property_box->notebook = gtk_notebook_new ();

	property_box->ok_button = gnome_stock_button (GNOME_STOCK_BUTTON_OK);
	property_box->apply_button
		= gnome_stock_button (GNOME_STOCK_BUTTON_APPLY);
	property_box->cancel_button
		= gnome_stock_button (GNOME_STOCK_BUTTON_CLOSE);
	property_box->help_button
		= gnome_stock_button (GNOME_STOCK_BUTTON_HELP);

	property_box->items = NULL;

	gtk_widget_set_sensitive (property_box->ok_button, FALSE);
	gtk_widget_set_sensitive (property_box->apply_button, FALSE);

	gtk_signal_connect (GTK_OBJECT (property_box->ok_button), "clicked",
			    GTK_SIGNAL_FUNC (apply_and_close), property_box);
	gtk_signal_connect (GTK_OBJECT (property_box->apply_button), "clicked",
			    GTK_SIGNAL_FUNC (global_apply), property_box);
	gtk_signal_connect (GTK_OBJECT (property_box->cancel_button),
			    "clicked",
			    GTK_SIGNAL_FUNC (just_close), property_box);
	gtk_signal_connect (GTK_OBJECT (property_box->help_button), "clicked",
			    GTK_SIGNAL_FUNC (help), property_box);

	/* FIXME: connect delete_event to run just_close?  */

	hbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox),
				   GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbox), GNOME_PAD);
	gtk_container_border_width (GTK_CONTAINER (hbox), GNOME_PAD);

	bf = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (bf), GTK_SHADOW_OUT);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (property_box), vbox);

	gtk_box_pack_start (GTK_BOX (vbox), property_box->notebook,
			    TRUE, TRUE, 0);

	gtk_container_add (GTK_CONTAINER (hbox), property_box->ok_button);
	gtk_container_add (GTK_CONTAINER (hbox), property_box->apply_button);
	gtk_container_add (GTK_CONTAINER (hbox), property_box->cancel_button);
	gtk_container_add (GTK_CONTAINER (hbox), property_box->help_button);

	gtk_container_add (GTK_CONTAINER (bf), hbox);
	gtk_box_pack_end (GTK_BOX (vbox), bf, FALSE, FALSE, 0);

	gtk_widget_show (property_box->ok_button);
	gtk_widget_show (property_box->apply_button);
	gtk_widget_show (property_box->cancel_button);
	gtk_widget_show (property_box->help_button);

	gtk_widget_show (hbox);
	gtk_widget_show (bf);
	gtk_widget_show (property_box->notebook);
	gtk_widget_show (vbox);
}

static void
gnome_property_box_destroy (GtkObject *object)
{
	GnomePropertyBox *property_box;
	GList *list;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_PROPERTY_BOX (object));

	property_box = GNOME_PROPERTY_BOX (object);

	for (list = property_box->items; list; list = list->next) {
		g_free ((GnomePropertyBoxItem *) list->data);
	}
	g_list_free (property_box->items);

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gnome_property_box_new (void)
{
	return gtk_type_new (gnome_property_box_get_type ());
}

static void
set_sensitive (GnomePropertyBox *property_box, GnomePropertyBoxItem *item)
{
	gtk_widget_set_sensitive (property_box->ok_button, item->dirty);
	gtk_widget_set_sensitive (property_box->apply_button, item->dirty);
}

void
gnome_property_box_changed (GnomePropertyBox *property_box)
{
	GList *list;
	GnomePropertyBoxItem *item;
	gint page;

	page =
	  gtk_notebook_current_page (GTK_NOTEBOOK (property_box->notebook));
	g_assert (page != -1);

	list = g_list_nth (property_box->items, page);
	g_assert (list);

	item = (GnomePropertyBoxItem *) list->data;
	item->dirty = TRUE;

	set_sensitive (property_box, item);
}

/* This is connected directly to the Apply button.  */
static void
global_apply (GtkObject *button, GnomePropertyBox *property_box)
{
	GList *list;
	GnomePropertyBoxItem *item;
	int n = 0;

	for (list = property_box->items; list != NULL; list = list->next) {
		item = (GnomePropertyBoxItem *) list->data;
		/* FIXME: there should be a way to report an error
		   during Apply.  That way we could prevent closing
		   the window if there were a problem.  */
		if (item->dirty) {
			gtk_signal_emit (GTK_OBJECT (property_box),
					 property_box_signals[APPLY], n);
			item->dirty = FALSE;
		}
		++n;
	}

	/* Emit an apply signal with a button of -1.  This means we
	   just finished a global apply.  Is this a hack?  */
	gtk_signal_emit (GTK_OBJECT (property_box),
			 property_box_signals[APPLY], -1);

	/* Doesn't matter which item we use.  */
	set_sensitive (property_box, item);
}

/* This is connected to the Help button.  */
static void
help (GtkObject *button, GnomePropertyBox *property_box)
{
	gint page;

	page =
	  gtk_notebook_current_page (GTK_NOTEBOOK (property_box->notebook));
	gtk_signal_emit (GTK_OBJECT (property_box), property_box_signals[HELP],
			 page);
}

/* This is connected to the Cancel button.  */
static void
just_close (GtkObject *button, GnomePropertyBox *property_box)
{
	int delete_handled = FALSE;

	gtk_signal_emit_by_name (GTK_OBJECT (property_box), "delete_event",
				 NULL, &delete_handled);
	if (!delete_handled)
		gtk_widget_destroy (GTK_WIDGET (property_box));
}

/* This is connected to the OK button.  */
static void
apply_and_close (GtkObject *button, GnomePropertyBox *property_box)
{
	global_apply (button, property_box);
	just_close (button, property_box);
}

gint
gnome_property_box_append_page (GnomePropertyBox *property_box,
				GtkWidget *child,
				GtkWidget *tab_label)
{
	GnomePropertyBoxItem *item;

	gtk_notebook_append_page (GTK_NOTEBOOK (property_box->notebook),
				  child, tab_label);

	item = g_new (GnomePropertyBoxItem, 1);
	item->dirty = FALSE;
	property_box->items = g_list_append (property_box->items, item);

	return g_list_length (property_box->items) - 1;
}
