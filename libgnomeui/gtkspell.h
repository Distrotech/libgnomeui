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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __GTK_SPELL_H__
#define __GTK_SPELL_H__

#include <gtk/gtkvbox.h>
#include <gtk/gtktooltips.h>

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GTK_SPELL(obj)		GTK_CHECK_CAST(obj, gtk_spell_get_type(), GtkSpell)
#define GTK_SPELL_CLASS(klass)		GTK_CHECK_CLASS_CAST(klass, gtk_spell_get_type(), GtkSpelliClass)
#define GTK_IS_SPELL(obj)		GTK_CHECK_TYPE(obj, gtk_spell_get_type())

typedef struct _GtkSpell GtkSpell;
typedef struct _GtkSpellClass GtkSpellClass;
typedef struct _GtkSpellInfo GtkSpellInfo;

struct _GtkSpellInfo {
	gchar* original;
	gchar* replacement;
	gchar* word;
	guint offset;
	GSList * words;
};

struct _GtkSpell {
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

struct _GtkSpellClass {
	GtkVBoxClass parent_class;
	void (* found_word)   (GtkSpell* spell, GtkSpellInfo* sp);
	void (* handled_word) (GtkSpell* spell, GtkSpellInfo* sp);
};

guint 		gtk_spell_get_type(void);

GtkWidget* 	gtk_spell_new(void);
/* check str for mispelled words  returns 0 if words are ok */
gint 		gtk_spell_check(GtkSpell* gtkspell, gchar* str);
/* accept word for this session only */
void 		gtk_spell_accept(GtkSpell* gtkspell, gchar* word);
/* insert word in personal dictionary */
void 		gtk_spell_insert(GtkSpell* gtkspell, gchar* word, gint lowercase);
/* go for the next word */
int			gtk_spell_next(GtkSpell* gtkspell);
/* kill the ispell process */
void		gtk_spell_kill(GtkSpell* gtkspell);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_SPELL_H__ */
