/* gnome-procbar.c - Gnome Process Bar.

   Copyright (C) 1998 Martin Baulig
   All rights reserved.

   Based on the orignal gtop/procbar.c from Radek Doulik.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License
   as published by the Free Software Foundation; either version 2, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.
*/
/*
  @NOTATION@
*/

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtkdrawingarea.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include "gnome-procbar.h"

struct _GnomeProcBarPrivate {
	GtkWidget *bar;
	GtkWidget *label;
	GtkWidget *frame;

	GdkPixmap *bs;
	GdkColor *colors;

	gboolean (*cb)(gpointer);
	gpointer cb_data;

	unsigned int *last;

	gint colors_allocated;
	gint first_request;
	gint n;
	gint tag;

	gboolean vertical : 1;
};


static void gnome_proc_bar_class_init (GnomeProcBarClass *class);
static void gnome_proc_bar_init       (GnomeProcBar      *pb);

static GtkHBoxClass *parent_class;

static gint gnome_proc_bar_expose (GtkWidget *w, GdkEventExpose *e, GnomeProcBar *pb);
static gint gnome_proc_bar_configure (GtkWidget *w, GdkEventConfigure *e, GnomeProcBar *pb);
static void gnome_proc_bar_size_request (GtkWidget *w, GtkRequisition *r, GnomeProcBar *pb);
static void gnome_proc_bar_finalize (GObject *o);
static void gnome_proc_bar_alloc_colors (GnomeProcBar *pb);
static void gnome_proc_bar_free_colors (GnomeProcBar *pb);
static void gnome_proc_bar_draw (GnomeProcBar *pb, const guint val []);
static void gnome_proc_bar_destroy (GtkObject *obj);
static gint gnome_proc_bar_timeout (gpointer data);

guint
gnome_proc_bar_get_type (void)
{
    static guint proc_bar_type = 0;

    if (!proc_bar_type) {
	GtkTypeInfo proc_bar_info = {
	    "GnomeProcBar",
	    sizeof (GnomeProcBar),
	    sizeof (GnomeProcBarClass),
	    (GtkClassInitFunc) gnome_proc_bar_class_init,
	    (GtkObjectInitFunc) gnome_proc_bar_init,
	    NULL,
	    NULL,
	    NULL
	};

	proc_bar_type = gtk_type_unique (gtk_hbox_get_type (), &proc_bar_info);
    }

    return proc_bar_type;
}

static void
gnome_proc_bar_class_init (GnomeProcBarClass *class)
{
    GtkObjectClass *object_class;
    GObjectClass *gobject_class;

    object_class = (GtkObjectClass *) class;
    gobject_class = (GObjectClass *) class;

    parent_class = gtk_type_class (gtk_hbox_get_type ());

    gobject_class->finalize = gnome_proc_bar_finalize;

    object_class->destroy = gnome_proc_bar_destroy;
}

static void
gnome_proc_bar_destroy (GtkObject *obj)
{
  GnomeProcBar *pb = GNOME_PROC_BAR (obj);

  if (pb->_priv->tag != -1) {
    gtk_timeout_remove (pb->_priv->tag);
    pb->_priv->tag = -1;
  }

  gnome_proc_bar_free_colors (pb);
  g_free(pb->_priv->colors);
  pb->_priv->colors = NULL;

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (obj);
}

static void
gnome_proc_bar_finalize (GObject *o)
{
    GnomeProcBar *pb;

    g_return_if_fail (o != NULL);
    g_return_if_fail (GNOME_IS_PROC_BAR (o));

    pb = GNOME_PROC_BAR(o);

    g_free(pb->_priv);
    pb->_priv = NULL;


    (* G_OBJECT_CLASS (parent_class)->finalize) (o);
}

static void
gnome_proc_bar_init (GnomeProcBar *pb)
{
	pb->_priv = g_new0(GnomeProcBarPrivate, 1);
	pb->_priv->colors_allocated = 0;
}

