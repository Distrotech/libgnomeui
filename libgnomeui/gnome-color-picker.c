/*
 * Copyright (C) 1998, 1999 Red Hat, Inc.
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */
/* Color picker button for GNOME
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <config.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtkcolorsel.h>
#include <gtk/gtkcolorseldialog.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkdrawingarea.h>
#include <gtk/gtkframe.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkcompat.h>
#include "gnome-color-picker.h"
#include <libgnome/gnome-i18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkbutton.h>

#include <libgnomeuiP.h>

/* These are the dimensions of the color sample in the color picker */
#define COLOR_PICKER_WIDTH  20
#define COLOR_PICKER_HEIGHT 12
#define COLOR_PICKER_PAD    1

/* Size of checks and gray levels for alpha compositing checkerboard*/
#define CHECK_SIZE  4
#define CHECK_DARK  (1.0 / 3.0)
#define CHECK_LIGHT (2.0 / 3.0)

struct _GnomeColorPickerPrivate {
	GdkPixbuf *pixbuf;	/* Pixbuf for rendering dithered sample */
	GdkGC *gc;		/* GC for drawing */

	GtkWidget *drawing_area;/* Drawing area for color sample */
	GtkWidget *cs_dialog;	/* Color selection dialog */

	gchar *title;		/* Title for the color selection window */

	gdouble r, g, b, a;	/* Red, green, blue, and alpha values */

	guint dither : 1;	/* Dither or just paint a solid color? */
	guint use_alpha : 1;	/* Use alpha or not */
};

enum {
	PROP_0,
	PROP_DITHER,
	PROP_USE_ALPHA,
	PROP_TITLE,
	PROP_RED,
	PROP_GREEN,
	PROP_BLUE,
	PROP_ALPHA,
	PROP_COLOR,
	PROP_COLOR_GDK,
	PROP_COLOR_RGBA
};

enum {
	COLOR_SET,
	LAST_SIGNAL
};

static void gnome_color_picker_class_init (GnomeColorPickerClass *class);
static void gnome_color_picker_init       (GnomeColorPicker      *cp);
static void gnome_color_picker_destroy    (GtkObject             *object);
static void gnome_color_picker_finalize   (GObject               *object);
static void gnome_color_picker_clicked    (GtkButton             *button);
static void gnome_color_picker_state_changed (GtkWidget *widget, GtkStateType previous_state);
static void gnome_color_picker_realize (GtkWidget *widget);
static void gnome_color_picker_style_set (GtkWidget *widget, GtkStyle *previous_style);
static void drag_data_get		(GtkWidget          *widget,
					 GdkDragContext     *context,
					 GtkSelectionData   *selection_data,
					 guint               info,
					 guint               time,
					 GnomeColorPicker   *cpicker);
static void drag_data_received		(GtkWidget        *widget,
					 GdkDragContext   *context,
					 gint              x,
					 gint              y,
					 GtkSelectionData *selection_data,
					 guint             info,
					 guint32           time,
					 GnomeColorPicker *cpicker);
static void gnome_color_picker_set_property (GObject            *object,
					     guint               param_id,
					     const GValue       *value,
					     GParamSpec         *pspec);
static void gnome_color_picker_get_property (GObject            *object,
					     guint               param_id,
					     GValue             *value,
					     GParamSpec         *pspec);


static guint color_picker_signals[LAST_SIGNAL] = { 0 };

static GtkButtonClass *parent_class;

static GtkTargetEntry drop_types[] = { { "application/x-color", 0, 0 } };

