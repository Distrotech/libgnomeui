#ifndef GNOME_PREFERENCES_H
#define GNOME_PREFERENCES_H
/****
  Gnome preferences settings

  These should be used only in libgnomeui and Gnome configuration
  utilities. Don't worry about them otherwise, the library should
  handle it (if not, fix the library, not your app).
  ****/

#include <libgnome/gnome-defs.h>
#include <gtk/gtkbbox.h>

BEGIN_GNOME_DECLS

/* Global config choices. App-specific choices are handled in GnomeApp. */

/* Load and sync the config file. */
void gnome_preferences_load(void);
void gnome_preferences_save(void);

GtkButtonBoxStyle gnome_preferences_get_button_layout (void);
void              gnome_preferences_set_button_layout (GtkButtonBoxStyle style);

END_GNOME_DECLS

#endif
