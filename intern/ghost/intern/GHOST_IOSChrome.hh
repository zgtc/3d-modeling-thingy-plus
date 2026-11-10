/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * Platform-neutral contents of the iPad's native header and tool rail.
 *
 * The header replaces Blender's top bar with one menu button and a scrollable strip of workspace
 * tabs; the mode stays in the 3D Viewport's own mode menu. The tool panel on the left rail, and the
 * Tools page of the Pencil squeeze palette, replace the tool shelf on the left of the viewport. Every
 * control queues an operator call, so tools, modes and workspaces are Blender's own.
 */

#pragma once

#include "GHOST_IOSInteraction.hh"
#include "GHOST_IOSTouchKeys.hh"

#include <cstring>

/* -------------------------------------------------------------------- */
/** \name Workspace tabs
 * \{ */

inline bool GHOST_IOSWorkspace_command(const char *name, char command[GHOST_IOS_COMMAND_MAX])
{
  if (name == nullptr || name[0] == '\0') {
    command[0] = '\0';
    return false;
  }
  return GHOST_IOSCommand_format(command, "ios.workspace", "name", name);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Tools
 * \{ */

struct GHOST_IOSToolItem {
  const char *title;
  /** SF Symbol name. */
  const char *symbol;
  /** Tool id for `wm.tool_set_by_id`. */
  const char *idname;
};

inline constexpr GHOST_IOSToolItem GHOST_IOS_TOOLS_OBJECT[] = {
    {"Select", "cursorarrow", "builtin.select_box"},
    {"Cursor", "scope", "builtin.cursor"},
    {"Move", "arrow.up.and.down.and.arrow.left.and.right", "builtin.move"},
    {"Rotate", "arrow.trianglehead.2.clockwise.rotate.90", "builtin.rotate"},
    {"Scale", "arrow.up.left.and.arrow.down.right", "builtin.scale"},
    {"Transform", "move.3d", "builtin.transform"},
    {"Annotate", "pencil.tip", "builtin.annotate"},
    {"Measure", "ruler", "builtin.measure"},
    {"Add Cube", "cube", "builtin.primitive_cube_add"},
};

inline constexpr GHOST_IOSToolItem GHOST_IOS_TOOLS_EDIT[] = {
    {"Select", "cursorarrow", "builtin.select_box"},
    {"Cursor", "scope", "builtin.cursor"},
    {"Move", "arrow.up.and.down.and.arrow.left.and.right", "builtin.move"},
    {"Rotate", "arrow.trianglehead.2.clockwise.rotate.90", "builtin.rotate"},
    {"Scale", "arrow.up.left.and.arrow.down.right", "builtin.scale"},
    {"Extrude", "square.and.arrow.up", "builtin.extrude_region"},
    {"Inset", "square.inset.filled", "builtin.inset_faces"},
    {"Bevel", "square.on.square.dashed", "builtin.bevel"},
    {"Loop Cut", "rectangle.split.2x1", "builtin.loop_cut"},
    {"Knife", "scissors", "builtin.knife"},
    {"Poly Build", "pentagon", "builtin.poly_build"},
    {"Spin", "arrow.clockwise.circle", "builtin.spin"},
    {"Smooth", "drop", "builtin.smooth"},
    {"Shear", "skew", "builtin.shear"},
    {"Rip", "bolt", "builtin.rip_region"},
};

inline constexpr GHOST_IOSToolItem GHOST_IOS_TOOLS_SCULPT[] = {
    {"Brush", "paintbrush.pointed", "builtin.brush"},
    {"Mask", "rectangle.dashed", "builtin.box_mask"},
    {"Hide", "eye.slash", "builtin.box_hide"},
    {"Face Set", "square.grid.2x2", "builtin.box_face_set"},
    {"Trim", "scissors", "builtin.box_trim"},
    {"Line Project", "line.diagonal", "builtin.line_project"},
    {"Mesh Filter", "waveform.path", "builtin.mesh_filter"},
    {"Cloth Filter", "tshirt", "builtin.cloth_filter"},
    {"Color Filter", "paintpalette", "builtin.color_filter"},
    {"Edit Face Set", "square.grid.3x3.topleft.filled", "builtin.face_set_edit"},
    {"Mask by Color", "eyedropper", "builtin.mask_by_color"},
    {"Move", "arrow.up.and.down.and.arrow.left.and.right", "builtin.move"},
    {"Rotate", "arrow.trianglehead.2.clockwise.rotate.90", "builtin.rotate"},
    {"Scale", "arrow.up.left.and.arrow.down.right", "builtin.scale"},
    {"Annotate", "pencil.tip", "builtin.annotate"},
};

/** Tools for the 3D Viewport mode under the pointer, or none in other editors and modes. */
inline const GHOST_IOSToolItem *GHOST_IOSDock_tools(const GHOST_IOSInteractionContext &context,
                                                    int *r_count)
{
  *r_count = 0;
  if (context.editor != GHOST_IOSEditor::View3D) {
    return nullptr;
  }
  switch (context.mode) {
    case GHOST_IOSMode::Object:
      *r_count = GHOST_IOS_ARRAY_LEN(GHOST_IOS_TOOLS_OBJECT);
      return GHOST_IOS_TOOLS_OBJECT;
    case GHOST_IOSMode::Edit:
      *r_count = GHOST_IOS_ARRAY_LEN(GHOST_IOS_TOOLS_EDIT);
      return GHOST_IOS_TOOLS_EDIT;
    case GHOST_IOSMode::Sculpt:
      *r_count = GHOST_IOS_ARRAY_LEN(GHOST_IOS_TOOLS_SCULPT);
      return GHOST_IOS_TOOLS_SCULPT;
    case GHOST_IOSMode::Paint:
    case GHOST_IOSMode::Other:
      break;
  }
  return nullptr;
}

/** The index of the active tool in \a tools, or -1. */
inline int GHOST_IOSDock_activeIndex(const GHOST_IOSToolItem *tools,
                                     const int count,
                                     const char *active_tool)
{
  for (int i = 0; i < count; i++) {
    if (std::strcmp(tools[i].idname, active_tool) == 0) {
      return i;
    }
  }
  return -1;
}

inline bool GHOST_IOSTool_command(const GHOST_IOSToolItem &tool,
                                  char command[GHOST_IOS_COMMAND_MAX])
{
  return GHOST_IOSCommand_format(command, "wm.tool_set_by_id", "name", tool.idname);
}

/**
 * The squeeze palette's Tools page: the first #GHOST_IOS_PALETTE_SLOT_COUNT tools of the mode.
 * \a r_commands holds the operator calls the slots point at. Returns the number of slots.
 */
inline int GHOST_IOSPalette_toolSlots(const GHOST_IOSInteractionContext &context,
                                      GHOST_IOSPaletteSlot r_slots[GHOST_IOS_PALETTE_SLOT_COUNT],
                                      char r_commands[][GHOST_IOS_COMMAND_MAX])
{
  int count = 0;
  const GHOST_IOSToolItem *tools = GHOST_IOSDock_tools(context, &count);
  if (count > GHOST_IOS_PALETTE_SLOT_COUNT) {
    count = GHOST_IOS_PALETTE_SLOT_COUNT;
  }
  for (int i = 0; i < count; i++) {
    GHOST_IOSTool_command(tools[i], r_commands[i]);
    r_slots[i].title = tools[i].title;
    r_slots[i].symbol = tools[i].symbol;
    r_slots[i].kind = GHOST_IOSSlotKind::Command;
    r_slots[i].stroke = {GHOST_kKeyUnknown, 0, nullptr};
    r_slots[i].command = r_commands[i];
  }
  return count;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Layout
 * \{ */

/** Height of the header strip and of the tool dock, in points. */
constexpr double GHOST_IOS_HEADER_HEIGHT = 52.0;
constexpr double GHOST_IOS_DOCK_HEIGHT = 60.0;

/** \} */
