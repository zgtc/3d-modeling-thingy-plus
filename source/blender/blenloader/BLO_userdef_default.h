/* SPDX-FileCopyrightText: 2001-2002 NaN Holding BV. All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include "DNA_userdef_types.h"

namespace blender {

#ifdef __cplusplus
extern "C" {
#endif

/** Default theme, see: `release/datafiles/userdef/userdef_default_theme.c`. */
extern const bTheme U_theme_default;

#ifdef __cplusplus
}
#endif

/**
 * Adjust a theme initialized from #U_theme_default for the current platform.
 * A no-op unless the platform defines a theme, see:
 * `release/datafiles/userdef/userdef_platform_theme.cc`.
 */
void BLO_userdef_theme_apply_platform(bTheme *btheme);

}  // namespace blender
