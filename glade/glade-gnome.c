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
#include <libgnomeui/libgnomeui.h>

static GtkWidget *
druidpagestandard_find_internal_child(GladeXML *xml, GtkWidget *parent,
				      const gchar *childname)
{
    if (!strcmp(childname, "vbox"))
	return GNOME_DRUID_PAGE_STANDARD(parent)->vbox;
    return NULL;
}

static GtkWidget *
dialog_find_internal_child(GladeXML *xml, GtkWidget *parent,
			   const gchar *childname)
{
    if (!strcmp(childname, "vbox"))
	return GNOME_DIALOG(parent)->vbox;
    g_warning ("Internal '%s'", childname);
    return NULL;
}

static GtkWidget *
entry_find_internal_child(GladeXML *xml, GtkWidget *parent,
			  const gchar *childname)
{
    if (!strcmp (childname, "entry"))
	return gnome_entry_gtk_entry (GNOME_ENTRY (parent));

    return NULL;
}

static GtkWidget *
file_entry_find_internal_chid(GladeXML *xml, GtkWidget *parent,
			      const gchar *childname)
{
    if (!strcmp (childname, "entry"))
	return gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (parent));

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


/* this macro puts a version check function into the module */
GLADE_MODULE_CHECK_INIT

void
glade_module_register_widgets (void)
{
    glade_require ("bonobo");

    glade_register_widget (GNOME_TYPE_APP, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GNOME_TYPE_APPBAR, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GNOME_TYPE_COLOR_PICKER,glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GNOME_TYPE_DATE_EDIT, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GNOME_TYPE_DIALOG, glade_standard_build_widget,
			   glade_standard_build_children, dialog_find_internal_child);
    glade_register_widget (GNOME_TYPE_DRUID, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GNOME_TYPE_DRUID_PAGE, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (GNOME_TYPE_DRUID_PAGE_EDGE, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GNOME_TYPE_DRUID_PAGE_STANDARD, glade_standard_build_widget,
			   glade_standard_build_children, druidpagestandard_find_internal_child);
    glade_register_widget (GNOME_TYPE_ENTRY, glade_standard_build_widget,
			   glade_standard_build_children, entry_find_internal_child);
    glade_register_widget (GNOME_TYPE_FILE_ENTRY, glade_standard_build_widget,
			   glade_standard_build_children, file_entry_find_internal_child);
    glade_register_widget (GNOME_TYPE_HREF, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GNOME_TYPE_ICON_ENTRY, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GNOME_TYPE_ICON_LIST, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GNOME_TYPE_ICON_SELECTION, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GNOME_TYPE_PIXMAP_ENTRY, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (GNOME_TYPE_PROPERTY_BOX, glade_standard_build_widget,
			   glade_standard_build_children, propertybox_find_internal_child);
    glade_register_widget (GNOME_TYPE_SCORES, glade_standard_build_widget,
			   NULL, NULL);

    glade_provide ("gnome");
}
