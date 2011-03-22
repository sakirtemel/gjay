/*
 * Gjay - Gtk+ DJ music playlist creator
 * Copyright (C) 2002 Chuck Groom
 * Copyright (C) 2010 Craig Small 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>

#include "gjay.h"
#include "ui.h"
#include "i18n.h"

static void menuitem_currentsong (void);
static void menuitem_quit (void);

static const GtkActionEntry entries[] = {
  { "FileMenu", NULL, "_File" },
  { "EditMenu", NULL, "_Edit" },
  { "HelpMenu", NULL, "_Help" },
  { "CurrentSong", GTK_STOCK_JUMP_TO, "_Go to current song", "<control>G", "Go to current song", G_CALLBACK(menuitem_currentsong)},
  { "Quit", GTK_STOCK_QUIT, "_Quit", "<control>Q", "Quit the program", G_CALLBACK(menuitem_quit)},
  { "Preferences", GTK_STOCK_PREFERENCES, "_Preferences", NULL, "Edit Preferences", G_CALLBACK(show_prefs_window) },
  { "About", GTK_STOCK_ABOUT, "_About", NULL, "About Gjay", G_CALLBACK(show_about_window)}
};
static guint n_entries = G_N_ELEMENTS(entries);

static const char *ui_description =
"<ui>"
"  <menubar name='MainMenu'>"
"    <menu action='FileMenu'>"
"      <menuitem action='CurrentSong'/>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='EditMenu'>"
"      <menuitem action='Preferences'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"
"</ui>";

GtkWidget * make_menubar ( void ) {
  GtkWidget *menubar;
  GtkActionGroup *action_group;
  GtkUIManager *ui_manager;
  GError *error;

  action_group = gtk_action_group_new("MenuActions");
  gtk_action_group_set_translation_domain(action_group, "blah");
  gtk_action_group_add_actions(action_group, entries, n_entries, NULL);
  ui_manager = gtk_ui_manager_new();
  gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);
  gtk_window_add_accel_group(GTK_WINDOW(gjay->main_window),
      gtk_ui_manager_get_accel_group(ui_manager));
  
  error = NULL;
  if (!gtk_ui_manager_add_ui_from_string(ui_manager, ui_description, -1, &error))
  {
    g_message ("building menus failed: %s", error->message);
    g_error_free(error);
    exit(1);
  }

  menubar = gtk_ui_manager_get_widget(ui_manager, "/MainMenu");
  return menubar;
}


void menuitem_currentsong (void) {
  song * s;
  GtkWidget * dialog;

  s = gjay->player_get_current_song();
  if (s) {
    explore_select_song(s);
  } else {
    if (gjay->player_is_running()) {
      gjay_error_dialog(_("Sorry, GJay doesn't appear to know that song"));
    } else {
      gchar * msg; 
      msg = g_strdup_printf(_("Sorry, unable to connect to %s.\nIs the player running?"),
          gjay->prefs->music_player_name);
      gjay_error_dialog(msg);
      g_free(msg);
    }
  }
}

static void menuitem_quit (void) {
    quit_app(NULL, NULL, NULL);
}
