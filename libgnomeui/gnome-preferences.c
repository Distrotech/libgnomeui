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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */

#include <config.h>
#include "gnome-preferences.h"
#include "libgnome/gnome-config.h"
#include <string.h>

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
  TRUE,               /* Menubars are detachable */
  TRUE,               /* Menubars are relieved */
  TRUE,               /* Toolbars are detachable */
  TRUE,               /* Toolbars are relieved */
  FALSE,              /* Toolbar buttons are relieved */
  TRUE,               /* Toolbars show lines for separators */
  TRUE,               /* Toolbars show labels */
  TRUE,               /* Center dialogs over apps when possible */
  TRUE,               /* Menus have a tearoff bar */
  TRUE,               /* Menu items have icons in them */
  TRUE,               /* Disable the Imlib cache */
  GTK_WINDOW_DIALOG,  /* Dialogs are treated specially */
  GTK_WIN_POS_CENTER, /* Put dialogs in center of screen. */
  GNOME_MDI_NOTEBOOK, /* Use notebook MDI mode. */
  GTK_POS_TOP         /* Show tabs on top of MDI notebooks. */
};

/* Tons of defines for where to store the preferences. */

/* Used for global defaults. (well, maybe someday) */
#define UI_APPNAME "/Gnome/UI_Prefs"

/* ============= Sections ====================== */
#define GENERAL   "/Gnome/UI_General/"
#define DIALOGS   "/Gnome/UI_Dialogs/"
#define STATUSBAR "/Gnome/UI_StatusBar/"
#define APP       "/Gnome/UI_GnomeApp/"
#define MDI       "/Gnome/UI_MDI/"
#define CACHE     "/Gnome/Cache/"

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
#define STATUSBAR_METER_ON_RIGHT   "StatusBar_Meter_on_Right"

#define MENUBAR_DETACHABLE_KEY     "Menubar_detachable"
#define MENUBAR_RELIEF_KEY         "Menubar_relieved"

#define TOOLBAR_DETACHABLE_KEY     "Toolbar_detachable"
#define TOOLBAR_RELIEF_KEY         "Toolbar_relieved"
#define TOOLBAR_RELIEF_BTN_KEY     "Toolbar_relieved_buttons"
#define TOOLBAR_LINES_KEY          "Toolbar_lines"
#define TOOLBAR_LABELS_KEY         "Toolbar_labels"

#define MENUS_HAVE_TEAROFF_KEY     "Menus_have_tearoff"
#define MENUS_HAVE_ICONS_KEY       "Menus_have_icons"
#define DISABLE_IMLIB_CACHE_KEY    "Disable_Imlib_cache"

/* =========== MDI ================================= */

#define MDI_MODE_KEY               "MDI_mode"
#define MDI_TAB_POS_KEY            "MDI_tab_pos"

static const gchar * const mdi_modes [] = {
  "Notebook",
  "Toplevel",
  "Modal"
};

#define NUM_MDI_MODES 3

static const gchar * const tab_positions [] = {
  "Left",
  "Right",
  "Top",
  "Bottom"
};

#define NUM_TAB_POSITIONS 4

static gboolean 
enum_from_strings(gint * setme, gchar * findme, 
		  const gchar * const array[], gint N)
{
	gboolean retval = FALSE;
	gint i = 0;

	if (findme == NULL) {
		/* Leave default */
		retval = TRUE;
	} else {
		while (i < N) {
			if (g_strcasecmp(findme, array[i]) == 0) {
				*setme = i;
				retval = TRUE;
				break;
			} 
			++i;
		}
	}
	return retval;
}

/**
 * gnome_preferences_load_custom
 * @settings: App-specified set of user preferences
 *
 * Description:
 * Uses gnome_config_xxx() interface to load a set of
 * standard GNOME preferences into the specified @settings object.
 **/
 
