/* gnome-procbar.c - Gnome Process Bar.

   Copyright (C) 1998 Martin Baulig

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

#include <config.h>
#include <gnome.h>
#include "gnome-procbar.h"

#define A (w->allocation)

static void gnome_proc_bar_class_init (GnomeProcBarClass *class);
static void gnome_proc_bar_init       (GnomeProcBar      *pb);

static GtkHBoxClass *parent_class;

static gint gnome_proc_bar_expose (GtkWidget *w, GdkEventExpose *e, GnomeProcBar *pb);
static gint gnome_proc_bar_configure (GtkWidget *w, GdkEventConfigure *e, GnomeProcBar *pb);
static void gnome_proc_bar_size_request (GtkWidget *w, GtkRequisition *r, GnomeProcBar *pb);
static void gnome_proc_bar_finalize (GtkObject *o);
static void gnome_proc_bar_setup_colors (GnomeProcBar *pb);
static void gnome_proc_bar_draw (GnomeProcBar *pb, unsigned val []);
static void gnome_proc_bar_destroy (GtkObject *obj);


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
	    (GtkArgSetFunc) NULL,
	    (GtkArgGetFunc) NULL
	};

	proc_bar_type = gtk_type_unique (gtk_hbox_get_type (), &proc_bar_info);
    }

    return proc_bar_type;
}

static void
gnome_proc_bar_class_init (GnomeProcBarClass *class)
{
    GtkObjectClass *object_class;

    object_class = (GtkObjectClass *) class;

    parent_class = gtk_type_class (gtk_hbox_get_type ());

    object_class->finalize = gnome_proc_bar_finalize;

    object_class->destroy = gnome_proc_bar_destroy;
}

static void
gnome_proc_bar_destroy (GtkObject *obj)
{
  GnomeProcBar *pb = GNOME_PROC_BAR (obj);

  if (pb->tag != -1) {
    gtk_timeout_remove (pb->tag);
    pb->tag = -1;
  }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (obj);
}

static void
gnome_proc_bar_finalize (GtkObject *o)
{
    g_return_if_fail (o != NULL);
    g_return_if_fail (GNOME_IS_PROC_BAR (o));

    (* GTK_OBJECT_CLASS (parent_class)->finalize) (o);
}

static void
gnome_proc_bar_init (GnomeProcBar *pb)
{
}

/**
 * gnome_proc_bar_new:
 * @label: Either %NULL or a @GtkWidget that will be shown at the left
 * side of the process bar.
 * @n: Number of items.
 * @colors: Pointer to an array of @n @GdkColor elements.
 * @cb: Callback function to update the process bar.
 *
 * Creates a new Gnome Process Bar.
 *
 * Returns the newly created @GnomeProcBar widget.
 */

GtkWidget *
gnome_proc_bar_new (GtkWidget *label, gint n, GdkColor *colors, gint (*cb)())
{
    GnomeProcBar *pb;

    pb = gtk_type_new (gnome_proc_bar_get_type ());

    pb->cb = cb;
    pb->n = n;
    pb->colors = colors;
    pb->vertical = FALSE;

    pb->tag = -1;
    pb->first_request = 1;
    pb->colors_allocated = 0;

    pb->last = g_new (unsigned, pb->n+1);
    pb->last [0] = 0;

    pb->bar = gtk_drawing_area_new ();
    pb->frame = gtk_frame_new (NULL);

    pb->bs = NULL;

    gtk_frame_set_shadow_type (GTK_FRAME (pb->frame), GTK_SHADOW_IN);

    gtk_container_add (GTK_CONTAINER (pb->frame), pb->bar);

    pb->label = label;

    if (label) {
	gtk_box_pack_start (GTK_BOX (pb), label, FALSE, TRUE, 0);
	gtk_widget_show (pb->label);
    }

    gtk_widget_set_events (pb->bar, GDK_EXPOSURE_MASK | gtk_widget_get_events (pb->bar));

    gtk_signal_connect (GTK_OBJECT (pb->bar), "expose_event",
			(GtkSignalFunc) gnome_proc_bar_expose, pb);
    gtk_signal_connect (GTK_OBJECT (pb->bar), "configure_event",
			(GtkSignalFunc) gnome_proc_bar_configure, pb);
    gtk_signal_connect (GTK_OBJECT (pb->bar), "size_request",
			(GtkSignalFunc) gnome_proc_bar_size_request, pb);

    gtk_box_pack_start_defaults (GTK_BOX (pb), pb->frame);

    gtk_widget_show (pb->frame);
    gtk_widget_show (pb->bar);

    return GTK_WIDGET (pb);
}

static gint
gnome_proc_bar_expose (GtkWidget *w, GdkEventExpose *e, GnomeProcBar *pb)
{
    if (pb->bs)
	gdk_window_copy_area (w->window,
			      w->style->black_gc,
			      e->area.x, e->area.y,
			      pb->bs,
			      e->area.x, e->area.y,
			      e->area.width, e->area.height);

    return TRUE;
}

