
#include <config.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include <libgnomeui.h>
#include <libgnomeui/gnome-druid.h>
#include <libgnomeui/gnome-druid-page.h>
#include <libgnomeui/gnome-druid-page-standard.h>


int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *druid;
  GtkWidget *druid_page;
  gtk_init (&argc, &argv);
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  druid = gnome_druid_new ();
  gtk_container_add (GTK_CONTAINER (window), druid);
  druid_page = gnome_druid_page_standard_new_with_vals ("Test Druid", NULL, NULL);
  gnome_druid_append_page (GNOME_DRUID (druid), GNOME_DRUID_PAGE (druid_page));
  gnome_druid_page_standard_append_item (GNOME_DRUID_PAGE_STANDARD (druid_page),
					 "Test _one:",
					 gtk_entry_new (),
					 "Longer information here");
  gnome_druid_page_standard_append_item (GNOME_DRUID_PAGE_STANDARD (druid_page),
					 "Test _two:",
					 gtk_entry_new (),
					 "Longer information here");
  gnome_druid_page_standard_append_item (GNOME_DRUID_PAGE_STANDARD (druid_page),
					 "Test t_hree:",
					 gtk_entry_new (),
					 "Longer information here");
  gnome_druid_page_standard_append_item (GNOME_DRUID_PAGE_STANDARD (druid_page),
					 "Test fou_r:",
					 gtk_entry_new (),
					 "Longer information here");
  gtk_widget_show_all (window);

  gtk_main ();
  return 0;
}
