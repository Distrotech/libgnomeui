/* Stock icons, buttons, and menu items.
   Copyright (C) 1997, 1998 Free Software Foundation

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Eckehard Berns  */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdktypes.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-i18n.h"
#include "libgnome/gnome-config.h"
#include "gnome-stock.h"
#include "gnome-pixmap.h"




/*
 * The stock buttons suck at this time. So maybe we should undefine
 * BUTTON_WANT_ICON. If it is undefined, the stock buttons will not include
 * a pixmap. I leave this defined, so that ppl see a difference between
 * stock buttons and application buttons.
 */
#define BUTTON_WANT_ICON

/*
 * Hmmm, I'm not sure, if using color context can consume to much colors,
 * so one can define this here. If defined, all color context modes other
 * then true/pseudo color and gray lead to the old shading method for
 * insensitive pixmaps. If undefined, the newer method will always be used
 */
#define NOT_ALWAYS_SHADE



#include "gnome-stock-imlib.h"



#define STOCK_SEP '.'
#define STOCK_SEP_STR "."

static GnomePixmap *gnome_stock_pixmap(GtkWidget *window, char *icon, char *subtype);
static GnomeStockPixmapEntry *lookup(char *icon, char *subtype, int fallback);

/*
 * GnomeStockPixmapWidget
 */

static GtkVBoxClass *parent_class = NULL;

static void
gnome_stock_pixmap_widget_destroy(GtkObject *object)
{
	GnomeStockPixmapWidget *w;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_STOCK_PIXMAP_WIDGET (object));

	w = GNOME_STOCK_PIXMAP_WIDGET (object);
	
	/* free resources */
	if (w->pixmap) {
		gtk_container_remove(GTK_CONTAINER(w), GTK_WIDGET(w->pixmap));
		w->pixmap = NULL;
	}
	if (w->regular) {
		gtk_widget_unref(GTK_WIDGET(w->regular));
		w->regular = NULL;
	}
	if (w->disabled) {
		gtk_widget_unref(GTK_WIDGET(w->disabled));
		w->disabled = NULL;
	}
	if (w->focused) {
		gtk_widget_unref(GTK_WIDGET(w->focused));
		w->focused = NULL;
	}
	if (w->icon) g_free(w->icon);

	if (GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(object);
}



/*
 * This function has turned out to be the load_pixmap_function and is not
 * used as signal catcher alone. That means, that you cannot trust the
 * value in prev_state.
 */
static void
gnome_stock_pixmap_widget_state_changed(GtkWidget *widget, guint prev_state)
{
	GnomeStockPixmapWidget *w = GNOME_STOCK_PIXMAP_WIDGET(widget);
	GnomePixmap *pixmap;

	if (!GTK_WIDGET_REALIZED(widget)) return;
	pixmap = NULL;
	if (GTK_WIDGET_HAS_FOCUS(widget)) {
		if (!w->focused) {
			w->focused = gnome_stock_pixmap(w->window, w->icon, GNOME_STOCK_PIXMAP_FOCUSED);
			gtk_widget_ref(GTK_WIDGET(w->focused));
			gtk_widget_show(GTK_WIDGET(w->focused));
		}
		pixmap = w->focused;
	} else if (!GTK_WIDGET_IS_SENSITIVE(widget)) {
		if (!w->disabled) {
			w->disabled = gnome_stock_pixmap(w->window, w->icon, GNOME_STOCK_PIXMAP_DISABLED);
			gtk_widget_ref(GTK_WIDGET(w->disabled));
			gtk_widget_show(GTK_WIDGET(w->disabled));
		}
		pixmap = w->disabled;
	} else {
		if (!w->regular) {
			w->regular = gnome_stock_pixmap(w->window, w->icon, GNOME_STOCK_PIXMAP_REGULAR);
			gtk_widget_ref(GTK_WIDGET(w->regular));
			gtk_widget_show(GTK_WIDGET(w->regular));
		}
		pixmap = w->regular;
	}
	if (pixmap == w->pixmap) return;
	if (w->pixmap) {
		gtk_container_remove(GTK_CONTAINER(w), GTK_WIDGET(w->pixmap));
		/* gtk_widget_queue_draw(widget); */
	}
	w->pixmap = pixmap;
	if (pixmap) {
		gtk_container_add(GTK_CONTAINER(w), GTK_WIDGET(w->pixmap));
		/* gtk_widget_queue_draw(widget); */
	}
}



static void
gnome_stock_pixmap_widget_realize(GtkWidget *widget)
{
	GnomeStockPixmapWidget *p;

	g_return_if_fail(widget != NULL);
	if (parent_class)
		(* GTK_WIDGET_CLASS(parent_class)->realize)(widget);
	p = GNOME_STOCK_PIXMAP_WIDGET(widget);
	if (p->pixmap) return;
	g_return_if_fail(p->window != NULL);
	gnome_stock_pixmap_widget_state_changed(widget, 0);
}



static void
gnome_stock_pixmap_widget_size_request(GtkWidget *widget,
				       GtkRequisition *requisition)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(requisition != NULL);
	g_return_if_fail(GNOME_IS_STOCK_PIXMAP_WIDGET(widget));

	if (!GNOME_STOCK_PIXMAP_WIDGET(widget)->pixmap) {
		requisition->width = GNOME_STOCK_PIXMAP_WIDGET(widget)->width;
		requisition->height = GNOME_STOCK_PIXMAP_WIDGET(widget)->height;
	} else {
		gtk_widget_size_request(GTK_WIDGET(GNOME_STOCK_PIXMAP_WIDGET(widget)->pixmap),
					requisition);
	}
}



