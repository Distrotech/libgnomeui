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

/* gnome-startup.c - Functions for handling one-time startups in sessions.
   Written by Tom Tromey <tromey@cygnus.com>.  */

#include <config.h>

#include "libgnome/libgnomeP.h"
#include "gnome-startup.h"

#include <string.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

gboolean
gnome_startup_acquire_token (const char *property,
			     const char *session_id)
{
  Atom atom, actual;
  unsigned long nitems, nbytes;
  unsigned char *current;
  int len, format;
  gboolean result;

  atom = XInternAtom (GDK_DISPLAY (), property, False);
  len = strlen (session_id);

  /* Append our session id to the property.  We do this to avoid a
     race condition: if two clients run this code, we want to make
     sure that only one client can acquire the lock.  */
  XChangeProperty (GDK_DISPLAY (), DefaultRootWindow (GDK_DISPLAY ()), atom,
		   XA_STRING, 8, PropModeAppend, session_id, len);

  if (XGetWindowProperty (GDK_DISPLAY (), DefaultRootWindow (GDK_DISPLAY ()),
			  atom, 0, len, False, XA_STRING,
			  &actual, &format,
			  &nitems, &nbytes, &current) != Success)
    current = NULL;

  if (! current)
    return 0;

  result = ! strncmp (current, session_id, len);
  XFree (current);

  return result;
}