GtkType
gnome_color_picker_get_type (void)
{
	static GtkType cp_type = 0;

	if (!cp_type) {
		GtkTypeInfo cp_info = {
			"GnomeColorPicker",
			sizeof (GnomeColorPicker),
			sizeof (GnomeColorPickerClass),
			(GtkClassInitFunc) gnome_color_picker_class_init,
			(GtkObjectInitFunc) gnome_color_picker_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		cp_type = gtk_type_unique (gtk_button_get_type (), &cp_info);
	}

	return cp_type;
}

static void
gnome_color_picker_class_init (GnomeColorPickerClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;
	GtkButtonClass *button_class;

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;
	button_class = (GtkButtonClass *) class;
	widget_class = (GtkWidgetClass *) class;
	parent_class = gtk_type_class (gtk_button_get_type ());

	color_picker_signals[COLOR_SET] =
		gtk_signal_new ("color_set",
				GTK_RUN_FIRST,
				GTK_CLASS_TYPE (object_class),
				GTK_SIGNAL_OFFSET (GnomeColorPickerClass, color_set),
				gnome_marshal_VOID__UINT_UINT_UINT_UINT,
				GTK_TYPE_NONE, 4,
				GTK_TYPE_UINT,
				GTK_TYPE_UINT,
				GTK_TYPE_UINT,
				GTK_TYPE_UINT);

	gobject_class->get_property = gnome_color_picker_get_property;
	gobject_class->set_property = gnome_color_picker_set_property;

        g_object_class_install_property
                (gobject_class,
                 PROP_DITHER,
                 g_param_spec_boolean ("dither", NULL, NULL,
				       TRUE,
				       (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_USE_ALPHA,
                 g_param_spec_boolean ("use_alpha", NULL, NULL,
				       FALSE,
				       (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_TITLE,
                 g_param_spec_string ("title", NULL, NULL,
				      NULL,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_RED,
                 g_param_spec_uint ("red", NULL, NULL,
				    0, 255, 0,
				    (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_GREEN,
                 g_param_spec_uint ("green", NULL, NULL,
				    0, 255, 0,
				    (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_BLUE,
                 g_param_spec_uint ("blue", NULL, NULL,
				    0, 255, 0,
				    (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_ALPHA,
                 g_param_spec_uint ("alpha", NULL, NULL,
				    0, 255, 0,
				    (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	object_class->destroy = gnome_color_picker_destroy;
	gobject_class->finalize = gnome_color_picker_finalize;
	widget_class->state_changed = gnome_color_picker_state_changed;
	widget_class->realize = gnome_color_picker_realize;
	widget_class->style_set = gnome_color_picker_style_set;
	button_class->clicked = gnome_color_picker_clicked;

	class->color_set = NULL;
}

/* Renders the pixmap for the case of dithered or use_alpha */
static void
render_dither (GnomeColorPicker *cp)
{
	gint dark_r, dark_g, dark_b;
	gint light_r, light_g, light_b;
	gint i, j, rowstride;
	gint c1[3], c2[3];
	guchar *pixels;
	guint8 insensitive_r = 0;
	guint8 insensitive_g = 0;
	guint8 insensitive_b = 0;

	/* Compute dark and light check colors */

	insensitive_r = GTK_WIDGET(cp)->style->bg[GTK_STATE_INSENSITIVE].red >> 8;
	insensitive_g = GTK_WIDGET(cp)->style->bg[GTK_STATE_INSENSITIVE].green >> 8;
	insensitive_b = GTK_WIDGET(cp)->style->bg[GTK_STATE_INSENSITIVE].blue >> 8;

	if (cp->_priv->use_alpha) {
		dark_r = (int) ((CHECK_DARK + (cp->_priv->r - CHECK_DARK) * cp->_priv->a) * 255.0 + 0.5);
		dark_g = (int) ((CHECK_DARK + (cp->_priv->g - CHECK_DARK) * cp->_priv->a) * 255.0 + 0.5);
		dark_b = (int) ((CHECK_DARK + (cp->_priv->b - CHECK_DARK) * cp->_priv->a) * 255.0 + 0.5);

		light_r = (int) ((CHECK_LIGHT + (cp->_priv->r - CHECK_LIGHT) * cp->_priv->a) * 255.0 + 0.5);
		light_g = (int) ((CHECK_LIGHT + (cp->_priv->g - CHECK_LIGHT) * cp->_priv->a) * 255.0 + 0.5);
		light_b = (int) ((CHECK_LIGHT + (cp->_priv->b - CHECK_LIGHT) * cp->_priv->a) * 255.0 + 0.5);
	} else {
		dark_r = light_r = (int) (cp->_priv->r * 255.0 + 0.5);
		dark_g = light_g = (int) (cp->_priv->g * 255.0 + 0.5);
		dark_b = light_b = (int) (cp->_priv->b * 255.0 + 0.5);
	}

	/* Fill image buffer */

	pixels = gdk_pixbuf_get_pixels (cp->_priv->pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (cp->_priv->pixbuf);
	for (j = 0; j < COLOR_PICKER_HEIGHT; j++) {
		if ((j / CHECK_SIZE) & 1) {
			c1[0] = dark_r;
			c1[1] = dark_g;
			c1[2] = dark_b;

			c2[0] = light_r;
			c2[1] = light_g;
			c2[2] = light_b;
		} else {
			c1[0] = light_r;
			c1[1] = light_g;
			c1[2] = light_b;

			c2[0] = dark_r;
			c2[1] = dark_g;
			c2[2] = dark_b;
		}

		for (i = 0; i < COLOR_PICKER_WIDTH; i++) {
			if (!GTK_WIDGET_SENSITIVE (GTK_WIDGET (cp)) && (i+j)%2) {
				*(pixels + j * rowstride + i * 3) = insensitive_r;
				*(pixels + j * rowstride + i * 3 + 1) = insensitive_g;
				*(pixels + j * rowstride + i * 3 + 2) = insensitive_b;
			} else if ((i / CHECK_SIZE) & 1) {
				*(pixels + j * rowstride + i * 3) = c1[0];
				*(pixels + j * rowstride + i * 3 + 1) = c1[1];
				*(pixels + j * rowstride + i * 3 + 2) = c1[2];
			} else {
				*(pixels + j * rowstride + i * 3) = c2[0];
				*(pixels + j * rowstride + i * 3 + 1) = c2[1];
				*(pixels + j * rowstride + i * 3 + 2) = c2[2];
			}
		}
	}
	if (cp->_priv->drawing_area->window)
		gdk_pixbuf_render_to_drawable (cp->_priv->pixbuf,
					       cp->_priv->drawing_area->window,
					       cp->_priv->gc,
					       0, 0, 0, 0,
					       COLOR_PICKER_WIDTH,
					       COLOR_PICKER_HEIGHT,
					       GDK_RGB_DITHER_MAX,
					       0, 0);
}

/* Renders the pixmap with the contents of the color sample */
static void
render (GnomeColorPicker *cp)
{
	if (cp->_priv->dither || cp->_priv->use_alpha)
		render_dither (cp);
	else {
		gint i, j, rowstride;
		guint8 insensitive_r = 0;
		guint8 insensitive_g = 0;
		guint8 insensitive_b = 0;
		guchar *pixels;

		pixels = gdk_pixbuf_get_pixels (cp->_priv->pixbuf);
		rowstride = gdk_pixbuf_get_rowstride (cp->_priv->pixbuf);
		insensitive_r = GTK_WIDGET (cp)->style->bg[GTK_STATE_INSENSITIVE].red >> 8;
		insensitive_g = GTK_WIDGET (cp)->style->bg[GTK_STATE_INSENSITIVE].green >> 8;
		insensitive_b = GTK_WIDGET (cp)->style->bg[GTK_STATE_INSENSITIVE].blue >> 8;

		for (i = 0; i < COLOR_PICKER_WIDTH; i++) {
			for (j = 0; j < COLOR_PICKER_HEIGHT; j++) {
				if (!GTK_WIDGET_SENSITIVE (GTK_WIDGET (cp)) && (i+j)%2) {
					*(pixels + j * rowstride + i * 3) = insensitive_r;
					*(pixels + j * rowstride + i * 3 + 1) = insensitive_g;
					*(pixels + j * rowstride + i * 3 + 2) = insensitive_b;
				} else {
					*(pixels + j * rowstride + i * 3) = cp->_priv->r * 255.0 + 0.5;
					*(pixels + j * rowstride + i * 3 + 1) = cp->_priv->g * 255.0 + 0.5;
					*(pixels + j * rowstride + i * 3 + 2) = cp->_priv->b * 255.0 + 0.5;
				}
			}
		}
	}
}

/* Handle exposure events for the color picker's drawing area */
static gint
expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	GnomeColorPicker *cp;

	cp = GNOME_COLOR_PICKER (data);
	if (cp->_priv->gc == NULL)
		cp->_priv->gc = gdk_gc_new (widget->window);

	gdk_pixbuf_render_to_drawable (cp->_priv->pixbuf,
				       widget->window,
				       cp->_priv->gc,
				       event->area.x,
				       event->area.y,
				       event->area.x,
				       event->area.y,
				       event->area.width,
				       event->area.height,
				       GDK_RGB_DITHER_MAX,
				       event->area.x,
				       event->area.y);
	return FALSE;
}
static void
gnome_color_picker_realize (GtkWidget *widget)
{
	GnomeColorPicker *cp = GNOME_COLOR_PICKER (widget);

	if (GTK_WIDGET_CLASS(parent_class)->realize)
		GTK_WIDGET_CLASS (parent_class)->realize (widget);
	render (cp);
}
static void
gnome_color_picker_style_set (GtkWidget *widget, GtkStyle *previous_style)
{
	if (GTK_WIDGET_CLASS(parent_class)->style_set)
		GTK_WIDGET_CLASS (parent_class)->style_set (widget, previous_style);
	if (GTK_WIDGET_REALIZED (widget))
		render (GNOME_COLOR_PICKER (widget));
}

static void
gnome_color_picker_state_changed (GtkWidget *widget, GtkStateType previous_state)
{
	if (widget->state == GTK_STATE_INSENSITIVE || previous_state == GTK_STATE_INSENSITIVE)
		render (GNOME_COLOR_PICKER (widget));
}

static void
drag_data_received (GtkWidget        *widget,
		    GdkDragContext   *context,
		    gint              x,
		    gint              y,
		    GtkSelectionData *selection_data,
		    guint             info,
		    guint32           time,
		    GnomeColorPicker *cpicker)
{
	guint16 *dropped;

	g_return_if_fail (cpicker != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cpicker));

	if (selection_data->length < 0)
		return;

	if ((selection_data->format != 16) || 
	    (selection_data->length != 8)) {
		g_warning (_("Received invalid color data\n"));
		return;
	}


	dropped = (guint16 *)selection_data->data;

	gnome_color_picker_set_i16(cpicker, dropped[0], dropped[1],
				   dropped[2], dropped[3]);
}

static void  
drag_data_get  (GtkWidget          *widget,
		GdkDragContext     *context,
		GtkSelectionData   *selection_data,
		guint               info,
		guint               time,
		GnomeColorPicker   *cpicker)
{
	gushort r, g, b, a;
	guint16 dropped[4];

	gnome_color_picker_get_i16(cpicker, &r, &g, &b, &a);

	dropped[0] = r;
	dropped[1] = g;
	dropped[2] = b;
	dropped[3] = a;

	gtk_selection_data_set (selection_data,
				selection_data->target,
				16/*fromat*/, (guchar *)dropped, 8/*length*/);
}

static void
gnome_color_picker_init (GnomeColorPicker *cp)
{
	GtkWidget *alignment;
	GtkWidget *frame;
	/* Create the widgets */

	cp->_priv = g_new0(GnomeColorPickerPrivate, 1);

	alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_container_set_border_width (GTK_CONTAINER (alignment), COLOR_PICKER_PAD);
	gtk_container_add (GTK_CONTAINER (cp), alignment);
	gtk_widget_show (alignment);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
	gtk_container_add (GTK_CONTAINER (alignment), frame);
	gtk_widget_show (frame);

	gtk_widget_push_colormap (gdk_rgb_get_cmap ());

	cp->_priv->drawing_area = gtk_drawing_area_new ();

	gtk_drawing_area_size (GTK_DRAWING_AREA (cp->_priv->drawing_area), COLOR_PICKER_WIDTH, COLOR_PICKER_HEIGHT);
	gtk_signal_connect (GTK_OBJECT (cp->_priv->drawing_area), "expose_event",
			    (GtkSignalFunc) expose_event,
			    cp);
	gtk_container_add (GTK_CONTAINER (frame), cp->_priv->drawing_area);
	gtk_widget_show (cp->_priv->drawing_area);

	cp->_priv->title = g_strdup (_("Pick a color")); /* default title */

	/* Create the buffer for the image so that we can create an image.  Also create the
	 * picker's pixmap.
	 */

	cp->_priv->pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, COLOR_PICKER_WIDTH, COLOR_PICKER_HEIGHT);

	cp->_priv->gc = NULL;
	gtk_widget_pop_colormap ();

	/* Start with opaque black, dither on, alpha disabled */

	cp->_priv->r = 0.0;
	cp->_priv->g = 0.0;
	cp->_priv->b = 0.0;
	cp->_priv->a = 1.0;
	cp->_priv->dither = TRUE;
	cp->_priv->use_alpha = FALSE;

	gtk_drag_dest_set (GTK_WIDGET (cp),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drop_types, 1, GDK_ACTION_COPY);
	gtk_drag_source_set (GTK_WIDGET(cp),
			     GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
			     drop_types, 1,
			     GDK_ACTION_COPY);
	gtk_signal_connect (GTK_OBJECT (cp),
			    "drag_data_received",
			    GTK_SIGNAL_FUNC (drag_data_received), cp);
	gtk_signal_connect (GTK_OBJECT (cp),
			    "drag_data_get",
			    GTK_SIGNAL_FUNC (drag_data_get), cp);
}

static void
gnome_color_picker_destroy (GtkObject *object)
{
	GnomeColorPicker *cp;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (object));

	cp = GNOME_COLOR_PICKER (object);

	if(cp->_priv->pixbuf)
		gdk_pixbuf_unref (cp->_priv->pixbuf);
	cp->_priv->pixbuf = NULL;
	if(cp->_priv->gc)
		gdk_gc_destroy (cp->_priv->gc);
	cp->_priv->gc = NULL;

	if (cp->_priv->cs_dialog)
		gtk_widget_destroy (cp->_priv->cs_dialog);
	cp->_priv->cs_dialog = NULL;

	g_free (cp->_priv->title);
	cp->_priv->title = NULL;

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gnome_color_picker_finalize (GObject *object)
{
	GnomeColorPicker *cp;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (object));

	cp = GNOME_COLOR_PICKER (object);

	g_free (cp->_priv);
	cp->_priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}


/**
 * gnome_color_picker_new
 *
 * Creates a new GNOME color picker widget. This returns a widget in the form of a small button
 * containing a swatch representing the current selected color. When the button is clicked,
 * a color-selection dialog will open, allowing the user to select a color. The swatch will be
 * updated to reflect the new color when the user finishes.
 *
 * Returns:
 * Pointer to new GNOME color picker widget.
 */

GtkWidget *
gnome_color_picker_new (void)
{
	return GTK_WIDGET (gtk_type_new (gnome_color_picker_get_type ()));
}

/* Callback used when the color selection dialog is destroyed */
static gboolean
cs_destroy (GtkWidget *widget, gpointer data)
{
	GnomeColorPicker *cp;

	cp = GNOME_COLOR_PICKER (data);

	cp->_priv->cs_dialog = NULL;

	return FALSE;
}

/* Callback for when the OK button in the color selection dialog is clicked */
static void
cs_ok_clicked (GtkWidget *widget, gpointer data)
{
	GnomeColorPicker *cp;
	gdouble color[4];
	gushort r, g, b, a;

	cp = GNOME_COLOR_PICKER (data);

	gtk_color_selection_get_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (cp->_priv->cs_dialog)->colorsel),
				       color);
	gtk_widget_destroy (cp->_priv->cs_dialog);

	cp->_priv->r = color[0];
	cp->_priv->g = color[1];
	cp->_priv->b = color[2];
	cp->_priv->a = cp->_priv->use_alpha ? color[3] : 1.0;

	render (cp);
	gtk_widget_draw (cp->_priv->drawing_area, NULL);

	/* Notify the world that the color was set */

	gnome_color_picker_get_i16 (cp, &r, &g, &b, &a);
	gtk_signal_emit (GTK_OBJECT (cp), color_picker_signals[COLOR_SET],
			 r, g, b, a);
}

static int
key_pressed (GtkWidget *widget, GdkEventKey *event, GnomeColorPicker *cp)
{
	if (event->keyval == GDK_Escape){
		gtk_button_clicked (GTK_BUTTON (GTK_COLOR_SELECTION_DIALOG (widget)->cancel_button));
		return 1;
	}
	return 0;
}

static void
gnome_color_picker_clicked (GtkButton *button)
{
	GnomeColorPicker *cp;
	GtkColorSelectionDialog *csd;
	gdouble color[4];

	g_return_if_fail (button != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (button));

	cp = GNOME_COLOR_PICKER (button);

	/*if dialog already exists, make sure it's shown and raised*/
	if(cp->_priv->cs_dialog) {
		csd = GTK_COLOR_SELECTION_DIALOG (cp->_priv->cs_dialog);
		gtk_widget_show (cp->_priv->cs_dialog);
		if (cp->_priv->cs_dialog->window)
			gdk_window_raise(cp->_priv->cs_dialog->window);
	} else {
		/* Create the dialog and connects its buttons */
                GtkWidget *parent;

                parent = gtk_widget_get_toplevel(GTK_WIDGET(cp));
                
		cp->_priv->cs_dialog = gtk_color_selection_dialog_new (cp->_priv->title);

                if (parent)
                        gtk_window_set_transient_for(GTK_WINDOW(cp->_priv->cs_dialog),
                                                     GTK_WINDOW(parent));
                
		csd = GTK_COLOR_SELECTION_DIALOG (cp->_priv->cs_dialog);
		gtk_signal_connect (GTK_OBJECT (cp->_priv->cs_dialog), "destroy",
				    (GtkSignalFunc) cs_destroy,
				    cp);

		gtk_signal_connect (GTK_OBJECT (cp->_priv->cs_dialog), "key_press_event",
				    (GtkSignalFunc) key_pressed, cp);
		gtk_signal_connect (GTK_OBJECT (csd->ok_button), "clicked",
				    (GtkSignalFunc) cs_ok_clicked,
				    cp);

		gtk_signal_connect_object (GTK_OBJECT (csd->cancel_button), "clicked",
					   (GtkSignalFunc) gtk_widget_destroy,
					   GTK_OBJECT(cp->_priv->cs_dialog));

		/* FIXME: do something about the help button */

		gtk_window_set_position (GTK_WINDOW (cp->_priv->cs_dialog), GTK_WIN_POS_MOUSE);

		/* If there is a grabed window, set new dialog as modal */
		if (gtk_grab_get_current())
			gtk_window_set_modal(GTK_WINDOW(cp->_priv->cs_dialog),TRUE);
	}
	gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION (csd->colorsel),
						     cp->_priv->use_alpha);

	color[0] = cp->_priv->r;
	color[1] = cp->_priv->g;
	color[2] = cp->_priv->b;
	color[3] = cp->_priv->use_alpha ? cp->_priv->a : 1.0;

	/* Hack: we set the color twice so that GtkColorSelection will remember its history */
	gtk_color_selection_set_color (GTK_COLOR_SELECTION (csd->colorsel), color);
	gtk_color_selection_set_color (GTK_COLOR_SELECTION (csd->colorsel), color);

	gtk_widget_show (cp->_priv->cs_dialog);
}


/**
 * gnome_color_picker_set_d
 * @cp: Pointer to GNOME color picker widget.
 * @r: Red color component, values are in [0.0, 1.0]
 * @g: Green color component, values are in [0.0, 1.0]
 * @b: Blue color component, values are in [0.0, 1.0]
 * @a: Alpha component, values are in [0.0, 1.0]
 *
 * Description:
 * Set color shown in the color picker widget using floating point values.
 */

void
gnome_color_picker_set_d (GnomeColorPicker *cp, gdouble r, gdouble g, gdouble b, gdouble a)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cp));
	g_return_if_fail ((r >=	0.0) &&	(r <= 1.0));
	g_return_if_fail ((g >=	0.0) &&	(g <= 1.0));
	g_return_if_fail ((b >=	0.0) && (b <= 1.0));
	g_return_if_fail ((a >=	0.0) && (a <= 1.0));

	cp->_priv->r = r;
	cp->_priv->g = g;
	cp->_priv->b = b;
	cp->_priv->a = a;

	render (cp);
	gtk_widget_draw (cp->_priv->drawing_area, NULL);
}


/**
 * gnome_color_picker_get_d
 * @cp: Pointer to GNOME color picker widget.
 * @r: Output location of red color component, values are in [0.0, 1.0]
 * @g: Output location of green color component, values are in [0.0, 1.0]
 * @b: Output location of blue color component, values are in [0.0, 1.0]
 * @a: Output location of alpha color component, values are in [0.0, 1.0]
 *
 * Description:
 * Retrieve color currently selected in the color picker widget in the form of floating point values.
 */

void
gnome_color_picker_get_d (GnomeColorPicker *cp, gdouble *r, gdouble *g, gdouble *b, gdouble *a)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cp));

	if (r)
		*r = cp->_priv->r;

	if (g)
		*g = cp->_priv->g;

	if (b)
		*b = cp->_priv->b;

	if (a)
		*a = cp->_priv->a;
}


/**
 * gnome_color_picker_set_i8
 * @cp: Pointer to GNOME color picker widget.
 * @r: Red color component, values are in [0, 255]
 * @g: Green color component, values are in [0, 255]
 * @b: Blue color component, values are in [0, 255]
 * @a: Alpha component, values are in [0, 255]
 *
 * Description:
 * Set color shown in the color picker widget using 8-bit integer values.
 */

void
gnome_color_picker_set_i8 (GnomeColorPicker *cp, guint8 r, guint8 g, guint8 b, guint8 a)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cp));
	/* Don't check range of r,g,b,a since it's a 8 bit unsigned type. */

	cp->_priv->r = r / 255.0;
	cp->_priv->g = g / 255.0;
	cp->_priv->b = b / 255.0;
	cp->_priv->a = a / 255.0;

	render (cp);
	gtk_widget_draw (cp->_priv->drawing_area, NULL);
}