static void
gnome_stock_pixmap_widget_class_init(GnomeStockPixmapWidgetClass *klass)
{
	GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
	object_class->destroy = gnome_stock_pixmap_widget_destroy;
	((GtkWidgetClass *)klass)->state_changed =
		gnome_stock_pixmap_widget_state_changed;
	((GtkWidgetClass *)klass)->realize =
		gnome_stock_pixmap_widget_realize;
	((GtkWidgetClass *)klass)->size_request =
		gnome_stock_pixmap_widget_size_request;
}



static void
gnome_stock_pixmap_widget_init(GtkObject *obj)
{
	GnomeStockPixmapWidget *w;

	g_return_if_fail(obj != NULL);
	g_return_if_fail(GNOME_IS_STOCK_PIXMAP_WIDGET(obj));

	w = GNOME_STOCK_PIXMAP_WIDGET(obj);
	w->icon = NULL;
	w->width = 0;
	w->height = 0;
	w->window = NULL;
	w->pixmap = NULL;
	w->regular = NULL;
	w->disabled = NULL;
	w->focused = NULL;
}



guint
gnome_stock_pixmap_widget_get_type(void)
{
	static guint new_type = 0;
	if (!new_type) {
		GtkTypeInfo type_info = {
			"GnomeStockPixmapWidget",
			sizeof(GnomeStockPixmapWidget),
			sizeof(GnomeStockPixmapWidgetClass),
			(GtkClassInitFunc)gnome_stock_pixmap_widget_class_init,
			(GtkObjectInitFunc)gnome_stock_pixmap_widget_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};
		new_type = gtk_type_unique(gtk_vbox_get_type(), &type_info);
		parent_class = gtk_type_class(gtk_vbox_get_type());
	}
	return new_type;
}



GtkWidget *
gnome_stock_pixmap_widget_new(GtkWidget *window, char *icon)
{
	GtkWidget *w;
	GnomeStockPixmapWidget *p;
	GnomeStockPixmapEntry *entry;

	g_return_val_if_fail(icon != NULL, NULL);
	entry = gnome_stock_pixmap_checkfor(icon, GNOME_STOCK_PIXMAP_REGULAR);
	g_return_val_if_fail(entry != NULL, NULL);

	w = gtk_type_new(gnome_stock_pixmap_widget_get_type());
	p = GNOME_STOCK_PIXMAP_WIDGET(w);
	p->icon = g_strdup(icon);
	if (entry->type == GNOME_STOCK_PIXMAP_TYPE_IMLIB_SCALED) {
		p->width = entry->imlib_s.scaled_width;
		p->height = entry->imlib_s.scaled_height;
	} else {
		p->width = entry->any.width;
		p->height = entry->any.height;
	}
	p->window = window;
	return w;
}



void
gnome_stock_pixmap_widget_set_icon(GnomeStockPixmapWidget *widget,
				   char *icon)
{
	GnomeStockPixmapEntry *entry;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(icon != NULL);
	g_return_if_fail(GNOME_IS_STOCK_PIXMAP_WIDGET(widget));

	entry = lookup(icon, GNOME_STOCK_PIXMAP_REGULAR, 0);
	g_return_if_fail(entry != NULL);

	if (widget->icon)
		if (0 == strcmp(widget->icon, icon))
			return;

	if (widget->regular) {
		gtk_widget_unref(GTK_WIDGET(widget->regular));
		widget->regular = NULL;
	}
	if (widget->disabled) {
		gtk_widget_unref(GTK_WIDGET(widget->disabled));
		widget->disabled = NULL;
	}
	if (widget->focused) {
		gtk_widget_unref(GTK_WIDGET(widget->focused));
		widget->focused = NULL;
	}
	if (widget->icon) g_free(widget->icon);
	widget->icon = g_strdup(icon);
	gnome_stock_pixmap_widget_state_changed(GTK_WIDGET(widget), 0);
}



/****************/
/* some helpers */
/****************/


struct scale_color {
	unsigned char r, g, b;
};

static struct scale_color scale_base = { 0xd6, 0xd6, 0xd6 };
static struct scale_color scale_trans = { 0xff, 0x00, 0xff };

