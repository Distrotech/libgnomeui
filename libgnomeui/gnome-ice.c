/* gnome-ice.c - Interface between ICE and Gtk.
   Written by Tom Tromey <tromey@cygnus.com>.  */

#include <config.h>

#ifdef HAVE_LIBSM
#include <X11/ICE/ICElib.h>
#endif /* HAVE_LIBSM */

#include "gnome.h"
#include "gnome-ice.h"

/* True if we've started listening to ICE.  */
static int ice_init = 0;

/* ICE connection tag as known by GDK event loop.  */
static guint ice_tag;

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
  if (opening)
    ice_tag = gdk_input_add (IceConnectionNumber (connection),
			     GDK_INPUT_READ, process_ice_messages,
			     (gpointer) connection);
  else
    gdk_input_remove (ice_tag);
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