/**
 * gnome_color_picker_get_i8
 * @cp: Pointer to GNOME color picker widget.
 * @r: Output location of red color component, values are in [0, 255]
 * @g: Output location of green color component, values are in [0, 255]
 * @b: Output location of blue color component, values are in [0, 255]
 * @a: Output location of alpha color component, values are in [0, 255]
 *
 * Description:
 * Retrieve color currently selected in the color picker widget in the form of 8-bit integer values.
 */

void
gnome_color_picker_get_i8 (GnomeColorPicker *cp, guint8 *r, guint8 *g, guint8 *b, guint8 *a)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cp));

	if (r)
		*r = (guint8) (cp->_priv->r * 255.0 + 0.5);

	if (g)
		*g = (guint8) (cp->_priv->g * 255.0 + 0.5);

	if (b)
		*b = (guint8) (cp->_priv->b * 255.0 + 0.5);

	if (a)
		*a = (guint8) (cp->_priv->a * 255.0 + 0.5);
}


/**
 * gnome_color_picker_set_i16
 * @cp: Pointer to GNOME color picker widget.
 * @r: Red color component, values are in [0, 65535]
 * @g: Green color component, values are in [0, 65535]
 * @b: Blue color component, values are in [0, 65535]
 * @a: Alpha component, values are in [0, 65535]
 *
 * Description:
 * Set color shown in the color picker widget using 16-bit integer values.
 */

