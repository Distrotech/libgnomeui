#include <config.h>
#include <gtk/gtktypeutils.h>
#include <libgnomeui.h>

void gnome_type_init (void);

void gnome_type_init (void) {
	static gboolean initialized = FALSE;

	if (initialized)
		return;

#include "gnometype_inits.c"

	initialized = TRUE;
}
