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
#include <glade/glade.h>
#include <glade/glade-build.h>
#include <tree.h>

#include <gnome.h>

#ifndef ENABLE_NLS
/* a slight optimisation when gettext is off */
#define glade_xml_gettext(xml, msgid) (msgid)
#endif
#undef _
#define _(msgid) (glade_xml_gettext(xml, msgid))

static const char *get_stock_name (const char *stock_name);
static gboolean get_stock_uiinfo (const char *stock_name, GnomeUIInfo *info);

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

static void
app_build_children(GladeXML *xml, GtkWidget *w, GNode *node,
		   const char *longname)
{
	xmlNodePtr info;
	GNode *childnode;
	char *content;

	for (childnode = node->children; childnode;
	     childnode = childnode->next) {
		xmlNodePtr xmlnode = childnode->data;
		GtkWidget *child;

		for (xmlnode = xmlnode->childs; xmlnode; xmlnode=xmlnode->next)
			if (!strcmp(xmlnode->name, "child_name"))
				break;
		content = xmlNodeGetContent(xmlnode);
		if (xmlnode && !strcmp(content, "GnomeApp:dock")) {
			/* the dock has already been created */
			glade_xml_set_common_params(xml,
					GTK_WIDGET(gnome_app_get_dock(GNOME_APP(w))),
					childnode, longname, "GnomeDock");
		} else if (xmlnode && !strcmp(content, "GnomeApp:appbar")) {
			child = glade_xml_build_widget(xml,childnode,longname);
			gnome_app_set_statusbar(GNOME_APP(w), child);
		} else {
			child = glade_xml_build_widget(xml,childnode,longname);
			gtk_container_add(GTK_CONTAINER(w), child);
		}
		if (content) free(content);
	}
}

static void
dock_build_children(GladeXML *xml, GtkWidget *w, GNode *node,
		   const char *longname)
{
	xmlNodePtr info;
	GNode *childnode;
	char *content;

	for (childnode = node->children; childnode;
	     childnode = childnode->next) {
		xmlNodePtr xmlnode = childnode->data;
		GtkWidget *child;

		for (xmlnode = xmlnode->childs; xmlnode; xmlnode=xmlnode->next)
			if (!strcmp(xmlnode->name, "child_name"))
				break;
		content = xmlNodeGetContent(xmlnode);
		if (xmlnode && !strcmp(content, "GnomeDock:contents")) {
			child = glade_xml_build_widget(xml,childnode,longname);
			gnome_dock_set_client_area(GNOME_DOCK(w), child);
		} else {
			/* a dock item */
			GnomeDockPlacement placement = GNOME_DOCK_TOP;
			guint band = 0, offset = 0;
			gint position = 0;

			child = glade_xml_build_widget(xml,childnode,longname);
			if (content) free(content);
			xmlnode = childnode->data;
			for (xmlnode = xmlnode->childs; xmlnode;
			     xmlnode = xmlnode->next) {
				content = xmlNodeGetContent(xmlnode);
				if (!strcmp(xmlnode->name, "placement"))
					placement = glade_enum_from_string(
						GTK_TYPE_GNOME_DOCK_PLACEMENT,
						content);
				else if (!strcmp(xmlnode->name, "band"))
					band = strtoul(content, NULL, 0);
				else if (!strcmp(xmlnode->name, "position"))
					position = strtol(content, NULL, 0);
				else if (!strcmp(xmlnode->name, "offset"))
					offset = strtoul(content, NULL, 0);
				if (content) free(content);
			}
			content = NULL;
			gnome_dock_add_item(GNOME_DOCK(w),
					    GNOME_DOCK_ITEM(child), placement,
					    band, position, offset, FALSE);
		}
		if (content) free(content);
	}
}

static void
menuitem_build_children (GladeXML *xml, GtkWidget *w, GNode *node,
                         const char *longname)
{
        GtkWidget *menu;
        if (!node->children) return;
        menu = glade_xml_build_widget(xml, node->children, longname);
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(w), menu);
        gtk_widget_hide(menu); /* wierd things happen if menu is initially visible */
}

