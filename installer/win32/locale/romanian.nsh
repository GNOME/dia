;;
;;  romanian.nsh
;;
;;  Romanian language strings for the Windows Dia NSIS installer.
;;  Windows Code page: 1250
;;
;;  Author for this translation: Cristian Secarã <cristi AT secarica DOT ro>, 2010.
;;
;;  Version 2

; GTK+ was not found
!define DIA_NO_GTK			"GTK+ nu este instalat. Folosiþi programul complet de instalare, care este disponibil de la http://dia-installer.de."

; Don't install over pre 0.95 versions
!define DIA_NO_INSTALL_OVER		"Eliminaþi complet instalarea Dia veche, sau instalaþi Dia într-o locaþie diferitã."

; License Page
!define DIA_LICENSE_BUTTON		"Înainte >"
!define DIA_LICENSE_BOTTOM_TEXT		"$(^Name) este publicat sub licenþa publicã generalã GNU (GPL). Licenþa este furnizatã aici numai cu scop informativ. $_CLICK"

; Components Page
!define DIA_SECTION_TITLE			"Editorul de diagrame Dia (necesar)"
!define DIA_SECTION_DESCRIPTION		"Fiºiere ºi dll-uri principale pentru Dia"
!define TRANSLATIONS_SECTION_TITLE	"Traduceri"
!define TRANSLATIONS_SECTION_DESCRIPTION  "Traduceri opþionale ale interfeþei utilizator Dia"
!define PYTHON_SECTION_TITLE              "Plug-in Python"
!define PYTHON_SECTION_DESCRIPTION              "Suport pentru limbajul de scripting Python 2.3. Nu selectaþi aceastã opþiune dacã Python nu este instalat."

; Installer Finish Page
!define DIA_FINISH_VISIT_WEB_SITE		"Visitaþi pagina Web Dia pentru Windows"

; DIA Section Prompts and Texts
!define DIA_UNINSTALL_DESC		"Dia (numai eliminare)"
!define DIA_PROMPT_WIPEOUT		"Directorul vechi care conþine Dia este pe cale de a fi ºters. Doriþi sã continuaþi?$\r$\rNotã: orice pluginuri standard pe care poate l-aþi instalat vor fi ºterse.$\rConfigurãrile per utilizator nu vor fi afectate."
!define DIA_PROMPT_DIR_EXISTS		"Directorul specificat existã deja. Tot conþinutul$\rva fi ºters. Doriþi sã continuaþi?"

; Uninstall Section Prompts
!define un.DIA_UNINSTALL_ERROR_1		"Programul de dezinstalare nu a putut gãsi intrãrile în registru pentru Dia.$\rEste posibil ca alt utilizator sã fi ºters aceastã aplicaþie."
!define un.DIA_UNINSTALL_ERROR_2		"Nu aveþi permisiunea de a dezinstala aceastã aplicaþie."
!define un.DIA_UNINSTALLATION_WARNING	"Aceasta va ºterge complet $INSTDIR ºi toate subdirectoarele. Continuaþi?"
!define un.DIA_DOTDIA_WARNING		"Aceasta va ºterge complet $PROFILE\.dia ºi toate subdirectoarele. Continuaþi?"
