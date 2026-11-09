/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <cstdlib>
#include <cstring>

#include "BKE_context.hh"
#include "BKE_layer.hh"
#include "BKE_screen.hh"
#include "BKE_wm_runtime.hh"
#include "BLI_listbase_iterator.hh"
#include "BLI_path_utils.hh"
#include "BLI_string.h"
#include "BLI_utildefines.h"
#include "BPY_extern_run.hh"

#include "DNA_object_types.h"
#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_windowmanager_types.h"
#include "DNA_workspace_types.h"

#include "RNA_access.hh"

#include "WM_api.hh"
#include "WM_toolsystem.hh"
#include "WM_types.hh"

#include "GHOST_IOSInteraction.hh"

#include "wm_event_system.hh"

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

static GHOST_IOSInteractionContext wm_ios_interaction_context(const Main &bmain, wmWindow &win)
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

  ScrArea *area = BKE_screen_find_area_xy(screen, SPACE_TYPE_ANY, eventstate->xy);
  if (area == nullptr) {
    return context;
  }
  context.editor = wm_ios_editor(area);
  context.mode = wm_ios_mode(bmain, win, area);
  const ARegion *region = BKE_area_find_region_xy(area, RGN_TYPE_ANY, eventstate->xy);
  context.over_main_region = region != nullptr && region->regiontype == RGN_TYPE_WINDOW;

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

/** Run a queued call as if from a hotkey in the editor under the pointer. */
static void wm_ios_run_command(bContext *C, wmWindow &win, char *command)
{
  char *line = command;
  char *next = strchr(line, '\n');
  if (next != nullptr) {
    *next++ = '\0';
  }
  wmOperatorType *ot = WM_operatortype_find(line, true);
  if (ot == nullptr) {
    return;
  }
  PointerRNA ptr = WM_operator_properties_create_ptr(ot);
  while (next != nullptr) {
    line = next;
    next = strchr(line, '\n');
    if (next != nullptr) {
      *next++ = '\0';
    }
    char *value = strchr(line, '=');
    if (value != nullptr) {
      *value++ = '\0';
      wm_ios_command_property(C, &ptr, line, value);
    }
  }

  wmWindow *prev_win = CTX_wm_window(C);
  ScrArea *prev_area = CTX_wm_area(C);
  ARegion *prev_region = CTX_wm_region(C);
  CTX_wm_window_set(C, &win);
  ScrArea *area = nullptr;
  ARegion *region = nullptr;
  bScreen *screen = WM_window_get_active_screen(&win);
  if (screen != nullptr && win.runtime->eventstate != nullptr) {
    area = BKE_screen_find_area_xy(screen, SPACE_TYPE_ANY, win.runtime->eventstate->xy);
    if (area != nullptr) {
      region = BKE_area_find_region_xy(area, RGN_TYPE_WINDOW, win.runtime->eventstate->xy);
    }
  }
  CTX_wm_area_set(C, area);
  CTX_wm_region_set(C, region);
  WM_operator_name_call_ptr(C, ot, wm::OpCallContext::InvokeDefault, &ptr, nullptr);
  CTX_wm_window_set(C, prev_win);
  CTX_wm_area_set(C, prev_area);
  CTX_wm_region_set(C, prev_region);
  WM_operator_properties_free(&ptr);
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
    GHOST_IOS_SetInteractionContext(ghost_window, wm_ios_interaction_context(*bmain, win));
  }
}

/** \} */

}  // namespace blender