static char *
scale_down(GtkWidget *window, unsigned char *datao, gint wo, gint ho, gint w, gint h)
{
	unsigned char *data, *p, *p2, *p3;
	long x, y, w2, h2, xo, yo, wo3, x2, y2, i, x3, y3;
	long yw, xw, ww, hw, r, g, b, r2, g2, b2;
	int trans;

	if (window) {
		GtkStyle *style = gtk_widget_get_style(window);
		scale_base.r = style->bg[0].red >> 8;
		scale_base.g = style->bg[0].green >> 8;
		scale_base.b = style->bg[0].blue >> 8;
	}
	data = g_malloc(w * h * 3);
	p = data;

	ww = (wo << 8) / w;
	hw = (ho << 8) / h;
	h2 = h << 8;
	w2 = w << 8;
	wo3 = wo * 3;
	for (y = 0; y < h2; y += 0x100) {
		yo = (y * ho) / h;
		y2 = yo & 0xff;
		yo >>= 8;
		for (x = 0; x < w2; x += 0x100) {
			xo = (x * wo) / w;
			x2 = xo & 0xff;
			xo >>= 8;
			i = xo + (yo * wo);
			p2 = datao + (i * 3);

			r2 = g2 = b2 = 0;
			yw = hw;
			y3 = y2;
			trans = 1;
			while (yw) {
				xw = ww;
				x3 = x2;
				p3 = p2;
				r = g = b = 0;
				while (xw) {
					if ((0x100 - x3) < xw)
						i = 0x100 - x3;
					else
						i = xw;
					if ((p3[0] == scale_trans.r) &&
					    (p3[1] == scale_trans.g) &&
					    (p3[2] == scale_trans.b)) {
						r += scale_base.r * i;
						g += scale_base.g * i;
						b += scale_base.b * i;
					} else {
						trans = 0;
						r += p3[0] * i;
						g += p3[1] * i;
						b += p3[2] * i;
					}
					p3 += 3;
					xw -= i;
					x3 = 0;
				}
				if ((0x100 - y3) < yw) {
					r2 += r * (0x100 - y3);
					g2 += g * (0x100 - y3);
					b2 += b * (0x100 - y3);
					yw -= 0x100 - y3;
				} else {
					r2 += r * yw;
					g2 += g * yw;
					b2 += b * yw;
					yw = 0;
				}
				y3 = 0;
				p2 += wo3;
			}
			if (trans) {
				*(p++) = scale_trans.r;
				*(p++) = scale_trans.g;
				*(p++) = scale_trans.b;
			} else {
				r2 /= ww * hw;
				g2 /= ww * hw;
				b2 /= ww * hw;
				*(p++) = r2 & 0xff;
				*(p++) = g2 & 0xff;
				*(p++) = b2 & 0xff;
			}
		}
	}

	return data;
}

struct _default_entries_data {
	char *icon, *subtype;
	char *label;
	gchar *rgb_data;
	int width, height;
	int scaled_width, scaled_height;
};

#define TB_W 20
#define TB_H 20
#define TIGERT_W 24
#define TIGERT_H 24
#define MENU_W 16
#define MENU_H 16

struct _default_entries_data entries_data[] = {
	{GNOME_STOCK_PIXMAP_NEW, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_new, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_SAVE, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_save, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_SAVE_AS, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_save_as, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_CUT, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_cut, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_HELP, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_help, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_PRINT, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_print, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_SEARCH, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_search, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_BACK, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_left_arrow, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_FORWARD, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_right_arrow, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_HOME, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_home, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_STOP, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_stop, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_REFRESH, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_refresh, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_OPEN, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_open, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_COPY, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_copy, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_PASTE, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_paste, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_PROPERTIES, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_properties, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_PREFERENCES, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_preferences, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_SCORES, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_scores, TB_W, TB_H, TB_W, TB_H},
	{GNOME_STOCK_PIXMAP_UNDO, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_undo, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_TIMER, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_timer, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_TIMER_STOP, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_timer_stopped, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_MAIL, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_mail, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_MAIL_RCV, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_mail_receive, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_MAIL_SND, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_mail_send, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_TRASH, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_trash, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_TRASH_FULL, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_trash_full, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},

	{GNOME_STOCK_PIXMAP_EXIT, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_exit, TB_W, TB_H, TB_W, TB_H},
	{GNOME_STOCK_BUTTON_OK, GNOME_STOCK_PIXMAP_REGULAR, N_("OK"), imlib_button_ok, TB_W, TB_H, TB_W, TB_H},
	{GNOME_STOCK_BUTTON_APPLY, GNOME_STOCK_PIXMAP_REGULAR, N_("Apply"), imlib_button_apply, TB_W, TB_H, TB_W, TB_H},
	{GNOME_STOCK_BUTTON_CANCEL, GNOME_STOCK_PIXMAP_REGULAR, N_("Cancel"), imlib_button_cancel, TB_W, TB_H, TB_W, TB_H},
	{GNOME_STOCK_BUTTON_CLOSE, GNOME_STOCK_PIXMAP_REGULAR, N_("Close"), imlib_exit, TB_W, TB_H, TB_W, TB_H},
	{GNOME_STOCK_BUTTON_YES, GNOME_STOCK_PIXMAP_REGULAR, N_("Yes"), imlib_button_yes, TB_W, TB_H, TB_W, TB_H},
	{GNOME_STOCK_BUTTON_NO, GNOME_STOCK_PIXMAP_REGULAR, N_("No"), imlib_button_no, TB_W, TB_H, TB_W, TB_H},
	{GNOME_STOCK_BUTTON_HELP, GNOME_STOCK_PIXMAP_REGULAR, N_("Help"), imlib_help, TIGERT_W, TIGERT_H, TB_W, TB_H},
	{GNOME_STOCK_MENU_NEW, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_new, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_SAVE, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_save, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_SAVE_AS, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_save_as, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_OPEN, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_open, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_QUIT, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_menu_exit, MENU_W, MENU_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_CUT, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_cut, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_COPY, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_copy, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_PASTE, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_paste, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_PROP, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_properties, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_PREF, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_preferences, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_UNDO, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_undo, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_PRINT, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_print, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_SEARCH, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_search, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_BACK, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_left_arrow, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_FORWARD, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_right_arrow, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_HOME, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_home, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_STOP, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_stop, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_REFRESH, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_refresh, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_EXIT, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_menu_exit, MENU_W, MENU_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_MAIL, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_mail, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_MAIL_RCV, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_mail_receive, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_MAIL_SND, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_mail_send, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_TRASH, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_trash, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_TRASH_FULL, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_trash_full, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_ABOUT, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_menu_about, MENU_W, MENU_H, MENU_W, MENU_H},
	/* TODO: I shouldn't waste a pixmap for that */
	{GNOME_STOCK_MENU_BLANK, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_menu_blank, MENU_W, MENU_H, MENU_W, MENU_H}, 
	{GNOME_STOCK_MENU_SCORES, GNOME_STOCK_PIXMAP_REGULAR, NULL, imlib_menu_scores, 20, 20, 20, 20},
};

