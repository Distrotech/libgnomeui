/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
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

#ifndef GNOME_ICON_TEXT_H
#define GNOME_ICON_TEXT_H



G_BEGIN_DECLS

typedef struct {
	gchar *text;
	GdkWChar *text_wc;	/* text in wide characters */
	gint width;
	gint text_length;	/* number of characters */
} GnomeIconTextInfoRow;

typedef struct {
	GList *rows;
	GdkFont *font;
	gint width;
	gint height;
	gint baseline_skip;
} GnomeIconTextInfo;

GnomeIconTextInfo *gnome_icon_layout_text    (GdkFont *font, const gchar *text,
					      const gchar *separators, gint max_width,
					      gboolean confine);

void               gnome_icon_paint_text     (GnomeIconTextInfo *ti,
					      GdkDrawable *drawable, GdkGC *gc,
					      gint x, gint y,
					      GtkJustification just);

void               gnome_icon_text_info_free (GnomeIconTextInfo *ti);

G_END_DECLS

#endif /* GNOME_ICON_TEXT_H */
