/* GnomeIconList widget - scrollable icon list
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_ICON_LIST_H
#define GNOME_ICON_LIST_H


#include <gtk/gtkcontainer.h>
#include <libgnome/gnome-defs.h>


BEGIN_GNOME_DECLS


#define GNOME_ICON_LIST(obj)         GTK_CHECK_CAST (obj, gnome_icon_list_get_type (), GnomeIconList)
#define GNOME_ICON_LIST_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gnome_icon_list_get_type (), GnomeIconListClass)
#define GNOME_IS_ICON_LIST(obj)      GTK_CHECK_TYPE (obj, gnome_icon_list_get_type ())


typedef enum {
	GNOME_ICON_LIST_ICONS,
	GNOME_ICON_LIST_TEXT_BELOW,
	GNOME_ICON_LIST_TEXT_RIGHT
} GnomeIconListMode;

typedef struct _GnomeIconList      GnomeIconList;
typedef struct _GnomeIconListClass GnomeIconListClass;

struct _GnomeIconList {
	GtkContainer container;

	int icons;
	GList *icon_list;
	GList *icon_list_end;

	int row_spacing;
	int col_spacing;
	int text_spacing;
	int icon_border;

	int x_offset;
	int y_offset;

	int max_icon_width;
	int max_icon_height;
	int max_pixmap_width;
	int max_pixmap_height;

	int icon_rows;
	int icon_cols;

	char *separators;

	GnomeIconListMode mode;
	int frozen : 1;

	GdkWindow *ilist_window;
	int ilist_window_width;
	int ilist_window_height;

	GdkGC *fg_gc;
	GdkGC *bg_gc;

	GtkShadowType border_type;
	GtkSelectionMode selection_mode;

	GList *selection;

	GtkWidget *vscrollbar;
	GtkWidget *hscrollbar;
	GtkPolicyType vscrollbar_policy;
	GtkPolicyType hscrollbar_policy;
	int vscrollbar_width;
	int hscrollbar_height;
	int last_selected;
	int last_clicked;
	int dirty;
};

struct _GnomeIconListClass {
	GtkContainerClass parent_class;

	void (* select_icon)   (GnomeIconList *ilist, gint num, GdkEvent *event);
	void (* unselect_icon) (GnomeIconList *ilist, gint num, GdkEvent *event);
};

guint          gnome_icon_list_get_type            (void);
GtkWidget     *gnome_icon_list_new                 (void);

void           gnome_icon_list_set_selection_mode  (GnomeIconList *ilist, GtkSelectionMode mode);
void           gnome_icon_list_set_policy          (GnomeIconList *ilist,
						    GtkPolicyType vscrollbar_policy,
						    GtkPolicyType hscrollbar_policy);

int            gnome_icon_list_append              (GnomeIconList *ilist, char *icon_filename, char *text);
void           gnome_icon_list_insert              (GnomeIconList *ilist, int pos, char *icon_filename, char *text);
void           gnome_icon_list_remove              (GnomeIconList *ilist, int pos);

int            gnome_icon_list_append_imlib        (GnomeIconList *ilist, GdkImlibImage *im, char *text);
void           gnome_icon_list_clear               (GnomeIconList *ilist);

void           gnome_icon_list_set_icon_data       (GnomeIconList *ilist, int pos, gpointer data);
void           gnome_icon_list_set_icon_data_full  (GnomeIconList *ilist, int pos, gpointer data,
						    GtkDestroyNotify destroy);
gpointer       gnome_icon_list_get_icon_data       (GnomeIconList *ilist, int pos);
int            gnome_icon_list_find_icon_from_data (GnomeIconList *ilist, gpointer data);

void           gnome_icon_list_select_icon         (GnomeIconList *ilist, int pos);
void           gnome_icon_list_unselect_icon       (GnomeIconList *ilist, int pos);

void           gnome_icon_list_freeze              (GnomeIconList *ilist);
void           gnome_icon_list_thaw                (GnomeIconList *ilist);

void           gnome_icon_list_moveto              (GnomeIconList *ilist, int pos, double yalign);
GtkVisibility  gnome_icon_list_icon_is_visible     (GnomeIconList *ilist, int pos);

void           gnome_icon_list_set_foreground     (GnomeIconList *ilist, int pos, GdkColor *color);
void           gnome_icon_list_set_background     (GnomeIconList *ilist, int pos, GdkColor *color);

void           gnome_icon_list_set_row_spacing    (GnomeIconList *ilist, int spacing);
void           gnome_icon_list_set_col_spacing    (GnomeIconList *ilist, int spacing);
void           gnome_icon_list_set_text_spacing   (GnomeIconList *ilist, int spacing);
void           gnome_icon_list_set_icon_border    (GnomeIconList *ilist, int spacing);

void           gnome_icon_list_set_separators     (GnomeIconList *ilist, char *separators);

void           gnome_icon_list_set_mode           (GnomeIconList *ilist, GnomeIconListMode mode);
void           gnome_icon_list_set_border         (GnomeIconList *ilist, GtkShadowType border);

int            gnome_icon_list_get_icon_at        (GnomeIconList *ilist, int x, int y);
void           gnome_icon_list_unselect_all       (GnomeIconList *ilist, GdkEvent *event, void *keep);


struct gnome_icon_text_info_row {
	char *text;
	int width;
};

struct gnome_icon_text_info {
	GList *rows;
	GdkFont *font;
	int width;
	int height;
	int baseline_skip;
};

/* Text layout routine for icons with text */
void                         gnome_icon_text_info_free (struct gnome_icon_text_info *ti);
struct gnome_icon_text_info *gnome_icon_layout_text    (GdkFont *font, char *text, char *separators,
							int max_width, int confine);
void                         gnome_icon_paint_text     (struct gnome_icon_text_info *ti,
							GdkDrawable *drawable, GdkGC *gc,
							int x_ofs, int y_ofs, int width);

END_GNOME_DECLS

#endif
