/* gnome-uidefs.h: Copyright (C) 1998 Havoc Pennington
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

/* These are the button numbers on a yes-no or ok-cancel GnomeDialog,
   and in the gnome-app-util callbacks. Make the program more
   readable, is all. */ 
#define GNOME_YES 0 
#define GNOME_NO 1 
#define GNOME_OK 0
#define GNOME_CANCEL 1

#include <gdk/gdkkeysyms.h>

/* These are keybindings, in GnomeUIInfo format. USE THEM OR DIE! 
   Add to the list as well..
*/
#define GNOME_KEY_NAME_EXIT 	        'Q'
#define GNOME_KEY_MOD_EXIT	        (GDK_CONTROL_MASK)
#define GNOME_KEY_NAME_CLOSE 	        'W'
#define	GNOME_KEY_MOD_CLOSE	        (GDK_CONTROL_MASK)

#define GNOME_KEY_NAME_CUT 	        'X'
#define GNOME_KEY_MOD_CUT 	        (GDK_CONTROL_MASK)
#define GNOME_KEY_NAME_COPY	        'C'
#define GNOME_KEY_MOD_COPY	        (GDK_CONTROL_MASK)
#define GNOME_KEY_NAME_PASTE 	        'V'
#define GNOME_KEY_MOD_PASTE 	        (GDK_CONTROL_MASK)
#define GNOME_KEY_NAME_SELECT_ALL       0
#define GNOME_KEY_MOD_SELECT_ALL        (0)
#define GNOME_KEY_NAME_CLEAR 	        0
#define GNOME_KEY_MOD_CLEAR 	        (0)

#define GNOME_KEY_NAME_UNDO  	        'Z'
#define GNOME_KEY_MOD_UNDO  	        (GDK_CONTROL_MASK)
#define GNOME_KEY_NAME_REDO	        'R'
#define GNOME_KEY_MOD_REDO	        (GDK_CONTROL_MASK)

#define GNOME_KEY_NAME_SAVE	        'S'
#define GNOME_KEY_MOD_SAVE	        (GDK_CONTROL_MASK)
#define GNOME_KEY_NAME_OPEN	        GDK_F3
#define GNOME_KEY_MOD_OPEN	        (0)
#define GNOME_KEY_NAME_SAVE_AS	        0
#define GNOME_KEY_MOD_SAVE_AS           (0)
#define GNOME_KEY_NAME_NEW	        0
#define GNOME_KEY_MOD_NEW	        (0)

#define GNOME_KEY_NAME_PRINT            0
#define GNOME_KEY_MOD_PRINT             (0)

#define GNOME_KEY_NAME_PRINT_SETUP      0
#define GNOME_KEY_MOD_PRINT_SETUP       (0)

#define GNOME_KEY_NAME_FIND             GDK_F6
#define GNOME_KEY_MOD_FIND              (0)
#define GNOME_KEY_NAME_FIND_AGAIN       GDK_F6
#define GNOME_KEY_MOD_FIND_AGAIN        (GDK_SHIFT_MASK)
#define GNOME_KEY_NAME_REPLACE          GDK_F7
#define GNOME_KEY_MOD_REPLACE           (0)

#define GNOME_KEY_NAME_NEW_WINDOW       0
#define GNOME_KEY_MOD_NEW_WINDOW        (0)
#define GNOME_KEY_NAME_CLOSE_WINDOW     0
#define GNOME_KEY_MOD_CLOSE_WINDOW      (0)

#define GNOME_KEY_NAME_REDO_MOVE        'R'
#define GNOME_KEY_MOD_REDO_MOVE         (GDK_CONTROL_MASK)
#define GNOME_KEY_NAME_UNDO_MOVE        'Z'
#define GNOME_KEY_MOD_UNDO_MOVE         (GDK_CONTROL_MASK)

#define GNOME_KEY_NAME_PAUSE_GAME       0
#define GNOME_KEY_MOD_PAUSE_GAME        (0)
#define GNOME_KEY_NAME_NEW_GAME         'N'
#define GNOME_KEY_MOD_NEW_GAME          (GDK_CONTROL_MASK)

#endif
