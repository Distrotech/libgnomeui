#ifndef _GNOME_ICON_TEXT_ITEM_H_
#define _GNOME_ICON_TEXT_ITEM_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkentry.h>
#include <libgnomeui/gnome-canvas.h>
#include <libgnomeui/gnome-icon-list.h>
#include <libgnomeui/gnome-icon-text.h>

BEGIN_GNOME_DECLS

#define GNOME_ICON_TEXT_ITEM(obj)     (GTK_CHECK_CAST((obj), \
        gnome_icon_text_item_get_type (), GnomeIconTextItem))
#define GNOME_ICON_TEXT_ITEM_CLASS(k) (GTK_CHECK_CLASS_CAST ((k),\
	gnome_icon_text_item_get_type ()))
#define GNOME_IS_ICON_TEXT_ITEM(o)    (GTK_CHECK_TYPE((o), \
	gnome_icon_text_item_get_type ()))

typedef struct {
	GnomeCanvasItem canvas_item;

	int      x, y;
	int      width;		/* Our assigned width */
	char     *fontname;	/* Font in which we display */
	gboolean is_editable;	/* Is this editable? */

	/* Flags */
	unsigned int editing:1; /* true if it is being edited */
	unsigned int selected:1;/* true if it should be displayed as selected */
	
	/* Hack: create an offscreen window, and place the entry there */
	GtkEntry  *entry;
	GtkWidget *entry_top;

	/* Standard equipment */
	GdkFont *font;

	char    *text;

	/* Layed out information */
	GnomeIconTextInfo *ti;
} GnomeIconTextItem;

typedef struct {
	GnomeCanvasItemClass parent_class;

	/* Signals we emit */
	int  (*text_changed)     (GnomeIconTextItem *);
	void (*height_changed)   (GnomeIconTextItem *);
} GnomeIconTextItemClass;

GtkType  gnome_icon_text_item_get_type  (void);
void     gnome_icon_text_item_configure (GnomeIconTextItem *iti,
					 int         x,
					 int         y,
					 int         width,
					 const char *fontname,
					 const char *text,
					 gboolean is_editable);
void     gnome_icon_text_item_setxy     (GnomeIconTextItem *iti,
					 int      x,
					 int      y);
void     gnome_icon_text_item_select    (GnomeIconTextItem *iti,
					 int      sel);
END_GNOME_DECLS
#endif /* _GNOME_ICON_ITEM_H_ */

