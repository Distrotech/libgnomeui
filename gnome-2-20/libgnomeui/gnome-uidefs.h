/* gnome-uidefs.h:
 * Copyright (C) 1998 Havoc Pennington
 * All rights reserved.
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
/*
  @NOTATION@
*/

#ifndef GNOME_UIDEFS_H
#define GNOME_UIDEFS_H

/* This file defines standard sizes, spacings, and whatever
   else seems standardizable via simple defines. */

/* All-purpose padding. If you always use these instead of making up 
   some arbitrary padding number that looks good on your screen, 
   people can change the "spaciousness" of the GUI globally. */
#define GNOME_PAD          8
#define GNOME_PAD_SMALL    4
#define GNOME_PAD_BIG      12

/* Deprecated with GnomeDialog, GTK has a much nicer way of
 * doing this */
#ifndef GNOME_DISABLE_DEPRECATED

/* These are the button numbers on a yes-no or ok-cancel GnomeDialog,
   and in the gnome-app-util callbacks. Make the program more
   readable, is all. */ 
#define GNOME_YES 0 
#define GNOME_NO 1 
#define GNOME_OK 0
#define GNOME_CANCEL 1

#endif

#include <gdk/gdkkeysyms.h>

/* These are keybindings, in GnomeUIInfo format. USE THEM OR DIE! 
   Add to the list as well..
*/

#define GNOME_KEY_NAME_QUIT 	        'q'
#define GNOME_KEY_MOD_QUIT	        (GDK_CONTROL_MASK)

#ifndef GNOME_DISABLE_DEPRECATED

#define GNOME_KEY_NAME_EXIT	GNOME_KEY_NAME_QUIT
#define GNOME_KEY_MOD_EXIT	GNOME_KEY_MOD_QUIT

#endif

#define GNOME_KEY_NAME_CLOSE 	        'w'
#define	GNOME_KEY_MOD_CLOSE	        (GDK_CONTROL_MASK)

#define GNOME_KEY_NAME_CUT 	        'x'
#define GNOME_KEY_MOD_CUT 	        (GDK_CONTROL_MASK)
#define GNOME_KEY_NAME_COPY	        'c'
#define GNOME_KEY_MOD_COPY	        (GDK_CONTROL_MASK)
#define GNOME_KEY_NAME_PASTE 	        'v'
#define GNOME_KEY_MOD_PASTE 	        (GDK_CONTROL_MASK)
#define GNOME_KEY_NAME_SELECT_ALL       'a'
#define GNOME_KEY_MOD_SELECT_ALL        (GDK_CONTROL_MASK)
#define GNOME_KEY_NAME_CLEAR 	        0
#define GNOME_KEY_MOD_CLEAR 	        (0)

#define GNOME_KEY_NAME_UNDO  	        'z'
#define GNOME_KEY_MOD_UNDO  	        (GDK_CONTROL_MASK)
#define GNOME_KEY_NAME_REDO	        'z'
#define GNOME_KEY_MOD_REDO	        (GDK_CONTROL_MASK | GDK_SHIFT_MASK)

#define GNOME_KEY_NAME_SAVE	        's'
#define GNOME_KEY_MOD_SAVE	        (GDK_CONTROL_MASK)
#define GNOME_KEY_NAME_OPEN	        'o'
#define GNOME_KEY_MOD_OPEN	        (GDK_CONTROL_MASK)
#define GNOME_KEY_NAME_SAVE_AS	        's'
#define GNOME_KEY_MOD_SAVE_AS           (GDK_CONTROL_MASK | GDK_SHIFT_MASK)
#define GNOME_KEY_NAME_NEW	        'n'
#define GNOME_KEY_MOD_NEW	        (GDK_CONTROL_MASK)

#define GNOME_KEY_NAME_PRINT            'p'
#define GNOME_KEY_MOD_PRINT             (GDK_CONTROL_MASK)

#define GNOME_KEY_NAME_PRINT_SETUP      0
#define GNOME_KEY_MOD_PRINT_SETUP       (0)

#define GNOME_KEY_NAME_FIND             'f'
#define GNOME_KEY_MOD_FIND              (GDK_CONTROL_MASK)
#define GNOME_KEY_NAME_FIND_AGAIN       'g'
#define GNOME_KEY_MOD_FIND_AGAIN        (GDK_CONTROL_MASK)
#define GNOME_KEY_NAME_REPLACE          'r'
#define GNOME_KEY_MOD_REPLACE           (GDK_CONTROL_MASK)

#define GNOME_KEY_NAME_NEW_WINDOW       0
#define GNOME_KEY_MOD_NEW_WINDOW        (0)
#define GNOME_KEY_NAME_CLOSE_WINDOW     0
#define GNOME_KEY_MOD_CLOSE_WINDOW      (0)

#define GNOME_KEY_NAME_REDO_MOVE        'z'
#define GNOME_KEY_MOD_REDO_MOVE         (GDK_CONTROL_MASK | GDK_SHIFT_MASK)
#define GNOME_KEY_NAME_UNDO_MOVE        'z'
#define GNOME_KEY_MOD_UNDO_MOVE         (GDK_CONTROL_MASK)

#define GNOME_KEY_NAME_PAUSE_GAME       GDK_Pause
#define GNOME_KEY_MOD_PAUSE_GAME        (0)
#define GNOME_KEY_NAME_NEW_GAME         'n'
#define GNOME_KEY_MOD_NEW_GAME          (GDK_CONTROL_MASK)

#endif
