/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Sozi objects support
 * Copyright (C) 2014 Paul Chavent
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "object.h"
#include "message.h"
#include "dia_dirs.h"

#include "intl.h"
#include "plug-ins.h"

#include "sozi-player.h"

extern DiaObjectType sozi_frame_type;
extern DiaObjectType sozi_media_type;

const gchar *sozi_version_ptr = 0;
const gchar *sozi_js_ptr = 0;
const gchar *sozi_extras_media_js_ptr = 0;
const gchar *sozi_css_ptr = 0;

static voidx
load_sozi_from_filesystem(const gchar *directory)
{
  struct
  {
    const gchar * filename;
    gchar *       data;
  } sozi [] =
      {
        { "sozi.version"         , 0 },
        { "sozi.js"              , 0 },
        { "sozi_extras_media.js" , 0 },
        { "sozi.css"             , 0 }
      };
  static const unsigned sozi_cnt = sizeof(sozi)/sizeof(sozi[0]);
  unsigned i;
  for (i = 0; i < sozi_cnt; i++) {
    GError * err = 0;
    gchar * filename = g_strconcat(directory, G_DIR_SEPARATOR_S,
                                   sozi[i].filename, NULL);
    if (!g_file_get_contents(filename,
                             &sozi[i].data,
                             NULL,
                             &err)) {
      g_error_free (err);
      while(i--) {
        g_free (sozi[i].data);
      }
      g_free(filename);
      break;
    }
    g_free(filename);
  }

  /* all files must be present to be taken into account */
  if (i == sozi_cnt) {
    sozi_version_ptr         = sozi[0].data;
    sozi_js_ptr              = sozi[1].data;
    sozi_extras_media_js_ptr = sozi[2].data;
    sozi_css_ptr             = sozi[3].data;
  }
}

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  char * dir;

  if (!dia_plugin_info_init(info, "Sozi",_("Sozi presentation objects"),
			    NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  object_register_type(&sozi_frame_type);
  object_register_type(&sozi_media_type);

  /* default to the player that comes with dia sources */

  sozi_version_ptr         = (const gchar *)sozi_version;
  sozi_js_ptr              = (const gchar *)sozi_min_js;
  sozi_extras_media_js_ptr = (const gchar *)sozi_extras_media_min_js;
  sozi_css_ptr             = (const gchar *)sozi_min_css;

  /* look for a player on the filesystem */

  dir = dia_get_data_directory("sozi");
  if(dir) {
    load_sozi_from_filesystem(dir);
    g_free(dir);
  }

  dir = dia_config_filename("sozi");
  if(dir) {
    load_sozi_from_filesystem(dir);
    g_free(dir);
  }

  if (getenv("DIA_SOZI_PATH")) {
    load_sozi_from_filesystem(getenv("DIA_SOZI_PATH"));
  }

  return DIA_PLUGIN_INIT_OK;
}
