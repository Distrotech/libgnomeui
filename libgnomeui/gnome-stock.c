#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtksignal.h>
#include "libgnome/gnome-defs.h"
#include "gnome-stock.h"

#include "../programs/gtt/tb_new.xpm"
#include "../programs/gtt/tb_save.xpm"
#include "../programs/gtt/tb_open.xpm"
#include "../programs/gtt/tb_cut.xpm"
#include "../programs/gtt/tb_copy.xpm"
#include "../programs/gtt/tb_paste.xpm"
#include "../programs/gtt/tb_properties.xpm"
/* #include "../programs/gtt/tb_prop_dis.xpm" */
#include "../programs/gtt/tb_unknown.xpm"
#include "../programs/gtt/tb_exit.xpm"
#include "gnome-stock-ok.xpm"
#include "gnome-stock-cancel.xpm"


#define STOCK_SEP '.'
#define STOCK_SEP_STR "."


/**************************/
/* GnomeStockPixmapWidget */
/**************************/

static GtkVBoxClass *parent_class = NULL;

static void
gnome_stock_pixmap_widget_destroy(GtkObject *object)
{
	GnomeStockPixmapWidget *w;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_STOCK_PIXMAP_WIDGET (object));

	w = GNOME_STOCK_PIXMAP_WIDGET (object);
	
	/* free resources */
	if (w->regular) gtk_widget_destroy(GTK_WIDGET(w->regular));
	if (w->disabled) gtk_widget_destroy(GTK_WIDGET(w->disabled));
	if (w->focused) gtk_widget_destroy(GTK_WIDGET(w->focused));
        if (w->icon) g_free(w->icon);

	if (GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(object);
}



static void
gnome_stock_pixmap_widget_state_changed(GtkWidget *widget, guint prev_state)
{
        GnomeStockPixmapWidget *w = GNOME_STOCK_PIXMAP_WIDGET(widget);
        GtkPixmap *pixmap;

        pixmap = NULL;
        if (GTK_WIDGET_HAS_FOCUS(widget)) {
                if (!w->focused) {
                        w->focused = gnome_stock_pixmap(w->window, w->icon, GNOME_STOCK_PIXMAP_FOCUSED);
                        gtk_widget_show(GTK_WIDGET(w->focused));
                }
                pixmap = w->focused;
        } else if (!GTK_WIDGET_IS_SENSITIVE(widget)) {
                if (!w->disabled) {
                        w->disabled = gnome_stock_pixmap(w->window, w->icon, GNOME_STOCK_PIXMAP_DISABLED);
                        gtk_widget_show(GTK_WIDGET(w->disabled));
                }
                pixmap = w->disabled;
        } else {
                if (!w->regular) {
                        w->regular = gnome_stock_pixmap(w->window, w->icon, GNOME_STOCK_PIXMAP_REGULAR);
                        gtk_widget_show(GTK_WIDGET(w->regular));
                }
                pixmap = w->regular;
        }
        if (pixmap == w->pixmap) return;
        if (w->pixmap) {
                gtk_container_remove(GTK_CONTAINER(w), GTK_WIDGET(w->pixmap));
                gtk_widget_queue_draw(widget);
        }
        w->pixmap = pixmap;
        if (pixmap) {
                gtk_container_add(GTK_CONTAINER(w), GTK_WIDGET(w->pixmap));
                gtk_widget_queue_draw(widget);
        }
}



static void
gnome_stock_pixmap_widget_class_init(GnomeStockPixmapWidgetClass *klass)
{
	GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
	object_class->destroy = gnome_stock_pixmap_widget_destroy;
        ((GtkWidgetClass *)klass)->state_changed =
                gnome_stock_pixmap_widget_state_changed;
}



