/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
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

#ifndef GNOME_TYPES_H
#define GNOME_TYPES_H
/****
  Gnome-wide useful types.
  ****/



G_BEGIN_DECLS

/* string is a g_malloc'd string which should be freed, or NULL if the
   user cancelled. */
typedef void (* GnomeStringCallback)(gchar * string, gpointer data); 

/* See gnome-uidefs for the Yes/No Ok/Cancel defines which can be
   "reply" */
typedef void (* GnomeReplyCallback)(gint reply, gpointer data);

/* Do something never, only when the user wants, or always. */
typedef enum {
  GNOME_PREFERENCES_NEVER,
  GNOME_PREFERENCES_USER,
  GNOME_PREFERENCES_ALWAYS
} GnomePreferencesType;

G_END_DECLS

#endif
