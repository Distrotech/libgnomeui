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

/* Global config choices. App-specific choices are handles in GnomeApp. */
typedef struct _GnomePreferences GnomePreferences;

struct _GnomePreferences {
  GtkButtonBoxStyle dialog_buttons_style;
  int property_box_buttons_ok : 1;
  int property_box_buttons_apply : 1;
  int property_box_buttons_close : 1;
  int property_box_buttons_help : 1;
};


/* Don't use this unless you know what you're doing. */
extern GnomePreferences gnome_preferences_global;

void gnome_preferences_load(void);
void gnome_preferences_save(void);

END_GNOME_DECLS

#endif




