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

/* gnome-ice.h - Interface between ICE and Gtk.
   Written by Tom Tromey <tromey@cygnus.com>.  */

#ifndef GNOME_ICE_H
#define GNOME_ICE_H



G_BEGIN_DECLS

/* This function should be called before any ICE functions are used.
   It will arrange for ICE connections to be read and dispatched via
   the Gtk event loop.  This function can be called any number of
   times without harm.  */
void gnome_ice_init (void);

G_END_DECLS

#endif /* GNOME_ICE_H */
