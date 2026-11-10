/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * The iPad touch layer's two-way channel with the window manager.
 *
 * The window manager reports what is under the pointer once per main loop pass, so the Pencil
 * palette and the command bar can show keys for the current editor and mode. In the other
 * direction, native controls queue operator calls that the window manager runs with the window
 * under the pointer as context. Plain C++ only, so window manager code can include it.
 */

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>

enum class GHOST_IOSEditor : uint8_t {
  Other,
  View3D,
  Image,
  Node,
};

enum class GHOST_IOSMode : uint8_t {
  Object,
  Edit,
  Sculpt,
  /** Texture, vertex and weight paint. */
  Paint,
  Other,
};

constexpr int GHOST_IOS_IDNAME_MAX = 64;
constexpr int GHOST_IOS_COMMAND_MAX = 256;
constexpr int GHOST_IOS_RESIZE_EDGE_MAX = 8;
constexpr int GHOST_IOS_WORKSPACE_MAX = 16;
constexpr int GHOST_IOS_WORKSPACE_NAME_MAX = 32;

/**
 * The draggable edge of a visible sidebar or toolbar, in GHOST client pixels (origin top left).
 * Dragging horizontally from (#x, any y in #y_min..#y_max) resizes the region in Blender.
 */
struct GHOST_IOSResizeEdge {
  int x = 0;
  int y_min = 0;
  int y_max = 0;
  /** The region lies to the right of the edge, like the N sidebar. */
  bool region_on_right = false;

  bool operator==(const GHOST_IOSResizeEdge &other) const
  {
    return x == other.x && y_min == other.y_min && y_max == other.y_max &&
           region_on_right == other.region_on_right;
  }
};

/** Keyboard shortcuts of a running modal tool, for the quick bar. */
constexpr int GHOST_IOS_MODAL_KEY_MAX = 6;
constexpr uint8_t GHOST_IOS_MODAL_KEY_SHIFT = 1 << 0;
constexpr uint8_t GHOST_IOS_MODAL_KEY_CTRL = 1 << 1;
constexpr uint8_t GHOST_IOS_MODAL_KEY_ALT = 1 << 2;
constexpr uint8_t GHOST_IOS_MODAL_KEY_OS = 1 << 3;

struct GHOST_IOSModalKey {
  char title[24] = "";
  /** A #GHOST_TKey. */
  int key = 0;
  /** #GHOST_IOS_MODAL_KEY_SHIFT and friends. */
  uint8_t modifiers = 0;

  bool operator==(const GHOST_IOSModalKey &other) const
  {
    return key == other.key && modifiers == other.modifiers &&
           std::strcmp(title, other.title) == 0;
  }
};

struct GHOST_IOSInteractionContext {
  GHOST_IOSEditor editor = GHOST_IOSEditor::Other;
  GHOST_IOSMode mode = GHOST_IOSMode::Object;
  /** The pointer is over the editor's main region, not a header, toolbar or sidebar. */
  bool over_main_region = false;
  /** Operator id of the innermost running modal operator, such as `TRANSFORM_OT_translate`. */
  char modal_operator[GHOST_IOS_IDNAME_MAX] = "";
  /** Active tool of the editor under the pointer, such as `builtin.move`. */
  char tool[GHOST_IOS_IDNAME_MAX] = "";
  /** Resizable sidebars and toolbars in the whole window. */
  GHOST_IOSResizeEdge resize_edges[GHOST_IOS_RESIZE_EDGE_MAX];
  int resize_edges_num = 0;
  /** The window's workspaces in tab order, and the active one, or -1. */
  char workspaces[GHOST_IOS_WORKSPACE_MAX][GHOST_IOS_WORKSPACE_NAME_MAX] = {};
  int workspaces_num = 0;
  int active_workspace = -1;
  /**
   * The timeline along the bottom, if any: its right edge and the top and bottom of its header
   * row, in GHOST client pixels. The minimize control sits at that right edge.
   */
  bool has_timeline = false;
  int timeline_right = 0;
  int timeline_header_top = 0;
  int timeline_header_bottom = 0;
  /**
   * Keyboard shortcuts of the running modal tool, from its modal keymap, in the order its status
   * line lists them. Mouse buttons are left out. The quick bar shows these while the tool runs.
   */
  GHOST_IOSModalKey modal_keys[GHOST_IOS_MODAL_KEY_MAX];
  int modal_keys_num = 0;

  bool sameModalKeys(const GHOST_IOSInteractionContext &other) const
  {
    if (modal_keys_num != other.modal_keys_num) {
      return false;
    }
    for (int i = 0; i < modal_keys_num; i++) {
      if (!(modal_keys[i] == other.modal_keys[i])) {
        return false;
      }
    }
    return true;
  }

