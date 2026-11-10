/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * Storage of the touch layer's settings file, and its sync through a folder the user picks.
 *
 * The file lives at `<Documents>/Settings/touch-settings.json`, next to Blender's own settings.
 * To share it between iPads, the user picks a folder once (for example in iCloud Drive) from the
 * command bar settings; a copy is kept there through a security-scoped bookmark. This needs no
 * iCloud entitlement. On launch and whenever the app becomes active, the most recently saved copy
 * wins. Without a picked folder, or when it cannot be reached, the local file is used silently.
 */

#pragma once

#include "GHOST_IOSSettingsFile.hh"

#ifdef __OBJC__

#  import <Foundation/Foundation.h>

@class UIViewController;

/** Posted on the main queue when the shared settings changed from outside a window. */
extern NSString *const GHOST_IOSTouchSettingsDidChangeNotification;

/** The settings every window shares, loaded on first use. */
GHOST_IOSSettingsFile &GHOST_IOSSettings_shared();
/** Stamp the save time and write the local file and the synced copy. */
void GHOST_IOSSettings_save();

/** The picked sync folder's name, or nil when settings are not synced. */
NSString *GHOST_IOSSettings_syncFolderName();
/** Ask for a sync folder. Settings from it are used when they are newer. */
void GHOST_IOSSettings_chooseSyncFolder(UIViewController *presenter);
/** Forget the sync folder; the local file stays. */
void GHOST_IOSSettings_stopSync();

#endif
