/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gnome-password-dialog.c - A use password prompting dialog widget.

   Copyright (C) 1999, 2000 Eazel, Inc.
   Copyright (C) 2006 Christian Persch
   
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

#include <string.h>

#include "gnome-password-dialog.h"
#include "gnometypebuiltins.h"

#include <glib/gi18n-lib.h>
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
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtksizegroup.h>
#include <gtk/gtkprogressbar.h>

#define GNOME_PASSWORD_DIALOG_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GNOME_TYPE_PASSWORD_DIALOG, GnomePasswordDialogDetails))
#define PARAM_STATIC G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB

struct _GnomePasswordDialogDetails
{
	GtkWidget *message_label;
	GtkWidget *username_entry;
	GtkWidget *domain_entry;
	GtkWidget *password_entry;
	GtkWidget *new_password_entry;
	GtkWidget *confirm_new_password_entry;
	GtkWidget *quality_meter;
	GtkWidget *table_alignment;
	GtkWidget *table;

	GtkWidget *remember_session_button;
	GtkWidget *remember_forever_button;

	GtkWidget *radio_vbox;
	GtkWidget *connect_with_no_userpass_button;
	GtkWidget *connect_with_userpass_button;

	GnomePasswordDialogQualityFunc quality_func;
	gpointer quality_data;
	GDestroyNotify quality_dnotify;

	/* Attributes */
	guint readonly_username : 1;
	guint readonly_domain : 1;
	guint show_username : 1;
	guint show_domain : 1;
	guint show_password : 1;
	guint show_new_password : 1;
	guint show_new_password_quality : 1;
	guint show_remember : 1;
	guint show_userpass_buttons : 1;
	guint show_userpass_buttons_set : 1;
	guint anonymous : 1;
	guint anonymous_set : 1;
};

enum
{
	PROP_0,
	PROP_SHOW_USERNAME,
	PROP_SHOW_DOMAIN,
	PROP_SHOW_PASSWORD,
	PROP_SHOW_NEW_PASSWORD,
	PROP_SHOW_NEW_PASSWORD_QUALITY,
	PROP_SHOW_USERPASS_BUTTONS,
	PROP_SHOW_REMEMBER,
	PROP_READONLY_USERNAME,
	PROP_READONLY_DOMAIN,
	PROP_ANONYMOUS,
	PROP_REMEMBER_MODE,
	PROP_MESSAGE,
	PROP_MESSAGE_MARKUP,
	PROP_USERNAME,
	PROP_DOMAIN,
	PROP_PASSWORD,
	PROP_NEW_PASSWORD
};

/* GtkDialog methods */
static void gnome_password_dialog_show (GtkWidget              *widget);

G_DEFINE_TYPE (GnomePasswordDialog, gnome_password_dialog, GTK_TYPE_DIALOG);

static void
gnome_password_dialog_set_message (GnomePasswordDialog *password_dialog, const gchar *message, gboolean markup)
{
	GnomePasswordDialogDetails *priv = password_dialog->details;

	if (message != NULL && message[0] != '\0') {
		gtk_label_set_use_markup (GTK_LABEL (priv->message_label), markup);
		gtk_label_set_label (GTK_LABEL (priv->message_label), message);
		gtk_widget_show (priv->message_label);
	} else {
		gtk_label_set_label (GTK_LABEL (priv->message_label), "");
		gtk_widget_hide (priv->message_label);
	}
}

static void
gnome_password_dialog_set_anon (GnomePasswordDialog *password_dialog, gboolean anonymous, gboolean explicit)
{
	GnomePasswordDialogDetails *priv = password_dialog->details;

	priv->anonymous = anonymous;
	priv->anonymous_set = explicit;

	if (anonymous) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->connect_with_no_userpass_button), TRUE);
	} else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->connect_with_userpass_button), TRUE);
	}

	gtk_widget_set_sensitive (priv->table_alignment, !anonymous || !priv->show_userpass_buttons);

	g_object_notify (G_OBJECT (password_dialog), "anonymous");
}

static void
userpass_radio_button_toggled (GtkWidget *widget, GnomePasswordDialog *password_dialog)
{
	GnomePasswordDialogDetails *priv = password_dialog->details;

	priv->anonymous = widget == priv->connect_with_no_userpass_button;
	priv->anonymous_set = TRUE;

	gtk_widget_set_sensitive (priv->table_alignment, !priv->anonymous);
	
	g_object_notify (G_OBJECT (password_dialog),"anonymous");
}

static GtkWidget *
add_row (GtkWidget *table, int row, const char *label_text, GtkWidget *entry)
{
	GtkWidget *label;

	label = gtk_label_new_with_mnemonic (label_text);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

	gtk_table_attach_defaults (GTK_TABLE (table), label,
				   0, 1, row, row + 1);
	gtk_table_attach_defaults (GTK_TABLE (table), entry,
				   1, 2, row, row + 1);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

	return label;
}

static void
remove_child (GtkWidget *child, GtkContainer *container)
{
	gtk_container_remove (container, child);
}

static void
update_entries_table (GnomePasswordDialog *dialog)
{
	GnomePasswordDialogDetails *priv = dialog->details;
	int row = 0, n_rows;

	gtk_container_foreach (GTK_CONTAINER (priv->table),
			       (GtkCallback) remove_child, priv->table);

	n_rows = priv->show_username +
		 priv->show_domain +
		 priv->show_password +
		 priv->show_new_password * (2 + priv->show_new_password_quality);
	if (n_rows == 0) {
		gtk_widget_hide (priv->table);
		return;
	}

	gtk_table_resize (GTK_TABLE (priv->table), n_rows, 2);

	if (priv->show_username) {
		add_row (priv->table, row++, _("_Username:"), priv->username_entry);
	}
	if (priv->show_domain) {
		add_row (priv->table, row++, _("_Domain:"), priv->domain_entry);
	}
	if (priv->show_password) {
		add_row (priv->table, row++, _("_Password:"), priv->password_entry);
	}
	if (priv->show_new_password) {
		add_row (priv->table, row++, _("_New password:"), priv->new_password_entry);
		add_row (priv->table, row++, _("Con_firm password:"), priv->confirm_new_password_entry);
	}
	if (priv->show_new_password && priv->show_new_password_quality) {
		add_row (priv->table, row++, _("Password quality:"), priv->quality_meter);
	}

	gtk_widget_show_all (priv->table);
}

