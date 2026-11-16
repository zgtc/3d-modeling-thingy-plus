/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * Native on-screen command bar for touch and Apple Pencil: Shift, Ctrl, Alt and Cmd, shortcut
 * keys for the editor under the pointer, and a key pad for typed transform values.
 */

#pragma once

#include "GHOST_IOSModifierLatch.hh"
#include "GHOST_IOSTouchKeys.hh"

/**
 * Room Blender's canvas leaves for the bar, which stays open beside it on one edge. The bar is
 * hidden while a hardware keyboard is connected, and the canvas then gets the full width back.
 */
inline void GHOST_IOSCommandBar_canvasInsets(const bool visible,
                                             const bool docked_left,
                                             const double bar_width,
                                             const double margin,
                                             double *r_leading,
                                             double *r_trailing)
{
  const double inset = visible ? bar_width + margin * 2.0 : 0.0;
  *r_leading = docked_left ? inset : 0.0;
  *r_trailing = docked_left ? 0.0 : inset;
}

#ifdef __OBJC__

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

@class UIMenu;
@class UIView;

/** The view that holds Blender's canvas and the bar side by side, and insets the canvas. */
@protocol GHOSTIOSCanvasInsetting <NSObject>
- (void)setCanvasInsetsLeading:(CGFloat)leading trailing:(CGFloat)trailing;
@end

/** Receives touches on the belt's keys. Timestamps are `UITouch.timestamp` seconds. */
@protocol GHOSTIOSModifierBeltDelegate <NSObject>
- (void)modifierBeltKey:(GHOST_IOSModifier)modifier
                   down:(BOOL)down
              timestamp:(NSTimeInterval)timestamp;
- (void)modifierBeltKeyCancelled:(GHOST_IOSModifier)modifier;
/** The belt was hidden, for example because a hardware keyboard connected. */
- (void)modifierBeltDidHide;
/** A shortcut or key pad key was tapped. */
- (void)commandBarKey:(GHOST_IOSKeyStroke)stroke;
@end

/**
 * Add the bar beside \a input_view, Blender's canvas. When the canvas' superview implements
 * #GHOSTIOSCanvasInsetting, the bar goes there and the canvas is inset so the bar covers nothing.
 * The bar is always open, docked to the left or right edge in the half the user last chose. The
 * delegate is not retained.
 */
NSObject *GHOST_IOSModifierBelt_create(UIView *input_view,
                                       id<GHOSTIOSModifierBeltDelegate> delegate);
void GHOST_IOSModifierBelt_destroy(NSObject *belt);
bool GHOST_IOSModifierBelt_containsView(NSObject *belt, UIView *view);
/** Show each key's state, indexed by #GHOST_IOSModifier. */
void GHOST_IOSModifierBelt_setModes(NSObject *belt,
                                    const GHOST_IOSModifierMode modes[GHOST_IOS_MODIFIER_COUNT]);
/**
 * Fade the belt while the app-drawn finger cursor is over it, so the target under the belt stays
 * visible. \a point is in \a input_view points.
 */
void GHOST_IOSModifierBelt_setCursor(NSObject *belt, CGPoint point, bool visible);
/** Show \a count shortcut keys below the modifiers. The keys are copied. */
void GHOST_IOSModifierBelt_setContextKeys(NSObject *belt, const GHOST_IOSBarKey *keys, int count);
/** Show the transform key pad next to the bar, while a modal operator takes typed values. */
void GHOST_IOSModifierBelt_setKeyPadVisible(NSObject *belt, bool visible);
/** The menu behind the bar's settings button. */
void GHOST_IOSModifierBelt_setSettingsMenu(NSObject *belt, UIMenu *menu);

#endif
