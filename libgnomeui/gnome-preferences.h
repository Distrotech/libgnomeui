/*
 * Copyright (C) 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

/* DEPRECATED */
#ifndef GNOME_PREFERENCES_H
#define GNOME_PREFERENCES_H
/****
  Gnome preferences settings

  These should be used only in libgnomeui and Gnome configuration
  utilities. Don't worry about them otherwise, the library should
  handle it (if not, fix the library, not your app).
  ****/

/*************
          OK, these were already not supposed to be used; now they
          are massively deprecated. Access the standardized GConf keys
          directly, instead.
**************/


#include <gtk/gtkbbox.h>

#include "gnome-mdi.h"

G_BEGIN_DECLS

/* Global config choices. App-specific choices are handled in GnomeApp. */

/* By now an opaque type */
typedef struct _GnomePreferences GnomePreferences;

/* Load and sync the config file. */
void gnome_preferences_load(void);
void gnome_preferences_load_custom(GnomePreferences *settings);
void gnome_preferences_save(void);
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

G_END_DECLS

#endif