static void
username_entry_activate (GtkWidget *widget, GtkWidget *dialog)
{
	GnomePasswordDialog *password_dialog = GNOME_PASSWORD_DIALOG (dialog);
	GnomePasswordDialogDetails *priv = password_dialog->details;
	
	if (GTK_WIDGET_VISIBLE (priv->domain_entry) &&
	    GTK_WIDGET_IS_SENSITIVE (priv->domain_entry))
		gtk_widget_grab_focus (priv->domain_entry);
	else if (GTK_WIDGET_VISIBLE (priv->password_entry) &&
		 GTK_WIDGET_IS_SENSITIVE (priv->password_entry))
		gtk_widget_grab_focus (priv->password_entry);
}

static void
domain_entry_activate (GtkWidget *widget, GtkWidget *dialog)
{
	GnomePasswordDialog *password_dialog = GNOME_PASSWORD_DIALOG (dialog);
	GnomePasswordDialogDetails *priv = password_dialog->details;
	
	if (GTK_WIDGET_VISIBLE (priv->password_entry) &&
	    GTK_WIDGET_IS_SENSITIVE (priv->password_entry))
		gtk_widget_grab_focus (priv->password_entry);
	else if (priv->show_new_password &&
		 GTK_WIDGET_IS_SENSITIVE (priv->new_password_entry))
		gtk_widget_grab_focus (priv->new_password_entry);
}

static void
remember_button_toggled (GtkWidget *widget, GObject *dialog)
{
	g_object_notify (dialog, "remember-mode");
}

static void
password_entry_activate (GtkWidget *widget, GnomePasswordDialog *password_dialog)
{
	GnomePasswordDialogDetails *priv = password_dialog->details;

	if (!priv->show_new_password) {
		gtk_window_activate_default (GTK_WINDOW (password_dialog));
		return;
	}

	if (priv->show_new_password) {
		gtk_widget_grab_focus (priv->new_password_entry);
	}
}

static void
new_password_entry_activate (GtkWidget *widget, GnomePasswordDialog *password_dialog)
{
	GnomePasswordDialogDetails *priv = password_dialog->details;

	gtk_widget_grab_focus (priv->confirm_new_password_entry);
}

static void
new_password_entries_changed (GtkWidget *entry, GnomePasswordDialog *password_dialog)
{
	GnomePasswordDialogDetails *priv = password_dialog->details;
	const gchar *new_password, *confirm_password;
	gdouble quality = 0.0;
	gboolean accept;

	new_password = gtk_entry_get_text (GTK_ENTRY (priv->new_password_entry));
	confirm_password = gtk_entry_get_text (GTK_ENTRY (priv->confirm_new_password_entry));

	if (priv->quality_func) {
		quality = priv->quality_func (password_dialog,
					      new_password,
					      priv->quality_data);
	}

	accept = strcmp (new_password, confirm_password) == 0;
	/* && (!priv->show_new_password_quality || quality > 0.5); */

	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->quality_meter), quality);

	gtk_dialog_set_response_sensitive (GTK_DIALOG (password_dialog),
					   GTK_RESPONSE_OK, accept);
}

static void
gnome_password_dialog_init (GnomePasswordDialog *password_dialog)
{
	GnomePasswordDialogDetails *priv;
	GtkDialog *dialog = GTK_DIALOG (password_dialog);
	GtkWindow *window = GTK_WINDOW (password_dialog);
	GtkWidget *hbox, *main_vbox, *vbox, *icon;
	GSList *group;

	priv = password_dialog->details = GNOME_PASSWORD_DIALOG_GET_PRIVATE (password_dialog);

	priv->show_username = TRUE;
	priv->show_password = TRUE;

	/* Set the dialog up with HIG properties */
	gtk_dialog_set_has_separator (dialog, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (dialog->vbox), 2); /* 2 * 5 + 2 = 12 */
	gtk_container_set_border_width (GTK_CONTAINER (dialog->action_area), 5);
	gtk_box_set_spacing (GTK_BOX (dialog->action_area), 6);

	gtk_window_set_resizable (window, FALSE);
	gtk_window_set_icon_name (window, GTK_STOCK_DIALOG_AUTHENTICATION);

	gtk_dialog_add_buttons (dialog,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				_("Co_nnect"), GTK_RESPONSE_OK,
				NULL);
	gtk_dialog_set_default_response (dialog, GTK_RESPONSE_OK);

	/* Build contents */
	hbox = gtk_hbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_box_pack_start (GTK_BOX (dialog->vbox), hbox, FALSE, FALSE, 0);

	icon = gtk_image_new_from_stock (GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (icon), 0.5, 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);

	main_vbox = gtk_vbox_new (FALSE, 18);
	gtk_box_pack_start (GTK_BOX (hbox), main_vbox, FALSE, FALSE, 0);

	priv->message_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->message_label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (priv->message_label), TRUE);
	gtk_box_pack_start (GTK_BOX (main_vbox), GTK_WIDGET (priv->message_label),
			    FALSE, FALSE, 0);
	gtk_widget_set_no_show_all (priv->message_label, TRUE);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);

	/* the radio buttons for anonymous login */
	priv->radio_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), priv->radio_vbox,
			    FALSE, FALSE, 0);
	gtk_widget_set_no_show_all (priv->radio_vbox, TRUE);

	priv->connect_with_no_userpass_button =
		gtk_radio_button_new_with_mnemonic (NULL, _("Connect _anonymously"));
	group = gtk_radio_button_get_group (
		GTK_RADIO_BUTTON (priv->connect_with_no_userpass_button));
	priv->connect_with_userpass_button =
		gtk_radio_button_new_with_mnemonic (
			group, _("Connect as u_ser:"));

	g_signal_connect (priv->connect_with_no_userpass_button, "toggled",
			  G_CALLBACK (userpass_radio_button_toggled), password_dialog);
	g_signal_connect (priv->connect_with_userpass_button, "toggled",
			  G_CALLBACK (userpass_radio_button_toggled), password_dialog);	

	gtk_box_pack_start (GTK_BOX (priv->radio_vbox),
			    priv->connect_with_no_userpass_button,
			    FALSE, FALSE, 0);	
	gtk_box_pack_start (GTK_BOX (priv->radio_vbox),
			    priv->connect_with_userpass_button,
			    FALSE, FALSE, 0);
	gtk_widget_show (priv->connect_with_no_userpass_button);
	gtk_widget_show (priv->connect_with_userpass_button);

	/* The table that holds the entries */
	priv->table_alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_box_pack_start (GTK_BOX (vbox), priv->table_alignment, 
			    FALSE, FALSE, 0);

	priv->table = gtk_table_new (3, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (priv->table), 12);
	gtk_table_set_row_spacings (GTK_TABLE (priv->table), 6);
	gtk_container_add (GTK_CONTAINER (priv->table_alignment), priv->table);

	priv->username_entry = g_object_ref_sink (gtk_entry_new ());
	priv->domain_entry = g_object_ref_sink (gtk_entry_new ());
	priv->password_entry = g_object_ref_sink (gtk_entry_new ());
	gtk_entry_set_visibility (GTK_ENTRY (priv->password_entry), FALSE);
	priv->new_password_entry = g_object_ref_sink (gtk_entry_new ());
	gtk_entry_set_visibility (GTK_ENTRY (priv->new_password_entry), FALSE);
	priv->confirm_new_password_entry = g_object_ref_sink (gtk_entry_new ());
	gtk_entry_set_visibility (GTK_ENTRY (priv->confirm_new_password_entry), FALSE);

	g_signal_connect (priv->username_entry, "activate",
			  G_CALLBACK (username_entry_activate), password_dialog);
	g_signal_connect (priv->domain_entry, "activate",
			  G_CALLBACK (domain_entry_activate), password_dialog);
	g_signal_connect (priv->password_entry, "activate",
			  G_CALLBACK (password_entry_activate), password_dialog);
	g_signal_connect (priv->new_password_entry, "activate",
			  G_CALLBACK (new_password_entry_activate), password_dialog);
	g_signal_connect_swapped (priv->confirm_new_password_entry, "activate",
				  G_CALLBACK (gtk_window_activate_default),
				  password_dialog);
	g_signal_connect (priv->new_password_entry, "changed",
			  G_CALLBACK (new_password_entries_changed), password_dialog);
	g_signal_connect (priv->confirm_new_password_entry, "changed",
			  G_CALLBACK (new_password_entries_changed), password_dialog);

	priv->quality_meter = g_object_ref_sink (gtk_progress_bar_new ());
	
	update_entries_table (password_dialog);

	gtk_widget_show_all (hbox);

	priv->remember_session_button =
		gtk_check_button_new_with_mnemonic (_("_Remember password for this session"));
	g_signal_connect (priv->remember_session_button, "toggled",
			  G_CALLBACK (remember_button_toggled), password_dialog);
	priv->remember_forever_button =
		gtk_check_button_new_with_mnemonic (_("Save password in _keyring"));
	g_signal_connect (priv->remember_forever_button, "toggled",
			  G_CALLBACK (remember_button_toggled), password_dialog);

	gtk_box_pack_start (GTK_BOX (vbox), priv->remember_session_button, 
			    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), priv->remember_forever_button, 
			    FALSE, FALSE, 0);
}

