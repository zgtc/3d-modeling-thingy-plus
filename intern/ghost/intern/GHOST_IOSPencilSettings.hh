/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * Platform-neutral Apple Pencil and gesture settings for the iPad touch layer.
 *
 * Squeeze and double tap follow the system Pencil setting unless the user picks an action for
 * this app. The first double tap asks what it should do, so nothing changes behind the user's
 * back. Values are stored as integers, so keep the numbers stable.
 */

#pragma once

#include "GHOST_IOSInteraction.hh"

#include <cstdint>
#include <cstring>

/** What the user picked for double tap. */
enum class GHOST_IOSDoubleTapChoice : uint8_t {
  /** Not chosen yet: the next double tap asks. */
  Unset = 0,
  RightClick = 1,
  PreviousTool = 2,
  Palette = 3,
  Eraser = 4,
  System = 5,
};
constexpr int GHOST_IOS_DOUBLE_TAP_CHOICE_LAST = 5;

/** What the user picked for squeeze. */
enum class GHOST_IOSSqueezeChoice : uint8_t {
  System = 0,
  Palette = 1,
  Off = 2,
};
constexpr int GHOST_IOS_SQUEEZE_CHOICE_LAST = 2;

/** Mirrors `UIPencilPreferredAction`, the user's choice in the system Pencil settings. */
enum class GHOST_IOSSystemPencilAction : uint8_t {
  Ignore,
  SwitchEraser,
  SwitchPrevious,
  ShowColorPalette,
  ShowInkAttributes,
  ShowContextualPalette,
  RunSystemShortcut,
};

/** What a double tap or squeeze does now. */
enum class GHOST_IOSPencilAction : uint8_t {
  None,
  /** Ask the user to pick a double tap action. Nothing else happens on that tap. */
  AskUser,
  RightClick,
  PreviousTool,
  Palette,
  /** Invert the brush in sculpt and paint modes, like holding Ctrl. */
  Eraser,
};

/** What a two-, three- or four-finger tap does. */
enum class GHOST_IOSGestureAction : uint8_t {
  None = 0,
  Undo = 1,
  Redo = 2,
  Search = 3,
  /** Toggle Focus mode: the editor under the pointer fills the window, panels hidden. */
  Focus = 4,
};
constexpr int GHOST_IOS_GESTURE_ACTION_LAST = 4;

/** Two-, three- and four-finger taps, in that order. */
constexpr int GHOST_IOS_TAP_GESTURE_COUNT = 3;

struct GHOST_IOSTouchSettings {
  GHOST_IOSDoubleTapChoice double_tap = GHOST_IOSDoubleTapChoice::Unset;
  GHOST_IOSSqueezeChoice squeeze = GHOST_IOSSqueezeChoice::System;
  /** Like iPadOS "Only Draw with Apple Pencil": fingers navigate the viewport, never paint. */
  bool pencil_only_drawing = false;
  /** More room between Blender's buttons, rows and panels for fingers. */
  bool touch_spacing = false;
  GHOST_IOSGestureAction tap_gestures[GHOST_IOS_TAP_GESTURE_COUNT] = {
      GHOST_IOSGestureAction::Undo,
      GHOST_IOSGestureAction::Redo,
      GHOST_IOSGestureAction::Search,
  };
};

inline GHOST_IOSDoubleTapChoice GHOST_IOSDoubleTapChoice_fromStored(const long value)
{
  if (value < 0 || value > GHOST_IOS_DOUBLE_TAP_CHOICE_LAST) {
    return GHOST_IOSDoubleTapChoice::Unset;
  }
  return GHOST_IOSDoubleTapChoice(value);
}

inline GHOST_IOSSqueezeChoice GHOST_IOSSqueezeChoice_fromStored(const long value)
{
  if (value < 0 || value > GHOST_IOS_SQUEEZE_CHOICE_LAST) {
    return GHOST_IOSSqueezeChoice::System;
  }
  return GHOST_IOSSqueezeChoice(value);
}

inline GHOST_IOSGestureAction GHOST_IOSGestureAction_fromStored(const long value,
                                                                const GHOST_IOSGestureAction def)
{
  if (value < 0 || value > GHOST_IOS_GESTURE_ACTION_LAST) {
    return def;
  }
  return GHOST_IOSGestureAction(value);
}