static void
toolbar_build_children (GladeXML *xml, GtkWidget *w, GNode *node,
			const char *longname)
{
	xmlNodePtr info;
	GNode *childnode;
	char *content;

	for (childnode = node->children; childnode;
	     childnode = childnode->next) {
		GtkWidget *child;
		xmlNodePtr xmlnode = childnode->data;

		/* insert a space into the toolbar if required */
		for (xmlnode = xmlnode->childs; xmlnode;
		     xmlnode = xmlnode->next)
			if (!strcmp(xmlnode->name, "child"))
				break;
		if (xmlnode) { /* the <child> node exists */
			for (xmlnode = xmlnode->childs; xmlnode;
			     xmlnode = xmlnode->next)
				if (!strcmp(xmlnode->name, "new_group"))
					break;
			if (xmlnode) {
				content = xmlNodeGetContent(xmlnode);
				if (content[0] == 'T')
					gtk_toolbar_append_space(
							GTK_TOOLBAR(w));
				free(content);
			}
		}
		
		/* check to see if this is a special Toolbar:button or just
		 * a standard widget we are adding to the toolbar */
		xmlnode = childnode->data;
		for (xmlnode = xmlnode->childs; xmlnode;
		     xmlnode = xmlnode->next)
			if (!strcmp(xmlnode->name, "child_name"))
				break;
		content = xmlNodeGetContent (xmlnode);
		if (xmlnode && !strcmp(content, "Toolbar:button")) {
			char *label = NULL, *icon = NULL, *stock = NULL;
			GtkWidget *iconw = NULL;

			if (content) free(content);
			xmlnode = childnode->data;
			for (xmlnode = xmlnode->childs; xmlnode;
			     xmlnode = xmlnode->next) {
				content = xmlNodeGetContent(xmlnode);
				if (!strcmp(xmlnode->name, "label")) {
					if (label) g_free(label);
					label = g_strdup(content);
				} else if (!strcmp(xmlnode->name, "icon")) {
					if (icon) g_free(icon);
					if (stock) g_free(stock);
					stock = NULL;
					icon = glade_xml_relative_file(xml,
								content);
				} else if (!strcmp(xmlnode->name, "stock_pixmap")) {
					if (icon) g_free(icon);
					if (stock) g_free(stock);
					icon = NULL;
					stock = g_strdup(content);
				}
				if (content) free(content);
			}
			if (stock) {
				iconw = gnome_stock_new_with_icon(
						get_stock_name(stock));
				g_free(stock);
			} else if (icon) {
				GdkPixmap *pix;
				GdkBitmap *mask;
				pix = gdk_pixmap_colormap_create_from_xpm(NULL,
					gtk_widget_get_colormap(w), &mask,
					NULL, icon);
				g_free(icon);
				iconw = gtk_pixmap_new(pix, mask);
				if (pix) gdk_pixmap_unref(pix);
				if (mask) gdk_bitmap_unref(mask);
			}
			child = gtk_toolbar_append_item(GTK_TOOLBAR(w),
							_(label), NULL, NULL,
							iconw, NULL, NULL);
			glade_xml_set_common_params(xml, child, childnode,
						    longname, "GtkButton");
		} else {
			if (content) free(content);
			child = glade_xml_build_widget(xml,childnode,longname);
			gtk_toolbar_append_widget(GTK_TOOLBAR(w), child,
						  NULL, NULL);
		}
	}
}

static void
menushell_build_children (GladeXML *xml, GtkWidget *w, GNode *node,
			  const char *longname)
{
	GNode *childnode;
	char *content;
	gint childnum = -1;
	GnomeUIInfo infos[2] = {
		{ GNOME_APP_UI_ITEM },
		GNOMEUIINFO_END
	};

	for (childnode = node->children; childnode;
	     childnode = childnode->next) {
		xmlNodePtr xmlnode = childnode->data;
		GtkWidget *child;
		gchar *tmp1 = NULL, *tmp2 = NULL;

		childnum++;
		for (xmlnode = xmlnode->childs; xmlnode;
		     xmlnode = xmlnode->next)
			if (!strcmp(xmlnode->name, "stock_item"))
				break;
		if (!xmlnode) {
			/* this is a normal menu item */
			child = glade_xml_build_widget(xml,childnode,longname);
			gtk_menu_shell_append(GTK_MENU_SHELL(w), child);
			continue;
		}
		content = xmlNodeGetContent(xmlnode);
		/* load the template GnomeUIInfo for this item */
		if (!get_stock_uiinfo(content, &infos[0])) {
			/* failure ... */
			char *tmp = content;
			if (!strncmp(tmp, "GNOMEUIINFO_", 12)) tmp += 12;
			child = gtk_menu_item_new_with_label(tmp);
			free(content);
			glade_xml_set_common_params(xml, child, childnode,
						    longname, "GtkMenuItem");
			gtk_menu_shell_append(GTK_MENU_SHELL(w), child);
			continue;
		}
		free(content);
		/* we now have the template for this item.  Now fill it in */
		xmlnode = childnode->data;
		for (xmlnode = xmlnode->childs; xmlnode;
		     xmlnode = xmlnode->next) {
			content = xmlNodeGetContent(xmlnode);
			if (!strcmp(xmlnode->name, "label")) {
				if (tmp1) g_free(tmp1);
				tmp1 = g_strdup(content);
				infos[0].label = _(tmp1);
			} else if (!strcmp(xmlnode->name, "tooltip")) {
				if (tmp2) g_free(tmp2);
				tmp2 = g_strdup(content);
				infos[0].hint = _(tmp2);
			}
			if (content) free(content);
		}
		gnome_app_fill_menu(GTK_MENU_SHELL(w), infos,
				    gtk_accel_group_get_default(), TRUE,
				    childnum);
		child = infos[0].widget;
		if (tmp1) g_free(tmp1);
		if (tmp2) g_free(tmp2);
		gtk_menu_item_remove_submenu(GTK_MENU_ITEM(child));
		glade_xml_set_common_params(xml, child, childnode,
					    longname, "GtkMenuItem");
	}
}

/* -- routines to build widgets -- */

