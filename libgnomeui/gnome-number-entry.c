/*
 * Copyright (C) 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

/* GnomeNumberEntry widget - Combo box with "Calculator" button for setting
 * 			     the number
 *
 * Author: George Lebl <jirka@5z.com>,
 *	   Federico Mena <federico@nuclecu.unam.mx>
 *		(the file entry which was a base for this)
 */

#include <config.h>

#include <gtk/gtkbutton.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include "libgnome/libgnomeP.h"
#include "gnome-number-entry.h"
#include "gnome-calculator.h"
#include "gnome-dialog.h"
#include "gnome-stock.h"

struct _GnomeNumberEntryPrivate {
	gchar *calc_dialog_title;
	
	GtkWidget *calc_dlg;

	GtkWidget *gentry;
};

static void gnome_number_entry_class_init (GnomeNumberEntryClass *class);
static void gnome_number_entry_init       (GnomeNumberEntry      *nentry);
static void gnome_number_entry_destroy    (GtkObject             *object);


static GtkHBoxClass *parent_class;

guint
gnome_number_entry_get_type (void)
{
	static guint number_entry_type = 0;

	if (!number_entry_type) {
		GtkTypeInfo number_entry_info = {
			"GnomeNumberEntry",
			sizeof (GnomeNumberEntry),
			sizeof (GnomeNumberEntryClass),
			(GtkClassInitFunc) gnome_number_entry_class_init,
			(GtkObjectInitFunc) gnome_number_entry_init,
			NULL,
			NULL,
			NULL
		};

		number_entry_type = gtk_type_unique (gtk_hbox_get_type (), &number_entry_info);
	}

	return number_entry_type;
}

static void
gnome_number_entry_class_init (GnomeNumberEntryClass *class)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) class;
	parent_class = gtk_type_class (gtk_hbox_get_type ());
	object_class->destroy = gnome_number_entry_destroy;

}

static void
calc_dialog_clicked (GtkWidget *widget, int button, gpointer data)
{
	GnomeDialog *dlg = GNOME_DIALOG(widget);
	GnomeCalculator *calc =
		GNOME_CALCULATOR(gtk_object_get_data(GTK_OBJECT(dlg),"calc"));
	
	if(button == 0) {
		GnomeNumberEntry *nentry;
		GtkWidget *entry;
		char *result_string;

		nentry = GNOME_NUMBER_ENTRY (gtk_object_get_data (GTK_OBJECT (dlg),"entry"));
		entry = gnome_number_entry_gtk_entry (nentry);

		result_string = g_strdup(gnome_calculator_get_result_string(calc));
		if(result_string) {
			g_strstrip(result_string);
			gtk_entry_set_text (GTK_ENTRY (entry), result_string);
			g_free(result_string);
		} else
			gtk_entry_set_text (GTK_ENTRY (entry), "");
	}
	gtk_widget_destroy (widget);
}

/**
 * gnome_number_entry_get_number:
 * @nentry: Pointer to #GnomeNumberEntry widget
 *
 * Description: Get the current number from the entry
 *
 * Returns: Value currently in the entry.
 **/
gdouble
gnome_number_entry_get_number(GnomeNumberEntry *nentry)
{
	GtkWidget *entry;
	gchar *text;
	gdouble r;

	g_return_val_if_fail (nentry != NULL, 0.0);
	g_return_val_if_fail (GNOME_IS_NUMBER_ENTRY (nentry), 0.0);

	entry = gnome_number_entry_gtk_entry (nentry);
	
	text = gtk_entry_get_text(GTK_ENTRY(entry));
	
	r=0;
	sscanf(text,"%lg",&r);
	
	return r;
}

/**
 * gnome_number_entry_set_number:
 * @nentry: Pointer to #GnomeNumberEntry widget
 * @number: Number to set the entry to
 *
 * Description: Set the entry to @number
 **/
