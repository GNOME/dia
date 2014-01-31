/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * gennsh.c
 * Copyright (C) 2009,2011,2013 Steffen Macke <sdteffen@sdteffen.de>
 *
 * gennsh is a program that allows to generate locale file for the
 * Dia for Windows installer
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <libintl.h>
#include <glib.h>

#define _(String) gettext(String)

int main(int argc, char *argv[])
{

  bindtextdomain("dia", "../../../build/win32/locale");
  bind_textdomain_codeset("dia", "UTF-8");
  textdomain("dia");

  /* Installer message if no GTK+ was found */
  printf("!define DIA_NO_GTK \"%s\"\n", _("GTK+ is not installed. Please use the full installer. It is available from http://dia-installer.de."));

  /* Installer message: Don't install over pre 0.95 versions */
  printf("!define DIA_NO_INSTALL_OVER \"%s\"\n", _("Please remove old Dia installations completely or install Dia to a different location."));

  /* Installer message:  License Page */
  printf("!define DIA_LICENSE_BUTTON \"%s\"\n", _("Next >"));

  /* Installer message, keep the $(^Name) and $_CLICK, these will be replaced */
  printf("!define DIA_LICENSE_BOTTOM_TEXT \"%s\"\n", _("$(^Name) is released under the GPL license. The license is provided here for information purposes only. $_CLICK"));

  /* Installer message: Components page */
  printf("!define DIA_SECTION_TITLE \"%s\"\n", _("Dia Diagram Editor (required)"));

  /* Installer message: Components page */
  printf("!define TRANSLATIONS_SECTION_TITLE \"%s\"\n", _("Translations"));

  /* Installer message: Component description */
  printf("!define TRANSLATIONS_SECTION_DESCRIPTION \"%s\"\n", _("Optional translations of the Dia user interface"));

  /* Installer message: Component description */
  printf("!define DIA_SECTION_DESCRIPTION \"%s\"\n", _("Core Dia files and dlls"));

  /* Installer message: Component name */
  printf("!define PYTHON_SECTION_TITLE \"%s\"\n", _("Python plug-in"));

  /* Installer message: Component description */
  printf("!define PYTHON_SECTION_DESCRIPTION \"%s\"\n", _("Support for the Python Scripting Language 2.3. Do not select this if Python is not installed."));

  /* Installer message: hyperlink text on finish page */
  printf("!define DIA_FINISH_VISIT_WEB_SITE \"%s\"\n", _("Visit the Dia for Windows Web Page"));

  /* Installer message: Dia uninstaller entry in Control Panel */
  printf("!define DIA_UNINSTALL_DESC \"%s\"\n", _("Dia (remove only)"));

  /* Installer message: directory delete confirmation line 1 */
  printf("!define DIA_PROMPT_WIPEOUT \"%s$\\r$\\r%s$\\r%s\"\n", _("Your old Dia directory is about to be deleted. Would you like to continue?"),
	/* Installer message: directory delete confirmation line 2 */
	 _("Note: Any non-standard plugins that you may have installed will be deleted."),
	/* Installer message: directory delete confirmation line 3*/
	_("Dia user settings will not be affected."));

  /* Installer message: DIA_PROMPT_DIR_EXISTS line 1 */
  printf("!define DIA_PROMPT_DIR_EXISTS \"%s$\\r%s\"\n", _("The installation directory you specified already exists. Any contents"),
	/* Installer message: DIA_PROMP_DIR_EXISTS line 2 */
	_("will be deleted. Would you like to continue?"));

  /* Installer message: registry entries not found line 1 */
  printf("!define un.DIA_UNINSTALL_ERROR_1 \"%s$\\r%s\"\n", _("The uninstaller could not find registry entries for Dia."),
	/* Installer message: registry entries not found line 2 */
	_("It is likely that another user installed this application."));

  /* Installer message: Uninstall error message */
  printf("!define un.DIA_UNINSTALL_ERROR_2 \"%s\"\n", _("You do not have permission to uninstall this application."));

  /* Installer message: Uninstallation warning. Keep $INSTDIR */
  printf("!define un.DIA_UNINSTALLATION_WARNING \"%s\"\n", _("This will completely delete $INSTDIR and all subdirectories. Continue?"));

  /* Installer message: Uninstallation warning. Keep $\PROFILE\.dia */
  printf("!define un.DIA_DOTDIA_WARNING \"%s\"\n", _("This will completely delete $PROFILE\\.dia and all subdirectories. Continue?"));
  return 0;
}
