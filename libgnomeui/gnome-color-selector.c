/* GNOME color selector in a button.
 * Written by Federico Mena <federico@nuclecu.unam.mx>
 * FIXME Copyright notice needed
 */


#include <config.h>

#include <stdio.h>
#include <gtk/gtk.h>

#include "libgnome/gnome-defs.h"
#include "gnome-color-selector.h"

#define PREVIEW_WIDTH  36
#define PREVIEW_HEIGHT 16


static void
gnome_color_selector_fill_buffer(guchar *buf, int width, double r, double g, double b)
{
	int     ir, ig, ib;
	guchar *p;

	g_assert(buf != NULL);

	ir = (int) (r * 255 + 0.5);
	ig = (int) (g * 255 + 0.5);
	ib = (int) (b * 255 + 0.5);

	p = buf;

	for (; width; width--) {
		*p++ = ir;
		*p++ = ig;
		*p++ = ib;
	} /* for */
} /* gnome_color_selector_fill_buffer */


static void
gnome_color_selector_render_preview(GtkPreview *preview,
				    guchar     *buf,
				    int         width,
				    int         height)
{
	int y;

	g_assert(preview != NULL);
	g_assert(buf != NULL);

	for (y = 0; y < height; y++)
		gtk_preview_draw_row(preview, buf, 0, y, width);

	gtk_widget_draw(GTK_WIDGET(preview), NULL);
} /* gnome_color_selector_render_preview */

static void
gnome_color_selector_set_cs_color(GnomeColorSelector *gcs)
{
	double                   color[4];
	GtkColorSelection       *cs;
	GtkColorSelectionDialog *csd;
	
	g_assert(gcs != NULL);

	if (gcs->cs_dialog) {
		color[0] = gcs->r;
		color[1] = gcs->g;
		color[2] = gcs->b;
		color[3] = 1.0; /* Always opaque */
		
		csd = GTK_COLOR_SELECTION_DIALOG(gcs->cs_dialog);
		cs  = GTK_COLOR_SELECTION(csd->colorsel);
		
		gtk_color_selection_set_color(cs, color);
	} /* if */
} /* gnome_color_selector_set_cs_color */


static void
gnome_color_selector_ok_clicked(GtkWidget *widget,
				gpointer   data)
{
	GnomeColorSelector *gcs;

	gcs = data;

	gtk_widget_hide(gcs->cs_dialog);

	if (gcs->set_color_func)
		(*gcs->set_color_func) (gcs, gcs->set_color_data);
} /* gnome_color_selector_ok_clicked */


static void
gnome_color_selector_cancel_clicked(GtkWidget *widget,
				    gpointer   data)
{
	GnomeColorSelector *gcs;

	gcs = data;

	gtk_widget_hide(gcs->cs_dialog);

	gnome_color_selector_set_color(gcs, gcs->tr, gcs->tg, gcs->tb);
} /* gnome_color_selector_cancel_clicked */


static void
gnome_color_selector_color_changed(GtkWidget *widget,
				   gpointer   data)
{
	GnomeColorSelector *gcs;
	GtkColorSelection  *cs;
	double              color[4];

	gcs = data;
	cs  = GTK_COLOR_SELECTION(widget);

	gtk_color_selection_get_color(cs, color);

	gcs->r = color[0];
	gcs->g = color[1];
	gcs->b = color[2];

	gnome_color_selector_fill_buffer(gcs->buf, PREVIEW_WIDTH, gcs->r, gcs->g, gcs->b);
	gnome_color_selector_render_preview(GTK_PREVIEW(gcs->preview), gcs->buf, PREVIEW_WIDTH, PREVIEW_HEIGHT);
} /* gnome_color_selector_color_changed */


static void
gnome_color_selector_button_clicked(GtkWidget *widget,
				    gpointer   data)
{
	GnomeColorSelector      *gcs;
	GtkColorSelectionDialog *csd;
	GtkColorSelection       *cs;

	gcs = data;

	if (!gcs->cs_dialog) {
		gtk_widget_push_visual (gtk_preview_get_visual ());
		gtk_widget_push_colormap (gtk_preview_get_cmap ());

		gcs->cs_dialog = gtk_color_selection_dialog_new("Pick a color");

		gtk_widget_pop_colormap ();
		gtk_widget_pop_visual ();

		csd = GTK_COLOR_SELECTION_DIALOG(gcs->cs_dialog);
		cs  = GTK_COLOR_SELECTION(csd->colorsel);

		gtk_color_selection_set_opacity(cs, FALSE);

		gtk_signal_connect(GTK_OBJECT(cs), "color_changed",
				   (GtkSignalFunc) gnome_color_selector_color_changed,
				   gcs);

		gtk_signal_connect(GTK_OBJECT(csd->ok_button), "clicked",
				   (GtkSignalFunc) gnome_color_selector_ok_clicked,
				   gcs);

		gtk_signal_connect(GTK_OBJECT(csd->cancel_button), "clicked",
				   (GtkSignalFunc) gnome_color_selector_cancel_clicked,
				   gcs);

		gtk_window_set_position(GTK_WINDOW(csd), GTK_WIN_POS_MOUSE);
	} /* if */

	if (!GTK_WIDGET_VISIBLE(gcs->cs_dialog)) {
		gnome_color_selector_get_color(gcs, &gcs->tr, &gcs->tg, &gcs->tb);  /* Save for cancel */

		/* FIXME: this is a hack; we set the color twice so
                 * that the color selector remembers it as its `old'
                 * color, too */
		
		gnome_color_selector_set_cs_color(gcs);
		gnome_color_selector_set_cs_color(gcs);

		gtk_widget_show(gcs->cs_dialog);
	} /* if */
} /* gnome_color_selector_button_clicked */

