#include "oafgnome.h"
#include <popt.h>
#include <liboaf/liboaf.h>

static void og_pre_args_parse(GnomeProgram *app,
			      const GnomeModuleInfo *mod_info);
static void og_post_args_parse(GnomeProgram *app,
			       const GnomeModuleInfo *mod_info);
static OAFRegistrationLocation rootwin_regloc;

static GnomeModuleInfo liboaf_module_info = {
  "liboaf", liboaf_version, "Object Activation Framework",
  NULL,
  oaf_preinit, oaf_postinit,
  oaf_popt_options,
  NULL
};

static GnomeModuleRequirement og_requirements[] = {
  {"0.0", liboaf_module_info},
  {"1.2.0", gtk_module_info},
  {NULL}
};

GnomeModuleInfo liboafgnome_module_info = {
  "liboafgnome", VERSION, "OAF integration for GNOME programs",
  og_requirements,
  og_pre_args_parse, og_post_args_parse,
  NULL,
  NULL
};

static void
og_pre_args_parse(GnomeProgram *app, const GnomeModuleInfo *mod_info)
{
  int dumb_argc = 1;
  char *dumb_argv[] = {NULL};
  dumb_argv[0] = program_invocation_name;
  (void) oaf_orb_init(&dumb_argc, dumb_argv);
}

static void
og_post_args_parse(GnomeProgram *app, const GnomeModuleInfo *mod_info)
{
  oaf_registration_location_add(&rootwin_regloc, -100, NULL);
}

/* Registration location stuff */
#include <gdk/gdkx.h>

static void
rootwin_lock(const OAFRegistrationLocation *regloc,
	     gpointer user_data)
{
  XGrabServer(GDK_DISPLAY());
}

static void
rootwin_unlock(const OAFRegistrationLocation *regloc,
	       gpointer user_data)
{
  XUngrabServer(GDK_DISPLAY());
}

static void char *
rootwin_check(const OAFRegistrationLocation *regloc,
	      const OAFRegistrationCategory *regcat,
	      int *ret_distance, gpointer user_data)
{
  GdkAtom name_server_ior_atom;
  Atom type;
  char *ior = NULL, *result;
  int format;
  unsigned long nitems, after;
  guint32 old_warnings;

  old_warnings = gdk_error_warnings;
  gdk_error_warnings = 0;
  gdk_error_code = 0;

  if(strcmp(regcat->name, "IDL:OAF/ActivationContext:1.0"))
    return NULL;

  name_server_ior_atom = gdk_atom_intern("OAFGNOME_AC_IOR", FALSE);
  /* XXX redo with gtk+? */
  XGetWindowProperty (GDK_DISPLAY(), GDK_ROOT_WINDOW(),
		      name_server_ior_atom, 0, 99999, False, AnyPropertyType,
		      &type, &format, &nitems, &after, &ior);

  if (type == None)
    return NULL;

  result = g_strdup(ior);
  XFree (ior);

  gdk_flush ();
  gdk_error_code = 0;
  gdk_error_warnings = old_warnings;
  gdk_flush ();

  return result;
}

static void
rootwin_register(const OAFRegistrationLocation *regloc,
		 const char *ior,
		 const OAFRegistrationCategory *regcat,
		 gpointer user_data)
{
  GdkAtom name_server_ior_atom = gdk_atom_intern("OAFGNOME_AC_IOR", FALSE);
  XChangeProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(),
		  name_server_ior_atom,
		  gdk_atom_intern("STRING"), 8, PropModeReplace,
		  (guchar *)ior, strlen(ior));
}

static void
rootwin_unregister(const OAFRegistrationLocation *regloc,
		   const char *ior,
		   const OAFRegistrationCategory *regcat,
		   gpointer user_data)
{
  XDeleteProperty (GDK_DISPLAY, GDK_ROOT_WINDOW(),
		   gdk_atom_intern("OAFGNOME_AC_IOR", FALSE));
}

static OAFRegistrationLocation rootwin_regloc = {
  rootwin_lock,
  rootwin_unlock,
  rootwin_check,
  rootwin_register,
  rootwin_unregister
};
