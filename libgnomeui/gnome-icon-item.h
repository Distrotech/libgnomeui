/* gnome-icon-text-item:  an editable text block with word wrapping for the
 * GNOME canvas.
 *
 * Copyright (C) 1998, 1999 The Free Software Foundation
 *
 * Authors: Miguel de Icaza <miguel@gnu.org>
 *          Federico Mena <federico@gimp.org>
 */

#ifndef _GNOME_ICON_TEXT_ITEM_H_
#define _GNOME_ICON_TEXT_ITEM_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkentry.h>
#include <libgnomeui/gnome-canvas.h>
#include <libgnomeui/gnome-icon-text.h>

BEGIN_GNOME_DECLS

#define GNOME_TYPE_ICON_TEXT_ITEM            (gnome_icon_text_item_get_type ())
#define GNOME_ICON_TEXT_ITEM(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_ICON_TEXT_ITEM, GnomeIconTextItem))
#define GNOME_ICON_TEXT_ITEM_CLASS(k)        (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ICON_TEXT_ITEM))
#define GNOME_IS_ICON_TEXT_ITEM(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_ICON_TEXT_ITEM))
#define GNOME_IS_ICON_TEXT_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ICON_TEXT_ITEM))

/* This structure has been converted to use public and private parts.  To avoid
 * breaking binary compatibility, the slots for private fields have been
 * replaced with padding.  Please remove these fields when gnome-libs has
 * reached another major version and it is "fine" to break binary compatibility.
 */
typedef struct {
	GnomeCanvasItem canvas_item;

	/* Size and maximum allowed width */
	int x, y;
	int width;

	/* Font name */
	char *fontname;

	/* Private data */
	gpointer priv; /* was GtkEntry *entry */
	gpointer pad1; /* was GtkWidget *entry_top */
	gpointer pad2; /* was GdkFont *font */

	/* Actual text */
	char *text;

	/* Text layout information */
	GnomeIconTextInfo *ti;

	/* Whether the text is being edited */
	unsigned int editing : 1;

	/* Whether the text item is selected */
	unsigned int selected : 1;

	unsigned int pad3; /* was unsigned int unselected_click : 1 */

	/* Whether the user is select-dragging a block of text */
	unsigned int selecting : 1;

	/* Whether the text is editable */
	unsigned int is_editable : 1;

	/* Whether the text is allocated by us (FALSE if allocated by the client) */
	unsigned int is_text_allocated : 1;
} GnomeIconTextItem;

typedef struct {
	GnomeCanvasItemClass parent_class;

	/* Signals we emit */
	int  (* text_changed)      (GnomeIconTextItem *iti);
	void (* height_changed)    (GnomeIconTextItem *iti);
	void (* width_changed)     (GnomeIconTextItem *iti);
	void (* editing_started)   (GnomeIconTextItem *iti);
	void (* editing_stopped)   (GnomeIconTextItem *iti);
	void (* selection_started) (GnomeIconTextItem *iti);
	void (* selection_stopped) (GnomeIconTextItem *iti);
} GnomeIconTextItemClass;

GtkType  gnome_icon_text_item_get_type      (void);

void     gnome_icon_text_item_configure     (GnomeIconTextItem *iti,
					     int                x,
					     int                y,
					     int                width,
					     const char        *fontname,
					     const char        *text,
					     gboolean           is_editable,
					     gboolean           is_static);

void     gnome_icon_text_item_setxy         (GnomeIconTextItem *iti,
					     int                x,
					     int                y);

void     gnome_icon_text_item_select        (GnomeIconTextItem *iti,
					     int                sel);

char    *gnome_icon_text_item_get_text      (GnomeIconTextItem *iti);

void     gnome_icon_text_item_start_editing (GnomeIconTextItem *iti);
void     gnome_icon_text_item_stop_editing  (GnomeIconTextItem *iti,
					     gboolean           accept);


END_GNOME_DECLS

#endif /* _GNOME_ICON_ITEM_H_ */

