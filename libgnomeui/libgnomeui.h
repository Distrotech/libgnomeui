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

/* XXX: hmm this should be defined by the app I guess */
#define GNOME_EXCLUDE_DEPRECATED 1

#include "libgnome/gnome-defs.h"
#include "libgnomeui/gnome-uidefs.h"
#include "libgnomeui/gnome-about.h"
#include "libgnomeui/gnome-animator.h"
#include "libgnomeui/gnome-app.h"
#include "libgnomeui/gnome-appbar.h"
#include "libgnomeui/gnome-app-helper.h"
#ifndef GNOME_EXCLUDE_EXPERIMENTAL
#include "libgnomeui/gnome-app-util.h"
#endif
#include "libgnomeui/gnome-canvas.h"
#include "libgnomeui/gnome-canvas-pixbuf.h"
#include "libgnomeui/gnome-canvas-line.h"
#include "libgnomeui/gnome-canvas-rect-ellipse.h"
#include "libgnomeui/gnome-canvas-polygon.h"
#include "libgnomeui/gnome-canvas-text.h"
#include "libgnomeui/gnome-canvas-util.h"
#include "libgnomeui/gnome-canvas-widget.h"
#include "libgnomeui/gnome-color-picker.h"
#include "libgnomeui/gnome-cursors.h"
#include "libgnomeui/gnome-dialog.h"
#include "libgnomeui/gnome-dialog-util.h"
/* There are nice third-party libs for this; 
   but maybe we should leave it. Let's assume it's
   up in the air, but people who aren't cutting 
   edge probably don't want to fool with it */
#ifndef GNOME_EXCLUDE_DEPRECATED
#include "libgnomeui/gnome-dns.h"
#endif
#include "libgnomeui/gnome-dock.h"
#include "libgnomeui/gnome-dock-band.h"
#include "libgnomeui/gnome-dock-item.h"
#include "libgnomeui/gnome-entry.h"
#include "libgnomeui/gnome-file-entry.h"
#include "libgnomeui/gnome-font-picker.h"
#include "libgnomeui/gnome-gconf.h"
#include "libgnomeui/gnome-geometry.h"
#include "libgnomeui/gnome-helpsys.h"

#include "libgnomeui/gnome-icon-list.h"
#include "libgnomeui/gnome-icon-item.h"
#include "libgnomeui/gnome-icon-sel.h"
#include "libgnomeui/gnome-icon-entry.h"
#include "libgnomeui/gnome-init.h"
#include "libgnomeui/gnome-less.h"
#include "libgnomeui/gnome-messagebox.h"
#include "libgnomeui/gnome-number-entry.h"
/* Considering moving this to gnome-print, so this file 
   is deprecated but the API isn't */
#include "libgnomeui/gnome-paper-selector.h"
#include "libgnomeui/gnome-unit-spinner.h"
#include "libgnomeui/gnome-popup-menu.h"
#include "libgnomeui/gnome-popup-help.h"
#include "libgnomeui/gnome-pixmap.h"
#include "libgnomeui/gnome-pixmap-entry.h"
#ifndef GNOME_EXCLUDE_DEPRECATED
#include "libgnomeui/gnome-preferences.h"
#endif
#include "libgnomeui/gnome-propertybox.h"
#include "libgnomeui/gnome-scores.h"
#include "libgnomeui/gnome-types.h"
#include "libgnomeui/gnome-stock.h"
#include "libgnomeui/gnome-client.h"
#include "libgnomeui/gtk-clock.h"
#include "libgnomeui/gtkdial.h"
#include "libgnomeui/gtkpixmapmenuitem.h"
#include "libgnomeui/gnome-dateedit.h"
#include "libgnomeui/gnome-calculator.h"
#include "libgnomeui/gnome-mdi.h"
#include "libgnomeui/gnome-mdi-child.h"
#include "libgnomeui/gnome-mdi-generic-child.h"
#include "libgnomeui/gnome-mdi-session.h"
#include "libgnomeui/gnometypebuiltins.h"
#include "libgnomeui/gnome-winhints.h"
#include "libgnomeui/gnome-href.h"
#ifndef GNOME_EXCLUDE_DEPRECATED
#include "libgnomeui/gnome-procbar.h"
#endif
#include "libgnomeui/gnome-druid.h"
#include "libgnomeui/gnome-druid-page.h"
#include "libgnomeui/gnome-druid-page-edge.h"
#include "libgnomeui/gnome-druid-page-standard.h"
#include "libgnomeui/oafgnome.h"
#include "libgnomeui/gnome-textfu.h"

#define GNOMEUI_INIT LIBGNOMEUI_INIT,GNOME_CLIENT_INIT,GNOME_GCONF_INIT

#ifdef COMPAT_1_0
#include "compat/1.0/libgnomeui-compat-1.0.h"
#endif

#endif
