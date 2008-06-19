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

#ifndef GNOME_DISABLE_DEPRECATED

#include <libgnomecanvas/gnome-canvas.h>
#include <libgnomecanvas/gnome-canvas-rich-text.h>
#include <libgnomecanvas/gnome-canvas-pixbuf.h>
#include <libgnomeui/gnome-icon-item.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define GNOME_TYPE_ICON_LIST            (gnome_icon_list_get_type ())
#define GNOME_ICON_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_ICON_LIST, GnomeIconList))
#define GNOME_ICON_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ICON_LIST, GnomeIconListClass))
#define GNOME_IS_ICON_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_ICON_LIST))
#define GNOME_IS_ICON_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ICON_LIST))
#define GNOME_ICON_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_ICON_LIST, GnomeIconListClass))

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

	/* Scroll adjustments */
	GtkAdjustment *adj;
	GtkAdjustment *hadj;
	
	/*< private >*/
	GnomeIconListPrivate * _priv;
};

struct _GnomeIconListClass {
	GnomeCanvasClass parent_class;

	void     (*select_icon)      (GnomeIconList *gil, gint num, GdkEvent *event);
	void     (*unselect_icon)    (GnomeIconList *gil, gint num, GdkEvent *event);
	void     (*focus_icon)       (GnomeIconList *gil, gint num);
	gboolean (*text_changed)     (GnomeIconList *gil, gint num, const char *new_text);

	/* Key Binding signals */
	void     (*move_cursor)      (GnomeIconList *gil, GtkDirectionType dir, gboolean clear_selection);
	void     (*toggle_cursor_selection) (GnomeIconList *gil);
	void     (*unused)       (GnomeIconList *unused);
	
	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};

enum {
	GNOME_ICON_LIST_IS_EDITABLE	= 1 << 0,
	GNOME_ICON_LIST_STATIC_TEXT	= 1 << 1
};

GType          gnome_icon_list_get_type            (void) G_GNUC_CONST;

GtkWidget     *gnome_icon_list_new                 (guint         icon_width,
						    GtkAdjustment *adj,
						    int           flags);

void           gnome_icon_list_construct           (GnomeIconList *gil,
						    guint icon_width,
						    GtkAdjustment *adj,
						    int flags);

void           gnome_icon_list_set_hadjustment    (GnomeIconList *gil,
						   GtkAdjustment *hadj);

void           gnome_icon_list_set_vadjustment    (GnomeIconList *gil,
						   GtkAdjustment *vadj);

/* To avoid excesive recomputes during insertion/deletion */
void           gnome_icon_list_freeze              (GnomeIconList *gil);
void           gnome_icon_list_thaw                (GnomeIconList *gil);


void           gnome_icon_list_insert              (GnomeIconList *gil,
						    int pos,
						    const char *icon_filename,
						    const char *text);
void           gnome_icon_list_insert_pixbuf       (GnomeIconList *gil,
						    int pos,
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
						    int pos);

guint          gnome_icon_list_get_num_icons       (GnomeIconList *gil);


/* Managing the selection */
GtkSelectionMode gnome_icon_list_get_selection_mode(GnomeIconList *gil);
void           gnome_icon_list_set_selection_mode  (GnomeIconList *gil,
						    GtkSelectionMode mode);
void           gnome_icon_list_select_icon         (GnomeIconList *gil,
						    int pos);
void           gnome_icon_list_unselect_icon       (GnomeIconList *gil,
						    int pos);
void           gnome_icon_list_select_all          (GnomeIconList *gil);
int            gnome_icon_list_unselect_all        (GnomeIconList *gil);
GList *        gnome_icon_list_get_selection       (GnomeIconList *gil);

/* Managing focus */
void           gnome_icon_list_focus_icon          (GnomeIconList *gil,
						    gint idx);

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
						    int pos, gpointer data,
						    GDestroyNotify destroy);
int            gnome_icon_list_find_icon_from_data (GnomeIconList *gil,
						    gpointer data);
gpointer       gnome_icon_list_get_icon_data       (GnomeIconList *gil,
						    int pos);

/* Visibility */
void           gnome_icon_list_moveto              (GnomeIconList *gil,
						    int pos, double yalign);
GtkVisibility  gnome_icon_list_icon_is_visible     (GnomeIconList *gil,
						    int pos);

int            gnome_icon_list_get_icon_at         (GnomeIconList *gil,
						    int x, int y);

int            gnome_icon_list_get_items_per_line  (GnomeIconList *gil);

/* Accessibility functions */
GnomeIconTextItem *gnome_icon_list_get_icon_text_item (GnomeIconList *gil,
						       int idx);

GnomeCanvasPixbuf *gnome_icon_list_get_icon_pixbuf_item (GnomeIconList *gil,
							 int idx);

G_END_DECLS

#endif /* GNOME_DISABLE_DEPRECATED */

#endif /* _GNOME_ICON_LIST_H_ */

