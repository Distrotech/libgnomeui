/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright 2002 Sun Microsystems Inc.
 *
 * Lib GNOME UI Accessibility support module
 *
 * This code copied from Wipro's panel-access.c.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */
#include <config.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkaccessible.h>
#include <atk/atkrelationset.h>
#include "libgnomeui-access.h"

static gint is_gail_loaded (GtkWidget *widget);

/* variable that indicates whether GAIL is loaded or not */
gint gail_loaded = -1;

/* Accessibility support routines for libgnomeui */
static gint
is_gail_loaded (GtkWidget *widget)
{
	AtkObject *aobj;
	if (gail_loaded == -1) {
		aobj = gtk_widget_get_accessible (widget);
		if (!GTK_IS_ACCESSIBLE (aobj))
			gail_loaded = 0;
		else
			gail_loaded = 1;
	}
	return gail_loaded;
}

/* routine to add accessible name and description to an atk object */
void
_add_atk_name_desc (GtkWidget *widget, gchar *name, gchar *desc)
{
	AtkObject *aobj;

	g_return_if_fail (GTK_IS_WIDGET (widget));

	if (! is_gail_loaded (widget))
		return;

	aobj = gtk_widget_get_accessible (widget);

	if (name)
		atk_object_set_name (aobj, name);
	if (desc)
		atk_object_set_description (aobj, desc);
}

void
_add_atk_description (GtkWidget *widget,
		      gchar     *desc)
{
	_add_atk_name_desc (widget, NULL, desc);
}

void
_add_atk_relation (GtkWidget *widget1, GtkWidget *widget2,
		   AtkRelationType w1_to_w2, AtkRelationType w2_to_w1)
{
	AtkObject      *atk_widget1;
	AtkObject      *atk_widget2;
	AtkRelationSet *relation_set;
	AtkRelation    *relation;
	AtkObject      *targets[1];

	atk_widget1 = gtk_widget_get_accessible (widget1);
	atk_widget2 = gtk_widget_get_accessible (widget2);

	/* Create the widget1 -> widget2 relation */
	relation_set = atk_object_ref_relation_set (atk_widget1);
	targets[0] = atk_widget2;
	relation = atk_relation_new (targets, 1, w1_to_w2);
	atk_relation_set_add (relation_set, relation);
	g_object_unref (relation);

	/* Create the widget2 -> widget1 relation */
	relation_set = atk_object_ref_relation_set (atk_widget2);
	targets[0] = atk_widget1;
	relation = atk_relation_new (targets, 1, w2_to_w1);
	atk_relation_set_add (relation_set, relation);
	g_object_unref (relation);
}
