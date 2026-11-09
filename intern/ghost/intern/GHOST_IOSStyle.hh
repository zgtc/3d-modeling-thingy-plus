/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * Shared look for the native UIKit controls drawn over Blender's interface.
 *
 * On iOS 26 and later the floating controls use Liquid Glass, the system material for
 * controls that float above content. Earlier systems keep a dark blur or a translucent fill.
 * Every control gets its shape and colors from here so they stay consistent.
 */

#pragma once

#ifdef __OBJC__

@class UIButton;
@class UIColor;
@class UIImage;
@class UIView;
@class UIVisualEffectView;
@class NSString;

/** Side of the square hit target used by every floating control, in points. */
constexpr double GHOST_IOS_CONTROL_SIZE = 44.0;

/** True when the Liquid Glass APIs are available at run time. */
bool GHOST_IOSStyle_hasGlass();

/** Accent color for active and selected states. Follows the system accent color. */
UIColor *GHOST_IOSStyle_accentColor();

/** An SF Symbol at the standard weight for floating controls. */
UIImage *GHOST_IOSStyle_symbol(NSString *name, double point_size);

/**
 * A rounded material background. Glass on iOS 26, a thin dark blur before. The caller owns
 * the returned view (+1 reference).
 */
UIVisualEffectView *GHOST_IOSStyle_createMaterialView(double corner_radius);

/** Make `button` a circular floating control showing `symbol`. */
void GHOST_IOSStyle_applyFloatingButton(UIButton *button, NSString *symbol);

/** Give a light tap of haptic feedback for a discrete state change, where supported. */
void GHOST_IOSStyle_selectionFeedback();

#endif