/* this is for the GnomeStockButton widget ... */
/* Keep these sorted */
typedef struct {
        const char *extension;
        const char *mapping;
} gnome_map_t;

static const gnome_map_t gnome_stock_button_mapping [] = {
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
static const gnome_map_t gnome_stock_pixmap_mapping [] = {
	{ "ABOUT", GNOME_STOCK_PIXMAP_ABOUT },
	{ "ADD", GNOME_STOCK_PIXMAP_ADD },
	{ "ALIGN_CENTER", GNOME_STOCK_PIXMAP_ALIGN_CENTER },
	{ "ALIGN_JUSTIFY", GNOME_STOCK_PIXMAP_ALIGN_JUSTIFY },
	{ "ALIGN_LEFT", GNOME_STOCK_PIXMAP_ALIGN_LEFT },
	{ "ALIGN_RIGHT", GNOME_STOCK_PIXMAP_ALIGN_RIGHT },
	{ "ATTACH", GNOME_STOCK_PIXMAP_ATTACH },
	{ "BACK", GNOME_STOCK_PIXMAP_BACK },
	{ "BOOK_BLUE", GNOME_STOCK_PIXMAP_BOOK_BLUE },
	{ "BOOK_GREEN", GNOME_STOCK_PIXMAP_BOOK_GREEN },
	{ "BOOK_OPEN", GNOME_STOCK_PIXMAP_BOOK_OPEN },
	{ "BOOK_RED", GNOME_STOCK_PIXMAP_BOOK_RED },
	{ "BOOK_YELLOW", GNOME_STOCK_PIXMAP_BOOK_YELLOW },
	{ "BOTTOM", GNOME_STOCK_PIXMAP_BOTTOM },
	{ "CDROM", GNOME_STOCK_PIXMAP_CDROM },
	{ "CLEAR", GNOME_STOCK_PIXMAP_CLEAR },
	{ "CLOSE", GNOME_STOCK_PIXMAP_CLOSE },
	{ "COLORSELECTOR", GNOME_STOCK_PIXMAP_COLORSELECTOR },
	{ "CONVERT", GNOME_STOCK_PIXMAP_CONVERT },
	{ "COPY", GNOME_STOCK_PIXMAP_COPY },
	{ "CUT", GNOME_STOCK_PIXMAP_CUT },
	{ "DISABLED", GNOME_STOCK_PIXMAP_DISABLED },
	{ "DOWN", GNOME_STOCK_PIXMAP_DOWN },
	{ "EXEC", GNOME_STOCK_PIXMAP_EXEC },
	{ "FIRST", GNOME_STOCK_PIXMAP_FIRST },
	{ "FOCUSED", GNOME_STOCK_PIXMAP_FOCUSED },
	{ "FONT", GNOME_STOCK_PIXMAP_FONT },
	{ "FORWARD", GNOME_STOCK_PIXMAP_FORWARD },
	{ "HELP", GNOME_STOCK_PIXMAP_HELP },
	{ "HOME", GNOME_STOCK_PIXMAP_HOME },
	{ "INDEX", GNOME_STOCK_PIXMAP_INDEX },
	{ "JUMP_TO", GNOME_STOCK_PIXMAP_JUMP_TO },
	{ "LAST", GNOME_STOCK_PIXMAP_LAST },
	{ "LINE_IN", GNOME_STOCK_PIXMAP_LINE_IN },
	{ "MAIL", GNOME_STOCK_PIXMAP_MAIL },
	{ "MAIL_FWD", GNOME_STOCK_PIXMAP_MAIL_FWD },
	{ "MAIL_NEW", GNOME_STOCK_PIXMAP_MAIL_NEW },
	{ "MAIL_RCV", GNOME_STOCK_PIXMAP_MAIL_RCV },
	{ "MAIL_RPL", GNOME_STOCK_PIXMAP_MAIL_RPL },
	{ "MAIL_SND", GNOME_STOCK_PIXMAP_MAIL_SND },
	{ "MIC", GNOME_STOCK_PIXMAP_MIC },
	{ "MIDI", GNOME_STOCK_PIXMAP_MIDI },
	{ "MULTIPLE", GNOME_STOCK_PIXMAP_MULTIPLE },
	{ "NEW", GNOME_STOCK_PIXMAP_NEW },
	{ "NOT", GNOME_STOCK_PIXMAP_NOT },
	{ "OPEN", GNOME_STOCK_PIXMAP_OPEN },
	{ "PASTE", GNOME_STOCK_PIXMAP_PASTE },
	{ "PREFERENCES", GNOME_STOCK_PIXMAP_PREFERENCES },
	{ "PRINT", GNOME_STOCK_PIXMAP_PRINT },
	{ "PROPERTIES", GNOME_STOCK_PIXMAP_PROPERTIES },
	{ "QUIT", GNOME_STOCK_PIXMAP_QUIT },
	{ "QUIT", GNOME_STOCK_PIXMAP_QUIT },
	{ "REDO", GNOME_STOCK_PIXMAP_REDO },
	{ "REFRESH", GNOME_STOCK_PIXMAP_REFRESH },
	{ "REGULAR", GNOME_STOCK_PIXMAP_REGULAR },
	{ "REMOVE", GNOME_STOCK_PIXMAP_REMOVE },
	{ "REVERT", GNOME_STOCK_PIXMAP_REVERT },
	{ "SAVE", GNOME_STOCK_PIXMAP_SAVE },
	{ "SAVE_AS", GNOME_STOCK_PIXMAP_SAVE_AS },
	{ "SCORES", GNOME_STOCK_PIXMAP_SCORES },
	{ "SEARCH", GNOME_STOCK_PIXMAP_SEARCH },
	{ "SPELLCHECK", GNOME_STOCK_PIXMAP_SPELLCHECK },
	{ "SRCHRPL", GNOME_STOCK_PIXMAP_SRCHRPL },
	{ "STOP", GNOME_STOCK_PIXMAP_STOP },
	{ "TABLE_BORDERS", GNOME_STOCK_PIXMAP_TABLE_BORDERS },
	{ "TABLE_FILL", GNOME_STOCK_PIXMAP_TABLE_FILL },
	{ "TEXT_BOLD", GNOME_STOCK_PIXMAP_TEXT_BOLD },
	{ "TEXT_BULLETED_LIST", GNOME_STOCK_PIXMAP_TEXT_BULLETED_LIST },
	{ "TEXT_INDENT", GNOME_STOCK_PIXMAP_TEXT_INDENT },
	{ "TEXT_ITALIC", GNOME_STOCK_PIXMAP_TEXT_ITALIC },
	{ "TEXT_NUMBERED_LIST", GNOME_STOCK_PIXMAP_TEXT_NUMBERED_LIST },
	{ "TEXT_STRIKEOUT", GNOME_STOCK_PIXMAP_TEXT_STRIKEOUT },
	{ "TEXT_UNDERLINE", GNOME_STOCK_PIXMAP_TEXT_UNDERLINE },
	{ "TEXT_UNINDENT", GNOME_STOCK_PIXMAP_TEXT_UNINDENT },
	{ "TIMER", GNOME_STOCK_PIXMAP_TIMER },
	{ "TIMER_STOP", GNOME_STOCK_PIXMAP_TIMER_STOP },
	{ "TOP", GNOME_STOCK_PIXMAP_TOP },
	{ "TRASH", GNOME_STOCK_PIXMAP_TRASH },
	{ "TRASH_FULL", GNOME_STOCK_PIXMAP_TRASH_FULL },
	{ "UNDELETE", GNOME_STOCK_PIXMAP_UNDELETE },
	{ "UNDO", GNOME_STOCK_PIXMAP_UNDO },
	{ "UP", GNOME_STOCK_PIXMAP_UP },
	{ "VOLUME", GNOME_STOCK_PIXMAP_VOLUME },
};
static const gnome_map_t gnome_stock_menu_mapping [] = {
	{ "ABOUT", GNOME_STOCK_MENU_ABOUT },
	{ "ALIGN_CENTER", GNOME_STOCK_MENU_ALIGN_CENTER },
	{ "ALIGN_JUSTIFY", GNOME_STOCK_MENU_ALIGN_JUSTIFY },
	{ "ALIGN_LEFT", GNOME_STOCK_MENU_ALIGN_LEFT },
	{ "ALIGN_RIGHT", GNOME_STOCK_MENU_ALIGN_RIGHT },
	{ "ATTACH", GNOME_STOCK_MENU_ATTACH },
	{ "BACK", GNOME_STOCK_MENU_BACK },
	{ "BLANK", GNOME_STOCK_MENU_BLANK },
	{ "BOOK_BLUE", GNOME_STOCK_MENU_BOOK_BLUE },
	{ "BOOK_GREEN", GNOME_STOCK_MENU_BOOK_GREEN },
	{ "BOOK_OPEN", GNOME_STOCK_MENU_BOOK_OPEN },
	{ "BOOK_RED", GNOME_STOCK_MENU_BOOK_RED },
	{ "BOOK_YELLOW", GNOME_STOCK_MENU_BOOK_YELLOW },
	{ "BOTTOM", GNOME_STOCK_MENU_BOTTOM },
	{ "CDROM", GNOME_STOCK_MENU_CDROM },
	{ "CLOSE", GNOME_STOCK_MENU_CLOSE },
	{ "CONVERT", GNOME_STOCK_MENU_CONVERT },
	{ "COPY", GNOME_STOCK_MENU_COPY },
	{ "CUT", GNOME_STOCK_MENU_CUT },
	{ "DOWN", GNOME_STOCK_MENU_DOWN },
	{ "EXEC", GNOME_STOCK_MENU_EXEC },
	{ "FIRST", GNOME_STOCK_MENU_FIRST },
	{ "FONT", GNOME_STOCK_MENU_FONT },
	{ "FORWARD", GNOME_STOCK_MENU_FORWARD },
	{ "HOME", GNOME_STOCK_MENU_HOME },
	{ "INDEX", GNOME_STOCK_MENU_INDEX },
	{ "JUMP_TO", GNOME_STOCK_MENU_JUMP_TO },
	{ "LAST", GNOME_STOCK_MENU_LAST },
	{ "LINE_IN", GNOME_STOCK_MENU_LINE_IN },
	{ "MAIL", GNOME_STOCK_MENU_MAIL },
	{ "MAIL_FWD", GNOME_STOCK_MENU_MAIL_FWD },
	{ "MAIL_NEW", GNOME_STOCK_MENU_MAIL_NEW },
	{ "MAIL_RCV", GNOME_STOCK_MENU_MAIL_RCV },
	{ "MAIL_RPL", GNOME_STOCK_MENU_MAIL_RPL },
	{ "MAIL_SND", GNOME_STOCK_MENU_MAIL_SND },
	{ "MIC", GNOME_STOCK_MENU_MIC },
	{ "MIDI", GNOME_STOCK_MENU_MIDI },
	{ "NEW", GNOME_STOCK_MENU_NEW },
	{ "OPEN", GNOME_STOCK_MENU_OPEN },
	{ "PASTE", GNOME_STOCK_MENU_PASTE },
	{ "PREF", GNOME_STOCK_MENU_PREF },
	{ "PRINT", GNOME_STOCK_MENU_PRINT },
	{ "PROP", GNOME_STOCK_MENU_PROP },
	{ "QUIT", GNOME_STOCK_MENU_QUIT },
	{ "QUIT", GNOME_STOCK_MENU_QUIT },
	{ "REDO", GNOME_STOCK_MENU_REDO },
	{ "REFRESH", GNOME_STOCK_MENU_REFRESH },
	{ "REVERT", GNOME_STOCK_MENU_REVERT },
	{ "SAVE", GNOME_STOCK_MENU_SAVE },
	{ "SAVE_AS", GNOME_STOCK_MENU_SAVE_AS },
	{ "SCORES", GNOME_STOCK_MENU_SCORES },
	{ "SEARCH", GNOME_STOCK_MENU_SEARCH },
	{ "SPELLCHECK", GNOME_STOCK_MENU_SPELLCHECK },
	{ "SRCHRPL", GNOME_STOCK_MENU_SRCHRPL },
	{ "STOP", GNOME_STOCK_MENU_STOP },
	{ "TEXT_BOLD", GNOME_STOCK_MENU_TEXT_BOLD },
	{ "TEXT_ITALIC", GNOME_STOCK_MENU_TEXT_ITALIC },
	{ "TEXT_STRIKEOUT", GNOME_STOCK_MENU_TEXT_STRIKEOUT },
	{ "TEXT_UNDERLINE", GNOME_STOCK_MENU_TEXT_UNDERLINE },
	{ "TIMER", GNOME_STOCK_MENU_TIMER },
	{ "TIMER_STOP", GNOME_STOCK_MENU_TIMER_STOP },
	{ "TOP", GNOME_STOCK_MENU_TOP },
	{ "TRASH", GNOME_STOCK_MENU_TRASH },
	{ "TRASH_FULL", GNOME_STOCK_MENU_TRASH_FULL },
	{ "UNDELETE", GNOME_STOCK_MENU_UNDELETE },
	{ "UNDO", GNOME_STOCK_MENU_UNDO },
	{ "UP", GNOME_STOCK_MENU_UP },
	{ "VOLUME", GNOME_STOCK_MENU_VOLUME },
};

typedef struct {
        const char *extension;
        GnomeUIInfo data;
} gnomeuiinfo_map_t;

static GnomeUIInfo tmptree[] = {
	GNOMEUIINFO_END
};

static const gnomeuiinfo_map_t gnome_uiinfo_mapping[] = {
	{ "ABOUT_ITEM", GNOMEUIINFO_MENU_ABOUT_ITEM(NULL, NULL) },
	{ "CLEAR_ITEM", GNOMEUIINFO_MENU_CLEAR_ITEM(NULL, NULL) },
	{ "CLOSE_ITEM", GNOMEUIINFO_MENU_CLOSE_ITEM(NULL, NULL) },
	{ "CLOSE_WINDOW_ITEM", GNOMEUIINFO_MENU_CLOSE_WINDOW_ITEM(NULL,NULL) },
	{ "COPY_ITEM", GNOMEUIINFO_MENU_COPY_ITEM(NULL, NULL) },
	{ "CUT_ITEM", GNOMEUIINFO_MENU_CUT_ITEM(NULL, NULL) },
	{ "EDIT_TREE", GNOMEUIINFO_MENU_EDIT_TREE(tmptree) },
	{ "END_GAME_ITEM", GNOMEUIINFO_MENU_END_GAME_ITEM(NULL, NULL) },
	{ "EXIT_ITEM", GNOMEUIINFO_MENU_EXIT_ITEM(NULL, NULL) },
	{ "FILES_TREE", GNOMEUIINFO_MENU_FILES_TREE(tmptree) },
	{ "FILE_TREE", GNOMEUIINFO_MENU_FILE_TREE(tmptree) },
	{ "FIND_AGAIN_ITEM", GNOMEUIINFO_MENU_FIND_AGAIN_ITEM(NULL, NULL) },
	{ "FIND_ITEM", GNOMEUIINFO_MENU_FIND_ITEM(NULL, NULL) },
	{ "GAME_TREE", GNOMEUIINFO_MENU_GAME_TREE(tmptree) },
	{ "HELP_TREE", GNOMEUIINFO_MENU_HELP_TREE(tmptree) },
	{ "HINT_ITEM", GNOMEUIINFO_MENU_HINT_ITEM(NULL, NULL) },
	{ "NEW_GAME_ITEM", GNOMEUIINFO_MENU_NEW_GAME_ITEM(NULL, NULL) },
	{ "NEW_ITEM", GNOMEUIINFO_MENU_NEW_ITEM(NULL, NULL, NULL, NULL) },
	{ "NEW_WINDOW_ITEM", GNOMEUIINFO_MENU_NEW_WINDOW_ITEM(NULL, NULL) },
	{ "OPEN_ITEM", GNOMEUIINFO_MENU_OPEN_ITEM(NULL, NULL) },
	{ "PASTE_ITEM", GNOMEUIINFO_MENU_PASTE_ITEM(NULL, NULL) },
	{ "PAUSE_GAME_ITEM", GNOMEUIINFO_MENU_PAUSE_GAME_ITEM(NULL, NULL) },
	{ "PREFERENCES_ITEM", GNOMEUIINFO_MENU_PREFERENCES_ITEM(NULL, NULL) },
	{ "PRINT_ITEM", GNOMEUIINFO_MENU_PRINT_ITEM(NULL, NULL) },
	{ "PRINT_SETUP_ITEM", GNOMEUIINFO_MENU_PRINT_SETUP_ITEM(NULL, NULL) },
	{ "PROPERTIES_ITEM", GNOMEUIINFO_MENU_PROPERTIES_ITEM(NULL, NULL) },
	{ "REDO_ITEM", GNOMEUIINFO_MENU_REDO_ITEM(NULL, NULL) },
	{ "REDO_MOVE_ITEM", GNOMEUIINFO_MENU_REDO_MOVE_ITEM(NULL, NULL) },
	{ "REPLACE_ITEM", GNOMEUIINFO_MENU_REPLACE_ITEM(NULL, NULL) },
	{ "RESTART_GAME_ITEM", GNOMEUIINFO_MENU_RESTART_GAME_ITEM(NULL,NULL) },
	{ "REVERT_ITEM", GNOMEUIINFO_MENU_REVERT_ITEM(NULL, NULL) },
	{ "SAVE_AS_ITEM", GNOMEUIINFO_MENU_SAVE_AS_ITEM(NULL, NULL) },
	{ "SAVE_ITEM", GNOMEUIINFO_MENU_SAVE_ITEM(NULL, NULL) },
	{ "SCORES_ITEM", GNOMEUIINFO_MENU_SCORES_ITEM(NULL, NULL) },
	{ "SELECT_ALL_ITEM", GNOMEUIINFO_MENU_SELECT_ALL_ITEM(NULL, NULL) },
	{ "SETTINGS_TREE", GNOMEUIINFO_MENU_SETTINGS_TREE(tmptree) },
	{ "UNDO_ITEM", GNOMEUIINFO_MENU_UNDO_ITEM(NULL, NULL) },
	{ "UNDO_MOVE_ITEM", GNOMEUIINFO_MENU_UNDO_MOVE_ITEM(NULL, NULL) },
	{ "VIEW_TREE", GNOMEUIINFO_MENU_VIEW_TREE(tmptree) },
	{ "WINDOWS_TREE", GNOMEUIINFO_MENU_WINDOWS_TREE(tmptree) },
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
        const int blen = strlen ("GNOME_STOCK_BUTTON_");
	const int plen = strlen ("GNOME_STOCK_PIXMAP_");
	const int mlen = strlen ("GNOME_STOCK_MENU_");
	gnome_map_t *v;
	gnome_map_t base;

        /* If an error happens, return this */
        if (!strncmp (stock_name, "GNOME_STOCK_BUTTON_", blen)) {
		base.extension = stock_name + blen;
		v = bsearch (
			     &base,
			     gnome_stock_button_mapping,
			     sizeof(gnome_stock_button_mapping) /
			       sizeof(gnome_map_t),
			     sizeof (gnome_stock_button_mapping [0]),
			     stock_compare);
		if (v)
			return v->mapping;
		else
			return NULL;
	} else if (!strncmp (stock_name, "GNOME_STOCK_PIXMAP_", plen)) {
		base.extension = stock_name + plen;
		v = bsearch (
			     &base,
			     gnome_stock_pixmap_mapping,
			     sizeof(gnome_stock_pixmap_mapping) /
			       sizeof(gnome_map_t),
			     sizeof (gnome_stock_pixmap_mapping [0]),
			     stock_compare);
		if (v)
			return v->mapping;
		else
			return NULL;
	} else if (!strncmp (stock_name, "GNOME_STOCK_MENU_", mlen)) {
		base.extension = stock_name + mlen;
		v = bsearch (
			     &base,
			     gnome_stock_menu_mapping,
			     sizeof(gnome_stock_menu_mapping) /
			       sizeof(gnome_map_t),
			     sizeof (gnome_stock_menu_mapping [0]),
			     stock_compare);
		if (v)
			return v->mapping;
		else
			return NULL;
	}
	return NULL;
}

static gboolean
get_stock_uiinfo (const char *stock_name, GnomeUIInfo *info)
{
        const int len = strlen ("GNOMEUIINFO_MENU_");
	gnomeuiinfo_map_t *v;
	gnomeuiinfo_map_t base;

        /* If an error happens, return this */
        if (!strncmp (stock_name, "GNOMEUIINFO_MENU_", len)) {
		base.extension = stock_name + len;
		v = bsearch (
			     &base,
			     gnome_uiinfo_mapping,
			     sizeof(gnome_uiinfo_mapping) /
			     sizeof(gnomeuiinfo_map_t),
			     sizeof (gnome_uiinfo_mapping [0]),
			     stock_compare);
		if (v) {
			*info = v->data;
			return TRUE;
		} else
			return FALSE;
	}
	return FALSE;
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
                button = gtk_button_new_with_label(_(string));
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
						     _(content));
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
						    _(content));
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
	wid = gnome_href_new(url, _(label));
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
	wid = gnome_file_entry_new(history_id, _(title));
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
	gtk_clock_set_format(GTK_CLOCK(wid), _(format));
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
appbar_new(GladeXML *xml, GNode *node)
{
	GtkWidget *wid;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	gboolean has_progress = TRUE, has_status = TRUE;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "has_progress"))
			has_progress = content[0] == 'T';
		else if (!strcmp(info->name, "has_status"))
			has_status = content[0] == 'T';
		if (content) free(content);
	}
	wid = gnome_appbar_new(has_progress, has_status,
			       GNOME_PREFERENCES_USER);
	return wid;
}

