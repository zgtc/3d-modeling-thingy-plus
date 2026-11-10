# SPDX-FileCopyrightText: 2026 Blender Authors
#
# SPDX-License-Identifier: GPL-2.0-or-later

list(APPEND SRC
  GHOST_IOSInteraction.hh
  intern/GHOST_ContextIOS.hh
  intern/GHOST_ContextIOS.mm
  intern/GHOST_EventTouch.hh
  intern/GHOST_IOSFileAccess.hh
  intern/GHOST_IOSFileAccess.mm
  intern/GHOST_IOSInputTuning.hh
  intern/GHOST_IOSModifierBelt.hh
  intern/GHOST_IOSModifierBelt.mm
  intern/GHOST_IOSChrome.hh
  intern/GHOST_IOSHeaderDock.hh
  intern/GHOST_IOSHeaderDock.mm
  intern/GHOST_IOSMenuBar.hh
  intern/GHOST_IOSMenuBar.mm
  intern/GHOST_IOSMenuItems.hh
  intern/GHOST_IOSModifierLatch.hh
  intern/GHOST_IOSPencilContact.hh
  intern/GHOST_IOSPencilPalette.hh
  intern/GHOST_IOSPencilPalette.mm
  intern/GHOST_IOSPencilSettings.hh
  intern/GHOST_IOSResizeHandles.hh
  intern/GHOST_IOSResizeHandles.mm
  intern/GHOST_IOSSettingsFile.hh
  intern/GHOST_IOSSettingsSync.hh
  intern/GHOST_IOSSettingsSync.mm
  intern/GHOST_IOSSettingsLocation.hh
  intern/GHOST_IOSSettingsLocation.mm
  intern/GHOST_IOSStyle.hh
  intern/GHOST_IOSStyle.mm
  intern/GHOST_IOSTouchKeys.hh
  intern/GHOST_IOSVirtualPointer.hh
  intern/GHOST_IOSVirtualPointer.mm
  intern/GHOST_IOSVirtualPointerState.hh
  intern/GHOST_SystemIOS.hh
  intern/GHOST_SystemIOS.mm
  intern/GHOST_SystemPathsCocoa.hh
  intern/GHOST_SystemPathsIOS.mm
  intern/GHOST_WindowIOS.hh
  intern/GHOST_WindowIOS.mm
)

add_definitions(
  -DGHOST_PLATFORM_SYSTEM_HEADER=\"GHOST_SystemIOS.hh\"
  -DGHOST_PLATFORM_SYSTEM_CLASS=GHOST_SystemIOS
  -DGHOST_PLATFORM_SYSTEM_NAME=\"IOS\"
)
