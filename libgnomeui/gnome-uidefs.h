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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

#endif
