/*
 * private header file for gnome-*-selector.h.
 *
 * Copyright (C) 2000 SuSE GmbH
 * Author: Martin Baulig <baulig@suse.de>
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

#ifndef GNOME_SELECTOR_PRIVATE_H
#define GNOME_SELECTOR_PRIVATE_H


#include <gtk/gtkvbox.h>
#include <libgnome/gnome-defs.h>
#include "gnome-selector.h"
#include "gnome-entry.h"

BEGIN_GNOME_DECLS

struct _GnomeSelectorPrivate {
	GtkWidget   *gentry;
	GtkWidget   *entry;

	gchar       *dialog_title;

	GSList      *dir_list;
	GSList      *file_list;
	GSList      *total_list;

	GtkWidget   *selector_widget;
	GtkWidget   *browse_dialog;

	GtkWidget   *box;
	GtkWidget   *hbox;
	GtkWidget   *browse_button;
	GtkWidget   *clear_button;

	guint32      flags;

	guint32      changed : 1;
	guint32      need_rebuild : 1;
	guint32      dirty : 1;

	guint        frozen;
};
	
END_GNOME_DECLS

#endif
