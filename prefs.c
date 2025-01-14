/*
 * Gjay - Gtk+ DJ music playlist creator
 * Copyright (C) 2002 Chuck Groom
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
 *
 */

/*
 * The preference XML format is:
 * 
 * <rootdir [extension_filer="t"]>Path</rootdir>
 * <flags [hide_tip=...] [wander=...]>
 * <daemon_action>enum</daemon_action>
 * <start>random|selected|color</start>
 * <selection_limit>songs|dir<selection_limit>
 * <rating [cutoff=...]>float</rating>
 * <hue>...
 * <brightness>...
 * <saturation>...
 * <bpm>...
 * <freq>...
 * <variance>...
 * <path_weight>...
 * <color type="hsv">float float float</color>
 * <time>int</time>
 * <max_working_set>int</max_working_set>
 * <player>int</player>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "gjay.h"


typedef enum {
    PE_GJAY_PREFS,
    PE_ROOTDIR,
    PE_FLAGS,
    PE_DAEMON_ACTION,
    PE_START,
    PE_SELECTION_LIMIT,
    PE_RATING,
    PE_HUE,
    PE_BRIGHTNESS,
    PE_SATURATION,
    PE_BPM,
    PE_FREQ,
    PE_VARIANCE,
    PE_PATH_WEIGHT,
    PE_COLOR,
    PE_TIME,
    PE_MAX_WORKING_SET,
    /* attributes */
    PE_EXTENSION_FILTER,
    PE_HIDE_TIP,
    PE_WANDER,
    PE_CUTOFF,
    PE_VERSION,
    PE_USE_RATINGS,
    PE_TYPE,
    /* values */
    PE_RANDOM,
    PE_SELECTED,
    PE_SONGS,
    PE_DIR,
    PE_MUSIC_PLAYER,
    PE_LAST
} pref_element_type;


char * pref_element_strs[PE_LAST] = {
    "gjay_prefs",
    "rootdir",
    "flags",
    "daemon_action",
    "start",
    "selection_limit",
    "rating",
    "hue",
    "brightness",
    "saturation",
    "bpm",
    "freq",
    "variance",
    "path_weight",
    "color",
    "time",
    "max_working_set",
    "extension_filter",
    "hide_tip",
    "wander",
    "cutoff",
    "version",
    "use_ratings",
    "type",
    "random",
    "selected",
    "songs",
    "dir",
    "player"
};

struct parser_data {
  GjayPrefs *prefs;
  pref_element_type element;
};


const char *music_player_names[] =
{
  "None",
#ifdef WITH_AUDCLIENT
  "Audacious",
#endif /* WITH_AUDCLIENT */
#ifdef WITH_MPDCLIENT
  "MPD",
#endif
#ifdef WITH_EXAILE
  "Exaile",
#endif
  NULL
};

#define print_pref_float(f,name,value) fprintf(f, "<%s>%f</%s>\n", pref_element_strs[(name)], (value), pref_element_strs[(name)])
#define print_pref_int(f,name,value) fprintf(f, "<%s>%d</%s>\n", pref_element_strs[(name)], (value), pref_element_strs[(name)])

static void     data_start_element  ( GMarkupParseContext *context,
                                      const gchar         *element_name,
                                      const gchar        **attribute_names,
                                      const gchar        **attribute_values,
                                      gpointer             user_data,
                                      GError             **error );
static void     data_text           ( GMarkupParseContext *context,
                                      const gchar         *text,
                                      gsize                text_len,  
                                      gpointer             user_data,
                                      GError             **error );
static int      get_element         ( gchar * element_name );


GjayPrefs*
load_prefs ( void ) {
    GMarkupParseContext * parse_context;
    GMarkupParser parser;
    gboolean result = TRUE;
    GError * error;
    char buffer[BUFFER_SIZE];
    FILE * f;
    gssize text_len;
    GjayPrefs *prefs;
	struct parser_data *parser_data;

    /* Set default values */
    prefs = g_malloc0(sizeof(GjayPrefs));

    prefs->rating = DEFAULT_RATING;
    prefs->use_ratings = FALSE;
    prefs->playlist_time = DEFAULT_PLAYLIST_TIME;
    prefs->max_working_set = DEFAULT_MAX_WORKING_SET;
    prefs->variance =
        prefs->hue = 
        prefs->brightness =
        prefs->bpm =
        prefs->freq =
        prefs->path_weight = DEFAULT_CRITERIA;
    prefs->saturation = 1;
    prefs->extension_filter = TRUE;
    prefs->start_color.H = 0;
    prefs->start_color.S = 0.5;
    prefs->start_color.V = 1.0;
    prefs->use_hsv = FALSE;
    prefs->daemon_action = PREF_DAEMON_QUIT;
    prefs->hide_tips = FALSE;
    snprintf(buffer, BUFFER_SIZE, "%s/%s/%s", getenv("HOME"), 
             GJAY_DIR, GJAY_PREFS);
    prefs->music_player = 0;

    f = fopen(buffer, "r");
    if (f) {
	    parser_data = g_malloc0(sizeof(struct parser_data));
		parser_data->prefs = prefs;

        parser.start_element = data_start_element;
        parser.text = data_text;
        parser.end_element = NULL; 

        parse_context = g_markup_parse_context_new(&parser, 0, parser_data, NULL);
        while (result && !feof(f)) {
            text_len = fread(buffer, 1, BUFFER_SIZE, f);
            result = g_markup_parse_context_parse ( parse_context,
                                                    buffer,
                                                    text_len,
                                                    &error);
            error = NULL;
        }
        g_markup_parse_context_free(parse_context);
        fclose(f);
    } 
    return prefs;
}