void
gnome_color_picker_set_i16 (GnomeColorPicker *cp, gushort r, gushort g, gushort b, gushort a)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cp));
	/* Don't check range of r,g,b,a since it's a 16 bit unsigned type. */

	cp->_priv->r = r / 65535.0;
	cp->_priv->g = g / 65535.0;
	cp->_priv->b = b / 65535.0;
	cp->_priv->a = a / 65535.0;

	render (cp);
	gtk_widget_draw (cp->_priv->drawing_area, NULL);
}


/**
 * gnome_color_picker_get_i16
 * @cp: Pointer to GNOME color picker widget.
 * @r: Output location of red color component, values are in [0, 65535]
 * @g: Output location of green color component, values are in [0, 65535]
 * @b: Output location of blue color component, values are in [0, 65535]
 * @a: Output location of alpha color component, values are in [0, 65535]
 *
 * Description:
 * Retrieve color currently selected in the color picker widget in the form of 16-bit integer values.
 */

void
gnome_color_picker_get_i16 (GnomeColorPicker *cp, gushort *r, gushort *g, gushort *b, gushort *a)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cp));

	if (r)
		*r = (gushort) (cp->_priv->r * 65535.0 + 0.5);

	if (g)
		*g = (gushort) (cp->_priv->g * 65535.0 + 0.5);

	if (b)
		*b = (gushort) (cp->_priv->b * 65535.0 + 0.5);

	if (a)
		*a = (gushort) (cp->_priv->a * 65535.0 + 0.5);
}


