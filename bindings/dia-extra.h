/*
 * C++ implementation of Dia Bindings Helper Functions
 *
 * Copyright 2007, Hans Breuer, GPL, see COPYING
 */

namespace dia {
  class Object;
  class Objects;

  //! register plug-ins, beware of double bindings
  void register_plugins (void);
  //! simplified message dialog
  void message (int severity, const char* msg);
  //! SWIG can't wrap the original directly, lets try this one
  Object* group_create (Objects* objects);
}
