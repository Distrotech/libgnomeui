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

/*
 * This define should be removed after GnomeLibs 0.40 has been released
 * and any code depending on the old ICON_LIST code should be fixed by then.
 */
#define GNOME_ICON_LIST_VERSION2

#include <libgnome/gnome-defs.h>
#include <libgnomeui/gnome-canvas.h>

#ifndef __GDK_IMLIB_H__
#include <gdk_imlib.h>
#endif

BEGIN_GNOME_DECLS

#define GNOME_ICON_LIST(obj)     GTK_CHECK_CAST (obj, gnome_icon_list_get_type (), GnomeIconList)
#define GNOME_ICON_LIST_CLASS(k) GTK_CHECK_CLASS_CAST (k, gnome_icon_list_get_type (), GnomeIconListClass)
#define GNOME_IS_ICON_LIST(obj)  GTK_CHECK_TYPE (obj, gnome_icon_list_get_type ())

typedef enum {
	GNOME_ICON_LIST_ICONS,
	GNOME_ICON_LIST_TEXT_BELOW,
	GNOME_ICON_LIST_TEXT_RIGHT
} GnomeIconListMode;

typedef struct {
	GnomeCanvas canvas;

	GtkAdjustment *adj;	/* The scrolling adjustment we use */
        GtkAdjustment *hadj;	/* dummmy need for GtkScrolledWindow compat */
  
	int icons;		/* Number of icons in the IconList */
	GList *icon_list;	/* The list of icons */
	
	int frozen;		/* frozen count */
	int dirty;		/* dirty flag */

	/* Various display spacing configuration */
	int row_spacing;
	int col_spacing;
	int text_spacing;
	int icon_border;

	/* Separators used to wrap the text displayed below the icon */
	char *separators;

	/* Display mode */
	GnomeIconListMode mode;

	/* Selection  */
	GtkSelectionMode selection_mode;

	/* A list of integers with the indexes of the current selection */
	GList *selection;

	/* Internal: used during band-selection */
	GList *preserve_selection; 

	int icon_width;		/* The icon width */
	int is_editable;	/* Whether the icon names are editable or not  */
	
	int last_selected;	/* Last icon selected */
	void *last_clicked;	/* Icon * */

	/* Timed scrolling */
	int timer_tag;		/* timeout tag for autoscrolling */
	int value_diff;		/* change the adjustment value by this */
	gdouble event_last_x;	/* The X position last time we read it */
	gdouble event_last_y;	/* The Y position last time we read it */
	
	/* Opaque to the user */
	GList *lines;

	/* Mouse band selection state */
	double sel_start_x;
	double sel_start_y;

	GnomeCanvasItem *sel_rect;
} GnomeIconList;

typedef struct {
	GnomeCanvasClass parent_class;

	void     (*select_icon)    (GnomeIconList *gil, gint num, GdkEvent *event);
	void     (*unselect_icon)  (GnomeIconList *gil, gint num, GdkEvent *event);
	gboolean (*text_changed)   (GnomeIconList *gil, gint num, const char *new_text);
} GnomeIconListClass;

guint          gnome_icon_list_get_type            (void);

GtkWidget     *gnome_icon_list_new                 (guint         icon_width, 
						    GtkAdjustment *adj,
						    gboolean      is_editable);
void           gnome_icon_list_construct           (GnomeIconList *gil,
						    guint icon_width,
						    GtkAdjustment *adj,
						    gboolean is_editable);

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
						    char *text);
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
						    GdkEvent *event, void *keep);

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




