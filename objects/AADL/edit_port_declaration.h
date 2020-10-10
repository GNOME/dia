/* AADL plugin for DIA
*
* Copyright (C) 2005 Laboratoire d'Informatique de Paris 6
* Author: Pierre Duquesne
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

#pragma once

G_BEGIN_DECLS

#define DIA_AADL_TYPE_EDIT_PORT_DECLARATION_OBJECT_CHANGE dia_aadl_edit_port_declaration_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaAADLEditPortDeclarationObjectChange,
                      dia_aadl_edit_port_declaration_object_change,
                      DIA_AADL, EDIT_PORT_DECLARATION_OBJECT_CHANGE,
                      DiaObjectChange)


DiaObjectChange *edit_port_declaration_callback (DiaObject *obj,
                                                 Point     *clicked,
                                                 gpointer   data);

G_END_DECLS
