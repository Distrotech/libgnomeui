/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gnome-authn-manager-private.h - machinary for handling authentication to URI's

   Copyright (C) 2004 Red Hat, Inc.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: Alexander Larsson <alexl@redhat.com>
*/

#ifndef GNOME_AUTHENTICATION_MANAGER_PRIVATE_H
#define GNOME_AUTHENTICATION_MANAGER_PRIVATE_H

/* TODO: These are just private to avoid breaking code freeze.
 * they should probably be exported later, as they could be
 * useful to others
 */
   
void 	gnome_authentication_manager_push_sync (void);
void 	gnome_authentication_manager_pop_sync (void);

void 	gnome_authentication_manager_push_async (void);
void 	gnome_authentication_manager_pop_async (void);

#endif /* GNOME_AUTHENTICATION_MANAGER_PRIVATE_H */
