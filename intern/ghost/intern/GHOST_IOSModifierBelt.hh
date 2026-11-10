/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * The iPad's left rail and quick bar.
 *
 * The rail runs down the left of Blender's canvas with the tools of the current 3D Viewport mode,
 * filling the height. The quick bar runs along the bottom: Shift, Ctrl, Alt and Cmd, the shortcut
 * keys for the editor and mode under the pointer (or of the running tool), and the settings
 * button. A divider above the quick bar sets its height. The
 * transform key pad shows above the quick bar while a modal operator takes typed values.
 */

#pragma once

#include "GHOST_IOSChrome.hh"
#include "GHOST_IOSModifierLatch.hh"
#include "GHOST_IOSTouchKeys.hh"

/** Width of the rail's panels, and the margin around them. */
constexpr double GHOST_IOS_RAIL_WIDTH = 66.0;
constexpr double GHOST_IOS_RAIL_MARGIN = 12.0;
/** Quick bar height limits and default, in points; the divider above it sets the height. */
constexpr double GHOST_IOS_QUICK_BAR_MIN_HEIGHT = 52.0;
constexpr double GHOST_IOS_QUICK_BAR_MAX_HEIGHT = 120.0;
constexpr double GHOST_IOS_QUICK_BAR_DEFAULT_HEIGHT = 60.0;
/** Height of the divider's touch strip, and the gap below the quick bar. */
constexpr double GHOST_IOS_QUICK_BAR_DIVIDER = 16.0;
constexpr double GHOST_IOS_QUICK_BAR_GAP = 8.0;

inline double GHOST_IOSQuickBar_clampHeight(const double height)
{
  return height < GHOST_IOS_QUICK_BAR_MIN_HEIGHT ? GHOST_IOS_QUICK_BAR_MIN_HEIGHT :
         height > GHOST_IOS_QUICK_BAR_MAX_HEIGHT ? GHOST_IOS_QUICK_BAR_MAX_HEIGHT :
                                                   height;
}

/**
 * Room Blender's canvas leaves on the left for the rail and at the bottom for the quick bar and
 * its divider. While a hardware keyboard is connected the quick bar hides and gives its room back;
 * the rail's tools stay.
 */
inline void GHOST_IOSRail_canvasInsets(const bool quick_bar_visible,
                                       const double quick_bar_height,
                                       double *r_leading,
                                       double *r_bottom)
{
  *r_leading = GHOST_IOS_RAIL_WIDTH + GHOST_IOS_RAIL_MARGIN * 2.0;
  *r_bottom = quick_bar_visible ? GHOST_IOSQuickBar_clampHeight(quick_bar_height) +
                                      GHOST_IOS_QUICK_BAR_DIVIDER + GHOST_IOS_QUICK_BAR_GAP :
                                  0.0;
}

/**
 * The timeline's height after the divider above the quick bar was dragged \a drag_up points up
 * from where the timeline was \a timeline_start pixels high. The divider moves with the
 * timeline's bottom edge, so the timeline gives or takes the whole drag; it never gets shorter
 * than \a timeline_min, its header row.
 */
inline int GHOST_IOSDivider_timelineHeight(const int timeline_start,
                                           const int timeline_min,
                                           const double drag_up,
                                           const double scale)
{
  const double height = double(timeline_start) - drag_up * scale;
  const int rounded = int(height + (height < 0.0 ? -0.5 : 0.5));
  return rounded < timeline_min ? timeline_min : rounded;
}

/** Buttons the window still handles, from the menu bar and gestures. */
enum class GHOST_IOSBarAction : uint8_t {
  /** Toggle Focus mode for the editor under the pointer. */
  Focus,
  /** Open the Properties editor in a floating window. */
  Properties,
  /** Open the Outliner in a floating window. */
  Outliner,
};

#ifdef __OBJC__

#  import <CoreGraphics/CoreGraphics.h>
#  import <Foundation/Foundation.h>
#  import <UIKit/UIGestureRecognizer.h>

@class UIMenu;
@class UIView;

/** The view that holds Blender's canvas and the native bars around it, and insets the canvas. */
@protocol GHOSTIOSCanvasInsetting <NSObject>
- (void)setCanvasInsetsLeading:(CGFloat)leading trailing:(CGFloat)trailing;
/** Room for the native header, not counting the screen's safe area. */
- (void)setCanvasInsetTop:(CGFloat)top;
/** Room for the quick bar, not counting the screen's safe area. */
- (void)setCanvasInsetBottom:(CGFloat)bottom;
@end

/** Receives touches on the rail and the quick bar. Timestamps are `UITouch.timestamp` seconds. */
@protocol GHOSTIOSModifierBeltDelegate <NSObject>
- (void)modifierBeltKey:(GHOST_IOSModifier)modifier
                   down:(BOOL)down
              timestamp:(NSTimeInterval)timestamp;
- (void)modifierBeltKeyCancelled:(GHOST_IOSModifier)modifier;
/** The quick bar was hidden, for example because a hardware keyboard connected. */
- (void)modifierBeltDidHide;
/** A shortcut or key pad key was tapped. */
- (void)commandBarKey:(const GHOST_IOSBarKey &)key;
/** Shortcut key \a index was held to replace it. */
- (void)commandBarEditKey:(int)index;
/** Tool \a index of those passed to #GHOST_IOSModifierBelt_setTools was tapped. */
- (void)commandBarTool:(int)index;
/** The divider above the quick bar is being dragged, \a points up from where it started. */
- (void)quickBarDivider:(UIGestureRecognizerState)state dragUp:(CGFloat)points;
@end

/**
 * Add the rail and the quick bar beside \a input_view, Blender's canvas. When the canvas'
 * superview implements #GHOSTIOSCanvasInsetting, they go there and the canvas is inset so they
 * cover nothing. The delegate is not retained.
 */
NSObject *GHOST_IOSModifierBelt_create(UIView *input_view,
                                       id<GHOSTIOSModifierBeltDelegate> delegate);
void GHOST_IOSModifierBelt_destroy(NSObject *belt);
bool GHOST_IOSModifierBelt_containsView(NSObject *belt, UIView *view);
/** Show each modifier key's state on the quick bar, indexed by #GHOST_IOSModifier. */
void GHOST_IOSModifierBelt_setModes(NSObject *belt,
                                    const GHOST_IOSModifierMode modes[GHOST_IOS_MODIFIER_COUNT]);
/** Kept for the window's cursor updates; the bars sit beside the canvas, so nothing fades. */
void GHOST_IOSModifierBelt_setCursor(NSObject *belt, CGPoint point, bool visible);
/** Show \a count shortcut keys on the quick bar. The keys are copied. */
void GHOST_IOSModifierBelt_setContextKeys(NSObject *belt, const GHOST_IOSBarKey *keys, int count);
/** Show the tools of the current mode on the rail, \a active highlighted (-1 for none). */
void GHOST_IOSModifierBelt_setTools(NSObject *belt,
                                    const GHOST_IOSToolItem *tools,
                                    int count,
                                    int active);
/** Show the transform key pad above the quick bar, while a modal operator takes typed values. */
void GHOST_IOSModifierBelt_setKeyPadVisible(NSObject *belt, bool visible);
/** The menu behind the quick bar's settings button. */
void GHOST_IOSModifierBelt_setSettingsMenu(NSObject *belt, UIMenu *menu);

#endif
