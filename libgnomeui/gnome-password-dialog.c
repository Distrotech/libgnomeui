/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gnome-password-dialog.c - A use password prompting dialog widget.

   Copyright (C) 1999, 2000 Eazel, Inc.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the ree Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: Ramiro Estrugo <ramiro@eazel.com>
*/

#include <config.h>
#include "gnome-i18nP.h"
#include "gnome-password-dialog.h"
#include "gnome-stock-icons.h"
#include <gtk/gtkbox.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkstock.h>
#include <gtk/gtktable.h>
#include <gtk/gtkvbox.h>

struct GnomePasswordDialogDetails
{
	/* Attributes */
	char *username;
	char *password;
	gboolean readonly_username;
	gboolean remember;
	char *remember_label_text;

	/* Internal widgetry and flags */
	GtkWidget *username_entry;
	GtkWidget *password_entry;
	
	GtkWidget *table;
	GtkWidget *remember_button;
};

/* Caption table rows indices */
static const guint CAPTION_TABLE_USERNAME_ROW = 0;
static const guint CAPTION_TABLE_PASSWORD_ROW = 1;

/* Layout constants */
static const guint DIALOG_BORDER_WIDTH = 6;
static const guint CAPTION_TABLE_BORDER_WIDTH = 4;

/* GnomePasswordDialogClass methods */
static void gnome_password_dialog_class_init (GnomePasswordDialogClass *password_dialog_class);
static void gnome_password_dialog_init       (GnomePasswordDialog      *password_dialog);

/* GObjectClass methods */
static void gnome_password_dialog_finalize         (GObject                *object);


/* GtkDialog callbacks */
static void dialog_show_callback                 (GtkWidget              *widget,
						  gpointer                callback_data);
static void dialog_close_callback                (GtkWidget              *widget,
						  gpointer                callback_data);

static gpointer parent_class;

GtkType
gnome_password_dialog_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (GnomePasswordDialogClass),
                        NULL, NULL,
			(GClassInitFunc) gnome_password_dialog_class_init,
                        NULL, NULL,
			sizeof (GnomePasswordDialog), 0,
			(GInstanceInitFunc) gnome_password_dialog_init,
			NULL
		};

                type = g_type_register_static (gtk_dialog_get_type(), 
					       "GnomePasswordDialog", 
					       &info, 0);

		parent_class = g_type_class_ref (gtk_dialog_get_type());
	}

	return type;
}


static void
gnome_password_dialog_class_init (GnomePasswordDialogClass * klass)
{
	G_OBJECT_CLASS (klass)->finalize = gnome_password_dialog_finalize;
}

static void
gnome_password_dialog_init (GnomePasswordDialog *password_dialog)
{
	password_dialog->details = g_new0 (GnomePasswordDialogDetails, 1);
}

