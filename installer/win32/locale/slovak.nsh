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
!define DIA_NO_GTK			"Prosím, nainštalujte si GTK+ verziu 2.6.0 alebo vyššiu. Nájdete ju na dia-installer.sourceforge.net"

; Don't install over pre 0.95 versions
!define DIA_NO_INSTALL_OVER		"Prosím, úplne odstránte staré inštalácie Dia alebo nainštalujte Dia na iné miesto."

; License Page
!define DIA_LICENSE_BUTTON		"Ïalej >"
!define DIA_LICENSE_BOTTOM_TEXT		"$(^Name) je uvo¾nenı za podmienok licencie GPL. Licencia je tu poskytnutá len pre informatívne úèely. $_CLICK"

; Components Page
!define DIA_SECTION_TITLE			"Dia - editor diagramov (povinné)"
!define TRANSLATIONS_SECTION_TITLE	"Preklady"
!define TRANSLATIONS_SECTION_DESCRIPTION  "Volitelné preklady pouívate¾ského rozhrania Dia"
!define DIA_SECTION_DESCRIPTION		"Hlavné súbory a kninice Dia"
!define PYTHON_SECTION_TITLE              "Zásuvnı modul Python"
!define PYTHON_SECTION_DESCRIPTION              "Podpora skriptovacieho jazyka Python 2.3. Nevyberajte túto vo¾bu, ak nie je Python nainštalovanı."

; Installer Finish Page
!define DIA_FINISH_VISIT_WEB_SITE		"Navštívte webstránku Dia pre Windows"

; DIA Section Prompts and Texts
!define DIA_UNINSTALL_DESC		"Dia (iba odstráni)"
!define DIA_PROMPT_WIPEOUT		"Váš starı adresár Dia bude zmazanı. Chcete pokraèova?$\r$\rPozn.: Všetky neštandardné zásuvné moduly, ktoré ste mono nainštalovali, budú zmazané.$\rPouívate¾ské nastavenia Dia nebudú ovplyvnené."
!define DIA_PROMPT_DIR_EXISTS		"Inštalaènı adresár, ktorı ste uviedli, u existuje. Všetok obsah$\rbude zmazanı. Chcete pokraèova?"

; Uninstall Section Prompts
!define un.DIA_UNINSTALL_ERROR_1		"Program na deinštaláciu nenašiel záznamy Dia v registri.$\rJe pravdepodobné, e túto aplikáciu nainštaloval inı pouívate¾."
!define un.DIA_UNINSTALL_ERROR_2		"Nemáte povolenie odinštalova túto aplikáciu."
!define un.DIA_UNINSTALLATION_WARNING	"Tımto sa úplne zmae $INSTDIR a všetky jeho podadresáre. Pokraèova?"
!define un.DIA_DOTDIA_WARNING		"Tımto sa úplne zmae $PROFILE\.dia a všetky jeho podadresáre. Pokraèova?"
