/* libglade - a library for building interfaces from XML files at runtime
 * Copyright (C) 1998  James Henstridge <james@daa.com.au>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glade/glade-build.h>
#include <tree.h>

#include <gnome.h>

/* -- routines to build the children for containers -- */

/* -- routines to build widgets -- */

/* this is for the GnomeStockButton widget ... */
/* Keep these sorted */
typedef struct {
        const char *extension;
        const char *mapping;
} gnome_map_t;
static gnome_map_t gnome_stock_button_mapping [] = {
        { "APPLY",  GNOME_STOCK_BUTTON_APPLY  },
        { "CANCEL", GNOME_STOCK_BUTTON_CANCEL },
        { "CLOSE",  GNOME_STOCK_BUTTON_CLOSE  },
        { "DOWN",   GNOME_STOCK_BUTTON_DOWN   },
        { "FONT",   GNOME_STOCK_BUTTON_FONT   },
        { "HELP",   GNOME_STOCK_BUTTON_HELP   },
        { "NEXT",   GNOME_STOCK_BUTTON_NEXT   },
        { "NO",     GNOME_STOCK_BUTTON_NO     },
        { "OK",     GNOME_STOCK_BUTTON_OK     },
        { "PREV",   GNOME_STOCK_BUTTON_PREV   },
        { "UP",     GNOME_STOCK_BUTTON_UP     },
        { "YES",    GNOME_STOCK_BUTTON_YES    },
};
static int
stock_compare (const void *a, const void *b)
{
        const gnome_map_t *ga = a;
        const gnome_map_t *gb = b;

        return strcmp (ga->extension, gb->extension);
}
/* create a stock button from a string representation of its name */
static GtkWidget *button_stock_new (const char *stock_name)
{
        GtkWidget *w;
        const int len = strlen ("GNOME_STOCK_BUTTON_");
        gnome_map_t *v;
        gnome_map_t base;
                
        /* If an error happens, return this */
        if (strncmp (stock_name, "GNOME_STOCK_BUTTON_", len) != 0)
                return gtk_button_new_with_label (stock_name);

        base.extension = stock_name + len;
        v = bsearch (
                &base,
                gnome_stock_button_mapping,
                sizeof(gnome_stock_button_mapping) / sizeof(gnome_map_t),
                sizeof (gnome_stock_button_mapping [0]),
                stock_compare);
        if (v)
                return gnome_stock_button (v->mapping);
        else
                return gtk_button_new_with_label (stock_name);
}

static GtkWidget *
stock_button_new(GladeXML *xml, GNode *node)
{
        GtkWidget *button;
        xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
        char *string = NULL;
        char *stock = NULL;
  
        /*
         * This should really be a container, but GLADE is wierd in this respect
.
         * If the label property is set for this widget, insert a label.
         * Otherwise, allow a widget to be packed
         */
        for (; info; info = info->next) {
                char *content = xmlNodeGetContent(info);
                if (!strcmp(info->name, "label")) {
                        if (string) g_free(string);
                        if (stock) g_free (stock);
                        string = g_strdup(content);
                        stock = NULL;
                } else if (!strcmp(info->name, "stock_button")) {
                        if (string) g_free(string);
                        if (stock) g_free(stock);
                        stock = g_strdup(content);
                        string = NULL;
                }
                if (content)
                        free(content);
        }
        if (string != NULL) {
                button = gtk_button_new_with_label(string);
        } else if (stock != NULL) {
                button = button_stock_new (stock);
        } else
                button = gtk_button_new();

        if (stock)
                g_free (stock);
        if (string)
                g_free (string);
        return button;
}

static GtkWidget *
color_picker_new(GladeXML *xml, GNode *node) {
	GtkWidget *wid = gnome_color_picker_new();
        xmlNodePtr info = ((xmlNodePtr)node->data)->childs;

        for (; info; info = info->next) {
                char *content = xmlNodeGetContent(info);
		
		if (!strcmp(info->name, "dither"))
			gnome_color_picker_set_dither(GNOME_COLOR_PICKER(wid),
						      content[0] == 'T');
		else if (!strcmp(info->name, "use_alpha"))
			gnome_color_picker_set_use_alpha(
				GNOME_COLOR_PICKER(wid), content[0] == 'T');
		else if (!strcmp(info->name, "title"))
			gnome_color_picker_set_title(GNOME_COLOR_PICKER(wid),
						     content);
		if (content)
			free(content);
	}
	return wid;
}

