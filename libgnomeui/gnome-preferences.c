/* GNOME GUI Library: gnome-preferences.c
 * Copyright (C) 1998 Free Software Foundation
 * Author: Havoc Pennington
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>
#include "gnome-preferences.h"
#include "libgnome/gnome-config.h"
#include <string.h>

typedef struct _GnomePreferences GnomePreferences;

struct _GnomePreferences {
  GtkButtonBoxStyle dialog_buttons_style;
  int property_box_buttons_ok : 1;
  int property_box_buttons_apply : 1;
  int property_box_buttons_close : 1;
  int property_box_buttons_help : 1;
  int statusbar_not_dialog : 1;
  int statusbar_is_interactive : 1;
  int toolbar_handlebox : 1;
  int menubar_handlebox : 1;
  int toolbar_relief : 1;
  int dialog_centered : 1;
  GtkWindowType dialog_type;
  GtkWindowPosition dialog_position; 
};

/* 
 * Variable holds current preferences.  
 */

/* These initial values are the Gnome defaults */
static GnomePreferences prefs =
{
  GTK_BUTTONBOX_END,  /* Position of dialog buttons. */
  TRUE,               /* PropertyBox has OK button */
  TRUE,               /* PropertyBox has Apply */
  TRUE,               /* PropertyBox has Close */
  TRUE,               /* PropertyBox has Help */
  FALSE,              /* Use dialogs, not the statusbar */
  FALSE,              /* Statusbar isn't interactive */
  TRUE,               /* Toolbar has handlebox */
  TRUE,               /* Menubar has handlebox */
  TRUE,               /* Toolbar buttons are relieved */
  TRUE,               /* Center dialogs over apps when possible */
  GTK_WINDOW_DIALOG,  /* Dialogs are treated specially */
  GTK_WIN_POS_MOUSE   /* Put dialogs at mouse pointer. */
};

/* Tons of defines for where to store the preferences. */

/* Used for global defaults. (well, maybe someday) */
#define UI_APPNAME "/Gnome/UI_Prefs"

/* ============= Sections ====================== */
#define GENERAL   "/Gnome/UI_General/"
#define DIALOGS   "/Gnome/UI_Dialogs/"
#define STATUSBAR "/Gnome/UI_StatusBar/"
#define APP       "/Gnome/UI_GnomeApp/"

/* ==================== GnomeDialog ===================== */

#define DIALOG_BUTTONS_STYLE_KEY "DialogButtonsStyle"

static const gchar * const dialog_button_styles [] = {
  "Default",
  "Spread",
  "Edge",
  "Start",
  "End"
};

#define NUM_BUTTON_STYLES 5

#define DIALOG_CENTERED_KEY "Dialog_is_Centered"

#define DIALOG_TYPE_KEY "DialogType"

static const gchar * const dialog_types [] = {
  "Toplevel",
  "Dialog"
};

#define NUM_DIALOG_TYPES 2

#define DIALOG_POSITION_KEY "DialogPosition"

static const gchar * const dialog_positions [] = {
  "None",
  "Center",
  "Mouse"
};

#define NUM_DIALOG_POSITIONS 3

/* ============ Property Box ======================= */

/* ignore this */
#define _PROPERTY_BOX_BUTTONS "PropertyBoxButton"

/* Each of these are bools; better way? */
#define PROPERTY_BOX_BUTTONS_OK_KEY _PROPERTY_BOX_BUTTONS"OK"
#define PROPERTY_BOX_BUTTONS_APPLY_KEY _PROPERTY_BOX_BUTTONS"Apply"
#define PROPERTY_BOX_BUTTONS_CLOSE_KEY _PROPERTY_BOX_BUTTONS"Close"
#define PROPERTY_BOX_BUTTONS_HELP_KEY _PROPERTY_BOX_BUTTONS"Help"

/* =========== GnomeApp ============================ */

#define STATUSBAR_DIALOG_KEY       "StatusBar_not_Dialog"
#define STATUSBAR_INTERACTIVE_KEY  "StatusBar_is_Interactive"

