#ifndef LIBGNOMEUI10COMPAT_H
#define LIBGNOMEUI10COMPAT_H

#define gnome_init(app_id, app_version, argc, argv) gnome_program_init(app_id, app_version, argc, argv, GNOMEUI_INIT, NULL)

#include <compat/1.0/libgnomeui/gnome-font-selector.h>
#include <compat/1.0/libgnomeui/gnome-guru.h>
#include <compat/1.0/libgnomeui/gnome-spell.h>
#include <compat/1.0/libgnomeui/gnome-startup.h>
#include <compat/1.0/libgnomeui/gtkcauldron.h>
#include <compat/1.0/libgnomeui/gnomeui10-compat.h>

#endif 
