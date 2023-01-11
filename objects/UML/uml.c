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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <string.h>
#include <stdio.h>

#include "object.h"
#include "uml.h"
#include "plug-ins.h"
#include "properties.h"

extern DiaObjectType umlclass_type;
extern DiaObjectType umlclass_template_type;
extern DiaObjectType note_type;
extern DiaObjectType dependency_type;
extern DiaObjectType realizes_type;
extern DiaObjectType generalization_type;
extern DiaObjectType association_type;
extern DiaObjectType implements_type;
extern DiaObjectType constraint_type;
extern DiaObjectType smallpackage_type;
extern DiaObjectType largepackage_type;
extern DiaObjectType actor_type;
extern DiaObjectType usecase_type;
extern DiaObjectType lifeline_type;
extern DiaObjectType objet_type;
extern DiaObjectType umlobject_type;
extern DiaObjectType message_type;
extern DiaObjectType component_type;
extern DiaObjectType classicon_type;
extern DiaObjectType state_type;
extern DiaObjectType activity_type;
extern DiaObjectType node_type;
extern DiaObjectType branch_type;
extern DiaObjectType fork_type;
extern DiaObjectType state_term_type;
extern DiaObjectType compfeat_type;
extern DiaObjectType uml_transition_type;

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "UML",
			    _("Unified Modelling Language diagram objects UML 1.3"),
			    NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  object_register_type(&umlclass_type);
  object_register_type(&note_type);
  object_register_type(&dependency_type);
  object_register_type(&realizes_type);
  object_register_type(&generalization_type);
  object_register_type(&association_type);
  object_register_type(&implements_type);
  object_register_type(&constraint_type);
  object_register_type(&smallpackage_type);
  object_register_type(&largepackage_type);
  object_register_type(&actor_type);
  object_register_type(&usecase_type);
  object_register_type(&lifeline_type);
  object_register_type(&objet_type);
  object_register_type(&umlobject_type);
  object_register_type(&message_type);
  object_register_type(&component_type);
  object_register_type(&classicon_type);
  object_register_type(&state_type);
  object_register_type(&state_term_type);
  object_register_type(&activity_type);
  object_register_type(&node_type);
  object_register_type(&branch_type);
  object_register_type(&fork_type);
  object_register_type(&compfeat_type);
  object_register_type(&uml_transition_type);

  return DIA_PLUGIN_INIT_OK;
}


PropEnumData _uml_visibilities[] = {
  { N_("Public"), DIA_UML_PUBLIC },
  { N_("Private"), DIA_UML_PRIVATE },
  { N_("Protected"), DIA_UML_PROTECTED },
  { N_("Implementation"), DIA_UML_IMPLEMENTATION },
  { NULL, 0 }
};

PropEnumData _uml_inheritances[] = {
  { N_("Abstract"), DIA_UML_ABSTRACT },
  { N_("Polymorphic (virtual)"), DIA_UML_POLYMORPHIC },
  { N_("Leaf (final)"), DIA_UML_LEAF },
  { NULL, 0 }
};

