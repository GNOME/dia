;;
;;  hungarian.nsh
;;
;;  Gabor Kelemen  <kelemeng@gnome.hu>
;;
;;  Hungarian strings for the Windows Dia NSIS installer.
;;  Windows Code page: 1250
;;
;;  Version 2
;;  Note: If translating this file, replace "!insertmacro DIA_MACRO_DEFAULT_STRING"
;;  with "!define".

; Make sure to update the DIA_MACRO_LANGUAGEFILE_END macro in
; langmacros.nsh when updating this file

; GTK+ was not found
!define DIA_NO_GTK			"Telepítse a GTK+ 2.6.0 vagy újabb verzióját. Ez elérhetõ a dia-installer.sourceforge.net oldalról."

; Don't install over pre 0.95 versions
!define DIA_NO_INSTALL_OVER		"Távolítsa el a 0.95 alatti verziószámú Dia telepítéseket, vagy válasszon másik helyet."

; License Page
!define DIA_LICENSE_BUTTON		"Tovább >"
!define DIA_LICENSE_BOTTOM_TEXT		"A $(^Name) a GPL licenc alatt kerül kiadásra. Az itt látható licenc csak tájékoztatási célt szolgál. $_CLICK"

; Components Page
!define DIA_SECTION_TITLE			"Dia diagramszerkesztõ (kötelezõ)"
!define TRANSLATIONS_SECTION_TITLE	"Fordítások"
!define TRANSLATIONS_SECTION_DESCRIPTION  "A Dia felhasználói felületének fordításai"
!define DIA_SECTION_DESCRIPTION		"Alapvetõ Dia fájlok és programkönyvtárak"
!define PYTHON_SECTION_TITLE        "Python bõvítmény"
!define PYTHON_SECTION_DESCRIPTION  "A Python parancsnyelv 2.3 változatának támogatása. Ne válassza ki, ha a Python nincs telepítve."
 

; Installer Finish Page
!define DIA_FINISH_VISIT_WEB_SITE		"Keresse fel a Dia for Windows weboldalát"

; DIA Section Prompts and Texts
!define DIA_UNINSTALL_DESC		"Dia (csak eltávolítás)"
!define DIA_PROMPT_WIPEOUT		"A régi Dia könyvtárának eltávolítására készül. Folytatni akarja?$\r$\rMegjegyzés: Az esetlegesen telepített nem szabványos bõvítmények törlésre kerülnek.$\rA Dia felhasználói beállításait ez nem érinti."
!define DIA_PROMPT_DIR_EXISTS		"A megadott telepítési könyvtár már létezik. A tartalma$\rtörlésre kerül. Folytatni akarja?"

; Uninstall Section Prompts
!define un.DIA_UNINSTALL_ERROR_1		"Az eltávolító program nem találja a Dia regisztrációs adatbázis-bejegyzéseit. $\rValószínûleg másik felhasználó telepítette ezt az alkalmazást."
!define un.DIA_UNINSTALL_ERROR_2		"Nincs jogosultsága eltávolítani ezt az alkalmazást."
!define un.DIA_UNINSTALLATION_WARNING	"Ez teljesen törli a(z) $INSTDIR könyvtárat és minden alkönyvtárát. Folytatja?"
!define un.DIA_DOTDIA_WARNING		"Ez teljesen törli a(z) $PROFILE\.dia könyvtárat és minden alkönyvtárát. Folytatja?"