static GtkWidget *
dock_new(GladeXML *xml, GNode *node)
{
	GtkWidget *wid = gnome_dock_new();
        xmlNodePtr info = ((xmlNodePtr)node->data)->childs;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "allow_floating"))
			gnome_dock_allow_floating_items(GNOME_DOCK(wid),
							content[0] == 'T');
		if (content) free(content);
	}

	return wid;
}

static GtkWidget *
dockitem_new(GladeXML *xml, GNode *node)
{
	GtkWidget *wid;
        xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	char *name = NULL;
	GnomeDockItemBehavior behaviour = GNOME_DOCK_ITEM_BEH_NORMAL;
	GtkShadowType shadow_type = GTK_SHADOW_OUT;
	
	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'e':
			if (!strcmp(info->name, "exclusive"))
				if (content[0] == 'T')
					behaviour |= GNOME_DOCK_ITEM_BEH_EXCLUSIVE;
			break;
		case 'l':
			if (!strcmp(info->name, "locked"))
				if (content[0] == 'T')
					behaviour |= GNOME_DOCK_ITEM_BEH_LOCKED;
			break;
		case 'n':
			if (!strcmp(info->name, "name")) {
				if (name) g_free(name);
				name = g_strdup(content);
			} else if (!strcmp(info->name, "never_floating"))
				if (content[0] == 'T')
					behaviour |= GNOME_DOCK_ITEM_BEH_NEVER_FLOATING;
			else if (!strcmp(info->name, "never_vertical"))
				if (content[0] == 'T')
					behaviour |= GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL;
			else if (!strcmp(info->name, "never_horizontal"))
				if (content[0] == 'T')
					behaviour |= GNOME_DOCK_ITEM_BEH_NEVER_HORIZONTAL;
			break;
		case 's':
			if (!strcmp(info->name, "shadow_type"))
				shadow_type = glade_enum_from_string(
						GTK_TYPE_SHADOW_TYPE, content);
			break;
		}
		if (content) free(content);
	}
	wid = gnome_dock_item_new(name, behaviour);
	gnome_dock_item_set_shadow_type(GNOME_DOCK_ITEM(wid), shadow_type);
	return wid;
}

