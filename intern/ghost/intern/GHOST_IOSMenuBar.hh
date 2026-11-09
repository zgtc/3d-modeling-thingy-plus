/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * Blender's menus in the iPadOS menu bar, shown when the user swipes down from the top of the
 * screen or holds Command with a keyboard attached.
 */

#pragma once

#include "GHOST_IOSMenuItems.hh"

#ifdef __OBJC__

#  import <Foundation/Foundation.h>

@protocol UIMenuBuilder;

/** Called from the app delegate's `buildMenuWithBuilder:` for the main menu system. */
void GHOST_IOSMenuBar_build(id<UIMenuBuilder> builder);
/** Rebuild the menu bar when the editor menus for \a context differ from the current ones. */
void GHOST_IOSMenuBar_setContext(const GHOST_IOSInteractionContext &context);

/*
 * Every menu item sends `blenderMenuCommand:` through the responder chain. Its `propertyList`
 * is the operator call; the window that receives it queues the call for the window manager.
 */

#endif
