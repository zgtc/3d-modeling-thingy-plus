/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * Native Apple Pencil palette, double tap prompt and touch settings for iPad.
 */

#pragma once

#include "GHOST_IOSPencilSettings.hh"
#include "GHOST_IOSTouchKeys.hh"

#ifdef __OBJC__

#  import <CoreGraphics/CoreGraphics.h>
#  import <Foundation/Foundation.h>

@class UIMenu;
@class UIView;
@class UIViewController;

@protocol GHOSTIOSPencilPaletteDelegate <NSObject>
/** \a index is into the slots passed to #GHOST_IOSPencilPalette_show. The palette has closed. */
- (void)pencilPaletteChoseSlot:(int)index;
/** A slot was held to replace it. The palette has closed. */
- (void)pencilPaletteEditSlot:(int)index;
@optional
/** The middle of the ring was tapped: show the other page. The palette stays open. */
- (void)pencilPaletteSwitchPage;
@end

/** Add a hidden palette to \a input_view. The delegate is not retained. */
NSObject *GHOST_IOSPencilPalette_create(UIView *input_view,
                                        id<GHOSTIOSPencilPaletteDelegate> delegate);
void GHOST_IOSPencilPalette_destroy(NSObject *palette);
/**
 * Open the radial palette around \a point, in \a input_view points, moved inwards as needed so
 * it stays on screen. \a slots must stay valid while it is open.
 */
void GHOST_IOSPencilPalette_show(NSObject *palette,
                                 CGPoint point,
                                 const GHOST_IOSPaletteSlot *slots,
                                 int count);
void GHOST_IOSPencilPalette_hide(NSObject *palette);
/** SF Symbol in the middle of the ring, which switches pages when tapped. */
void GHOST_IOSPencilPalette_setCenterSymbol(NSObject *palette, const char *symbol);
bool GHOST_IOSPencilPalette_isVisible(NSObject *palette);
bool GHOST_IOSPencilPalette_containsView(NSObject *palette, UIView *view);

/** The user's touch settings, or the defaults. */
GHOST_IOSTouchSettings GHOST_IOSTouchSettings_load();
void GHOST_IOSTouchSettings_save(const GHOST_IOSTouchSettings &settings);

/** The system Pencil settings, from `UIPencilInteraction`. */
GHOST_IOSSystemPencilAction GHOST_IOSPencil_systemTapAction();
GHOST_IOSSystemPencilAction GHOST_IOSPencil_systemSqueezeAction();

/**
 * Ask what double tap should do, in a sheet pointing at \a point in \a source_view.
 * \a chosen runs with the answer; a dismissed sheet leaves the choice unset.
 */
void GHOST_IOSPencil_askDoubleTap(UIViewController *presenter,
                                  UIView *source_view,
                                  CGPoint point,
                                  void (^chosen)(GHOST_IOSDoubleTapChoice choice));

/** Settings menu entries that need the window, such as presenting a folder picker. */
enum class GHOST_IOSSettingsMenuCommand : uint8_t {
  ChooseSyncFolder,
  StopSync,
  ResetCustomizations,
};

/**
 * The settings menu for the command bar. \a changed runs with the new settings, already saved;
 * \a command runs for entries the window handles.
 */
UIMenu *GHOST_IOSTouchSettings_menu(const GHOST_IOSTouchSettings &settings,
                                    void (^changed)(const GHOST_IOSTouchSettings &settings),
                                    void (^command)(GHOST_IOSSettingsMenuCommand command));

#endif
