/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef EML_H
#define EML_H

#include <glib.h>
#include "connectionpoint.h"

typedef struct _EMLFunction EMLFunction;
typedef struct _EMLParameter EMLParameter;
typedef struct _EMLInterface EMLInterface;
typedef struct _EMLConnected EMLConnected;

typedef enum _EMLParameterType {
  EML_OTHER,
  EML_RELATION
} EMLParameterType;

struct _EMLConnected {
  ConnectionPoint *left_connection;
  ConnectionPoint *right_connection;
};

struct _EMLParameter {
  ConnectionPoint *left_connection;
  ConnectionPoint *right_connection;
  gchar *name;
  EMLParameterType type;
  GList *relmembers;
};

struct _EMLFunction {
  ConnectionPoint *left_connection;
  ConnectionPoint *right_connection;
  gchar* module;
  gchar *name;
  GList *parameters; /* List of EMLParameter */
};

struct _EMLInterface {
  gchar *name;
  GList *functions;
  GList *messages;
};


extern gchar * eml_get_parameter_string(EMLParameter *);
extern gchar * eml_get_ifmessage_string(EMLParameter *);
extern gchar * eml_get_function_string(EMLFunction *);
extern gchar * eml_get_iffunction_string(EMLFunction *);
extern EMLParameter * eml_parameter_copy(EMLParameter *);
extern EMLParameter * eml_ifmessage_copy(EMLParameter *);
extern EMLFunction * eml_function_copy(EMLFunction *);
extern EMLFunction * eml_iffunction_copy(EMLFunction *);
extern void eml_parameter_destroy(EMLParameter *);
extern void eml_ifmessage_destroy(EMLParameter *);
extern void eml_function_destroy(EMLFunction *);
extern void eml_iffunction_destroy(EMLFunction *);
extern EMLInterface * eml_interface_new(void);
extern EMLParameter * eml_parameter_new(void);
extern EMLParameter * eml_ifmessage_new(void);
extern EMLFunction * eml_function_new(void);
extern void eml_parameter_write(AttributeNode , EMLParameter *);
extern void eml_ifmessage_write(AttributeNode , EMLParameter *);
extern void eml_function_write(AttributeNode , EMLFunction *);
extern void eml_interface_write(AttributeNode , EMLInterface *);
extern void eml_iffunction_write(AttributeNode , EMLFunction *);
extern EMLParameter * eml_parameter_read(DataNode );
extern EMLParameter * eml_ifmessage_read(DataNode );
extern EMLFunction * eml_function_read(DataNode );
extern EMLInterface* eml_interface_read(DataNode );
extern EMLFunction * eml_iffunction_read(DataNode );
extern void emlconnected_new(EMLConnected *);
extern void emlconnected_destroy(EMLConnected *);
extern void function_connections_new(EMLFunction *);
extern void function_connections_destroy(EMLFunction *);
extern void interface_connections_new( EMLInterface *);
extern EMLInterface* emlprocess_interface_copy( EMLInterface *);
extern void emlprocess_interface_destroy( EMLInterface *);
extern void eml_interface_destroy( EMLInterface *);
extern EMLInterface* eml_interface_copy( EMLInterface *);

#endif /* EML_H */
