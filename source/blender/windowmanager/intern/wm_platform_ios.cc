/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "BKE_context.hh"
#include "BKE_layer.hh"
#include "BKE_main.hh"
#include "BKE_screen.hh"
#include "BKE_wm_runtime.hh"
#include "BLI_listbase.h"
#include "BLI_listbase_iterator.hh"
#include "BLI_math_vector.h"
#include "BLI_rect.h"
#include "BLI_path_utils.hh"
#include "BLI_string.h"
#include "BLI_utildefines.h"
#include "BLT_translation.hh"
#include "BPY_extern_run.hh"

#include "DNA_action_types.h"
#include "DNA_object_types.h"
#include "DNA_theme_types.h"
#include "DNA_userdef_types.h"
#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_windowmanager_types.h"
#include "DNA_workspace_types.h"

#include "RNA_access.hh"

#include "WM_api.hh"
#include "WM_toolsystem.hh"
#include "WM_types.hh"

#include "ED_screen.hh"

#include "GHOST_IOSInteraction.hh"

#include "wm_event_system.hh"
#include "wm_ios_layout.hh"
#include "wm_ios_status_keys.hh"
#include "wm_window.hh"

namespace blender {

void wm_init_ios_cycles_smoke(bContext *C)
{
  const char *requested_device = BLI_getenv("BLENDER_IOS_CYCLES_SMOKE");
  if (requested_device == nullptr) {
    return;
  }
  const bool use_metal = STREQ(requested_device, "METAL");

  /* Run only after startup scripts have registered the Cycles render engine. This exercises the
   * embedded module, CPU-device discovery, scene synchronization, TBB task runtime, CPU kernels,
   * and image output rather than treating a successful import as render acceptance. */
  const bool ok = BPY_run_string_exec(
      C,
      nullptr,
      "import bpy, _cycles, os, tempfile\n"
      "_ios_cycles_target = ('METAL' if "
      "os.environ.get('BLENDER_IOS_CYCLES_SMOKE', '').upper() == 'METAL' else 'CPU')\n"
      "_ios_cycles_devices = _cycles.available_devices("
      "_ios_cycles_target if _ios_cycles_target == 'METAL' else 'NONE')\n"
      "assert any(device[1] == _ios_cycles_target for device in _ios_cycles_devices), "
      "_ios_cycles_devices\n"
      "_ios_cycles_scene = bpy.context.scene\n"
      "_ios_cycles_preferences = None\n"
      "_ios_cycles_previous_compute_type = None\n"
      "_ios_cycles_previous_device_uses = []\n"
      "if _ios_cycles_target == 'METAL':\n"
      "    _ios_cycles_preferences = bpy.context.preferences.addons['cycles'].preferences\n"
      "    _ios_cycles_previous_compute_type = _ios_cycles_preferences.compute_device_type\n"
      "    _ios_cycles_preferences.compute_device_type = 'METAL'\n"
      "    _ios_cycles_metal_devices = _ios_cycles_preferences.get_devices_for_type('METAL')\n"
      "    _ios_cycles_previous_device_uses = [(device, device.use) "
      "for device in _ios_cycles_metal_devices]\n"
      "    for device in _ios_cycles_metal_devices:\n"
      "        device.use = (device.type == 'METAL')\n"
      "_ios_cycles_previous = (\n"
      "    _ios_cycles_scene.render.engine,\n"
      "    _ios_cycles_scene.render.resolution_x,\n"
      "    _ios_cycles_scene.render.resolution_y,\n"
      "    _ios_cycles_scene.render.resolution_percentage,\n"
      "    _ios_cycles_scene.render.filepath,\n"
      "    _ios_cycles_scene.render.image_settings.file_format,\n"
      "    _ios_cycles_scene.cycles.device,\n"
      "    _ios_cycles_scene.cycles.samples,\n"
      "    _ios_cycles_scene.cycles.use_adaptive_sampling,\n"
      "    _ios_cycles_scene.cycles.use_denoising,\n"
      ")\n"
      "_ios_cycles_path = os.path.join(tempfile.gettempdir(), 'blender-ios-cycles-smoke.png')\n"
      "try:\n"
      "    _ios_cycles_scene.render.engine = 'CYCLES'\n"
      "    _ios_cycles_scene.cycles.device = "
      "('GPU' if _ios_cycles_target == 'METAL' else 'CPU')\n"
      "    _ios_cycles_scene.cycles.samples = 1\n"
      "    _ios_cycles_scene.cycles.use_adaptive_sampling = False\n"
      "    _ios_cycles_scene.cycles.use_denoising = False\n"
      "    _ios_cycles_scene.render.resolution_x = 8\n"
      "    _ios_cycles_scene.render.resolution_y = 8\n"
      "    _ios_cycles_scene.render.resolution_percentage = 100\n"
      "    _ios_cycles_scene.render.filepath = _ios_cycles_path\n"
      "    _ios_cycles_scene.render.image_settings.file_format = 'PNG'\n"
      "    assert bpy.ops.render.render(write_still=True) == {'FINISHED'}\n"
      "    assert os.path.getsize(_ios_cycles_path) > 64\n"
      "finally:\n"
      "    if os.path.exists(_ios_cycles_path):\n"
      "        os.remove(_ios_cycles_path)\n"
      "    (_ios_cycles_scene.render.engine,\n"
      "     _ios_cycles_scene.render.resolution_x,\n"
      "     _ios_cycles_scene.render.resolution_y,\n"
      "     _ios_cycles_scene.render.resolution_percentage,\n"
      "     _ios_cycles_scene.render.filepath,\n"
      "     _ios_cycles_scene.render.image_settings.file_format,\n"
      "     _ios_cycles_scene.cycles.device,\n"
      "     _ios_cycles_scene.cycles.samples,\n"
      "     _ios_cycles_scene.cycles.use_adaptive_sampling,\n"
      "     _ios_cycles_scene.cycles.use_denoising) = _ios_cycles_previous\n"
      "    for device, previous_use in _ios_cycles_previous_device_uses:\n"
      "        device.use = previous_use\n"
      "    if _ios_cycles_preferences is not None:\n"
      "        _ios_cycles_preferences.compute_device_type = _ios_cycles_previous_compute_type\n"
      "    if _ios_cycles_target == 'METAL':\n"
      "        del _ios_cycles_metal_devices\n"
      "del _ios_cycles_devices, _ios_cycles_path, _ios_cycles_previous, _ios_cycles_scene\n"
      "del _ios_cycles_preferences, _ios_cycles_previous_compute_type\n"
      "del _ios_cycles_previous_device_uses\n"
      "del _ios_cycles_target\n");

  if (ok) {
    fprintf(stderr,
            "BLENDER_IOS_CYCLES_READY=%s-render\n",
            use_metal ? "metal" : "cpu");
  }
  else {
    fprintf(stderr, "BLENDER_IOS_CYCLES_FAILED\n");
  }
}

/* -------------------------------------------------------------------- */
/** \name Touch layer channel
 *
 * The iPad command bar and Pencil palette in GHOST show keys for the editor and mode under the
 * pointer, and queue operator calls such as switching back to the previous tool.
 * \{ */

static GHOST_IOSEditor wm_ios_editor(const ScrArea *area)
{
  switch (area->spacetype) {
    case SPACE_VIEW3D:
      return GHOST_IOSEditor::View3D;
    case SPACE_IMAGE:
      return GHOST_IOSEditor::Image;
    case SPACE_NODE:
      return GHOST_IOSEditor::Node;
    default:
      return GHOST_IOSEditor::Other;
  }
}

static GHOST_IOSMode wm_ios_object_mode(const Object *object)
{
  if (object == nullptr || object->mode == OB_MODE_OBJECT) {
    return GHOST_IOSMode::Object;
  }
  if (object->mode & OB_MODE_EDIT) {
    return GHOST_IOSMode::Edit;
  }
  if (object->mode & OB_MODE_SCULPT) {
    return GHOST_IOSMode::Sculpt;
  }
  if (object->mode & (OB_MODE_TEXTURE_PAINT | OB_MODE_VERTEX_PAINT | OB_MODE_WEIGHT_PAINT)) {
    return GHOST_IOSMode::Paint;
  }
  return GHOST_IOSMode::Other;
}

static GHOST_IOSMode wm_ios_mode(const Main &bmain, wmWindow &win, const ScrArea *area)
{
  if (area->spacetype == SPACE_IMAGE) {
    const SpaceImage *space = static_cast<const SpaceImage *>(area->spacedata.first);
    if (space == nullptr) {
      return GHOST_IOSMode::Other;
    }
    return space->mode == SI_MODE_PAINT ? GHOST_IOSMode::Paint :
           space->mode == SI_MODE_UV    ? GHOST_IOSMode::Edit :
                                          GHOST_IOSMode::Other;
  }
  if (area->spacetype != SPACE_VIEW3D) {
    return GHOST_IOSMode::Other;
  }
  Scene *scene = WM_window_get_active_scene(&win);
  ViewLayer *view_layer = WM_window_get_active_view_layer(&win);
  BKE_view_layer_synced_ensure(bmain, scene, view_layer);
  return wm_ios_object_mode(BKE_view_layer_active_object_get(view_layer));
}

/** The timeline along the bottom of \a screen: the lowest timeline editor, or null. */
static ScrArea *wm_ios_bottom_timeline(bScreen &screen)
{
  ScrArea *bottom = nullptr;
  for (ScrArea &area : screen.areabase) {
    const SpaceAction *saction = area.spacetype == SPACE_ACTION ?
                                     static_cast<const SpaceAction *>(area.spacedata.first) :
                                     nullptr;
    if (saction != nullptr && saction->mode == SACTCONT_TIMELINE &&
        (bottom == nullptr || area.totrct.ymin < bottom->totrct.ymin))
    {
      bottom = &area;
    }
  }
  return bottom;
}

/** Where the native minimize control goes: the right end of the timeline's header row. */
static void wm_ios_timeline_rect(wmWindow &win, bScreen &screen, GHOST_IOSInteractionContext &context)
{
  ScrArea *area = wm_ios_bottom_timeline(screen);
  if (area == nullptr) {
    return;
  }
  const ARegion *header = nullptr;
  for (const ARegion &region : area->regionbase) {
    if (region.regiontype == RGN_TYPE_HEADER && region.runtime != nullptr &&
        region.runtime->visible)
    {
      header = &region;
    }
  }
  const rcti &rect = header != nullptr ? header->winrct : area->totrct;
  int x = area->totrct.xmax, y_top = rect.ymax, x_bottom = x, y_bottom = rect.ymin;
  wm_cursor_position_to_ghost_client_coords(&win, &x, &y_top);
  wm_cursor_position_to_ghost_client_coords(&win, &x_bottom, &y_bottom);
  context.has_timeline = true;
  context.timeline_right = x;
  context.timeline_header_top = y_top;
  context.timeline_header_bottom = y_bottom;
}

/** Timeline height before it was minimized, in window pixels, to give back when expanded. */
static int wm_ios_timeline_expanded_height = 0;

/**
 * Minimized, the bottom timeline is only its header row and the 3D Viewport above takes the room;
 * expanded, it gets back the height it had, at least a few rows.
 */
static void wm_ios_set_timeline_minimized(wmWindow &win, const bool minimized)
{
  bScreen *screen = WM_window_get_active_screen(&win);
  if (screen == nullptr) {
    return;
  }
  ScrArea *area = wm_ios_bottom_timeline(*screen);
  if (area == nullptr || area->v1 == nullptr || area->v2 == nullptr || area->v4 == nullptr) {
    return;
  }
  const int header = ED_area_headersize();
  const int bottom = area->v1->vec.y;
  const int top = area->v2->vec.y;
  const int height = top - bottom;
  int target = height;
  if (minimized) {
    if (height > header + 2) {
      wm_ios_timeline_expanded_height = height;
    }
    target = header + 1;
  }
  else {
    target = std::max(wm_ios_timeline_expanded_height, header * 5);
  }
  if (target == height) {
    return;
  }
  const int new_top = bottom + target;
  const int left = area->v1->vec.x;
  const int right = area->v4->vec.x;
  /* Areas share their corner vertices, so moving the edge resizes the editor above too. */
  for (ScrVert &vert : screen->vertbase) {
    if (vert.vec.y == top && vert.vec.x >= left && vert.vec.x <= right) {
      vert.vec.y = short(new_top);
    }
  }
  screen->do_refresh = true;
}

/**
 * The draggable edges of visible sidebars and toolbars, for the native grab handles. Blender's own
 * resize zone sits on the region's inner edge (see `region_azone_edge`).
 */
static void wm_ios_resize_edges(wmWindow &win, bScreen &screen, GHOST_IOSInteractionContext &context)
{
  context.resize_edges_num = 0;
  for (ScrArea &area : screen.areabase) {
    for (ARegion &region : area.regionbase) {
      if (context.resize_edges_num == GHOST_IOS_RESIZE_EDGE_MAX) {
        return;
      }
      if (!ELEM(region.regiontype, RGN_TYPE_UI, RGN_TYPE_TOOLS) || region.runtime == nullptr || region.runtime->visible == 0 ||
          (region.flag & (RGN_FLAG_HIDDEN | RGN_FLAG_TOO_SMALL)) ||
          BLI_rcti_size_x(&region.winrct) <= 1)
      {
        continue;
      }
      const int align = RGN_ALIGN_ENUM_FROM_MASK(region.alignment);
      if (!ELEM(align, RGN_ALIGN_LEFT, RGN_ALIGN_RIGHT)) {
        continue;
      }
      /* A left-aligned region resizes from its right edge, and the other way around. */
      int x = align == RGN_ALIGN_LEFT ? region.winrct.xmax : region.winrct.xmin;
      int y_top = region.winrct.ymax;
      int x_bottom = x;
      int y_bottom = region.winrct.ymin;
      wm_cursor_position_to_ghost_client_coords(&win, &x, &y_top);
      wm_cursor_position_to_ghost_client_coords(&win, &x_bottom, &y_bottom);
      GHOST_IOSResizeEdge &edge = context.resize_edges[context.resize_edges_num++];
      edge.x = x;
      edge.y_min = y_top;
      edge.y_max = y_bottom;
      edge.region_on_right = align == RGN_ALIGN_RIGHT;
    }
  }
}

/**
 * The keyboard shortcuts of the innermost modal tool that has a modal keymap, in the order its
 * status line lists them, for the quick bar. Mouse bindings are left out: on a touch screen they
 * are of no use, and the status line drops them too.
 */
static void wm_ios_modal_keys(wmWindowManager &wm,
                              wmWindow &win,
                              GHOST_IOSInteractionContext &context)
{
  wmOperator *op = nullptr;
  wmKeyMap *keymap = nullptr;
  for (wmEventHandler &handler_base : win.runtime->modalhandlers) {
    if (handler_base.type != WM_HANDLER_TYPE_OP) {
      continue;
    }
    wmEventHandler_Op &handler = reinterpret_cast<wmEventHandler_Op &>(handler_base);
    if (handler.op == nullptr) {
      continue;
    }
    wmOperator *op_test = handler.op->opm ? handler.op->opm : handler.op;
    wmKeyMap *keymap_test = WM_keymap_active(&wm, op_test->type->modalkeymap);
    if (keymap_test && keymap_test->modal_items) {
      op = op_test;
      keymap = keymap_test;
      break;
    }
  }
  if (keymap == nullptr) {
    return;
  }
  const EnumPropertyItem *items = static_cast<const EnumPropertyItem *>(keymap->modal_items);
  for (int i = 0; items[i].identifier && context.modal_keys_num < GHOST_IOS_MODAL_KEY_MAX; i++) {
    if (items[i].identifier[0] == '\0') {
      continue;
    }
    if (keymap->poll_modal_item && !keymap->poll_modal_item(op, items[i].value)) {
      continue;
    }
    for (const wmKeyMapItem &kmi : keymap->items) {
      if (kmi.propvalue != items[i].value || (kmi.flag & KMI_INACTIVE) || kmi.val == KM_RELEASE)
      {
        continue;
      }
      if (wm_ios_modal_key_add(context,
                               IFACE_(items[i].name),
                               kmi.type,
                               kmi.shift == KM_MOD_HELD,
                               kmi.ctrl == KM_MOD_HELD,
                               kmi.alt == KM_MOD_HELD,
                               kmi.oskey == KM_MOD_HELD))
      {
        break;
      }
    }
  }
}

static GHOST_IOSInteractionContext wm_ios_interaction_context(wmWindowManager &wm,
                                                              const Main &bmain,
                                                              wmWindow &win)
{
  GHOST_IOSInteractionContext context;
  const wmEvent *eventstate = win.runtime->eventstate;
  bScreen *screen = WM_window_get_active_screen(&win);
  if (eventstate == nullptr || screen == nullptr) {
    return context;
  }

  /* The innermost modal operator is the first operator handler. */
  for (wmEventHandler &handler : win.runtime->modalhandlers) {
    if (handler.type != WM_HANDLER_TYPE_OP) {
      continue;
    }
    const wmOperator *op = reinterpret_cast<wmEventHandler_Op &>(handler).op;
    if (op != nullptr && op->type != nullptr) {
      STRNCPY(context.modal_operator, op->type->idname);
      break;
    }
  }
  wm_ios_modal_keys(wm, win, context);

  ScrArea *area = BKE_screen_find_area_xy(screen, SPACE_TYPE_ANY, eventstate->xy);
  if (area == nullptr) {
    return context;
  }
  context.editor = wm_ios_editor(area);
  context.mode = wm_ios_mode(bmain, win, area);
  const ARegion *region = BKE_area_find_region_xy(area, RGN_TYPE_ANY, eventstate->xy);
  context.over_main_region = region != nullptr && region->regiontype == RGN_TYPE_WINDOW;

  wm_ios_resize_edges(win, *screen, context);
  wm_ios_timeline_rect(win, *screen, context);

  /* Workspace tabs, in Blender's order. */
  const WorkSpace *active_workspace = WM_window_get_active_workspace(&win);
  for (const WorkSpace &workspace : bmain.workspaces) {
    if (context.workspaces_num == GHOST_IOS_WORKSPACE_MAX) {
      break;
    }
    if (&workspace == active_workspace) {
      context.active_workspace = context.workspaces_num;
    }
    STRNCPY(context.workspaces[context.workspaces_num++], workspace.id.name + 2);
  }

  WorkSpace *workspace = WM_window_get_active_workspace(&win);
  bToolKey tkey;
  if (workspace != nullptr &&
      WM_toolsystem_key_from_context(bmain,
                                     WM_window_get_active_scene(&win),
                                     WM_window_get_active_view_layer(&win),
                                     area,
                                     &tkey))
  {
    const bToolRef *tref = WM_toolsystem_ref_find(workspace, &tkey);
    if (tref != nullptr) {
      STRNCPY(context.tool, tref->idname);
    }
  }
  return context;
}

/** Set one property of \a ptr from text, converting it to the property's type. */
static void wm_ios_command_property(bContext *C,
                                    PointerRNA *ptr,
                                    const char *name,
                                    const char *value)
{
  PropertyRNA *prop = RNA_struct_find_property(ptr, name);
  if (prop == nullptr) {
    return;
  }
  switch (RNA_property_type(prop)) {
    case PROP_STRING:
      RNA_property_string_set(ptr, prop, value);
      break;
    case PROP_ENUM: {
      int enum_value = 0;
      if (RNA_property_enum_value(C, ptr, prop, value, &enum_value)) {
        RNA_property_enum_set(ptr, prop, enum_value);
      }
      break;
    }
    case PROP_BOOLEAN:
      RNA_property_boolean_set(ptr, prop, STREQ(value, "1") || STREQ(value, "True"));
      break;
    case PROP_INT:
      RNA_property_int_set(ptr, prop, atoi(value));
      break;
    case PROP_FLOAT:
      RNA_property_float_set(ptr, prop, float(atof(value)));
      break;
    default:
      break;
  }
}

/** Spacing in #uiStyle before the touch preset changed it, to restore it exactly. */
static WMIOSStyleSpacing wm_ios_default_spacing;

/**
 * Touch spacing preset: more room beside buttons so a finger lands on the intended one, a little
 * more between rows and panels, and slightly larger text (#wm_ios_touch_spacing). The global UI
 * scale is left alone.
 */
static void wm_ios_set_touch_spacing(const bool enabled)
{
  uiStyle *style = static_cast<uiStyle *>(U.uistyles.first);
  if (style == nullptr) {
    return;
  }
  WMIOSStyleSpacing &spacing = wm_ios_default_spacing;
  if (!spacing.saved) {
    spacing = {true,
               style->columnspace,
               style->templatespace,
               style->boxspace,
               style->buttonspacex,
               style->buttonspacey,
               style->panelspace,
               style->panelouter,
               style->widget.points,
               style->grouplabel.points,
               style->paneltitle.points};
  }
  const WMIOSStyleSpacing touch = wm_ios_touch_spacing(spacing, enabled);
  style->columnspace = touch.columnspace;
  style->templatespace = touch.templatespace;
  style->boxspace = touch.boxspace;
  style->buttonspacex = touch.buttonspacex;
  style->buttonspacey = touch.buttonspacey;
  style->panelspace = touch.panelspace;
  style->panelouter = touch.panelouter;
  style->widget.points = touch.widget_points;
  style->grouplabel.points = touch.grouplabel_points;
  style->paneltitle.points = touch.paneltitle_points;
  WM_main_add_notifier(NC_WINDOW, nullptr);
}

/** The iPad's native header and tool dock are active. */
static bool wm_ios_native_chrome = false;

/** Hide the 3D Viewport tool shelf once, when the native tool dock takes its place. */
static void wm_ios_hide_tool_shelves(Main &bmain)
{
  for (bScreen &screen : bmain.screens) {
    for (ScrArea &area : screen.areabase) {
      if (area.spacetype != SPACE_VIEW3D) {
        continue;
      }
      for (ARegion &region : area.regionbase) {
        if (region.regiontype == RGN_TYPE_TOOLS && !(region.flag & RGN_FLAG_HIDDEN)) {
          region.flag |= RGN_FLAG_HIDDEN;
          screen.do_refresh = true;
        }
      }
    }
  }
}

/**
 * Keep Blender's top bar hidden while the native header replaces it. Leaving Focus mode
 * shows every global area again, so this runs on every main loop pass.
 */
static void wm_ios_hide_topbar(wmWindow &win)
{
  for (ScrArea &area : win.global_areas.areabase) {
    if (area.spacetype == SPACE_TOPBAR && area.global != nullptr &&
        !(area.global->flag & GLOBAL_AREA_IS_HIDDEN))
    {
      area.global->flag |= GLOBAL_AREA_IS_HIDDEN;
      if (bScreen *screen = WM_window_get_active_screen(&win)) {
        screen->do_refresh = true;
      }
    }
  }
}

/** Commands for the touch layer that are not operators. */
static bool wm_ios_run_builtin(bContext *C, const char *idname, const char *name, const char *value)
{
  if (STREQ(idname, "ios.workspace")) {
    if (name != nullptr && STREQ(name, "name")) {
      Main *bmain = CTX_data_main(C);
      wmWindow *win = CTX_wm_window(C);
      WorkSpace *workspace = static_cast<WorkSpace *>(
          BLI_findstring(&bmain->workspaces, value, offsetof(ID, name) + 2));
      if (workspace != nullptr && win != nullptr &&
          workspace != WM_window_get_active_workspace(win))
      {
        WM_window_set_active_workspace(C, win, workspace);
      }
    }
    return true;
  }
  if (STREQ(idname, "ios.timeline")) {
    if (wmWindow *win = CTX_wm_window(C)) {
      wm_ios_set_timeline_minimized(
          *win, name != nullptr && STREQ(name, "minimized") && STREQ(value, "1"));
    }
    return true;
  }
  if (STREQ(idname, "ios.native_chrome")) {
    const bool enabled = name != nullptr && STREQ(name, "enabled") && STREQ(value, "1");
    if (enabled && !wm_ios_native_chrome) {
      wm_ios_hide_tool_shelves(*CTX_data_main(C));
    }
    wm_ios_native_chrome = enabled;
    return true;
  }
  if (STREQ(idname, "ios.touch_spacing")) {
    wm_ios_set_touch_spacing(name != nullptr && STREQ(name, "enabled") && STREQ(value, "1"));
    return true;
  }
  if (STREQ(idname, "ios.open_editor")) {
    if (name == nullptr || !STREQ(name, "space")) {
      return true;
    }
    if (STREQ(value, "PROPERTIES")) {
      WM_window_open_temp(C, IFACE_("Properties"), SPACE_PROPERTIES, false);
    }
    else if (STREQ(value, "OUTLINER")) {
      WM_window_open_temp(C, IFACE_("Outliner"), SPACE_OUTLINER, false);
    }
    return true;
  }
  return false;
}

/** Split off the next line of \a text, which is changed in place. */
static char *wm_ios_next_line(char **text)
{
  char *line = *text;
  if (line == nullptr) {
    return nullptr;
  }
  char *newline = strchr(line, '\n');
  if (newline != nullptr) {
    *newline = '\0';
    *text = newline + 1;
  }
  else {
    *text = nullptr;
  }
  return line;
}

/** Run a queued call as if from a hotkey in the editor under the pointer, or under `@x,y`. */
static void wm_ios_run_command(bContext *C, wmWindow &win, char *command)
{
  char *rest = command;
  const char *idname = wm_ios_next_line(&rest);
  int xy[2];
  bool has_xy = false;
  if (win.runtime->eventstate != nullptr) {
    copy_v2_v2_int(xy, win.runtime->eventstate->xy);
    has_xy = true;
  }
  if (rest != nullptr && rest[0] == '@') {
    char *target = wm_ios_next_line(&rest);
    int x = 0, y = 0;
    if (sscanf(target + 1, "%d,%d", &x, &y) == 2) {
      wm_cursor_position_from_ghost_client_coords(&win, &x, &y);
      xy[0] = x;
      xy[1] = y;
      has_xy = true;
    }
  }

  wmWindow *prev_win = CTX_wm_window(C);
  ScrArea *prev_area = CTX_wm_area(C);
  ARegion *prev_region = CTX_wm_region(C);
  CTX_wm_window_set(C, &win);
  ScrArea *area = nullptr;
  ARegion *region = nullptr;
  bScreen *screen = WM_window_get_active_screen(&win);
  if (screen != nullptr && has_xy) {
    area = BKE_screen_find_area_xy(screen, SPACE_TYPE_ANY, xy);
    if (area != nullptr) {
      region = BKE_area_find_region_xy(area, RGN_TYPE_WINDOW, xy);
    }
  }
  CTX_wm_area_set(C, area);
  CTX_wm_region_set(C, region);

  if (STRPREFIX(idname, "ios.")) {
    char *line = wm_ios_next_line(&rest);
    char *value = line != nullptr ? strchr(line, '=') : nullptr;
    if (value != nullptr) {
      *value++ = '\0';
    }
    wm_ios_run_builtin(C, idname, value != nullptr ? line : nullptr, value);
  }
  else if (wmOperatorType *ot = WM_operatortype_find(idname, true)) {
    PointerRNA ptr = WM_operator_properties_create_ptr(ot);
    while (char *line = wm_ios_next_line(&rest)) {
      char *value = strchr(line, '=');
      if (value != nullptr) {
        *value++ = '\0';
        wm_ios_command_property(C, &ptr, line, value);
      }
    }
    WM_operator_name_call_ptr(C, ot, wm::OpCallContext::InvokeDefault, &ptr, nullptr);
    WM_operator_properties_free(&ptr);
  }

  CTX_wm_window_set(C, prev_win);
  CTX_wm_area_set(C, prev_area);
  CTX_wm_region_set(C, prev_region);
}

void wm_ios_touch_channel_update(bContext *C)
{
  wmWindowManager *wm = CTX_wm_manager(C);
  Main *bmain = CTX_data_main(C);
  if (wm == nullptr || bmain == nullptr) {
    return;
  }
  for (wmWindow &win : wm->windows) {
    void *ghost_window = win.runtime->ghostwin;
    if (ghost_window == nullptr) {
      continue;
    }
    char command[GHOST_IOS_COMMAND_MAX];
    while (GHOST_IOS_TakeCommand(ghost_window, command)) {
      wm_ios_run_command(C, win, command);
    }
    if (wm_ios_native_chrome) {
      wm_ios_hide_topbar(win);
    }
    GHOST_IOS_SetInteractionContext(ghost_window, wm_ios_interaction_context(*wm, *bmain, win));
  }
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Factory layout for touch
 *
 * Applied only to the factory startup file, never to files the user saved.
 * \{ */

/**
 * Give the Outliner and Properties column on the right more room, so item names and property
 * labels are not cut off at the touch UI scale (#wm_ios_right_column_left).
 */
static void wm_ios_widen_right_column(bScreen &screen)
{
  int screen_right = 0;
  for (const ScrVert &vert : screen.vertbase) {
    screen_right = std::max(screen_right, int(vert.vec.x));
  }
  int column_left = screen_right;
  for (const ScrArea &area : screen.areabase) {
    if (ELEM(area.spacetype, SPACE_OUTLINER, SPACE_PROPERTIES) && area.v4->vec.x == screen_right) {
      column_left = std::min(column_left, int(area.v1->vec.x));
    }
  }
  if (column_left <= 0 || column_left >= screen_right) {
    return;
  }
  const int new_left = wm_ios_right_column_left(screen_right, column_left);
  if (new_left >= column_left) {
    return;
  }
  /* Areas share their corner vertices, so moving the edge resizes both sides of it. */
  for (ScrVert &vert : screen.vertbase) {
    if (vert.vec.x == column_left) {
      vert.vec.x = short(new_left);
    }
  }
}

/**
 * Give the Outliner at the top of the right column at least two fifths of the column's height
 * (#wm_ios_outliner_bottom), so more of the scene's hierarchy shows at once.
 */
static void wm_ios_grow_outliner(bScreen &screen)
{
  int screen_right = 0;
  for (const ScrVert &vert : screen.vertbase) {
    screen_right = std::max(screen_right, int(vert.vec.x));
  }
  for (ScrArea &outliner : screen.areabase) {
    if (outliner.spacetype != SPACE_OUTLINER || outliner.v4->vec.x != screen_right) {
      continue;
    }
    /* The editor directly below it, sharing its bottom edge. */
    const int left = outliner.v1->vec.x;
    const int bottom = outliner.v1->vec.y;
    for (const ScrArea &below : screen.areabase) {
      if (&below == &outliner || below.v2->vec.y != bottom || below.v1->vec.x != left ||
          below.v4->vec.x != screen_right)
      {
        continue;
      }
      const int new_bottom = wm_ios_outliner_bottom(
          outliner.v2->vec.y, below.v1->vec.y, bottom);
      if (new_bottom >= bottom || new_bottom <= below.v1->vec.y + HEADERY * 2) {
        break;
      }
      for (ScrVert &vert : screen.vertbase) {
        if (vert.vec.y == bottom && vert.vec.x >= left && vert.vec.x <= screen_right) {
          vert.vec.y = short(new_bottom);
        }
      }
      break;
    }
    break;
  }
}

/** Shrink a timeline along the bottom of the screen to its header: a single row of controls. */
static void wm_ios_collapse_timeline(bScreen &screen)
{
  for (ScrArea &area : screen.areabase) {
    const SpaceAction *saction = area.spacetype == SPACE_ACTION ?
                                     static_cast<const SpaceAction *>(area.spacedata.first) :
                                     nullptr;
    if (saction == nullptr || saction->mode != SACTCONT_TIMELINE || area.v1->vec.y != 0) {
      continue;
    }
    const int top = area.v2->vec.y;
    const int new_top = area.v1->vec.y + HEADERY + 1;
    if (new_top >= top) {
      continue;
    }
    const int left = area.v1->vec.x;
    const int right = area.v4->vec.x;
    for (ScrVert &vert : screen.vertbase) {
      if (vert.vec.y == top && vert.vec.x >= left && vert.vec.x <= right) {
        vert.vec.y = short(new_top);
      }
    }
  }
}

void wm_ios_update_startup_defaults(Main *bmain, const char * /*app_template*/)
{
  for (bScreen &screen : bmain->screens) {
    wm_ios_widen_right_column(screen);
    wm_ios_grow_outliner(screen);
    wm_ios_collapse_timeline(screen);
  }
}

/** \} */

}  // namespace blender
