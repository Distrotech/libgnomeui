#include <gtk/gtktypeutils.h>
#include "libgnomeui.h"

#include "gnometypebuiltins_vars.c"
#include "gnometypebuiltins_evals.c"

void
gnome_type_init() {
  int i;

  struct {
    gchar * type_name;
    GtkType *type_id;
    GtkType parent;
    GtkEnumValue *values;
  } builtin_info[GNOME_TYPE_NUM_BUILTINS + 1] = {
#include "gnometypebuiltins_ids.c"
    { NULL }
  };

  for (i = 0; i < GNOME_TYPE_NUM_BUILTINS; i++)
    {
      GtkType type_id = GTK_TYPE_INVALID;
      g_assert (builtin_info[i].type_name != NULL);
      if ( builtin_info[i].parent == GTK_TYPE_ENUM )
      	type_id = gtk_type_register_enum (builtin_info[i].type_name, builtin_info[i].values);
      else if ( builtin_info[i].parent == GTK_TYPE_FLAGS )
      	type_id = gtk_type_register_flags (builtin_info[i].type_name, builtin_info[i].values);

      g_assert (type_id != GTK_TYPE_INVALID);
      (*builtin_info[i].type_id) = type_id;
    }

}