static GtkWidget *
menubar_new(GladeXML *xml, GNode *node)
{
	return gtk_menu_bar_new();
}

static GtkWidget *
menu_new(GladeXML *xml, GNode *node)
{
	return gtk_menu_new();
}

static GtkWidget *
pixmapmenuitem_new(GladeXML *xml, GNode *node)
{
	/* This needs to be rewritten to handle the GNOMEUIINFO_* macros.
	 * For now, we have something that is useable, but not perfect */
	GtkWidget *wid;
        xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	char *label = NULL;
	const char *stock_icon = NULL;
	gboolean right = FALSE;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		if (!strcmp(info->name, "label")) {
			if (label) g_free(label);
			label = g_strdup(content);
		} else if (!strcmp(info->name, "stock_icon")) {
			stock_icon = get_stock_name(content);
		} else if (!strcmp(info->name, "right_justify"))
			right = content[0] == 'T';
		if (content) free(content);
	}
	wid = gtk_pixmap_menu_item_new();
	if (label) {
		GtkWidget *lwid = gtk_label_new(_(label));
		gtk_container_add(GTK_CONTAINER(wid), lwid);
		gtk_widget_show(lwid);
		g_free(label);
	}
	if (stock_icon) {
		GtkWidget *iconw = gnome_stock_new_with_icon(stock_icon);
		gtk_pixmap_menu_item_set_pixmap(GTK_PIXMAP_MENU_ITEM(wid),
						iconw);
		gtk_widget_show(iconw);
	}
	if (right)
		gtk_menu_item_right_justify(GTK_MENU_ITEM(wid));
	return wid;
}

