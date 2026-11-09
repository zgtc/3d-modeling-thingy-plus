/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * Contents of the iPadOS menu bar: Blender's top bar menus and the menus of the editor and mode
 * under the pointer. Every item is an operator call by name, so it goes through Blender's own
 * operators, undo and add-ons. Plain C++ so the tables can be tested on any host.
 */

#pragma once

#include "GHOST_IOSInteraction.hh"

#include <cstring>

struct GHOST_IOSMenuItem {
  const char *title;
  const char *idname;
  const char *property = nullptr;
  const char *value = nullptr;
};

struct GHOST_IOSMenu {
  /** Stable id, part of the `UIMenu` identifier. */
  const char *id;
  const char *title;
  const GHOST_IOSMenuItem *items;
  int count;
};

#define GHOST_IOS_MENU(id, title, items) \
  GHOST_IOSMenu \
  { \
    id, title, items, int(sizeof(items) / sizeof(*(items))) \
  }

inline constexpr GHOST_IOSMenuItem GHOST_IOS_MENU_FILE[] = {
    {"New", "wm.read_homefile"},
    {"Open…", "wm.open_mainfile"},
    {"Save", "wm.save_mainfile"},
    {"Save As…", "wm.save_as_mainfile"},
    {"Import", "wm.call_menu", "name", "TOPBAR_MT_file_import"},
    {"Export", "wm.call_menu", "name", "TOPBAR_MT_file_export"},
};

inline constexpr GHOST_IOSMenuItem GHOST_IOS_MENU_EDIT[] = {
    {"Undo", "ed.undo"},
    {"Redo", "ed.redo"},
    {"Adjust Last Operation…", "screen.redo_last"},
    {"Search…", "wm.search_menu"},
    {"Preferences…", "screen.userpref_show"},
};

inline constexpr GHOST_IOSMenuItem GHOST_IOS_MENU_RENDER[] = {
    {"Render Image", "render.render"},
    {"Render Animation", "render.render", "animation", "1"},
    {"View Render", "render.view_show"},
};

inline constexpr GHOST_IOSMenuItem GHOST_IOS_MENU_WINDOW[] = {
    {"Focus Mode", "screen.screen_full_area", "use_hide_panels", "1"},
    {"Properties", "ios.open_editor", "space", "PROPERTIES"},
    {"Outliner", "ios.open_editor", "space", "OUTLINER"},
};

inline constexpr GHOST_IOSMenuItem GHOST_IOS_MENU_HELP[] = {
    {"Blender Manual", "wm.url_open_preset", "type", "MANUAL"},
};

inline constexpr GHOST_IOSMenuItem GHOST_IOS_MENU_VIEW3D[] = {
    {"Frame Selected", "view3d.view_selected"},
    {"Frame All", "view3d.view_all"},
    {"Toggle Quad View", "screen.region_quadview"},
    {"Toolbar", "wm.context_toggle", "data_path", "space_data.show_region_toolbar"},
    {"Sidebar", "wm.context_toggle", "data_path", "space_data.show_region_ui"},
    {"Snap…", "wm.call_menu_pie", "name", "VIEW3D_MT_snap_pie"},
    {"Pivot Point…", "wm.call_menu_pie", "name", "VIEW3D_MT_pivot_pie"},
};

inline constexpr GHOST_IOSMenuItem GHOST_IOS_MENU_OBJECT[] = {
    {"Add…", "wm.call_menu", "name", "VIEW3D_MT_add"},
    {"Duplicate", "object.duplicate_move"},
    {"Delete", "object.delete"},
    {"Join", "object.join"},
    {"Shade Smooth", "object.shade_smooth"},
    {"Apply…", "wm.call_menu", "name", "VIEW3D_MT_object_apply"},
};

inline constexpr GHOST_IOSMenuItem GHOST_IOS_MENU_MESH[] = {
    {"Extrude", "mesh.extrude_region_move"},
    {"Subdivide", "mesh.subdivide"},
    {"Merge…", "wm.call_menu", "name", "VIEW3D_MT_edit_mesh_merge"},
    {"Delete…", "wm.call_menu", "name", "VIEW3D_MT_edit_mesh_delete"},
    {"Normals…", "wm.call_menu", "name", "VIEW3D_MT_edit_mesh_normals"},
};

