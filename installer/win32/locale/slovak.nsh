;;
;;  slovak.nsh
;;
;;  Default language strings for the Windows Dia NSIS installer.
;;  Windows Code page: 1252
;;
;;  Version 2
;;  Note: If translating this file, replace "!define DIA_MACRO_DEFAULT_STRING"
;;  with "!define".

; Make sure to update the DIA_MACRO_LANGUAGEFILE_END macro in
; langmacros.nsh when updating this file

; GTK+ was not found
!define DIA_MACRO_DEFAULT_STRING DIA_NO_GTK			"Prosím, nainštalujte si GTK+ verziu 2.6.0 aleb vyššiu. Je dostupná z dia-installer.sourceforge.net"

; License Page
!define DIA_MACRO_DEFAULT_STRING DIA_LICENSE_BUTTON		"Ďalej >"
!define DIA_MACRO_DEFAULT_STRING DIA_LICENSE_BOTTOM_TEXT		"$(^Name) je uvoľnený za podmienok licencie GPL. Licencia je tu poskytnutá len pre informatívne účely. $_CLICK"

; Components Page
!define DIA_MACRO_DEFAULT_STRING DIA_SECTION_TITLE			"Dia Diagram Editor (potrebné)"
!define DIA_MACRO_DEFAULT_STRING TRANSLATIONS_SECTION_TITLE	"Preklady"
!define DIA_MACRO_DEFAULT_STRING TRANSLATIONS_SECTION_DESCRIPTION  "Voliteľné preklady používateľského rozhrania Dia"
!define DIA_MACRO_DEFAULT_STRING DIA_SECTION_DESCRIPTION		"Hlavné súbory a knižnice Dia"

; Installer Finish Page
!define DIA_MACRO_DEFAULT_STRING DIA_FINISH_VISIT_WEB_SITE		"Navštívte webstránku Dia pre Windows"

; DIA Section Prompts and Texts
!define DIA_MACRO_DEFAULT_STRING DIA_UNINSTALL_DESC		"Dia (iba odstrániť)"
!define DIA_MACRO_DEFAULT_STRING DIA_PROMPT_WIPEOUT		"Váš starý adresár Dia bude zmazaný. Chcete pokračovať?$\r$\rPozn.: Všetky neštandardné zásuvné moduly, ktoré ste možno nainštalovali, budú zmazané.$\rPoužívateľské nastavenia Dia nebudú ovplyvnené."
!define DIA_MACRO_DEFAULT_STRING DIA_PROMPT_DIR_EXISTS		"Inštalačný adresár, ktorý ste uviedli, už existuje. Všetok obsah$\rbude zmazaný. Chcete pokračovať?"

; Uninstall Section Prompts
!define DIA_MACRO_DEFAULT_STRING un.DIA_UNINSTALL_ERROR_1		"Program pre odinštaláciu nenašiel záznamy Dia v registri.$\rJe preavdepodobné, že túto aplikáciu inštaloval iný používateľ."
!define DIA_MACRO_DEFAULT_STRING un.DIA_UNINSTALL_ERROR_2		"Nemáte povolenie odinštalovať túto aplikáciu."
!define DIA_MACRO_DEFAULT_STRING un.DIA_UNINSTALLATION_WARNING	"Týmto sa úplne zmaže $INSTDIR a všetky jeho podadresáre. Pokračovať?"
!define DIA_MACRO_DEFAULT_STRING un.DIA_DOTDIA_WARNING		"Týmto sa úplne zmaže $PROFILE\.dia a všetky jeho podadresáre. Pokračovať?"