void
gnome_preferences_load_custom(GnomePreferences *settings)
{
  /* Probably this function should be rewritten to use the 
   *  _preferences_get functions
   */
  gboolean b;
  gchar * s;

  gnome_config_push_prefix(DIALOGS);

  s = gnome_config_get_string(DIALOG_BUTTONS_STYLE_KEY);


  if ( ! enum_from_strings((int*) &settings->dialog_buttons_style, s,
			   dialog_button_styles, NUM_BUTTON_STYLES) ) {
    g_warning("Didn't recognize buttonbox style in libgnomeui config");
  }

  g_free(s);
  
  s = gnome_config_get_string(DIALOG_TYPE_KEY);

  if ( ! enum_from_strings((int*) &settings->dialog_type, s,
			   dialog_types, NUM_DIALOG_TYPES) ) {
    g_warning("Didn't recognize dialog type in libgnomeui config");
  }

  g_free(s);
  
  s = gnome_config_get_string(DIALOG_POSITION_KEY);

  if ( ! enum_from_strings((int*) &settings->dialog_position, s,
			   dialog_positions, NUM_DIALOG_POSITIONS) ) {
    g_warning("Didn't recognize dialog position in libgnomeui config");
  }  

  g_free(s);
  
  /* Fixme. There's a little problem with no error value from the 
     bool get function. This makes it yucky to do the propertybox 
     thing. The intermediate 'b' variable is only in anticipation
     of a future way to check for errors. */

  b = gnome_config_get_bool_with_default(DIALOG_CENTERED_KEY"=true",
					 NULL);
  settings->dialog_centered = b;

  /* This is unused for now */
  b = gnome_config_get_bool_with_default(PROPERTY_BOX_BUTTONS_OK_KEY"=true",
					 NULL);
  settings->property_box_buttons_ok = b;

  b = gnome_config_get_bool_with_default(PROPERTY_BOX_BUTTONS_APPLY_KEY"=true",
					 NULL);
  settings->property_box_buttons_apply = b;

  b = gnome_config_get_bool_with_default(PROPERTY_BOX_BUTTONS_CLOSE_KEY"=true",
					 NULL);
  settings->property_box_buttons_close = b;

  b = gnome_config_get_bool_with_default(PROPERTY_BOX_BUTTONS_HELP_KEY"=true",
					 NULL);
  settings->property_box_buttons_help = b;

  gnome_config_pop_prefix();
  gnome_config_push_prefix(STATUSBAR);

  b = gnome_config_get_bool_with_default(STATUSBAR_DIALOG_KEY"=false",
					 NULL);
  settings->statusbar_not_dialog = b;
  
  b = gnome_config_get_bool_with_default(STATUSBAR_INTERACTIVE_KEY"=false",
					 NULL);
  settings->statusbar_is_interactive = b;

  b = gnome_config_get_bool_with_default(STATUSBAR_METER_ON_RIGHT"=true",
					 NULL);
  settings->statusbar_meter_on_right = b;

  gnome_config_pop_prefix();
  gnome_config_push_prefix(APP);

  b = gnome_config_get_bool_with_default(MENUBAR_DETACHABLE_KEY"=true",
					 NULL);
  settings->menubar_detachable = b;

  b = gnome_config_get_bool_with_default(MENUBAR_RELIEF_KEY"=true",
					 NULL);
  settings->menubar_relief = b;

  b = gnome_config_get_bool_with_default(TOOLBAR_DETACHABLE_KEY"=true",
					 NULL);
  settings->toolbar_detachable = b;

  b = gnome_config_get_bool_with_default(TOOLBAR_RELIEF_KEY"=true",
					 NULL);
  settings->toolbar_relief = b;

  b = gnome_config_get_bool_with_default(TOOLBAR_RELIEF_BTN_KEY"=false",
					 NULL);
  settings->toolbar_relief_btn = b;

  b = gnome_config_get_bool_with_default(TOOLBAR_LINES_KEY"=true",
					 NULL);
  settings->toolbar_lines = b;

  b = gnome_config_get_bool_with_default(TOOLBAR_LABELS_KEY"=true",
					 NULL);
  settings->toolbar_labels = b;

  b = gnome_config_get_bool_with_default (MENUS_HAVE_TEAROFF_KEY"=true",
					  NULL);
  settings->menus_have_tearoff = b;

  b = gnome_config_get_bool_with_default (MENUS_HAVE_ICONS_KEY"=true",
					  NULL);
  settings->menus_have_icons = b;

  gnome_config_pop_prefix ();
  gnome_config_push_prefix (CACHE);
  b = gnome_config_get_bool_with_default (DISABLE_IMLIB_CACHE_KEY"=true",
					  NULL);
  settings->disable_imlib_cache = b;
  
  gnome_config_pop_prefix();
  gnome_config_push_prefix(MDI);

  s = gnome_config_get_string(MDI_MODE_KEY);

  if ( ! enum_from_strings((int*) &settings->mdi_mode, s,
			   mdi_modes, NUM_MDI_MODES) ) {
    g_warning("Didn't recognize MDI mode in libgnomeui config");
  }

  g_free(s);
  
  s = gnome_config_get_string(MDI_TAB_POS_KEY);

  if ( ! enum_from_strings((int*) &settings->mdi_tab_pos, s,
			   tab_positions, NUM_TAB_POSITIONS) ) {
    g_warning("Didn't recognize MDI notebook tab position in libgnomeui config");
  }

  g_free(s);
  
  gnome_config_pop_prefix();
}

