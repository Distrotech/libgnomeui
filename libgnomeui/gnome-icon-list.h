/* GnomeIconList widget - scrollable icon list
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Authors:
 *   Federico Mena   <federico@nuclecu.unam.mx>
 *   Miguel de Icaza <miguel@nuclecu.unam.mx>
 */

#ifndef _GNOME_ICON_LIST_H_
#define _GNOME_ICON_LIST_H_

#include <libgnome/gnome-defs.h>
#include <libgnomeui/gnome-canvas.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

BEGIN_GNOME_DECLS

#define GNOME_ICON_LIST(obj)     GTK_CHECK_CAST (obj, gnome_icon_list_get_type (), GnomeIconList)
#define GNOME_ICON_LIST_CLASS(k) GTK_CHECK_CLASS_CAST (k, gnome_icon_list_get_type (), GnomeIconListClass)
#define GNOME_IS_ICON_LIST(obj)  GTK_CHECK_TYPE (obj, gnome_icon_list_get_type ())

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
typedef struct {
	GnomeCanvas canvas;

	/* Scroll adjustments */
	GtkAdjustment *adj;
	GtkAdjustment *hadj;

	/* Number of icons in the list */
	int icons;

	/* Private data */
	gpointer priv; /* was GList *icon_list */

	/* A list of integers with the indices of the currently selected icons */
	GList *selection;
} GnomeIconList;

typedef struct {
	GnomeCanvasClass parent_class;

	void     (*select_icon)    (GnomeIconList *gil, gint num, GdkEvent *event);
	void     (*unselect_icon)  (GnomeIconList *gil, gint num, GdkEvent *event);
	gboolean (*text_changed)   (GnomeIconList *gil, gint num, const char *new_text);
} GnomeIconListClass;

#define GNOME_ICON_LIST_IS_EDITABLE 1
#define GNOME_ICON_LIST_STATIC_TEXT 2

guint          gnome_icon_list_get_type            (void);

GtkWidget     *gnome_icon_list_new                 (guint         icon_width,
						    GtkAdjustment *adj,
						    int           flags);
GtkWidget     *gnome_icon_list_new_flags           (guint         icon_width,
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
						    int pos, const char *icon_filename,
						    const char *text);

void           gnome_icon_list_insert_pixbuf        (GnomeIconList *gil,
						     int pos, GdkPixbuf *im,
						     const char *text);

int            gnome_icon_list_append              (GnomeIconList *gil,
						    const char *icon_filename,
						    const char *text);
int            gnome_icon_list_append_pixbuf        (GnomeIconList *gil,
						     GdkPixbuf *im,
						     const char *text);
void           gnome_icon_list_clear               (GnomeIconList *gil);
void           gnome_icon_list_remove              (GnomeIconList *gil, int pos);


/* Managing the selection */
void           gnome_icon_list_set_selection_mode  (GnomeIconList *gil,
						    GtkSelectionMode mode);
void           gnome_icon_list_select_icon         (GnomeIconList *gil,
						    int idx);
void           gnome_icon_list_unselect_icon       (GnomeIconList *gil,
						    int pos);
int            gnome_icon_list_unselect_all        (GnomeIconList *gil,
						    GdkEvent *event, gpointer keep);

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

/* Attaching information to the icons */
void           gnome_icon_list_set_icon_data       (GnomeIconList *gil,
						    int pos, gpointer data);
void           gnome_icon_list_set_icon_data_full  (GnomeIconList *gil,
						    int pos, gpointer data,
						    GtkDestroyNotify destroy);
int            gnome_icon_list_find_icon_from_data (GnomeIconList *gil,
						    gpointer data);
gpointer       gnome_icon_list_get_icon_data       (GnomeIconList *gil,
						    int pos);

/* Visibility */
void           gnome_icon_list_moveto              (GnomeIconList *gil,
						    int pos, double yalign);
GtkVisibility  gnome_icon_list_icon_is_visible     (GnomeIconList *gil,
						    int pos);

int            gnome_icon_list_get_icon_at         (GnomeIconList *gil, int x, int y);

int            gnome_icon_list_get_items_per_line  (GnomeIconList *gil);

END_GNOME_DECLS

#endif /* _GNOME_ICON_LIST_H_ */
