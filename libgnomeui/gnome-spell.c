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

#include <config.h>

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include "gnome-spell.h"

#include "libgnome/gnome-i18nP.h"
#include "libgnomeui/gnome-window-icon.h"

#ifndef HAVE_GDK_CHILD_REGISTER

/* SIGCHLD handler for gdk */

/* in gdk.h */

typedef void (*GChildFunc) (pid_t pid, gint exit_status, gpointer user_data);

void gdk_child_register (pid_t pid, 
	                     GChildFunc func, 
	                     gpointer user_data);

/* in gdk.c */

typedef struct _GdkChild GdkChild;

struct _GdkChild {
	pid_t pid;
	GChildFunc func;
	gpointer user_data;
};

static GSList* sigchld_handlers = NULL;

static void
gdk_sigchld_handler(int sig_num) {
	gint status;
	GSList* list;
	GSList* tmp;
	GdkChild *sigc=NULL;
	pid_t pid;

	pid = waitpid(0, &status, WNOHANG);

	list = sigchld_handlers;
	while ( list ) {
		sigc = (GdkChild *)list->data;
		if ( pid == sigc->pid ) {
			if ( sigc->func )
				(*sigc->func)(pid, status, sigc->user_data);
			g_free(sigc);
			tmp = list;
			list = list->next;
			tmp->next = NULL;
			g_slist_free(tmp);
		} else
			list = list->next;
	}
	signal(SIGCHLD, gdk_sigchld_handler);
	return; /* ?RETSIGTYPE? */
}

void 
gdk_child_register(pid_t pid, GChildFunc func, gpointer user_data) {
	GdkChild *sigc;

	sigc = g_new(GdkChild, 1);
	sigc->pid = pid;
	sigc->func = func;
	sigc->user_data = user_data;

	sigchld_handlers = g_slist_prepend(sigchld_handlers, sigc);
	signal(SIGCHLD, gdk_sigchld_handler);

}

#endif

enum {
	FOUND_WORD,
	HANDLED_WORD,
	LAST_SIGNAL
};

typedef void (*GnomeSpellFunction) (GtkObject* obj, gpointer arg1, gpointer data);

static void gnome_spell_class_init(GnomeSpellClass* klass);
static void gnome_spell_init(GnomeSpell* spell);

static void gnome_spell_filesel_ok(GtkButton* fsel);
static void gnome_spell_filesel_cancel(GtkButton* fsel);
static void gnome_spell_browse_handler(GtkButton * button, GtkEntry* entry);
static void gnome_spell_enable_tooltips(GtkToggleButton* b, GnomeSpell* spell);
static void gnome_spell_marshaller (GtkObject* obj, GtkSignalFunc func, gpointer func_data, GtkArg* args);
static void gnome_spell_config(GnomeSpell* spell);
static void gnome_spell_start_proc(GnomeSpell* spell);
static void gnome_spell_send_string(GnomeSpell* spell, gchar* s);
static gchar* gnome_spell_read_string(GnomeSpell* spell);
static void gnome_spell_fill_info(GnomeSpellInfo* sp, gchar* src);
static void gnome_spell_found_word(GnomeSpell* spell, GnomeSpellInfo* sp);
static void gnome_spell_button_handler(GtkButton* button, GnomeSpell* spell);
static void gnome_spell_list_handler(GtkButton* button, GtkListItem* item, GnomeSpell* spell);
static void gnome_spell_kill_process(GtkButton* button, GnomeSpell* spell);
static void gnome_spell_word_activate(GtkEntry* entry, GnomeSpell* spell);
static gint gnome_spell_check_replace(GnomeSpell* spell, gchar* word);

static GtkWidget* filesel = NULL;
static GtkWidget* ask_dialog = NULL;
static GtkWidget* ask_label = NULL;
static gint ask_ok = 0;

static gint spell_signals[LAST_SIGNAL] = { 0 };

/*static GtkVBoxClass* parent_class = 0;*/

static void
gnome_spell_child_exited(pid_t pid, gint status, GnomeSpell * spell) {

	g_return_if_fail(spell != NULL);
	g_return_if_fail(pid > 0);

	spell->spell_pid = 0;
	if ( spell->rispell )
		fclose(spell->rispell);
	if ( spell->wispell )
		fclose(spell->wispell);

}

static void
gnome_spell_marshaller (GtkObject* obj, GtkSignalFunc func, gpointer func_data, GtkArg* args) {
	GnomeSpellFunction rfunc;

	rfunc = (GnomeSpellFunction) func;
	(*rfunc) (obj, args[0].d.pointer_data, func_data);
}