static void
color_dropped (GtkWidget        *widget,
	       GdkDragContext   *context,
	       gint              x,
	       gint              y,
	       GtkSelectionData *selection_data,
	       guint             info,
	       guint             time,
	       gpointer          data)
{
	GnomeColorSelector *gcs = data;
	unsigned int i;

	if ((selection_data->format == 16) && (selection_data->length == 8)) {
		guint16 *dropped = (guint16 *)selection_data->data;
		for (i = 0; i < 4; i++)
			printf ("%d ", dropped[i]);
		printf("\n");
	
		gnome_color_selector_set_color (gcs, 
						(gdouble)dropped [0]/0xffff, 
						(gdouble)dropped [1]/0xffff, 
						(gdouble)dropped [2]/0xffff);
	} else
		g_warning ("Invalid color dropped!");
}

GnomeColorSelector *
gnome_color_selector_new(SetColorFunc set_color_func,
			 gpointer     set_color_data)
{
	GnomeColorSelector *gcs;
	GtkWidget          *alignment;
	GtkWidget          *frame;
	static GtkTargetEntry drop_types = { "application/x-color", 0, 0 };

	g_warning ("GnomeColorSelector is now deprecated.  You should use GnomeColorPicker instead.");

	gcs = g_malloc(sizeof(GnomeColorSelector));

	gcs->cs_dialog = NULL;

	gcs->buf = g_malloc(PREVIEW_WIDTH * 3 * sizeof(guchar));

	gcs->set_color_func = set_color_func;
	gcs->set_color_data = set_color_data;
	
	gcs->button = gtk_button_new();
	gtk_signal_connect(GTK_OBJECT(gcs->button), "clicked",
			   (GtkSignalFunc) gnome_color_selector_button_clicked,
			   gcs);

	alignment = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
	gtk_container_set_border_width(GTK_CONTAINER(alignment), 1);
	gtk_container_add(GTK_CONTAINER(gcs->button), alignment);
	gtk_widget_show(alignment);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(alignment), frame);
	gtk_widget_show(frame);

	gtk_widget_push_visual (gtk_preview_get_visual ());
	gtk_widget_push_colormap (gtk_preview_get_cmap ());

	gcs->preview = gtk_preview_new(GTK_PREVIEW_COLOR);

	gtk_drag_dest_set (gcs->preview,
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   &drop_types, 1, GDK_ACTION_COPY);

	gtk_signal_connect (GTK_OBJECT (gcs->preview), "drag_data_received",
			    GTK_SIGNAL_FUNC (color_dropped), gcs);

	gtk_preview_size(GTK_PREVIEW(gcs->preview), PREVIEW_WIDTH, PREVIEW_HEIGHT);

	gtk_widget_pop_colormap ();
	gtk_widget_pop_visual ();
	
	gtk_container_add(GTK_CONTAINER(frame), gcs->preview);
	gtk_widget_show(gcs->preview);

	gnome_color_selector_set_color(gcs, 0.0, 0.0, 0.0);

	return gcs;
} /* gnome_color_selector_new */


void
gnome_color_selector_destroy(GnomeColorSelector *gcs)
{
	g_return_if_fail(gcs != NULL);

	if (gcs->buf)
		g_free(gcs->buf);

	if (gcs->cs_dialog)
		gtk_widget_destroy(gcs->cs_dialog);

	g_free(gcs);
} /* gnome_color_selector_destroy */


GtkWidget *
gnome_color_selector_get_button(GnomeColorSelector *gcs)
{
	g_return_val_if_fail(gcs != NULL, NULL);

	return gcs->button;
} /* gnome_color_selector_get_button */


void
gnome_color_selector_set_color(GnomeColorSelector *gcs,
			       double              r,
			       double              g,
			       double              b)
{
	g_return_if_fail(gcs != NULL);

	gcs->r = CLAMP(r, 0.0, 1.0);
	gcs->g = CLAMP(g, 0.0, 1.0);
	gcs->b = CLAMP(b, 0.0, 1.0);

	gnome_color_selector_fill_buffer(gcs->buf, PREVIEW_WIDTH, gcs->r, gcs->g, gcs->b);
	gnome_color_selector_render_preview(GTK_PREVIEW(gcs->preview), gcs->buf, PREVIEW_WIDTH, PREVIEW_HEIGHT);

	gnome_color_selector_set_cs_color(gcs);
} /* gnome_color_selector_set_color */


void
gnome_color_selector_set_color_int(GnomeColorSelector *gcs,
				   int                 r,
				   int                 g,
				   int                 b,
				   int                 scale)
{
	g_return_if_fail(gcs != NULL);

	gnome_color_selector_set_color(gcs,
				       (double) r / scale,
				       (double) g / scale,
				       (double) b / scale);
} /* gnome_color_selector_set_color_int */


void
gnome_color_selector_get_color(GnomeColorSelector *gcs,
			       double             *r,
			       double             *g,
			       double             *b)
{
	g_return_if_fail(gcs != NULL);
	g_return_if_fail(r != NULL);
	g_return_if_fail(g != NULL);
	g_return_if_fail(b != NULL);

	*r = gcs->r;
	*g = gcs->g;
	*b = gcs->b;
} /* gnome_color_selector_get_color */


void
gnome_color_selector_get_color_int(GnomeColorSelector *gcs,
				   int                *r,
				   int                *g,
				   int                *b,
				   int                 scale)
{
	g_return_if_fail(gcs != NULL);
	g_return_if_fail(r != NULL);
	g_return_if_fail(g != NULL);
	g_return_if_fail(b != NULL);

	*r = (int) (gcs->r * scale + 0.5);
	*g = (int) (gcs->g * scale + 0.5);
	*b = (int) (gcs->b * scale + 0.5);
} /* gnome_color_selector_get_color_int */