static GObject *
gnome_password_dialog_constructor (GType type,
				   guint n_construct_properties,
				   GObjectConstructParam *construct_params)
{
	GObject *object;
	GnomePasswordDialog *dialog;
	GnomePasswordDialogDetails *priv;
	const gchar *username;

	object = G_OBJECT_CLASS (gnome_password_dialog_parent_class)->constructor
			(type, n_construct_properties, construct_params);

	dialog = GNOME_PASSWORD_DIALOG (object);
	priv = dialog->details;

	if (priv->show_userpass_buttons_set &&
	    !priv->anonymous_set) {
		username = gtk_entry_get_text (GTK_ENTRY (priv->username_entry));
		gnome_password_dialog_set_anon (dialog,
						username == NULL || username[0] == '\0', FALSE);
	}

	return object;
}

static void
gnome_password_dialog_finalize (GObject *object)
{
	GnomePasswordDialog *password_dialog = GNOME_PASSWORD_DIALOG (object);
	GnomePasswordDialogDetails *priv = password_dialog->details;

	g_object_unref (priv->username_entry);
	g_object_unref (priv->domain_entry);
	g_object_unref (priv->password_entry);
	g_object_unref (priv->new_password_entry);
	g_object_unref (priv->confirm_new_password_entry);
	g_object_unref (priv->quality_meter);

	if (priv->quality_data && priv->quality_dnotify) {
		priv->quality_dnotify (priv->quality_data);
	}

	G_OBJECT_CLASS (gnome_password_dialog_parent_class)->finalize (object);
}