static int entries_data_num = sizeof(entries_data) / sizeof(entries_data[0]);


static char *
build_hash_key(char *icon, char *subtype)
{
	return g_copy_strings(icon, STOCK_SEP_STR, subtype ? subtype : GNOME_STOCK_PIXMAP_REGULAR, NULL);
}



static GHashTable *
stock_pixmaps(void)
{
	static GHashTable *hash = NULL;
	GnomeStockPixmapEntry *entry;
	int i;

	if (hash) return hash;

	hash = g_hash_table_new(g_str_hash, g_str_equal);

	for (i = 0; i < entries_data_num; i++) {
		entry = g_malloc(sizeof(GnomeStockPixmapEntry));
		entry->any.width = entries_data[i].width;
		entry->any.height = entries_data[i].height;
		entry->any.label = entries_data[i].label;
		entry->type = GNOME_STOCK_PIXMAP_TYPE_IMLIB_SCALED;
		entry->imlib_s.scaled_width = entries_data[i].scaled_width;
		entry->imlib_s.scaled_height = entries_data[i].scaled_height;
		entry->imlib_s.rgb_data = entries_data[i].rgb_data;
		entry->imlib_s.shape.r = 255;
		entry->imlib_s.shape.g = 0;
		entry->imlib_s.shape.b = 255;
		entry->imlib_s.shape.pixel = 0;
		g_hash_table_insert(hash,
				    build_hash_key(entries_data[i].icon,
						   entries_data[i].subtype),
				    entry);
	}
	
#ifdef DEBUG
	{
		time_t t;
		int i;
		unsigned char *bla;

		bla = g_malloc(3 * 24 * 24);
		t = time(NULL);
		while (t == time(NULL)) ;
		t++;
		i = 0;
		while ((t == time(NULL)) && (i < 0x1000000)) {
			g_free(scale_down(NULL, bla, 24, 24, 16, 16));
			i++;
		}
		g_print("%x %d\n", i, i);
	}
#endif

	return hash;
}


static GnomeStockPixmapEntry *
lookup(char *icon, char *subtype, int fallback)
{
	char *s;
	GHashTable *hash = stock_pixmaps();
	GnomeStockPixmapEntry *entry;

	s = build_hash_key(icon, subtype);
	entry = (GnomeStockPixmapEntry *)g_hash_table_lookup(hash, s);
	if (!entry) {
		if (!fallback) return NULL;
		g_free(s);
		s = build_hash_key(icon, GNOME_STOCK_PIXMAP_REGULAR);
		entry = (GnomeStockPixmapEntry *)
			g_hash_table_lookup(hash, s);
	}
	g_free(s);
	return entry;
}



static GnomePixmap *
create_pixmap_from_data(GtkWidget *window, GnomeStockPixmapEntryData *data)
{
	GnomePixmap *pixmap;

	pixmap = GNOME_PIXMAP(gnome_pixmap_new_from_xpm_d_at_size(data->xpm_data, data->width, data->height));
	return pixmap;
}



static GnomePixmap *
create_pixmap_from_imlib(GtkWidget *window, GnomeStockPixmapEntryImlib *data)
{
	static GdkImlibColor shape_color = { 0xff, 0, 0xff, 0 };

	return (GnomePixmap *)gnome_pixmap_new_from_rgb_d_shaped(data->rgb_data,
								 NULL,
								 data->width,
								 data->height,
								 &shape_color);
}


static GnomePixmap *
create_pixmap_from_imlib_scaled(GtkWidget *window,
				GnomeStockPixmapEntryImlibScaled *data)
{
	static GdkImlibColor shape_color = { 0xff, 0, 0xff, 0 };
	gchar *d;

	if ((data->width != data->scaled_width) ||
	    (data->height != data->scaled_height)) {
		d = scale_down(window, data->rgb_data, data->width, data->height,
			       data->scaled_width, data->scaled_height);
	} else {
		d = data->rgb_data;
	}

	return (GnomePixmap *)gnome_pixmap_new_from_rgb_d_shaped(d, NULL,
								 data->scaled_width,
								 data->scaled_height,
								 &shape_color);
}



static GnomePixmap *
create_pixmap_from_path(GnomeStockPixmapEntryPath *data)
{
	return GNOME_PIXMAP(gnome_pixmap_new_from_file(data->pathname));
}



