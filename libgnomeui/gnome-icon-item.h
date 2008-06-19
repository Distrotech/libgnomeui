/*
 * Copyright (C) 1998, 1999, 2000, 2001 Free Software Foundation
 * Copyright (C) 2001 Anders Carlsson
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
 *
 * Author: Anders Carlsson <andersca@gnu.org>
 *
 * Based on the GNOME 1.0 icon item by Miguel de Icaza and Federico Mena.
 */

#ifndef _GNOME_ICON_TEXT_ITEM_H_
#define _GNOME_ICON_TEXT_ITEM_H_

#ifndef GNOME_DISABLE_DEPRECATED

#include <libgnomecanvas/gnome-canvas.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _GnomeIconTextItem        GnomeIconTextItem;
typedef struct _GnomeIconTextItemClass   GnomeIconTextItemClass;
typedef struct _GnomeIconTextItemPrivate GnomeIconTextItemPrivate;

#define GNOME_TYPE_ICON_TEXT_ITEM            (gnome_icon_text_item_get_type ())
#define GNOME_ICON_TEXT_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_ICON_TEXT_ITEM, GnomeIconTextItem))
#define GNOME_ICON_TEXT_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ICON_TEXT_ITEM, GnomeIconTextItemClass))
#define GNOME_IS_ICON_TEXT_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_ICON_TEXT_ITEM))
#define GNOME_IS_ICON_TEXT_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ICON_TEXT_ITEM))
#define GNOME_ICON_TEXT_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_ICON_TEXT_ITEM, GnomeIconTextItemClass))

struct _GnomeIconTextItem {
	GnomeCanvasItem parent_instance;

	/* Size and maximum allowed width */
	int x, y;
	int width;

	/* Font name */
	char *fontname;

	/* Actual text */
	char *text;

	/* Whether the text is being edited */
	unsigned int editing : 1;

	/* Whether the text item is selected */
	unsigned int selected : 1;

	/* Whether the text item is focused */
	unsigned int focused : 1;
	
	/* Whether the text is editable */
	unsigned int is_editable : 1;

	/* Whether the text is allocated by us (FALSE if allocated by the client) */
	unsigned int is_text_allocated : 1;

	GnomeIconTextItemPrivate *_priv;
};

struct _GnomeIconTextItemClass {
	GnomeCanvasItemClass parent_class;

	/* Signals we emit */
	gboolean  (* text_changed)      (GnomeIconTextItem *iti);
	void (* height_changed)    (GnomeIconTextItem *iti);
	void (* width_changed)     (GnomeIconTextItem *iti);
	void (* editing_started)   (GnomeIconTextItem *iti);
	void (* editing_stopped)   (GnomeIconTextItem *iti);
	void (* selection_started) (GnomeIconTextItem *iti);
	void (* selection_stopped) (GnomeIconTextItem *iti);
	
	/* Virtual functions */
	GtkEntry* (* create_entry)  (GnomeIconTextItem *iti);

        /* Padding for possible expansion */
	gpointer padding1;
};

GType        gnome_icon_text_item_get_type      (void) G_GNUC_CONST;

void         gnome_icon_text_item_configure     (GnomeIconTextItem *iti,
						 int                x,
						 int                y,
						 int                width,
						 const char        *fontname,
						 const char        *text,
						 gboolean           is_editable,
						 gboolean           is_static);

void         gnome_icon_text_item_setxy         (GnomeIconTextItem *iti,
						 int                x,
						 int                y);

void         gnome_icon_text_item_select        (GnomeIconTextItem *iti,
						 gboolean                sel);

void         gnome_icon_text_item_focus         (GnomeIconTextItem *iti,
						 gboolean                focused);

const char  *gnome_icon_text_item_get_text      (GnomeIconTextItem *iti);

void         gnome_icon_text_item_start_editing (GnomeIconTextItem *iti);

void         gnome_icon_text_item_stop_editing  (GnomeIconTextItem *iti,
						 gboolean           accept);

GtkEditable *gnome_icon_text_item_get_editable  (GnomeIconTextItem *iti);

G_END_DECLS

#endif /* GNOME_DISABLE_DEPRECATED */

#endif /* _GNOME_ICON_TEXT_ITEM_H_ */
