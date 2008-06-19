/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gnome-password-dialog.h - A use password prompting dialog widget.

   Copyright (C) 1999, 2000 Eazel, Inc.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
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

#ifndef GNOME_PASSWORD_DIALOG_H
#define GNOME_PASSWORD_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GNOME_TYPE_PASSWORD_DIALOG            (gnome_password_dialog_get_type ())
#define GNOME_PASSWORD_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_PASSWORD_DIALOG, GnomePasswordDialog))
#define GNOME_PASSWORD_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_PASSWORD_DIALOG, GnomePasswordDialogClass))
#define GNOME_IS_PASSWORD_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_PASSWORD_DIALOG))
#define GNOME_IS_PASSWORD_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_PASSWORD_DIALOG))

typedef struct _GnomePasswordDialog        GnomePasswordDialog;
typedef struct _GnomePasswordDialogClass   GnomePasswordDialogClass;
typedef struct _GnomePasswordDialogDetails GnomePasswordDialogDetails;

struct _GnomePasswordDialog
{
	GtkDialog gtk_dialog;

	GnomePasswordDialogDetails *details;
};

struct _GnomePasswordDialogClass
{
	GtkDialogClass parent_class;
};

typedef enum {
	GNOME_PASSWORD_DIALOG_REMEMBER_NOTHING,
	GNOME_PASSWORD_DIALOG_REMEMBER_SESSION,
	GNOME_PASSWORD_DIALOG_REMEMBER_FOREVER
} GnomePasswordDialogRemember;

typedef gdouble (* GnomePasswordDialogQualityFunc) (GnomePasswordDialog *password_dialog,
 						    const char *password,
						    gpointer user_data);

GType    gnome_password_dialog_get_type (void);
GtkWidget* gnome_password_dialog_new      (const char *dialog_title,
					   const char *message,
					   const char *username,
					   const char *password,
					   gboolean    readonly_username);

gboolean   gnome_password_dialog_run_and_block           (GnomePasswordDialog *password_dialog);

/* Attribute mutators */
void gnome_password_dialog_set_show_username       (GnomePasswordDialog *password_dialog,
						    gboolean             show);
void gnome_password_dialog_set_show_domain         (GnomePasswordDialog *password_dialog,
						    gboolean             show);
void gnome_password_dialog_set_show_password       (GnomePasswordDialog *password_dialog,
						    gboolean             show);
void gnome_password_dialog_set_show_new_password   (GnomePasswordDialog *password_dialog,
						    gboolean             show);
void gnome_password_dialog_set_show_new_password_quality (GnomePasswordDialog *password_dialog,
							  gboolean             show);
void gnome_password_dialog_set_username            (GnomePasswordDialog *password_dialog,
						    const char          *username);
void gnome_password_dialog_set_domain              (GnomePasswordDialog *password_dialog,
						    const char          *domain);
void gnome_password_dialog_set_password            (GnomePasswordDialog *password_dialog,
						    const char          *password);
void gnome_password_dialog_set_new_password        (GnomePasswordDialog *password_dialog,
						    const char          *password);
void gnome_password_dialog_set_password_quality_func (GnomePasswordDialog *password_dialog,
						      GnomePasswordDialogQualityFunc func,
						      gpointer data,
						      GDestroyNotify dnotify);
void gnome_password_dialog_set_readonly_username   (GnomePasswordDialog *password_dialog,
						    gboolean             readonly);
void gnome_password_dialog_set_readonly_domain     (GnomePasswordDialog *password_dialog,
						    gboolean             readonly);

void                        gnome_password_dialog_set_show_remember (GnomePasswordDialog         *password_dialog,
								     gboolean                     show_remember);
void                        gnome_password_dialog_set_remember      (GnomePasswordDialog         *password_dialog,
								     GnomePasswordDialogRemember  remember);
GnomePasswordDialogRemember gnome_password_dialog_get_remember      (GnomePasswordDialog         *password_dialog);
void                        gnome_password_dialog_set_show_userpass_buttons (GnomePasswordDialog         *password_dialog,
                                                                     	     gboolean                     show_userpass_buttons);

/* Attribute accessors */
char *     gnome_password_dialog_get_username            (GnomePasswordDialog *password_dialog);
char *     gnome_password_dialog_get_domain              (GnomePasswordDialog *password_dialog);
char *     gnome_password_dialog_get_password            (GnomePasswordDialog *password_dialog);
char *     gnome_password_dialog_get_new_password        (GnomePasswordDialog *password_dialog);

gboolean   gnome_password_dialog_anon_selected 		 (GnomePasswordDialog *password_dialog);

G_END_DECLS

#endif /* GNOME_PASSWORD_DIALOG_H */