static void
build_disabled_pixmap(GtkWidget *window, GnomePixmap **inout_pixmap)
{
	GdkGC *gc;
	GdkWindow *pixmap = (*inout_pixmap)->pixmap;
	gint w, h, x, y;
	GdkGCValues vals;
	GdkVisual *visual;
	GdkImage *image;
	GdkColorContext *cc;
	GdkColor color;
	GdkColormap *cmap;
	gint32 red, green, blue;
#ifdef NOT_ALWAYS_SHADE
	GtkStyle *style;
#endif

	g_return_if_fail(window != NULL);
	g_return_if_fail(GTK_WIDGET_REALIZED(window));

	gdk_window_get_size(pixmap, &w, &h);
	visual = gtk_widget_get_visual(GTK_WIDGET(*inout_pixmap));
	cmap = gtk_widget_get_colormap(GTK_WIDGET(*inout_pixmap));
	gc = gdk_gc_new (pixmap);

	cc = gdk_color_context_new(visual, cmap);
#ifdef NOT_ALWAYS_SHADE
	if ((cc->mode != GDK_CC_MODE_TRUE) &&
	    (cc->mode != GDK_CC_MODE_MY_GRAY)) {
		gdk_color_context_free(cc);
		style = gtk_widget_get_style(window);
		color = style->bg[0];
		gdk_gc_set_foreground (gc, &color);
		for (y = 0; y < h; y ++) {
			for (x = y % 2; x < w; x += 2) {
				gdk_draw_point(pixmap, gc, x, y);
			}
		}
		return;
	}
#endif

	image = gdk_image_get(pixmap, 0, 0, w, h);
	gdk_gc_get_values(gc, &vals);
	style = gtk_widget_get_style(window);
	color = style->bg[0];
	red = color.red;
	green = color.green;
	blue = color.blue;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			GdkColor c;
			int failed;
			c.pixel = gdk_image_get_pixel(image, x, y);
			gdk_color_context_query_color(cc, &c);
			c.red = (((gint32)c.red - red) >> 1)
				+ red;
			c.green = (((gint32)c.green - green) >> 1)
				+ green;
			c.blue = (((gint32)c.blue - blue) >> 1)
				+ blue;
			c.pixel = gdk_color_context_get_pixel(cc, c.red,
							      c.green,
							      c.blue,
							      &failed);
			gdk_image_put_pixel(image, x, y, c.pixel);
		}
	}
	gdk_draw_image(pixmap, gc, image, 0, 0, 0, 0, w, h);
	gdk_image_destroy(image);
	gdk_color_context_free(cc);
}



/**********************/
/* utitlity functions */
/**********************/



static GnomePixmap *
gnome_stock_pixmap(GtkWidget *window, char *icon, char *subtype)
{
	GnomeStockPixmapEntry *entry;
	GnomePixmap *pixmap;

	g_return_val_if_fail(icon != NULL, NULL);
	/* subtype can be NULL, so not checked */
	entry = lookup(icon, subtype, TRUE);
	if (!entry) return NULL;

	if ((entry->type != GNOME_STOCK_PIXMAP_TYPE_IMLIB) &&
	    (entry->type != GNOME_STOCK_PIXMAP_TYPE_IMLIB_SCALED)) {
		g_return_val_if_fail(window != NULL, NULL);
		g_return_val_if_fail(GTK_IS_WIDGET(window), NULL);
	}
	
	pixmap = NULL;
	switch (entry->type) {
	 case GNOME_STOCK_PIXMAP_TYPE_DATA:
		pixmap = create_pixmap_from_data(window, &entry->data);
		break;
	 case GNOME_STOCK_PIXMAP_TYPE_PATH:
		pixmap = create_pixmap_from_path(&entry->path);
		break;
	 case GNOME_STOCK_PIXMAP_TYPE_IMLIB:
		pixmap = create_pixmap_from_imlib(window, &entry->imlib);
		break;
	 case GNOME_STOCK_PIXMAP_TYPE_IMLIB_SCALED:
		pixmap = create_pixmap_from_imlib_scaled(window, &entry->imlib_s);
		break;
	 default:
		g_assert_not_reached();
		break;
	}
	/* check if we have to draw our own disabled pixmap */
	/* TODO: should be optimized a bit */
	if ((NULL == lookup(icon, subtype, 0)) && (pixmap) &&
	    (0 == strcmp(subtype, GNOME_STOCK_PIXMAP_DISABLED))) {
		build_disabled_pixmap(window, &pixmap);
	}
	return pixmap;
}



GtkWidget *
gnome_stock_pixmap_widget(GtkWidget *window, char *icon)
{
	GtkWidget *w;

	w = gnome_stock_pixmap_widget_new(window, icon);
	return w;
}


gint
gnome_stock_pixmap_register(char *icon, char *subtype,
			    GnomeStockPixmapEntry *entry)
{
	g_return_val_if_fail(NULL == lookup(icon, subtype, 0), 0);
	g_return_val_if_fail(entry != NULL, 0);
	g_hash_table_insert(stock_pixmaps(), build_hash_key(icon, subtype),
			    entry);
	return 1;
}



gint
gnome_stock_pixmap_change(char *icon, char *subtype,
			  GnomeStockPixmapEntry *entry)
{
	GHashTable *hash;
	char *key;

	g_return_val_if_fail(NULL != lookup(icon, subtype, 0), 0);
	g_return_val_if_fail(entry != NULL, 0);
	hash = stock_pixmaps();
	key = build_hash_key(icon, subtype);
	g_hash_table_remove(hash, key);
	g_hash_table_insert(hash, key, entry);
	/* TODO: add some method to change all pixmaps (or at least
	   the pixmap_widgets) to the new icon */
	return 1;
}



GnomeStockPixmapEntry *
gnome_stock_pixmap_checkfor(char *icon, char *subtype)
{
	return lookup(icon, subtype, 0);
}