static void
gnome_password_dialog_set_property (GObject *object,
				    guint prop_id,
				    const GValue *value,
				    GParamSpec *pspec)
{
	GnomePasswordDialog *dialog = GNOME_PASSWORD_DIALOG (object);

	switch (prop_id)
	{
		case PROP_SHOW_USERNAME:
			gnome_password_dialog_set_show_username (dialog, g_value_get_boolean (value));
			break;
		case PROP_SHOW_DOMAIN:
			gnome_password_dialog_set_show_domain (dialog, g_value_get_boolean (value));
			break;
		case PROP_SHOW_PASSWORD:
			gnome_password_dialog_set_show_password (dialog, g_value_get_boolean (value));
			break;
		case PROP_SHOW_NEW_PASSWORD:
			gnome_password_dialog_set_show_new_password (dialog, g_value_get_boolean (value));
			break;
		case PROP_SHOW_NEW_PASSWORD_QUALITY:
			gnome_password_dialog_set_show_new_password_quality (dialog, g_value_get_boolean (value));
			break;
		case PROP_SHOW_USERPASS_BUTTONS:
			gnome_password_dialog_set_show_userpass_buttons (dialog, g_value_get_boolean (value));
			break;
		case PROP_SHOW_REMEMBER:
			gnome_password_dialog_set_show_remember (dialog, g_value_get_boolean (value));
			break;
		case PROP_READONLY_USERNAME:
			gnome_password_dialog_set_readonly_username (dialog, g_value_get_boolean (value));
			break;
		case PROP_READONLY_DOMAIN:
			gnome_password_dialog_set_readonly_domain (dialog, g_value_get_boolean (value));
			break;
		case PROP_ANONYMOUS:
			gnome_password_dialog_set_anon (dialog, g_value_get_boolean (value), TRUE);
			break;
		case PROP_REMEMBER_MODE:
			gnome_password_dialog_set_remember (dialog, g_value_get_enum (value));
			break;
		case PROP_MESSAGE:
			gnome_password_dialog_set_message (dialog, g_value_get_string (value), FALSE);
			break;
		case PROP_MESSAGE_MARKUP:
			gnome_password_dialog_set_message (dialog, g_value_get_string (value), TRUE);
			break;
		case PROP_USERNAME:
			gnome_password_dialog_set_username (dialog, g_value_get_string (value));
			break;
		case PROP_DOMAIN:
			gnome_password_dialog_set_domain (dialog, g_value_get_string (value));
			break;
		case PROP_PASSWORD:
			gnome_password_dialog_set_password (dialog, g_value_get_string (value));
			break;
		case PROP_NEW_PASSWORD:
			gnome_password_dialog_set_new_password (dialog, g_value_get_string (value));
			break;
			
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gnome_password_dialog_get_property (GObject *object,
				    guint prop_id,
				    GValue *value,
				    GParamSpec *pspec)
{
	GnomePasswordDialog *dialog = GNOME_PASSWORD_DIALOG (object);
	GnomePasswordDialogDetails *priv = dialog->details;

	switch (prop_id)
	{
		case PROP_SHOW_USERNAME:
			g_value_set_boolean (value, priv->show_username);
			break;
		case PROP_SHOW_DOMAIN:
			g_value_set_boolean (value, priv->show_domain);
			break;
		case PROP_SHOW_PASSWORD:
			g_value_set_boolean (value, priv->show_password);
			break;
		case PROP_SHOW_NEW_PASSWORD:
			g_value_set_boolean (value, priv->show_new_password);
			break;
		case PROP_SHOW_NEW_PASSWORD_QUALITY:
			g_value_set_boolean (value, priv->show_new_password_quality);
			break;
		case PROP_SHOW_USERPASS_BUTTONS:
			g_value_set_boolean (value, priv->show_userpass_buttons);
			break;
		case PROP_SHOW_REMEMBER:
			g_value_set_boolean (value, priv->show_remember);
			break;
		case PROP_READONLY_USERNAME:
			g_value_set_boolean (value, priv->readonly_username);
			break;
		case PROP_READONLY_DOMAIN:
			g_value_set_boolean (value, priv->readonly_domain);
			break;
		case PROP_ANONYMOUS:
			g_value_set_boolean (value, gnome_password_dialog_anon_selected (dialog));
			break;
		case PROP_REMEMBER_MODE:
			g_value_set_enum (value, gnome_password_dialog_get_remember (dialog));
			break;
		case PROP_MESSAGE:
		case PROP_MESSAGE_MARKUP:
			g_value_set_string (value, gtk_label_get_label (GTK_LABEL (priv->message_label)));
			break;
		case PROP_USERNAME:
			g_value_take_string (value, gnome_password_dialog_get_username (dialog));
			break;
		case PROP_DOMAIN:
			g_value_take_string (value, gnome_password_dialog_get_domain (dialog));
			break;
		case PROP_PASSWORD:
			g_value_take_string (value, gnome_password_dialog_get_password (dialog));
			break;
		case PROP_NEW_PASSWORD:
			g_value_take_string (value, gnome_password_dialog_get_new_password (dialog));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gnome_password_dialog_class_init (GnomePasswordDialogClass * klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	gnome_password_dialog_parent_class = (GObjectClass *) g_type_class_peek_parent (klass);

	gobject_class->constructor = gnome_password_dialog_constructor;
	gobject_class->finalize = gnome_password_dialog_finalize;
	gobject_class->get_property = gnome_password_dialog_get_property;
	gobject_class->set_property = gnome_password_dialog_set_property;

	widget_class->show = gnome_password_dialog_show;

	g_type_class_add_private (gobject_class, sizeof (GnomePasswordDialogDetails));

	/**
	* GnomePasswordDialog:show-username:
	*
	* Whether the username entry is shown.
	*
	* Since: 2.18
	*/
	g_object_class_install_property
		(gobject_class,
		 PROP_SHOW_USERNAME,
		 g_param_spec_boolean ("show-username",
				       "show-username",
				       "show-username",
				       TRUE,
				       G_PARAM_READWRITE | PARAM_STATIC));

	/**
	* GnomePasswordDialog:show-domain:
	 *
	 * Whether the domain entry is shown.
	 *
	 * Since: 2.18
	 */
	g_object_class_install_property
		(gobject_class,
		 PROP_SHOW_DOMAIN,
		 g_param_spec_boolean ("show-domain",
				       "show-domain",
				       "show-domain",
				       FALSE,
				       G_PARAM_READWRITE | PARAM_STATIC));

	/**
	* GnomePasswordDialog:show-password:
	 *
	 * Whether the password entry is shown.
	 *
	 * Since: 2.18
	 */
	g_object_class_install_property
		(gobject_class,
		 PROP_SHOW_PASSWORD,
		 g_param_spec_boolean ("show-password",
				       "show-password",
				       "show-password",
				       TRUE,
				       G_PARAM_READWRITE | PARAM_STATIC));

	/**
	* GnomePasswordDialog:show-new-password:
	 *
	 * Whether the new password and new password confirmation entries are shown.
	 *
	 * Since: 2.18
	 */
	g_object_class_install_property
		(gobject_class,
		 PROP_SHOW_NEW_PASSWORD,
		 g_param_spec_boolean ("show-new-password",
				       "show-new-password",
				       "show-new-password",
				       FALSE,
				       G_PARAM_READWRITE | PARAM_STATIC));

	/**
	* GnomePasswordDialog:show-new-password-quality:
	 *
	 * Whether the new password quality meter is shown.
	 *
	 * Since: 2.18
	 */
	g_object_class_install_property
		(gobject_class,
		 PROP_SHOW_NEW_PASSWORD_QUALITY,
		 g_param_spec_boolean ("show-new-password-quality",
				       "show-new-password-quality",
				       "show-new-password-quality",
				       FALSE,
				       G_PARAM_READWRITE | PARAM_STATIC));

	/**
	* GnomePasswordDialog:show-userpass-buttons:
	 *
	 * Whether the dialog allows switching between anonymous and
	 * user/password authentication.
	 *
	 * Since: 2.18
	 */
	g_object_class_install_property
		(gobject_class,
		 PROP_SHOW_USERPASS_BUTTONS,
		 g_param_spec_boolean ("show-userpass-buttons",
				       "show-userpass-buttons",
				       "show-userpass-buttons",
				       FALSE,
				       G_PARAM_READWRITE | PARAM_STATIC));

	/**
	* GnomePasswordDialog:show-remember:
	 *
	 * Whether the dialog shows the remember in session and 
	 * remember in keyring check boxes.
	 *
	 * Since: 2.18
	 */
	g_object_class_install_property
		(gobject_class,
		 PROP_SHOW_REMEMBER,
		 g_param_spec_boolean ("show-remember",
				       "show-remember",
				       "show-remember",
				       FALSE,
				       G_PARAM_READWRITE | PARAM_STATIC));

	/**
	* GnomePasswordDialog:readonly-username:
	 *
	 * Whether the username entry is read-only.
	 *
	 * Since: 2.18
	 */
	g_object_class_install_property
		(gobject_class,
		 PROP_READONLY_USERNAME,
		 g_param_spec_boolean ("readonly-username",
				       "readonly-username",
				       "readonly-username",
				       FALSE,
				       G_PARAM_READWRITE | PARAM_STATIC));

	/**
	* GnomePasswordDialog:readonly-domain:
	 *
	 * Whether the domain entry is read-only.
	 *
	 * Since: 2.18
	 */
	g_object_class_install_property
		(gobject_class,
		 PROP_READONLY_DOMAIN,
		 g_param_spec_boolean ("readonly-domain",
				       "readonly-domain",
				       "readonly-domain",
				       FALSE,
				       G_PARAM_READWRITE | PARAM_STATIC));

	/**
	* GnomePasswordDialog:anonymous:
	 *
	 * Whether the anonymous option is chosen.
	 *
	 * Since: 2.18
	 */
	g_object_class_install_property
		(gobject_class,
		 PROP_ANONYMOUS,
		 g_param_spec_boolean ("anonymous",
				       "anonymous",
				       "anonymous",
				       FALSE,
				       G_PARAM_READWRITE | PARAM_STATIC));

	/**
	* GnomePasswordDialog:remember-mode:
	 *
	 * The selected remember mode.
	 *
	 * Since: 2.18
	 */
	g_object_class_install_property
		(gobject_class,
		 PROP_REMEMBER_MODE,
		 g_param_spec_enum ("remember-mode",
				    "remember-mode",
				    "remember-mode",
				    GNOME_TYPE_PASSWORD_DIALOG_REMEMBER,
				    GNOME_PASSWORD_DIALOG_REMEMBER_NOTHING,
				    G_PARAM_READWRITE | PARAM_STATIC));
	
	/**
	* GnomePasswordDialog:message:
	 *
	 * The message displayed in the password dialog.
	 *
	 * Since: 2.18
	 */
	g_object_class_install_property
		(gobject_class,
		 PROP_MESSAGE,
		 g_param_spec_string ("message",
				      "message",
				      "message",
				      NULL,
				      G_PARAM_READWRITE | PARAM_STATIC));
	
	/**
	* GnomePasswordDialog:message-markup:
	 *
	 * The message displayed in the password dialog, using pango markup.
	 *
	 * Since: 2.18
	 */
	g_object_class_install_property
		(gobject_class,
		 PROP_MESSAGE_MARKUP,
		 g_param_spec_string ("message-markup",
				      "message-markup",
				      "message-markup",
				      NULL,
				      G_PARAM_READWRITE | PARAM_STATIC));
	
	/**
	* GnomePasswordDialog:username:
	 *
	 * The typed username.
	 *
	 * Since: 2.18
	 */
	g_object_class_install_property
		(gobject_class,
		 PROP_USERNAME,
		 g_param_spec_string ("username",
				      "username",
				      "username",
				      NULL,
				      G_PARAM_READWRITE | PARAM_STATIC));
	
	/**
	* GnomePasswordDialog:domain:
	 *
	 * The typed domain.
	 *
	 * Since: 2.18
	 */
	g_object_class_install_property
		(gobject_class,
		 PROP_DOMAIN,
		 g_param_spec_string ("domain",
				      "domain",
				      "domain",
				      NULL,
				      G_PARAM_READWRITE | PARAM_STATIC));

	/**
	* GnomePasswordDialog:password:
	 *
	 * The typed password.
	 *
	 * Since: 2.18
	 */
	g_object_class_install_property
		(gobject_class,
		 PROP_PASSWORD,
		 g_param_spec_string ("password",
				      "password",
				      "password",
				      NULL,
				      G_PARAM_READWRITE | PARAM_STATIC));

	/**
	* GnomePasswordDialog:new-password:
	 *
	 * The typed new password, or %NULL if the input in the new password
	 * and the new password confirmation fields don't match.
	 *
	 * Since: 2.18
	 */
	g_object_class_install_property
			(gobject_class,
			 PROP_NEW_PASSWORD,
			 g_param_spec_string ("new-password",
					      "new-password",
					      "new-password",
					      NULL,
					      G_PARAM_READWRITE | PARAM_STATIC));
}

/* GtkDialog callbacks */
static void
gnome_password_dialog_show (GtkWidget *widget)
{
	GnomePasswordDialog *password_dialog = GNOME_PASSWORD_DIALOG (widget);
	GnomePasswordDialogDetails *priv = password_dialog->details;
	GtkWidget *focus = NULL;
	const gchar *text;

	GTK_WIDGET_CLASS (gnome_password_dialog_parent_class)->show (widget);

	if (GTK_WIDGET_VISIBLE (priv->password_entry) &&
	    GTK_WIDGET_IS_SENSITIVE (priv->password_entry) &&
	    priv->show_password)
		/* Password is the default place to focus */
		focus = priv->password_entry;

	if (GTK_WIDGET_VISIBLE (priv->domain_entry) &&
	    GTK_WIDGET_IS_SENSITIVE (priv->domain_entry) &&
	    priv->show_domain) {
		/* Empty domain entry gets focus */
		text = gtk_entry_get_text (GTK_ENTRY (priv->domain_entry));
		if (!focus || !text || !text[0])
			focus = priv->domain_entry;
	}

	if (GTK_WIDGET_VISIBLE (priv->username_entry) &&
	    GTK_WIDGET_IS_SENSITIVE (priv->username_entry) &&
	    priv->show_username) {
		/* Empty username entry gets focus */
		text = gtk_entry_get_text (GTK_ENTRY (priv->username_entry));
		if (!focus || !text || !text[0])
			focus = priv->username_entry;
	}

	if (focus) {
		gtk_widget_grab_focus (focus);
	}
}

/* Public GnomePasswordDialog methods */

/**
 * gnome_password_dialog_new:
 * @dialog_title: The title of the dialog
 * @message: Message text for the dialog
 * @username: The username to be used in the dialog
 * @password: Password to be used
 * @readonly_username: Boolean value that controls whether the user
 * can edit the username or not
 * 
 * Description: Creates a new password dialog with an optional title, 
 * message, username, password etc. The user will be given the option to
 * save the password for this session only or store it permanently in her
 * keyring.
 * 
 * Returns: A new password dialog.
 *
 * Since: 2.4
 **/
GtkWidget *
gnome_password_dialog_new (const char	*dialog_title,
			   const char	*message,
			   const char	*username,
			   const char	*password,
			   gboolean	 readonly_username)
{
	GnomePasswordDialog *password_dialog;
	GtkWindow *window;

	password_dialog = g_object_new (GNOME_TYPE_PASSWORD_DIALOG,
					"title", dialog_title,
					"message", message,
					"readonly-username", readonly_username,
					NULL);

	window = GTK_WINDOW (password_dialog);

 	gtk_window_set_position (window, GTK_WIN_POS_CENTER);
	gtk_window_set_modal (window, TRUE);
	gtk_window_set_resizable (window, FALSE);
	gtk_window_set_keep_above (window, TRUE);
	g_signal_connect (password_dialog, "close",
			  G_CALLBACK (gtk_widget_hide), NULL);
	
	gnome_password_dialog_set_username (password_dialog, username);
	gnome_password_dialog_set_password (password_dialog, password);
	
	return GTK_WIDGET (password_dialog);
}

/**
 * gnome_password_dialog_run_and_block:
 * @password_dialog: A #GnomePasswordDialog
 * 
 * Description: Gets the user input from PasswordDialog.
 * 
 * Returns: %TRUE if "Connect" button is pressed. %FALSE if "Cancel"  button is pressed.
 *
 * Since: 2.4
 **/
gboolean
gnome_password_dialog_run_and_block (GnomePasswordDialog *password_dialog)
{
	gint button_clicked;

	g_return_val_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog), FALSE);

	button_clicked = gtk_dialog_run (GTK_DIALOG (password_dialog));
	gtk_widget_hide (GTK_WIDGET (password_dialog));

	return button_clicked == GTK_RESPONSE_OK;
}

/**
 * gnome_password_dialog_set_username:
 * @password_dialog: A #GnomePasswordDialog
 * @username: The username that should be set
 *
 * Description: Sets the username in the password dialog.
 *
 * Since: 2.4
 **/
void
gnome_password_dialog_set_username (GnomePasswordDialog	*password_dialog,
				    const char		*username)
{
	GnomePasswordDialogDetails *priv;

	g_return_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog));

	priv = password_dialog->details;
	g_return_if_fail (priv->username_entry != NULL);

	gtk_entry_set_text (GTK_ENTRY (priv->username_entry),
			    username ? username : "");

	g_object_notify (G_OBJECT (password_dialog), "username");
}

