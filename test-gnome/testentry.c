
#include <config.h>

#include <libgnomeui.h>

gint
main (gint argc, gchar **argv)
{
	GtkWidget *window, *entry, *frame, *vbox;
	GnomeProgram *program;
	
	program = gnome_program_init ("testentry", "1.0",
			    libgnomeui_module_info_get (),
			    argc, argv,
			    NULL);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect (window, "delete_event",
			  G_CALLBACK (gtk_main_quit), NULL);

	vbox = gtk_vbox_new (FALSE, 0);

	entry = gnome_entry_new ("test-entry-history");
	frame = gtk_frame_new ("Entry");
	gtk_container_add (GTK_CONTAINER (frame), entry);
	gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

	entry = gnome_file_entry_new ("test-file-entry-history", "Select a file");
	frame = gtk_frame_new ("File Entry");
	gtk_container_add (GTK_CONTAINER (frame), entry);
	gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

	entry = gnome_pixmap_entry_new ("test-pixmap-entry-history", "Select a file", TRUE);
	frame = gtk_frame_new ("Pixmap Entry");
	gtk_container_add (GTK_CONTAINER (frame), entry);
	gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

	gtk_container_add (GTK_CONTAINER (window), vbox);
	gtk_widget_show_all (window);

	gtk_window_set_modal (GTK_WINDOW (window), TRUE);

	gtk_main ();
	g_object_unref (program);
	
	return 0;
}