static void 
gnome_spell_filesel_ok(GtkButton* fsel) {
	gchar* name;
	GtkEntry *entry = (GtkEntry*)gtk_object_get_user_data(GTK_OBJECT(filesel));
	gtk_widget_hide(GTK_WIDGET(filesel));
	gtk_grab_remove(filesel);
	name = gtk_file_selection_get_filename(GTK_FILE_SELECTION(filesel));
	if ( entry && name )
		gtk_entry_set_text(entry, name);
}

static void 
gnome_spell_filesel_cancel(GtkButton* fsel) {
	gtk_widget_hide(GTK_WIDGET(filesel));
	gtk_grab_remove(filesel);
}

static void
gnome_spell_browse_handler(GtkButton * button, GtkEntry* entry) {
	if ( !filesel ) {
		filesel = gtk_file_selection_new(_("Select dictionary"));
		gnome_window_icon_set_from_default (GTK_WINDOW (filesel));
		gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button), 
			"clicked", (GtkSignalFunc)gnome_spell_filesel_ok, NULL);
		gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->cancel_button), 
			"clicked", (GtkSignalFunc)gnome_spell_filesel_cancel, NULL);
	}
	gtk_object_set_user_data(GTK_OBJECT(filesel), (gpointer)entry);
	gtk_widget_show(filesel);
	gtk_grab_add(filesel);
}

static void 
gnome_spell_enable_tooltips(GtkToggleButton* b, GnomeSpell *spell) {

	if ( b->active )
		gtk_tooltips_enable(spell->tooltips);
	else
		gtk_tooltips_disable(spell->tooltips);
}

static void
gnome_spell_ask_cancel(GtkObject* obj) {
	ask_ok = 0;
	gtk_grab_remove(ask_dialog);
	gtk_widget_hide(ask_dialog);
}

static void
gnome_spell_ask_ok(GtkObject* obj) {
	ask_ok = 1;
	gtk_grab_remove(ask_dialog);
	gtk_widget_hide(ask_dialog);
}

static gint
gnome_spell_ask_close(GtkObject* obj) {
	ask_ok = 0;
	gtk_grab_remove(ask_dialog);
	gtk_widget_hide(ask_dialog);
	return FALSE;
}

static void
gnome_spell_ask(gchar* word) {
	GtkWidget* button;
	gchar buf[1024];

	g_snprintf(buf, sizeof(buf), _("The word `%s'\nis not in the dictionary.\nProceed anyway?"), word);

	if ( !ask_dialog ) {
		ask_dialog = gtk_dialog_new();
		gtk_window_set_title(GTK_WINDOW(ask_dialog), _("Confirm"));
		gtk_container_set_border_width(GTK_CONTAINER(ask_dialog), 5);
		gtk_signal_connect(GTK_OBJECT(ask_dialog), "delete_event",
			(GtkSignalFunc)gnome_spell_ask_close, NULL);
		gtk_signal_connect(GTK_OBJECT(ask_dialog), "destroy",
			(GtkSignalFunc)gnome_spell_ask_close, NULL);
		ask_label = gtk_label_new("");
		gtk_widget_show(ask_label);
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(ask_dialog)->vbox), ask_label);
		button = gtk_button_new_with_label(_("OK"));
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_widget_grab_default(button);
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
			(GtkSignalFunc)gnome_spell_ask_ok, NULL);
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(ask_dialog)->action_area), button);
		gtk_widget_show(button);
		button = gtk_button_new_with_label(_("Cancel"));
		gtk_signal_connect(GTK_OBJECT(button), "clicked",
			(GtkSignalFunc)gnome_spell_ask_cancel, NULL);
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(ask_dialog)->action_area), button);
		gtk_widget_show(button);
	}
	gtk_label_set_text(GTK_LABEL(ask_label), buf);
	gtk_widget_show(ask_dialog);
	gtk_grab_add(ask_dialog);
}

static gint 
gnome_spell_check_replace(GnomeSpell* spell, gchar* word) {
	gchar* result;
	gchar* buf;
	gint found = 1;

	buf = alloca(strlen(word)+3);
	g_snprintf(buf, strlen(word) + 3, "^%s\n", word);
	gnome_spell_send_string(spell, buf);
	while ( (result=gnome_spell_read_string(spell)) && strcmp(result, "\n") ) {
		found = 0;
		g_free(result);
	}
	g_free(result);
	if ( !found ) {
		/* FIXME: hack: ask for confirmation */
		ask_ok = -1;
		gnome_spell_ask(word);
		while ( ask_ok < 0 )
			gtk_main_iteration();
	} else
		ask_ok =1;

	return ask_ok;
}

