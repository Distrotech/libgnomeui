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

/* How buttons are layed out in dialogs */
GtkButtonBoxStyle gnome_preferences_get_button_layout (void);
void              gnome_preferences_set_button_layout (GtkButtonBoxStyle style);

/* Whether to use the statusbar instead of dialogs when possible:
   TRUE means use the statusbar */
gboolean          gnome_preferences_get_statusbar_dialog     (void);
void              gnome_preferences_set_statusbar_dialog     (gboolean statusbar);

/* Whether the statusbar can be used for interactive questions 
   TRUE means the statusbar is interactive */
gboolean          gnome_preferences_get_statusbar_interactive(void);
void              gnome_preferences_set_statusbar_interactive(gboolean b);

/* Whether to have handleboxes on the various parts of GnomeApp */
gboolean          gnome_preferences_get_toolbar_handlebox    (void);
void              gnome_preferences_set_toolbar_handlebox    (gboolean b);
gboolean          gnome_preferences_get_menubar_handlebox    (void);
void              gnome_preferences_set_menubar_handlebox    (gboolean b);

/* Whether toolbar buttons have a beveled edge */
gboolean          gnome_preferences_get_toolbar_relief    (void);
void              gnome_preferences_set_toolbar_relief    (gboolean b);

END_GNOME_DECLS

#endif