/* GObjectClass methods */
static void
gnome_password_dialog_finalize (GObject *object)
{
	GnomePasswordDialog *password_dialog;
	
	password_dialog = GNOME_PASSWORD_DIALOG (object);
	
	g_free (password_dialog->details->username);
	g_free (password_dialog->details->password);
	g_free (password_dialog->details->remember_label_text);
	g_free (password_dialog->details);

	if (G_OBJECT_CLASS (parent_class)->finalize != NULL)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

/* GtkDialog callbacks */
static void
dialog_show_callback (GtkWidget *widget, gpointer callback_data)
{
	GnomePasswordDialog *password_dialog;

	password_dialog = GNOME_PASSWORD_DIALOG (callback_data);

	if (!password_dialog->details->readonly_username) {
		gtk_widget_grab_focus (password_dialog->details->username_entry);
	}
	else {
		gtk_widget_grab_focus (password_dialog->details->password_entry);
	}
}

static void
dialog_close_callback (GtkWidget *widget, gpointer callback_data)
{
	gtk_widget_hide (widget);
}

static void
add_row (GtkWidget *table, int row, const char *label_text, GtkWidget *entry)
{
	GtkWidget *label;

	label = gtk_label_new_with_mnemonic (label_text);
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

	gtk_table_attach (GTK_TABLE (table), label,
			  0, 1,
			  row, row + 1,
			  GTK_FILL,
			  (GTK_FILL|GTK_EXPAND),
			  0, 0);
	

	gtk_table_attach (GTK_TABLE (table), entry,
			  1, 2,
			  row, row + 1,
			  (GTK_FILL|GTK_EXPAND),
			  (GTK_FILL|GTK_EXPAND),
			  0, 0);

	gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
}

static void
username_entry_activate (GtkWidget *widget, GtkWidget *dialog)
{
	GnomePasswordDialog *password_dialog;

	password_dialog = GNOME_PASSWORD_DIALOG (dialog);
	
	if (GTK_WIDGET_SENSITIVE (password_dialog->details->password_entry))
		gtk_widget_grab_focus (password_dialog->details->password_entry);
}

/* Public GnomePasswordDialog methods */
GtkWidget *
gnome_password_dialog_new (const char	*dialog_title,
			   const char	*message,
			   const char	*username,
			   const char	*password,
			   gboolean	 readonly_username)
{
	GnomePasswordDialog *password_dialog;
	GtkWidget *table;
	GtkLabel *message_label;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *dialog_icon;

	password_dialog = GNOME_PASSWORD_DIALOG (gtk_widget_new (gnome_password_dialog_get_type (), NULL));

	gtk_window_set_title (GTK_WINDOW (password_dialog), dialog_title);
	gtk_dialog_add_buttons (GTK_DIALOG (password_dialog),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OK, GTK_RESPONSE_OK,
				NULL);

	/* Setup the dialog */
	gtk_dialog_set_has_separator (GTK_DIALOG (password_dialog), FALSE);

 	gtk_window_set_position (GTK_WINDOW (password_dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_modal (GTK_WINDOW (password_dialog), TRUE);

 	gtk_container_set_border_width (GTK_CONTAINER (password_dialog), DIALOG_BORDER_WIDTH);

	gtk_dialog_set_default_response (GTK_DIALOG (password_dialog), GTK_RESPONSE_OK);

	g_signal_connect (password_dialog, "show",
			  G_CALLBACK (dialog_show_callback), password_dialog);
	g_signal_connect (password_dialog, "close",
			  G_CALLBACK (dialog_close_callback), password_dialog);

	/* The table that holds the captions */
	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_table_set_row_spacings (GTK_TABLE (table), 12);
	
	password_dialog->details->username_entry = gtk_entry_new ();
	password_dialog->details->password_entry = gtk_entry_new ();
	gtk_entry_set_visibility (GTK_ENTRY (password_dialog->details->password_entry), FALSE);

	g_signal_connect (password_dialog->details->username_entry,
			  "activate",
			  G_CALLBACK (username_entry_activate),
			  password_dialog);

	g_signal_connect_swapped (password_dialog->details->password_entry,
				  "activate",
				  G_CALLBACK (gtk_window_activate_default),
				  password_dialog);
	add_row (table, 0, _("_Username:"), password_dialog->details->username_entry);
	add_row (table, 1, _("_Password:"), password_dialog->details->password_entry);
	
	/* Adds some eye-candy to the dialog */
	hbox = gtk_hbox_new (FALSE, 12);
	dialog_icon = gtk_image_new_from_stock (GNOME_STOCK_AUTHENTICATION, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (dialog_icon), 0.5, 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), dialog_icon, FALSE, FALSE, 0);

	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (password_dialog)->vbox), 12);
 	gtk_container_set_border_width (GTK_CONTAINER(hbox), 6);
	gtk_box_set_spacing (GTK_BOX (hbox), 12);

	/* Fills the vbox */
	vbox = gtk_vbox_new (FALSE, 0);

	if (message) {
		message_label = GTK_LABEL (gtk_label_new (message));
		gtk_label_set_justify (message_label, GTK_JUSTIFY_LEFT);
		gtk_label_set_line_wrap (message_label, TRUE);

		gtk_box_pack_start (GTK_BOX (vbox),
				    GTK_WIDGET (message_label),
				    TRUE,	/* expand */
				    TRUE,	/* fill */
				    5);		/* padding */
	}
	gtk_box_pack_start (GTK_BOX (vbox), table, 
			    TRUE, TRUE, 0);

	/* Configure the table */
	gtk_container_set_border_width (table, CAPTION_TABLE_BORDER_WIDTH);

	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 5);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (password_dialog)->vbox),
			    hbox,
			    TRUE,	/* expand */
			    TRUE,	/* fill */
			    0);       	/* padding */
	
	gtk_widget_show_all (GTK_DIALOG (password_dialog)->vbox);
	
	gnome_password_dialog_set_username (password_dialog, username);
	gnome_password_dialog_set_password (password_dialog, password);
	gnome_password_dialog_set_readonly_username (password_dialog, readonly_username);
	
	return GTK_WIDGET (password_dialog);
}

gboolean
gnome_password_dialog_run_and_block (GnomePasswordDialog *password_dialog)
{
	gint button_clicked;

	g_return_val_if_fail (password_dialog != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog), FALSE);

	button_clicked = gtk_dialog_run (GTK_DIALOG (password_dialog));
	gtk_widget_hide (GTK_WIDGET (password_dialog));

	return button_clicked == GTK_RESPONSE_OK;
}

void
gnome_password_dialog_set_username (GnomePasswordDialog	*password_dialog,
				       const char		*username)
{
	g_return_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog));

	gtk_entry_set_text (GTK_ENTRY (password_dialog->details->username_entry),
			    username);
}

void
gnome_password_dialog_set_password (GnomePasswordDialog	*password_dialog,
				       const char		*password)
{
	g_return_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog));

	gtk_entry_set_text (GTK_ENTRY (password_dialog->details->password_entry),
			    password);
}

void
gnome_password_dialog_set_readonly_username (GnomePasswordDialog	*password_dialog,
						gboolean		readonly)
{
	g_return_if_fail (password_dialog != NULL);
	g_return_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog));

	password_dialog->details->readonly_username = readonly;

	gtk_widget_set_sensitive (password_dialog->details->username_entry,
				  !readonly);
}

char *
gnome_password_dialog_get_username (GnomePasswordDialog *password_dialog)
{
	g_return_val_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog), NULL);

	return g_strdup (gtk_entry_get_text (GTK_ENTRY (password_dialog->details->username_entry)));
}

char *
gnome_password_dialog_get_password (GnomePasswordDialog *password_dialog)
{
	g_return_val_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog), NULL);

	return g_strdup (gtk_entry_get_text (GTK_ENTRY (password_dialog->details->password_entry)));
}