static void
gnome_spell_word_activate(GtkEntry* entry, GnomeSpell* spell) {
	if ( !gnome_spell_check_replace(spell, gtk_entry_get_text(entry)) )
		return;
	if ( spell->spellinfo ) {
		GnomeSpellInfo * sp = (GnomeSpellInfo*)spell->spellinfo->data;
		GSList* tmp;
		sp->replacement = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
		gtk_signal_emit(GTK_OBJECT(spell), spell_signals[HANDLED_WORD], sp);
		gnome_spell_fill_info(sp, NULL);
		tmp = spell->spellinfo;
		spell->spellinfo = g_slist_remove_link(spell->spellinfo, tmp);
		g_free(sp);
		g_slist_free(tmp);
		gtk_entry_set_text(GTK_ENTRY(spell->word), "");
		gtk_list_remove_items(GTK_LIST(spell->list), (GTK_LIST(spell->list))->children);
	}
	if ( spell->spellinfo ) {
		gtk_signal_emit(GTK_OBJECT(spell), spell_signals[FOUND_WORD], spell->spellinfo->data);
	}
}

static void 
gnome_spell_button_handler(GtkButton* button, GnomeSpell* spell) {
	gchar* word = gtk_entry_get_text(GTK_ENTRY(spell->word));
	g_return_if_fail ( word != NULL );

	if ( button == GTK_BUTTON(spell->b_accept) )
		gnome_spell_accept(spell, word);
	else if ( button == GTK_BUTTON(spell->b_replace) ) {
		if ( !gnome_spell_check_replace(spell, word) )
			return;
		if ( spell->spellinfo ) {
			GnomeSpellInfo * sp = (GnomeSpellInfo*)spell->spellinfo->data;
			sp->replacement = g_strdup(word);
		}
	} else if ( button != GTK_BUTTON(spell->b_skip) )
		gnome_spell_insert(spell, word, button == GTK_BUTTON(spell->b_insertl));
	/* button "skip" jumps here */
	gtk_entry_set_text(GTK_ENTRY(spell->word), "");
	gtk_list_remove_items(GTK_LIST(spell->list), (GTK_LIST(spell->list))->children);
	/* We handled this damn word, so we can delete it from the list */
	if ( spell->spellinfo ) {
		GnomeSpellInfo * sp = (GnomeSpellInfo*)spell->spellinfo->data;
		gtk_signal_emit(GTK_OBJECT(spell), spell_signals[HANDLED_WORD], sp);
		gnome_spell_fill_info(sp, NULL);
		spell->spellinfo = g_slist_remove(spell->spellinfo, sp);
		g_free(sp);
	}
	/* If there are other words in the list emit the foud_word signal */
	if ( spell->spellinfo ) {
		gtk_signal_emit(GTK_OBJECT(spell),
			spell_signals[FOUND_WORD], spell->spellinfo->data);
	}
}

static void 
gnome_spell_list_handler(GtkButton* button, GtkListItem* item, GnomeSpell* spell) {
	gchar* word;
	gtk_label_get( GTK_LABEL( GTK_BIN(item)->child ), &word);
	gtk_entry_set_text(GTK_ENTRY(spell->word), word);
}

static void 
gnome_spell_kill_process(GtkButton* button, GnomeSpell* spell) {
	GSList* awords;
	gnome_spell_kill(spell);
	if ( GTK_TOGGLE_BUTTON(spell->discard)->active && spell->awords ) {
		awords = spell->awords;
		while ( awords ) {
			g_free(awords->data);
			awords = awords->next;
		}
		g_slist_free(spell->awords);
		spell->awords = 0;
	}
}


static void
gnome_spell_config(GnomeSpell* spell) {
	char* spellid;
	GSList* awords;
	spellid = gnome_spell_read_string(spell);
	if ( !spellid || !strstr(spellid, "spell") ) {
		g_warning("What spell is this (%s)?\n", spellid); /* cut the check for "spell"? */
		gnome_spell_kill(spell);
	} else {
		gnome_spell_send_string(spell, "!\n");
		awords = spell->awords;
		while ( awords ) {
			gnome_spell_send_string(spell, "@");
			gnome_spell_send_string(spell, awords->data);
			gnome_spell_send_string(spell, "\n");
			awords = awords->next;
		}
	}
	g_free(spellid);
}

