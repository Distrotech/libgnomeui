/* libglade - a library for building interfaces from XML files at runtime
 * Copyright (C) 1998, 1999  James Henstridge <james@daa.com.au>
 * Copyright (C) 1999 Miguel de Icaza (miguel@gnu.org)
 *
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

static const char * get_stock_name (const char *stock_name);

/* -- routines to build the children for containers -- */

static void
gnomedialog_build_children(GladeXML *xml, GtkWidget *w, GNode *node,
			    const char *longname)
{
	xmlNodePtr info;
	GNode *childnode;
	char *vboxname, *content;

	for (info = ((xmlNodePtr)node->children->data)->childs;
	     info; info = info->next)
		if (!strcmp(info->name, "name"))
			break;
	g_assert(info != NULL);

	content = xmlNodeGetContent(info);
	vboxname = g_strconcat(longname, ".", content, NULL);
	if (content) free(content);

	for (childnode = node->children->children; childnode;
	     childnode = childnode->next) {
		GtkWidget *child;
		xmlNodePtr xmlnode = childnode->data;
		gboolean expand = TRUE, fill = TRUE, start = TRUE;
		gint padding = 0;

		for (xmlnode = xmlnode->childs; xmlnode; xmlnode=xmlnode->next)
			if (!strcmp(xmlnode->name, "child_name"))
				break;
		content = xmlNodeGetContent(xmlnode);
		if (xmlnode && !strcmp(content, "GnomeDialog:action_area")) {
			char *parent_name;
			GNode *buttonnode;

			if (content) free(content);

			/* this is the action area -- here we add the buttons*/
			for (buttonnode = childnode->children; buttonnode;
			     buttonnode = buttonnode->next) {
				const char *stock;
				xmlnode = buttonnode->data;

				for (xmlnode = xmlnode->childs; xmlnode;
				     xmlnode = xmlnode->next)
					if (!strcmp(xmlnode->name,
						    "stock_button"))
						break;
				if (!xmlnode) continue;
				content = xmlNodeGetContent(xmlnode);
				stock = get_stock_name(content);
				if (!stock) stock = content;
				gnome_dialog_append_button(GNOME_DIALOG(w),
							   stock);
				if (content) free(content);
			}
			continue;
		}
		if (content) free(content);

		child = glade_xml_build_widget(xml, childnode, vboxname);
		for (xmlnode = ((xmlNodePtr)childnode->data)->childs;
		     xmlnode; xmlnode = xmlnode->next)
			if (!strcmp(xmlnode->name, "child"))
				break;
		if (!xmlnode) {
			gtk_box_pack_start_defaults(
				GTK_BOX(GNOME_DIALOG(w)->vbox), child);
			continue;
		}
		for (xmlnode=xmlnode->childs; xmlnode; xmlnode=xmlnode->next) {
			content = xmlNodeGetContent(xmlnode);
			switch(xmlnode->name[0]) {
			case 'e':
				if (!strcmp(xmlnode->name, "expand"))
					expand = content[0] == 'T';
				break;
			case 'f':
				if (!strcmp(xmlnode->name, "fill"))
					fill = content[0] == 'T';
				break;
			case 'p':
				if (!strcmp(xmlnode->name, "padding"))
					padding = strtol(content, NULL, 0);
				else if (!strcmp(xmlnode->name, "pack"))
					start = strcmp(content, "GTK_PACK_START");
				break;
			}
			if (content) free(content);
		}
		if (start)
			gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(w)->vbox),
					   child, expand, fill, padding);
		else
			gtk_box_pack_end(GTK_BOX(GNOME_DIALOG(w)->vbox),
					 child, expand, fill, padding);
	}
	g_free(vboxname);
}

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

static const char *
get_stock_name (const char *stock_name)
{
        const int len = strlen ("GNOME_STOCK_BUTTON_");
	gnome_map_t *v;
	gnome_map_t base;

        /* If an error happens, return this */
        if (strncmp (stock_name, "GNOME_STOCK_BUTTON_", len) != 0)
                return NULL;

        base.extension = stock_name + len;
        v = bsearch (
                &base,
                gnome_stock_button_mapping,
                sizeof(gnome_stock_button_mapping) / sizeof(gnome_map_t),
                sizeof (gnome_stock_button_mapping [0]),
                stock_compare);
        if (v)
                return v->mapping;
        else
                return NULL;
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
		const char *tmp = get_stock_name(stock);
		if (tmp)
			button = gnome_stock_button(tmp);
		else
			button = gtk_button_new_with_label(stock);
        } else
                button = gtk_button_new();

        if (stock)
                g_free (stock);
        if (string)
                g_free (string);
        return button;
}

