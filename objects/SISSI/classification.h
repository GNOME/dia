/*
*
* C++ Interface: classification
*
* Description: 
*
*
* Author: Christophe Mace <mace@athena>, (C) 2004
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#ifndef CLASSIFICATION_H
#define CLASSIFICATION_H


typedef enum {
  NO_PROTECTION,
  RESTRICTED_DIFFUSION,
  SPECIAL_COUNTRY_CONFIDENTIAL,
  CONFIDENTIAL,
  NATO_CONFIDENTIAL,
  PERSONAL_CONFIDENTIAL,
  MEDICAL_CONFIDENTIAL,
  INDUSTRIE_CONFIDENTIAL,
  DEFENSE_CONFIDENTIAL,
  SECRET,
  DEFENSE_SECRET,
  SECRET_SPECIAL_COUNTRY,
  NATO_SECRET,
  VERY_SECRET,
  NATO_VERY_SECRET,
  TSissiConfidentiality
};

static PropEnumData propriete_confidentialite_data[] = {
  { N_("No Protection"), NO_PROTECTION },
  { N_("Restricted Diffusion") , RESTRICTED_DIFFUSION },
  { N_("Special Country Confidential") , SPECIAL_COUNTRY_CONFIDENTIAL },
  { N_("Confidential") , CONFIDENTIAL },
  { N_("NATO Confidential") , NATO_CONFIDENTIAL },
  { N_("Personal Confidential") , PERSONAL_CONFIDENTIAL },
  { N_("Medical Confidential") , MEDICAL_CONFIDENTIAL },
  { N_("Industrie Confidential") , INDUSTRIE_CONFIDENTIAL },
  { N_("Defense Confidential") , DEFENSE_CONFIDENTIAL },
  { N_("Secret") , SECRET },
  { N_("Defense Secret") , DEFENSE_SECRET },
  { N_("Secret special country") , SECRET_SPECIAL_COUNTRY },
  { N_("NATO Secret") , NATO_SECRET },
  { N_("Very Secret") , VERY_SECRET },
  { N_("NATO Very Secret") , NATO_VERY_SECRET },
  { NULL, 0}
} ;

#endif