void
gnome_number_entry_set_number(GnomeNumberEntry *nentry, double number)
{
	GtkWidget *entry;
	gchar *text;

	g_return_if_fail (nentry != NULL);
	g_return_if_fail (GNOME_IS_NUMBER_ENTRY (nentry));

	entry = gnome_number_entry_gtk_entry (nentry);

	text = g_strdup_printf("%g", number);
	gtk_entry_set_text(GTK_ENTRY(entry), text);
	g_free(text);
}

static void
calc_dialog_destroyed(GtkWidget *w, GnomeNumberEntry *nentry)
{
	g_return_if_fail (nentry != NULL);
	g_return_if_fail (GNOME_IS_NUMBER_ENTRY (nentry));

	nentry->_priv->calc_dlg = NULL;
}

static void
browse_clicked (GtkWidget *widget, gpointer data)
{
	GnomeNumberEntry *nentry;
	GtkWidget *dlg;
	GtkWidget *calc;
        GtkWidget *parent;
	GtkAccelGroup *accel;
        
	g_return_if_fail (data != NULL);
	g_return_if_fail (GNOME_IS_NUMBER_ENTRY (data));

	nentry = GNOME_NUMBER_ENTRY (data);
	
	/*if dialog already exists, make sure it's shown and raised*/
	if(nentry->_priv->calc_dlg) {
		gtk_widget_show(nentry->_priv->calc_dlg);
		if(nentry->_priv->calc_dlg->window)
			gdk_window_raise(nentry->_priv->calc_dlg->window);
		calc = gtk_object_get_data (GTK_OBJECT (nentry->_priv->calc_dlg),
					    "calc");
		gnome_calculator_set(GNOME_CALCULATOR(calc),
				     gnome_number_entry_get_number(nentry));
		return;
	}

        parent = gtk_widget_get_toplevel(GTK_WIDGET(nentry));
        
	dlg = gnome_dialog_new (nentry->_priv->calc_dialog_title
				? nentry->_priv->calc_dialog_title
				: _("Calculator"),
				GNOME_STOCK_BUTTON_OK,
				GNOME_STOCK_BUTTON_CANCEL,
				NULL);
        if (parent)
                gnome_dialog_set_parent(GNOME_DIALOG(dlg), GTK_WINDOW(parent));
        
        
	gnome_dialog_set_default(GNOME_DIALOG(dlg), 0);

	calc = gnome_calculator_new();
	gnome_calculator_set(GNOME_CALCULATOR(calc),
			     gnome_number_entry_get_number(nentry));

	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dlg)->vbox),
			   calc,TRUE,TRUE,0);

	gtk_object_set_data (GTK_OBJECT (dlg), "entry", nentry);
	gtk_object_set_data (GTK_OBJECT (dlg), "calc", calc);

	gtk_signal_connect (GTK_OBJECT (dlg), "clicked",
			    (GtkSignalFunc) calc_dialog_clicked,
			    NULL);
	gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
			    (GtkSignalFunc) calc_dialog_destroyed,
			    nentry);

	/* add calculator accels to our window*/

	accel = gnome_calculator_get_accel_group(GNOME_CALCULATOR(calc));
	gtk_window_add_accel_group(GTK_WINDOW(dlg), accel);

	gtk_widget_show_all (dlg);
	
	nentry->_priv->calc_dlg = dlg;
}

static void
gnome_number_entry_init (GnomeNumberEntry *nentry)
{
	GtkWidget *button;

	g_return_if_fail (nentry != NULL);
	g_return_if_fail (GNOME_IS_NUMBER_ENTRY (nentry));

	nentry->_priv = g_new0(GnomeNumberEntryPrivate, 1);

	nentry->_priv->calc_dialog_title = NULL;

	gtk_box_set_spacing (GTK_BOX (nentry), 4);

	nentry->_priv->gentry = gnome_entry_new (NULL);

	nentry->_priv->calc_dlg = NULL;
	
	gtk_box_pack_start (GTK_BOX (nentry), nentry->_priv->gentry, TRUE, TRUE, 0);
	gtk_widget_show (nentry->_priv->gentry);

	button = gtk_button_new_with_label (_("Calculator..."));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    (GtkSignalFunc) browse_clicked,
			    nentry);
	gtk_box_pack_start (GTK_BOX (nentry), button, FALSE, FALSE, 0);
	gtk_widget_show (button);
}

