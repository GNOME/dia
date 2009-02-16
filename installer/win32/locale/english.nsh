;;
;;  english.nsh
;;
;;  Default language strings for the Windows Dia NSIS installer.
;;  Windows Code page: 1252
;;
;;  Version 2
;;  Note: If translating this file, replace "!insertmacro DIA_MACRO_DEFAULT_STRING"
;;  with "!define".

; Make sure to update the DIA_MACRO_LANGUAGEFILE_END macro in
; langmacros.nsh when updating this file

; GTK+ was not found
!insertmacro DIA_MACRO_DEFAULT_STRING DIA_NO_GTK			"GTK+ is not installed. Please use the full installer. It is available from http://dia-installer.de."

; Don't install over pre 0.95 versions
!insertmacro DIA_MACRO_DEFAULT_STRING DIA_NO_INSTALL_OVER		"Please remove old Dia installations completely or install Dia to a different location."

; License Page
!insertmacro DIA_MACRO_DEFAULT_STRING DIA_LICENSE_BUTTON		"Next >"
!insertmacro DIA_MACRO_DEFAULT_STRING DIA_LICENSE_BOTTOM_TEXT		"$(^Name) is released under the GPL license. The license is provided here for information purposes only. $_CLICK"

; Components Page
!insertmacro DIA_MACRO_DEFAULT_STRING DIA_SECTION_TITLE			"Dia Diagram Editor (required)"
!insertmacro DIA_MACRO_DEFAULT_STRING TRANSLATIONS_SECTION_TITLE	"Translations"
!insertmacro DIA_MACRO_DEFAULT_STRING TRANSLATIONS_SECTION_DESCRIPTION  "Optional translations of the Dia user interface"
!insertmacro DIA_MACRO_DEFAULT_STRING DIA_SECTION_DESCRIPTION		"Core Dia files and dlls"
!insertmacro DIA_MACRO_DEFAULT_STRING PYTHON_SECTION_TITLE              "Python plug-in"
!insertmacro DIA_MACRO_DEFAULT_STRING PYTHON_SECTION_DESCRIPTION              "Support for the Python Scripting Language 2.3. Do not select this if Python is not installed."

; Installer Finish Page
!insertmacro DIA_MACRO_DEFAULT_STRING DIA_FINISH_VISIT_WEB_SITE		"Visit the Dia for Windows Web Page"

; DIA Section Prompts and Texts
!insertmacro DIA_MACRO_DEFAULT_STRING DIA_UNINSTALL_DESC		"Dia (remove only)"
!insertmacro DIA_MACRO_DEFAULT_STRING DIA_PROMPT_WIPEOUT		"Your old Dia directory is about to be deleted. Would you like to continue?$\r$\rNote: Any non-standard plugins that you may have installed will be deleted.$\rDia user settings will not be affected."
!insertmacro DIA_MACRO_DEFAULT_STRING DIA_PROMPT_DIR_EXISTS		"The installation directory you specified already exists. Any contents$\rwill be deleted. Would you like to continue?"

; Uninstall Section Prompts
!insertmacro DIA_MACRO_DEFAULT_STRING un.DIA_UNINSTALL_ERROR_1		"The uninstaller could not find registry entries for Dia.$\rIt is likely that another user installed this application."
!insertmacro DIA_MACRO_DEFAULT_STRING un.DIA_UNINSTALL_ERROR_2		"You do not have permission to uninstall this application."
!insertmacro DIA_MACRO_DEFAULT_STRING un.DIA_UNINSTALLATION_WARNING	"This will completely delete $INSTDIR and all subdirectories. Continue?"
!insertmacro DIA_MACRO_DEFAULT_STRING un.DIA_DOTDIA_WARNING		"This will completely delete $PROFILE\.dia and all subdirectories. Continue?"
