/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
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

/* GnomeImageSelector widget - combo box with auto-saved history
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_IMAGE_SELECTOR_H
#define GNOME_IMAGE_SELECTOR_H


#include <libgnome/gnome-selector.h>
#include <libgnomeui/gnome-selector-widget.h>


G_BEGIN_DECLS


#define GNOME_TYPE_IMAGE_SELECTOR            (gnome_image_selector_get_type ())
#define GNOME_IMAGE_SELECTOR(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_IMAGE_SELECTOR, GnomeImageSelector))
#define GNOME_IMAGE_SELECTOR_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_IMAGE_SELECTOR, GnomeImageSelectorClass))
#define GNOME_IS_IMAGE_SELECTOR(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_IMAGE_SELECTOR))
#define GNOME_IS_IMAGE_SELECTOR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_IMAGE_SELECTOR))


typedef struct _GnomeImageSelector        GnomeImageSelector;
typedef struct _GnomeImageSelectorPrivate GnomeImageSelectorPrivate;
typedef struct _GnomeImageSelectorClass   GnomeImageSelectorClass;

struct _GnomeImageSelector {
    GnomeSelectorWidget widget;

    /*< private >*/
    GnomeImageSelectorPrivate *_priv;
};

struct _GnomeImageSelectorClass {
    GnomeSelectorWidgetClass parent_class;
};


GType        gnome_image_selector_get_type          (void) G_GNUC_CONST;

GtkWidget   *gnome_image_selector_new               (void);

G_END_DECLS

#endif