static GtkWidget *
stock_button_from_entry (char *type, GnomeStockPixmapEntry *entry)
{
	GtkWidget *button, *label, *hbox;
#ifdef BUTTON_WANT_ICON
	GtkWidget *pixmap;
#endif /* BUTTON_WANT_ICON */

	button = gtk_button_new();
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(button), hbox);
	if (entry->any.label)
		label = gtk_label_new(dgettext(PACKAGE, entry->any.label));
	else
		label = gtk_label_new(dgettext(PACKAGE, type));
	gtk_widget_show(label);
	gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 7);

#ifdef BUTTON_WANT_ICON
	pixmap = gnome_stock_pixmap_widget(button, type);
	if (pixmap) {
		gtk_widget_show(pixmap);
		gtk_box_pack_start_defaults(GTK_BOX(hbox), pixmap);
	}
#endif /* BUTTON_WANT_ICON */
	return button;
}

GtkWidget *
gnome_stock_button(char *type)
{
	GnomeStockPixmapEntry *entry;

	entry = lookup(type,GNOME_STOCK_PIXMAP_REGULAR,0);
	g_assert(entry != NULL);

	return stock_button_from_entry (type, entry);
}

GtkWidget *
gnome_stock_or_ordinary_button (char *type)
{
	GnomeStockPixmapEntry *entry;

	entry = lookup(type,GNOME_STOCK_PIXMAP_REGULAR,0);
	if (entry != NULL)
		return stock_button_from_entry (type, entry);

	return gtk_button_new_with_label (type);
}



/***********/
/*  menus  */
/***********/

static int use_icons = -1;

GtkWidget *
gnome_stock_menu_item(char *type, char *text)
{
	GtkWidget *hbox, *w, *menu_item;

	g_return_val_if_fail(type != NULL, NULL);
	g_return_val_if_fail(gnome_stock_pixmap_checkfor(type, GNOME_STOCK_PIXMAP_REGULAR), NULL);
	g_return_val_if_fail(text != NULL, NULL);

	if (use_icons == -1) {
		/* TODO: fetch default value from gnome_config */
		use_icons = TRUE;
	}
	if (use_icons) {
		hbox = gtk_hbox_new(FALSE, 2);
		gtk_widget_show(hbox);
		w = gnome_stock_pixmap_widget(hbox, type);
		gtk_widget_show(w);
		gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 0);

		w = gtk_label_new(text);
		gtk_widget_show(w);
		gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 0);

		menu_item = gtk_menu_item_new();
		gtk_container_add(GTK_CONTAINER(menu_item), hbox);
	} else {
		menu_item = gtk_menu_item_new_with_label(text);
	}

	return menu_item;
}



/***********************/
/*  menu accelerators  */
/***********************/

typedef struct _AccelEntry AccelEntry;
struct _AccelEntry {
	guchar key;
	guint8 mod;
};

struct default_AccelEntry {
	char *type;
	AccelEntry entry;
};

