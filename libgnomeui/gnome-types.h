#ifndef GNOME_TYPES_H
#define GNOME_TYPES_H
/****
  Gnome-wide useful types.
  ****/

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

/* string is a g_malloc'd string which should be freed, or NULL if the
   user cancelled. */
typedef void (* GnomeStringCallback)(gchar * string, gpointer data); 

/* See gnome-uidefs for the Yes/No Ok/Cancel defines which can be
   "reply" */
typedef void (* GnomeReplyCallback)(gint reply, gpointer data);


END_GNOME_DECLS

#endif
