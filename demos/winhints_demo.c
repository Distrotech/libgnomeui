#include <config.h>
#include <gnome.h>
#include <gdk/gdkx.h>
#include "gnome-winhints.h"


void layers_cb (GtkWidget *widget, void *data);
void about_cb (GtkWidget *widget, void *data);
void state_cb (GtkWidget *widget, void *data);
void skip_cb (GtkWidget *widget, void *data);
void protocols_cb (GtkWidget *widget, void *data);
void workspace_cb (GtkWidget *widget, void *data);
void quit_cb (GtkWidget *widget, void *data);
static GList* get_workspaces(void);
static void prepare_app(void);

GtkWidget *app;
GtkWidget *lbox, *wsbox;
GtkWidget *allwsbut;
GList *ws_list;
GtkWidget *minbut, *maxvbut, *maxhbut, *hidbut, *rollbut, *dockbut;
GtkWidget *sfocbut, *smenubut, *sbarbut, *fixedbut, *ignorebut;

int main(int argc, char *argv[])
{
  gnome_init ("winhints-test", VERSION, argc, argv);
  
  prepare_app ();
  
  gtk_main ();
  
  return 0;
}

static void prepare_app(void)
{
  GtkWidget *vb,*vb1, *hb, *hb1, *mvb;
  GtkWidget *sw;
  GtkWidget *frame;
  GtkWidget *item;
  GtkWidget *button;
  GtkWidget *apic, *label;
  GList *tmp_list;
  
  
  app = gnome_app_new (_("wmhints-test"), _("winhints-test"));
  gtk_widget_realize (app);
  gtk_signal_connect (GTK_OBJECT (app), "delete_event",
                      GTK_SIGNAL_FUNC (quit_cb),
                      NULL);
  
  /* this must be called after the app widget is realized */

  gnome_win_hints_init();
  if (!gnome_win_hints_wm_exists())
    fprintf(stderr, "Your WM does not support GNOME extended hints\n");
  
  mvb=gtk_vbox_new(FALSE, 0);
  gnome_app_set_contents ( GNOME_APP (app), mvb);
  gtk_widget_show (mvb);
  gtk_widget_realize(app);

  hb=gtk_hbox_new(TRUE, 0);
  gtk_box_pack_start(GTK_BOX(mvb), hb, FALSE, FALSE, 0);
  gtk_widget_show (hb);

  vb =gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(hb), vb, FALSE, FALSE, 0);
  gtk_widget_show(vb);

  frame=gtk_frame_new(_("Layer:"));
  gtk_box_pack_start(GTK_BOX(vb), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
  gtk_widget_show(frame);

  vb1 =gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(frame), vb1);
  gtk_container_set_border_width(GTK_CONTAINER(vb1), 5);
  gtk_widget_show(vb1);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_usize(sw, 100, 100);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(vb1), sw, FALSE, FALSE, 0);
  gtk_widget_show(sw);

  lbox = gtk_list_new();
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), lbox);
  gtk_widget_show(lbox);

  tmp_list=NULL;
  
  item = gtk_list_item_new_with_label( _("Above Dock"));
  tmp_list=g_list_prepend(tmp_list, item);
  gtk_object_set_data(GTK_OBJECT(item), "layer", "10");
  gtk_widget_show(item);
  item = gtk_list_item_new_with_label( _("Dock"));
  tmp_list=g_list_prepend(tmp_list, item);
  gtk_object_set_data(GTK_OBJECT(item), "layer", "8");
  gtk_widget_show(item);
  item = gtk_list_item_new_with_label( _("On Top"));
  tmp_list=g_list_prepend(tmp_list, item);
  gtk_object_set_data(GTK_OBJECT(item), "layer", "6");
  gtk_widget_show(item);
  item = gtk_list_item_new_with_label( _("Normal"));
  tmp_list=g_list_prepend(tmp_list, item);
  gtk_object_set_data(GTK_OBJECT(item), "layer", "4");
  gtk_widget_show(item);
  item = gtk_list_item_new_with_label( _("Below"));
  tmp_list=g_list_prepend(tmp_list, item);
  gtk_object_set_data(GTK_OBJECT(item), "layer", "2");
  gtk_widget_show(item);
  item = gtk_list_item_new_with_label( _("Desktop"));
  tmp_list=g_list_prepend(tmp_list, item);
  gtk_object_set_data(GTK_OBJECT(item), "layer", "0");
  gtk_widget_show(item);

  gtk_list_insert_items(GTK_LIST(lbox), tmp_list, -1);
  gtk_list_select_item(GTK_LIST(lbox), 2);
  
  button = gtk_button_new_with_label(_("Set Layer"));
  gtk_box_pack_start(GTK_BOX(vb1), button, FALSE, FALSE, 2);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(layers_cb), lbox);
  gtk_widget_show(button);


  frame=gtk_frame_new(_("Workspaces:"));
  gtk_box_pack_start(GTK_BOX(vb), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
  gtk_widget_show(frame);
  
  vb1 =gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(frame), vb1);
  gtk_container_set_border_width(GTK_CONTAINER(vb1), 5);
  gtk_widget_show(vb1);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_usize(sw, 100, 100);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(vb1), sw, FALSE, FALSE, 0);
  gtk_widget_show(sw);

  wsbox = gtk_list_new();
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), wsbox);
  gtk_list_append_items(GTK_LIST(wsbox), get_workspaces());
  gtk_widget_show(wsbox);

  allwsbut = gtk_check_button_new_with_label(_("Sticky"));
  gtk_box_pack_start(GTK_BOX(vb1), allwsbut, FALSE, FALSE, 0);
  gtk_widget_show(allwsbut);

  button = gtk_button_new_with_label(_("Set Workspace"));
  gtk_box_pack_start(GTK_BOX(vb1), button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(workspace_cb), lbox);
  gtk_widget_show(button);

  vb =gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(hb), vb, TRUE, TRUE, 0);
  gtk_widget_show(vb);

  frame=gtk_frame_new(_("State Toggles:"));
  gtk_box_pack_start(GTK_BOX(vb), frame, FALSE, TRUE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
  gtk_widget_show(frame);

  vb1 =gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(frame), vb1);
  gtk_container_set_border_width(GTK_CONTAINER(vb1), 5);
  gtk_widget_show(vb1);

  minbut = gtk_check_button_new_with_label(_("Minimized"));
  gtk_box_pack_start(GTK_BOX(vb1), minbut, FALSE, FALSE, 0);
  gtk_widget_show(minbut);

  maxvbut = gtk_check_button_new_with_label(_("Maximized Vertical"));
  gtk_box_pack_start(GTK_BOX(vb1), maxvbut, FALSE, FALSE, 0);
  gtk_widget_show(maxvbut);

  maxhbut = gtk_check_button_new_with_label(_("Maximized Horizontal"));
  gtk_box_pack_start(GTK_BOX(vb1), maxhbut, FALSE, FALSE, 0);
  gtk_widget_show(maxhbut);

  hidbut = gtk_check_button_new_with_label(_("Hidden"));
  gtk_box_pack_start(GTK_BOX(vb1), hidbut, FALSE, FALSE, 0);
  gtk_widget_show(hidbut);

  rollbut = gtk_check_button_new_with_label(_("Shaded"));
  gtk_box_pack_start(GTK_BOX(vb1), rollbut, FALSE, FALSE, 0);
  gtk_widget_show(rollbut);

  fixedbut = gtk_check_button_new_with_label(_("Fixed Position"));
  gtk_box_pack_start(GTK_BOX(vb1), fixedbut, FALSE, FALSE, 0);
  gtk_widget_show(fixedbut);

  ignorebut = gtk_check_button_new_with_label(_("Ignore on Arrange"));
  gtk_box_pack_start(GTK_BOX(vb1), ignorebut, FALSE, FALSE, 0);
  gtk_widget_show(ignorebut);
  
  button = gtk_button_new_with_label(_("Set State"));
  gtk_box_pack_start(GTK_BOX(vb1), button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(state_cb), lbox);
  gtk_widget_show(button);

  frame=gtk_frame_new(_("Skip Toggles:"));
  gtk_box_pack_start(GTK_BOX(vb), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
  gtk_widget_show(frame);

  vb1 =gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(frame), vb1);
  gtk_container_set_border_width(GTK_CONTAINER(vb1), 5);
  gtk_widget_show(vb1);

  sfocbut = gtk_check_button_new_with_label(_("Skip Focus"));
  gtk_box_pack_start(GTK_BOX(vb1), sfocbut, FALSE, FALSE, 0);
  gtk_widget_show(sfocbut);

  smenubut = gtk_check_button_new_with_label(_("Skip Window Menu"));
  gtk_box_pack_start(GTK_BOX(vb1), smenubut, FALSE, FALSE, 0);
  gtk_widget_show(smenubut);

  sbarbut = gtk_check_button_new_with_label(_("Skip Taskbar / WinList"));
  gtk_box_pack_start(GTK_BOX(vb1), sbarbut, FALSE, FALSE, 0);
  gtk_widget_show(sbarbut);

  button = gtk_button_new_with_label(_("Set Skip"));
  gtk_box_pack_start(GTK_BOX(vb1), button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(skip_cb), lbox);
  gtk_widget_show(button);

  hb1 = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(mvb), hb1, FALSE, FALSE, 0);
  gtk_widget_show(hb1);

  button = gtk_button_new();
  gtk_box_pack_end(GTK_BOX(hb1), button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(about_cb), lbox);
  gtk_widget_show(button);

  hb = gtk_hbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(hb), 5);
  gtk_container_add(GTK_CONTAINER(button), hb);
  gtk_widget_show(hb);

  apic = gnome_stock_pixmap_widget_new(GTK_WIDGET(app), GNOME_STOCK_PIXMAP_HELP);
  gtk_box_pack_start(GTK_BOX(hb), apic, FALSE, FALSE, 0);
  gtk_widget_show(apic);

  label = gtk_label_new(_("About"));
  gtk_box_pack_end(GTK_BOX(hb), label, FALSE, FALSE, 5);
  gtk_widget_show(label);
  
  button = gnome_stock_button(GNOME_STOCK_BUTTON_CLOSE);
  gtk_box_pack_end(GTK_BOX(hb1), button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(quit_cb), lbox);
  gtk_widget_show(button);
    
  gtk_widget_show (app);
}