static void
gnome_stock_pixmap_widget_init(GtkObject *obj)
{
        GnomeStockPixmapWidget *w;

        g_return_if_fail(obj != NULL);
        g_return_if_fail(GNOME_IS_STOCK_PIXMAP_WIDGET(obj));

        w = GNOME_STOCK_PIXMAP_WIDGET(obj);
        w->icon = NULL;
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

        g_return_val_if_fail(icon != NULL, NULL);
        g_return_val_if_fail(gnome_stock_pixmap_checkfor(icon, GNOME_STOCK_PIXMAP_REGULAR), NULL);

	w = gtk_type_new(gnome_stock_pixmap_widget_get_type());
        p = GNOME_STOCK_PIXMAP_WIDGET(w);
        p->icon = g_strdup(icon);
        p->window = window;

        p->pixmap = gnome_stock_pixmap(window, icon,
                                       GNOME_STOCK_PIXMAP_REGULAR);
        p->regular = p->pixmap;
        gtk_widget_show(GTK_WIDGET(p->pixmap));
        gtk_container_add(GTK_CONTAINER(w), GTK_WIDGET(p->pixmap));

	return w;
}



/****************/
/* some helpers */
/****************/


struct _default_entries_data {
        char *icon, *subtype;
        gchar **xpm_data;
};

struct _default_entries_data entries_data[] = {
        {GNOME_STOCK_PIXMAP_NEW, GNOME_STOCK_PIXMAP_REGULAR, tb_new_xpm},
        {GNOME_STOCK_PIXMAP_SAVE, GNOME_STOCK_PIXMAP_REGULAR, tb_save_xpm},
        {GNOME_STOCK_PIXMAP_OPEN, GNOME_STOCK_PIXMAP_REGULAR, tb_open_xpm},
        {GNOME_STOCK_PIXMAP_CUT, GNOME_STOCK_PIXMAP_REGULAR, tb_cut_xpm},
        {GNOME_STOCK_PIXMAP_COPY, GNOME_STOCK_PIXMAP_REGULAR, tb_copy_xpm},
        {GNOME_STOCK_PIXMAP_PASTE, GNOME_STOCK_PIXMAP_REGULAR, tb_paste_xpm},
        {GNOME_STOCK_PIXMAP_PROPERTIES, GNOME_STOCK_PIXMAP_REGULAR, tb_properties_xpm},
/*         {GNOME_STOCK_PIXMAP_PROPERTIES, GNOME_STOCK_PIXMAP_DISABLED, tb_prop_dis_xpm}, */
        {GNOME_STOCK_PIXMAP_HELP, GNOME_STOCK_PIXMAP_REGULAR, tb_unknown_xpm},
        {GNOME_STOCK_BUTTON_OK, GNOME_STOCK_PIXMAP_REGULAR, gnome_stock_ok_xpm},
        {GNOME_STOCK_BUTTON_APPLY, GNOME_STOCK_PIXMAP_REGULAR, gnome_stock_ok_xpm},
        {GNOME_STOCK_BUTTON_CLOSE, GNOME_STOCK_PIXMAP_REGULAR, tb_exit_xpm},
        {GNOME_STOCK_BUTTON_CANCEL, GNOME_STOCK_PIXMAP_REGULAR, gnome_stock_cancel_xpm},
};
static int entries_data_num = sizeof(entries_data) / sizeof(entries_data[0]);


static char *
build_hash_key(char *icon, char *subtype)
{
        char *s;

        s = g_malloc(strlen(icon) + strlen(subtype) + 2);
        strcpy(s, icon);
        strcat(s, STOCK_SEP_STR);
        if (subtype)
                strcat(s, subtype);
        else
                strcat(s, GNOME_STOCK_PIXMAP_REGULAR);
        return s;
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
                entry->type = GNOME_STOCK_PIXMAP_TYPE_DATA;
                entry->data.xpm_data = entries_data[i].xpm_data;
                g_hash_table_insert(hash,
                                    build_hash_key(entries_data[i].icon,
                                                   entries_data[i].subtype),
                                    entry);
        }

        return hash;
}


