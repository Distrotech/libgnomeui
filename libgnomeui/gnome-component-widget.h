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

/* GnomeSelector widget
 *
 * Author: Martin Baulig <baulig@suse.de>
 */

#ifndef GNOME_SELECTOR_WIDGET_H
#define GNOME_SELECTOR_WIDGET_H


#include <bonobo/bonobo-widget.h>
#include <libgnome/Gnome.h>


G_BEGIN_DECLS


#define GNOME_TYPE_SELECTOR_WIDGET            (gnome_selector_widget_get_type ())
#define GNOME_SELECTOR_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_SELECTOR_WIDGET, GnomeSelectorWidget))
#define GNOME_SELECTOR_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_SELECTOR_WIDGET, GnomeSelectorWidgetClass))
#define GNOME_IS_SELECTOR_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_SELECTOR_WIDGET))
#define GNOME_IS_SELECTOR_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_SELECTOR_WIDGET))


typedef struct _GnomeSelectorWidget             GnomeSelectorWidget;
typedef struct _GnomeSelectorWidgetPrivate      GnomeSelectorWidgetPrivate;
typedef struct _GnomeSelectorWidgetClass        GnomeSelectorWidgetClass;

struct _GnomeSelectorWidget {
    BonoboWidget widget;
        
    /*< private >*/
    GnomeSelectorWidgetPrivate *_priv;
};

struct _GnomeSelectorWidgetClass {
    BonoboWidgetClass parent_class;
};

GtkType
gnome_selector_widget_get_type              (void) G_GNUC_CONST;

G_END_DECLS

#endif
