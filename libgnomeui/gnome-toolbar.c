/* GNOME GUI Library
 * Copyright (C) 1997 The Free Software Foundation
 *
 * Author: Aldy Hernandez (aldy@uaa.edu)
 *
 * GNOME toolbar implementation
 */

#include <gtk/gtk.h>
#include "gnome-toolbar.h"
#include "libgnome/gnome-defs.h"
#include "libgnomeui/gnome-pixmap.h"

/* Make sure these match ``enum ToolbarDefaultItem'' */
static struct {
  char *label;
  char *pixmap_filename;
} ToolbarDefaultItems [] =
  {
    { "Open",	"gnome-open-icon.xpm" },
    { "Close",	"gnome-close-icon.xpm" },
    { "Reload",	"gnome-reload-icon.xpm" },
    { "Print",	"gnome-print-icon.xpm" },
    { "Exit",	"gnome-exit-icon.xpm" }
  };

GnomeToolbar *
gnome_create_toolbar (void *parent,
		      enum GnomePackMethod packmethod)
{
	GnomeToolbar *tb;
	
	tb = (GnomeToolbar *) g_malloc (sizeof (GnomeToolbar));
	tb->toolbar = NULL;
	tb->box = gtk_hbox_new (FALSE, 0);
	tb->style = GNOME_TB_NOTHING;
	tb->items = NULL;
	tb->nitems = 0;
	
	switch (packmethod){
	case GNOME_TB_CONTAINER_ADD:
		gtk_container_add (GTK_CONTAINER ((GtkWidget *)parent), tb->box);
		break;
		
	case GNOME_TB_PACK_START:
		gtk_box_pack_start (GTK_BOX ((GtkWidget *) parent), tb->box, FALSE, FALSE, 0);
		break;
		
	case GNOME_TB_PACK_END:
		gtk_box_pack_end (GTK_BOX ((GtkWidget *) parent), tb->box, FALSE, FALSE, 0);
		break;
		
	default:
		printf ("gnome_create_toolbar: invalid packmethod %d\n", packmethod);
	}
	
	gtk_container_border_width (GTK_CONTAINER (tb->box), 2);
	gtk_widget_show (tb->box);
	
	return tb;
}

/* Destroy toolbar permanently */
void
gnome_destroy_toolbar (GnomeToolbar *toolbar)
{
	gtk_widget_destroy (toolbar->box);
}

void
gnome_toolbar_add (GnomeToolbar *toolbar, char *label, char *pixmap_filename,
		   GnomeToolbarFunc func, void *data)
{
	int i = toolbar->nitems;
	
	toolbar->items = g_realloc (toolbar->items, sizeof (GnomeToolbarItem) * (toolbar->nitems + 1));
	
	toolbar->items[i].label = label;
	toolbar->items[i].func = func;
	toolbar->items[i].data = data;
	toolbar->items[i].pixmap_filename = pixmap_filename;
	++toolbar->nitems;
}

void
gnome_toolbar_add_default (GnomeToolbar *toolbar,
			   enum GnomeToolbarDefaultItem tb_default,
			   GnomeToolbarFunc func, void *data)
{
	gnome_toolbar_add (toolbar, ToolbarDefaultItems[tb_default].label,
			   ToolbarDefaultItems[tb_default].pixmap_filename,
			   func, data);
}

void
gnome_toolbar_set_style (GnomeToolbar *toolbar,
			 enum GnomeToolbarStyle style)
{
	int i;
	
	toolbar->style = style;
	
	if (toolbar->toolbar)
		gtk_widget_destroy (toolbar->toolbar);
	
	if (style & GNOME_TB_VERTICAL)
		toolbar->toolbar = gtk_vbox_new (FALSE, 0);
	else
		toolbar->toolbar = gtk_hbox_new (FALSE, 0);
	
	gtk_container_border_width (GTK_CONTAINER (toolbar->toolbar), 3);
	gtk_box_pack_start (GTK_BOX (toolbar->box), toolbar->toolbar, FALSE, FALSE, 3);
	
	for (i=0; i < toolbar->nitems; ++i){
		GtkWidget *button;
		GtkWidget *vbox;
		GtkWidget *label = NULL;
		GtkWidget *pixmap = NULL;
		
		button = gtk_button_new ();
		vbox = gtk_vbox_new (FALSE, 0);
		
		if (style & GNOME_TB_TEXT && toolbar->items[i].label)
			label = gtk_label_new (toolbar->items[i].label);
		
		if (style & GNOME_TB_ICONS && toolbar->items[i].pixmap_filename)
			pixmap = gnome_create_pixmap_widget (toolbar->box,
							     toolbar->box,
							     toolbar->items[i].pixmap_filename);
		
		if (style & GNOME_TB_ICONS && !pixmap) 
			label = gtk_label_new ("undefined");
		
		gtk_container_add (GTK_CONTAINER (button), vbox);
		gtk_container_border_width (GTK_CONTAINER (button), 1);
		
		if (pixmap) {		/* Place pixmap */
			gtk_box_pack_start (GTK_BOX (vbox), pixmap, FALSE, FALSE, 0);
			gtk_widget_show (pixmap);
		}
		
		if (label) {		/* Place label */
			gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
			gtk_widget_show (label);
		}
		
		gtk_box_pack_start (GTK_BOX (toolbar->toolbar), button, FALSE, FALSE, 0);
		gtk_widget_show (button);
		gtk_widget_show (vbox);
		
		gtk_signal_connect (GTK_OBJECT (button), "clicked",
				    (GtkSignalFunc) toolbar->items[i].func,
				    toolbar->items[i].data);
	}
	gtk_widget_show (toolbar->toolbar);
}