/**
 * gnome_proc_bar_construct:
 * @pb: A #GnomeProBar object to construct
 * @label: Either %NULL or a #GtkWidget that will be shown at the left
 * side of the process bar.
 * @n: Number of items.
 * @colors: Pointer to an array of @n #GdkColor elements.
 * @cb: Callback function to update the process bar.
 *
 * Constructs the @pb objects with @n items with the colors of
 * @colors. To do automatic updating, you set the @cb to a function
 * which takes a single void pointer as an argument and returns %TRUE
 * or %FALSE.  When it returns %FALSE the timer stops running and the
 * function stops getting called. You need to call
 * #gnome_proc_bar_start with the time interval and the data argument
 * that will be passed to the callback to actually start executing the
 * timer.
 * Note a change from 1.x behaviour.  Now the @colors argument is copied so
 * you are responsible for freeing it on your end.
 *
 */
void
gnome_proc_bar_construct (GnomeProcBar *pb, GtkWidget *label, gint n, const GdkColor *colors, gboolean (*cb)(gpointer))
{
    g_return_if_fail (pb != NULL);
    g_return_if_fail (GNOME_IS_PROC_BAR (pb));
    g_return_if_fail (colors != NULL);
    
    pb->_priv->cb = cb;
    pb->_priv->n = n;
    pb->_priv->colors = g_new(GdkColor, n);
    memcpy(pb->_priv->colors, colors, n * sizeof(GdkColor));

    pb->_priv->vertical = FALSE;

    pb->_priv->last = g_new (unsigned, pb->_priv->n+1);
    pb->_priv->last [0] = 0;

    pb->_priv->bar = gtk_drawing_area_new ();
    pb->_priv->frame = gtk_frame_new (NULL);

    pb->_priv->bs = NULL;

    gtk_frame_set_shadow_type (GTK_FRAME (pb->_priv->frame), GTK_SHADOW_IN);

    gtk_container_add (GTK_CONTAINER (pb->_priv->frame), pb->_priv->bar);

    pb->_priv->label = label;

    if (label) {
	gtk_box_pack_start (GTK_BOX (pb), label, FALSE, TRUE, 0);
	gtk_widget_show (label);
    }

    gtk_widget_set_events (pb->_priv->bar, GDK_EXPOSURE_MASK | gtk_widget_get_events (pb->_priv->bar));

    gtk_signal_connect (GTK_OBJECT (pb->_priv->bar), "expose_event",
			(GtkSignalFunc) gnome_proc_bar_expose, pb);
    gtk_signal_connect (GTK_OBJECT (pb->_priv->bar), "configure_event",
			(GtkSignalFunc) gnome_proc_bar_configure, pb);
    gtk_signal_connect (GTK_OBJECT (pb->_priv->bar), "size_request",
			(GtkSignalFunc) gnome_proc_bar_size_request, pb);

    gtk_box_pack_start_defaults (GTK_BOX (pb), pb->_priv->frame);

    gtk_widget_show (pb->_priv->frame);
    gtk_widget_show (pb->_priv->bar);

}

/**
 * gnome_proc_bar_new:
 * @label: Either %NULL or a #GtkWidget that will be shown at the left
 * side of the process bar.
 * @n: Number of items.
 * @colors: Pointer to an array of @n #GdkColor elements.
 * @cb: Callback function to update the process bar.
 *
 * Description: Creates a new Gnome Process Bar with @n items with the
 * colors of @colors. To do automatic updating, you set the @cb to a function
 * which takes a single void pointer as an argument and returns %TRUE or %FALSE.
 * When it returns %FALSE the timer stops running and the function stops getting
 * called. You need to call #gnome_proc_bar_start with the time interval and
 * the data argument that will be passed to the callback to actually start
 * executing the timer.
 * Note a change from 1.x behaviour.  Now the @colors argument is copied so
 * you are responsible for freeing it on your end.
 *
 * Returns: The newly created #GnomeProcBar widget.
 */
GtkWidget *
gnome_proc_bar_new (GtkWidget *label, gint n, const GdkColor *colors, gboolean (*cb)(gpointer))
{
    GnomeProcBar *pb;

    pb = gtk_type_new (gnome_proc_bar_get_type ());

    gnome_proc_bar_construct (pb, label, n, colors, cb);
    return GTK_WIDGET (pb);
}

static gint
gnome_proc_bar_expose (GtkWidget *w, GdkEventExpose *e, GnomeProcBar *pb)
{
    if (pb->_priv->bs)
	gdk_window_copy_area (w->window,
			      w->style->black_gc,
			      e->area.x, e->area.y,
			      pb->_priv->bs,
			      e->area.x, e->area.y,
			      e->area.width, e->area.height);

    return TRUE;
}

