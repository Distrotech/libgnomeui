/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
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

#ifndef LIBGNOMEUI_H
#define LIBGNOMEUI_H

#include <libgnomebase/gnome-defs.h>
#include <libgnomeui/gnome-uidefs.h>
#include <libgnomeui/gnome-about.h>
#include <libgnomeui/gnome-animator.h>
#include <libgnomeui/gnome-app.h>
#include <libgnomeui/gnome-appbar.h>
#include <libgnomeui/gnome-app-helper.h>
#include <libgnomecanvas/gnome-canvas.h>
#include <libgnomecanvas/gnome-canvas-pixbuf.h>
#include <libgnomecanvas/gnome-canvas-line.h>
#include <libgnomecanvas/gnome-canvas-rect-ellipse.h>
#include <libgnomecanvas/gnome-canvas-polygon.h>
#include <libgnomecanvas/gnome-canvas-text.h>
#include <libgnomecanvas/gnome-canvas-util.h>
#include <libgnomecanvas/gnome-canvas-widget.h>
#include <libgnomeui/gnome-color-picker.h>
#include <libgnomeui/gnome-cursors.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-dialog-util.h>
#include <libgnomeui/gnome-dock.h>
#include <libgnomeui/gnome-dock-band.h>
#include <libgnomeui/gnome-dock-item.h>
#include <libgnomeui/gnome-entry.h>
#include <libgnomeui/gnome-selector.h>
#include <libgnomeui/gnome-file-selector.h>
#include <libgnomeui/gnome-font-picker.h>
#include <libgnomeui/gnome-gconf.h>
#include <libgnomeui/gnome-geometry.h>
#include <libgnomeui/gnome-helpsys.h>

#include <libgnomeui/gnome-icon-list.h>
#include <libgnomeui/gnome-icon-item.h>
#include <libgnomeui/gnome-icon-selector.h>
#include <libgnomeui/gnome-icon-entry.h>
#include <libgnomeui/gnome-canvas-init.h>
#include <libgnomeui/gnome-init.h>
#include <libgnomeui/gnome-less.h>
#include <libgnomeui/gnome-macros.h>
#include <libgnomeui/gnome-messagebox.h>
/* Considering moving this to gnome-print, so this file 
   is deprecated but the API isn't */
#include <libgnomeui/gnome-paper-selector.h>
#include <libgnomeui/gnome-unit-spinner.h>
#include <libgnomeui/gnome-popup-menu.h>
#include <libgnomeui/gnome-popup-help.h>
#include <libgnomeui/gnome-pixmap.h>
#include <libgnomeui/gnome-propertybox.h>
#include <libgnomeui/gnome-scores.h>
#include <libgnomeui/gnome-types.h>
#include <libgnomeui/gnome-stock.h>
#include <libgnomeui/gnome-client.h>
#include <libgnomeui/gtk-clock.h>
#include <libgnomeui/gtkdial.h>
#include <libgnomeui/gtkpixmapmenuitem.h>
#include <libgnomeui/gnome-dateedit.h>
#include <libgnomeui/gnome-mdi.h>
#include <libgnomeui/gnome-mdi-child.h>
#include <libgnomeui/gnome-mdi-generic-child.h>
#include <libgnomeui/gnome-mdi-session.h>
#include <libgnomeui/gnometypebuiltins.h>
#include <libgnomeui/gnome-winhints.h>
#include <libgnomeui/gnome-href.h>
#include <libgnomeui/gnome-druid.h>
#include <libgnomeui/gnome-druid-page.h>
#include <libgnomeui/gnome-druid-page-edge.h>
#include <libgnomeui/gnome-druid-page-standard.h>
#include <libgnomeui/oafgnome.h>
#include <libgnomeui/gnome-textfu.h>
#include <libgnomeui/gnome-vfs-util.h>

#ifdef COMPAT_1_0
#include "compat/1.0/libgnomeui-compat-1.0.h"
#endif

#endif
