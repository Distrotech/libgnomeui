/* GnomeNumberEntry widget - Combo box with "Calculator" button for setting
 * 			     the number
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: George Lebl <jirka@5z.com>,
 *	   Federico Mena <federico@nuclecu.unam.mx>
 *		(the file entry which was a base for this)
 */

#include <gtk/gtkbutton.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include "libgnome/libgnome.h"
#include "libgnomeui/libgnomeui.h"
#include "gnome-number-entry.h"


static void gnome_number_entry_class_init (GnomeNumberEntryClass *class);
static void gnome_number_entry_init       (GnomeNumberEntry      *nentry);
static void gnome_number_entry_finalize   (GtkObject             *object);


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
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
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
	object_class->finalize = gnome_number_entry_finalize;

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
		nentry = GNOME_NUMBER_ENTRY (gtk_object_get_data (GTK_OBJECT (dlg),"entry"));
		entry = gnome_number_entry_gtk_entry (nentry);

		if(*calc->result_string!=' ')
			gtk_entry_set_text (GTK_ENTRY (entry),
					    calc->result_string);
		else
			gtk_entry_set_text (GTK_ENTRY (entry),
					    &calc->result_string[1]);
	}
	gtk_widget_destroy (widget);
}

gdouble
gnome_number_entry_get_number(GnomeNumberEntry *nentry)
{
	GtkWidget *entry;
	char *text;
	gdouble r;

	entry = gnome_number_entry_gtk_entry (nentry);
	
	text = gtk_entry_get_text(GTK_ENTRY(entry));
	
	r=0;
	sscanf(text,"%lg",&r);
	
	return r;
}

static void
calc_realized(GtkWidget *w, gpointer data)
{
	GnomeNumberEntry *nentry = data;
	gnome_calculator_set(GNOME_CALCULATOR(w),
			     gnome_number_entry_get_number(nentry));
}

static void
browse_clicked (GtkWidget *widget, gpointer data)
{
	GnomeNumberEntry *nentry;
	GtkWidget *dlg;
	GtkWidget *calc;

	nentry = GNOME_NUMBER_ENTRY (data);
	
	dlg = gnome_dialog_new (nentry->calc_dialog_title
				? nentry->calc_dialog_title
				: _("Calculator"),
				GNOME_STOCK_BUTTON_OK,
				GNOME_STOCK_BUTTON_CANCEL,
				NULL);
	gnome_dialog_set_default(GNOME_DIALOG(dlg), 0);

	calc = gnome_calculator_new();
	gtk_signal_connect_after(GTK_OBJECT(calc),"realize",
				 GTK_SIGNAL_FUNC(calc_realized),
				 nentry);
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dlg)->vbox),
			   calc,TRUE,TRUE,0);

	gtk_object_set_data (GTK_OBJECT (dlg), "entry", nentry);
	gtk_object_set_data (GTK_OBJECT (dlg), "calc", calc);

	gtk_signal_connect (GTK_OBJECT (dlg), "clicked",
			    (GtkSignalFunc) calc_dialog_clicked,
			    NULL);

	/* add calculator accels to our window*/
#ifdef GTK_HAVE_ACCEL_GROUP
	gtk_window_add_accel_group(GTK_WINDOW(dlg),
				   GNOME_CALCULATOR(calc)->accel);
#else
	gtk_window_add_accelerator_table(GTK_WINDOW(dlg),
					 GNOME_CALCULATOR(calc)->accel);
#endif


	gtk_widget_show_all (dlg);
}

static void
gnome_number_entry_init (GnomeNumberEntry *nentry)
{
	GtkWidget *button;

	nentry->calc_dialog_title = NULL;

	gtk_box_set_spacing (GTK_BOX (nentry), 4);

	nentry->gentry = gnome_entry_new (NULL);
	
	gtk_box_pack_start (GTK_BOX (nentry), nentry->gentry, TRUE, TRUE, 0);
	gtk_widget_show (nentry->gentry);

	button = gtk_button_new_with_label (_("Calculator..."));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    (GtkSignalFunc) browse_clicked,
			    nentry);
	gtk_box_pack_start (GTK_BOX (nentry), button, FALSE, FALSE, 0);
	gtk_widget_show (button);
}

static void
gnome_number_entry_finalize (GtkObject *object)
{
	GnomeNumberEntry *nentry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_NUMBER_ENTRY (object));

	nentry = GNOME_NUMBER_ENTRY (object);

	if (nentry->calc_dialog_title)
		g_free (nentry->calc_dialog_title);

	(* GTK_OBJECT_CLASS (parent_class)->finalize) (object);
}

GtkWidget *
gnome_number_entry_new (char *history_id, char *calc_dialog_title)
{
	GnomeNumberEntry *nentry;

	nentry = gtk_type_new (gnome_number_entry_get_type ());

	gnome_entry_set_history_id (GNOME_ENTRY (nentry->gentry), history_id);
	gnome_number_entry_set_title (nentry, calc_dialog_title);

	return GTK_WIDGET (nentry);
}

GtkWidget *
gnome_number_entry_gnome_entry (GnomeNumberEntry *nentry)
{
	g_return_val_if_fail (nentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_NUMBER_ENTRY (nentry), NULL);

	return nentry->gentry;
}

GtkWidget *
gnome_number_entry_gtk_entry (GnomeNumberEntry *nentry)
{
	g_return_val_if_fail (nentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_NUMBER_ENTRY (nentry), NULL);

	return gnome_entry_gtk_entry (GNOME_ENTRY (nentry->gentry));
}

void
gnome_number_entry_set_title (GnomeNumberEntry *nentry, char *calc_dialog_title)
{
	g_return_if_fail (nentry != NULL);
	g_return_if_fail (GNOME_IS_NUMBER_ENTRY (nentry));

	if (nentry->calc_dialog_title)
		g_free (nentry->calc_dialog_title);

	nentry->calc_dialog_title = g_strdup (calc_dialog_title); /* handles NULL correctly */
}
