#ifndef GNOME_PREFERENCES_H
#define GNOME_PREFERENCES_H
/****
  Gnome preferences settings

  These should be used only in libgnomeui and Gnome configuration
  utilities. Don't worry about them otherwise, the library should
  handle it (if not, fix the library, not your app).
  ****/

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

/* Used for global defaults. */
#define GNOME_UI_APPNAME "/_Gnome_UI_Prefs"


/* ============= Sections ====================== */
#define GNOME_GENERAL   "/_Gnome_UI_General"

/* ============== Status Bar =================== */

#define GNOME_STATUS_BAR_KEY "/StatusBarUsage"

/* Three possible values; strings improve human-readability of config
   file. Should only be parsed once per startup, so no big deal. */

#define  GNOME_STATUS_BAR_NONE "None" 
#define  GNOME_STATUS_BAR_MIN  "Minimum"  /* only non-interactive messages */
#define  GNOME_STATUS_BAR_INTERACTIVE "Interactive"

/* ============ Property Box ======================= */

/* ignore this */
#define _GNOME_PROPERTY_BOX_BUTTONS "/PropertyBoxButtons"

/* Each of these are bools; better way? */
#define GNOME_PROPERTY_BOX_BUTTONS_OK_KEY _GNOME_PROPERTY_BOX_BUTTONS"OK"
#define GNOME_PROPERTY_BOX_BUTTONS_APPLY_KEY _GNOME_PROPERTY_BOX_BUTTONS"Apply"
#define GNOME_PROPERTY_BOX_BUTTONS_CLOSE_KEY _GNOME_PROPERTY_BOX_BUTTONS"Close"
#define GNOME_PROPERTY_BOX_BUTTONS_HELP_KEY _GNOME_PROPERTY_BOX_BUTTONS"Help"

/* ==================== GnomeDialog ===================== */

/* Use positions from GnomeApp; that stuff needs to be broken out of 
   GnomeApp */



/* Stuff set on an app-by-app basis. */
typedef _GnomeAppPreferences GnomeAppPreferences;

struct {
  const gchar * status_bar_usage;
} _GnomeAppPreferences;

/* Stuff set globally */
typedef _GnomeUIPreferences GnomeUIPreferences;

struct {
  const gchar * dialog_buttons_position;
  int property_box_buttons_ok : 1;
  int property_box_buttons_apply : 1;
  int property_box_buttons_close : 1;
  int property_box_buttons_help : 1;
} _GnomeUIPreferences;

END_GNOME_DECLS

#endif