  bool operator==(const GHOST_IOSInteractionContext &other) const
  {
    return editor == other.editor && mode == other.mode &&
           over_main_region == other.over_main_region &&
           std::strcmp(modal_operator, other.modal_operator) == 0 &&
           std::strcmp(tool, other.tool) == 0 && sameResizeEdges(other) &&
           sameWorkspaces(other) && has_timeline == other.has_timeline &&
           timeline_right == other.timeline_right &&
           timeline_header_top == other.timeline_header_top &&
           timeline_header_bottom == other.timeline_header_bottom && sameModalKeys(other);
  }
  bool sameWorkspaces(const GHOST_IOSInteractionContext &other) const
  {
    if (workspaces_num != other.workspaces_num || active_workspace != other.active_workspace) {
      return false;
    }
    for (int i = 0; i < workspaces_num; i++) {
      if (std::strcmp(workspaces[i], other.workspaces[i]) != 0) {
        return false;
      }
    }
    return true;
  }
  bool sameResizeEdges(const GHOST_IOSInteractionContext &other) const
  {
    if (resize_edges_num != other.resize_edges_num) {
      return false;
    }
    for (int i = 0; i < resize_edges_num; i++) {
      if (!(resize_edges[i] == other.resize_edges[i])) {
        return false;
      }
    }
    return true;
  }
  bool operator!=(const GHOST_IOSInteractionContext &other) const
  {
    return !(*this == other);
  }
};

/**
 * An operator call: the operator id on the first line, then one `name=value` property per line.
 * Values are strings; the window manager converts them to the property's type.
 *
 * A line `@x,y` (GHOST client pixels) runs the call in the area at that point instead of the area
 * under the pointer. Ids starting with `ios.` are handled by the window manager's iOS shim:
 * - `ios.open_editor` with `space=PROPERTIES` or `space=OUTLINER` opens a floating editor.
 * - `ios.touch_spacing` with `enabled=1` or `enabled=0` sets the touch spacing preset.
 * - `ios.workspace` with `name=<workspace>` makes that workspace active.
 * - `ios.timeline` with `minimized=1` or `minimized=0` shrinks the bottom timeline to its header
 *   or gives it back a few rows.
 * - `ios.native_chrome` with `enabled=1` hides Blender's top bar for the native header, and
 *   hides the 3D Viewport tool shelf once, for the native tool dock.
 */
inline bool GHOST_IOSCommand_format(char command[GHOST_IOS_COMMAND_MAX],
                                    const char *idname,
                                    const char *property = nullptr,
                                    const char *value = nullptr)
{
  const size_t idname_len = std::strlen(idname);
  size_t len = idname_len;
  if (property != nullptr) {
    len += 1 + std::strlen(property) + 1 + std::strlen(value);
  }
  if (idname_len == 0 || len >= size_t(GHOST_IOS_COMMAND_MAX)) {
    command[0] = '\0';
    return false;
  }
  std::strcpy(command, idname);
  if (property != nullptr) {
    std::strcat(command, "\n");
    std::strcat(command, property);
    std::strcat(command, "=");
    std::strcat(command, value);
  }
  return true;
}

/** Like #GHOST_IOSCommand_format, aimed at the area at (\a x, \a y) in GHOST client pixels. */
inline bool GHOST_IOSCommand_formatAt(char command[GHOST_IOS_COMMAND_MAX],
                                      const int x,
                                      const int y,
                                      const char *idname,
                                      const char *property = nullptr,
                                      const char *value = nullptr)
{
  char target[32];
  std::snprintf(target, sizeof(target), "\n@%d,%d", x, y);
  char rest[GHOST_IOS_COMMAND_MAX];
  if (!GHOST_IOSCommand_format(rest, idname, property, value)) {
    command[0] = '\0';
    return false;
  }
  const char *newline = std::strchr(rest, '\n');
  const size_t idname_len = newline != nullptr ? size_t(newline - rest) : std::strlen(rest);
  if (std::strlen(rest) + std::strlen(target) >= size_t(GHOST_IOS_COMMAND_MAX)) {
    command[0] = '\0';
    return false;
  }
  std::memcpy(command, rest, idname_len);
  command[idname_len] = '\0';
  std::strcat(command, target);
  if (newline != nullptr) {
    std::strcat(command, newline);
  }
  return true;
}

/**
 * Report the context of \a ghost_window, a `GHOST_IWindow *`. Cheap when nothing changed.
 * Implemented by the iOS window; must be called on the main thread.
 */
void GHOST_IOS_SetInteractionContext(void *ghost_window,
                                     const GHOST_IOSInteractionContext &context);

/** Take the oldest queued operator call for \a ghost_window. Returns false when none is queued. */
bool GHOST_IOS_TakeCommand(void *ghost_window, char command[GHOST_IOS_COMMAND_MAX]);
