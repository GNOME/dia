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
!define DIA_NO_GTK				"GTK+ ist nicht installiert. Bitte benutzen Sie die komplette Installationsprogramm. Es kann unter http://dia-installer.de heruntergeladen werden."

; Don't install over pre 0.95 versions
!define DIA_NO_INSTALL_OVER			"Bitte deinstallieren Sie Dia Versionen vor 0.95 komplett oder installieren Sie Dia unter einem anderen Pfad."

; License Page
!define DIA_LICENSE_BUTTON			"Weiter >"
!define DIA_LICENSE_BOTTOM_TEXT		"$(^Name) wird unter der GPL Lizenz veröffentlicht. Die Lizenz hier dient nur der Information. $_CLICK"
 
; Components Page
!define DIA_SECTION_TITLE			"Dia Diagrameditor (erforderlich)"
!define DIA_SECTION_DESCRIPTION			"Dia Dateien und -DLLs"
!define TRANSLATIONS_SECTION_TITLE		"Übersetzungen"
!define TRANSLATIONS_SECTION_DESCRIPTION 	"Optional Übersetzungen der Dia-Benutzeroberfläche für verschiedene Sprachen"
!define PYTHON_SECTION_TITLE                    "Python plug-in"
!define PYTHON_SECTION_DESCRIPTION              "Unterstützung für die Skriptsprache Python 2.3. Bitte nur installieren, falls Python vorhanden ist."
  
; Installer Finish Page
!define DIA_FINISH_VISIT_WEB_SITE		"Besuchen Sie die Dia für Windows Webseite"
 
; DIA Section Prompts and Texts
!define DIA_UNINSTALL_DESC			"Dia (nur entfernen)"
!define DIA_PROMPT_WIPEOUT			"Ihre altes Dia-Verzeichnis soll gelöscht werden. Möchten Sie fortfahren?$\r$\rHinweis: Alle nicht-Standard Plugins, die Sie evtl. installiert haben werden$\rgelöscht. Dia-Benutzereinstellungen sind nicht betroffen."
!define DIA_PROMPT_DIR_EXISTS		"Das Installationsverzeichnis, dass Sie angegeben haben, existiert schon. Der Verzeichnisinhalt$\rwird gelöscht. Möchten Sie fortfahren?"
  
; GTK+ Themes section
!define GTK_NO_THEME_INSTALL_RIGHTS		"Sie haben keine Berechtigung, um ein GTK+ Theme zu installieren."
 
; Uninstall Section Prompts
!define un.DIA_UNINSTALL_ERROR_1		"Der Deinstaller konnte keine Registrierungschlüssel für Dia finden.$\rEs ist wahrscheinlich, dass ein anderer Benutzer diese Anwendunng installiert hat."
!define un.DIA_UNINSTALL_ERROR_2		"Sie haben keine Berechtigung, diese Anwendung zu deinstallieren."
!define un.DIA_UNINSTALLATION_WARNING		"Die Deinstallation wird $INSTDIR und alle Unterverzeichnisse komplett löschen. Fortfahren?"
!define un.DIA_DOTDIA_WARNING			"Die Deinstallation wird $PROFILE\.dia und alle Unterverzeichnisse komplett löschen. Fortfahren?"