static void
gnome_number_entry_destroy (GtkObject *object)
{
	GnomeNumberEntry *nentry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_NUMBER_ENTRY (object));

	nentry = GNOME_NUMBER_ENTRY (object);

	if (nentry->_priv->calc_dlg)
		gtk_widget_destroy(nentry->_priv->calc_dlg);
	nentry->_priv->calc_dlg = NULL;

	g_free (nentry->_priv->calc_dialog_title);
	nentry->_priv->calc_dialog_title = NULL;

	g_free(nentry->_priv);
	nentry->_priv = NULL;

	if(GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/**
 * gnome_number_entry_construct:
 * @nentry: Pointer to GnomeNumberEntry widget
 * @history_id: The history id given to #gnome_entry_new
 * @calc_dialog_title: Title of the calculator dialog
 *
 * Description: For language bindings and subclassing, use 
 * #gnome_number_entry_new from C instead.
 *
 * Returns:
 **/
void
gnome_number_entry_construct (GnomeNumberEntry *nentry,
			      const gchar *history_id,
			      const gchar *calc_dialog_title)
{
	g_return_if_fail (nentry != NULL);
	g_return_if_fail (GNOME_IS_NUMBER_ENTRY(nentry));

	gnome_entry_construct (GNOME_ENTRY (nentry->_priv->gentry), history_id);

	gnome_number_entry_set_title (nentry, calc_dialog_title);
}

/**
 * gnome_number_entry_new:
 * @history_id: The history id given to #gnome_entry_new
 * @calc_dialog_title: Title of the calculator dialog
 *
 * Description: Creates a new number entry widget, with a history id
 * and title for the calculator dialog.
 *
 * Returns: New widget
 **/
GtkWidget *
gnome_number_entry_new (const gchar *history_id,
			const gchar *calc_dialog_title)
{
	GnomeNumberEntry *nentry;

	nentry = gtk_type_new (gnome_number_entry_get_type ());

	gnome_number_entry_construct (nentry, history_id, calc_dialog_title);

	return GTK_WIDGET (nentry);
}

/**
 * gnome_number_entry_gnome_entry:
 * @nentry: Pointer to GnomeNumberEntry widget
 *
 * Description: Get the GnomeEntry component of the
 * GnomeNumberEntry for lower-level manipulation.
 *
 * Returns: GnomeEntry widget
 **/
GtkWidget *
gnome_number_entry_gnome_entry (GnomeNumberEntry *nentry)
{
	g_return_val_if_fail (nentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_NUMBER_ENTRY (nentry), NULL);

	return nentry->_priv->gentry;
}

/**
 * gnome_number_entry_gtk_entry:
 * @nentry: Pointer to GnomeNumberEntry widget
 *
 * Description: Get the GtkEntry component of the
 * GnomeNumberEntry for Gtk+-level manipulation.
 *
 * Returns: GtkEntry widget
 **/
GtkWidget *
gnome_number_entry_gtk_entry (GnomeNumberEntry *nentry)
{
	g_return_val_if_fail (nentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_NUMBER_ENTRY (nentry), NULL);

	return gnome_entry_gtk_entry (GNOME_ENTRY (nentry->_priv->gentry));
}

/**
 * gnome_number_entry_set_title:
 * @nentry: Pointer to GnomeNumberEntry widget
 * @calc_dialog_title: New title
 *
 * Description: Set the title of the calculator dialog to @calc_dialog_title.
 * Takes effect the next time the calculator button is pressed.
 *
 * Returns:
 **/
void
gnome_number_entry_set_title (GnomeNumberEntry *nentry, const gchar *calc_dialog_title)
{
	g_return_if_fail (nentry != NULL);
	g_return_if_fail (GNOME_IS_NUMBER_ENTRY (nentry));

	if (nentry->_priv->calc_dialog_title)
		g_free (nentry->_priv->calc_dialog_title);

	nentry->_priv->calc_dialog_title = g_strdup (calc_dialog_title); /* handles NULL correctly */
}
