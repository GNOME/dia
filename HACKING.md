# Coding on Dia

Feel free to hack away at dia, but you're advised to contact
the dia maintainers and/or the mailing list if you do any
larger work --- this is in everyone's interest so that work is
not duplicated.

For more information on the authors, maintainers, etc., of dia,
please see the file AUTHORS and the git history, after 20 years a lot of people have worked on Dia

Visit the dia repository at https://gitlab.gnome.org/GNOME/dia
for more information on the dia mailing list and many other
dia-related things.

If you need to alter the list of contributors, documentors,
etc., the main list is in app/authors.h.


## Some comments about the source
Everything on the screen 'inherits' from the structure Object
in `lib/object.h`. (PS. this is a nice place to start reading the code).

Inherits in C means (as in GTK) that it begins with a copy of that structure.
Some base classes exists in `lib/`, for example:

- `element.h` (for doing 'box-like' objects)
- `connection.h` (for doing 'line-like' objects)
- `orth_conn.h` (for doing connections with orthogonal lines, like the uml-stuff)
- `render_object.h` (for doing picture-like objects).

These base classes are then subclassed in the different object in the
object-libraries like objects/standard object/UML and object/network.

The objects work by filling out two structures that the main program (`app/*`)
uses to handle the objects. The ObjectType structure which consists of some
info and a pointer to the type-operations (create+load+save). There's one
ObjectType per object type currently loaded. Then the Object structure, there
exists a copy of this for each object of the kind on screen (and in
copy-buffers). This contains some info like: type, bounding_box, position,
handles (the rectangles you move with the mouse) and connections. It also
contains a pointer to the object-operations. These are called from the main
program when if wants the object to do something. All ops take an Object as
the first argument. This is usually casted to the subtype in the function
headed (gives all those pita warnings) so that you directly can use the info
stored in the subclasses. Most ops are quite self-describing, and the code can
be copy-pasted from an object like the one you're doing. Rendering to
screen/postscript is done through the 'DiaRenderer' abstraction that can be found
in `lib/diarenderer.h`.

### XML based objects
You can (from version 0.80) create new objects using a SVG like XML language.
The file doc/custom-shapes has more information about this.

### Handles and connection points
An object has handles to resize it. A handle can be moved either because
the user dragged it with the mouse, or the handle is attached to another
object, which moved itself. The handles are displayed as little squares
(red: normal, green: attached to an object, blue: can't be moved).

When the handle of an object is connected to another object, it's always
on special points called connection points, displayed as crosses.

Implementation:

- each object has an array of pointer to ConnectionPoint.
- each object has an array of pointer to Handle.
- each Handle has a pointer to 1 ConnectionPoint (NULL if the handle if
the Handle is not connected).
- each ConnectionPoint has a list of all objects connected to it.

The Object type does not manage the allocation/deallocation of handles and
connection points. When saving a diagram the pointer from the handle to
the connectionpoint is saved as the index of the connectionpoint. So make
sure the order of the connectionpoints is the same when loading the saved
object.

## Static analysis

Static analysis is great!
It is also [recommended](https://github.com/coreinfrastructure/best-practices-badge/blob/master/doc/criteria.md#static_analysis) by the Core Infrastructure Initiative.
We currently support two static analysis tools: clang [scan-build](http://clang-analyzer.llvm.org/scan-build) [coverity](https://scan.coverity.com/).

### scan-build

See [Meson Documentation](https://mesonbuild.com/Using-multiple-build-directories.html#specialized-uses)
for details on how to use it.  The basic idea is that `scan-build` replaces
environment variables like GCC and GXX with custom invocations that perform
the analysis.

### Coverity

Dia GNOME has a coverity web-page here: https://scan.coverity.com/projects/dia-gnome/

#### Known bug: `error: identifier "_Float128" is undefined`
See https://software.intel.com/en-us/forums/intel-c-compiler/topic/742701 for more details.

Simply use clang for compilation: https://mesonbuild.com/Running-Meson.html
