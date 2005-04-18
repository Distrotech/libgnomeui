/* -*- Mode: C; tab-width: 8; c-basic-offset: 8; indent-tabs-mode: nil -*- */
/* gnome-ui-win32.c: Win32-specific code in libgnomeui
 * Copyright (C) 2005 Novell, Inc
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <windows.h>
#include <string.h>
#include <glib.h>

/* localedir uses system codepage as it is passed to the non-UTF8ified
 * gettext library
 */
static char *localedir = NULL;

/* The others are in UTF-8 */
static char *datadir;

static HMODULE hmodule;
G_LOCK_DEFINE_STATIC (mutex);

/* Silence gcc with prototypes. Yes, this is silly. */
BOOL WINAPI DllMain (HINSTANCE hinstDLL,
		     DWORD     fdwReason,
		     LPVOID    lpvReserved);

const char *_gnome_ui_get_localedir (void);
const char *_gnome_ui_get_datadir (void);

/* DllMain used to tuck away the DLL's HMODULE */
BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
	 DWORD     fdwReason,
	 LPVOID    lpvReserved)
{
        switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
                hmodule = hinstDLL;
                break;
        }
        return TRUE;
}

static char *
replace_prefix (const char *runtime_prefix,
                const char *configure_time_path)
{
        if (strncmp (configure_time_path, LIBGNOMEUI_PREFIX "/", strlen (LIBGNOMEUI_PREFIX) + 1) == 0) {
                return g_strconcat (runtime_prefix,
                                    configure_time_path + strlen (LIBGNOMEUI_PREFIX),
                                    NULL);
        } else
                return g_strdup (configure_time_path);
}

static void
setup (void)
{
	char *full_prefix = NULL;  
	char *short_prefix = NULL; 
	char *p;

        G_LOCK (mutex);
        if (localedir != NULL) {
                G_UNLOCK (mutex);
                return;
        }

        if (GetVersion () < 0x80000000) {
                /* NT-based Windows has wide char API */
                wchar_t wcbfr[1000];
                if (GetModuleFileNameW (hmodule, wcbfr,
                                        G_N_ELEMENTS (wcbfr))) {
                        full_prefix = g_utf16_to_utf8 (wcbfr, -1,
                                                       NULL, NULL, NULL);
                        if (GetShortPathNameW (wcbfr, wcbfr,
                                               G_N_ELEMENTS (wcbfr)))
                                short_prefix = g_utf16_to_utf8 (wcbfr, -1,
                                                                NULL, NULL, NULL);
                }
        } else {
                /* Win9x, yecch */
                char cpbfr[1000];
                if (GetModuleFileNameA (hmodule, cpbfr, G_N_ELEMENTS (cpbfr)))
                        full_prefix = short_prefix =
                                g_locale_to_utf8 (cpbfr, -1,
                                                  NULL, NULL, NULL);
        }

        if (full_prefix != NULL) {
                p = strrchr (full_prefix, '\\');
                if (p != NULL)
                        *p = '\0';
      
                p = strrchr (full_prefix, '\\');
                if (p && (g_ascii_strcasecmp (p + 1, "bin") == 0))
                        *p = '\0';
        } else {
                full_prefix = "";
        }

        if (short_prefix != NULL) {
                p = strrchr (short_prefix, '\\');
                if (p != NULL)
                        *p = '\0';
      
                p = strrchr (short_prefix, '\\');
                if (p && (g_ascii_strcasecmp (p + 1, "bin") == 0))
                        *p = '\0';
        } else {
                short_prefix = "";
        }

        localedir = replace_prefix (short_prefix, GNOMEUILOCALEDIR);
        p = localedir;
        localedir = g_locale_from_utf8 (localedir, -1, NULL, NULL, NULL);
        g_free (p);

        datadir = replace_prefix (full_prefix, LIBGNOMEUI_DATADIR);

	G_UNLOCK (mutex);
}

const char *
_gnome_ui_get_localedir (void)
{
        setup ();
        return localedir;
}

const char *
_gnome_ui_get_datadir (void)
{
        setup ();
        return datadir;
}