struct default_AccelEntry default_accel_hash[] = {
	{GNOME_STOCK_MENU_NEW, {'N', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_OPEN, {'O', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_SAVE, {'S', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_SAVE_AS, {'S', GDK_SHIFT_MASK | GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_QUIT, {'Q', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_CUT, {'X', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_COPY, {'C', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_PASTE, {'V', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_PROP, {0, 0}},
	{GNOME_STOCK_MENU_PREF, {'E', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_ABOUT, {'A', GDK_MOD1_MASK}},
	{GNOME_STOCK_MENU_SCORES, {0, 0}},
	{GNOME_STOCK_MENU_UNDO, {'Z', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_PRINT, {'P', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_SEARCH, {'S', GDK_MOD1_MASK}},
	{GNOME_STOCK_MENU_BACK, {'B', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_FORWARD, {'F', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_MAIL, {0, 0}},
	{GNOME_STOCK_MENU_MAIL_RCV, {0, 0}},
	{GNOME_STOCK_MENU_MAIL_SND, {0, 0}},
	{NULL}
};


static char *
accel_to_string(AccelEntry *entry)
{
	static char s[30];

	s[0] = 0;
	if (!entry->key) return NULL;
	if (entry->mod & GDK_CONTROL_MASK)
		strcat(s, "Ctl+");
	if (entry->mod & GDK_MOD1_MASK)
		strcat(s, "Alt+");
	if (entry->mod & GDK_SHIFT_MASK)
		strcat(s, "Shft+");
	if (entry->mod & GDK_MOD2_MASK)
		strcat(s, "Mod2+");
	if (entry->mod & GDK_MOD3_MASK)
		strcat(s, "Mod3+");
	if (entry->mod & GDK_MOD4_MASK)
		strcat(s, "Mod4+");
	if (entry->mod & GDK_MOD5_MASK)
		strcat(s, "Mod5+");
	if ((entry->key >= 'a') && (entry->key <= 'z')) {
		s[strlen(s) + 1] = 0;
		s[strlen(s)] = entry->key - 'a' + 'A';
	} else if ((entry->key >= 'A') && (entry->key <= 'Z')) {
		s[strlen(s) + 1] = 0;
		s[strlen(s)] = entry->key;
	} else {
		return NULL;
	}
	return s;
}


static void
accel_from_string(char *s, guchar *key, guint8 *mod)
{
	char *p, *p1;

	*mod = 0;
	*key = 0;
	if (!s) return;
	p = s;
	do {
		p1 = p;
		while ((*p) && (*p != '+')) p++;
		if (*p == '+') {
			*p = 0;
			if (0 == g_strcasecmp(p1, "Ctl"))
				*mod |= GDK_CONTROL_MASK;
			else if (0 == g_strcasecmp(p1, "Alt"))
				*mod |= GDK_MOD1_MASK;
			else if (0 == g_strcasecmp(p1, "Shft"))
				*mod |= GDK_SHIFT_MASK;
			else if (0 == g_strcasecmp(p1, "Mod2"))
				*mod |= GDK_MOD2_MASK;
			else if (0 == g_strcasecmp(p1, "Mod3"))
				*mod |= GDK_MOD3_MASK;
			else if (0 == g_strcasecmp(p1, "Mod4"))
				*mod |= GDK_MOD4_MASK;
			else if (0 == g_strcasecmp(p1, "Mod5"))
				*mod |= GDK_MOD5_MASK;
			*p = '+';
			p++;
		}
	} while (*p);
	if (p1[1] == 0) {
		*key = *p1;
	} else {
		*key = 0;
		*mod = 0;
		return;
	}
}


static void
accel_read_rc(gpointer key, gpointer value, gpointer data)
{
	char *path, *s;
	AccelEntry *entry = value;
	gboolean got_default;

	path = g_copy_strings(data, key, "=", accel_to_string(value), NULL);
	s = gnome_config_get_string_with_default(path, &got_default);
	g_free(path);
	if (got_default) {
		g_free(s);
		return;
	}
	accel_from_string(s, &entry->key, &entry->mod);
	g_free(s);
}


static GHashTable *
accel_hash(void) {
	static GHashTable *hash = NULL;
	struct default_AccelEntry *p;

	if (!hash) {
		hash = g_hash_table_new(g_str_hash, g_str_equal);
		for (p = default_accel_hash; p->type; p++)
			g_hash_table_insert(hash, p->type, &p->entry);
		g_hash_table_foreach(hash, accel_read_rc,
				     "/GnomeStock/Accelerators/");
	}
	return hash;
}

gboolean
gnome_stock_menu_accel(char *type, guchar *key, guint8 *mod)
{
	AccelEntry *entry;

	entry = g_hash_table_lookup(accel_hash(), type);
	if (!entry) {
		*key = 0;
		*mod = 0;
		return FALSE;
	}

	*key = entry->key;
	*mod = entry->mod;
	return (*key != 0);
}

void
gnome_stock_menu_accel_parse(char *section)
{
	g_return_if_fail(section != NULL);
	g_hash_table_foreach(accel_hash(), accel_read_rc,
			     section);
}



/********************************/
/*  accelerator definition box	*/
/********************************/

#include <libgnomeui/gnome-messagebox.h>
#include <libgnomeui/gnome-propertybox.h>

static void
accel_dlg_apply(GtkWidget *box, int n)
{
	GtkCList *clist;
	char *section, *key, *s;
	int i;

	if (n != 0) return;
	clist = gtk_object_get_data(GTK_OBJECT(box), "clist");
	section = gtk_object_get_data(GTK_OBJECT(box), "section");
	for (i = 0; i < clist->rows; i++) {
		gtk_clist_get_text(clist, i, 0, &s);
		key = g_copy_strings(section, s, NULL);
		gtk_clist_get_text(clist, i, 1, &s);
		gnome_config_set_string(key, s);
		g_free(key);
	}
	gnome_config_sync();
}



static void
accel_dlg_help(GtkWidget *box, int n)
{
	GtkWidget *w;
	
	w = gnome_message_box_new("No help available yet!", "info",
				 GNOME_STOCK_BUTTON_OK, NULL);
	gtk_widget_show(w);
}



static void
accel_dlg_select_ok(GtkWidget *widget, GtkWindow *window)
{
	AccelEntry entry;
	GtkToggleButton *check;
	gchar *key, *s, *s2;
	int row;

	key = gtk_entry_get_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(window), "key")));
	if (!key) {
		entry.key = 0;
		entry.mod = 0;
	} else {
		accel_from_string(key, &entry.key, &entry.mod);
		entry.mod = 0;
		check = gtk_object_get_data(GTK_OBJECT(window), "shift");
		if (check->active)
			entry.mod |= GDK_SHIFT_MASK;
		check = gtk_object_get_data(GTK_OBJECT(window), "control");
		if (check->active)
			entry.mod |= GDK_CONTROL_MASK;
		check = gtk_object_get_data(GTK_OBJECT(window), "alt");
		if (check->active)
			entry.mod |= GDK_MOD1_MASK;
	}
	row = (int)gtk_object_get_data(GTK_OBJECT(window), "row");
	gtk_clist_get_text(GTK_CLIST(gtk_object_get_data(GTK_OBJECT(window), "clist")),
			   row, 1, &s);
	if (!s) s = "";
	s2 = accel_to_string(&entry);
	if (!s2) s2 = "";
	if (g_strcasecmp(s2, s)) {
		gnome_property_box_changed(GNOME_PROPERTY_BOX(gtk_object_get_data(GTK_OBJECT(window), "box")));
		gtk_clist_set_text(GTK_CLIST(gtk_object_get_data(GTK_OBJECT(window),
								 "clist")),
					     row, 1, accel_to_string(&entry));
	}
}



static void
accel_dlg_select(GtkCList *widget, int row, int col, GdkEventButton *event)
{
	AccelEntry entry;
	GtkTable *table;
	GtkWidget *window, *w;
	char *s;

	gtk_clist_unselect_row(widget, row, col);
	s = NULL;
	gtk_clist_get_text(widget, row, col, &s);
	if (s) {
		accel_from_string(s, &entry.key, &entry.mod);
	} else {
		entry.key = 0;
		entry.mod = 0;
	}
	window = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(window), "Menu Accelerator");
	gtk_object_set_data(GTK_OBJECT(window), "clist", widget);
	gtk_object_set_data(GTK_OBJECT(window), "row", (gpointer)row);
	gtk_object_set_data(GTK_OBJECT(window), "col", (gpointer)col);
	gtk_object_set_data(GTK_OBJECT(window), "box",
			    gtk_object_get_data(GTK_OBJECT(widget), "box"));

	table = (GtkTable *)gtk_table_new(0, 0, FALSE);
	gtk_widget_show(GTK_WIDGET(table));
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(window)->vbox),
			  GTK_WIDGET(table));

	gtk_clist_get_text(GTK_CLIST(widget), row, 0, &s);
	s = g_copy_strings("Accelerator for ", s, NULL);
	w = gtk_label_new(s);
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 0, 2, 0, 1);

	w = gtk_label_new("Key:");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 0, 1, 1, 2);
	w = gtk_entry_new();
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 1, 2, 1, 2);
	gtk_object_set_data(GTK_OBJECT(window), "key", w);
	if (accel_to_string(&entry)) {
		s = g_strdup(accel_to_string(&entry));
		if (strrchr(s, '+'))
			gtk_entry_set_text(GTK_ENTRY(w), strrchr(s, '+') + 1);
		else
			gtk_entry_set_text(GTK_ENTRY(w), s);
		g_free(s);
	}

	w = gtk_check_button_new_with_label("Control");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 0, 2, 2, 3);
	gtk_object_set_data(GTK_OBJECT(window), "control", w);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(w),
				    entry.mod & GDK_CONTROL_MASK);

	w = gtk_check_button_new_with_label("Shift");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 0, 2, 3, 4);
	gtk_object_set_data(GTK_OBJECT(window), "shift", w);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(w),
				    entry.mod & GDK_SHIFT_MASK);

