/* WARNING ____ IMMATURE API ____ liable to change */

/* gnome-guru.h: Copyright (C) 1998 Free Software Foundation
 *  A wizard widget
 * Written by: Havoc Pennington, based on John Ellis's code.
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */

#ifndef GNOME_GURU_H
#define GNOME_GURU_H

#include "libgnome/gnome-defs.h"
#include <gtk/gtk.h>
#include "gnome-dialog.h"

BEGIN_GNOME_DECLS

typedef struct _GnomeGuru GnomeGuru;
typedef struct _GnomeGuruClass GnomeGuruClass;

#define GNOME_TYPE_GURU            (gnome_guru_get_type ())
#define GNOME_GURU(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_GURU, GnomeGuru))
#define GNOME_GURU_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_GURU, GnomeGuruClass))
#define GNOME_IS_GURU(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_GURU))
#define GNOME_IS_GURU_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_GURU))

struct _GnomeGuruPage;

struct _GnomeGuru {
  GtkVBox vbox;

  /* all private, don't touch it. */

  GtkWidget * graphic;

  GList * pages;

  struct _GnomeGuruPage* current_page;

  GtkWidget * next;
  GtkWidget * back;
  GtkWidget * finish;
  GtkWidget * page_title;
  GtkWidget * page_box;
  GtkWidget * buttonbox;

  guint has_dialog : 1;
};

struct _GnomeGuruClass {
  GtkVBoxClass parent_class;

  void (* cancelled)(GnomeGuru* guru);
  void (* finished) (GnomeGuru* guru);
};

guint       gnome_guru_get_type       (void);

/* any of the args can be NULL */
GtkWidget * gnome_guru_new                       (const gchar * name,
						  GtkWidget   * graphic,
						  GnomeDialog * dialog);

void        gnome_guru_construct                 (GnomeGuru   * guru,
						  const gchar * name, 
						  GtkWidget   * graphic,
						  GnomeDialog * dialog);

/* If you want an action when the page is shown, just connect to
   GtkWidget::show. Don't show the page yourself. */
void        gnome_guru_append_page               (GnomeGuru   * guru,
						  const gchar * name,
						  GtkWidget   * widget);

void        gnome_guru_next_set_sensitive        (GnomeGuru   * guru,
						  gboolean      sensitivity);

void        gnome_guru_back_set_sensitive        (GnomeGuru   * guru,
						  gboolean      sensitivity);

GtkWidget * gnome_guru_current_page              (GnomeGuru   * guru);

END_GNOME_DECLS
   
#endif /* GNOME_GURU_H */




