#ifndef OAF_GNOME_H
#define OAF_GNOME_H 1

#include <libgnome/libgnome.h>

extern GnomeModuleInfo liboafgnome_module_info;

#define OAF_GNOME_INIT GNOME_PARAM_MODULE,&liboafgnome_module_info

#endif
