#include <stdlib.h>

#include <gtk/gtkmain.h>

#include <glade/glade-xml.h>

#include <libgnomeui/gnome-ui-init.h>

int
main (int argc, char *argv[])
{
	GladeXML *xml;
#if 1
	GLogLevelFlags fatal_mask;

	fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
	g_log_set_always_fatal (fatal_mask | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);
#endif

	gnome_program_init ("test-libglade-gnome-noupgrade", "0.1",
			    LIBGNOMEUI_MODULE,
			    argc, argv, NULL);

	xml = glade_xml_new ("test-libglade-gnome-noupgrade.glade2", NULL, NULL);

	g_assert (xml != NULL);

	if (getenv ("TEST_LIBGLADE_SHOW")) {
		GtkWidget *toplevel;
		const char *windows[] = { "app1", "dialog1", "messagebox1", "propertybox1", "about2", "window1", NULL };
		int i;

		for (i = 0; windows[i]; i++) {
			toplevel = glade_xml_get_widget (xml, windows[i]);
			if (!toplevel)
				continue;
			g_signal_connect (G_OBJECT (toplevel), "delete_event",
					  G_CALLBACK (gtk_main_quit), NULL);
			gtk_widget_show_all (toplevel);
			gtk_main ();
		}
	}

	g_object_unref (G_OBJECT (xml));

	return 0;
}
