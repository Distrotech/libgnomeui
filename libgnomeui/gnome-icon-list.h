/*
 * Copyright (C) 1998, 1999 Free Software Foundation
 * Copyright (C) 2000 Red Hat, Inc.
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

/* GnomeIconList widget - scrollable icon list
 *
 *
 * Authors:
 *   Federico Mena   <federico@nuclecu.unam.mx>
 *   Miguel de Icaza <miguel@nuclecu.unam.mx>
 */

#ifndef _GNOME_ICON_LIST_H_
#define _GNOME_ICON_LIST_H_


#include <libgnomecanvas/gnome-canvas.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define GNOME_TYPE_ICON_LIST            (gnome_icon_list_get_type ())
#define GNOME_ICON_LIST(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_ICON_LIST, GnomeIconList))
#define GNOME_ICON_LIST_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ICON_LIST, GnomeIconListClass))
#define GNOME_IS_ICON_LIST(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_ICON_LIST))
#define GNOME_IS_ICON_LIST_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ICON_LIST))
#define GNOME_ICON_LIST_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_ICON_LIST, GnomeIconListClass))

typedef struct _GnomeIconList        GnomeIconList;
typedef struct _GnomeIconListPrivate GnomeIconListPrivate;
typedef struct _GnomeIconListClass   GnomeIconListClass;

typedef enum {
	GNOME_ICON_LIST_ICONS,
	GNOME_ICON_LIST_TEXT_BELOW,
	GNOME_ICON_LIST_TEXT_RIGHT
} GnomeIconListMode;

/* This structure has been converted to use public and private parts.  To avoid
 * breaking binary compatibility, the slots for private fields have been
 * replaced with padding.  Please remove these fields when gnome-libs has
 * reached another major version and it is "fine" to break binary compatibility.
 */
struct _GnomeIconList {
	GnomeCanvas canvas;

	/*< private >*/
	GnomeIconListPrivate * _priv;
};

struct _GnomeIconListClass {
	GnomeCanvasClass parent_class;

	void     (*select_icon)    (GnomeIconList *gil, gint num, GdkEvent *event);
	void     (*unselect_icon)  (GnomeIconList *gil, gint num, GdkEvent *event);
	gboolean (*text_changed)   (GnomeIconList *gil, gint num, const char *new_text);
};

enum {
	GNOME_ICON_LIST_IS_EDITABLE	= 1 << 0,
	GNOME_ICON_LIST_STATIC_TEXT	= 1 << 1
};

guint          gnome_icon_list_get_type            (void) G_GNUC_CONST;

GtkWidget     *gnome_icon_list_new                 (guint         icon_width,
						    int           flags);
void           gnome_icon_list_construct           (GnomeIconList *gil,
						    guint icon_width,
						    int flags);


/* To avoid excesive recomputes during insertion/deletion */
void           gnome_icon_list_freeze              (GnomeIconList *gil);
void           gnome_icon_list_thaw                (GnomeIconList *gil);


void           gnome_icon_list_insert              (GnomeIconList *gil,
						    int idx,
						    const char *icon_filename,
						    const char *text);
void           gnome_icon_list_insert_pixbuf       (GnomeIconList *gil,
						    int idx,
						    GdkPixbuf *im,
						    const char *icon_filename,
						    const char *text);

int            gnome_icon_list_append              (GnomeIconList *gil,
						    const char *icon_filename,
						    const char *text);
int            gnome_icon_list_append_pixbuf       (GnomeIconList *gil,
						    GdkPixbuf *im,
						    const char *icon_filename,
						    const char *text);

void           gnome_icon_list_clear               (GnomeIconList *gil);
void           gnome_icon_list_remove              (GnomeIconList *gil,
						    int idx);

guint          gnome_icon_list_get_num_icons       (GnomeIconList *gil);


/* Managing the selection */
GtkSelectionMode gnome_icon_list_get_selection_mode(GnomeIconList *gil);
void           gnome_icon_list_set_selection_mode  (GnomeIconList *gil,
						    GtkSelectionMode mode);
void           gnome_icon_list_select_icon         (GnomeIconList *gil,
						    int idx);
void           gnome_icon_list_unselect_icon       (GnomeIconList *gil,
						    int idx);
int            gnome_icon_list_unselect_all        (GnomeIconList *gil);
GList *        gnome_icon_list_get_selection       (GnomeIconList *gil);

/* Setting the spacing values */
void           gnome_icon_list_set_icon_width      (GnomeIconList *gil,
						    int w);
void           gnome_icon_list_set_row_spacing     (GnomeIconList *gil,
						    int pixels);
void           gnome_icon_list_set_col_spacing     (GnomeIconList *gil,
						    int pixels);
void           gnome_icon_list_set_text_spacing    (GnomeIconList *gil,
						    int pixels);
void           gnome_icon_list_set_icon_border     (GnomeIconList *gil,
						    int pixels);
void           gnome_icon_list_set_separators      (GnomeIconList *gil,
						    const char *sep);
/* Icon filename. */
gchar *        gnome_icon_list_get_icon_filename   (GnomeIconList *gil,
						    int idx);
int            gnome_icon_list_find_icon_from_filename (GnomeIconList *gil,
							const char *filename);

/* Attaching information to the icons */
void           gnome_icon_list_set_icon_data       (GnomeIconList *gil,
						    int idx, gpointer data);
void           gnome_icon_list_set_icon_data_full  (GnomeIconList *gil,
						    int idx, gpointer data,
						    GtkDestroyNotify destroy);
int            gnome_icon_list_find_icon_from_data (GnomeIconList *gil,
						    gpointer data);
gpointer       gnome_icon_list_get_icon_data       (GnomeIconList *gil,
						    int idx);

/* Visibility */
void           gnome_icon_list_moveto              (GnomeIconList *gil,
						    int idx, double yalign);
GtkVisibility  gnome_icon_list_icon_is_visible     (GnomeIconList *gil,
						    int idx);

int            gnome_icon_list_get_icon_at         (GnomeIconList *gil,
						    int x, int y);

int            gnome_icon_list_get_items_per_line  (GnomeIconList *gil);

G_END_DECLS

#endif /* _GNOME_ICON_LIST_H_ */
