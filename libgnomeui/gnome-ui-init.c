#include <config.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include "libgnome/libgnome.h"
#include "gnome-colors.h"

static void gnome_rc_parse(gchar *command);

void
gnome_init (gint *argc, gchar ***argv)
{
	/* now we replace gtk_init() with gnome_init() in our apps */
	gtk_init(argc, argv);
	gnome_colors_init();
	
	gnome_rc_parse(*argv[0]);
	
	gnomelib_init (argc, argv);
}

/* perhaps this belongs in libgnome.. move it if you like. */

/* automagically parse all the gtkrc files for us.
 * 
 * Parse:
 * $gnomedatadir/gtkrc
 * $gnomedatadir/$apprc
 * ~/.ghome/gtkrc
 * ~/.gnome/$apprc
 *
 * appname is derived from argv[0].  IMHO this is a great solution.
 * It provides good consistancy (you always know the rc file will be
 * the same name as the executable), and it's easy for the programmer.
 * 
 * If you don't like it.. give me a good reason.  Symlin
 */
static void gnome_rc_parse(gchar *command)
{
	gint i;
	gint buf_len;
	gint found = 0;
	gchar *buf = NULL;
	gchar *file;
	gchar *apprc;
	
	buf_len = strlen(command);
	
	for (i = 0; i < buf_len; i++) {
		if (command[buf_len - i] == '/') {
			buf = g_strdup (&command[buf_len - i + 1]);
			found = TRUE;
			break;
		}
	}
	
	if (!found)
		buf = g_strdup (command);
	
	apprc = g_malloc (strlen(buf) + 3);
	sprintf(apprc, "%src", buf);
	
	g_free(buf);
	
	
	/* <gnomedatadir>/gtkrc */
	file = gnome_datadir_file("gtkrc");
	if (file){
		gtk_rc_parse (file);
		g_free (file);
	}

	/* <gnomedatadir>/<progname> */
	file = gnome_datadir_file(apprc);
	if (file){
		gtk_rc_parse (file);
		g_free (file);
	}
	
	/* ~/.gnome/gtkrc */
	file = gnome_util_home_file("gtkrc");
	if (file){
		gtk_rc_parse (file);
		g_free (file);
	}
	
	/* ~/.gnome/<progname> */
	file = gnome_util_home_file(apprc);
	if (file){
		gtk_rc_parse (file);
		g_free (file);
	}
	
	g_free (apprc);
}