static GtkWidget *
toolbar_new(GladeXML *xml, GNode *node)
{
	GtkWidget *tool;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	GtkOrientation orient = GTK_ORIENTATION_HORIZONTAL;
	GtkToolbarStyle style = GTK_TOOLBAR_BOTH;
	GtkToolbarSpaceStyle spaces = GTK_TOOLBAR_SPACE_EMPTY;
	int space_size = 5;
	gboolean tooltips = TRUE;

	for (; info; info = info->next) {
		char *content = xmlNodeGetContent(info);

		switch (info->name[0]) {
		case 'o':
			if (!strcmp(info->name, "orientation"))
				orient = glade_enum_from_string(GTK_TYPE_ORIENTATION,
								content);
			break;
		case 's':
			if (!strcmp(info->name, "space_size"))
				space_size = strtol(content, NULL, 0);
			else if (!strcmp(info->name, "space_style"))
				spaces = glade_enum_from_string(
					GTK_TYPE_TOOLBAR_SPACE_STYLE, content);
			break;
		case 't':
			if (!strcmp(info->name, "type"))
				style = glade_enum_from_string(
					GTK_TYPE_TOOLBAR_STYLE, content);
			else if (!strcmp(info->name, "tooltips"))
				tooltips = content[0] == 'T';
			break;
		}

		if (content)
			free(content);
	}
	tool = gtk_toolbar_new(orient, style);
	gtk_toolbar_set_space_size(GTK_TOOLBAR(tool), space_size);
	gtk_toolbar_set_space_style(GTK_TOOLBAR(tool), spaces);
	gtk_toolbar_set_tooltips(GTK_TOOLBAR(tool), tooltips);
	return tool;
}