void layers_cb (GtkWidget *widget, void *data)
{
  GtkWidget *list;
  GnomeWinLayer new_layer;
  
  list = (GtkWidget*)data;
  if((GTK_LIST(list)->selection) == NULL)
    {
      return;
    }
  switch(atoi(gtk_object_get_data(GTK_OBJECT(GTK_LIST(list)->selection->data), "layer")))
    {
     case 0:
      new_layer = WIN_LAYER_DESKTOP;
      break;
     case 2:
      new_layer = WIN_LAYER_BELOW;
      break;
     case 4:
      new_layer = WIN_LAYER_NORMAL;
      break;
     case 6:
      new_layer = WIN_LAYER_NORMAL;
      break;
     case 8:
      new_layer = WIN_LAYER_ONTOP;
      break;
     case 10:
      new_layer = WIN_LAYER_DOCK;
      break;
     default:
      new_layer = WIN_LAYER_NORMAL;
    }
  
  gnome_win_hints_set_layer(GTK_WIDGET(app), new_layer);
  
  return;
}

void quit_cb (GtkWidget *widget, void *data)
{
  gtk_main_quit ();
  return;
}

void workspace_cb (GtkWidget *widget, void *data)
{
  char *tmpbuf;
  gint ws;
  
  /* set all workspaces - state change */
  if(GTK_TOGGLE_BUTTON(allwsbut)->active)
    gnome_win_hints_set_state(GTK_WIDGET(app), WIN_STATE_STICKY);
  else
    gnome_win_hints_set_state(GTK_WIDGET(app), 0);
  
  /* change workspace to selection */
  if(GTK_LIST(wsbox)->selection == NULL)
    return;
  tmpbuf = gtk_object_get_data(GTK_OBJECT(GTK_LIST(wsbox)->selection->data), "ws");
  ws = atoi(tmpbuf);
  gnome_win_hints_set_workspace(GTK_WIDGET(app), ws);
  if(GTK_TOGGLE_BUTTON(allwsbut)->active)
    gnome_win_hints_set_state(GTK_WIDGET(app), WIN_STATE_STICKY);
  else
    gnome_win_hints_set_state(GTK_WIDGET(app), 0);
  
  return;
}