static GtkWidget *
font_picker_new(GladeXML *xml, GNode *node) {
	GtkWidget *wid = gnome_font_picker_new();
        xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	gboolean use_font = FALSE;
	gint use_font_size = 14;

        for (; info; info = info->next) {
                char *content = xmlNodeGetContent(info);
		
		if (!strcmp(info->name, "title"))
			gnome_font_picker_set_title(GNOME_FONT_PICKER(wid),
						    content);
		else if (!strcmp(info->name, "preview_text"))
			gnome_font_picker_set_preview_text(
				GNOME_FONT_PICKER(wid), content);
		else if (!strcmp(info->name, "mode"))
			gnome_font_picker_set_mode(GNOME_FONT_PICKER(wid),
				glade_enum_from_string(
					GTK_TYPE_GNOME_FONT_PICKER_MODE,
					content));
		else if (!strcmp(info->name, "show_size"))
			gnome_font_picker_fi_set_show_size(
				GNOME_FONT_PICKER(wid), content[0] == 'T');
		else if (!strcmp(info->name, "use_font"))
			use_font = (content[0] == 'T');
		else if (!strcmp(info->name, "use_font_size"))
			use_font_size = strtol(content, NULL, 0);
		if (content)
			free(content);
	}
	gnome_font_picker_fi_set_use_font_in_label(GNOME_FONT_PICKER(wid),
						   use_font,
						   use_font_size);
	return wid;
}

/* -- does not look like this widget is fully coded yet
static GtkWidget *
icon_entry_new(GladeXML *xml, GNode *node) {
	GtkWidget *wid = gnome_color_picker_new();

}
*/

static GtkWidget *
href_new(GladeXML *xml, GNode *node) {
	GtkWidget *wid;
        xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	gchar *url = NULL, *label = NULL;

        for (; info; info = info->next) {
                char *content = xmlNodeGetContent(info);
		
		if (!strcmp(info->name, "url")) {
			if (url) g_free(url);
			url = g_strdup(content);
		} else if (!strcmp(info->name, "label")) {
			if (label) g_free(label);
			label = g_strdup(content);
		}
		if (content)
			free(content);
	}
	wid = gnome_href_new(url, label);
	if (url) g_free(url);
	if (label) g_free(label);
	return wid;
}

static GtkWidget *
entry_new(GladeXML *xml, GNode *node) {
	GtkWidget *wid;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	gchar *history_id = NULL;
	gint max_saved = 10;

        for (; info; info = info->next) {
                char *content = xmlNodeGetContent(info);
		
		if (!strcmp(info->name, "history_id")) {
			if (history_id) g_free(history_id);
			history_id = g_strdup(content);
		} else if (!strcmp(info->name, "max_saved"))
			max_saved = strtol(content, NULL, 0);
		if (content)
			free(content);
	}
	wid = gnome_entry_new(history_id);
	if (history_id) g_free(history_id);
	gnome_entry_set_max_saved(GNOME_ENTRY(wid), max_saved);
	return wid;
}

static GtkWidget *
file_entry_new(GladeXML *xml, GNode *node) {
	GtkWidget *wid;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	gchar *history_id = NULL, *title = NULL;
	gboolean directory = FALSE, modal = FALSE;

        for (; info; info = info->next) {
                char *content = xmlNodeGetContent(info);
		
		if (!strcmp(info->name, "history_id")) {
			if (history_id) g_free(history_id);
			history_id = g_strdup(content);
		} else if (!strcmp(info->name, "title")) {
			if (title) g_free(title);
			title = g_strdup(content);
		} else if (!strcmp(info->name, "directory"))
			directory = (content[0] == 'T');
		else if (!strcmp(info->name, "modal"))
			modal = (content[0] == 'T');
		if (content)
			free(content);
	}
	wid = gnome_file_entry_new(history_id, title);
	if (history_id) g_free(history_id);
	if (title) g_free(title);
	gnome_file_entry_set_directory(GNOME_FILE_ENTRY(wid), directory);
	gnome_file_entry_set_modal(GNOME_FILE_ENTRY(wid), modal);
	return wid;
}

static GtkWidget *
clock_new(GladeXML *xml, GNode *node) {
	GtkWidget *wid;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	GtkClockType ctype = GTK_CLOCK_REALTIME;
	gchar *format = NULL;
	time_t seconds = 0;
	gint interval = 60;

        for (; info; info = info->next) {
                char *content = xmlNodeGetContent(info);
		
		if (!strcmp(info->name, "type"))
			ctype = glade_enum_from_string(GTK_TYPE_CLOCK_TYPE,
						       content);
		else if (!strcmp(info->name, "format")) {
			if (format) g_free(format);
			format = g_strdup(content);
		} else if (!strcmp(info->name, "seconds"))
			seconds = strtol(content, NULL, 0);
		else if (!strcmp(info->name, "interval"))
			interval = strtol(content, NULL, 0);
		if (content)
			free(content);
	}
	wid = gtk_clock_new(ctype);
	gtk_clock_set_format(GTK_CLOCK(wid), format);
	if (format) g_free(format);
	gtk_clock_set_seconds(GTK_CLOCK(wid), seconds);
	gtk_clock_set_update_interval(GTK_CLOCK(wid), interval);
	return wid;
}