static void 
gnome_spell_start_proc(GnomeSpell* spell) {
	gint fd1[2];
	gint fd2[2];
	gchar* cmd = "ispell";
	gchar* mcmd;
	gchar* args[16] = { 
		"gnome-spellchecker",
		"-a",
		0
	};
	gchar* arg;
	gint argpos = 2;
	mcmd = gtk_entry_get_text(GTK_ENTRY(spell->command));
	if ( mcmd && *mcmd )
		cmd = mcmd;
	if ( (arg=gtk_entry_get_text(GTK_ENTRY(spell->dict))) && *arg ) {
		args[argpos++] = "-d";
		args[argpos++] = arg;
	}
	if ( (arg=gtk_entry_get_text(GTK_ENTRY(spell->pdict))) && *arg ) {
		args[argpos++] = "-p";
		args[argpos++] = arg;
	}
	if ( (arg=gtk_entry_get_text(GTK_ENTRY(spell->wchars))) && *arg ) {
		args[argpos++] = "-w";
		args[argpos++] = arg;
	}
	if ( (arg=gtk_entry_get_text(GTK_ENTRY(spell->wlen))) && *arg ) {
		args[argpos++] = "-W";
		args[argpos++] = arg;
	}
	if ( (GTK_TOGGLE_BUTTON(spell->sort))->active )
		args[argpos++] = "-S";
	if ( (GTK_TOGGLE_BUTTON(spell->runtog))->active )
		args[argpos++] = "-C";

	if ( pipe(fd1) || pipe(fd2) ) {
		g_warning("gnome-spell: pipe() failed: %s", g_strerror(errno));
		return;
	}

	spell->spell_pid = fork();
	if ( spell->spell_pid < 0 ) {
		g_warning("gnome-spell: fork() failed: %s", g_strerror(errno));
		return;
	} else if ( spell->spell_pid == 0 ) {
		close(fd1[0]);
		close(fd2[1]);
		if ( dup2(fd2[0], 0) < 0 ) {
			g_warning("gnome-spell: dup2() failed: %s", g_strerror(errno));
			_exit(1);
		}
		if ( dup2(fd1[1], 1) < 0 ) {
			g_warning("gnome-spell: dup2() failed: %s", g_strerror(errno));
			_exit(1);
		}
		execvp(cmd, args);
		g_warning("gnome-spell: exec() failed: %s", g_strerror(errno));
		_exit(1);
	} else if ( spell->spell_pid > 0 ) {
		gdk_child_register(spell->spell_pid, (GChildFunc)gnome_spell_child_exited, spell);
		close(fd2[0]);
		close(fd1[1]);
		spell->rispell = fdopen(fd1[0], "r");
		if ( !spell->rispell ) {
			g_warning("gnome-spell: r-fdopen() failed: %s", g_strerror(errno));
			gnome_spell_kill(spell);
		}
		spell->wispell = fdopen(fd2[1], "w");
		if ( !spell->wispell ) {
			g_warning("gnome-spell: w-fdopen() failed: %s", g_strerror(errno));
			gnome_spell_kill(spell);
		}
	}
}

static void 
gnome_spell_send_string(GnomeSpell* spell, gchar* s) {
	if ( spell->spell_pid <= 0 ) {
		gnome_spell_start_proc(spell);
		if ( spell->spell_pid <= 0 ) { /* No luck */
			g_warning("Cannot start ispell process");
			return;
		}
		gnome_spell_config(spell);
	}
	if ( spell->spell_pid <= 0 )
		return;
	fprintf(spell->wispell, "%s", s);
	fflush(spell->wispell);
#ifdef TEST
	fprintf(stderr, "WRITE: %s", s);
#endif
}

static gchar* 
gnome_spell_read_string(GnomeSpell* spell) {
	gchar* buf;
	guint buflen=1024;
	if ( spell->spell_pid <= 0 || ! spell->rispell )
		return NULL;
	buf = alloca(buflen);
	if ( !fgets(buf, buflen, spell->rispell) )
		return NULL;
	/* discard longer lines? */
#ifdef TEST
	fprintf(stderr, "READ: %s", buf);
#endif
	return g_strdup(buf);
}

