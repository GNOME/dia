;;
;;  french.nsh
;;
;;  French language strings for the Windows Dia NSIS installer.
;;  Windows Code page: 1252
;;
;;  Version 2
;;  Author: Yannick LE NY <y.le.ny@ifrance.com>, 2005.
;;  Author: Steffen Macke <sdteffen@gmail.com> 2006

; No GTK+ was found
!define DIA_NO_GTK				"GTK+ is not installed. Please use the full installer. It is available from http://dia-installer.de." 

; Don't install over pre 0.95 versions
!define DIA_NO_INSTALL_OVER			"Please remove old Dia installations completely or install Dia to a different location."

; License Page
!define  DIA_LICENSE_BUTTON			"Suivant >"
!define  DIA_LICENSE_BOTTOM_TEXT		"$(^Name) est publi� sous la licence GPL. La licence est fourni ici uniquement pour information. $_CLICK"

; Components Page
!define  DIA_SECTION_TITLE			"Editeur de diagramme Dia (requis)"
!define  DIA_SECTION_DESCRIPTION		"Fichiers principaux Dia et dlls"
!define  TRANSLATIONS_SECTION_TITLE		"Translations"
!define  TRANSLATIONS_SECTION_DESCRIPTION	"Optional translations of the Dia user interface."
!define  PYTHON_SECTION_TITLE                   "Python plug-in"
!define PYTHON_SECTION_DESCRIPTION              "Support for the Python Scripting Language 2.3. Do not select this if Python is not installed."

; Installer Finish Page
!define  DIA_FINISH_VISIT_WEB_SITE		"Visiter la page Web de Dia pour Windows"

; DIA Section Prompts and Texts
!define  DIA_UNINSTALL_DESC			"Dia (supprimer uniquement)"
!define  DIA_PROMPT_WIPEOUT			"Votre ancien repertoire Dia est sur le point d'etre supprime. Voulez-vous continuer?$\r$\rNote: Tous les plugins non-standard que vous avez installe seront supprim�s.$\rLes parametres utilisateur ne seront pas affectes."
!define  DIA_PROMPT_DIR_EXISTS		"Le repertoire d'installation que vous aves indique existe deja. Tout le contenu de $\rsera supprime. Voulez-vous continuer?"

; Uninstall Section Prompts
!define  un.DIA_UNINSTALL_ERROR_1		"Le desinstalleur ne peut pas trouver les entrees de registre pour Dia.$\rIl est probable qu'un autre utilisateur a installe cette application."
!define  un.DIA_UNINSTALL_ERROR_2		"Vous n'avez pas la permission de desinstaller cette application."
!define  un.DIA_UNINSTALLATION_WARNING		"Cela supprimera completement $INSTDIR et tous ses sous-repertoire. Continuer?"
!define  un.DIA_DOTDIA_WARNING 			"Cela supprimera completement $PROFILE\.dia et tous ses sous-repertoire. Continuer?"
