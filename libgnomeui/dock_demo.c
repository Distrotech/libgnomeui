#include <gnome.h>

#include "gnome-dock.h"

static GtkWidget *app;
static GtkWidget *dock;
static GtkWidget *dock_items[6];
static GtkWidget *toolbars[6];
static GtkWidget *drawing_area;
static GtkWidget *client_frame;

static GnomeUIInfo toolbar_infos[6][10] = {
  { { GNOME_APP_UI_ITEM, "New", "Create a new file", NULL, NULL, NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_NEW, 0, 0, NULL },
    { GNOME_APP_UI_ITEM, "Open", "Open an existing file", NULL, NULL, NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_OPEN, 0, 0, NULL },
    { GNOME_APP_UI_ITEM, "Save", "Save the current file", NULL, NULL, NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SAVE, 0, 0, NULL },
    { GNOME_APP_UI_ITEM, "Save as", "Save the current file with a new name", NULL, NULL, NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SAVE_AS, 0, 0, NULL },
    GNOMEUIINFO_END },

  { { GNOME_APP_UI_ITEM, "Close", "Close the current file", NULL, NULL, NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_CLOSE, 0, 0, NULL },
    { GNOME_APP_UI_ITEM, "Exit", "Exit the program", NULL, NULL, NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_EXIT, 0, 0, NULL },
    GNOMEUIINFO_END },

  { { GNOME_APP_UI_ITEM, "Undo", "Undo the last operation", NULL, NULL, NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_UNDO, 0, 0, NULL },
    { GNOME_APP_UI_ITEM, "_Redo", "Redo the last undo-ed operation", NULL, NULL, NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_REDO, 0, 0, NULL },
    GNOMEUIINFO_END },

  { { GNOME_APP_UI_ITEM, "Cut", "Cut the selection to the clipboard", NULL, NULL, NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_CUT, 0, 0, NULL },
    { GNOME_APP_UI_ITEM, "Copy", "Copy the selection to the clipboard", NULL, NULL, NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_COPY, 0, 0, NULL },
    { GNOME_APP_UI_ITEM, "Paste", "Paste the contents of the clipboard", NULL, NULL, NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_PASTE, 0, 0, NULL },
    GNOMEUIINFO_END },

  { { GNOME_APP_UI_ITEM, "First", "Go to the first item", NULL, NULL, NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_FIRST, 0, 0, NULL },
    { GNOME_APP_UI_ITEM, "Last", "Go to the last item", NULL, NULL, NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_LAST, 0, 0, NULL },
    GNOMEUIINFO_END },
  { { GNOME_APP_UI_ITEM, "First", "Go to the first item", NULL, NULL, NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_FIRST, 0, 0, NULL },
    { GNOME_APP_UI_ITEM, "Last", "Go to the last item", NULL, NULL, NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_LAST, 0, 0, NULL },
    GNOMEUIINFO_END }

};

static void delete_callback (GtkWidget *w)
{
  gchar *layout_string;
  GnomeDockLayout *layout;

  layout = gnome_dock_get_layout (GNOME_DOCK (dock));
  layout_string = gnome_dock_layout_create_string (layout);
  gnome_config_set_string ("Dock/Placement", layout_string);
  gnome_config_sync ();

  gtk_object_unref (GTK_OBJECT (layout));

  gtk_main_quit ();
}

static void layout_changed_callback (GtkWidget *w)
{
  puts ("Layout changed");
}

static void
do_ui_signal_connect (GnomeUIInfo *uiinfo, gchar *signal_name, GnomeUIBuilderData *uibdata)
{
}

int
main (int argc, char **argv)
{
  GnomeUIBuilderData uibdata;
  GnomeDockLayout *layout;
  int i;

  /* I am having troubles with CVS gnome-libs today, so let's do
     things by hand.  */

  gnome_init ("dock_demo", "1.0", argc, argv);

  app = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (app), "Test!");
  gtk_window_set_default_size (GTK_WINDOW (app), 400, 400);

  dock = gnome_dock_new ();
  gtk_container_add (GTK_CONTAINER (app), dock);

  uibdata.connect_func = do_ui_signal_connect;
  uibdata.data = NULL;
  uibdata.is_interp = FALSE;
  uibdata.relay_func = NULL;
  uibdata.destroy_func = NULL;

  gnome_config_push_prefix ("/DockTest/");

  layout = gnome_dock_layout_new ();

  for (i = 0; i < 6; i++)
    {
      toolbars[i] = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL,
                                     GTK_TOOLBAR_ICONS);
      gtk_container_set_border_width (GTK_CONTAINER (toolbars[i]), 1);

      gnome_app_fill_toolbar_custom (GTK_TOOLBAR (toolbars[i]),
                                     toolbar_infos[i],
                                     &uibdata,
                                     NULL);

      if (i == 0)
        dock_items[i] = gnome_dock_item_new ("SomeBar",
                                             GNOME_DOCK_ITEM_BEH_EXCLUSIVE
                                             | GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL);
      else if (i == 1)
        dock_items[i] = gnome_dock_item_new ("LockedBar",
                                             GNOME_DOCK_ITEM_BEH_LOCKED);
      else
        {
          gchar *name;

          name = g_strdup_printf ("AnotherBar%d", i);
          dock_items[i] = gnome_dock_item_new (name,
                                               GNOME_DOCK_ITEM_BEH_NORMAL);
        }

      gtk_container_set_border_width (GTK_CONTAINER (dock_items[i]), 1);
      gtk_container_add (GTK_CONTAINER (dock_items[i]), toolbars[i]);

      if (i < 3)
        gnome_dock_layout_add_item (layout,
                                    GNOME_DOCK_ITEM (dock_items[i]),
                                    GNOME_DOCK_TOP,
                                    i, 0, 0);
      else
        gnome_dock_layout_add_item (layout,
                                    GNOME_DOCK_ITEM (dock_items[i]),
                                    GNOME_DOCK_BOTTOM,
                                    i - 4, 0, 0);

      gtk_widget_show (toolbars[i]);
      gtk_widget_show (dock_items[i]);
    }

  {
    gchar *layout_string;

    layout_string = gnome_config_get_string ("Dock/Placement");
    gnome_dock_layout_parse_string (GNOME_DOCK_LAYOUT (layout),
                                    layout_string);
    g_free (layout_string);
  }

  gnome_dock_layout_add_to_dock (GNOME_DOCK_LAYOUT (layout),
                                 GNOME_DOCK (dock));

  gtk_object_unref (GTK_OBJECT(layout));

  client_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type(GTK_FRAME (client_frame), GTK_SHADOW_IN);
  drawing_area = gtk_drawing_area_new();

  gtk_signal_connect (GTK_OBJECT (app),
                      "delete_event",
                      GTK_SIGNAL_FUNC (delete_callback),
                      NULL);

  gtk_signal_connect (GTK_OBJECT (dock),
                      "layout_changed",
                      GTK_SIGNAL_FUNC (layout_changed_callback),
                      NULL);

  gnome_dock_set_client_area (GNOME_DOCK (dock), client_frame);
  gtk_widget_show (client_frame);
  gtk_widget_show (drawing_area);
  gtk_widget_show (dock);

  gtk_widget_show (app);
  gtk_main ();

  return 0;
}

