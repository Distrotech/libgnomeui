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

/* GnomeSelector widget - pure virtual widget.
 *
 *   Use the Gnome{File,Icon,Pixmap}Selector subclasses.
 *
 * Author: Martin Baulig <baulig@suse.de>
 */

#ifndef GNOME_SELECTOR_H
#define GNOME_SELECTOR_H


#include <gtk/gtkvbox.h>
#include <libgnome/gnome-defs.h>


BEGIN_GNOME_DECLS


#define GNOME_TYPE_SELECTOR            (gnome_selector_get_type ())
#define GNOME_SELECTOR(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_SELECTOR, GnomeSelector))
#define GNOME_SELECTOR_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_SELECTOR, GnomeSelectorClass))
#define GNOME_IS_SELECTOR(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_SELECTOR))
#define GNOME_IS_SELECTOR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_SELECTOR))
#define GNOME_SELECTOR_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_SELECTOR, GnomeSelectorClass))


typedef struct _GnomeSelector         GnomeSelector;
typedef struct _GnomeSelectorPrivate  GnomeSelectorPrivate;
typedef struct _GnomeSelectorClass    GnomeSelectorClass;

struct _GnomeSelector {
	GtkVBox vbox;
	
	/*< private >*/
	GnomeSelectorPrivate *_priv;
};

struct _GnomeSelectorClass {
	GtkVBoxClass parent_class;

	void (*changed) (GnomeSelector *selector);
	void (*browse) (GnomeSelector *selector);
	void (*clear) (GnomeSelector *selector);

	void (*update_filelist) (GnomeSelector *selector);
	gboolean (*check_filename) (GnomeSelector *selector,
				    const gchar *filename);
	gboolean (*set_filename) (GnomeSelector *selector,
				  const gchar *filename);
	void (*add_directory) (GnomeSelector *selector,
			       const gchar *directory);
};


guint        gnome_selector_get_type         (void);

/* This is a purely virtual class, so there is no _new method.
 * Use gnome_{file,icon,pixmap}_selector_new instead. */

void         gnome_selector_construct        (GnomeSelector *selector,
                                              const gchar *history_id,
					      const gchar *dialog_title,
					      GtkWidget *selector_widget,
					      GtkWidget *browse_dialog);

/*only return a file if the `check_filename' method succeeded. */
gchar       *gnome_selector_get_filename     (GnomeSelector *selector);

/* checks whether this is a valid filename. */
gboolean     gnome_selector_check_filename   (GnomeSelector *selector,
                                              const gchar *filename);

/* set the filename to something, returns TRUE on success. */
gboolean     gnome_selector_set_filename     (GnomeSelector *selector,
                                              const gchar *filename);

/* Remove all entries from the selector. */
void         gnome_selector_clear            (GnomeSelector *selector);

/* Add all files from this directory. */
void         gnome_selector_add_directory    (GnomeSelector *selector,
					      const gchar *directory);

/* Update file list. */
void         gnome_selector_update_file_list (GnomeSelector *selector);

/* get/set the dialog title. */
const gchar *gnome_selector_get_dialog_title (GnomeSelector *selector);
void         gnome_selector_set_dialog_title (GnomeSelector *selector,
					      const gchar *dialog_title);

/* returns the GnomeEntry widget of this selector. */
GtkWidget   *gnome_selector_get_gnome_entry  (GnomeSelector *selector);


END_GNOME_DECLS

#endif
