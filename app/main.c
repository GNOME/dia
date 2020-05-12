/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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
#include <config.h>

#include "app_procs.h"

#include "interface.h"
#include "persistence.h"

int main(int argc, char *argv[])
{
  char *lang;
  char *val;
  persistence_load();
  lang = persistence_register_string ("lang_lang", "");
  val = getenv ("LANG");
  if (val == NULL || lang != NULL && lang[0] != NULL && strcmp(val, lang) != 0) {
      #ifdef G_OS_WIN32
      _putenv_s("LANG", lang);
      #else
      setenv("LANG", lang, 1);
      #endif
  }
  app_init(argc, argv);

  if (!app_is_interactive())
    return 0;

  toolbox_show();

  app_splash_done();
  
  gtk_main ();

  return 0;
}

