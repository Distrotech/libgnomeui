/* -*- Mode: C; c-basic-offset: 8 -*- */
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

#include <stdlib.h>
#include <string.h>
#include <glade/glade.h>
#include <glade/glade-build.h>

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
gnomedialog_build_children(GladeXML *xml, GtkWidget *w, GladeWidgetInfo *info,
			    const char *longname)
{
	GList *tmp;
	char *vboxname;

	vboxname = g_strconcat (longname, ".", info->name, NULL);

	/* all dialog children are inside the main vbox */
	for (tmp = ((GladeWidgetInfo *)info->children->data)->children;
	     tmp; tmp = tmp->next) {
		GladeWidgetInfo *cinfo = tmp->data;
		GtkWidget *child;
		GList *tmp2;
		gboolean is_action_area = FALSE;
		gboolean expand = TRUE, fill = TRUE, start = TRUE;
		gint padding = 0;

		for (tmp2 = cinfo->attributes; tmp2; tmp2 = tmp2->next) {
			GladeAttribute *attr = tmp2->data;
			if (!strcmp(attr->name, "child_name") &&
			    !strcmp(attr->value, "Dialog:action_area")) {
				is_action_area = TRUE;
				break;
			}
		}

		if (is_action_area) {
			char *parent_name;

			parent_name = g_strconcat(vboxname, ".", cinfo->name,
						  NULL);

			/* this is the action area -- here we add the buttons*/
			for (tmp2 = cinfo->children; tmp2; tmp2 = tmp2->next) {
				GladeWidgetInfo *ccinfo = tmp2->data;
				const char *stock, *string = NULL;
				GList *tmp3;

				for (tmp3 = ccinfo->attributes;
				     tmp3; tmp3 = tmp3->next) {
					GladeAttribute *attr = tmp3->data;
					if (!strcmp(attr->name,
						    "stock_button")) {
						string = attr->value;
						break;
					}
				}
				stock = get_stock_name(string);
				if (!stock) stock = string;
				gnome_dialog_append_button(GNOME_DIALOG(w),
							   stock);
				/* connect signal handlers, etc ... */
				child = g_list_last(
					GNOME_DIALOG(w)->buttons)->data;
				glade_xml_set_common_params(xml, child,
							    ccinfo,
							    parent_name);
			}
			g_free(parent_name);
			continue;
		}

		child = glade_xml_build_widget(xml, cinfo, vboxname);
		for (tmp2 = cinfo->child_attributes; tmp2; tmp2 = tmp2->next) {
			GladeAttribute *attr = tmp2->data;

			switch(attr->name[0]) {
			case 'e':
				if (!strcmp(attr->name, "expand"))
					expand = attr->value[0] == 'T';
				break;
			case 'f':
				if (!strcmp(attr->name, "fill"))
					fill = attr->value[0] == 'T';
				break;
			case 'p':
				if (!strcmp(attr->name, "padding"))
					padding = strtol(attr->value, NULL, 0);
				else if (!strcmp(attr->name, "pack"))
					start = strcmp(attr->value,
						       "GTK_PACK_START");
				break;
			}
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
messagebox_build_children(GladeXML *xml, GtkWidget *w, GladeWidgetInfo *info,
			    const char *longname)
{
	GList *tmp;
	GladeWidgetInfo *cinfo;
	GtkWidget *child;

	/* the message box contains a vbox which contains a hbuttonbox ... */
	cinfo = ((GladeWidgetInfo *)info->children->data)->children->data;
	/* the children of the hbuttonbox are the buttons ... */
	for (tmp = cinfo->children; tmp; tmp = tmp->next) {
		GList *tmp2;
		const char *stock, *string = NULL;

		cinfo = tmp->data;
		for (tmp2 = cinfo->attributes; tmp2; tmp2 = tmp2->next) {
			GladeAttribute *attr = tmp2->data;
			if (!strcmp(attr->name, "stock_button")) {
				string = attr->value;
				break;
			}
		}
		stock = get_stock_name(string);
		if (!stock) stock = string;
		gnome_dialog_append_button(GNOME_DIALOG(w), stock);
		child = g_list_last(GNOME_DIALOG(w)->buttons)->data;
		glade_xml_set_common_params(xml, child, cinfo,
					    longname);
	}
}

static void
app_build_children(GladeXML *xml, GtkWidget *w, GladeWidgetInfo *info,
		   const char *longname)
{
	GList *tmp;

	for (tmp = info->children; tmp; tmp = tmp->next) {
		GladeWidgetInfo *cinfo = tmp->data;
		GList *tmp2;
		gboolean is_dock = FALSE, is_appbar = FALSE;
		GtkWidget *child;

		for (tmp2 = cinfo->attributes; tmp2; tmp2 = tmp2->next) {
			GladeAttribute *attr = tmp2->data;
			if (!strcmp(attr->name, "child_name")) {
				is_dock   = !strcmp(attr->value,
						    "GnomeApp:dock");
				is_appbar = !strcmp(attr->value,
						    "GnomeApp:appbar");
				break;
			}
		}
		if (is_dock) {
			/* the dock has already been created */
			glade_xml_set_common_params(xml,
				GTK_WIDGET(gnome_app_get_dock(GNOME_APP(w))),
				cinfo, longname);
		} else if (is_appbar) {
			child = glade_xml_build_widget(xml, cinfo, longname);
			gnome_app_set_statusbar(GNOME_APP(w), child);
		} else {
			child = glade_xml_build_widget(xml, cinfo, longname);
			gtk_container_add(GTK_CONTAINER(w), child);
		}
	}
}

static void
dock_build_children(GladeXML *xml, GtkWidget *w, GladeWidgetInfo *info,
		   const char *longname)
{
	GList *tmp;

	for (tmp = info->children; tmp; tmp = tmp->next) {
		GladeWidgetInfo *cinfo = tmp->data;
		GList *tmp2;
		gboolean is_contents = FALSE;
		GtkWidget *child;

		for (tmp2 = cinfo->attributes; tmp2; tmp2 = tmp2->next) {
			GladeAttribute *attr = tmp2->data;
			if (!strcmp(attr->name, "child_name")) {
				is_contents = !strcmp(attr->value,
						      "GnomeDock:contents");
				break;
			}
		}
		if (is_contents) {
			child = glade_xml_build_widget(xml, cinfo, longname);
			gnome_dock_set_client_area(GNOME_DOCK(w), child);
		} else {
			/* a dock item */
			GnomeDockPlacement placement = GNOME_DOCK_TOP;
			guint band = 0, offset = 0;
			gint position = 0;

			child = glade_xml_build_widget(xml, cinfo, longname);
			for (tmp2 = cinfo->attributes; tmp2; tmp2=tmp2->next) {
				GladeAttribute *attr = tmp2->data;

				if (!strcmp(attr->name, "placement"))
					placement = glade_enum_from_string(
						GTK_TYPE_GNOME_DOCK_PLACEMENT,
						attr->value);
				else if (!strcmp(attr->name, "band"))
					band = strtoul(attr->value, NULL, 0);
				else if (!strcmp(attr->name, "position"))
					position = strtol(attr->value, NULL,0);
				else if (!strcmp(attr->name, "offset"))
					offset = strtoul(attr->value, NULL, 0);
			}
			gnome_dock_add_item(GNOME_DOCK(w),
					    GNOME_DOCK_ITEM(child), placement,
					    band, position, offset, FALSE);
		}
	}
}

static void
menuitem_build_children (GladeXML *xml, GtkWidget *w, GladeWidgetInfo *info,
                         const char *longname)
{
        GtkWidget *menu;
        if (!info->children) return;
        menu = glade_xml_build_widget(xml, info->children->data, longname);
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(w), menu);
	/* weird things happen if menu is initially visible */
        gtk_widget_hide(menu);
}

static void
toolbar_build_children (GladeXML *xml, GtkWidget *w, GladeWidgetInfo *info,
			const char *longname)
{
	GList *tmp;

	for (tmp = info->children; tmp; tmp = tmp->next) {
		GladeWidgetInfo *cinfo = tmp->data;
		GtkWidget *child;
		GList *tmp2;
		gboolean is_button = FALSE;

		for (tmp2 = cinfo->child_attributes; tmp2; tmp2 = tmp2->next) {
			GladeAttribute *attr = tmp2->data;
			if (!strcmp(attr->name, "new_group") &&
			    attr->value[0] == 'T')
				gtk_toolbar_append_space(GTK_TOOLBAR(w));
		}
		
		/* check to see if this is a special Toolbar:button or just
		 * a standard widget we are adding to the toolbar */
		for (tmp2 = cinfo->attributes; tmp2; tmp2 = tmp2->next) {
			GladeAttribute *attr = tmp2->data;
			if (!strcmp(attr->name, "child_name")) {
				is_button = !strcmp(attr->value,
						    "Toolbar:button");
				break;
			}
		}
		if (is_button) {
			char *label = NULL, *icon = NULL, *stock = NULL;
			GtkWidget *iconw = NULL;

			for (tmp2 = cinfo->attributes; tmp2; tmp2=tmp2->next) {
				GladeAttribute *attr = tmp2->data;

				if (!strcmp(attr->name, "label")) {
					label = attr->value;
				} else if (!strcmp(attr->name, "icon")) {
					if (icon) g_free(icon);
					stock = NULL;
					icon = glade_xml_relative_file(xml,
								attr->value);
				} else if (!strcmp(attr->name,"stock_pixmap")){
					if (icon) g_free(icon);
					icon = NULL;
					stock = attr->value;
				}
			}
			if (stock) {
				iconw = gnome_stock_new_with_icon(
						get_stock_name(stock));
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
			glade_xml_set_common_params(xml, child, cinfo,
						    longname);
		} else {
			child = glade_xml_build_widget(xml, cinfo, longname);
			gtk_toolbar_append_widget(GTK_TOOLBAR(w), child,
						  NULL, NULL);
		}
	}
}

static void
menushell_build_children (GladeXML *xml, GtkWidget *w, GladeWidgetInfo *info,
			  const char *longname)
{
	GList *tmp;
	gint childnum = -1;
	GnomeUIInfo infos[2] = {
		{ GNOME_APP_UI_ITEM },
		GNOMEUIINFO_END
	};
	GtkAccelGroup *uline = NULL;

	if (strcmp(info->class, "GtkMenuBar") != 0) {
		uline = gtk_menu_ensure_uline_accel_group(GTK_MENU(w));
		glade_xml_push_uline_accel(xml, uline);
	}

	for (tmp = info->children; tmp; tmp = tmp->next) {
		GladeWidgetInfo *cinfo = tmp->data;
		GList *tmp2;
		GtkWidget *child;
		gchar *stock_name = NULL;

		childnum++;
		for (tmp2 = cinfo->attributes; tmp2; tmp2 = tmp2->next) {
			GladeAttribute *attr = tmp2->data;

			if (!strcmp(attr->name, "stock_item")) {
				stock_name = attr->value;
				break;
			}
		}
		if (!stock_name) {
			/* this is a normal menu item */
			child = glade_xml_build_widget(xml, cinfo, longname);
			gtk_menu_shell_append(GTK_MENU_SHELL(w), child);
			continue;
		}
		/* load the template GnomeUIInfo for this item */
		if (!get_stock_uiinfo(stock_name, &infos[0])) {
			/* failure ... */
			if (!strncmp(stock_name, "GNOMEUIINFO_", 12))
				stock_name += 12;
			child = gtk_menu_item_new_with_label(stock_name);
			glade_xml_set_common_params(xml, child, cinfo,
						    longname);
			gtk_menu_shell_append(GTK_MENU_SHELL(w), child);
			continue;
		}
		/* we now have the template for this item.  Now fill it in */
		for (tmp2 = cinfo->attributes; tmp2; tmp2 = tmp2->next) {
			GladeAttribute *attr = tmp2->data;

			if (!strcmp(attr->name, "label"))
				infos[0].label = _(attr->value);
			else if (!strcmp(attr->name, "tooltip"))
				infos[0].hint = _(attr->value);
		}
		gnome_app_fill_menu(GTK_MENU_SHELL(w), infos,
				    glade_xml_ensure_accel(xml), TRUE,
				    childnum);
		child = infos[0].widget;
		gtk_menu_item_remove_submenu(GTK_MENU_ITEM(child));
		glade_xml_set_common_params(xml, child, cinfo,
					    longname);
	}
	if (uline)
		glade_xml_pop_uline_accel(xml);
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
#ifdef GNOME_STOCK_PIXMAP_MIDI
	{ "MIDI", GNOME_STOCK_PIXMAP_MIDI },
#endif
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
#ifdef GNOME_STOCK_MENU_MIDI
	{ "MIDI", GNOME_STOCK_MENU_MIDI },
#endif
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
stock_button_new(GladeXML *xml, GladeWidgetInfo *info)
{
        GtkWidget *button;
	GList *tmp;
        char *string = NULL;
        char *stock = NULL;
  
        /*
         * This should really be a container, but GLADE is weird in this
	 * respect.
         * If the label property is set for this widget, insert a label.
         * Otherwise, allow a widget to be packed
         */
        for (tmp = info->attributes; tmp; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;

                if (!strcmp(attr->name, "label")) {
                        string = attr->value;
                        stock = NULL;
                } else if (!strcmp(attr->name, "stock_button")) {
                        stock = attr->value;
                        string = NULL;
                }
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

        return button;
}

static GtkWidget *
color_picker_new(GladeXML *xml, GladeWidgetInfo *info)
{
	GtkWidget *wid = gnome_color_picker_new();
	GList *tmp;

        for (tmp = info->attributes; tmp; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;
		
		if (!strcmp(attr->name, "dither"))
			gnome_color_picker_set_dither(GNOME_COLOR_PICKER(wid),
						      attr->value[0] == 'T');
		else if (!strcmp(attr->name, "use_alpha"))
			gnome_color_picker_set_use_alpha(
				GNOME_COLOR_PICKER(wid), attr->value[0]=='T');
		else if (!strcmp(attr->name, "title"))
			gnome_color_picker_set_title(GNOME_COLOR_PICKER(wid),
						     _(attr->value));
	}
	return wid;
}

static GtkWidget *
font_picker_new(GladeXML *xml, GladeWidgetInfo *info)
{
	GtkWidget *wid = gnome_font_picker_new();
	GList *tmp;
	gboolean use_font = FALSE;
	gint use_font_size = 14;

        for (tmp = info->attributes; tmp; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;
		
		if (!strcmp(attr->name, "title"))
			gnome_font_picker_set_title(GNOME_FONT_PICKER(wid),
						    _(attr->value));
		else if (!strcmp(attr->name, "preview_text"))
			gnome_font_picker_set_preview_text(
				GNOME_FONT_PICKER(wid), attr->value);
		else if (!strcmp(attr->name, "mode"))
			gnome_font_picker_set_mode(GNOME_FONT_PICKER(wid),
				glade_enum_from_string(
					GTK_TYPE_GNOME_FONT_PICKER_MODE,
					attr->value));
		else if (!strcmp(attr->name, "show_size"))
			gnome_font_picker_fi_set_show_size(
				GNOME_FONT_PICKER(wid), attr->value[0] == 'T');
		else if (!strcmp(attr->name, "use_font"))
			use_font = (attr->value[0] == 'T');
		else if (!strcmp(attr->name, "use_font_size"))
			use_font_size = strtol(attr->value, NULL, 0);
	}
	gnome_font_picker_fi_set_use_font_in_label(GNOME_FONT_PICKER(wid),
						   use_font,
						   use_font_size);
	return wid;
}

/* -- does not look like this widget is fully coded yet
static GtkWidget *
icon_entry_new(GladeXML *xml, GladeWidgetInfo *info)
{
	GtkWidget *wid = gnome_color_picker_new();

}
*/

static GtkWidget *
href_new(GladeXML *xml, GladeWidgetInfo *info)
{
	GtkWidget *wid;
	GList *tmp;
	gchar *url = NULL, *label = NULL;

        for (tmp = info->attributes; tmp; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;
		
		if (!strcmp(attr->name, "url"))
			url = attr->value;
		else if (!strcmp(attr->name, "label"))
			label = attr->value;
	}
	wid = gnome_href_new(url, _(label));
	return wid;
}

static GtkWidget *
entry_new(GladeXML *xml, GladeWidgetInfo *info)
{
	GtkWidget *wid;
	GList *tmp;
	gchar *history_id = NULL;
	gint max_saved = 10;

        for (tmp = info->attributes; tmp; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;
		
		if (!strcmp(attr->name, "history_id"))
			history_id = attr->value;
		else if (!strcmp(attr->name, "max_saved"))
			max_saved = strtol(attr->value, NULL, 0);
	}
	wid = gnome_entry_new(history_id);
	gnome_entry_set_max_saved(GNOME_ENTRY(wid), max_saved);
	return wid;
}

static GtkWidget *
file_entry_new(GladeXML *xml, GladeWidgetInfo *info)
{
	GtkWidget *wid;
	GList *tmp;
	gchar *history_id = NULL, *title = NULL;
	gboolean directory = FALSE, modal = FALSE;

        for (tmp = info->attributes; tmp; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;
		
		if (!strcmp(attr->name, "history_id"))
			history_id = attr->value;
		else if (!strcmp(attr->name, "title"))
			title = attr->value;
		else if (!strcmp(attr->name, "directory"))
			directory = (attr->value[0] == 'T');
		else if (!strcmp(attr->name, "modal"))
			modal = (attr->value[0] == 'T');
	}
	wid = gnome_file_entry_new(history_id, _(title));
	gnome_file_entry_set_directory(GNOME_FILE_ENTRY(wid), directory);
	gnome_file_entry_set_modal(GNOME_FILE_ENTRY(wid), modal);
	return wid;
}

static GtkWidget *
clock_new(GladeXML *xml, GladeWidgetInfo *info)
{
	GtkWidget *wid;
	GList *tmp;
	GtkClockType ctype = GTK_CLOCK_REALTIME;
	gchar *format = NULL;
	time_t seconds = 0;
	gint interval = 60;

        for (tmp = info->attributes; tmp; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;
		
		if (!strcmp(attr->name, "type"))
			ctype = glade_enum_from_string(GTK_TYPE_CLOCK_TYPE,
						       attr->value);
		else if (!strcmp(attr->name, "format"))
			format = attr->value;
		else if (!strcmp(attr->name, "seconds"))
			seconds = strtol(attr->value, NULL, 0);
		else if (!strcmp(attr->name, "interval"))
			interval = strtol(attr->value, NULL, 0);
	}
	wid = gtk_clock_new(ctype);
	gtk_clock_set_format(GTK_CLOCK(wid), _(format));
	gtk_clock_set_seconds(GTK_CLOCK(wid), seconds);
	gtk_clock_set_update_interval(GTK_CLOCK(wid), interval);
	return wid;
}

/* -- GnomeAnimator not finished */

static GtkWidget *
less_new(GladeXML *xml, GladeWidgetInfo *info)
{
	GtkWidget *wid = gnome_less_new();
	GList *tmp;

        for (tmp = info->attributes; tmp; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;
		
		if (!strcmp(attr->name, "font"))
			gnome_less_set_font(GNOME_LESS(wid),
					    gdk_font_load(attr->value));
	}
	return wid;
}

static GtkWidget *
calculator_new(GladeXML *xml, GladeWidgetInfo *info)
{
	return gnome_calculator_new();
}

static GtkWidget *
paper_selector_new(GladeXML *xml, GladeWidgetInfo *info)
{
	return gnome_paper_selector_new();
}

static GtkWidget *
spell_new(GladeXML *xml, GladeWidgetInfo *info)
{
	return gnome_spell_new();
}

static GtkWidget *
dial_new(GladeXML *xml, GladeWidgetInfo *info)
{
	GtkWidget *wid = gtk_dial_new(glade_get_adjustment(info));
	GList *tmp;

        for (tmp = info->attributes; tmp; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;

		if (!strcmp(attr->name, "view_only"))
			gtk_dial_set_view_only(GTK_DIAL(wid),
					       attr->value[0] == 'T');
		else if (!strcmp(attr->name, "update_policy"))
			gtk_dial_set_update_policy(GTK_DIAL(wid),
				glade_enum_from_string(GTK_TYPE_UPDATE_TYPE,
						       attr->value));
	}
	return wid;
}

static GtkWidget *
appbar_new(GladeXML *xml, GladeWidgetInfo *info)
{
	GtkWidget *wid;
	GList *tmp;
	gboolean has_progress = TRUE, has_status = TRUE;

	for (tmp = info->attributes; tmp; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;

		if (!strcmp(attr->name, "has_progress"))
			has_progress = attr->value[0] == 'T';
		else if (!strcmp(attr->name, "has_status"))
			has_status = attr->value[0] == 'T';
	}
	wid = gnome_appbar_new(has_progress, has_status,
			       GNOME_PREFERENCES_USER);
	return wid;
}

static GtkWidget *
dock_new(GladeXML *xml, GladeWidgetInfo *info)
{
	GtkWidget *wid = gnome_dock_new();
	GList *tmp;

	for (tmp = info->attributes; tmp; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;

		if (!strcmp(attr->name, "allow_floating"))
			gnome_dock_allow_floating_items(GNOME_DOCK(wid),
							attr->value[0] == 'T');
	}

	return wid;
}

static GtkWidget *
dockitem_new(GladeXML *xml, GladeWidgetInfo *info)
{
	GtkWidget *wid;
	GList *tmp;
	char *name = NULL;
	GnomeDockItemBehavior behaviour = GNOME_DOCK_ITEM_BEH_NORMAL;
	GtkShadowType shadow_type = GTK_SHADOW_OUT;
	
	for (tmp = info->attributes; tmp; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;

		switch (attr->name[0]) {
		case 'e':
			if (!strcmp(attr->name, "exclusive"))
				if (attr->value[0] == 'T')
					behaviour |= GNOME_DOCK_ITEM_BEH_EXCLUSIVE;
			break;
		case 'l':
			if (!strcmp(attr->name, "locked"))
				if (attr->value[0] == 'T')
					behaviour |= GNOME_DOCK_ITEM_BEH_LOCKED;
			break;
		case 'n':
			if (!strcmp(attr->name, "name"))
				name = attr->value;
			else if (!strcmp(attr->name, "never_floating"))
				if (attr->value[0] == 'T')
					behaviour |= GNOME_DOCK_ITEM_BEH_NEVER_FLOATING;
			else if (!strcmp(attr->name, "never_vertical"))
				if (attr->value[0] == 'T')
					behaviour |= GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL;
			else if (!strcmp(attr->name, "never_horizontal"))
				if (attr->value[0] == 'T')
					behaviour |= GNOME_DOCK_ITEM_BEH_NEVER_HORIZONTAL;
			break;
		case 's':
			if (!strcmp(attr->name, "shadow_type"))
				shadow_type = glade_enum_from_string(
						GTK_TYPE_SHADOW_TYPE,
						attr->value);
			break;
		}
	}
	wid = gnome_dock_item_new(name, behaviour);
	gnome_dock_item_set_shadow_type(GNOME_DOCK_ITEM(wid), shadow_type);
	return wid;
}

static GtkWidget *
menubar_new(GladeXML *xml, GladeWidgetInfo *info)
{
	GtkWidget *wid = gtk_menu_bar_new();

	/* a kludge to remove the menu bar border if we are being packed in
	 * a dock item */
	if (!strcmp(info->parent->class, "GnomeDockItem"))
		gtk_menu_bar_set_shadow_type(GTK_MENU_BAR(wid),
					     GTK_SHADOW_NONE);
	return wid;
}

static GtkWidget *
menu_new(GladeXML *xml, GladeWidgetInfo *info)
{
	return gtk_menu_new();
}

static GtkWidget *
pixmapmenuitem_new(GladeXML *xml, GladeWidgetInfo *info)
{
	/* This needs to be rewritten to handle the GNOMEUIINFO_* macros.
	 * For now, we have something that is useable, but not perfect */
	GtkWidget *wid;
	GList *tmp;
	char *label = NULL;
	const char *stock_icon = NULL;
	gboolean right = FALSE;

	for (tmp = info->attributes; tmp; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;

		if (!strcmp(attr->name, "label"))
			label = attr->value;
		else if (!strcmp(attr->name, "stock_icon"))
			stock_icon = get_stock_name(attr->value);
		else if (!strcmp(attr->name, "right_justify"))
			right = attr->value[0] == 'T';
	}
	wid = gtk_pixmap_menu_item_new();
	if (label) {
		GtkAccelGroup *accel;
		guint key;
		GtkWidget *lwid = gtk_label_new("");
		key = gtk_label_parse_uline(GTK_LABEL(lwid), _(label));
		if (key) {
			accel = glade_xml_get_uline_accel(xml);
			if (accel)
				gtk_widget_add_accelerator(wid,
							   "activate_item",
							   accel, key, 0, 0);
			else {
				/* not inside a GtkMenu -- must be on menubar*/
				accel = glade_xml_ensure_accel(xml);
				gtk_widget_add_accelerator(wid,
							   "activate_item",
							   accel, key,
							   GDK_MOD1_MASK, 0);
			}
		}
		gtk_container_add(GTK_CONTAINER(wid), lwid);
		gtk_widget_show(lwid);
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
toolbar_new(GladeXML *xml, GladeWidgetInfo *info)
{
	GtkWidget *tool;
	GList *tmp;
	GtkOrientation orient = GTK_ORIENTATION_HORIZONTAL;
	GtkToolbarStyle style = GTK_TOOLBAR_BOTH;
	GtkToolbarSpaceStyle spaces = GTK_TOOLBAR_SPACE_EMPTY;
	int space_size = 5;
	gboolean tooltips = TRUE;

	for (tmp = info->attributes; tmp; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;

		switch (attr->name[0]) {
		case 'o':
			if (!strcmp(attr->name, "orientation"))
				orient = glade_enum_from_string(
					GTK_TYPE_ORIENTATION, attr->value);
			break;
		case 's':
			if (!strcmp(attr->name, "space_size"))
				space_size = strtol(attr->value, NULL, 0);
			else if (!strcmp(attr->name, "space_style"))
				spaces = glade_enum_from_string(
					GTK_TYPE_TOOLBAR_SPACE_STYLE,
					attr->value);
			break;
		case 't':
			if (!strcmp(attr->name, "type"))
				style = glade_enum_from_string(
					GTK_TYPE_TOOLBAR_STYLE, attr->value);
			else if (!strcmp(attr->name, "tooltips"))
				tooltips = attr->value[0] == 'T';
			break;
		}
	}
	tool = gtk_toolbar_new(orient, style);
	gtk_toolbar_set_space_size(GTK_TOOLBAR(tool), space_size);
	gtk_toolbar_set_space_style(GTK_TOOLBAR(tool), spaces);
	gtk_toolbar_set_tooltips(GTK_TOOLBAR(tool), tooltips);

	gtk_toolbar_set_button_relief(GTK_TOOLBAR(tool),
		gnome_preferences_get_toolbar_relief_btn()?GTK_RELIEF_NORMAL:
				      GTK_RELIEF_NONE);
	return tool;
}

static GtkWidget *
about_new(GladeXML *xml, GladeWidgetInfo *info)
{
	GtkWidget *wid;
	GList *tmp;
	gchar *title = gnome_app_id, *version = gnome_app_version;
	gchar *copyright = NULL;
	gchar **authors = NULL;
	gchar *comments = NULL, *logo = NULL;

        for (tmp = info->attributes; tmp; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;

		if (!strcmp(attr->name, "copyright"))
			copyright = attr->value;
		else if (!strcmp(attr->name, "authors")) {
			if (authors) g_strfreev(authors);
			authors = g_strsplit(attr->value, "\n", 0);
		} else if (!strcmp(attr->name, "comments"))
			comments = attr->value;
		else if (!strcmp(attr->name, "logo"))
			logo = attr->value;
	}
	wid = gnome_about_new(title, version, _(copyright),
			      (const gchar **)authors, _(comments), logo);
	/* if (title) g_free(title); */
	/* if (version) g_free(version); */
	if (authors) g_strfreev(authors);

	glade_xml_set_toplevel(xml, GTK_WINDOW(wid));

	return wid;
}

static GtkWidget *
gnomedialog_new(GladeXML *xml, GladeWidgetInfo *info)
{
	GtkWidget *win;
	GList *tmp;
	gint xpos = -1, ypos = -1;
	gboolean allow_shrink = TRUE, allow_grow = TRUE, auto_shrink = FALSE;
	gboolean auto_close = FALSE, hide_on_close = FALSE;
	gchar *title = NULL, *wmname = NULL, *wmclass = NULL;

	for (tmp = info->attributes; tmp; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;

		switch (attr->name[0]) {
		case 'a':
			if (!strcmp(attr->name, "allow_grow"))
				allow_grow = attr->value[0] == 'T';
			else if (!strcmp(attr->name, "allow_shrink"))
				allow_shrink = attr->value[0] == 'T';
			else if (!strcmp(attr->name, "auto_shrink"))
				auto_shrink = attr->value[0] == 'T';
			else if (!strcmp(attr->name, "auto_close"))
				auto_close = attr->value[0] == 'T';
			break;
		case 'h':
			if (!strcmp(attr->name, "hide_on_close"))
				hide_on_close = attr->value[0] == 'T';
		case 't':
			if (!strcmp(attr->name, "title"))
				title = attr->value;
			break;
		case 'w':
			if (!strcmp(attr->name, "wmclass_name"))
				wmname = attr->value;
			else if (!strcmp(attr->name, "wmclass_class"))
				wmclass = attr->value;
			break;
		case 'x':
			if (attr->name[1] == '\0')
				xpos = strtol(attr->value, NULL, 0);
			break;
		case 'y':
			if (attr->name[1] == '\0')
				ypos = strtol(attr->value, NULL, 0);
			break;
		}
	}
	win = gnome_dialog_new(_(title), NULL);
	gtk_window_set_policy(GTK_WINDOW(win), allow_shrink, allow_grow,
			      auto_shrink);
	if (wmname || wmclass)
		gtk_window_set_wmclass(GTK_WINDOW(win),
				       wmname?wmname:"", wmclass?wmclass:"");
	gnome_dialog_set_close(GNOME_DIALOG(win), auto_close);
	gnome_dialog_close_hides(GNOME_DIALOG(win), hide_on_close);
	if (xpos >= 0 || ypos >= 0)
		gtk_widget_set_uposition (win, xpos, ypos);

	glade_xml_set_toplevel(xml, GTK_WINDOW(win));

	return win;
}

static GtkWidget *
messagebox_new(GladeXML *xml, GladeWidgetInfo *info)
{
	GtkWidget *win;
	GList *tmp;
	gchar *typename = GNOME_MESSAGE_BOX_GENERIC, *message = NULL;

	for (tmp = info->attributes; tmp; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;

		switch (attr->name[0]) {
		case 'm':
			if (!strcmp(attr->name, "message"))
				message = attr->value;
			else if (!strcmp(attr->name, "message_box_type")) {
				gchar *str = attr->value;
				if (strncmp(str, "GNOME_MESSAGE_BOX_", 18))
					break;
				str += 18;
				if (!strcmp(str, "INFO"))
					typename = GNOME_MESSAGE_BOX_INFO;
				else if (!strcmp(str, "WARNING"))
					typename = GNOME_MESSAGE_BOX_WARNING;
				else if (!strcmp(str, "ERROR"))
					typename = GNOME_MESSAGE_BOX_ERROR;
				else if (!strcmp(str, "QUESTION"))
					typename = GNOME_MESSAGE_BOX_QUESTION;
				else if (!strcmp(str, "GENERIC"))
					typename = GNOME_MESSAGE_BOX_GENERIC;
			}
			break;
		}
	}
	/* create the message box with no buttons */
	win = gnome_message_box_new(_(message), typename, NULL);

	glade_xml_set_toplevel(xml, GTK_WINDOW(win));

	return win;
}

static GtkWidget *
app_new(GladeXML *xml, GladeWidgetInfo *info)
{
	GtkWidget *win;
	GList *tmp;
	gchar *title = NULL, *wmname = NULL, *wmclass = NULL;
	gboolean enable_layout = TRUE;
	gboolean allow_shrink = TRUE, allow_grow = TRUE, auto_shrink = FALSE;

	for (tmp = info->attributes; tmp; tmp = tmp->next) {
		GladeAttribute *attr = tmp->data;

		switch (attr->name[0]) {
		case 'a':
			if (!strcmp(attr->name, "allow_grow"))
				allow_grow = attr->value[0] == 'T';
			else if (!strcmp(attr->name, "allow_shrink"))
				allow_shrink = attr->value[0] == 'T';
			else if (!strcmp(attr->name, "auto_shrink"))
				auto_shrink = attr->value[0] == 'T';
			break;
		case 'e':
			if (!strcmp(attr->name, "enable_layout_config"))
				enable_layout = attr->value[0] == 'T';
			break;
		case 't':
			if (!strcmp(attr->name, "title"))
				title = attr->value;
			break;
		case 'w':
			if (!strcmp(attr->name, "wmclass_name"))
				wmname = attr->value;
			else if (!strcmp(attr->name, "wmclass_class"))
				wmclass = attr->value;
			break;
		}
	}
	win = gnome_app_new(gnome_app_id, _(title));
	gtk_window_set_policy(GTK_WINDOW(win), allow_shrink, allow_grow,
			      auto_shrink);
	if (wmname || wmclass)
		gtk_window_set_wmclass(GTK_WINDOW(win),
				       wmname?wmname:"", wmclass?wmclass:"");
	gnome_app_enable_layout_config(GNOME_APP(win), enable_layout);

	glade_xml_set_toplevel(xml, GTK_WINDOW(win));

	return win;
}

/* -- routines to initialise these widgets with libglade -- */

static const GladeWidgetBuildData widget_data [] = {
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
	{ "GnomeMessageBox",    messagebox_new,     messagebox_build_children},
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