	w = gtk_check_button_new_with_label("Alt");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 0, 2, 4, 5);
	gtk_object_set_data(GTK_OBJECT(window), "alt", w);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(w),
				    entry.mod & GDK_MOD1_MASK);

	w = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);
	gtk_widget_show(w);
	gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
				  (GtkSignalFunc)gtk_widget_destroy,
				  GTK_OBJECT(window));
	gtk_box_pack_end_defaults(GTK_BOX(GTK_DIALOG(window)->action_area), w);
	w = gnome_stock_button(GNOME_STOCK_BUTTON_OK);
	gtk_widget_show(w);
	gtk_signal_connect(GTK_OBJECT(w), "clicked",
			   (GtkSignalFunc)accel_dlg_select_ok,
			   window);
	gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
				  (GtkSignalFunc)gtk_widget_destroy,
				  GTK_OBJECT(window));
	gtk_box_pack_end_defaults(GTK_BOX(GTK_DIALOG(window)->action_area), w);
	gtk_widget_show(window);
}



void
gnome_stock_menu_accel_dlg(char *section)
{
	GnomePropertyBox *box;
	GtkWidget *w, *label;
	struct default_AccelEntry *p;
	char *titles[2];
	char *row_data[2];

	box = GNOME_PROPERTY_BOX(gnome_property_box_new());
	gtk_window_set_title(GTK_WINDOW(box), _("Menu Accelerator Keys"));

	label = gtk_label_new(_("Global"));
	gtk_widget_show(label);
	titles[0] = _("Menu Item");
	titles[1] = _("Accelerator");
	w = gtk_clist_new_with_titles(2, titles);
	gtk_object_set_data(GTK_OBJECT(box), "clist", w);
	gtk_widget_set_usize(w, -1, 170);
	gtk_clist_set_column_width(GTK_CLIST(w), 0, 100);
	gtk_clist_column_titles_passive(GTK_CLIST(w));
	gtk_widget_show(w);
	gtk_signal_connect(GTK_OBJECT(w), "select_row",
			   (GtkSignalFunc)accel_dlg_select,
			   NULL);
	gtk_object_set_data(GTK_OBJECT(w), "box", box);
	for (p = default_accel_hash; p->type; p++) {
		row_data[0] = p->type;
		row_data[1] = g_strdup(accel_to_string(&p->entry));
		gtk_clist_append(GTK_CLIST(w), row_data);
		g_free(row_data[1]);
	}
	gnome_property_box_append_page(box, w, label);

	if (!section) {
		gtk_object_set_data(GTK_OBJECT(box), "section",
				    "/GnomeStock/Accelerators/");
	} else {
		gtk_object_set_data(GTK_OBJECT(box), "section", section);
		/* TODO: maybe add another page for the app's menu accelerator
		 * config */
	}

	gtk_signal_connect(GTK_OBJECT(box), "apply",
			  (GtkSignalFunc)accel_dlg_apply, NULL);
	gtk_signal_connect(GTK_OBJECT(box), "help",
			  (GtkSignalFunc)accel_dlg_help, NULL);

	gtk_widget_show(GTK_WIDGET(box));
}

