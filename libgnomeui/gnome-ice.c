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

/* gnome-ice.c - Interface between ICE and Gtk.
   Written by Tom Tromey <tromey@cygnus.com>.  */

#include <config.h>

#ifdef HAVE_LIBSM
#include <X11/ICE/ICElib.h>
#endif /* HAVE_LIBSM */

#include <unistd.h>
#include <fcntl.h>

#include <gtk/gtk.h>
#include "gnome-ice.h"

#ifdef HAVE_LIBSM

static void gnome_ice_io_error_handler (IceConn connection);

static void new_ice_connection (IceConn connection, IcePointer client_data, 
				Bool opening, IcePointer *watch_data);

/* This is called when data is available on an ICE connection.  */
static gboolean
process_ice_messages (gpointer client_data, gint source,
		      GdkInputCondition condition)
{
  IceConn connection = (IceConn) client_data;
  IceProcessMessagesStatus status;

  status = IceProcessMessages (connection, NULL, NULL);

  if (status == IceProcessMessagesIOError)
    {
      IcePointer context = IceGetConnectionContext (connection);

      if (context && GTK_IS_OBJECT (context))
	{
	  guint disconnect_id = gtk_signal_lookup ("disconnect", 
						   GTK_OBJECT_TYPE (context));

	  if (disconnect_id > 0)
	    gtk_signal_emit (GTK_OBJECT (context), disconnect_id);
	}
      else
	{
	  IceSetShutdownNegotiation (connection, False);
	  IceCloseConnection (connection);
	}
    }

  return TRUE;
}

/* This is called when a new ICE connection is made.  It arranges for
   the ICE connection to be handled via the event loop.  */
static void
new_ice_connection (IceConn connection, IcePointer client_data, Bool opening,
		    IcePointer *watch_data)
{
  gint input_id;

  if (opening)
    {
      /* Make sure we don't pass on these file descriptors to any
         exec'ed children */
      fcntl(IceConnectionNumber(connection),F_SETFD,
	    fcntl(IceConnectionNumber(connection),F_GETFD,0) | FD_CLOEXEC);

      input_id = gdk_input_add (IceConnectionNumber (connection),
				GDK_INPUT_READ|GDK_INPUT_EXCEPTION,
				(GdkInputFunction)process_ice_messages,
				(gpointer) connection);

      *watch_data = (IcePointer) GINT_TO_POINTER (input_id);
    }
  else 
    {
      input_id = GPOINTER_TO_INT ((gpointer) *watch_data);

      gdk_input_remove (input_id);
    }
}

static IceIOErrorHandler gnome_ice_installed_handler;

/* We call any handler installed before (or after) gnome_ice_init but 
   avoid calling the default libICE handler which does an exit() */
static void
gnome_ice_io_error_handler (IceConn connection)
{
    if (gnome_ice_installed_handler)
      (*gnome_ice_installed_handler) (connection);
}    

#endif /* HAVE_LIBSM */

void
gnome_ice_init (void)
{
#ifdef HAVE_LIBSM
  static gboolean ice_init = FALSE;

  if (! ice_init)
    {
      IceIOErrorHandler default_handler;

      gnome_ice_installed_handler = IceSetIOErrorHandler (NULL);
      default_handler = IceSetIOErrorHandler (gnome_ice_io_error_handler);

      if (gnome_ice_installed_handler == default_handler)
	gnome_ice_installed_handler = NULL;

      IceAddConnectionWatch (new_ice_connection, NULL);

      ice_init = TRUE;
    }
#endif /* HAVE_LIBSM */
}