/** The app's action for a system Pencil preference. */
inline GHOST_IOSPencilAction GHOST_IOSPencilAction_fromSystem(
    const GHOST_IOSSystemPencilAction system)
{
  switch (system) {
    case GHOST_IOSSystemPencilAction::Ignore:
      return GHOST_IOSPencilAction::None;
    case GHOST_IOSSystemPencilAction::SwitchEraser:
      return GHOST_IOSPencilAction::Eraser;
    case GHOST_IOSSystemPencilAction::SwitchPrevious:
      return GHOST_IOSPencilAction::PreviousTool;
    case GHOST_IOSSystemPencilAction::ShowColorPalette:
    case GHOST_IOSSystemPencilAction::ShowInkAttributes:
    case GHOST_IOSSystemPencilAction::ShowContextualPalette:
      return GHOST_IOSPencilAction::Palette;
    case GHOST_IOSSystemPencilAction::RunSystemShortcut:
      /* The system runs the shortcut itself. */
      return GHOST_IOSPencilAction::None;
  }
  return GHOST_IOSPencilAction::None;
}

inline GHOST_IOSPencilAction GHOST_IOSPencil_doubleTapAction(
    const GHOST_IOSDoubleTapChoice choice, const GHOST_IOSSystemPencilAction system)
{
  switch (choice) {
    case GHOST_IOSDoubleTapChoice::Unset:
      return GHOST_IOSPencilAction::AskUser;
    case GHOST_IOSDoubleTapChoice::RightClick:
      return GHOST_IOSPencilAction::RightClick;
    case GHOST_IOSDoubleTapChoice::PreviousTool:
      return GHOST_IOSPencilAction::PreviousTool;
    case GHOST_IOSDoubleTapChoice::Palette:
      return GHOST_IOSPencilAction::Palette;
    case GHOST_IOSDoubleTapChoice::Eraser:
      return GHOST_IOSPencilAction::Eraser;
    case GHOST_IOSDoubleTapChoice::System:
      return GHOST_IOSPencilAction_fromSystem(system);
  }
  return GHOST_IOSPencilAction::AskUser;
}

inline GHOST_IOSPencilAction GHOST_IOSPencil_squeezeAction(
    const GHOST_IOSSqueezeChoice choice, const GHOST_IOSSystemPencilAction system)
{
  /* A system shortcut always wins: iPadOS runs it whatever the app does. */
  if (system == GHOST_IOSSystemPencilAction::RunSystemShortcut) {
    return GHOST_IOSPencilAction::None;
  }
  switch (choice) {
    case GHOST_IOSSqueezeChoice::Off:
      return GHOST_IOSPencilAction::None;
    case GHOST_IOSSqueezeChoice::Palette:
      return GHOST_IOSPencilAction::Palette;
    case GHOST_IOSSqueezeChoice::System:
      return GHOST_IOSPencilAction_fromSystem(system);
  }
  return GHOST_IOSPencilAction::None;
}

/** Whether a finger tap or drag may click at the pointer. */
inline bool GHOST_IOSFinger_mayClick(const GHOST_IOSTouchSettings &settings,
                                     const GHOST_IOSInteractionContext &context)
{
  if (!settings.pencil_only_drawing || !context.over_main_region) {
    /* Headers, toolbars and other editors keep working with a finger. */
    return true;
  }
  return context.editor != GHOST_IOSEditor::View3D && context.editor != GHOST_IOSEditor::Image;
}

/** Remembers the tool before the current one, per editor and mode. */
class GHOST_IOSToolHistory {
 public:
  void update(const GHOST_IOSInteractionContext &context)
  {
    if (context.tool[0] == '\0') {
      return;
    }
    if (context.editor != editor_ || context.mode != mode_) {
      /* Tools of another mode cannot be set here. */
      editor_ = context.editor;
      mode_ = context.mode;
      copy(current_, context.tool);
      previous_[0] = '\0';
      return;
    }
    if (std::strcmp(current_, context.tool) != 0) {
      copy(previous_, current_);
      copy(current_, context.tool);
    }
  }

  /** The tool to switch back to, or an empty string. */
  const char *previous() const
  {
    return previous_;
  }

 private:
  static void copy(char dst[GHOST_IOS_IDNAME_MAX], const char *src)
  {
    std::strncpy(dst, src, GHOST_IOS_IDNAME_MAX - 1);
    dst[GHOST_IOS_IDNAME_MAX - 1] = '\0';
  }

  GHOST_IOSEditor editor_ = GHOST_IOSEditor::Other;
  GHOST_IOSMode mode_ = GHOST_IOSMode::Other;
  char current_[GHOST_IOS_IDNAME_MAX] = "";
  char previous_[GHOST_IOS_IDNAME_MAX] = "";
};
