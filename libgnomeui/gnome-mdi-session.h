/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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

/*
 * gnome-mdi-session.h - session managament functions
 * written by Martin Baulig <martin@home-of-linux.org>
 */

#ifndef __GNOME_MDI_SESSION_H__
#define __GNOME_MDI_SESSION_H__

#include <string.h>

#include <libgnomeui/gnome-mdi.h>

G_BEGIN_DECLS

/* This function should parse the config string and return a newly
 * created GnomeMDIChild. */
typedef GnomeMDIChild *(*GnomeMDIChildCreator) (const gchar *);

/* gnome_mdi_restore_state(): call this with the GnomeMDI object, the
 * config section name and the function used to recreate the GnomeMDIChildren
 * from their config strings. */
gboolean	gnome_mdi_restore_state	(GnomeMDI *mdi, const gchar *section,
					 GnomeMDIChildCreator child_create_func);

/* gnome_mdi_save_state (): call this with the GnomeMDI object as the
 * first and the config section name as the second argument. */
void		gnome_mdi_save_state	(GnomeMDI *mdi, const gchar *section);

G_END_DECLS

#endif