static void
gnome_proc_bar_size_request (GtkWidget *w, GtkRequisition *r, GnomeProcBar *pb)
{
    if (!pb->first_request) {
	r->width = w->allocation.width;
	r->height = w->allocation.height;
    }
    pb->first_request = 0;
}

static void
gnome_proc_bar_setup_colors (GnomeProcBar *pb)
{
    GdkColormap *cmap;
    gint i;

    cmap = gdk_window_get_colormap (pb->bar->window);
    for (i=0; i<pb->n; i++)
	gdk_color_alloc (cmap, &pb->colors [i]);

    pb->colors_allocated = 1;
}

static gint
gnome_proc_bar_configure (GtkWidget *w, GdkEventConfigure *e,
			  GnomeProcBar *pb)
{
    gnome_proc_bar_setup_colors (pb);

    if (pb->bs) {
	gdk_pixmap_unref (pb->bs);
	pb->bs = NULL;
    }

    pb->bs = gdk_pixmap_new (w->window,
			     w->allocation.width,
			     w->allocation.height,
			     -1);

    gdk_draw_rectangle (w->window, w->style->black_gc, TRUE, 0, 0,
			w->allocation.width, w->allocation.height);

    gnome_proc_bar_draw (pb, pb->last);

    return TRUE;
}

#undef A

#define W (pb->bar)
#define A (pb->bar->allocation)

static void
gnome_proc_bar_draw (GnomeProcBar *pb, unsigned val [])
{
    unsigned tot = 0;
    gint i;
    gint x;
    gint wr, w;
    GdkGC *gc;

    w = pb->vertical ? A.height : A.width;
    x = 0;

    for (i=0; i<pb->n; i++)
	tot += val [i+1];

    if (!GTK_WIDGET_REALIZED (pb->bar) || !tot)
	return;

    gc = gdk_gc_new (pb->bar->window);

    for (i=0; i<pb->n; i++) {
	if (i<pb->n-1)
	    wr = (unsigned) w * ((float)val [i+1]/tot);
	else
	    wr = (pb->vertical ? A.height : A.width) - x;

	gdk_gc_set_foreground (gc,
			       &pb->colors [i]);

	if (pb->vertical)
	    gdk_draw_rectangle (pb->bs,
				gc,
				TRUE,
				0, A.height - x - wr,
				A.width, wr);
	else
	    gdk_draw_rectangle (pb->bs,
				gc,
				TRUE,
				x, 0,
				wr, A.height);
	
	x += wr;
    }
		
    gdk_window_copy_area (pb->bar->window,
			  gc,
			  0, 0,
			  pb->bs,
			  0, 0,
			  A.width, A.height);

    gdk_gc_destroy (gc);
}

#undef W
#undef A

void
gnome_proc_bar_set_values (GnomeProcBar *pb, unsigned val [])
{
    unsigned tot = val [0];
    gint i;
    gint change = 0;

    g_return_if_fail (pb != NULL);
    g_return_if_fail (GNOME_IS_PROC_BAR (pb));

    if (!GTK_WIDGET_REALIZED (pb->bar)) {
	for (i=0; i<pb->n+1; i++)
	    pb->last [i] = val [i];
	return;
    }

    /* check if values changed */

    for (i=0; i<pb->n+1; i++) {
	if (val[i] != pb->last [i]) {
	    change = 1;
	    break;
	}
	pb->last [i] = val [i];
    }

    gnome_proc_bar_draw (pb, val);
}

void
gnome_proc_bar_start (GnomeProcBar *pb, gint gtime, gpointer data)

{
    g_return_if_fail (pb != NULL);
    g_return_if_fail (GNOME_IS_PROC_BAR (pb));

    if (pb->tag != -1)
      gtk_timeout_remove (pb->tag);

    if (pb->cb) {
	pb->cb (data);
	pb->tag = gtk_timeout_add (gtime, pb->cb, data);
    }
}

void
gnome_proc_bar_stop (GnomeProcBar *pb)

{
    g_return_if_fail (pb != NULL);
    g_return_if_fail (GNOME_IS_PROC_BAR (pb));

    if (pb->tag != -1)
	gtk_timeout_remove (pb->tag);

    pb->tag = -1;
}

void
gnome_proc_bar_update (GnomeProcBar *pb, GdkColor *colors)
{
    char tmp [BUFSIZ];
    gint i;

    g_return_if_fail (pb != NULL);
    g_return_if_fail (GNOME_IS_PROC_BAR (pb));

    for (i=0;i<pb->n;i++) {
	sprintf (tmp, "#%04x%04x%04x",
		 colors [i].red, colors [i].green,
		 colors [i].blue);
	gdk_color_parse (tmp, &pb->colors [i]);
    }

    gnome_proc_bar_setup_colors (pb);
}

void
gnome_proc_bar_set_orient (GnomeProcBar *pb, gboolean vertical)
{
    g_return_if_fail (pb != NULL);
    g_return_if_fail (GNOME_IS_PROC_BAR (pb));

    pb->vertical = vertical;
}