void save_prefs ( GjayPrefs *prefs) {
    char buffer[BUFFER_SIZE], buffer_temp[BUFFER_SIZE];
    char * utf8;
    FILE * f;
    
    snprintf(buffer, BUFFER_SIZE, "%s/%s/%s", getenv("HOME"), 
             GJAY_DIR, GJAY_PREFS);
    snprintf(buffer_temp, BUFFER_SIZE, "%s_temp", buffer);
    f = fopen(buffer_temp, "w");
    if (f) {
        fprintf(f, "<gjay_prefs version=\"%s\">\n", VERSION);
        if (prefs->song_root_dir) {
            fprintf(f, "<%s", pref_element_strs[PE_ROOTDIR]);
            if (prefs->extension_filter)
                fprintf(f, " %s=\"t\"", pref_element_strs[PE_EXTENSION_FILTER]);
            utf8 = strdup_to_utf8(prefs->song_root_dir);
            fprintf(f, ">%s</%s>\n", 
                    utf8,
                    pref_element_strs[PE_ROOTDIR]);
            g_free(utf8);
        }
        fprintf(f, "<%s", pref_element_strs[PE_FLAGS]);
        if (prefs->hide_tips)
            fprintf(f, " %s=\"t\"", pref_element_strs[PE_HIDE_TIP]);
        if (prefs->wander)
            fprintf(f, " %s=\"t\"", pref_element_strs[PE_WANDER]);
        if (prefs->use_ratings)
            fprintf(f, " %s=\"t\"", pref_element_strs[PE_USE_RATINGS]);
        fprintf(f, "></%s>\n", pref_element_strs[PE_FLAGS]);
        

        fprintf(f, "<%s>", pref_element_strs[PE_START]);
        if (prefs->start_selected)
            fprintf(f, "%s", pref_element_strs[PE_SELECTED]);
        else if (prefs->use_color)
            fprintf(f, "%s", pref_element_strs[PE_COLOR]);
        else
            fprintf(f, "%s", pref_element_strs[PE_RANDOM]);
        fprintf(f, "</%s>\n", pref_element_strs[PE_START]);

        fprintf(f, "<%s>", pref_element_strs[PE_SELECTION_LIMIT]);        
        if (prefs->use_selected_songs)
            fprintf(f, "%s", pref_element_strs[PE_SONGS]);
        else if (prefs->use_selected_dir) 
            fprintf(f, "%s", pref_element_strs[PE_DIR]);
        fprintf(f, "</%s>\n", pref_element_strs[PE_SELECTION_LIMIT]);        
        
        fprintf(f, "<%s>%d</%s>\n",
                pref_element_strs[PE_TIME],
                prefs->playlist_time,
                pref_element_strs[PE_TIME]);

	fprintf(f, "<%s>%d</%s>\n",
		pref_element_strs[PE_MAX_WORKING_SET],
		prefs->max_working_set,
		pref_element_strs[PE_MAX_WORKING_SET]);

        fprintf(f, "<%s %s=\"hsv\">%f %f %f</%s>\n",
                pref_element_strs[PE_COLOR],
                pref_element_strs[PE_TYPE],
                prefs->start_color.H,
                prefs->start_color.S, 
                prefs->start_color.V, 
                pref_element_strs[PE_COLOR]);
                
        if (prefs->rating_cutoff) {
            fprintf(f, "<%s %s=\"t\">", 
                    pref_element_strs[PE_RATING],
                    pref_element_strs[PE_CUTOFF]);
        } else {
            fprintf(f, "<%s>", pref_element_strs[PE_RATING]);
        }
        fprintf(f, "%f", prefs->rating);
        fprintf(f, "</%s>\n", pref_element_strs[PE_RATING]);
        
        print_pref_int(f,PE_DAEMON_ACTION, prefs->daemon_action);
        print_pref_int(f,PE_MUSIC_PLAYER, prefs->music_player);
        print_pref_float(f,PE_VARIANCE,prefs->variance);
        print_pref_float(f,PE_HUE,prefs->hue);
        print_pref_float(f,PE_BRIGHTNESS,prefs->brightness);
        print_pref_float(f,PE_SATURATION,prefs->saturation);
        print_pref_float(f,PE_BPM,prefs->bpm);
        print_pref_float(f,PE_FREQ,prefs->freq);
        print_pref_float(f,PE_PATH_WEIGHT,prefs->path_weight);
        fprintf(f, "</gjay_prefs>\n");
        fclose(f);
        rename(buffer_temp, buffer);
    } else {
        fprintf(stderr, "Unable to write prefs %s\n", buffer);
    }
}


