/* GTK - Spell widget
 * Copyright (C) 1997 Paolo Molaro
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  
 * 02111-1307, USA.
 */

#ifndef __GNOME_SPELL_H__
#define __GNOME_SPELL_H__

#include <libgnome/gnome-defs.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtktooltips.h>

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

BEGIN_GNOME_DECLS

#define GNOME_TYPE_SPELL            (gnome_spell_get_type ())
#define GNOME_SPELL(obj)	    (GTK_CHECK_CAST((obj), GNOME_TYPE_SPELL, GnomeSpell))
#define GNOME_SPELL_CLASS(klass)    (GTK_CHECK_CLASS_CAST((klass), GNOME_TYPE_SPELL, GnomeSpelliClass))
#define GNOME_IS_SPELL(obj)	    (GTK_CHECK_TYPE((obj), GNOME_TYPE_SPELL))
#define GNOME_IS_SPELL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_SPELL))

typedef struct _GnomeSpell GnomeSpell;
typedef struct _GnomeSpellClass GnomeSpellClass;
typedef struct _GnomeSpellInfo GnomeSpellInfo;

struct _GnomeSpellInfo {
	const gchar* original;
	gchar* replacement;
	gchar* word;
	guint offset;
	GSList * words;
};

struct _GnomeSpell {
	GtkVBox vbox;

	GSList * spellinfo;

	GtkWidget* page_spell;
	GtkWidget* page_config;
	GtkWidget* word;
	GtkWidget* list;
	GtkWidget* b_accept;
	GtkWidget* b_skip;
	GtkWidget* b_replace;
	GtkWidget* b_insert;
	GtkWidget* b_insertl;
	GtkWidget* command;
	GtkWidget* dict;
	GtkWidget* pdict;
	GtkWidget* wchars;
	GtkWidget* wlen;
	GtkWidget* sort;
	GtkWidget* runtog;
	GtkWidget* discard;
	GtkWidget* tooltip;
	GtkWidget* killproc;
	GtkTooltips* tooltips;
	GSList* awords;

	FILE* rispell;
	FILE* wispell;
	pid_t spell_pid;
};

struct _GnomeSpellClass {
	GtkVBoxClass parent_class;
	void (* found_word)   (GnomeSpell* spell, GnomeSpellInfo* sp);
	void (* handled_word) (GnomeSpell* spell, GnomeSpellInfo* sp);
};

guint 		gnome_spell_get_type(void);

GtkWidget* 	gnome_spell_new(void);
/* check str for mispelled words  returns 0 if words are ok */
gint 		gnome_spell_check(GnomeSpell* gtkspell, const gchar* str);
/* accept word for this session only */
void 		gnome_spell_accept(GnomeSpell* gtkspell, const gchar* word);
/* insert word in personal dictionary */
void 		gnome_spell_insert(GnomeSpell* gtkspell, const gchar* word, gboolean lowercase);
/* go for the next word */
int			gnome_spell_next(GnomeSpell* gtkspell);
/* kill the ispell process */
void		gnome_spell_kill(GnomeSpell* gtkspell);

END_GNOME_DECLS

#endif /* __GNOME_SPELL_H__ */