#define TOOLBAR_HANDLEBOX_KEY      "Toolbar_has_Handlebox"
#define MENUBAR_HANDLEBOX_KEY      "Menubar_has_Handlebox"

#define TOOLBAR_RELIEF_KEY         "Toolbar_relieved_buttons"

static gboolean 
enum_from_strings(gint * setme, gchar * findme, 
		  const gchar * const array[], gint N)
{
  gboolean retval = FALSE;
  gint i = 0;
  if ( findme == NULL ) {
    /* Leave default */
    retval = TRUE;
  }
  else {
    while ( i < N ) {
      if ( g_strcasecmp(findme, array[i]) == 0 ) {
	*setme = i;
	retval = TRUE;
	break;
      } 
      ++i;
    }
  }

  g_free(findme);
  return retval;
}

void gnome_preferences_load(void)
{
  /* Probably this function should be rewritten to use the 
     _preferences_get functions */
  gboolean b;
  gchar * s;

  gnome_config_push_prefix(DIALOGS);

  s = gnome_config_get_string(DIALOG_BUTTONS_STYLE_KEY);


  if ( ! enum_from_strings((int*) &prefs.dialog_buttons_style, s,
			   dialog_button_styles, NUM_BUTTON_STYLES) ) {
    g_warning("Didn't recognize buttonbox style in libgnomeui config");
  }
  
  s = gnome_config_get_string(DIALOG_TYPE_KEY);

  if ( ! enum_from_strings((int*) &prefs.dialog_type, s,
			   dialog_types, NUM_DIALOG_TYPES) ) {
    g_warning("Didn't recognize dialog type in libgnomeui config");
  }

  s = gnome_config_get_string(DIALOG_POSITION_KEY);

  if ( ! enum_from_strings((int*) &prefs.dialog_position, s,
			   dialog_positions, NUM_DIALOG_POSITIONS) ) {
    g_warning("Didn't recognize dialog position in libgnomeui config");
  }  

  /* Fixme. There's a little problem with no error value from the 
     bool get function. This makes it yucky to do the propertybox 
     thing. The intermediate 'b' variable is only in anticipation
     of a future way to check for errors. */

  b = gnome_config_get_bool_with_default(DIALOG_CENTERED_KEY"=true",
					 NULL);
  prefs.dialog_centered = b;

#if 0
  /* This is unused for now */
  b = gnome_config_get_bool_with_default(PROPERTY_BOX_BUTTONS_OK_KEY"=true",
					 NULL);
  prefs.property_box_buttons_ok = b;

  b = gnome_config_get_bool_with_default(PROPERTY_BOX_BUTTONS_APPLY_KEY"=true",
					 NULL);
  prefs.property_box_buttons_apply = b;

  b = gnome_config_get_bool_with_default(PROPERTY_BOX_BUTTONS_CLOSE_KEY"=true",
					 NULL);
  prefs.property_box_buttons_close = b;

  b = gnome_config_get_bool_with_default(PROPERTY_BOX_BUTTONS_HELP_KEY"=true",
					 NULL);
  prefs.property_box_buttons_help = b;
#endif

  gnome_config_pop_prefix();
  gnome_config_push_prefix(STATUSBAR);

  b = gnome_config_get_bool_with_default(STATUSBAR_DIALOG_KEY"=false",
					 NULL);
  prefs.statusbar_not_dialog = b;
  
  b = gnome_config_get_bool_with_default(STATUSBAR_INTERACTIVE_KEY"=false",
					 NULL);
  prefs.statusbar_is_interactive = b;

  gnome_config_pop_prefix();
  gnome_config_push_prefix(APP);

  b = gnome_config_get_bool_with_default(TOOLBAR_HANDLEBOX_KEY"=true",
					 NULL);
  
  prefs.toolbar_handlebox = b;

  b = gnome_config_get_bool_with_default(MENUBAR_HANDLEBOX_KEY"=true",
					 NULL);
  
  prefs.menubar_handlebox = b;

  b = gnome_config_get_bool_with_default(TOOLBAR_RELIEF_KEY"=true",
					 NULL);
  prefs.toolbar_relief = b;

  gnome_config_pop_prefix();
}

