#include <config.h>
#include <gtk/gtktypeutils.h>
#include <libgnomeui.h>

#include "gnometypebuiltins_vars.c"
#include "gnometypebuiltins_evals.c"
void gnome_type_init(void);

void
gnome_type_init(void) {
  static gboolean initialized = FALSE;

  if (!initialized) {
    int i;

    static struct {
      gchar           *type_name;
      GtkType         *type_id;
      GtkType          parent;
      gconstpointer    pointer1;
      gconstpointer    pointer2;
      gconstpointer    pointer3;
      gboolean         boolean1;
    } builtin_info[GNOME_TYPE_NUM_BUILTINS + 1] = {
#include "gnometypebuiltins_ids.c"
      { NULL }
    };

    initialized = TRUE;

    for (i = 0; i < GNOME_TYPE_NUM_BUILTINS; i++)
      {
	GtkType type_id = GTK_TYPE_INVALID;
	g_assert (builtin_info[i].type_name != NULL);
	if ( builtin_info[i].parent == GTK_TYPE_ENUM )
	  type_id = g_enum_register_static (builtin_info[i].type_name, (GtkEnumValue *)builtin_info[i].pointer1);
	else if ( builtin_info[i].parent == GTK_TYPE_FLAGS )
	  type_id = g_flags_register_static (builtin_info[i].type_name, (GtkFlagValue *)builtin_info[i].pointer1);
	else if ( builtin_info[i].parent == GTK_TYPE_BOXED )
	  type_id = g_boxed_type_register_static (builtin_info[i].type_name, (GBoxedInitFunc)builtin_info[i].pointer1, (GBoxedCopyFunc)builtin_info[i].pointer2, (GBoxedFreeFunc)builtin_info[i].pointer3, builtin_info[1].boolean1);

	g_assert (type_id != GTK_TYPE_INVALID);
	(*builtin_info[i].type_id) = type_id;
      }
  }
}

