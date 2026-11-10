/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * The iPad's native header (menu button, mode switch, workspace tabs) and tool dock.
 */

#pragma once

#include "GHOST_IOSChrome.hh"

#ifdef __OBJC__

#  import <Foundation/Foundation.h>

@class UIView;

@protocol GHOSTIOSHeaderDockDelegate <NSObject>
/** An operator call, see #GHOST_IOSCommand_format. */
- (void)headerDockQueueCommand:(const char *)command;
@end

/**
 * Add the header above and the dock below \a canvas, in its superview, which must implement
 * #GHOSTIOSCanvasInsetting so the canvas makes room for them. The delegate is not retained.
 * Returns nil when the superview cannot make room.
 */
NSObject *GHOST_IOSHeaderDock_create(UIView *canvas, id<GHOSTIOSHeaderDockDelegate> delegate);
void GHOST_IOSHeaderDock_destroy(NSObject *header_dock);
/** Follow the editor, mode, active tool and workspaces under the pointer. */
void GHOST_IOSHeaderDock_setContext(NSObject *header_dock,
                                    const GHOST_IOSInteractionContext &context);
bool GHOST_IOSHeaderDock_containsView(NSObject *header_dock, UIView *view);

#endif