/**
 * gnome_preferences_save_custom
 * @settings: App-specified set of user preferences
 *
 * Description:
 * Uses gnome_config_xxx() interface to store a set of
 * standard GNOME preferences from info in the @settings object.
 **/
 
void
gnome_preferences_save_custom(GnomePreferences *settings)
{
  gnome_config_push_prefix(DIALOGS);
  
  gnome_config_set_string(DIALOG_BUTTONS_STYLE_KEY, 
			  dialog_button_styles[settings->dialog_buttons_style]);

  gnome_config_set_string(DIALOG_TYPE_KEY, 
			  dialog_types[settings->dialog_type]);

  gnome_config_set_string(DIALOG_POSITION_KEY, 
			  dialog_positions[settings->dialog_position]);
  
  gnome_config_set_bool  (DIALOG_CENTERED_KEY,
			  settings->dialog_centered);

  gnome_config_pop_prefix();

  gnome_config_push_prefix(STATUSBAR);

  gnome_config_set_bool(STATUSBAR_DIALOG_KEY,
			settings->statusbar_not_dialog);
  gnome_config_set_bool(STATUSBAR_INTERACTIVE_KEY,
			settings->statusbar_is_interactive);
  gnome_config_set_bool(STATUSBAR_METER_ON_RIGHT,
			settings->statusbar_meter_on_right);


  gnome_config_pop_prefix();
  gnome_config_push_prefix(APP);

  gnome_config_set_bool(MENUBAR_DETACHABLE_KEY,
			settings->menubar_detachable);
  gnome_config_set_bool(MENUBAR_RELIEF_KEY,
			settings->menubar_relief);
  gnome_config_set_bool(TOOLBAR_DETACHABLE_KEY,
			settings->toolbar_detachable);
  gnome_config_set_bool(TOOLBAR_RELIEF_KEY,
			settings->toolbar_relief);
  gnome_config_set_bool(TOOLBAR_RELIEF_BTN_KEY,
			settings->toolbar_relief_btn);
  gnome_config_set_bool(TOOLBAR_LINES_KEY,
			settings->toolbar_lines);
  gnome_config_set_bool(TOOLBAR_LABELS_KEY,
			settings->toolbar_labels);
  gnome_config_set_bool(MENUS_HAVE_TEAROFF_KEY,
			settings->menus_have_tearoff);
  gnome_config_set_bool(MENUS_HAVE_ICONS_KEY,
			settings->menus_have_icons);
  gnome_config_set_bool(DISABLE_IMLIB_CACHE_KEY,
			settings->disable_imlib_cache);
  gnome_config_pop_prefix();
  gnome_config_push_prefix(MDI);

  gnome_config_set_string(MDI_MODE_KEY, mdi_modes[settings->mdi_mode]);
  gnome_config_set_string(MDI_TAB_POS_KEY, tab_positions[settings->mdi_tab_pos]);

  gnome_config_pop_prefix();

  gnome_config_sync();
}

/**
 * gnome_preferences_load
 *
 * Description:
 * Uses gnome_config_xxx() API to load a standard set of GNOME
 * preferences into the default GNOME preferences object.
 **/
 
void gnome_preferences_load(void)
{
  gnome_preferences_load_custom(&prefs);
}

/**
 * gnome_preferences_save
 *
 * Description:
 * Uses gnome_config_xxx() API to store a standard set of GNOME
 * preferences using info in the default GNOME preferences object.
 **/
 
void gnome_preferences_save(void)
{
  gnome_preferences_save_custom(&prefs);
}


/**
 * gnome_preferences_get_button_layout 
 *
 * Description:
 * Obtain the button style from the default GNOME preferences object.
 *
 * Returns:
 * Enumerated type indicating the default GNOME dialog button style.
 **/
    
GtkButtonBoxStyle gnome_preferences_get_button_layout (void)
{
  return prefs.dialog_buttons_style;
}


/**
 * gnome_preferences_set_button_layout :
 * @style: Enumerated type indicating the default GNOME dialog button style.
 *
 * Set the default GNOME preferences object's default button style.
 **/

void gnome_preferences_set_button_layout (GtkButtonBoxStyle style)
{
	g_return_if_fail ((style <= GTK_BUTTONBOX_END));
	prefs.dialog_buttons_style = style;
}


/**
 * gnome_preferences_get_statusbar_dialog     
 *
 * Description:
 * Determine whether or not the statusbar is a dialog.
 *
 * Returns:
 * %FALSE if statusbar is a dialog, %TRUE if not.
 **/

