/* gnome-dentry-edit.h: Copyright (C) 1998 Free Software Foundation
 *
 * Written by: Havoc Pennington, based on code by John Ellis.
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

/******************** NOTE: this is an object, not a widget.
 ********************       You must supply a GtkNotebook.
 The reason for this is that you might want this in a property box, 
 or in your own notebook. Look at the test program at the bottom 
 of gnome-dentry-edit.c for a usage example.
 */

#ifndef GNOME_DENTRY_EDIT_H
#define GNOME_DENTRY_EDIT_H

#include <gtk/gtk.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-dentry.h"

BEGIN_GNOME_DECLS

typedef struct _GnomeDEntryEdit GnomeDEntryEdit;
typedef struct _GnomeDEntryEditClass GnomeDEntryEditClass;

#define GNOME_TYPE_DENTRY_EDIT            (gnome_dentry_edit_get_type ())
#define GNOME_DENTRY_EDIT(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_DENTRY_EDIT, GnomeDEntryEdit))
#define GNOME_DENTRY_EDIT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_DENTRY_EDIT, GnomeDEntryEditClass))
#define GNOME_IS_DENTRY_EDIT(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_DENTRY_EDIT))
#define GNOME_IS_DENTRY_EDIT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_DENTRY_EDIT))

struct _GnomeDEntryEdit {
  GtkObject object;
  
  /*semi public entries, you should however use macros to get these*/
  GtkWidget *child1;
  GtkWidget *child2;
  
  /* Remaining fields are private - if you need them, 
     please add an accessor function. */

  GtkWidget *name_entry;
  GtkWidget *comment_entry;
  GtkWidget *exec_entry;
  GtkWidget *tryexec_entry;
  GtkWidget *doc_entry;

  GtkWidget *type_combo;

  GtkWidget *terminal_button;  

  GtkWidget *icon_entry;
  
  GtkWidget *translations;
  GtkWidget *transl_lang_entry;
  GtkWidget *transl_name_entry;
  GtkWidget *transl_comment_entry;
};

struct _GnomeDEntryEditClass {
  GtkObjectClass parent_class;

  /* Any information changed */
  void (* changed)         (GnomeDEntryEdit * gee);
  /* These two more specific signals are provided since they 
     will likely require a display update */
  /* The icon in particular has changed. */
  void (* icon_changed)    (GnomeDEntryEdit * gee);
  /* The name of the item has changed. */
  void (* name_changed)    (GnomeDEntryEdit * gee);
};

guint       gnome_dentry_edit_get_type  (void);

/*create a new dentry and get the children using the below macros
  or use the utility new_notebook below*/
GtkObject * gnome_dentry_edit_new       (void);
#define gnome_dentry_edit_child1(d) (GNOME_DENTRY_EDIT(d)->child1)
#define gnome_dentry_edit_child2(d) (GNOME_DENTRY_EDIT(d)->child2)

/* Create a new edit in this notebook - appends two pages to the 
   notebook. */
GtkObject * gnome_dentry_edit_new_notebook(GtkNotebook * notebook);

void        gnome_dentry_edit_clear     (GnomeDEntryEdit * dee);

/* The GnomeDEntryEdit does not store a dentry, and it does not keep
   track of the location field of GnomeDesktopEntry which will always
   be NULL. */

/* Make the display reflect dentry at path */
void        gnome_dentry_edit_load_file  (GnomeDEntryEdit * dee,
					  const gchar * path);

/* Copy the contents of this dentry into the display */
void        gnome_dentry_edit_set_dentry (GnomeDEntryEdit * dee,
					  GnomeDesktopEntry * dentry);

/* Generate a dentry based on the contents of the display */
GnomeDesktopEntry * gnome_dentry_edit_get_dentry(GnomeDEntryEdit * dee);
/*XXX: whoops!!!!, this call is left here just for binary
  compatibility, it should be gnome_dentry_edit_get_dentry*/
GnomeDesktopEntry * gnome_dentry_get_dentry(GnomeDEntryEdit * dee);

/* Return an allocated string, you need to g_free it. */
gchar *     gnome_dentry_edit_get_icon   (GnomeDEntryEdit * dee);
gchar *     gnome_dentry_edit_get_name   (GnomeDEntryEdit * dee);

/* These are accessor functions for the widgets that make up the
   GnomeDEntryEdit widget. */
GtkWidget * gnome_dentry_get_name_entry      (GnomeDEntryEdit * dee);
GtkWidget * gnome_dentry_get_comment_entry   (GnomeDEntryEdit * dee);
GtkWidget * gnome_dentry_get_exec_entry      (GnomeDEntryEdit * dee);
GtkWidget * gnome_dentry_get_tryexec_entry   (GnomeDEntryEdit * dee);
GtkWidget * gnome_dentry_get_doc_entry       (GnomeDEntryEdit * dee);
GtkWidget * gnome_dentry_get_icon_entry      (GnomeDEntryEdit * dee);

END_GNOME_DECLS
   
#endif /* GNOME_DENTRY_EDIT_H */