static void
build_page_spell(GnomeSpell* spell) {
	GtkWidget* hbox;
	GtkWidget* button;
	GtkWidget* label;
	GtkWidget* list;
	GtkWidget* scwindow;
	GtkWidget* entry;
	GtkWidget* vbox;
	GtkWidget* page_spell=spell->page_spell;

	hbox = gtk_hbox_new(0, 5);
	gtk_widget_show(hbox);
	label = gtk_label_new(_("Word:"));
	gtk_widget_show(label);
	entry = gtk_entry_new();
	gtk_widget_show(entry);
	gtk_signal_connect(GTK_OBJECT(entry), "activate",
		(GtkSignalFunc)gnome_spell_word_activate, spell);
	/* FIXME: make the activate signal replace */
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	gtk_box_pack_start_defaults(GTK_BOX(page_spell), hbox);
	spell->word = entry;

	hbox = gtk_hbox_new(0, 5);
	gtk_widget_show(hbox);
	
	vbox = gtk_vbox_new(0, 5);
	gtk_widget_show(vbox);
	label = gtk_label_new(_("Alternatives:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	scwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show(scwindow);
	list = gtk_list_new();
	gtk_widget_show(list);
	/* FIXME: make a double click replace */
	gtk_signal_connect(GTK_OBJECT(list), "select_child",
		(GtkSignalFunc)gnome_spell_list_handler, spell);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scwindow), list);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), scwindow, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
	spell->list = list;

	vbox = gtk_vbox_new(0, 5);
	gtk_widget_show(vbox);
	button = gtk_button_new_with_label(_("Accept"));
	gtk_widget_show(button);
	spell->b_accept = button;
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
		(GtkSignalFunc)gnome_spell_button_handler, spell);
	gtk_tooltips_set_tip(spell->tooltips, button, _("Accept the word for\nthis session only"), "");
	gtk_container_add(GTK_CONTAINER(vbox), button);
	button = gtk_button_new_with_label(_("Skip"));
	gtk_widget_show(button);
	spell->b_skip = button;
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
		(GtkSignalFunc)gnome_spell_button_handler, spell);
	gtk_tooltips_set_tip(spell->tooltips, button, _("Ignore this word\nthis time only"), "");
	gtk_container_add(GTK_CONTAINER(vbox), button);
	button = gtk_button_new_with_label(_("Replace"));
	gtk_widget_show(button);
	spell->b_replace = button;
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
		(GtkSignalFunc)gnome_spell_button_handler, spell);
	gtk_tooltips_set_tip(spell->tooltips, button, _("Replace the word"), "");
	gtk_container_add(GTK_CONTAINER(vbox), button);
	button = gtk_button_new_with_label(_("Insert"));
	gtk_widget_show(button);
	spell->b_insertl = button;
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
		(GtkSignalFunc)gnome_spell_button_handler, spell);
	gtk_tooltips_set_tip(spell->tooltips, button, _("Insert the word\nin your personal dictionary"), "");
	gtk_container_add(GTK_CONTAINER(vbox), button);
	button = gtk_button_new_with_label(_("Insert with case"));
	gtk_widget_show(button);
	gtk_container_add(GTK_CONTAINER(vbox), button);
	spell->b_insert = button;
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
		(GtkSignalFunc)gnome_spell_button_handler, spell);
	gtk_tooltips_set_tip(spell->tooltips, button, _("Insert the word\nin your personal dictionary\n(if mixed-case preserve\ncase-sensitivity)"), "");
	gtk_box_pack_end(GTK_BOX(hbox), vbox, TRUE, FALSE, 0);
	gtk_box_pack_start_defaults(GTK_BOX(page_spell), hbox);
}

static void
build_page_config(GnomeSpell* spell) {
	GtkWidget* label;
	GtkWidget* hbox;
	GtkWidget* entry;
	GtkWidget* button;
	GtkWidget* page_config=spell->page_config;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	label = gtk_label_new(_("Ispell command:"));
	gtk_widget_show(label);
	entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), "ispell");
	gtk_widget_show(entry);
	spell->command = entry;
	/* browse button ... */
	button = gtk_button_new_with_label(_("Browse..."));
	gtk_widget_show(button);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		(GtkSignalFunc) gnome_spell_browse_handler, entry);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(page_config), hbox);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	label = gtk_label_new(_("Main dictionary:"));
	gtk_widget_show(label);
	entry = gtk_entry_new();
	gtk_widget_show(entry);
	spell->dict = entry;
	/* browse button ... */
	button = gtk_button_new_with_label(_("Browse..."));
	gtk_widget_show(button);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		(GtkSignalFunc) gnome_spell_browse_handler, entry);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(page_config), hbox);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	label = gtk_label_new(_("Personal dictionary:"));
	gtk_widget_show(label);
	entry = gtk_entry_new();
	gtk_widget_show(entry);
	spell->pdict = entry;
	/* browse button ... */
	button = gtk_button_new_with_label(_("Browse..."));
	gtk_widget_show(button);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		(GtkSignalFunc) gnome_spell_browse_handler, entry);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(page_config), hbox);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	label = gtk_label_new(_("Valid chars in words:"));
	gtk_widget_show(label);
	entry = gtk_entry_new();
	gtk_widget_show(entry);
	spell->wchars = entry;
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(page_config), hbox);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	label = gtk_label_new(_("Check words longer than:"));
	gtk_widget_show(label);
	entry = gtk_entry_new();
	gtk_widget_show(entry);
	spell->wlen = entry;
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(page_config), hbox);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	button = gtk_check_button_new_with_label(_("Sort by correctness"));
	gtk_widget_show(button);
	spell->sort = button;
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(page_config), hbox);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	button = gtk_check_button_new_with_label(_("Accept run-together words"));
	gtk_widget_show(button);
	spell->runtog = button;
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(page_config), hbox);

	/* include guesses? */

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	button = gtk_check_button_new_with_label(_("Discard accepted words on kill"));
	gtk_widget_show(button);
	spell->discard = button;
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(page_config), hbox);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	button = gtk_check_button_new_with_label(_("Enable tooltips"));
	gtk_widget_show(button);
	spell->tooltip = button;
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(page_config), hbox);
	gtk_signal_connect(GTK_OBJECT(button), "toggled",
		(GtkSignalFunc)gnome_spell_enable_tooltips, spell);

	hbox = gtk_hseparator_new();
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(page_config), hbox);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	button = gtk_button_new_with_label(_("Kill ispell process"));
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
		(GtkSignalFunc)gnome_spell_kill_process, spell);
	button = gtk_button_new_with_label("Apply");
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
		(GtkSignalFunc)gnome_spell_kill_process, spell);
	gtk_container_add(GTK_CONTAINER(page_config), hbox);

}

