/* -*- Mode: C; c-set-style: gnu indent-tabs-mode: t; c-basic-offset: 4; tab-width: 8 -*- */
/*
 * private header file for gnome-*-selector.h.
 *
 * Copyright (C) 2000 SuSE GmbH
 * Author: Martin Baulig <baulig@suse.de>
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

#ifndef GNOME_SELECTORP_H
#define GNOME_SELECTORP_H


#include <gtk/gtkvbox.h>

#include "gnome-selector.h"

#include <gconf/gconf-client.h>

G_BEGIN_DECLS

enum {
    GNOME_SELECTOR_ASYNC_TYPE_UNSPECIFIED = 0,
    GNOME_SELECTOR_ASYNC_TYPE_CHECK_FILENAME = 1,
    GNOME_SELECTOR_ASYNC_TYPE_CHECK_DIRECTORY,
    GNOME_SELECTOR_ASYNC_TYPE_ADD_FILE,
    GNOME_SELECTOR_ASYNC_TYPE_ADD_DIRECTORY,
    GNOME_SELECTOR_ASYNC_TYPE_ADD_URI,
    GNOME_SELECTOR_ASYNC_TYPE_ADD_URI_LIST,
    GNOME_SELECTOR_ASYNC_TYPE_SET_URI,
    GNOME_SELECTOR_ASYNC_TYPE_LAST
};

GnomeSelectorAsyncHandle *
_gnome_selector_async_handle_get        (GnomeSelector            *selector,
                                         GnomeSelectorAsyncType    async_type,
                                         const char               *uri,
                                         GnomeSelectorAsyncFunc    async_func,
                                         gpointer                  user_data);

void
_gnome_selector_async_handle_add        (GnomeSelectorAsyncHandle *async_handle,
                                         gpointer                  async_data,
                                         GDestroyNotify            async_data_destroy);

void
_gnome_selector_async_handle_remove     (GnomeSelectorAsyncHandle *async_handle,
                                         gpointer                  async_data);

void
_gnome_selector_async_handle_completed  (GnomeSelectorAsyncHandle *async_handle,
                                         gboolean                  success);

void
_gnome_selector_async_handle_destroy    (GnomeSelectorAsyncHandle *async_handle);

void
_gnome_selector_async_handle_set_error  (GnomeSelectorAsyncHandle *async_handle,
                                         GError                   *error);

guint
_gnome_selector_register_list           (GnomeSelector            *selector,
                                         GQuark                    list_quark);

void
_gnome_selector_unregister_list         (GnomeSelector            *selector,
                                         guint                     list_id);

GSList **
_gnome_selector_get_list_by_id          (GnomeSelector            *selector,
                                         guint                     list_id);

GSList *
_gnome_selector_deep_copy_slist         (GSList                   *thelist);

void
_gnome_selector_deep_free_slist         (GSList                   *thelist);

G_END_DECLS

#endif