static GnomeStockPixmapEntry *
lookup(char *icon, char *subtype)
{
        char *s;
        GHashTable *hash = stock_pixmaps();
        GnomeStockPixmapEntry *entry;

        s = build_hash_key(icon, subtype);
        entry = (GnomeStockPixmapEntry *)g_hash_table_lookup(hash, s);
        if (!entry) {
                g_free(s);
                s = build_hash_key(icon, GNOME_STOCK_PIXMAP_REGULAR);
                entry = (GnomeStockPixmapEntry *)
                        g_hash_table_lookup(hash, s);
        }
        g_free(s);
        return entry;
}



static GtkPixmap *
create_pixmap_from_data(GtkWidget *window, GnomeStockPixmapEntryData *data)
{
        GtkPixmap *pixmap;
        GdkPixmap *pmap;
        GdkBitmap *bmap;
        GtkStyle *style;
        GdkWindow *gwin;

        if (window) {
                style = gtk_widget_get_style(window);
                gwin = window->window;
        } else {
                style = gtk_widget_get_default_style();
                gwin = NULL;
        }
        pmap = gdk_pixmap_create_from_xpm_d(gwin, &bmap,
                                            &style->bg[GTK_STATE_NORMAL],
                                            data->xpm_data);
	pixmap = (GtkPixmap *)gtk_pixmap_new(pmap, bmap);
        return pixmap;
}



static void
build_disabled_pixmap(GtkWidget *window, GtkPixmap **inout_pixmap)
{
        GdkGC *gc = window->style->bg_gc[0];
        GdkWindow *pixmap = (*inout_pixmap)->pixmap;
        gint w, h, x, y;

        gdk_window_get_size(pixmap, &w, &h);
        for (y = 0; y < h; y ++) {
                for (x = y % 2; x < w; x += 2) {
                        gdk_draw_point(pixmap, gc, x, y);
                }
        }
}



/**********************/
/* utitlity functions */
/**********************/



GtkPixmap *
gnome_stock_pixmap(GtkWidget *window, char *icon, char *subtype)
{
        GnomeStockPixmapEntry *entry;
        GtkPixmap *pixmap;

        g_return_val_if_fail(icon != NULL, NULL);
        /* subtype can be NULL, so not checked */
        /* window can be NULL, but if not, window has to be GtkWidget */
        if (window)
                g_return_val_if_fail(GTK_IS_WIDGET(window), NULL);

        entry = lookup(icon, subtype);
        if (!entry) return NULL;
        pixmap = NULL;
        switch (entry->type) {
        case GNOME_STOCK_PIXMAP_TYPE_DATA:
                pixmap = create_pixmap_from_data(window, &(entry->data));
                break;
        default:
                g_assert_not_reached();
                break;
        }
        /* check if we have to draw our own diabled pixmap */
        /* TODO: should be optimized a bit */
        if ((entry == lookup(icon, GNOME_STOCK_PIXMAP_REGULAR)) &&
            (pixmap) &&
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
        g_print(__FILE__ "gnome_stock_pixmap_register: not implemented yet\n");
        return 0;
}



gint
gnome_stock_pixmap_change(char *icon, char *subtype,
                          GnomeStockPixmapEntry *entry)
{
        g_print(__FILE__ "gnome_stock_pixmap_change: not implemented yet\n");
        return 0;
}



GnomeStockPixmapEntry *
gnome_stock_pixmap_checkfor(char *icon, char *subtype)
{
        return lookup(icon, subtype);
}



GtkWidget *
gnome_stock_button(char *type)
{
        GtkWidget *button, *label, *hbox;
        GtkWidget *pixmap;

        button = gtk_button_new();
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_widget_show(hbox);
        gtk_container_add(GTK_CONTAINER(button), hbox);
        label = gtk_label_new(gettext(type));
        gtk_widget_show(label);
        gtk_box_pack_start_defaults(GTK_BOX(hbox), label);
        
        pixmap = gnome_stock_pixmap_widget(button, type);
        if (pixmap) {
                gtk_widget_show(pixmap);
                gtk_box_pack_end_defaults(GTK_BOX(hbox), pixmap);
        }
        return button;
}

