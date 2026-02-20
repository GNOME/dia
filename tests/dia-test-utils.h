/*
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: Copyright 2025 Zander Brown
 *
 * Originally from KGX (kgx-test-utils.h)
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

void            dia_expect_property_notify               (gpointer      object,
                                                          const char   *property);
void            dia_expect_no_notify                     (gpointer      object);
void            dia_assert_expected_notifies             (gpointer      object);
void            dia_test_property_notify                 (gpointer      object,
                                                          const char   *property,
                                                          ...);

G_END_DECLS
