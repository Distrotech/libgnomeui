#include <gtk/gtk.h>
#include <string.h>
#include "libgnome/gnome-defs.h"
#include "gnome-properties.h"
#include "gnome-actionarea.h"
#include "gnome-propertybox.h"

GnomePropertyConfigurator *
gnome_property_configurator_new (void)
{
	GnomePropertyConfigurator *this = g_malloc (sizeof (GnomePropertyConfigurator));

	this->props = NULL;
	this->property_box = NULL;

	return this;
}

void
gnome_property_configurator_destroy (GnomePropertyConfigurator *this)
{
	if (this->property_box)
		gtk_widget_destroy (this->property_box);
	g_list_free (this->props);
}



void
gnome_property_configurator_register (GnomePropertyConfigurator *this,
				      int (*callback)(GnomePropertyRequest))
{
	this->props = g_list_append (this->props, callback);
}

/* This is run when the user applies a change in the property box.  */
static void
apply_page (GtkObject *object, gint page, gpointer data)
{
	GnomePropertyConfigurator *this = (GnomePropertyConfigurator *) data;
	int (*cb)(GnomePropertyRequest);

	if (page == -1) {
		/* Applied all pages.  Now write and sync.  */
		gnome_property_configurator_request_foreach (this, GNOME_PROPERTY_WRITE);
		gnome_config_sync ();
		return;
	}

	cb = (int (*)(GnomePropertyRequest))
		(g_list_nth (this->props, page)->data);
	if (cb)
		(*cb) (GNOME_PROPERTY_APPLY);
}

void
gnome_property_configurator_setup (GnomePropertyConfigurator *this)
{
	this->property_box = gnome_property_box_new ();
	gtk_widget_show (this->property_box);

	gtk_signal_connect (GTK_OBJECT (this->property_box), "apply",
			    (GtkSignalFunc) apply_page, (gpointer) this);
}

static void
request (int (*cb)(GnomePropertyRequest), GnomePropertyRequest r)
{
	(*cb) (r);
}

void
gnome_property_configurator_request_foreach (GnomePropertyConfigurator *th,
					     GnomePropertyRequest r)
{
	g_list_foreach (th->props, (GFunc)request, (gpointer)r);
}
