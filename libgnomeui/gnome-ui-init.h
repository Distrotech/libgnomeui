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

#ifndef GNOME_INIT_H
#define GNOME_INIT_H

G_BEGIN_DECLS

#include <libgnome/gnome-program.h>

#define LIBGNOMEUI_PARAM_CRASH_DIALOG	"show-crash-dialog"
#define LIBGNOMEUI_PARAM_DISPLAY	"display"
#define LIBGNOMEUI_PARAM_DEFAULT_ICON	"default-icon"

extern GnomeModuleInfo libgnomeui_module_info;
extern GnomeModuleInfo libbonoboui_module_info;

/* The gnome_init define is in libgnomeui.h so it can be a macro */
int gnome_init_with_popt_table(const char *app_id,
			       const char *app_version,
			       int argc, char **argv,
			       const struct poptOption *options,
			       int flags,
			       poptContext *return_ctx);


G_END_DECLS

#endif
