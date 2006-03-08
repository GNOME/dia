;;
;;  german.nsh
;;
;;  German language strings for the Windows DIA NSIS installer.
;;  Windows Code page: 1252
;;
;;  Author: Bjoern Voigt <bjoern@cs.tu-berlin.de>, 2003.
;;  Version 2
;;

; No GTK+ was found
!define DIA_NO_GTK				"Bitte installieren sie GTK+ Version 2.6.0 oder besser. Es kann unter http://dia-installer.sourceforge.net heruntergeladen werden."

; License Page
!define DIA_LICENSE_BUTTON			"Weiter >"
!define DIA_LICENSE_BOTTOM_TEXT		"$(^Name) wird unter der GPL Lizenz ver?ffentlicht. Die Lizenz hier dient nur der Information. $_CLICK"
 
; Components Page
!define DIA_SECTION_TITLE			"Dia Diagrameditor (erforderlich)"
!define DIA_SECTION_DESCRIPTION			"Dia Dateien und -DLLs"
!define TRANSLATIONS_SECTION_TITLE		"Uebersetzungen"
!define TRANSLATIONS_SECTION_DESCRIPTION 	"Optional Uebersetzungen der Dia-Benutzeroberflaeche fuer verschiedene Sprachen"
  
; Installer Finish Page
!define DIA_FINISH_VISIT_WEB_SITE		"Besuchen Sie die Dia für Windows Webseite"
 
; DIA Section Prompts and Texts
!define DIA_UNINSTALL_DESC			"Dia (nur entfernen)"
!define DIA_PROMPT_WIPEOUT			"Ihre altes Dia-Verzeichnis soll gel?scht werden. M?chten Sie fortfahren?$\r$\rHinweis: Alle nicht-Standard Plugins, die Sie evtl. installiert haben werden$\rgel?scht. Dia-Benutzereinstellungen sind nicht betroffen."
!define DIA_PROMPT_DIR_EXISTS		"Das Installationsverzeichnis, da? Sie angegeben haben, existiert schon. Der Verzeichnisinhalt$\rwird gel?scht. M?chten Sie fortfahren?"
  
; GTK+ Themes section
!define GTK_NO_THEME_INSTALL_RIGHTS		"Sie haben keine Berechtigung, um ein GTK+ Theme zu installieren."
 
; Uninstall Section Prompts
!define un.DIA_UNINSTALL_ERROR_1		"Der Deinstaller konnte keine Registrierungsshlüssel für Dia finden.$\rEs ist wahrscheinlich, da? ein anderer Benutzer diese Anwendunng installiert hat."
!define un.DIA_UNINSTALL_ERROR_2		"Sie haben keine Berechtigung, diese Anwendung zu deinstallieren."
!define un.DIA_UNINSTALLATION_WARNING		"Die Deinstallation wird $INSTDIR und alle Unterverzeichnisse komplett l?schen. Fortfahren?"
!define un.DIA_DOTDIA_WARNING			"Die Deinstallation wird $PROFILE\.dia und all Unterverzeichnisse komplett l?schen. Fortfahren?"

