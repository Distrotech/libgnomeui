/* My vague and ugly attempt at adding popup menus to GnomeUI */
/* By: Mark Crichton <mcrichto@purdue.edu> */
/* Written under the heavy infulence of whatever was playing
 * in my CDROM drive at the time... */
/* First pass: July 8, 1998 */

#include <gtk/gtk.h>
#include <libgnome/gnome-defs.h>

gnome_app_create_popup_menus(GnomeApp *, GtkWidget *, GnomeUIInfo *, gpointer *);
gnome_app_create_popup_menus_custom(GnomeApp *,
				    GtkWidget *,
				    GnomeUIInfo *,
       				    gpointer *,
			            GnomeUIBuilderData *);
