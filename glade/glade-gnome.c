/* -*- Mode: C; c-basic-offset: 4 -*-
 * libglade - a library for building interfaces from XML files at runtime
 * Copyright (C) 1998-2001  James Henstridge <james@daa.com.au>
 *
 * glade-gnome.c: support for libgnomeui widgets in libglade.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

/* this file is only built if GNOME support is enabled */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <glade/glade-init.h>
#include <glade/glade-build.h>

#include <libgnomeui.h>

static GtkWidget *
druidpagestandard_find_internal_child(GladeXML *xml, GtkWidget *parent,
				      const gchar *childname)
{
    if (!strcmp(childname, "vbox"))
	return GNOME_DRUID_PAGE_STANDARD(parent)->vbox;
    return NULL;
}

static GtkWidget *
entry_find_internal_child(GladeXML *xml, GtkWidget *parent,
			  const gchar *childname)
{
    return NULL;
}

static GtkWidget *
propertybox_find_internal_child(GladeXML *xml, GtkWidget *parent,
				const gchar *childname)
{
    if (!strcmp(childname, "vbox"))
	return GNOME_DIALOG(parent)->vbox;
#if 0
    else if (!strcmp(childname, "action_area"))
	return GNOME_DIALOG(parent)->action_area;
#endif
    else if (!strcmp(childname, "notebook"))
	return GNOME_PROPERTY_BOX(parent)->notebook;
    else if (!strcmp(childname, "ok_button"))
	return GNOME_PROPERTY_BOX(parent)->ok_button;
    else if (!strcmp(childname, "apply_button"))
	return GNOME_PROPERTY_BOX(parent)->apply_button;
    else if (!strcmp(childname, "cancel_button"))
	return GNOME_PROPERTY_BOX(parent)->cancel_button;
    else if (!strcmp(childname, "help_button"))
	return GNOME_PROPERTY_BOX(parent)->help_button;
    return NULL;
}


static GladeWidgetBuildData widget_data[] = {
    { "GnomeApp", glade_standard_build_widget, glade_standard_build_children,
      gnome_app_get_type },
    { "GnomeAppBar", glade_standard_build_widget, NULL,
      gnome_appbar_get_type },
    { "GnomeColorPicker", glade_standard_build_widget, NULL,
      gnome_color_picker_get_type },
    { "GnomeDateEdit", glade_standard_build_widget, NULL,
      gnome_date_edit_get_type },
    { "GnomeDruid", glade_standard_build_widget, glade_standard_build_children,
      gnome_druid_get_type },
    { "GnomeDruidPage", glade_standard_build_widget, glade_standard_build_children,
      gnome_druid_page_get_type },
    { "GnomeDruidPageEdge", glade_standard_build_widget, NULL,
      gnome_druid_page_edge_get_type },
    { "GnomeDruidPageStandard", glade_standard_build_widget, glade_standard_build_children,
      gnome_druid_page_standard_get_type, 0, druidpagestandard_find_internal_child },
    { "GnomeEntry", glade_standard_build_widget, glade_standard_build_children,
      gnome_entry_get_type, 0, entry_find_internal_child },
    { "GnomeFileEntry", glade_standard_build_widget, NULL,
      gnome_file_entry_get_type },
    { "GnomeHRef", glade_standard_build_widget, NULL,
      gnome_href_get_type },
    { "GnomeIconEntry", glade_standard_build_widget, NULL,
      gnome_icon_entry_get_type },
    { "GnomeIconList", glade_standard_build_widget, NULL,
      gnome_icon_list_get_type },
    { "GnomeIconSelection", glade_standard_build_widget, NULL,
      gnome_icon_selection_get_type },
    { "GnomePixmapEntry", glade_standard_build_widget, NULL,
      gnome_pixmap_entry_get_type },
    { "GnomePropertyBox", glade_standard_build_widget, glade_standard_build_children,
      gnome_property_box_get_type, 0, propertybox_find_internal_child },
    { "GnomeScores", glade_standard_build_widget, NULL,
      gnome_scores_get_type },
    { NULL, NULL, NULL, 0, 0 }
};

/* this macro puts a version check function into the module */
GLADE_MODULE_CHECK_INIT

void
glade_module_register_widgets (void)
{
    glade_provide ("gnome");
    glade_register_widgets (widget_data);
}
