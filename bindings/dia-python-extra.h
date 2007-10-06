/*
 * some helper function not to pollute the multilingual dia.swig file 
 *
 * Copyright 2007, Hans Breuer, GPL, see COPYING
 */
#ifndef DIA__PYTHON_EXTRA_H
#define DIA__PYTHON_EXTRA_H

#include <vector>
#include <string>

#include "dia-properties.h"

bool DiaPythonExtra_ToVector (PyObject*, std::vector<double>& vec);
bool DiaPythonExtra_ToVector (PyObject*, std::vector<dia::IProperty*>& vec);

#endif /* DIA__PYTHON_EXTRA_H */

