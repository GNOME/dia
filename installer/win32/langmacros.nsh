;;
;; Windows DIA NSIS installer language macros
;;

!macro DIA_MACRO_DEFAULT_STRING LABEL VALUE
  !ifndef "${LABEL}"
    !define "${LABEL}" "${VALUE}"
    !ifdef INSERT_DEFAULT
      !warning "${LANG} lang file mising ${LABEL}, using default.."
    !endif
  !endif
!macroend

!macro DIA_MACRO_LANGSTRING_INSERT LABEL LANG
  LangString "${LABEL}" "${LANG_${LANG}}" "${${LABEL}}"
  !undef "${LABEL}"
!macroend

!macro DIA_MACRO_LANGUAGEFILE_BEGIN LANG
  !define CUR_LANG "${LANG}"
!macroend

!macro DIA_MACRO_LANGUAGEFILE_END
  !define INSERT_DEFAULT
  !include "${DIA_DEFAULT_LANGFILE}"
  !undef INSERT_DEFAULT

  ; DIA Language file Version 2
  ; String labels should match those from the default language file.
  
  ; .onInit checks
  !insertmacro DIA_MACRO_LANGSTRING_INSERT DIA_NO_GTK				${CUR_LANG}
  !insertmacro DIA_MACRO_LANGSTRING_INSERT DIA_NO_INSTALL_OVER			${CUR_LANG}

  ; License Page
  !insertmacro DIA_MACRO_LANGSTRING_INSERT DIA_LICENSE_BUTTON			${CUR_LANG}
  !insertmacro DIA_MACRO_LANGSTRING_INSERT DIA_LICENSE_BOTTOM_TEXT		${CUR_LANG}

  ; Components Page
  !insertmacro DIA_MACRO_LANGSTRING_INSERT DIA_SECTION_TITLE			${CUR_LANG}
  !insertmacro DIA_MACRO_LANGSTRING_INSERT DIA_SECTION_DESCRIPTION		${CUR_LANG}
  !insertmacro DIA_MACRO_LANGSTRING_INSERT TRANSLATIONS_SECTION_TITLE		${CUR_LANG}
  !insertmacro DIA_MACRO_LANGSTRING_INSERT TRANSLATIONS_SECTION_DESCRIPTION	${CUR_LANG}
  !insertmacro DIA_MACRO_LANGSTRING_INSERT PYTHON_SECTION_TITLE                 ${CUR_LANG}
  !insertmacro DIA_MACRO_LANGSTRING_INSERT PYTHON_SECTION_DESCRIPTION           ${CUR_LANG}
  
  ; Installer Finish Page
  !insertmacro DIA_MACRO_LANGSTRING_INSERT DIA_FINISH_VISIT_WEB_SITE		${CUR_LANG}

  ; DIA Section Prompts and Texts
  !insertmacro DIA_MACRO_LANGSTRING_INSERT DIA_UNINSTALL_DESC			${CUR_LANG}
  !insertmacro DIA_MACRO_LANGSTRING_INSERT DIA_PROMPT_WIPEOUT			${CUR_LANG}
  !insertmacro DIA_MACRO_LANGSTRING_INSERT DIA_PROMPT_DIR_EXISTS		${CUR_LANG}

  ; Uninstall Section Prompts
  !insertmacro DIA_MACRO_LANGSTRING_INSERT un.DIA_UNINSTALL_ERROR_1		${CUR_LANG}
  !insertmacro DIA_MACRO_LANGSTRING_INSERT un.DIA_UNINSTALL_ERROR_2		${CUR_LANG}
  !insertmacro DIA_MACRO_LANGSTRING_INSERT un.DIA_UNINSTALLATION_WARNING	${CUR_LANG}
  !insertmacro DIA_MACRO_LANGSTRING_INSERT un.DIA_DOTDIA_WARNING		${CUR_LANG}

  !undef CUR_LANG
!macroend

!macro DIA_MACRO_INCLUDE_LANGFILE LANG FILE
  !insertmacro DIA_MACRO_LANGUAGEFILE_BEGIN "${LANG}"
  !include "${FILE}"
  !insertmacro DIA_MACRO_LANGUAGEFILE_END
!macroend