static GList *get_workspaces(void)
{
  GList *tmp_list, *tmp_list2, *ptr;
  gint i;
  GtkWidget *item;
  char tmpbuf[1024];
  
  tmp_list2 = NULL;
  tmp_list = gnome_win_hints_get_workspace_names();
  
  ptr = tmp_list;
  i = 0;
  while(ptr)
    {
      item = gtk_list_item_new_with_label(ptr->data);
      gtk_widget_show(item);
      g_snprintf(tmpbuf, sizeof(tmpbuf), "%d", i++);
      gtk_object_set_data(GTK_OBJECT(item), "ws", g_strdup(tmpbuf));
      tmp_list2 = g_list_append(tmp_list2, item);
      ptr = ptr->next;
    }
  return tmp_list2;
}

void state_cb (GtkWidget *widget, void *data)
{
  GnomeWinState state;
  
  state = 0;
  
  if(GTK_TOGGLE_BUTTON(minbut)->active)
    state |=  WIN_STATE_MINIMIZED;
  if(GTK_TOGGLE_BUTTON(maxvbut)->active)
    state |= WIN_STATE_MAXIMIZED_VERT;
  if(GTK_TOGGLE_BUTTON(maxhbut)->active)
    state |= WIN_STATE_MAXIMIZED_HORIZ;
  if(GTK_TOGGLE_BUTTON(hidbut)->active)
    state |= WIN_STATE_HIDDEN;
  if(GTK_TOGGLE_BUTTON(rollbut)->active)
    state |= WIN_STATE_SHADED;
  if(GTK_TOGGLE_BUTTON(fixedbut)->active)
    state |= WIN_STATE_FIXED_POSITION;
  if(GTK_TOGGLE_BUTTON(ignorebut)->active)
    state |= WIN_STATE_ARRANGE_IGNORE;
  
  gnome_win_hints_set_state(GTK_WIDGET(app), state);
  
  return;
}

void skip_cb (GtkWidget *widget, void *data)
{
  GnomeWinHints skip;

  skip = 0;
  if (GTK_TOGGLE_BUTTON(sfocbut)->active)
    skip |= WIN_HINTS_SKIP_FOCUS;
  if (GTK_TOGGLE_BUTTON(smenubut)->active)
    skip |= WIN_HINTS_SKIP_WINLIST;
  if (GTK_TOGGLE_BUTTON(sbarbut)->active)
    skip |= WIN_HINTS_SKIP_TASKBAR;
  
  gnome_win_hints_set_hints(GTK_WIDGET(app), skip);
  return;
}

void about_cb(GtkWidget *widget, gpointer data)
{
  GtkWidget *about;
  const gchar *authors[] = {"Max Watson", NULL};

  about = gnome_about_new (_("WIN_HINTS Test"), "0.1",
                           _("Copyright (C) 1998"),
                           authors,
                           _("Simple test app to check how the WIN_HINTS work.\n"
                             "And to test the gnome_win_hints_* functions for errors. :)"),
                           NULL);
  gtk_widget_show (about);

  return;
}