/**
 * gnome_color_picker_set_dither
 * @cp: Pointer to GNOME color picker widget.
 * @dither: %TRUE if color sample should be dithered, %FALSE if not.
 *
 * Description:
 * Sets whether the picker should dither the color sample or just paint
 * a solid rectangle.
 */

void
gnome_color_picker_set_dither (GnomeColorPicker *cp, gboolean dither)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cp));

	cp->_priv->dither = dither ? TRUE : FALSE;

	render (cp);
	gtk_widget_draw (cp->_priv->drawing_area, NULL);
}


/**
 * gnome_color_picker_get_dither
 * @cp: Pointer to GNOME color picker widget.
 *
 * Description:
 * Does the picker dither the color sample or just paint
 * a solid rectangle.
 *
 * Returns: %TRUE if color sample is dithered, %FALSE if not.
 */

gboolean
gnome_color_picker_get_dither (GnomeColorPicker *cp)
{
	g_return_val_if_fail (cp != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_COLOR_PICKER (cp), FALSE);

	return cp->_priv->dither ? TRUE : FALSE;
}


/**
 * gnome_color_picker_set_use_alpha
 * @cp: Pointer to GNOME color picker widget.
 * @use_alpha: %TRUE if color sample should use alpha channel, %FALSE if not.
 *
 * Description:
 * Sets whether or not the picker should use the alpha channel.
 */

