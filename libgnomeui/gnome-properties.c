#include <gtk/gtk.h>
#include <string.h>
#include "libgnome/gnome-defs.h"
#include "gnome-properties.h"
#include "gnome-actionarea.h"

GnomePropertyConfigurator *
gnome_property_configurator_new (void)
{
	GnomePropertyConfigurator *this = g_malloc (sizeof (GnomePropertyConfigurator));

	this->props = NULL;
	this->notebook = NULL;

	return this;
}

void
gnome_property_configurator_destroy (GnomePropertyConfigurator *this)
{
	if (this->notebook)
		gtk_widget_destroy (this->notebook);
	g_list_free (this->props);
}



void
gnome_property_configurator_register (GnomePropertyConfigurator *this,
				      int (*callback)(GnomePropertyRequest))
{
	this->props = g_list_append (this->props, callback);
}

void
gnome_property_configurator_setup (GnomePropertyConfigurator *this)
{
	this->notebook = gtk_notebook_new ();
	gtk_widget_show (this->notebook);
}

gint
gnome_property_configurator_request (GnomePropertyConfigurator *th,
				     GnomePropertyRequest r)
{
	int (*cb)(GnomePropertyRequest);

	if (!th->notebook)
		return 0;

	cb = (int (*)(GnomePropertyRequest))
		(g_list_nth
		 (th->props,
		  gtk_notebook_current_page (GTK_NOTEBOOK (th->notebook)))->data);


	/* printf ("request %d %x\n", gtk_notebook_current_page (th->notebook), cb); */

	if (cb)
		return (*cb) (r);
	else
		return 0;
}

static void
request (int (*cb)(GnomePropertyRequest), GnomePropertyRequest r)
{
	(*cb) (r);
}

gint
gnome_property_configurator_request_foreach (GnomePropertyConfigurator *th,
					     GnomePropertyRequest r)
{
	g_list_foreach (th->props, (GFunc)request, (gpointer)r);
}