static GtkWidget*
build_widget(GnomeSpell* spell) {
	GtkWidget* notebook;
	GtkWidget* label;

	notebook = gtk_notebook_new();
	/*gtk_container_set_border_width(GTK_CONTAINER(notebook), 10);*/

	spell->page_spell = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(spell->page_spell), 10);
	gtk_widget_show(spell->page_spell);
	build_page_spell(spell);
	label = gtk_label_new(_("Spell"));
	gtk_widget_show(label);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), spell->page_spell, label);
	
	spell->page_config = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(spell->page_config);
	build_page_config(spell);
	label = gtk_label_new(_("Configure"));
	gtk_widget_show(label);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), spell->page_config, label);
	
	gtk_widget_show(notebook);
	
	return notebook;
}

static void
gnome_spell_destroy (GtkObject* s) {
	GnomeSpell* spell = (GnomeSpell*)s;
	if ( spell->tooltips )
		gtk_object_unref(GTK_OBJECT(spell->tooltips));
	/* FIXME: sigchld handler */
	if ( spell->rispell )
		fclose(spell->rispell);
	if ( spell->wispell )
		fclose(spell->wispell);
	if ( spell->spell_pid > 0 )
		waitpid(spell->spell_pid, NULL, WNOHANG | WUNTRACED);
	if ( spell->spellinfo ) {
		g_slist_foreach(spell->spellinfo, (GFunc)gnome_spell_fill_info, NULL);
		g_slist_foreach(spell->spellinfo, (GFunc)g_free, NULL);
		g_slist_free(spell->spellinfo);
	}
	if (filesel ) {
		gtk_widget_destroy(filesel);
		filesel = NULL;
	}
	if (spell->awords) {
		g_slist_foreach(spell->awords, (GFunc)g_free, NULL);
		g_slist_free(spell->awords);
	}
}

/**
 * gnome_spell_get_type
 *
 * Description:
 * Returns unique Gtk+ type identifier for the GNOME spell checker
 * widget
 *
 * Returns: Gtk+ type identifier
 **/

guint
gnome_spell_get_type () {
	static guint gnome_spell_type = 0;

	if ( !gnome_spell_type ) {
		GtkTypeInfo spell_info = {
			"GnomeSpell",
			sizeof(GnomeSpell),
			sizeof(GnomeSpellClass),
			(GtkClassInitFunc) gnome_spell_class_init,
			(GtkObjectInitFunc) gnome_spell_init,
		};
		gnome_spell_type = gtk_type_unique(gtk_vbox_get_type(), &spell_info);
	}
	return gnome_spell_type;
}