void
gnome_color_picker_set_use_alpha (GnomeColorPicker *cp, gboolean use_alpha)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cp));

	cp->_priv->use_alpha = use_alpha ? TRUE : FALSE;

	render (cp);
	gtk_widget_draw (cp->_priv->drawing_area, NULL);
}

/**
 * gnome_color_picker_get_use_alpha
 * @cp: Pointer to GNOME color picker widget.
 *
 * Description:  Does the picker use the alpha channel?
 *
 * Returns:  %TRUE if color sample uses alpha channel, %FALSE if not.
 */

gboolean
gnome_color_picker_get_use_alpha (GnomeColorPicker *cp)
{
	g_return_val_if_fail (cp != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_COLOR_PICKER (cp), FALSE);

	return cp->_priv->use_alpha ? TRUE : FALSE;
}


/**
 * gnome_color_picker_set_title
 * @cp: Pointer to GNOME color picker widget.
 * @title: String containing new window title.
 *
 * Description:
 * Sets the title for the color selection dialog.
 */

void
gnome_color_picker_set_title (GnomeColorPicker *cp, const gchar *title)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cp));
	g_return_if_fail (title != NULL);

	g_free (cp->_priv->title);
	cp->_priv->title = g_strdup (title);

	if (cp->_priv->cs_dialog)
		gtk_window_set_title (GTK_WINDOW (cp->_priv->cs_dialog), cp->_priv->title);
}

