/*
 * Copyright (C) 2000 SuSE GmbH
 * Author: Martin Baulig <baulig@suse.de>
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

/* GnomeFileSelectorDialog - the new file selector dialog.
 *
 * Author: Martin Baulig <baulig@suse.de>
 */

#ifndef GNOME_FILE_SELECTOR_DIALOG_H
#define GNOME_FILE_SELECTOR_DIALOG_H


#include <gtk/gtkwindow.h>



G_BEGIN_DECLS


#define GNOME_TYPE_FILE_SELECTOR_DIALOG            (gnome_file_selector_dialog_get_type ())
#define GNOME_FILE_SELECTOR_DIALOG(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_FILE_SELECTOR_DIALOG, GnomeFileSelectorDialog))
#define GNOME_FILE_SELECTOR_DIALOG_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_FILE_SELECTOR_DIALOG, GnomeFileSelectorDialogClass))
#define GNOME_IS_FILE_SELECTOR_DIALOG(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_FILE_SELECTOR_DIALOG))
#define GNOME_IS_FILE_SELECTOR_DIALOG_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_FILE_SELECTOR_DIALOG))
#define GNOME_FILE_SELECTOR_DIALOG_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_FILE_SELECTOR_DIALOG, GnomeFileSelectorDialogClass))


typedef struct _GnomeFileSelectorDialog         GnomeFileSelectorDialog;
typedef struct _GnomeFileSelectorDialogPrivate  GnomeFileSelectorDialogPrivate;
typedef struct _GnomeFileSelectorDialogClass    GnomeFileSelectorDialogClass;

struct _GnomeFileSelectorDialog {
	GtkWindow window;
	
	/*< private >*/
	GnomeFileSelectorDialogPrivate *_priv;
};

struct _GnomeFileSelectorDialogClass {
	GtkWindowClass parent_class;

	void (*update) (GnomeFileSelectorDialog *fsdialog);
};

guint
gnome_file_selector_dialog_get_type     (void) G_GNUC_CONST;

GtkWidget *
gnome_file_selector_dialog_new          (const gchar *dialog_title);

void
gnome_file_selector_dialog_construct    (GnomeFileSelectorDialog *fsdialog,
					 const gchar *dialog_title);

/* Update the file selector dialog.*/
void
gnome_file_selector_dialog_update       (GnomeFileSelectorDialog *fsdialog);

/* Get/Set current directory of the file selector dialog.*/
void
gnome_file_selector_dialog_set_dir      (GnomeFileSelectorDialog *fsdialog,
					 const gchar *directory);

const gchar *
gnome_file_selector_dialog_get_dir      (GnomeFileSelectorDialog *fsdialog);

/* Get/Set "Home Directory" of the file selector dialog.*/
void
gnome_file_selector_dialog_set_home_dir (GnomeFileSelectorDialog *fsdialog,
					 const gchar *home_directory);

const gchar *
gnome_file_selector_dialog_get_home_dir (GnomeFileSelectorDialog *fsdialog);


G_END_DECLS

#endif

