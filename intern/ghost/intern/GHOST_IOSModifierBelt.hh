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

/** Buttons below the shortcut keys. */
enum class GHOST_IOSBarAction : uint8_t {
  /** Toggle Focus mode for the editor under the pointer. */
  Focus,
  /** Open the Properties editor in a floating window. */
  Properties,
  /** Open the Outliner in a floating window. */
  Outliner,
};

#ifdef __OBJC__

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

@class UIMenu;
@class UIView;

/** Receives touches on the belt's keys. Timestamps are `UITouch.timestamp` seconds. */
@protocol GHOSTIOSModifierBeltDelegate <NSObject>
- (void)modifierBeltKey:(GHOST_IOSModifier)modifier
                   down:(BOOL)down
              timestamp:(NSTimeInterval)timestamp;
- (void)modifierBeltKeyCancelled:(GHOST_IOSModifier)modifier;
/** The belt was hidden, for example because a hardware keyboard connected. */
- (void)modifierBeltDidHide;
/** A shortcut or key pad key was tapped. */
- (void)commandBarKey:(const GHOST_IOSBarKey &)key;
/** Shortcut key \a index was held to replace it. */
- (void)commandBarEditKey:(int)index;
/** One of the Focus, Properties or Outliner buttons was tapped. */
- (void)commandBarAction:(GHOST_IOSBarAction)action;
@end

/**
 * Add the bar to \a input_view. It docks vertically to the left or right edge, in the top or
 * bottom half the user last chose, and starts collapsed to a small handle. The delegate is not
 * retained.
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