/**
 * gnome_password_dialog_set_password:
 * @password_dialog: A #GnomePasswordDialog
 * @password: The password that should be set
 *
 * Description: Sets the password in the password dialog.
 *
 * Since: 2.4
 **/
void
gnome_password_dialog_set_password (GnomePasswordDialog	*password_dialog,
				       const char		*password)
{
	GnomePasswordDialogDetails *priv;

	g_return_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog));

	priv = password_dialog->details;

	gtk_entry_set_text (GTK_ENTRY (priv->password_entry),
			    password ? password : "");

	g_object_notify (G_OBJECT (password_dialog), "password");
}

/**
 * gnome_password_dialog_set_new_password:
 * @password_dialog: A #GnomePasswordDialog
 * @password: The new password that should be set
 *
 * Description: Sets the new password and new password confirmation fields
 * in the password dialog.
 *
 * Since: 2.18
 **/
void
gnome_password_dialog_set_new_password (GnomePasswordDialog *password_dialog, const gchar *password)
{
	GnomePasswordDialogDetails *priv;

	g_return_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog));

	priv = password_dialog->details;

	gtk_entry_set_text (GTK_ENTRY (priv->new_password_entry), password ? password : "");
	gtk_entry_set_text (GTK_ENTRY (priv->confirm_new_password_entry), password ? password : "");
}

