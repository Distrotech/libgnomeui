#include <libgnomeui/gnome-icon-theme.h>
#include <stdlib.h>
#include <string.h>

static void
usage (void)
{
  g_print ("usage: test lookup <theme name> <icon name> [size]]\n"
	   " or\n"
	   "usage: test list <theme name> [context]\n");
}


int
main (int argc, char *argv[])
{
  GnomeIconTheme *icon_theme;
  char *icon;
  char *context;
  char *themename;
  GList *list;
  int size = 48;
  int i;
  const GnomeIconData *icon_data;
  
  g_type_init ();

  if (argc < 3)
    {
      usage ();
      return 1;
    }

  themename = argv[2];
  
  icon_theme = gnome_icon_theme_new ();
  
  gnome_icon_theme_set_custom_theme (icon_theme, themename);

  if (strcmp (argv[1], "list") == 0)
    {
      if (argc >= 4)
	context = argv[3];
      else
	context = NULL;

      list = gnome_icon_theme_list_icons (icon_theme,
					   context);
      
      while (list)
	{
	  g_print ("%s\n", (char *)list->data);
	  list = list->next;
	}
    }
  else if (strcmp (argv[1], "lookup") == 0)
    {
      if (argc < 4)
	{
	  g_object_unref (icon_theme);
	  usage ();
	  return 1;
	}
      
      if (argc >= 5)
	size = atoi (argv[4]);
      
      icon = gnome_icon_theme_lookup_icon (icon_theme, argv[3], size, &icon_data, NULL);
      g_print ("icon for %s at %dx%d is %s\n", argv[3], size, size, icon);
      if (icon_data)
	{
	  if (icon_data->has_embedded_rect)
	    g_print ("Embedded rect: %d, %d, %d, %d\n",
		     icon_data->x0,
		     icon_data->y0,
		     icon_data->x1,
		     icon_data->y1);

	  if (icon_data->attach_points)
	    {
	      g_print ("Attach Points: ");
	      for (i = 0; i < icon_data->n_attach_points; i++)
		g_print ("%d, %d; ",
			 icon_data->attach_points[i].x,
			 icon_data->attach_points[i].y);
	      g_print ("\n");
	    }
	  if (icon_data->display_name)
	    {
	      g_print ("Display name: %s\n", icon_data->display_name);
	    }
	}
      
      g_free (icon);
    }
  else
    {
      g_object_unref (icon_theme);
      usage ();
      return 1;
    }
 

  g_object_unref (icon_theme);
  
  return 0;
}