inline constexpr GHOST_IOSMenuItem GHOST_IOS_MENU_SCULPT[] = {
    {"Mask…", "wm.call_menu", "name", "VIEW3D_MT_mask"},
    {"Face Sets…", "wm.call_menu", "name", "VIEW3D_MT_face_sets"},
    {"Voxel Remesh", "object.voxel_remesh"},
};

/** Menus that are always there, after the application menu. */
inline int GHOST_IOSMenuBar_topMenus(const GHOST_IOSMenu **r_menus)
{
  static const GHOST_IOSMenu menus[] = {
      GHOST_IOS_MENU("file", "File", GHOST_IOS_MENU_FILE),
      GHOST_IOS_MENU("edit", "Edit", GHOST_IOS_MENU_EDIT),
      GHOST_IOS_MENU("render", "Render", GHOST_IOS_MENU_RENDER),
      GHOST_IOS_MENU("window", "Window", GHOST_IOS_MENU_WINDOW),
      GHOST_IOS_MENU("help", "Help", GHOST_IOS_MENU_HELP),
  };
  *r_menus = menus;
  return int(sizeof(menus) / sizeof(*menus));
}

/** Menus for the editor and mode under the pointer, between Render and Window. */
inline int GHOST_IOSMenuBar_editorMenus(const GHOST_IOSInteractionContext &context,
                                        const GHOST_IOSMenu **r_menus)
{
  static const GHOST_IOSMenu object[] = {
      GHOST_IOS_MENU("view", "Viewport", GHOST_IOS_MENU_VIEW3D),
      GHOST_IOS_MENU("object", "Object", GHOST_IOS_MENU_OBJECT),
  };
  static const GHOST_IOSMenu edit[] = {
      GHOST_IOS_MENU("view", "Viewport", GHOST_IOS_MENU_VIEW3D),
      GHOST_IOS_MENU("mesh", "Mesh", GHOST_IOS_MENU_MESH),
  };
  static const GHOST_IOSMenu sculpt[] = {
      GHOST_IOS_MENU("view", "Viewport", GHOST_IOS_MENU_VIEW3D),
      GHOST_IOS_MENU("sculpt", "Sculpt", GHOST_IOS_MENU_SCULPT),
  };
  static const GHOST_IOSMenu view[] = {
      GHOST_IOS_MENU("view", "Viewport", GHOST_IOS_MENU_VIEW3D),
  };
  if (context.editor != GHOST_IOSEditor::View3D) {
    *r_menus = nullptr;
    return 0;
  }
  switch (context.mode) {
    case GHOST_IOSMode::Object:
      *r_menus = object;
      return 2;
    case GHOST_IOSMode::Edit:
      *r_menus = edit;
      return 2;
    case GHOST_IOSMode::Sculpt:
      *r_menus = sculpt;
      return 2;
    case GHOST_IOSMode::Paint:
    case GHOST_IOSMode::Other:
      break;
  }
  *r_menus = view;
  return 1;
}

/** Whether moving from \a a to \a b changes the editor menus, so the menu bar must be rebuilt. */
inline bool GHOST_IOSMenuBar_needsRebuild(const GHOST_IOSInteractionContext &a,
                                          const GHOST_IOSInteractionContext &b)
{
  const GHOST_IOSMenu *menus_a, *menus_b;
  const int count_a = GHOST_IOSMenuBar_editorMenus(a, &menus_a);
  const int count_b = GHOST_IOSMenuBar_editorMenus(b, &menus_b);
  return count_a != count_b || menus_a != menus_b;
}

/** The command for \a item, see #GHOST_IOSCommand_format. */
inline bool GHOST_IOSMenuItem_command(const GHOST_IOSMenuItem &item,
                                      char command[GHOST_IOS_COMMAND_MAX])
{
  return GHOST_IOSCommand_format(command, item.idname, item.property, item.value);
}
