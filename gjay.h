/**
 * GJay, copyright (c) 2002 Chuck Groom
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 1, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef GJAY_H
#define GJAY_H

#include <stdio.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <assert.h>
#include <limits.h>
#include "rgbhsv.h"
#include "songs.h"
#include "prefs.h"
#include "ui.h"

typedef enum {
    UI = 0,
    DAEMON_INIT,    /* Pre-daemon mode, waiting for UI process activation */
    DAEMON,
    DAEMON_DETACHED,
    PLAYLIST        /* Generate a playlist and quit */
} gjay_mode;


/* Default directory for storing app info */
#define GJAY_VERSION     "0.2.5"
#define GJAY_DIR         ".gjay"
#define GJAY_PREFS       "prefs.xml"
#define GJAY_FILE_DATA   "data.xml"
#define GJAY_DAEMON_DATA "daemon.xml"
#define GJAY_QUEUE       "analysis_queue"
#define GJAY_TEMP        "temp_analysis_append"
#define GJAY_PID         "gjay.pid"

/* We use fixed-size buffers for labels and filenames */
#define BUFFER_SIZE      FILENAME_MAX

/* Color wheel size */
#define CATEGORIZE_DIAMETER   200   
#define SELECT_RADIUS         3


/* State */
extern gjay_mode mode;
extern gint      xmms_session;

/* User option */
extern gint      verbosity;

/* utilities */
void    read_line   ( FILE * f, char * buffer, int buffer_len);
#define strdup_to_utf8(str) (strdup_convert(str, "UTF8", "LATIN1"))
#define strdup_to_latin1(str) (strdup_convert(str, "LATIN1", "UTF8"))
gchar * strdup_convert ( const gchar * str, 
                         const gchar * enc_to, 
                         const gchar * enc_from );

/* xmms.c */
void        init_xmms   ( void );
void        play_song   ( song * s );
void        play_songs  ( GList * slist );
void        play_files  ( GList * list);


#endif /* GJAY_H */