static void
gnome_proc_bar_size_request (GtkWidget *w, GtkRequisition *r, GnomeProcBar *pb)
{
    /* FIXME: first_request is always 0, plus this seems incredibly hackish */
    if (!pb->_priv->first_request) {
	r->width = w->allocation.width;
	r->height = w->allocation.height;
    }
    pb->_priv->first_request = 0;
}

static void
gnome_proc_bar_alloc_colors (GnomeProcBar *pb)
{
    g_return_if_fail (pb != NULL);
    g_return_if_fail (GNOME_IS_PROC_BAR (pb));
    g_return_if_fail (pb->_priv->bar != NULL);
    g_return_if_fail (pb->_priv->bar->window != NULL);

    if( ! pb->_priv->colors_allocated) {
	    GdkColormap *cmap;
	    gboolean *success = g_new(gboolean, pb->_priv->n);
	    cmap = gdk_window_get_colormap (pb->_priv->bar->window);
	    gdk_colormap_alloc_colors (cmap,
				       pb->_priv->colors,
				       pb->_priv->n,
				       FALSE, TRUE, success);
	    /* FIXME: We should do something on failiure */
	    g_free(success);
    }

    pb->_priv->colors_allocated = 1;
}

static void
gnome_proc_bar_free_colors (GnomeProcBar *pb)
{
    g_return_if_fail (pb != NULL);
    g_return_if_fail (GNOME_IS_PROC_BAR (pb));
    g_return_if_fail (pb->_priv->bar != NULL);
    g_return_if_fail (pb->_priv->bar->window != NULL);

    if(pb->_priv->colors_allocated) {
	    GdkColormap *cmap;
	    cmap = gdk_window_get_colormap (pb->_priv->bar->window);
	    gdk_colormap_free_colors(cmap, pb->_priv->colors, pb->_priv->n);
    }

    pb->_priv->colors_allocated = 0;
}

static gint
gnome_proc_bar_configure (GtkWidget *w, GdkEventConfigure *e,
			  GnomeProcBar *pb)
{
    gnome_proc_bar_alloc_colors (pb);

    if (pb->_priv->bs) {
	gdk_pixmap_unref (pb->_priv->bs);
	pb->_priv->bs = NULL;
    }

    pb->_priv->bs = gdk_pixmap_new (w->window,
			     w->allocation.width,
			     w->allocation.height,
			     -1);

    gdk_draw_rectangle (w->window, w->style->black_gc, TRUE, 0, 0,
			w->allocation.width, w->allocation.height);

    gnome_proc_bar_draw (pb, pb->_priv->last);

    return TRUE;
}

static void
gnome_proc_bar_draw (GnomeProcBar *pb, const guint val [])
{
    unsigned tot = 0;
    gint i;
    gint x;
    gint wr, w;
    GdkGC *gc;

    w = pb->_priv->vertical ?
	    pb->_priv->bar->allocation.height :
	    pb->_priv->bar->allocation.width;
    x = 0;

    for (i=0; i<pb->_priv->n; i++)
	tot += val [i+1];

    if (!GTK_WIDGET_REALIZED (pb->_priv->bar) || !tot)
	return;

    gc = gdk_gc_new (pb->_priv->bar->window);

    for (i=0; i<pb->_priv->n; i++) {
	if (i<pb->_priv->n-1)
	    wr = (unsigned) w * ((float)val [i+1]/tot);
	else
	    wr = (pb->_priv->vertical ?
		  pb->_priv->bar->allocation.height :
		  pb->_priv->bar->allocation.width) - x;

	gdk_gc_set_foreground (gc,
			       &pb->_priv->colors [i]);

	if (pb->_priv->vertical)
	    gdk_draw_rectangle (pb->_priv->bs,
				gc,
				TRUE,
				0,
				pb->_priv->bar->allocation.height - x - wr,
				pb->_priv->bar->allocation.width,
				wr);
	else
	    gdk_draw_rectangle (pb->_priv->bs,
				gc,
				TRUE,
				x, 0,
				wr, pb->_priv->bar->allocation.height);
	
	x += wr;
    }
		
    gdk_window_copy_area (pb->_priv->bar->window,
			  gc,
			  0, 0,
			  pb->_priv->bs,
			  0, 0,
			  pb->_priv->bar->allocation.width,
			  pb->_priv->bar->allocation.height);

    gdk_gc_destroy (gc);
}

