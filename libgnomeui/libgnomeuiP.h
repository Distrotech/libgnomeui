/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
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

#ifndef LIBGNOMEUIP_H
#define LIBGNOMEUIP_H

#include <glib.h>
#include <gobject/gsignal.h>
#include <gtk/gtktypeutils.h>



#include "gnome-macros.h"
#include "gnometypebuiltins.h"
#include "gnomemarshal.h"

G_BEGIN_DECLS

void gnome_type_init(void);

/* FIXME: broken */
extern const GList *gnome_i18n_get_language_list (const gchar *category_name);
extern char *gnome_i18n_get_preferred_language (void);

G_END_DECLS

#endif /* LIBGNOMEUIP_H */

