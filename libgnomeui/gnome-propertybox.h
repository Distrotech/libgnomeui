/* gnome-propertybox.h - Property dialog box.

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

#ifndef __GNOME_PROPERTY_BOX_H__
#define __GNOME_PROPERTY_BOX_H__

#include "gnome-dialog.h"
#include "libgnome/gnome-defs.h"

BEGIN_GNOME_DECLS

#define GNOME_TYPE_PROPERTY_BOX            (gnome_property_box_get_type ())
#define GNOME_PROPERTY_BOX(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_PROPERTY_BOX, GnomePropertyBox))
#define GNOME_PROPERTY_BOX_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_PROPERTY_BOX, GnomePropertyBoxClass))
#define GNOME_IS_PROPERTY_BOX(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_PROPERTY_BOX))
#define GNOME_IS_PROPERTY_BOX_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_PROPERTY_BOX))

/*the flag used on the notebook pages to see if a change happened on a certain page or not*/
#define GNOME_PROPERTY_BOX_DIRTY	"gnome_property_box_dirty"


typedef struct _GnomePropertyBox      GnomePropertyBox;
typedef struct _GnomePropertyBoxClass GnomePropertyBoxClass;

struct _GnomePropertyBox
{
	GnomeDialog dialog;

	GtkWidget *notebook;	    /* The notebook widget.  */
	GtkWidget *ok_button;	    /* OK button.  */
	GtkWidget *apply_button;    /* Apply button.  */
	GtkWidget *cancel_button;   /* Cancel/Close button.  */
	GtkWidget *help_button;	    /* Help button.  */
};

struct _GnomePropertyBoxClass
{
	GnomeDialogClass parent_class;

	void (* apply) (GnomePropertyBox *propertybox, gint page_num);
	void (* help)  (GnomePropertyBox *propertybox, gint page_num);
};

guint     gnome_property_box_get_type    (void);
GtkWidget *gnome_property_box_new        (void);

/*
 * Call this when the user changes something in the current page of
 * the notebook.
 */
void      gnome_property_box_changed     (GnomePropertyBox *property_box);

void      gnome_property_box_set_modified(GnomePropertyBox *property_box,
					  gboolean state);


gint	  gnome_property_box_append_page (GnomePropertyBox *property_box,
					  GtkWidget *child,
					  GtkWidget *tab_label);

#ifndef GNOME_EXCLUDE_DEPRECATED
/* Deprecated, use set_modified */
void      gnome_property_box_set_state   (GnomePropertyBox *property_box,
					  gboolean state);
#endif

END_GNOME_DECLS

#endif /* __GNOME_PROPERTY_BOX_H__ */