/**
 * gnome_password_dialog_set_password_quality_func:
 * @password_dialog: A #GnomePasswordDialog
 * @func: the new #GnomePasswordDialogQualityFunc
 * @data: user data to pass to the quality func
 * @destroy_notify:
 *
 * Description: Sets the function which measures the quality of the
 * new password as it's typed.
 *
 * Since: 2.18
 **/
void
gnome_password_dialog_set_password_quality_func (GnomePasswordDialog *password_dialog,
						 GnomePasswordDialogQualityFunc func,
						 gpointer data,
						 GDestroyNotify destroy_notify)
{
	GnomePasswordDialogDetails *priv;
	gpointer old_data;
	GDestroyNotify dnotify;

	g_return_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog));

	priv = password_dialog->details;

	old_data = priv->quality_data;
	dnotify = priv->quality_dnotify;

	priv->quality_func = func;
	priv->quality_data = data;
	priv->quality_dnotify = destroy_notify;

	if (old_data && dnotify) {
		dnotify (old_data);
	}
}

/**
 * gnome_password_dialog_set_domain:
 * @password_dialog: A #GnomePasswordDialog
 * @domain: The domain that should be set
 *
 * Description: Sets the domain field in the password dialog to @domain.
 *
 * Since: 2.4
 **/
void
gnome_password_dialog_set_domain (GnomePasswordDialog	*password_dialog,
				  const char		*domain)
{
	GnomePasswordDialogDetails *priv;

	g_return_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog));

	priv = password_dialog->details;
	g_return_if_fail (priv->domain_entry != NULL);

	gtk_entry_set_text (GTK_ENTRY (priv->domain_entry),
			    domain ? domain : "");

	g_object_notify (G_OBJECT (password_dialog), "domain");
}

/**
 * gnome_password_dialog_set_show_username:
 * @password_dialog: A #GnomePasswordDialog
 * @show: Boolean value that controls whether the username entry has to
 * appear or not.
 *
 * Description: Shows or hides the username entry in the password dialog based on the value of @show. 
 *
 * Since: 2.6
 **/
void
gnome_password_dialog_set_show_username (GnomePasswordDialog *password_dialog,
					 gboolean             show)
{
	GnomePasswordDialogDetails *priv;

	g_return_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog));

	priv = password_dialog->details;

	show = show != FALSE;
	if (priv->show_username != show) {
		priv->show_username = show;

		update_entries_table (password_dialog);

		g_object_notify (G_OBJECT (password_dialog), "show-username");
	}
}

