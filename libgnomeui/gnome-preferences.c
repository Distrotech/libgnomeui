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

/* 
 * Global variable holds current preferences.  
 */

GnomePreferences gnome_preferences_global =
{
  GTK_BUTTONBOX_END,  /* Position of dialog buttons. */
  TRUE,               /* PropertyBox has OK button */
  TRUE,               /* PropertyBox has Apply */
  TRUE,               /* PropertyBox has Close */
  TRUE                /* PropertyBox has Help */
};

/* Tons of defines for where to store the preferences. */

/* Used for global defaults. */
#define UI_APPNAME "/Gnome/UI_Prefs"


/* ============= Sections ====================== */
#define GENERAL   "/Gnome/UI_General/"
#define DIALOGS   "/Gnome/UI_Dialogs/"

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

/* ============ Property Box ======================= */

/* ignore this */
#define _PROPERTY_BOX_BUTTONS "PropertyBoxButton"

/* Each of these are bools; better way? */
#define PROPERTY_BOX_BUTTONS_OK_KEY _PROPERTY_BOX_BUTTONS"OK"
#define PROPERTY_BOX_BUTTONS_APPLY_KEY _PROPERTY_BOX_BUTTONS"Apply"
#define PROPERTY_BOX_BUTTONS_CLOSE_KEY _PROPERTY_BOX_BUTTONS"Close"
#define PROPERTY_BOX_BUTTONS_HELP_KEY _PROPERTY_BOX_BUTTONS"Help"


void gnome_preferences_load(void)
{
  gboolean b;
  gint i;
  gchar * s;

  gnome_config_push_prefix(DIALOGS);

  s = gnome_config_get_string(DIALOG_BUTTONS_STYLE_KEY);

  if ( s == NULL ) {
    ; /* Leave the default initialization, nothing found. */
  }
  else {
    i = 0;
    while ( i < NUM_BUTTON_STYLES ) {
      if ( strcasecmp(s, dialog_button_styles[i]) == 0 ) {
	gnome_preferences_global.dialog_buttons_style = i;
	break;
      } 
      ++i;
    }
    if ( i == NUM_BUTTON_STYLES ) {
      /* We got all the way to the end without finding one */
      g_warning("Didn't recognize buttonbox style in libgnomeui config\n");
    }
  }
  g_free(s); 
  s = NULL;
  
  /* Fixme. There's a little problem with no error value from the 
     bool get function. This makes it yucky to do the propertybox 
     thing. The intermediate 'b' variable is only in anticipation
     of a future way to check for errors. */

  b = gnome_config_get_bool_with_default(PROPERTY_BOX_BUTTONS_OK_KEY"=true",
					 NULL);
  gnome_preferences_global.property_box_buttons_ok = b;

  b = gnome_config_get_bool_with_default(PROPERTY_BOX_BUTTONS_APPLY_KEY"=true",
					 NULL);
  gnome_preferences_global.property_box_buttons_apply = b;

  b = gnome_config_get_bool_with_default(PROPERTY_BOX_BUTTONS_CLOSE_KEY"=true",
					 NULL);
  gnome_preferences_global.property_box_buttons_close = b;

  b = gnome_config_get_bool_with_default(PROPERTY_BOX_BUTTONS_HELP_KEY"=true",
					 NULL);
  gnome_preferences_global.property_box_buttons_help = b;

  gnome_config_pop_prefix();
}

void gnome_preferences_save(void)
{
  gnome_config_push_prefix(DIALOGS);
  
  gnome_config_set_string(DIALOG_BUTTONS_STYLE_KEY, 
			  dialog_button_styles[gnome_preferences_global.dialog_buttons_style]);

  gnome_config_pop_prefix();
  gnome_config_sync();
}
