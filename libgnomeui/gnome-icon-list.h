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
#include <gdk_imlib.h>

BEGIN_GNOME_DECLS

#define GNOME_TYPE_ICON_LIST            (gnome_icon_list_get_type ())
#define GNOME_ICON_LIST(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_ICON_LIST, GnomeIconList))
#define GNOME_ICON_LIST_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ICON_LIST, GnomeIconListClass))
#define GNOME_IS_ICON_LIST(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_ICON_LIST))
#define GNOME_IS_ICON_LIST_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ICON_LIST))

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

	int pad3; /* was int frozen */
	int pad4; /* was int dirty */
	int pad5; /* was int row_spacing */
	int pad6; /* was int col_spacing */
	int pad7; /* was int text_spacing */
	int pad8; /* was int icon_border */
	gpointer pad9; /* was char *separators */
	GnomeIconListMode pad10; /* was GnomeIconListMode mode */
	GtkSelectionMode pad11; /* was GtkSelectionMode selection_mode */

	/* A list of integers with the indices of the currently selected icons */
	GList *selection;

	gpointer pad12; /* was GList *preserve_selection */
	int pad13; /* was int icon_width */
	unsigned int pad14 : 1; /* was unsigned int is_editable : 1 */
	unsigned int pad15 : 1; /* was unsigned int static_text : 1 */
	int pad16; /* was int last_selected */
	gpointer pad17; /* was void *last_clicked */
	int pad18; /* was int timer_tag */
	int pad19; /* was int value_diff */
	gdouble pad20; /* was gdouble event_last_x */
	gdouble pad21; /* was gdouble event_last_y */
	gpointer pad22; /* was GList *lines */
	int pad23; /* was int total_height */
	double pad24; /* was double sel_start_x */
	double pad25; /* was double sel_start_y */
	gpointer pad26; /* was GnomeCanvasItem *sel_rect */
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


void           gnome_icon_list_insert_imlib        (GnomeIconList *gil,
						    int pos, GdkImlibImage *im,
						    const char *text);

int            gnome_icon_list_append              (GnomeIconList *gil,
						    const char *icon_filename,
						    const char *text);
int            gnome_icon_list_append_imlib        (GnomeIconList *gil,
						    GdkImlibImage *im,
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