/**
 * gnome_password_dialog_set_show_domain:
 * @password_dialog: A #GnomePasswordDialog
 * @show: Boolean value that controls whether the domain entry has to 
 * appear or not.
 *
 * Description: Shows or hides the domain field in the password dialog based on the value of @show.
 *
 * Since: 2.6
 **/
void
gnome_password_dialog_set_show_domain (GnomePasswordDialog *password_dialog,
				       gboolean             show)
{
	GnomePasswordDialogDetails *priv;

	g_return_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog));

	priv = password_dialog->details;

	show = show != FALSE;
	if (priv->show_domain != show) {
		priv->show_domain = show;

		update_entries_table (password_dialog);

		g_object_notify (G_OBJECT (password_dialog), "show-domain");
	}
}

/**
 * gnome_password_dialog_set_show_password:
 * @password_dialog: A #GnomePasswordDialog
 * @show: Boolean value that controls whether the password entry has to
 * appear or not.
 *
 * Description: Shows or hides the password field in the password dialog based on the value of @show.
 *
 * Since: 2.6
 **/
void
gnome_password_dialog_set_show_password (GnomePasswordDialog *password_dialog,
					 gboolean             show)
{
	GnomePasswordDialogDetails *priv;

	g_return_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog));

	priv = password_dialog->details;

	show = show != FALSE;
	if (priv->show_password != show) {
		priv->show_password = show;

		update_entries_table (password_dialog);

		g_object_notify (G_OBJECT (password_dialog), "show-password");
	}
}

/**
 * gnome_password_dialog_set_show_new_password:
 * @password_dialog: A #GnomePasswordDialog
 * @show: Boolean value that controls whether the new password entry has to
 * appear or not.
 *
 * Description: Shows or hides the new password and new password confirmation
 * fields based on the value of @show.
 *
 * Since: 2.18
 **/
void
gnome_password_dialog_set_show_new_password (GnomePasswordDialog *password_dialog,
					     gboolean             show)
{
	GnomePasswordDialogDetails *priv;

	g_return_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog));

	priv = password_dialog->details;

	show = show != FALSE;
	if (priv->show_new_password != show) {
		priv->show_new_password = show;

		update_entries_table (password_dialog);

		g_object_notify (G_OBJECT (password_dialog), "show-new-password");
	}
}

/**
 * gnome_password_dialog_set_show_new_password_quality:
 * @password_dialog: A #GnomePasswordDialog
 * @show: Boolean value that controls whether the new password quality meter will
 * appear or not.
 *
 * Description: Shows or hides the new password quality meter.
 *
 * Since: 2.18
 **/
void
gnome_password_dialog_set_show_new_password_quality (GnomePasswordDialog *password_dialog,
						     gboolean             show)
{
	GnomePasswordDialogDetails *priv;

	g_return_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog));

	priv = password_dialog->details;

	show = show != FALSE;
	if (priv->show_new_password_quality != show) {
		priv->show_new_password_quality = show;

		update_entries_table (password_dialog);

		g_object_notify (G_OBJECT (password_dialog), "show-new-password-quality");
	}
}

/**
 * gnome_password_dialog_set_readonly_username:
 * @password_dialog: A #GnomePasswordDialog
 * @readonly : Boolean value that controls whether the user
 * can edit the username or not
 * 
 * Description: Sets the editable nature of the username field in the password 
 * dialog based on the boolean value @readonly.
 *  
 * Since: 2.4
 **/
void
gnome_password_dialog_set_readonly_username (GnomePasswordDialog	*password_dialog,
					     gboolean			 readonly)
{
	GnomePasswordDialogDetails *priv;

	g_return_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog));

	priv = password_dialog->details;

	readonly = readonly != FALSE;

	if (priv->readonly_username != readonly) {
		priv->readonly_username = readonly;
		gtk_widget_set_sensitive (priv->username_entry, !readonly);

		g_object_notify (G_OBJECT (password_dialog), "readonly-username");
	}
}

/**
 * gnome_password_dialog_set_readonly_domain:
 * @password_dialog: A #GnomePasswordDialog
 * @readonly : Boolean value that controls whether the user
 * can edit the domain or not
 *
 * Description: Sets the editable nature of the domain field in the password
 * dialog based on the boolean value @readonly.
 *              
 * Since: 2.6
 **/
void
gnome_password_dialog_set_readonly_domain (GnomePasswordDialog	*password_dialog,
					   gboolean		readonly)
{
	GnomePasswordDialogDetails *priv;

	g_return_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog));

	priv = password_dialog->details;

	readonly = readonly != FALSE;

	if (priv->readonly_domain != readonly) {
		priv->readonly_domain = readonly;

		gtk_widget_set_sensitive (priv->domain_entry, !readonly);

		g_object_notify (G_OBJECT (password_dialog), "readonly-domain");
	}
}

/**
 * gnome_password_dialog_get_username:
 * @password_dialog: A #GnomePasswordDialog
 *
 * Description: Gets the username from the password dialog.
 *
 * Returns: The username, a char*.
 *
 * Since: 2.4
 **/
  
char *
gnome_password_dialog_get_username (GnomePasswordDialog *password_dialog)
{
	GnomePasswordDialogDetails *priv;

	g_return_val_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog), NULL);

	priv = password_dialog->details;

	return g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->username_entry)));
}

/**
 * gnome_password_dialog_get_domain:
 * @password_dialog: A #GnomePasswordDialog
 *
 * Description: Gets the domain name from the password dialog.
 *
 * Returns: The domain name, a char*.
 *
 * Since: 2.4
 **/
char *
gnome_password_dialog_get_domain (GnomePasswordDialog *password_dialog)
{
	GnomePasswordDialogDetails *priv;

	g_return_val_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog), NULL);

	priv = password_dialog->details;
	
	return g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->domain_entry)));
}

/**
 * gnome_password_dialog_get_password:
 * @password_dialog: A #GnomePasswordDialog
 *
 * Description: Gets the password from the password dialog.
 *
 * Returns: The password, a char*.
 *
 * Since: 2.4
 **/
char *
gnome_password_dialog_get_password (GnomePasswordDialog *password_dialog)
{
	GnomePasswordDialogDetails *priv;

	g_return_val_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog), NULL);

	priv = password_dialog->details;

	return g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->password_entry)));
}

/**
 * gnome_password_dialog_get_new_password:
 * @password_dialog: A #GnomePasswordDialog
 *
 * Description: Gets the new password from the password dialog.
 *
 * Returns: The password, or %NULL if the entries in the new password
 * field and the confirmation field don't match.
 *
 * Since: 2.18
 **/
