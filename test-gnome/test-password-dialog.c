
#include <config.h>

#include <gtk/gtk.h>
#include <libgnomeui.h>

static GtkWidget *password_dialog = NULL;

static void
authenticate_boink_callback (GtkWidget *button, gpointer user_data)
{
	gboolean  result;
	char	  *username;
	char	  *password;

	if (password_dialog == NULL) {
		password_dialog = gnome_password_dialog_new ("Authenticate Me",
							     "My secret message.",
							     "foouser",
							     "sekret",
							     TRUE);
	}

	result = gnome_password_dialog_run_and_block (GNOME_PASSWORD_DIALOG (password_dialog));

	username = gnome_password_dialog_get_username (GNOME_PASSWORD_DIALOG (password_dialog));
	password = gnome_password_dialog_get_password (GNOME_PASSWORD_DIALOG (password_dialog));

	g_assert (username != NULL);
	g_assert (password != NULL);

	if (result) {
		g_print ("authentication submitted: username='%s' , password='%s'\n",
			 username,
			 password);
	}
	else {
		g_print ("authentication cancelled:\n");
	}
	
	g_free (username);
	g_free (password);
}

static void
exit_callback (GtkWidget *button, gpointer user_data)
{
	if (password_dialog != NULL) {
		gtk_widget_destroy (password_dialog);
	}

	gtk_main_quit ();
}

int
main (int argc, char * argv[])
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *authenticate_button;
	GtkWidget *exit_button;

	gnome_program_init ("test-gnome-password-dialog", VERSION,
			    libgnomeui_module_info_get (), argc, argv,
			    NULL);
	
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width (GTK_CONTAINER (window), 4);

	vbox = gtk_vbox_new (TRUE, 0);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	authenticate_button = gtk_button_new_with_label ("Boink me to authenticate");
	exit_button = gtk_button_new_with_label ("Exit");

	g_signal_connect (authenticate_button,
			    "clicked",
			    G_CALLBACK (authenticate_boink_callback),
			    NULL);

	g_signal_connect (exit_button,
			    "clicked",
			    G_CALLBACK (exit_callback),
			    NULL);

	gtk_box_pack_start (GTK_BOX (vbox), authenticate_button, TRUE, TRUE, 4);
	gtk_box_pack_end (GTK_BOX (vbox), exit_button, TRUE, TRUE, 0);
	
	gtk_widget_show_all (window);

	gtk_main ();

	return 0;
}