/* -- GnomeAnimator not finished */

static GtkWidget *
less_new(GladeXML *xml, GNode *node) {
	GtkWidget *wid = gnome_less_new();
        xmlNodePtr info = ((xmlNodePtr)node->data)->childs;

        for (; info; info = info->next) {
                char *content = xmlNodeGetContent(info);
		
		if (!strcmp(info->name, "font"))
			gnome_less_set_font(GNOME_LESS(wid),
					    gdk_font_load(content));
		if (content)
			free(content);
	}
	return wid;
}

static GtkWidget *
calculator_new(GladeXML *xml, GNode *node) {
	return gnome_calculator_new();
}

static GtkWidget *
paper_selector_new(GladeXML *xml, GNode *node) {
	return gnome_paper_selector_new();
}

static GtkWidget *
spell_new(GladeXML *xml, GNode *node) {
	return gnome_spell_new();
}

static GtkWidget *
dial_new(GladeXML *xml, GNode *node) {
	GtkWidget *wid = gtk_dial_new(glade_get_adjustment(node));
        xmlNodePtr info = ((xmlNodePtr)node->data)->childs;

        for (; info; info = info->next) {
                char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "view_only"))
			gtk_dial_set_view_only(GTK_DIAL(wid), content[0]=='T');
		else if (!strcmp(info->name, "update_policy"))
			gtk_dial_set_update_policy(GTK_DIAL(wid),
				glade_enum_from_string(GTK_TYPE_UPDATE_TYPE,
						       content));
		if (content)
			free(content);
	}
	return wid;
}

static GtkWidget *
about_new(GladeXML *xml, GNode *node) {
	GtkWidget *wid;
        xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	gchar *title = "AnApp", *version = "0.0", *copyright = NULL;
	gchar **authors = NULL;
	gchar *comments = NULL, *logo = NULL;

        for (; info; info = info->next) {
                char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "copyright")) {
			if (copyright) g_free(copyright);
			copyright = g_strdup(content);
		} else if (!strcmp(info->name, "authors")) {
			if (authors) g_strfreev(authors);
			authors = g_strsplit(content, "\n", 0);
		} else if (!strcmp(info->name, "comments")) {
			if (comments) g_free(comments);
			comments = g_strdup(content);
		} else if (!strcmp(info->name, "logo")) {
			if (logo) g_free(logo);
			logo = g_strdup(content);
		}
		if (content)
			free(content);
	}
	wid = gnome_about_new(title, version, copyright,
			      (const gchar **)authors, comments, logo);
	/* if (title) g_free(title); */
	/* if (version) g_free(version); */
	if (copyright) g_free(copyright);
	if (authors) g_strfreev(authors);
	if (comments) g_free(comments);
	if (logo) g_free(logo);
	return wid;
}

/* -- dialog box build routine goes here */

/* -- routines to initialise these widgets with libglade -- */

static const GladeWidgetBuildData widget_data[] = {
	/* this one is an alias for the stock button widget */
	{ "GnomeStockButton", stock_button_new, glade_standard_build_children},
	/* this is for the stock button hack ... */
	{ "GtkButton",        stock_button_new, glade_standard_build_children},

	{ "GnomeColorPicker",   color_picker_new,   NULL },
	{ "GnomeFontPicker",    font_picker_new,    NULL },
	{ "GnomeHRef",          href_new,           NULL },
	{ "GnomeEntry",         entry_new,          NULL },
	{ "GnomeFileEntry",     file_entry_new,     NULL },
	{ "GtkClock",           clock_new,          NULL },
	{ "GnomeLess",          less_new,           NULL },
	{ "GnomeCalculator",    calculator_new,     NULL },
	{ "GnomePaperSelector", paper_selector_new, NULL },
	{ "GnomeSpell",         spell_new,          NULL },
	{ "GtkDial",            dial_new,           NULL },
	{ "GnomeAbout",         about_new,          NULL },
	{ NULL, NULL, NULL }
};

void glade_init_gnome_widgets(void) {
	glade_register_widgets(widget_data);
}