char *
gnome_password_dialog_get_new_password (GnomePasswordDialog *password_dialog)
{
	GnomePasswordDialogDetails *priv;
	const gchar *new_password, *confirm_password;

	g_return_val_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog), NULL);

	priv = password_dialog->details;

	new_password = gtk_entry_get_text (GTK_ENTRY (priv->new_password_entry));
	confirm_password = gtk_entry_get_text (GTK_ENTRY (priv->confirm_new_password_entry));
	if (strcmp (new_password, confirm_password) != 0) {
		return NULL;
	}

	return g_strdup (new_password);
}

/**
 * gnome_password_dialog_set_show_userpass_buttons:
 * @password_dialog: A #GnomePasswordDialog
 * @show_userpass_buttons: Boolean value that controls whether the radio buttons for connecting 
 * anonymously and connecting as user should be shown or not.
 *
 * Description: Shows the radio buttons for connecting anonymously and connecting as user if 
 * @show_userpass_buttons is #TRUE. Also makes the 'Username' and 'Password' fields greyed out if the 
 * radio button for connecting anonymously is active. If @show_userpass_buttons is #FALSE, then these
 * radio buttons are hidden and the 'Username' and 'Password' fields will be made active.
 * 
 * Since: 2.8
 */
void
gnome_password_dialog_set_show_userpass_buttons (GnomePasswordDialog         *password_dialog,
                                                 gboolean                     show)
{
	GnomePasswordDialogDetails *priv;
	const gchar *username;
	gboolean sensitive = TRUE;

	g_return_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog));

	priv = password_dialog->details;

	show = show != FALSE;
	priv->show_userpass_buttons_set = TRUE;

	if (priv->show_userpass_buttons != show) {
		priv->show_userpass_buttons = show;	
		g_object_set (priv->radio_vbox, "visible", priv->show_userpass_buttons, NULL);

		if (show) {
			sensitive = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->connect_with_no_userpass_button));
		}

		gtk_widget_set_sensitive (priv->table_alignment, sensitive);
		gtk_alignment_set_padding (GTK_ALIGNMENT (priv->table_alignment),
					   0, 0, show ? 12 : 0, 0);

		/* Compute default */
		if (!priv->anonymous_set) {
			username = gtk_entry_get_text (GTK_ENTRY (priv->username_entry));
			gnome_password_dialog_set_anon (password_dialog,
					username == NULL || username[0] == '\0', FALSE);
		}

		g_object_notify (G_OBJECT (password_dialog), "readonly-domain");
	}
}

/**
 * gnome_password_dialog_anon_selected:
 * @password_dialog: A #GnomePasswordDialog
 * 
 * Description: Checks whether anonymous support is set to #TRUE and the radio button for connecting
 * as anonymous user is active.
 *
 * Returns: #TRUE if anonymous support is set and the radio button is active, #FALSE otherwise.
 *
 **/
gboolean
gnome_password_dialog_anon_selected (GnomePasswordDialog *password_dialog)
{
	GnomePasswordDialogDetails *priv;
	gboolean active;

	g_return_val_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog), FALSE);

	priv = password_dialog->details;
	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->connect_with_no_userpass_button));

	return priv->show_userpass_buttons && active;
}

/**
 * gnome_password_dialog_set_show_remember:
 * @password_dialog: A #GnomePasswordDialog
 * @show_remember: Boolean value that controls whether the check buttons for password retention
 * should appear or not.
 *
 * Description: Shows or hides the check buttons to save password in keyring and remember password for 
 * session based on the value of @show_remember.
 *
 * Since: 2.6
 **/
void
gnome_password_dialog_set_show_remember (GnomePasswordDialog         *password_dialog,
					 gboolean                     show)
{
	GnomePasswordDialogDetails *priv;

	g_return_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog));

	priv = password_dialog->details;

	show = show != FALSE;

	if (priv->show_remember != show) {
		priv->show_remember = show;

		if (show) {
			gtk_widget_show (priv->remember_session_button);
			gtk_widget_show (priv->remember_forever_button);
		} else {
			gtk_widget_hide (priv->remember_session_button);
			gtk_widget_hide (priv->remember_forever_button);
		}

		g_object_notify (G_OBJECT (password_dialog), "show-remember");
	}
}

/**
 * gnome_password_dialog_set_remember:
 * @password_dialog: A #GnomePasswordDialog.
 * @remember: A #GnomePasswordDialogRemember.
 *
 * Description: Based on the value of #GnomePasswordDialogRemember, sets the state of
 * the check buttons to remember password for the session and save password to keyring .
 *
 * Since: 2.6
 **/
void
gnome_password_dialog_set_remember      (GnomePasswordDialog         *password_dialog,
					 GnomePasswordDialogRemember  remember)
{
	GnomePasswordDialogDetails *priv;
	gboolean session, forever;

	g_return_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog));

	priv = password_dialog->details;

	if (gnome_password_dialog_get_remember (password_dialog) != remember) {
		session = FALSE;
		forever = FALSE;
		if (remember == GNOME_PASSWORD_DIALOG_REMEMBER_SESSION) {
			session = TRUE;
		} else if (remember == GNOME_PASSWORD_DIALOG_REMEMBER_FOREVER){
			forever = TRUE;
		}
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->remember_session_button),
					session);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->remember_forever_button),
					forever);

		g_object_notify (G_OBJECT (password_dialog), "remember-mode");
	}
}

/**
 * gnome_password_dialog_get_remember:
 * @password_dialog: A #GnomePasswordDialog
 *
 * Description: Gets the state of the check buttons to remember password for the session and save
 * password to keyring.
 *
 * Returns: a #GnomePasswordDialogRemember, which indicates whether to remember the password for the session
 * or forever.
 *
 * Since: 2.6
 **/
GnomePasswordDialogRemember
gnome_password_dialog_get_remember (GnomePasswordDialog         *password_dialog)
{
	GnomePasswordDialogDetails *priv;
	gboolean session, forever;

	g_return_val_if_fail (GNOME_IS_PASSWORD_DIALOG (password_dialog), GNOME_PASSWORD_DIALOG_REMEMBER_NOTHING);

	priv = password_dialog->details;

	session = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->remember_session_button));
	forever = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->remember_forever_button));
	if (forever) {
		return GNOME_PASSWORD_DIALOG_REMEMBER_FOREVER;
	} else if (session) {
		return GNOME_PASSWORD_DIALOG_REMEMBER_SESSION;
	}
	return GNOME_PASSWORD_DIALOG_REMEMBER_NOTHING;
}
