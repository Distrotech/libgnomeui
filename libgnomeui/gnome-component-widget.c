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

#include <config.h>
#include <libgnomeui/gnome-component-widget.h>
#include <bonobo/bonobo-moniker-util.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-async.h>
#include <bonobo/bonobo-main.h>

struct _GnomeComponentWidgetPrivate {
    gchar *moniker;
    Bonobo_Unknown corba_objref;
    Bonobo_UIContainer corba_ui_container;

    gboolean constructed;
};

static GObject*
gnome_component_widget_constructor (GType                  type,
				   guint                  n_construct_properties,
				   GObjectConstructParam *construct_properties);

static BonoboWidgetClass *gnome_component_widget_parent_class;

enum {
    PROP_0,

    /* Construction properties */
    PROP_MONIKER,
    PROP_CORBA_OBJREF,
    PROP_CORBA_UI_CONTAINER,
};

static void
gnome_component_widget_finalize (GObject *object)
{
    GnomeComponentWidget *widget = GNOME_COMPONENT_WIDGET (object);

    g_free (widget->_priv);
    widget->_priv = NULL;

    G_OBJECT_CLASS (gnome_component_widget_parent_class)->finalize (object);
}

static void
gnome_component_widget_set_property (GObject *object, guint param_id,
				    const GValue *value, GParamSpec *pspec)
{
    GnomeComponentWidget *widget;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_COMPONENT_WIDGET (object));

    widget = GNOME_COMPONENT_WIDGET (object);

    switch (param_id) {
    case PROP_MONIKER:
	g_assert (!widget->_priv->constructed);
	widget->_priv->moniker = g_value_dup_string (value);
	break;
    case PROP_CORBA_OBJREF:
	g_assert (!widget->_priv->constructed);
	widget->_priv->corba_objref = g_value_get_pointer (value);
	break;
    case PROP_CORBA_UI_CONTAINER:
	g_assert (!widget->_priv->constructed);
	widget->_priv->corba_ui_container = g_value_get_pointer (value);
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	break;
    }
}

static void
gnome_component_widget_get_property (GObject *object, guint param_id, GValue *value,
				    GParamSpec *pspec)
{
    GnomeComponentWidget *widget;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_COMPONENT_WIDGET (object));

    widget = GNOME_COMPONENT_WIDGET (object);

    switch (param_id) {
    case PROP_MONIKER:
	g_value_set_string (value, widget->_priv->moniker);
	break;
    case PROP_CORBA_OBJREF:
	g_value_set_pointer (value, widget->_priv->corba_objref);
	break;
    case PROP_CORBA_UI_CONTAINER:
	g_value_set_pointer (value, widget->_priv->corba_ui_container);
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	break;
    }
}

static void
gnome_component_widget_class_init (GnomeComponentWidgetClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;

    gnome_component_widget_parent_class = g_type_class_peek_parent (klass);

    object_class->constructor = gnome_component_widget_constructor;
    object_class->finalize = gnome_component_widget_finalize;
    object_class->set_property = gnome_component_widget_set_property;
    object_class->get_property = gnome_component_widget_get_property;

    /* Construction properties */
    g_object_class_install_property
	(object_class,
	 PROP_MONIKER,
	 g_param_spec_string ("moniker", NULL, NULL,
			      NULL,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(object_class,
	 PROP_CORBA_OBJREF,
	 g_param_spec_pointer ("corba-objref", NULL, NULL,
			       (G_PARAM_READABLE | G_PARAM_WRITABLE |
				G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(object_class,
	 PROP_CORBA_UI_CONTAINER,
	 g_param_spec_pointer ("corba-ui-container", NULL, NULL,	
			       (G_PARAM_READABLE | G_PARAM_WRITABLE |
				G_PARAM_CONSTRUCT_ONLY)));
}

static void
gnome_component_widget_init (GnomeComponentWidget *widget)
{
    widget->_priv = g_new0 (GnomeComponentWidgetPrivate, 1);
}

GtkType
gnome_component_widget_get_type (void)
{
    static GtkType type = 0;

    if (! type) {
	static const GtkTypeInfo info = {
	    "GnomeComponentWidget",
	    sizeof (GnomeComponentWidget),
	    sizeof (GnomeComponentWidgetClass),
	    (GtkClassInitFunc) gnome_component_widget_class_init,
	    (GtkObjectInitFunc) gnome_component_widget_init,
	    NULL, /* reserved_1 */
	    NULL, /* reserved_2 */
	    (GtkClassInitFunc) NULL
	};

	type = gtk_type_unique (bonobo_widget_get_type (), &info);
    }

    return type;
}

extern GnomeComponentWidget *
gnome_component_widget_do_construct (GnomeComponentWidget *);

GnomeComponentWidget *
gnome_component_widget_do_construct (GnomeComponentWidget *widget)
{
    GnomeComponentWidgetPrivate *priv;

    g_return_val_if_fail (widget != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_COMPONENT_WIDGET (widget), NULL);

    priv = widget->_priv;

    if ((priv->corba_objref != CORBA_OBJECT_NIL) && (priv->moniker != NULL)) {
	g_warning (G_STRLOC ": Cannot specify both `moniker' and `corba-objref', using moniker!");
	g_free (priv->moniker);
	priv->moniker = NULL;
    }

    if (priv->corba_objref) {
	CORBA_Environment ev;
	BonoboWidget *retval;
	Bonobo_Control corba_control;

	CORBA_exception_init (&ev);
	corba_control = Bonobo_Unknown_queryInterface (priv->corba_objref, "IDL:Bonobo/Control:1.0", &ev);
	if (BONOBO_EX (&ev) || (corba_control == CORBA_OBJECT_NIL)) {
	    CORBA_exception_free (&ev);
	    g_object_unref (G_OBJECT (widget));
	    return NULL;
	}
	CORBA_exception_free (&ev);

	retval = bonobo_widget_construct_control_from_objref (BONOBO_WIDGET (widget),
							      corba_control, priv->corba_ui_container);

	if (retval == NULL) {
	    g_object_unref (G_OBJECT (widget));
	    return NULL;
	}

	priv->corba_objref = bonobo_widget_get_objref (retval);
	priv->constructed = TRUE;

	return widget;
    }

    if (priv->moniker) {
	BonoboWidget *retval;

	retval = bonobo_widget_construct_control (BONOBO_WIDGET (widget), priv->moniker,
						  priv->corba_ui_container);

	if (retval == NULL) {
	    g_object_unref (G_OBJECT (widget));
	    return NULL;
	}

	priv->corba_objref = bonobo_widget_get_objref (retval);
	priv->constructed = TRUE;

	return widget;
    }

    g_warning (G_STRLOC ": Must use either the `moniker' or the `corba-objref' property!");
    g_object_unref (G_OBJECT (widget));

    return NULL;

}

static GObject*
gnome_component_widget_constructor (GType                  type,
				   guint                  n_construct_properties,
				   GObjectConstructParam *construct_properties)
{
    GObject *object = G_OBJECT_CLASS (gnome_component_widget_parent_class)->
	constructor (type, n_construct_properties, construct_properties);
    GnomeComponentWidget *widget = GNOME_COMPONENT_WIDGET (object);

    if (type == GNOME_TYPE_COMPONENT_WIDGET)
	return (GObject *) gnome_component_widget_do_construct (widget);
    else
	return object;
}
