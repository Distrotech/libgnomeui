#include "oafgnome.h"
#include <libgnome/gnome-defs.h>
#include <popt.h>
#include <liboaf/liboaf.h>
#include <gdk/gdkx.h>
#include <gdk/gdk.h>
#include <libgnomeui/gnome-init.h>

static void og_pre_args_parse(GnomeProgram *app,
			      const GnomeModuleInfo *mod_info);
static void og_post_args_parse(GnomeProgram *app,
			       const GnomeModuleInfo *mod_info);
static OAFRegistrationLocation rootwin_regloc;

static GnomeModuleInfo orbit_module_info = {
  "ORBit", orbit_version, "CORBA implementation",
   NULL,
   NULL, NULL,
   NULL,
   NULL
};

static GnomeModuleRequirement liboaf_requirements[] = {
  {"0.5.1", &orbit_module_info},
  {NULL}
};

static GnomeModuleInfo liboaf_module_info = {
  "liboaf", liboaf_version, "Object Activation Framework",
  liboaf_requirements,
  (GnomeModuleHook)oaf_preinit, (GnomeModuleHook)oaf_postinit,
  oaf_popt_options,
  NULL
};

static GnomeModuleRequirement og_requirements[] = {
  {"0.0", &liboaf_module_info},
  {"1.2.0", &gtk_module_info},
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
  gdk_flush();
}

static void
rootwin_unlock(const OAFRegistrationLocation *regloc,
	       gpointer user_data)
{
  XUngrabServer(GDK_DISPLAY());
  gdk_flush();
}

static char *
rootwin_check(const OAFRegistrationLocation *regloc,
	      const OAFRegistrationCategory *regcat,
	      int *ret_distance, gpointer user_data)
{
  GdkAtom name_server_ior_atom;
  GdkAtom type, fmt;
  char *ior = NULL;
  gint actual_length;

  if(strcmp(regcat->name, "IDL:OAF/ActivationContext:1.0"))
    return NULL;

  if(!
     gdk_property_get (GDK_ROOT_PARENT(),
		       gdk_atom_intern("OAFGNOME_AC_IOR", FALSE),
		       gdk_atom_intern("STRING", FALSE),
		       0, 99999, FALSE, &type, &fmt, &actual_length,
		       (guchar *)&ior))
    return NULL;

  return ior;
}

static void
rootwin_register(const OAFRegistrationLocation *regloc,
		 const char *ior,
		 const OAFRegistrationCategory *regcat,
		 gpointer user_data)
{
  gdk_property_change(GDK_ROOT_PARENT(), gdk_atom_intern("OAFGNOME_AC_IOR", FALSE),
		      gdk_atom_intern("STRING", FALSE), 8, GDK_PROP_MODE_REPLACE, (guchar *) ior, strlen(ior));
}

static void
rootwin_unregister(const OAFRegistrationLocation *regloc,
		   const char *ior,
		   const OAFRegistrationCategory *regcat,
		   gpointer user_data)
{
  gdk_property_delete(GDK_ROOT_PARENT(), gdk_atom_intern("OAFGNOME_AC_IOR", FALSE));
}

static OAFRegistrationLocation rootwin_regloc = {
  rootwin_lock,
  rootwin_unlock,
  rootwin_check,
  rootwin_register,
  rootwin_unregister
};