gboolean          gnome_preferences_get_statusbar_dialog     (void)
{
  return prefs.statusbar_not_dialog;
}


/**
 * gnome_preferences_set_statusbar_dialog:
 * @b: %FALSE if statusbar is a dialog, %TRUE if not.
 *
 * Indicate whether or not the default for GNOME status bars
 * is a dialog.
 *
 */

void              gnome_preferences_set_statusbar_dialog     (gboolean b)
{
  prefs.statusbar_not_dialog = b;
}


/**
 * gnome_preferences_get_statusbar_interactive
 *
 * Description:
 * Determine whether or not the statusbar is interactive.
 *
 * Returns:
 * %TRUE if statusbar is interactive, %FALSE if not.
 **/

gboolean          gnome_preferences_get_statusbar_interactive(void)
{
  return prefs.statusbar_is_interactive;
}


/**
 * gnome_preferences_set_statusbar_interactive:
 * @b: %TRUE if statusbar is interactive, %FALSE if not.
 *
 * Indicate whether or not the GNOME status bars are, by default,
 * interactive.
 **/

void              gnome_preferences_set_statusbar_interactive(gboolean b)
{
  prefs.statusbar_is_interactive = b;
}


/**
 * gnome_preferences_get_statusbar_meter_on_right 
 *
 * Description:
 * Determine whether or not the statusbar's meter is on the right-hand side. 
 *
 * Returns:
 * %TRUE if statusbar meter is on the right side, %FALSE if not.
 **/

gboolean gnome_preferences_get_statusbar_meter_on_right (void)
{
  return prefs.statusbar_meter_on_right;
}


/**
 * gnome_preferences_set_statusbar_meter_on_right:
 * @statusbar_meter_on_right: %TRUE if statusbar meter is on the right side, %FALSE if not.
 *
 * Indicate whether or not the GNOME status bars are, by default,
 * on the right-hand side.
 **/

void
gnome_preferences_set_statusbar_meter_on_right (gboolean statusbar_meter_on_right)
{
  prefs.statusbar_meter_on_right = statusbar_meter_on_right;
}


/**
 * gnome_preferences_get_menubar_detachable:
 *
 * Determine whether or not a menu bar is, by default,
 * detachable from its parent frame.
 *
 * Returns:
 * %TRUE if menu bars are detachable, %FALSE if not.
 */
gboolean          gnome_preferences_get_menubar_detachable   (void)
{
  return prefs.menubar_detachable;
}


/**
 * gnome_preferences_set_menubar_detachable:
 * @b: %TRUE if menu bars are detachable, %FALSE if not.
 *
 * Indicate whether or not the GNOME menu bars are, by default,
 * detachable from their parent frame.
 */
void              gnome_preferences_set_menubar_detachable   (gboolean b)
{
  prefs.menubar_detachable = b;
}


/**
 * gnome_preferences_get_menubar_relief:
 *
 * Returns: the relieft settings for the menubar.
 */
gboolean          gnome_preferences_get_menubar_relief    (void)
{
  return prefs.menubar_relief;
}


/**
 * gnome_preferences_set_menubar_relief:
 * @b: 
 *
 */
void              gnome_preferences_set_menubar_relief    (gboolean b)
{
  prefs.menubar_relief = b;
}


/**
 * gnome_preferences_get_toolbar_detachable:
 *
 */
gboolean          gnome_preferences_get_toolbar_detachable   (void)
{
  return prefs.toolbar_detachable;
}


/**
 * gnome_preferences_set_toolbar_detachable:
 * @b:
 *
 */
void              gnome_preferences_set_toolbar_detachable   (gboolean b)
{
  prefs.toolbar_detachable = b;
}


/**
 * gnome_preferences_get_toolbar_relief:
 *
 */
gboolean          gnome_preferences_get_toolbar_relief    (void)
{
  return prefs.toolbar_relief;
}


/**
 * gnome_preferences_set_toolbar_relief:
 * @b:
 *
 */
void              gnome_preferences_set_toolbar_relief    (gboolean b)
{
  prefs.toolbar_relief = b;
}


/**
 * gnome_preferences_get_toolbar_relief_btn:
 *
 */
gboolean          gnome_preferences_get_toolbar_relief_btn (void)
{
  return prefs.toolbar_relief_btn;
}


/**
 * gnome_preferences_set_toolbar_relief_btn:
 * @b:
 *
 */
void              gnome_preferences_set_toolbar_relief_btn (gboolean b)
{
  prefs.toolbar_relief_btn = b;
}


/**
 * gnome_preferences_get_toolbar_lines:
 *
 */
