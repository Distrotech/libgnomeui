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


typedef struct _GnomeIceInternal
{
  gint input_id;
} GnomeIceInternal;

static GnomeIceInternal*
malloc_gnome_ice_internal ()
{
  GnomeIceInternal *gnome_ice_internal = g_malloc (sizeof (GnomeIceInternal));
  gnome_ice_internal->input_id = 0;
  return gnome_ice_internal;
}

static void
free_gnome_ice_internal (GnomeIceInternal *gnome_ice_internal)
{
  g_assert (gnome_ice_internal != NULL);
  g_free (gnome_ice_internal);
}

/* map IceConn pointers to GnomeIceInternal pointers */
static GHashTable *__gnome_ice_internal_hash = NULL;


#ifdef HAVE_LIBSM

static void new_ice_connection (IceConn connection, IcePointer client_data, Bool opening, IcePointer *watch_data);

/* This is called when data is available on an ICE connection.  */
static gboolean
process_ice_messages (gpointer client_data, gint source,
		      GdkInputCondition condition)
{
  IceProcessMessagesStatus status;
  IceConn connection = (IceConn) client_data;

  if(condition & GDK_INPUT_EXCEPTION) {
    new_ice_connection(connection, NULL, FALSE, NULL);
    return FALSE;
  }

  status = IceProcessMessages (connection, NULL, NULL);

  /* FIXME: handle case when status==closed.  */

  return TRUE;
}

/* This is called when a new ICE connection is made.  It arranges for
   the ICE connection to be handled via the event loop.  */
static void
new_ice_connection (IceConn connection, IcePointer client_data, Bool opening,
		    IcePointer *watch_data)
{
  GnomeIceInternal *gnome_ice_internal;

  if (opening)
    {
      gnome_ice_internal = g_hash_table_lookup (__gnome_ice_internal_hash, connection);
      if (gnome_ice_internal)
	{
	  gdk_input_remove (gnome_ice_internal->input_id);
	  gnome_ice_internal->input_id = 0;
	}
      else
	{
	  gnome_ice_internal = malloc_gnome_ice_internal ();
	  g_hash_table_insert (__gnome_ice_internal_hash, connection, gnome_ice_internal);
	}

      /* Make sure we don't pass on these file descriptors to any
         exec'ed children */
       fcntl(IceConnectionNumber(connection),F_SETFD,
	     fcntl(IceConnectionNumber(connection),F_GETFD,0) | FD_CLOEXEC);
       gnome_ice_internal->input_id = 
	 gdk_input_add (IceConnectionNumber (connection),
			GDK_INPUT_READ|GDK_INPUT_EXCEPTION,
			(GdkInputFunction)process_ice_messages,
			(gpointer) connection);
    }
  else 
    {
      gnome_ice_internal = g_hash_table_lookup (__gnome_ice_internal_hash, connection);
      if (! gnome_ice_internal)
	{
	  return;
	}

      gdk_input_remove (gnome_ice_internal->input_id);
      g_hash_table_remove (__gnome_ice_internal_hash, connection);
      free_gnome_ice_internal (gnome_ice_internal);
    }
}

#endif /* HAVE_LIBSM */

void
gnome_ice_init (void)
{
  static gboolean ice_init = FALSE;

  if (! ice_init)
    {
#ifdef HAVE_LIBSM
      IceAddConnectionWatch (new_ice_connection, NULL);

      /* map IceConn pointers to GnomeIceInternal pointers */
      __gnome_ice_internal_hash = g_hash_table_new (g_direct_hash, NULL);
#endif /* HAVE_LIBSM */

      ice_init = TRUE;
    }
}