/* Called for open tags <foo bar="baz"> */
void data_start_element  (GMarkupParseContext *context,
                          const gchar         *element_name,
                          const gchar        **attribute_names,
                          const gchar        **attribute_values,
                          gpointer             user_data,
                          GError             **error) {
    gint k;
    pref_element_type attr;
	struct parser_data *parser_data = (struct parser_data*)user_data;
	GjayPrefs *prefs = parser_data->prefs;

    parser_data->element = get_element((char *) element_name);
    
    for (k = 0; attribute_names[k]; k++) {
        attr = get_element((char *) attribute_names[k]);
        switch(attr) {
        case PE_TYPE:
            if (strcasecmp(attribute_values[k], "hsv") == 0) {
                prefs->use_hsv = TRUE;
            }
            break;
        case PE_EXTENSION_FILTER:
            if (parser_data->element == PE_ROOTDIR)
                prefs->extension_filter = TRUE;
            break;
        case PE_HIDE_TIP:
            if (parser_data->element == PE_FLAGS)
                prefs->hide_tips = TRUE;
            break;
        case PE_WANDER:
            if (parser_data->element == PE_FLAGS)
                prefs->wander = TRUE;
            break;
        case PE_USE_RATINGS:
            if (parser_data->element == PE_FLAGS)
                prefs->use_ratings = TRUE;
            break;
        case PE_CUTOFF:
            if (parser_data->element == PE_RATING)
                prefs->rating_cutoff = TRUE;
            break;
        default:
            break;
        }
    }
}    

void data_text ( GMarkupParseContext *context,
                 const gchar         *text,
                 gsize                text_len,  
                 gpointer             user_data,
                 GError             **error) {
    gchar buffer[BUFFER_SIZE];
    gchar * buffer_str;
    pref_element_type val;
	struct parser_data *parser_data = (struct parser_data*)user_data;
	GjayPrefs *prefs = parser_data->prefs;

    memcpy(buffer, text, text_len);
    buffer[text_len] = '\0';

    switch(parser_data->element) {
    case PE_ROOTDIR:
        prefs->song_root_dir = strdup_to_latin1(buffer); 
        break;
    case PE_DAEMON_ACTION:
        prefs->daemon_action = atoi(buffer);
        break;
    case PE_MUSIC_PLAYER:
        prefs->music_player = atoi(buffer);
        break;
    case PE_START:
    case PE_SELECTION_LIMIT:
        val = get_element((char *) buffer);
        switch(val) {
        case PE_SELECTED:
            prefs->start_selected = TRUE;
            break;
        case PE_COLOR:
            prefs->use_color = TRUE;
            break;
        case PE_SONGS:
            prefs->use_selected_songs = TRUE;
            break;  
        case PE_DIR:
            prefs->use_selected_dir = TRUE;
            break;
        case PE_RANDOM:
            /* Don't do anything */
        default:
            break;
        }
        break;
    case PE_RATING:
        prefs->rating = strtof_gjay(buffer, NULL);
        break;
    case PE_HUE:
        prefs->hue = strtof_gjay(buffer, NULL);
        break;
    case PE_BRIGHTNESS:
        prefs->brightness = strtof_gjay(buffer, NULL);
        break;
    case PE_SATURATION:
        prefs->saturation = strtof_gjay(buffer, NULL);
        break;
    case PE_BPM:
        prefs->bpm = strtof_gjay(buffer, NULL);
        break;
    case PE_FREQ:
        prefs->freq = strtof_gjay(buffer, NULL);
        break;
    case PE_VARIANCE:
        prefs->variance = strtof_gjay(buffer, NULL);
        break;
    case PE_PATH_WEIGHT:
        prefs->path_weight = strtof_gjay(buffer, NULL);  
        break;
    case PE_COLOR:
        if (prefs->use_hsv) {
            prefs->start_color.H = strtof_gjay(buffer, &buffer_str);
            prefs->start_color.S = strtof_gjay(buffer_str, &buffer_str);
            prefs->start_color.V = strtof_gjay(buffer_str, NULL);
        } else {
            HB hb;
            hb.H = strtof_gjay(buffer, &buffer_str);
            hb.B = strtof_gjay(buffer_str, NULL);
            prefs->start_color = hb_to_hsv(hb);
        }
        break;
    case PE_TIME:
        prefs->playlist_time = atoi(buffer);
        break;
    default:
        break;
    case PE_MAX_WORKING_SET:
        prefs->max_working_set = atoi(buffer);
        break;
    }
    parser_data->element = PE_LAST;
}

static int get_element ( gchar * element_name ) {
    int k;
    for (k = 0; k < PE_LAST; k++) {
        if (strcasecmp(pref_element_strs[k], element_name) == 0)
            return k;
    }
    return k;
}
