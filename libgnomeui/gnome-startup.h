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

/* DEPRECATED */
/* gnome-startup.h - Functions for handling one-time startups in sessions.
   Written by Tom Tromey <tromey@cygnus.com>.  */

#ifndef GNOME_STARTUP_H
#define GNOME_STARTUP_H

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

/* This function is used by configurator programs that set some global
   X server state.  The general idea is that such a program can be run
   in an `initialization' mode, where it sets the server state
   according to some saved state.  This function implements a mutex
   for such programs.  The mutex a property on the root window.  Call
   this function with the session manager's session id.  If it returns
   false, then do nothing.  If it returns true, then the caller has
   obtained the mutex and should proceed with initialization.  The
   property name is generally of the form GNOME_<PROGRAM>_PROPERTY.  */

gboolean gnome_startup_acquire_token (const gchar *property_name,
				      const gchar *sm_id);

END_GNOME_DECLS

#endif /* GNOME_STARTUP_H */