void gnome_preferences_save(void)
{
  gnome_config_push_prefix(DIALOGS);
  
  gnome_config_set_string(DIALOG_BUTTONS_STYLE_KEY, 
			  dialog_button_styles[prefs.dialog_buttons_style]);

  gnome_config_set_string(DIALOG_TYPE_KEY, 
			  dialog_types[prefs.dialog_type]);

  gnome_config_set_string(DIALOG_POSITION_KEY, 
			  dialog_positions[prefs.dialog_position]);
  
  gnome_config_set_bool  (DIALOG_CENTERED_KEY,
			  prefs.dialog_centered);

  gnome_config_pop_prefix();

  gnome_config_push_prefix(STATUSBAR);

  gnome_config_set_bool(STATUSBAR_DIALOG_KEY,
			prefs.statusbar_not_dialog);
  gnome_config_set_bool(STATUSBAR_INTERACTIVE_KEY,
			prefs.statusbar_is_interactive);


  gnome_config_pop_prefix();
  gnome_config_push_prefix(APP);

  gnome_config_set_bool(TOOLBAR_HANDLEBOX_KEY,
			prefs.toolbar_handlebox);
  gnome_config_set_bool(MENUBAR_HANDLEBOX_KEY,
			prefs.menubar_handlebox);
  gnome_config_set_bool(TOOLBAR_RELIEF_KEY,
			prefs.toolbar_relief);

  gnome_config_pop_prefix();
  gnome_config_sync();
}

GtkButtonBoxStyle gnome_preferences_get_button_layout (void)
{
  return prefs.dialog_buttons_style;
}

void              gnome_preferences_set_button_layout (GtkButtonBoxStyle style)
{
  g_return_if_fail( (style >= 0) && (style <= GTK_BUTTONBOX_END) );
  prefs.dialog_buttons_style = style;
}

gboolean          gnome_preferences_get_statusbar_dialog     (void)
{
  return prefs.statusbar_not_dialog;
}

void              gnome_preferences_set_statusbar_dialog     (gboolean b)
{
  prefs.statusbar_not_dialog = b;
}

gboolean          gnome_preferences_get_statusbar_interactive(void)
{
  return prefs.statusbar_is_interactive;
}

void              gnome_preferences_set_statusbar_interactive(gboolean b)
{
  prefs.statusbar_is_interactive = b;
}

gboolean          gnome_preferences_get_toolbar_handlebox    (void)
{
  return prefs.toolbar_handlebox;
}

void              gnome_preferences_set_toolbar_handlebox    (gboolean b)
{
  prefs.toolbar_handlebox = b;
}

gboolean          gnome_preferences_get_menubar_handlebox    (void)
{
  return prefs.menubar_handlebox;
}

void              gnome_preferences_set_menubar_handlebox    (gboolean b)
{
  prefs.menubar_handlebox = b;
}

gboolean          gnome_preferences_get_toolbar_relief    (void)
{
  return prefs.toolbar_relief;
}

void              gnome_preferences_set_toolbar_relief    (gboolean b)
{
  prefs.toolbar_relief = b;
}

gboolean          gnome_preferences_get_dialog_centered      ()
{
  return prefs.dialog_centered;
}

void              gnome_preferences_set_dialog_centered      (gboolean b)
{
  prefs.dialog_centered = b;
}

GtkWindowType     gnome_preferences_get_dialog_type          ()
{
  return prefs.dialog_type;
}

void              gnome_preferences_set_dialog_type          (GtkWindowType t)
{
  prefs.dialog_type = t;
}

/* Whether dialogs are GTK_WIN_POS_NONE, GTK_WIN_POS_CENTER,
   GTK_WIN_POS_MOUSE */
GtkWindowPosition gnome_preferences_get_dialog_position      ()
{
  return prefs.dialog_position;
}

void              gnome_preferences_set_dialog_position      (GtkWindowPosition p)
{
  prefs.dialog_position = p;
}

