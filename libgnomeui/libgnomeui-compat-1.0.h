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

#ifndef LIBGNOMEUI10COMPAT_H
#define LIBGNOMEUI10COMPAT_H

#define gnome_init(app_id, app_version, argc, argv) gnome_program_init(app_id, app_version, argc, argv, GNOMEUI_INIT, NULL)

#include <compat/1.0/libgnomeui/gnome-font-selector.h>
#include <compat/1.0/libgnomeui/gnome-guru.h>
#include <compat/1.0/libgnomeui/gnome-spell.h>
#include <compat/1.0/libgnomeui/gnome-startup.h>
#include <compat/1.0/libgnomeui/gtkcauldron.h>
#include <compat/1.0/libgnomeui/gnomeui10-compat.h>

#endif 