static GtkWidget *
color_picker_new(GladeXML *xml, GNode *node)
{
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
font_picker_new(GladeXML *xml, GNode *node)
{
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
href_new(GladeXML *xml, GNode *node)
{
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
entry_new(GladeXML *xml, GNode *node)
{
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
file_entry_new(GladeXML *xml, GNode *node)
{
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
clock_new(GladeXML *xml, GNode *node)
{
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
less_new(GladeXML *xml, GNode *node)
{
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
calculator_new(GladeXML *xml, GNode *node)
{
	return gnome_calculator_new();
}

static GtkWidget *
paper_selector_new(GladeXML *xml, GNode *node)
{
	return gnome_paper_selector_new();
}

static GtkWidget *
spell_new(GladeXML *xml, GNode *node)
{
	return gnome_spell_new();
}

static GtkWidget *
dial_new(GladeXML *xml, GNode *node)
{
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

static GtkWidget *
gnomedialog_new(GladeXML *xml, GNode *node)
{
	GtkWidget *win;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	gint xpos = -1, ypos = -1;
	gboolean allow_shrink = TRUE, allow_grow = TRUE, auto_shrink = FALSE;
	gboolean auto_close = FALSE, hide_on_close = FALSE;
	gchar *title = NULL;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'a':
			if (!strcmp(info->name, "allow_grow"))
				allow_grow = content[0] == 'T';
			else if (!strcmp(info->name, "allow_shrink"))
				allow_shrink = content[0] == 'T';
			else if (!strcmp(info->name, "auto_shrink"))
				auto_shrink = content[0] == 'T';
			else if (!strcmp(info->name, "auto_close"))
				auto_close = content[0] == 'T';
			break;
		case 'h':
			if (!strcmp(info->name, "hide_on_close"))
				hide_on_close = content[0] == 'T';
		case 't':
			if (!strcmp(info->name, "title")) {
				if (title) g_free(title);
				title = g_strdup(content);
			}
			break;
		case 'x':
			if (info->name[1] == '\0') xpos = strtol(content, NULL, 0);
			break;
		case 'y':
			if (info->name[1] == '\0') ypos = strtol(content, NULL, 0);
			break;
		}

		if (content)
			free(content);
	}
	win = gnome_dialog_new(title, NULL);
	if (title) g_free(title);
	gtk_window_set_policy(GTK_WINDOW(win), allow_shrink, allow_grow,
			      auto_shrink);
	gnome_dialog_set_close(GNOME_DIALOG(win), auto_close);
	gnome_dialog_close_hides(GNOME_DIALOG(win), hide_on_close);
	if (xpos >= 0 || ypos >= 0)
		gtk_widget_set_uposition (win, xpos, ypos);
	return win;
}

static GtkWidget *
messagebox_new(GladeXML *xml, GNode *node)
{
	GtkWidget *win;
	GNode *child;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	gchar *typename = GNOME_MESSAGE_BOX_GENERIC, *message = NULL;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'm':
			if (!strcmp(info->name, "message")) {
				if (message) g_free(message);
				message = g_strdup(content);
			}
			break;
		case 't':
			if (!strcmp(info->name, "type")) {
				if (strncmp(content, "GNOME_MESSAGE_BOX_", 18))
					break;
				if (!strcmp(&content[18], "INFO"))
					typename = GNOME_MESSAGE_BOX_INFO;
				else if (!strcmp(&content[18], "WARNING"))
					typename = GNOME_MESSAGE_BOX_WARNING;
				else if (!strcmp(&content[18], "ERROR"))
					typename = GNOME_MESSAGE_BOX_ERROR;
				else if (!strcmp(&content[18], "QUESTION"))
					typename = GNOME_MESSAGE_BOX_QUESTION;
				else if (!strcmp(&content[18], "GENERIC"))
					typename = GNOME_MESSAGE_BOX_GENERIC;
			}
			break;
		}
		if (content) free(content);
	}
	/* create the message box with no buttons */
	win = gnome_message_box_new(message, typename, NULL);
	/* the message box contains a vbox which contains a hbuttonbox ... */
	child = node->children->children;
	/* the children of the hbuttonbox are the buttons ... */
	for (child = child->children; child; child = child->next) {
		const char *stock;
		char *content;

		info = child->data;
		for (info = info->childs; info; info = info->next)
			if (!strcmp(info->name, "stock_button"))
				break;
		if (!info) continue;
		content = xmlNodeGetContent(info);
		stock = get_stock_name(content);
		if (!stock) stock = content;
		gnome_dialog_append_button(GNOME_DIALOG(win), stock);
		if (content) free(content);
	}
	return win;
}
/* -- dialog box build routine goes here */

/* -- routines to initialise these widgets with libglade -- */

static const GladeWidgetBuildData widget_data [] = {
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
	{ "GnomeDialog",        gnomedialog_new,   gnomedialog_build_children},
	{ "GnomeMessageBox",    messagebox_new,     NULL },
	{ NULL, NULL, NULL }
};

void glade_init_gnome_widgets(void)
{
	glade_register_widgets(widget_data);
}