static gint
gnome_proc_bar_timeout (gpointer data)
{
	GnomeProcBar *pb = data;
	gint result;

	GDK_THREADS_ENTER ();
	result = pb->_priv->cb (pb->_priv->cb_data);
	GDK_THREADS_LEAVE ();

	return result;
}

/**
 * gnome_proc_bar_set_values:
 * @pb: Pointer to a #GnomeProcBar object
 * @val: pointer to an array of @pb->_priv->n integers
 *
 * Description: Set the values of @pb to @val and redraw it. You will
 * probably call this function in the callback to update the values.
 *
 * Returns:
 */
void
gnome_proc_bar_set_values (GnomeProcBar *pb, const guint val [])
{
    gint i;
    gint change = 0;

    g_return_if_fail (pb != NULL);
    g_return_if_fail (GNOME_IS_PROC_BAR (pb));

    if (!GTK_WIDGET_REALIZED (pb->_priv->bar)) {
	for (i=0; i<pb->_priv->n+1; i++)
	    pb->_priv->last [i] = val [i];
	return;
    }

    /* check if values changed */

    for (i=0; i<pb->_priv->n+1; i++) {
	if (val[i] != pb->_priv->last [i]) {
	    change = 1;
	    break;
	}
	pb->_priv->last [i] = val [i];
    }

    gnome_proc_bar_draw (pb, val);
}

/**
 * gnome_proc_bar_start:
 * @pb: Pointer to a #GnomeProcBar object
 * @gtime: time interval in ms
 * @data: data to the callback
 *
 * Description: Start a timer, and call the callback that was set
 * on #gnome_proc_bar_new with the @data.
 *
 * Returns:
 */
void
gnome_proc_bar_start (GnomeProcBar *pb, gint gtime, gpointer data)

{
    g_return_if_fail (pb != NULL);
    g_return_if_fail (GNOME_IS_PROC_BAR (pb));

    if (pb->_priv->tag != -1) {
      gtk_timeout_remove (pb->_priv->tag);
      pb->_priv->tag = -1;
    }

    if (pb->_priv->cb) {
	pb->_priv->cb (data);
        pb->_priv->cb_data = data;
	pb->_priv->tag = gtk_timeout_add (gtime, gnome_proc_bar_timeout, pb);
    }
}

/**
 * gnome_proc_bar_stop:
 * @pb: Pointer to a #GnomeProcBar object
 *
 * Description: Stop running the callback in the timer.
 *
 * Returns:
 */
void
gnome_proc_bar_stop (GnomeProcBar *pb)

{
    g_return_if_fail (pb != NULL);
    g_return_if_fail (GNOME_IS_PROC_BAR (pb));

    if (pb->_priv->tag != -1)
	gtk_timeout_remove (pb->_priv->tag);

    pb->_priv->tag = -1;
}

/**
 * gnome_proc_bar_update:
 * @pb: Pointer to a #GnomeProcBar object
 * @colors: Pointer to an array of @pb->_priv->n #GdkColor elements
 *
 * Description: Update @pb with @colors. @pb is not redrawn,
 * it is only redrawn when you call #gnome_proc_bar_set_values
 *
 * Returns:
 */
void
gnome_proc_bar_update (GnomeProcBar *pb, const GdkColor *colors)
{
    g_return_if_fail (pb != NULL);
    g_return_if_fail (GNOME_IS_PROC_BAR (pb));
    g_return_if_fail (colors != NULL);

    gnome_proc_bar_free_colors (pb);
    memcpy(pb->_priv->colors, colors, sizeof(GdkColor) * pb->_priv->n);
    gnome_proc_bar_alloc_colors (pb);
}

/**
 * gnome_proc_bar_set_orient:
 * @pb: Pointer to a #GnomeProcBar object
 * @vertical: %TRUE if vertical %FALSE if horizontal
 *
 * Description: Sets the orientation of @pb to vertical if
 * @vertical is %TRUE or to horizontal if @vertical is %FALSE.
 *
 * Returns:
 */
void
gnome_proc_bar_set_orient (GnomeProcBar *pb, gboolean vertical)
{
    g_return_if_fail (pb != NULL);
    g_return_if_fail (GNOME_IS_PROC_BAR (pb));

    pb->_priv->vertical = vertical;
}
