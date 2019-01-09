* Sheets editor is broken
* GtkAction needs porting to GAction (and therefore menubar GtkUIManager -> GMenuModel)
* We need G\[tk\]Application to use GAction so we need a cleaner split between cli and GUI
* Reorganise dia & libdia into dia, libdia & libdiaui where most widgetry lives in libdiaui
  and ideally libdia doesn't depend on Gtk at all
* Keyboard shortcuts are tempremental when working with DiaCanvas
* Scroll wheel is broken for DiaCanvas, Probably need to implement GtkScrollable and use
  a standard GtkScrolledWindow instead of a pair of GtkScrollbar
* DiaDisplay should probably be a GtkGrid subclass
* DiaObject & co whould ideally become GObjects with much of the property infrastructure
  being ported to GObject properties or other system open to introspection
* Plugin API/ABI is already broken and poorly defined, We could avoid reinventing the
  wheel by using libpeas which would give us the chance to support plugins in languages
  other than C
* PyDia is largly broken (not least by being Py2 & using PyGtk) but with libpeas we
  could probably just drop it without loosing much
* Navigation tool is largly broken, esp on Wayland
