/*
 * Copyright (C) 1998, 1999, 2000 Free Software Foundation
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

/* gnome-icon-text-item:  an editable text block with word wrapping for the
 * GNOME canvas.
 *
 * Authors: Miguel de Icaza <miguel@gnu.org>
 *          Federico Mena <federico@gimp.org>
 */

#ifndef _GNOME_ICON_TEXT_ITEM_H_
#define _GNOME_ICON_TEXT_ITEM_H_


#include <gtk/gtkentry.h>
#include <libgnomecanvas/gnome-canvas.h>
#include <libgnomeui/gnome-icon-text.h>

G_BEGIN_DECLS

#define GNOME_TYPE_ICON_TEXT_ITEM            (gnome_icon_text_item_get_type ())
#define GNOME_ICON_TEXT_ITEM(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_ICON_TEXT_ITEM, GnomeIconTextItem))
#define GNOME_ICON_TEXT_ITEM_CLASS(k)        (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ICON_TEXT_ITEM))
#define GNOME_IS_ICON_TEXT_ITEM(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_ICON_TEXT_ITEM))
#define GNOME_IS_ICON_TEXT_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ICON_TEXT_ITEM))
#define GNOME_ICON_TEXT_ITEM_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_ICON_TEXT_ITEM, GnomeIconTextItemClass))

/* This structure has been converted to use public and private parts.  To avoid
 * breaking binary compatibility, the slots for private fields have been
 * replaced with padding.  Please remove these fields when gnome-libs has
 * reached another major version and it is "fine" to break binary compatibility.
 */
typedef struct {
	GnomeCanvasItem canvas_item;

	/* Font name */
	char *fontname;

	/* Private data */
	gpointer priv; /* was GtkEntry *entry */

	/* Actual text */
	char *text;

	/* Text layout information */
	GnomeIconTextInfo *ti;

	/* Size and maximum allowed width */
	int x, y;
	int width;

	/* Whether the text is being edited */
	unsigned int editing : 1;

	/* Whether the text item is selected */
	unsigned int selected : 1;

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

GtkType  gnome_icon_text_item_get_type      (void) G_GNUC_CONST;

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

const char *gnome_icon_text_item_get_text   (GnomeIconTextItem *iti);

void     gnome_icon_text_item_start_editing (GnomeIconTextItem *iti);
void     gnome_icon_text_item_stop_editing  (GnomeIconTextItem *iti,
					     gboolean           accept);


G_END_DECLS

#endif /* _GNOME_ICON_ITEM_H_ */
