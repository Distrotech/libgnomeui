/* gnome-ice.h - Interface between ICE and Gtk.
   Written by Tom Tromey <tromey@cygnus.com>.  */

#ifndef GNOME_ICE_H
#define GNOME_ICE_H

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

/* This function should be called before any ICE functions are used.
   It will arrange for ICE connections to be read and dispatched via
   the Gtk event loop.  This function can be called any number of
   times without harm.  */
void gnome_ice_init (void);

END_GNOME_DECLS

#endif /* GNOME_ICE_H */