gboolean          gnome_preferences_get_toolbar_lines     (void)
{
  return prefs.toolbar_lines;
}


/**
 * gnome_preferences_set_toolbar_lines:
 * @b:
 *
 */
void              gnome_preferences_set_toolbar_lines     (gboolean b)
{
  prefs.toolbar_lines = b;
}


/**
 * gnome_preferences_get_toolbar_labels:
 *
 */
gboolean          gnome_preferences_get_toolbar_labels    (void)
{
  return prefs.toolbar_labels;
}


/**
 * gnome_preferences_set_toolbar_labels:
 * @b:
 *
 */
void              gnome_preferences_set_toolbar_labels    (gboolean b)
{
  prefs.toolbar_labels = b;
}


/**
 * gnome_preferences_get_dialog_centered:
 *
 */
gboolean          gnome_preferences_get_dialog_centered      (void)
{
  return prefs.dialog_centered;
}


/**
 * gnome_preferences_set_dialog_centered:
 * @b:
 *
 */
void              gnome_preferences_set_dialog_centered      (gboolean b)
{
  prefs.dialog_centered = b;
}


/**
 * gnome_preferences_get_dialog_type:
 *
 */
GtkWindowType     gnome_preferences_get_dialog_type          (void)
{
  return prefs.dialog_type;
}


/**
 * gnome_preferences_set_dialog_type:
 * @t:
 *
 */
void              gnome_preferences_set_dialog_type          (GtkWindowType t)
{
  prefs.dialog_type = t;
}

/* Whether dialogs are GTK_WIN_POS_NONE, GTK_WIN_POS_CENTER,
   GTK_WIN_POS_MOUSE */

/**
 * gnome_preferences_get_dialog_position:
 *
 */
GtkWindowPosition gnome_preferences_get_dialog_position      (void)
{
  return prefs.dialog_position;
}


/**
 * gnome_preferences_set_dialog_position:
 * @p:
 *
 */
void              gnome_preferences_set_dialog_position      (GtkWindowPosition p)
{
  prefs.dialog_position = p;
}


/**
 * gnome_preferences_get_mdi_mode:
 *
 */
GnomeMDIMode      gnome_preferences_get_mdi_mode             (void)
{
  return prefs.mdi_mode;
}


/**
 * gnome_preferences_set_mdi_mode:
 * @m:
 *
 */
void              gnome_preferences_set_mdi_mode             (GnomeMDIMode m)
{
  prefs.mdi_mode = m;
}


/**
 * gnome_preferences_get_mdi_tab_pos:
 *
 */
GtkPositionType   gnome_preferences_get_mdi_tab_pos          (void)
{
  return prefs.mdi_tab_pos;
}


/**
 * gnome_preferences_set_mdi_tab_pos:
 * @p:
 *
 */

void              gnome_preferences_set_mdi_tab_pos          (GtkPositionType p)
{
  prefs.mdi_tab_pos = p;
}


/**
 * gnome_preferences_get_property_box_apply:
 *
 */
int gnome_preferences_get_property_box_apply (void)
{
	return prefs.property_box_buttons_apply;
}


/**
 * gnome_preferences_set_property_box_button_apply:
 * @v:
 *
 */

void gnome_preferences_set_property_box_button_apply (int v)
{
	prefs.property_box_buttons_apply = v;
}


/**
 * gnome_preferences_get_menus_have_tearoff:
 *
 */

gboolean gnome_preferences_get_menus_have_tearoff (void)
{
	return prefs.menus_have_tearoff;
}


/**
 * gnome_preferences_set_menus_have_tearoff:
 * @b:
 *
 */
void gnome_preferences_set_menus_have_tearoff (gboolean b)
{
	prefs.menus_have_tearoff = b;
}


/**
 * gnome_preferences_get_menus_have_icons:
 *
 */
int gnome_preferences_get_menus_have_icons (void)
{
	return prefs.menus_have_icons;
}


/**
 * gnome_preferences_set_menus_have_icons:
 * @have_icons:
 *
 */
void gnome_preferences_set_menus_have_icons (int have_icons)
{
	prefs.menus_have_icons = have_icons ? TRUE : FALSE;
}


/**
 * gnome_preferences_get_disable_imlib_cache:
 *
 * Description:
 */
int gnome_preferences_get_disable_imlib_cache (void)
{
	return prefs.disable_imlib_cache;
}


/**
 * gnome_preferences_set_disable_imlib_cache:
 * @disable_imlib_cache:
 *
 */
void gnome_preferences_set_disable_imlib_cache (int disable_imlib_cache)
{
	prefs.disable_imlib_cache = disable_imlib_cache ? TRUE : FALSE;
}