/**
 * gnome_color_picker_get_title
 * @cp: Pointer to GNOME color picker widget.
 *
 * Description:
 * Gets the title of the color selection dialog.
 *
 * Returns:  An internal string, do not free the return value
 */

const char *
gnome_color_picker_get_title (GnomeColorPicker *cp)
{
	g_return_val_if_fail (cp != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_COLOR_PICKER (cp), NULL);

	return cp->_priv->title;
}

static void
gnome_color_picker_set_property (GObject            *object,
				 guint               param_id,
				 const GValue       *value,
				 GParamSpec         *pspec)
{
	GnomeColorPicker *self;
	gushort r, g, b, a;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (object));

	self = GNOME_COLOR_PICKER (object);

	switch (param_id) {
	case PROP_DITHER:
		gnome_color_picker_set_dither(self, g_value_get_boolean (value));
		break;
	case PROP_USE_ALPHA:
		gnome_color_picker_set_use_alpha(self, g_value_get_boolean (value));
		break;
	case PROP_TITLE:
		gnome_color_picker_set_title(self, g_value_get_string (value));
		break;
	case PROP_RED:
		gnome_color_picker_get_i16(self, &r, &g, &b, &a);
		gnome_color_picker_set_i16(self,
					   g_value_get_uint (value), g, b, a);
		break;
	case PROP_GREEN:
		gnome_color_picker_get_i16(self, &r, &g, &b, &a);
		gnome_color_picker_set_i16(self,
					   r, g_value_get_uint (value), b, a);
		break;
	case PROP_BLUE:
		gnome_color_picker_get_i16(self, &r, &g, &b, &a);
		gnome_color_picker_set_i16(self,
					   r, g, g_value_get_uint (value), a);
		break;
	case PROP_ALPHA:
		gnome_color_picker_get_i16(self, &r, &g, &b, &a);
		gnome_color_picker_set_i16(self,
					   r, g, b, g_value_get_uint (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
gnome_color_picker_get_property (GObject            *object,
				 guint               param_id,
				 GValue             *value,
				 GParamSpec         *pspec)
{
	GnomeColorPicker *self;
	gushort val;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (object));

	self = GNOME_COLOR_PICKER (object);

	switch (param_id) {
	case PROP_DITHER:
		g_value_set_boolean (value, gnome_color_picker_get_dither(self));
		break;
	case PROP_USE_ALPHA:
		g_value_set_boolean (value, gnome_color_picker_get_use_alpha(self));
		break;
	case PROP_TITLE:
		g_value_set_string (value, self->_priv->title);
		break;
	case PROP_RED:
		gnome_color_picker_get_i16(self, &val, NULL, NULL, NULL);
		g_value_set_uint (value, val);
		break;
	case PROP_GREEN:
		gnome_color_picker_get_i16(self, NULL, &val, NULL, NULL);
		g_value_set_uint (value, val);
		break;
	case PROP_BLUE:
		gnome_color_picker_get_i16(self, NULL, NULL, &val, NULL);
		g_value_set_uint (value, val);
		break;
	case PROP_ALPHA:
		gnome_color_picker_get_i16(self, NULL, NULL, NULL, &val);
		g_value_set_uint (value, val);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}
