#include <config.h>
#include <gtk/gtktypeutils.h>
#include "libgnomeui.h"

#include "gnometypebuiltins_vars.c"
#include "gnometypebuiltins_evals.c"
void gnome_type_init(void);

/* maybe this should be put in GTK */
static GtkType
gnome_type_register_boxed (const gchar *name) {
  GTypeInfo info = {0}; /* All zeros */

  return g_type_register_static(GTK_TYPE_BOXED, name, &info);
}

void
gnome_type_init(void) {
  int i;

  static struct {
    const gchar * type_name;
    GtkType *type_id;
    GtkType parent;
    const GtkEnumValue *values;
  } builtin_info[GNOME_TYPE_NUM_BUILTINS + 1] = {
#include "gnometypebuiltins_ids.c"
    { NULL }
  };

  for (i = 0; i < GNOME_TYPE_NUM_BUILTINS; i++)
    {
      GtkType type_id = GTK_TYPE_INVALID;
      g_assert (builtin_info[i].type_name != NULL);
      if ( builtin_info[i].parent == GTK_TYPE_ENUM )
      	type_id = g_enum_register_static (builtin_info[i].type_name, (GtkEnumValue *)builtin_info[i].values);
      else if ( builtin_info[i].parent == GTK_TYPE_FLAGS )
      	type_id = g_flags_register_static (builtin_info[i].type_name, (GtkFlagValue *)builtin_info[i].values);
      else if ( builtin_info[i].parent == GTK_TYPE_BOXED )
        type_id = gnome_type_register_boxed (builtin_info[i].type_name);

      g_assert (type_id != GTK_TYPE_INVALID);
      (*builtin_info[i].type_id) = type_id;
    }

}