static void
gnome_spell_class_init(GnomeSpellClass* klass) {
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass*) klass;
	widget_class = (GtkWidgetClass*) klass;

	spell_signals[FOUND_WORD] = gtk_signal_new ("found_word",
		GTK_RUN_FIRST|GTK_RUN_NO_RECURSE,
		object_class->type,
		GTK_SIGNAL_OFFSET(GnomeSpellClass, found_word),
		gnome_spell_marshaller, GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

	spell_signals[HANDLED_WORD] = gtk_signal_new ("handled_word",
		GTK_RUN_FIRST|GTK_RUN_NO_RECURSE,
		object_class->type,
		GTK_SIGNAL_OFFSET(GnomeSpellClass, handled_word),
		gnome_spell_marshaller, GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

	gtk_object_class_add_signals(object_class, spell_signals, LAST_SIGNAL);

	object_class->destroy = gnome_spell_destroy;

	klass->found_word = gnome_spell_found_word;
	klass->handled_word = NULL;

}

static void
gnome_spell_init(GnomeSpell* spell) {
	GtkWidget * noteb;

	spell->spell_pid = -1;
	spell->rispell = spell->wispell = NULL;
	spell->tooltips = NULL;
	spell->spellinfo = NULL;
	spell->awords = NULL;

	spell->tooltips = gtk_tooltips_new();
	noteb = build_widget(spell);

	if ( !GTK_TOGGLE_BUTTON(spell->tooltip)->active )
		gtk_tooltips_disable(spell->tooltips);
	gtk_container_add(GTK_CONTAINER(spell), noteb);

}

/**
 * gnome_spell_new
 *
 * Description:
 * Create a new GNOME spell checker object.
 *
 * Returns:  Pointer to new GNOME spell checker object, or %NULL on
 * failure.
 **/

GtkWidget*
gnome_spell_new() {
	return GTK_WIDGET(gtk_type_new(gnome_spell_get_type()));
}

/* used also for freeing sp->* stuff with src == NULL */
static void
gnome_spell_fill_info(GnomeSpellInfo* sp, gchar* src) {
	gchar* p;
	gchar * reject = " ,\t\n\r\v:";
	gint count;
	char *tokp;

	sp->offset = 0;
	sp->original = NULL;
	if ( sp->word ) {
		g_free(sp->word);
		sp->word = NULL;
	}
	if ( sp->replacement ) {
		g_free(sp->replacement);
		sp->replacement = NULL;
	}
	if ( sp->words ) {
		g_slist_foreach(sp->words, (GFunc)g_free, NULL);
		g_slist_free(sp->words);
		sp->words = NULL;
	}

	if (!src ) return;
	while( *src && isspace(*src) ) ++src;
	p = src;
	if ( *p != '&' && *p != '?' && *p != '#' ) {
		g_warning("Ispell output corrupt!\n");
		/* FIXME: se arriva qui SIGSEV */
		return;
	}
	++p;
	p = strtok_r(p, reject, &tokp);
	if ( !p ) return;
	sp->word = g_strdup(p);
	if ( *src == '#' ) {
		sp->offset = atoi(p);
		if ( sp->offset > 0 ) --sp->offset;
	} else {
		p = strtok_r(NULL, reject, &tokp);
		if ( p ) {
			count = atoi(p);
		} else return;
		p = strtok_r(NULL, reject, &tokp);
		if ( p ) {
			sp->offset = atoi(p);
			if ( sp->offset > 0 ) --sp->offset;
		} else return;
		while ( (p=strtok_r(NULL, ",\n\t", &tokp)) ) {
			while (p && isspace(*p)) ++p;
			sp->words = g_slist_append(sp->words, g_strdup(p));
		}
	}
}

/**
 * gnome_spell_check
 * @spell: Pointer to GNOME spell checker object.
 * @str: String to be spell-checked.
 *
 * Description:
 * Perform spell-checking on one or more words.
 *
 * FIXME: there is a problem when you call gnome_spell_check(spell, "bogus bogus"):
 * if you accept bogus the first time, it will be reported again
 * because ispell checked it before...
 * The easiest solution is to spell-check a word at a time (but it's slow).
 *
 * Returns: 1 if spelling is ok, 0 if not, -1 on system error.
 **/

gint
gnome_spell_check(GnomeSpell* spell, const gchar* str) {
	GnomeSpellInfo *sp;
	gchar* buf;
	gchar* result;

	g_return_val_if_fail(spell != NULL, 0);
	g_return_val_if_fail(str != NULL, 0);
	g_return_val_if_fail(*str, 0);
	g_return_val_if_fail(GNOME_IS_SPELL(spell), 0);

	buf = alloca(strlen(str)+3);
	/* check "\n" */
	g_snprintf(buf, strlen(str) + 3, "^%s\n", str);
	result = strchr(buf, '\n');
	++result;
	*result = 0;
	result = NULL;
	gnome_spell_send_string(spell, buf);
	if ( spell->spell_pid <= 0 )
		return -1;
	while ( (result=gnome_spell_read_string(spell)) && strcmp(result, "\n") ) {
		sp = g_new(GnomeSpellInfo, 1);
		sp->word = sp->replacement = NULL;
		sp->words = NULL;
		gnome_spell_fill_info(sp, result);
		sp->original = str;
		spell->spellinfo = g_slist_append(spell->spellinfo, sp);
		g_free(result);
	}
	g_free(result);
	if ( spell->spellinfo ) {
		gtk_signal_emit(GTK_OBJECT(spell),
			spell_signals[FOUND_WORD], spell->spellinfo->data);
		return 1;
	}
	return 0;
}

static void 
gnome_spell_found_word(GnomeSpell* spell, GnomeSpellInfo* sp) {
	GtkWidget* item;
	GSList* list;
	GList * ilist = NULL;

	g_return_if_fail(spell != NULL);
	g_return_if_fail(sp != NULL);

	gtk_list_remove_items(GTK_LIST(spell->list), (GTK_LIST(spell->list))->children);
	gtk_entry_set_text(GTK_ENTRY(spell->word), sp->word);
	list = sp->words;
	while (list) {
		item = gtk_list_item_new_with_label(list->data);
		gtk_widget_show(item);
		ilist = g_list_prepend(ilist, item);
		list = list->next;
	}
	if ( ilist )
		gtk_list_append_items(GTK_LIST(spell->list), g_list_reverse(ilist));
}

static int 
check_word(const gchar* s) {
	if ( !s || !*s ) return 1;
	while ( *s && (*s == '-' || isalnum(*s)) ) ++s;
	return *s;
}

/**
 * gnome_spell_accept
 * @spell: Pointer to GNOME spell checker object.
 * @word: Word to be ignored.
 *
 * Description:
 * Adds a single word to the runtime list of words that the spelling
 * checker should ignore.
 **/

void
gnome_spell_accept(GnomeSpell *spell, const gchar* word) {
	gchar* buf;
	
	g_return_if_fail(spell != NULL);
	g_return_if_fail(word != NULL);
	g_return_if_fail(GNOME_IS_SPELL(spell));

	buf = alloca(strlen(word)+3);

	if ( check_word(word) ) {
		g_warning("`%s' not a valid word for ispell", word);
		return;
	}
	g_snprintf(buf, strlen(word) + 3, "@%s\n", word);
	/* mettere in awords */
	spell->awords = g_slist_prepend(spell->awords, g_strdup(word));
	gnome_spell_send_string(spell, buf);
}

/**
 * gnome_spell_insert
 * @spell: Pointer to GNOME spell checker object.
 * @word: Word to be added to private dictionary.
 * @lowercase: %TRUE if the added word should be changed to lowercase, %FALSE if not.
 *
 * Description:
 * Adds a single word to the spelling checker's private dictionary.
 **/

void
gnome_spell_insert(GnomeSpell* spell, const gchar* word, gboolean lowercase) {
	gchar* w;
	gchar* buf;

	g_return_if_fail(spell != NULL);
	g_return_if_fail(word != NULL);
	g_return_if_fail(GNOME_IS_SPELL(spell));

	w = alloca(strlen(word)+1);
	buf = alloca(strlen(word)+3);

	if ( check_word(word) ) {
		g_warning("`%s' not a valid word for ispell", word);
		return;
	}
	strcpy(w, word);
	if ( lowercase ) {
		char *p = w;
		for (p = w; *p; p++){
			*p = tolower(*p);
		}
	}
	/*  if ispell dies in a bad way it dosen't save the dictionary.
		save it at every update? (append "#\n" to the string below) */
	g_snprintf(buf, strlen(word) + 3, "*%s\n", w);
	gnome_spell_send_string(spell, buf);
}

/**
 * gnome_spell_next
 * @spell: Pointer to GNOME spell checker object.
 *
 * Description:
 * Adds a single word to the runtime list of words that the spelling
 * checker should ignore.
 * 
 * Returns: %TRUE on success, %FALSE on failure.
 **/

int 
gnome_spell_next(GnomeSpell* spell) {
	GnomeSpellInfo * sp;

	g_return_val_if_fail(spell != NULL, 0);
	g_return_val_if_fail(GNOME_IS_SPELL(spell), 0);
	
	if ( !spell->spellinfo )
		return 0;
	sp = (GnomeSpellInfo*)spell->spellinfo->data;
	gtk_signal_emit(GTK_OBJECT(spell), spell_signals[HANDLED_WORD], sp);
	gnome_spell_fill_info(sp, NULL);
	spell->spellinfo = g_slist_remove(spell->spellinfo, sp);
	g_free(sp);
	if ( spell->spellinfo ) {
		gtk_signal_emit(GTK_OBJECT(spell),
			spell_signals[FOUND_WORD], spell->spellinfo->data);
		return 1;
	}
	return 0;
}


/**
 * gnome_spell_kill
 * @spell: Pointer to GNOME spell checker object.
 *
 * Description:
 * Terminates the external spelling checker process, if present.
 **/

void
gnome_spell_kill(GnomeSpell* spell) {
	g_return_if_fail(spell != NULL);
	g_return_if_fail(GNOME_IS_SPELL(spell));

	if ( spell->spell_pid <= 0 )
		return;
	kill(spell->spell_pid, SIGTERM);

	spell->spell_pid = 0;
}
