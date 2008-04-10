;;
;;  slovak.nsh
;;
;; Ivan Masar <helix84@users.sourceforge.net>
;;
;;  Default language strings for the Windows Dia NSIS installer.
;;  Windows Code page: 1250
;;
;;  Version 2
;;  Note: If translating this file, replace "!define DIA_MACRO_DEFAULT_STRING"
;;  with "!define".

; Make sure to update the DIA_MACRO_LANGUAGEFILE_END macro in
; langmacros.nsh when updating this file

; GTK+ was not found
!define DIA_NO_GTK			"Prosím, nainštalujte si GTK+ verziu 2.6.0 aleb vyššiu. Nájdete ju na dia-installer.sourceforge.net"

; Don't install over pre 0.95 versions
!define DIA_NO_INSTALL_OVER		"Prosím, úplne odstránte staré inštalácie Dia alebo nainštalujte Dia na iné miesto."

; License Page
!define DIA_LICENSE_BUTTON		"Dalej >"
!define DIA_LICENSE_BOTTOM_TEXT		"$(^Name) je uvolnený za podmienok licencie GPL. Licencia je tu poskytnutá len pre informatívne úcely. $_CLICK"

; Components Page
!define DIA_SECTION_TITLE			"Dia - editor diagramov (potrebné)"
!define TRANSLATIONS_SECTION_TITLE	"Preklady"
!define TRANSLATIONS_SECTION_DESCRIPTION  "Volitelné preklady používatelského rozhrania Dia"
!define DIA_SECTION_DESCRIPTION		"Hlavné súbory a knižnice Dia"
!define PYTHON_SECTION_TITLE              "Zásuvný modul Python"
!define PYTHON_SECTION_DESCRIPTION              "Podpora skriptovacieho jazyka Python 2.2. Nevyberajte túto volbu, ak nie je Python nainštalovaný."

; Installer Finish Page
!define DIA_FINISH_VISIT_WEB_SITE		"Navštívte webstránku Dia pre Windows"

; DIA Section Prompts and Texts
!define DIA_UNINSTALL_DESC		"Dia (iba odstránit)"
!define DIA_PROMPT_WIPEOUT		"Váš starý adresár Dia bude zmazaný. Chcete pokracovat?$\r$\rPozn.: Všetky neštandardné zásuvné moduly, ktoré ste možno nainštalovali, budú zmazané.$\rPoužívatelské nastavenia Dia nebudú ovplyvnené."
!define DIA_PROMPT_DIR_EXISTS		"Inštalacný adresár, ktorý ste uviedli, už existuje. Všetok obsah$\rbude zmazaný. Chcete pokracovat?"

; Uninstall Section Prompts
!define un.DIA_UNINSTALL_ERROR_1		"Program na deinštaláciu nenašiel záznamy Dia v registri.$\rJe pravdepodobné, že túto aplikáciu nainštaloval iný používatel."
!define un.DIA_UNINSTALL_ERROR_2		"Nemáte povolenie odinštalovat túto aplikáciu."
!define un.DIA_UNINSTALLATION_WARNING	"Týmto sa úplne zmaže $INSTDIR a všetky jeho podadresáre. Pokracovat?"
!define un.DIA_DOTDIA_WARNING		"Týmto sa úplne zmaže $PROFILE\.dia a všetky jeho podadresáre. Pokracovat?"
