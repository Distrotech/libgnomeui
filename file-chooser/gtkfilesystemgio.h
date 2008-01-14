/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2007 Carlos Garnacho
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Carlos Garnacho Parro  <carlos@imendio.com>
 */

#ifndef __GTK_FILE_SYSTEM_GIO_H__
#define __GTK_FILE_SYSTEM_GIO_H__


#include <glib-object.h>
#define GTK_FILE_SYSTEM_ENABLE_UNSUPPORTED
#include <gtk/gtkfilesystem.h>
#undef GTK_FILE_SYSTEM_ENABLE_UNSUPPORTED

G_BEGIN_DECLS


#define GTK_TYPE_FILE_SYSTEM_GIO  (gtk_file_system_gio_get_type ())
#define GTK_FILE_SYSTEM_GIO(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_FILE_SYSTEM_GIO, GtkFileSystemGio))
#define GTK_IS_FILE_SYSTEM_GIO(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_FILE_SYSTEM_GIO))

typedef struct GtkFileSystemGio GtkFileSystemGio;

GType          gtk_file_system_gio_get_type (void);
GtkFileSystem *gtk_file_system_gio_new      (void);


G_END_DECLS

#endif /* __GTK_FILE_SYSTEM_GIO_H__ */
