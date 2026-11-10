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

struct GHOST_IOSInteractionContext {
  GHOST_IOSEditor editor = GHOST_IOSEditor::Other;
  GHOST_IOSMode mode = GHOST_IOSMode::Object;
  /** The pointer is over the editor's main region, not a header, toolbar or sidebar. */
  bool over_main_region = false;
  /** Operator id of the innermost running modal operator, such as `TRANSFORM_OT_translate`. */
  char modal_operator[GHOST_IOS_IDNAME_MAX] = "";
  /** Active tool of the editor under the pointer, such as `builtin.move`. */
  char tool[GHOST_IOS_IDNAME_MAX] = "";

  bool operator==(const GHOST_IOSInteractionContext &other) const
  {
    return editor == other.editor && mode == other.mode &&
           over_main_region == other.over_main_region &&
           std::strcmp(modal_operator, other.modal_operator) == 0 &&
           std::strcmp(tool, other.tool) == 0;
  }
  bool operator!=(const GHOST_IOSInteractionContext &other) const
  {
    return !(*this == other);
  }
};

/**
 * An operator call: the operator id on the first line, then one `name=value` property per line.
 * Values are strings; the window manager converts them to the property's type.
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

/**
 * Report the context of \a ghost_window, a `GHOST_IWindow *`. Cheap when nothing changed.
 * Implemented by the iOS window; must be called on the main thread.
 */
void GHOST_IOS_SetInteractionContext(void *ghost_window,
                                     const GHOST_IOSInteractionContext &context);

/** Take the oldest queued operator call for \a ghost_window. Returns false when none is queued. */
bool GHOST_IOS_TakeCommand(void *ghost_window, char command[GHOST_IOS_COMMAND_MAX]);
