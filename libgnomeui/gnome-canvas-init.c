/*
 * Copyright (C) 1999, 2000 Red Hat, Inc.
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

#include <config.h>
#include <gobject/genums.h>
#include <gobject/gboxed.h>

#include <libgnomecanvas/libgnomecanvas.h>
#include <libgnome/libgnome.h>

static void libgnomecanvas_pre_args_parse (GnomeProgram *,
					   const GnomeModuleInfo *);

static GnomeModuleRequirement libgnomecanvas_requirements[] = {
    {VERSION, &libgnome_module_info},
    {NULL}
};

GnomeModuleInfo libgnomecanvas_module_info = {
    "libgnomecanvas", VERSION, "GNOME Canvas",
    libgnomecanvas_requirements,
    libgnomecanvas_pre_args_parse, NULL, NULL,
    NULL,
};

static void
libgnomecanvas_pre_args_parse (GnomeProgram *program,
			       const GnomeModuleInfo *mod_info)
{
    libgnomecanvas_types_init ();
}