static GtkWidget *
about_new(GladeXML *xml, GNode *node)
{
	GtkWidget *wid;
        xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	gchar *title = gnome_app_id, *version = gnome_app_version;
	gchar *copyright = NULL;
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
	wid = gnome_about_new(title, version, _(copyright),
			      (const gchar **)authors, _(comments), logo);
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
	win = gnome_dialog_new(_(title), NULL);
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
	win = gnome_message_box_new(_(message), typename, NULL);
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

static GtkWidget *
app_new(GladeXML *xml, GNode *node)
{
	GtkWidget *win;
	xmlNodePtr info = ((xmlNodePtr)node->data)->childs;
	gchar *title = NULL;
	gboolean enable_layout = TRUE;
	gboolean allow_shrink = TRUE, allow_grow = TRUE, auto_shrink = FALSE;

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
			break;
		case 'e':
			if (!strcmp(info->name, "enable_layout_config"))
				enable_layout = content[0] == 'T';
			break;
		case 't':
			if (!strcmp(info->name, "title")) {
				if (title) g_free(title);
				title = g_strdup(content);
			}
			break;
		}
		if (content) free(content);
	}
	win = gnome_app_new(gnome_app_id, _(title));
	if (title) g_free(title);
	gtk_window_set_policy(GTK_WINDOW(win), allow_shrink, allow_grow,
			      auto_shrink);
	gnome_app_enable_layout_config(GNOME_APP(win), enable_layout);
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
	{ "GnomeAppBar",        appbar_new,         NULL },

	{ "GnomeDock",          dock_new,           dock_build_children},
	{ "GnomeDockItem",      dockitem_new,   glade_standard_build_children},
	{ "GtkToolbar",         toolbar_new,        toolbar_build_children},
	{ "GtkMenuBar",         menubar_new,        menushell_build_children},
	{ "GtkMenu",            menu_new,           menushell_build_children},
	{ "GtkPixmapMenuItem",  pixmapmenuitem_new, menuitem_build_children},

	{ "GnomeAbout",         about_new,          NULL },
	{ "GnomeDialog",        gnomedialog_new,   gnomedialog_build_children},
	{ "GnomeMessageBox",    messagebox_new,     NULL },
	{ "GnomeApp",           app_new,            app_build_children},
	{ NULL, NULL, NULL }
};

void glade_init_gnome_widgets(void)
{
	glade_register_widgets(widget_data);
}

/**
 * glade_gnome_init
 *
 * This function performs initialisation of glade, similar to what glade_init
 * does (in fact it calls glade_init for you).  The difference is that it
 * also initialises the GNOME widget building routines.
 *
 * As well as calling this initialisation function, GNOME programs should
 * also link with the libglade-gnome library, which contains all the
 * GNOME libglade stuff.
 */
void glade_gnome_init(void)
{
	static gboolean initialised = FALSE;

	if (initialised) return;
	initialised = TRUE;
	glade_init();
	glade_init_gnome_widgets();
}
