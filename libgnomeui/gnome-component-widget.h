/* -*- Mode: C; c-set-style: gnu indent-tabs-mode: t; c-basic-offset: 4; tab-width: 8 -*- */
/*
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

/* GnomeSelector client
 *
 * Author: Martin Baulig <baulig@suse.de>
 */

#ifndef GNOME_SELECTOR_CLIENT_H
#define GNOME_SELECTOR_CLIENT_H


#include <bonobo/bonobo-widget.h>
#include <libgnome/Gnome.h>


G_BEGIN_DECLS


#define GNOME_TYPE_SELECTOR_CLIENT            (gnome_selector_client_get_type ())
#define GNOME_SELECTOR_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_SELECTOR_CLIENT, GnomeSelectorClient))
#define GNOME_SELECTOR_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_SELECTOR_CLIENT, GnomeSelectorClientClass))
#define GNOME_IS_SELECTOR_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_SELECTOR_CLIENT))
#define GNOME_IS_SELECTOR_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_SELECTOR_CLIENT))


typedef struct _GnomeSelectorClient             GnomeSelectorClient;
typedef struct _GnomeSelectorClientPrivate      GnomeSelectorClientPrivate;
typedef struct _GnomeSelectorClientClass        GnomeSelectorClientClass;

typedef struct _GnomeSelectorClientAsyncHandle  GnomeSelectorClientAsyncHandle;

typedef void (*GnomeSelectorClientAsyncFunc)   (GnomeSelectorClient            *client,
                                                GnomeSelectorClientAsyncHandle *async_handle,
                                                GNOME_Selector_AsyncType        async_type,
                                                const gchar                    *uri,
                                                const gchar                    *error,
                                                gboolean                        success,
                                                gpointer                        user_data);


struct _GnomeSelectorClient {
    BonoboWidget widget;
        
    /*< private >*/
    GnomeSelectorClientPrivate *_priv;
};

struct _GnomeSelectorClientClass {
    BonoboWidgetClass parent_class;
};

GtkType
gnome_selector_client_get_type              (void) G_GNUC_CONST;

GnomeSelectorClient *
gnome_selector_client_new                   (const gchar         *moniker,
                                             Bonobo_UIContainer   uic);

GnomeSelectorClient *
gnome_selector_client_new_from_objref       (GNOME_Selector       corba_selector,
                                             Bonobo_UIContainer   uic);

GnomeSelectorClient *
gnome_selector_client_construct             (GnomeSelectorClient *client,
                                             const gchar         *moniker,
                                             Bonobo_UIContainer   uic);

GnomeSelectorClient *
gnome_selector_client_construct_from_objref (GnomeSelectorClient *client,
                                             GNOME_Selector       corba_selector,
                                             Bonobo_UIContainer   uic);

/* Get/set the text in the entry widget. */
gchar *
gnome_selector_client_get_entry_text        (GnomeSelectorClient *client);

void
gnome_selector_client_set_entry_text        (GnomeSelectorClient *client,
                                             const gchar         *text);

/* If the entry widget is derived from GtkEditable, then we can use this
 * function to send an "activate" signal to it. */
void
gnome_selector_client_activate_entry        (GnomeSelectorClient *client);

/* Get/set URI. */

gchar *
gnome_selector_client_get_uri               (GnomeSelectorClient *client);

void
gnome_selector_client_set_uri               (GnomeSelectorClient             *client,
                                             GnomeSelectorClientAsyncHandle **handle_return,
                                             const gchar                     *uri,
                                             guint                            timeout_msec,
                                             GnomeSelectorClientAsyncFunc     async_func,
                                             gpointer                         user_data,
                                             GDestroyNotify                   destroy_fn);

G_END_DECLS

#endif
