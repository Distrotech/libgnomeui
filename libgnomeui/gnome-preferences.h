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

#include "gnome-mdi.h"

BEGIN_GNOME_DECLS

/* Global config choices. App-specific choices are handled in GnomeApp. */

typedef struct _GnomePreferences GnomePreferences;

struct _GnomePreferences {
  GtkButtonBoxStyle dialog_buttons_style;
  int property_box_buttons_ok : 1;
  int property_box_buttons_apply : 1;
  int property_box_buttons_close : 1;
  int property_box_buttons_help : 1;
  int statusbar_not_dialog : 1;
  int statusbar_is_interactive : 1;
  int statusbar_meter_on_right : 1;
  int menubar_detachable : 1;
  int menubar_relief : 1;
  int toolbar_detachable : 1;
  int toolbar_relief : 1;
  int toolbar_relief_btn : 1;
  int toolbar_lines : 1;
  int toolbar_labels : 1;
  int dialog_centered : 1;
  int menus_have_tearoff : 1;
  int menus_have_icons : 1;
  int disable_imlib_cache : 1;
  GtkWindowType dialog_type;
  GtkWindowPosition dialog_position;
  GnomeMDIMode mdi_mode;
  GtkPositionType mdi_tab_pos;
};

/* Load and sync the config file. */
void gnome_preferences_load(void);
void gnome_preferences_save(void);

void gnome_preferences_load_custom(GnomePreferences *settings);
void gnome_preferences_save_custom(GnomePreferences *settings);

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

/* Whether the AppBar progress meter goes on the right or left */
gboolean          gnome_preferences_get_statusbar_meter_on_right (void);
void              gnome_preferences_set_statusbar_meter_on_right (gboolean status_meter_on_right);


/* Whether menubars can be detached */
gboolean          gnome_preferences_get_menubar_detachable   (void);
void              gnome_preferences_set_menubar_detachable   (gboolean b);

/* Whether menubars have a beveled edge */
gboolean          gnome_preferences_get_menubar_relief       (void);
void              gnome_preferences_set_menubar_relief       (gboolean b);

/* Whether toolbars can be detached */
gboolean          gnome_preferences_get_toolbar_detachable   (void);
void              gnome_preferences_set_toolbar_detachable   (gboolean b);

/* Whether toolbars have a beveled edge  */
gboolean          gnome_preferences_get_toolbar_relief       (void);
void              gnome_preferences_set_toolbar_relief       (gboolean b);

/* Whether toolbar buttons have a beveled edge */
gboolean          gnome_preferences_get_toolbar_relief_btn   (void);
void              gnome_preferences_set_toolbar_relief_btn   (gboolean b);

/* Whether toolbars show lines in separators  */
gboolean          gnome_preferences_get_toolbar_lines        (void);
void              gnome_preferences_set_toolbar_lines        (gboolean b);

/* Whether toolbars show labels  */
gboolean          gnome_preferences_get_toolbar_labels       (void);
void              gnome_preferences_set_toolbar_labels       (gboolean b);

/* Whether to try to center dialogs over their parent window.
   If it's possible, dialog_position is ignored. If not,
   fall back to dialog_position. */
gboolean          gnome_preferences_get_dialog_centered      (void);
void              gnome_preferences_set_dialog_centered      (gboolean b);

/* Whether dialogs are GTK_WINDOW_DIALOG or GTK_WINDOW_TOPLEVEL */
GtkWindowType     gnome_preferences_get_dialog_type          (void);
void              gnome_preferences_set_dialog_type          (GtkWindowType t);

/* Whether dialogs are GTK_WIN_POS_NONE, GTK_WIN_POS_CENTER,
   GTK_WIN_POS_MOUSE */
GtkWindowPosition gnome_preferences_get_dialog_position      (void);
void              gnome_preferences_set_dialog_position      (GtkWindowPosition p);

/* default MDI mode and MDI notebook tab position */
GnomeMDIMode      gnome_preferences_get_mdi_mode             (void);
void              gnome_preferences_set_mdi_mode             (GnomeMDIMode m);
GtkPositionType   gnome_preferences_get_mdi_tab_pos          (void);
void              gnome_preferences_set_mdi_tab_pos          (GtkPositionType p);

/* Whether property box has Apply button.  */
int               gnome_preferences_get_property_box_apply (void);
void              gnome_preferences_set_property_box_button_apply (int v);

/* Whether menus can be torn off */
gboolean          gnome_preferences_get_menus_have_tearoff   (void);
void              gnome_preferences_set_menus_have_tearoff   (gboolean b);

/* Whether menu items have icons in them or not */
int               gnome_preferences_get_menus_have_icons (void);
void              gnome_preferences_set_menus_have_icons (int have_icons);

/* Whether we want to disable the imlib cache or not */
int               gnome_preferences_get_disable_imlib_cache (void);
void              gnome_preferences_set_disable_imlib_cache (int disable_imlib_cache);

END_GNOME_DECLS

#endif
