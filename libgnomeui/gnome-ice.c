/* gnome-ice.c - Interface between ICE and Gtk.
   Written by Tom Tromey <tromey@cygnus.com>.  */

#include <config.h>

#ifdef HAVE_LIBSM
#include <X11/ICE/ICElib.h>
#endif /* HAVE_LIBSM */

#include <gtk/gtk.h>
#include "gnome-ice.h"

/* True if we've started listening to ICE.  */
static int ice_init = 0;

#ifdef HAVE_LIBSM

/* This is called when data is available on an ICE connection.  */
static void
process_ice_messages (gpointer client_data, gint source,
		      GdkInputCondition condition)
{
  IceProcessMessagesStatus status;
  IceConn connection = (IceConn) client_data;

  status = IceProcessMessages (connection, NULL, NULL);
  /* FIXME: handle case when status==closed.  */
}

/* This is called when a new ICE connection is made.  It arranges for
   the ICE connection to be handled via the event loop.  */
static void
new_ice_connection (IceConn connection, IcePointer client_data, Bool opening,
		    IcePointer *watch_data)
{
  gint tag;

  /* It turns out to be easier for us to always call gdk_input_add,
     even if we are trying to remove the input.  The reason is that
     this is simpler than keeping a hash table mapping connections
     onto gdk tags.  This is losing.  */
  tag = gdk_input_add (IceConnectionNumber (connection),
		       GDK_INPUT_READ, process_ice_messages,
		       (gpointer) connection);
  if (! opening)
    gdk_input_remove (tag);
}

#endif /* HAVE_LIBSM */

void
gnome_ice_init (void)
{
  if (! ice_init)
    {
#ifdef HAVE_LIBSM
      IceAddConnectionWatch (new_ice_connection, NULL);
#endif /* HAVE_LIBSM */
      ice_init = 1;
    }
}
