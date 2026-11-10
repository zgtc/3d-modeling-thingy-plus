/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * Platform-neutral key sets for the Pencil squeeze palette, the command bar and its key pad.
 *
 * Every key sends the same key event a hardware keyboard would, so Blender's keymap decides what
 * it does and add-ons and custom keymaps keep working.
 */

#pragma once

#include "GHOST_IOSInteraction.hh"
#include "GHOST_Types.hh"

#include <cmath>
#include <cstdint>
#include <cstring>

enum : uint8_t {
  GHOST_IOS_STROKE_SHIFT = 1 << 0,
  GHOST_IOS_STROKE_CTRL = 1 << 1,
  GHOST_IOS_STROKE_ALT = 1 << 2,
  GHOST_IOS_STROKE_OS = 1 << 3,
};

struct GHOST_IOSKeyStroke {
  GHOST_TKey key;
  /** #GHOST_IOS_STROKE_SHIFT and friends. */
  uint8_t modifiers;
  /** Text the key types, for number fields. Null for keys that type nothing. */
  const char *utf8;
};

enum class GHOST_IOSSlotKind : uint8_t {
  /** Send #GHOST_IOSPaletteSlot::stroke. */
  Key,
  /** Toggle Ctrl on for the next strokes, which inverts sculpt and paint brushes. */
  InvertBrush,
};

struct GHOST_IOSPaletteSlot {
  const char *title;
  /** SF Symbol name. */
  const char *symbol;
  GHOST_IOSSlotKind kind;
  GHOST_IOSKeyStroke stroke;
};

constexpr int GHOST_IOS_PALETTE_SLOT_COUNT = 8;

#define GHOST_IOS_KEY(key) {key, 0, nullptr}
#define GHOST_IOS_KEY_MOD(key, mods) {key, mods, nullptr}
#define GHOST_IOS_KEY_TEXT(key, text) {key, 0, text}

inline constexpr GHOST_IOSPaletteSlot GHOST_IOS_PALETTE_OBJECT[GHOST_IOS_PALETTE_SLOT_COUNT] = {
    {"Move",
     "arrow.up.and.down.and.arrow.left.and.right",
     GHOST_IOSSlotKind::Key,
     GHOST_IOS_KEY(GHOST_kKeyG)},
    {"Rotate",
     "arrow.trianglehead.2.clockwise.rotate.90",
     GHOST_IOSSlotKind::Key,
     GHOST_IOS_KEY(GHOST_kKeyR)},
    {"Scale",
     "arrow.up.left.and.arrow.down.right",
     GHOST_IOSSlotKind::Key,
     GHOST_IOS_KEY(GHOST_kKeyS)},
    {"Add",
     "plus",
     GHOST_IOSSlotKind::Key,
     GHOST_IOS_KEY_MOD(GHOST_kKeyA, GHOST_IOS_STROKE_SHIFT)},
    {"Delete", "trash", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyX)},
    {"Duplicate",
     "plus.square.on.square",
     GHOST_IOSSlotKind::Key,
     GHOST_IOS_KEY_MOD(GHOST_kKeyD, GHOST_IOS_STROKE_SHIFT)},
    {"Undo",
     "arrow.uturn.backward",
     GHOST_IOSSlotKind::Key,
     GHOST_IOS_KEY_MOD(GHOST_kKeyZ, GHOST_IOS_STROKE_CTRL)},
    {"Redo",
     "arrow.uturn.forward",
     GHOST_IOSSlotKind::Key,
     GHOST_IOS_KEY_MOD(GHOST_kKeyZ, GHOST_IOS_STROKE_CTRL | GHOST_IOS_STROKE_SHIFT)},
};

inline constexpr GHOST_IOSPaletteSlot GHOST_IOS_PALETTE_EDIT[GHOST_IOS_PALETTE_SLOT_COUNT] = {
    {"Vertex", "circle.grid.2x2", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKey1)},
    {"Edge", "line.diagonal", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKey2)},
    {"Face", "square", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKey3)},
    {"Extrude", "square.and.arrow.up", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyE)},
    {"Inset", "square.inset.filled", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyI)},
    {"Loop Cut",
     "rectangle.split.2x1",
     GHOST_IOSSlotKind::Key,
     GHOST_IOS_KEY_MOD(GHOST_kKeyR, GHOST_IOS_STROKE_CTRL)},
    {"Knife", "scissors", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyK)},
    {"Merge", "arrow.triangle.merge", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyM)},
};

/* Sculpt keeps its default brush hotkeys and radial controls; F and Shift F follow the Pencil. */
inline constexpr GHOST_IOSPaletteSlot GHOST_IOS_PALETTE_SCULPT[GHOST_IOS_PALETTE_SLOT_COUNT] = {
    {"Radius", "circle.dashed", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyF)},
    {"Strength",
     "dial.medium",
     GHOST_IOSSlotKind::Key,
     GHOST_IOS_KEY_MOD(GHOST_kKeyF, GHOST_IOS_STROKE_SHIFT)},
    {"Draw", "paintbrush.pointed", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyV)},
    {"Smooth", "drop", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyS)},
    {"Mask", "theatermasks", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyA)},
    {"Invert", "plusminus", GHOST_IOSSlotKind::InvertBrush, GHOST_IOS_KEY(GHOST_kKeyUnknown)},
    {"Undo",
     "arrow.uturn.backward",
     GHOST_IOSSlotKind::Key,
     GHOST_IOS_KEY_MOD(GHOST_kKeyZ, GHOST_IOS_STROKE_CTRL)},
    {"Redo",
     "arrow.uturn.forward",
     GHOST_IOSSlotKind::Key,
     GHOST_IOS_KEY_MOD(GHOST_kKeyZ, GHOST_IOS_STROKE_CTRL | GHOST_IOS_STROKE_SHIFT)},
};

inline constexpr GHOST_IOSPaletteSlot GHOST_IOS_PALETTE_PAINT[GHOST_IOS_PALETTE_SLOT_COUNT] = {
    {"Radius", "circle.dashed", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyF)},
    {"Strength",
     "dial.medium",
     GHOST_IOSSlotKind::Key,
     GHOST_IOS_KEY_MOD(GHOST_kKeyF, GHOST_IOS_STROKE_SHIFT)},
    {"Invert", "plusminus", GHOST_IOSSlotKind::InvertBrush, GHOST_IOS_KEY(GHOST_kKeyUnknown)},
    {"Swap Colors", "arrow.left.arrow.right", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyX)},
    {"Stroke", "scribble", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyE)},
    {"Frame", "viewfinder", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyNumpadPeriod)},
    {"Undo",
     "arrow.uturn.backward",
     GHOST_IOSSlotKind::Key,
     GHOST_IOS_KEY_MOD(GHOST_kKeyZ, GHOST_IOS_STROKE_CTRL)},
    {"Redo",
     "arrow.uturn.forward",
     GHOST_IOSSlotKind::Key,
     GHOST_IOS_KEY_MOD(GHOST_kKeyZ, GHOST_IOS_STROKE_CTRL | GHOST_IOS_STROKE_SHIFT)},
};

/** The palette for what is under the Pencil tip. Always #GHOST_IOS_PALETTE_SLOT_COUNT slots. */
inline const GHOST_IOSPaletteSlot *GHOST_IOSPalette_slots(
    const GHOST_IOSInteractionContext &context)
{
  switch (context.mode) {
    case GHOST_IOSMode::Edit:
      return GHOST_IOS_PALETTE_EDIT;
    case GHOST_IOSMode::Sculpt:
      return GHOST_IOS_PALETTE_SCULPT;
    case GHOST_IOSMode::Paint:
      return GHOST_IOS_PALETTE_PAINT;
    case GHOST_IOSMode::Object:
    case GHOST_IOSMode::Other:
      break;
  }
  return GHOST_IOS_PALETTE_OBJECT;
}

/* -------------------------------------------------------------------- */
/** \name Radial layout
 * \{ */

struct GHOST_IOSPalettePoint {
  double x;
  double y;
};

/**
 * Center of slot \a index of \a count, starting at the top and going clockwise. Screen
 * coordinates: y grows downwards.
 */
inline GHOST_IOSPalettePoint GHOST_IOSPalette_slotCenter(const GHOST_IOSPalettePoint center,
                                                         const double radius,
                                                         const int index,
                                                         const int count)
{
  const double angle = -M_PI_2 + 2.0 * M_PI * double(index) / double(count);
  return {center.x + radius * std::cos(angle), center.y + radius * std::sin(angle)};
}

/**
 * The slot under \a point, or -1 in the middle and outside the ring. The ring spans
 * \a inner_radius to \a outer_radius around \a center.
 */
inline int GHOST_IOSPalette_slotAt(const GHOST_IOSPalettePoint center,
                                   const double inner_radius,
                                   const double outer_radius,
                                   const int count,
                                   const GHOST_IOSPalettePoint point)
{
  const double dx = point.x - center.x;
  const double dy = point.y - center.y;
  const double distance = std::sqrt(dx * dx + dy * dy);
  if (count <= 0 || distance < inner_radius || distance > outer_radius) {
    return -1;
  }
  /* Clockwise from the top, shifted half a slot so each slot owns the wedge around it. */
  double angle = std::atan2(dy, dx) + M_PI_2 + M_PI / double(count);
  angle = std::fmod(angle, 2.0 * M_PI);
  if (angle < 0.0) {
    angle += 2.0 * M_PI;
  }
  const int index = int(angle / (2.0 * M_PI / double(count)));
  return index < count ? index : count - 1;
}

/**
 * Move the palette center so a palette of \a outer_radius stays \a margin inside a
 * \a width by \a height area. A palette that cannot fit is centered on that axis.
 */
inline GHOST_IOSPalettePoint GHOST_IOSPalette_clampCenter(const GHOST_IOSPalettePoint center,
                                                          const double outer_radius,
                                                          const double margin,
                                                          const double width,
                                                          const double height)
{
  const double reach = outer_radius + margin;
  auto clamp_axis = [reach](const double value, const double size) {
    if (size < reach * 2.0) {
      return size * 0.5;
    }
    return value < reach ? reach : (value > size - reach ? size - reach : value);
  };
  return {clamp_axis(center.x, width), clamp_axis(center.y, height)};
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Command bar
 * \{ */

struct GHOST_IOSBarKey {
  const char *title;
  GHOST_IOSKeyStroke stroke;
};

constexpr int GHOST_IOS_BAR_KEY_MAX = 6;

inline constexpr GHOST_IOSBarKey GHOST_IOS_BAR_OBJECT[] = {
    {"Tab", GHOST_IOS_KEY(GHOST_kKeyTab)},
    {"G", GHOST_IOS_KEY_TEXT(GHOST_kKeyG, "g")},
    {"R", GHOST_IOS_KEY_TEXT(GHOST_kKeyR, "r")},
    {"S", GHOST_IOS_KEY_TEXT(GHOST_kKeyS, "s")},
    {"X", GHOST_IOS_KEY_TEXT(GHOST_kKeyX, "x")},
    {"Esc", GHOST_IOS_KEY(GHOST_kKeyEsc)},
};

inline constexpr GHOST_IOSBarKey GHOST_IOS_BAR_EDIT[] = {
    {"Tab", GHOST_IOS_KEY(GHOST_kKeyTab)},
    {"G", GHOST_IOS_KEY_TEXT(GHOST_kKeyG, "g")},
    {"R", GHOST_IOS_KEY_TEXT(GHOST_kKeyR, "r")},
    {"S", GHOST_IOS_KEY_TEXT(GHOST_kKeyS, "s")},
    {"E", GHOST_IOS_KEY_TEXT(GHOST_kKeyE, "e")},
    {"X", GHOST_IOS_KEY_TEXT(GHOST_kKeyX, "x")},
};

inline constexpr GHOST_IOSBarKey GHOST_IOS_BAR_SCULPT[] = {
    {"F", GHOST_IOS_KEY_TEXT(GHOST_kKeyF, "f")},
    {"⇧F", GHOST_IOS_KEY_MOD(GHOST_kKeyF, GHOST_IOS_STROKE_SHIFT)},
    {"H", GHOST_IOS_KEY_TEXT(GHOST_kKeyH, "h")},
    {"⌥H", GHOST_IOS_KEY_MOD(GHOST_kKeyH, GHOST_IOS_STROKE_ALT)},
    {"Tab", GHOST_IOS_KEY(GHOST_kKeyTab)},
    {"Esc", GHOST_IOS_KEY(GHOST_kKeyEsc)},
};

inline constexpr GHOST_IOSBarKey GHOST_IOS_BAR_NODE[] = {
    {"Add", GHOST_IOS_KEY_MOD(GHOST_kKeyA, GHOST_IOS_STROKE_SHIFT)},
    {"G", GHOST_IOS_KEY_TEXT(GHOST_kKeyG, "g")},
    {"X", GHOST_IOS_KEY_TEXT(GHOST_kKeyX, "x")},
    {"Tab", GHOST_IOS_KEY(GHOST_kKeyTab)},
    {"H", GHOST_IOS_KEY_TEXT(GHOST_kKeyH, "h")},
    {"Esc", GHOST_IOS_KEY(GHOST_kKeyEsc)},
};

inline constexpr GHOST_IOSBarKey GHOST_IOS_BAR_OTHER[] = {
    {"Tab", GHOST_IOS_KEY(GHOST_kKeyTab)},
    {"Esc", GHOST_IOS_KEY(GHOST_kKeyEsc)},
    {"Delete", GHOST_IOS_KEY(GHOST_kKeyDelete)},
    {"Return", GHOST_IOS_KEY(GHOST_kKeyEnter)},
};

#define GHOST_IOS_ARRAY_LEN(array) int(sizeof(array) / sizeof(*(array)))

/** Context keys for the editor and mode under the pointer, at most #GHOST_IOS_BAR_KEY_MAX. */
inline const GHOST_IOSBarKey *GHOST_IOSCommandBar_keys(const GHOST_IOSInteractionContext &context,
                                                       int *r_count)
{
  if (context.editor == GHOST_IOSEditor::Node) {
    *r_count = GHOST_IOS_ARRAY_LEN(GHOST_IOS_BAR_NODE);
    return GHOST_IOS_BAR_NODE;
  }
  if (context.editor == GHOST_IOSEditor::View3D || context.editor == GHOST_IOSEditor::Image) {
    switch (context.mode) {
      case GHOST_IOSMode::Object:
        *r_count = GHOST_IOS_ARRAY_LEN(GHOST_IOS_BAR_OBJECT);
        return GHOST_IOS_BAR_OBJECT;
      case GHOST_IOSMode::Edit:
        *r_count = GHOST_IOS_ARRAY_LEN(GHOST_IOS_BAR_EDIT);
        return GHOST_IOS_BAR_EDIT;
      case GHOST_IOSMode::Sculpt:
      case GHOST_IOSMode::Paint:
        *r_count = GHOST_IOS_ARRAY_LEN(GHOST_IOS_BAR_SCULPT);
        return GHOST_IOS_BAR_SCULPT;
      case GHOST_IOSMode::Other:
        break;
    }
  }
  *r_count = GHOST_IOS_ARRAY_LEN(GHOST_IOS_BAR_OTHER);
  return GHOST_IOS_BAR_OTHER;
}

/** Transform key pad, row by row, four keys per row. */
inline constexpr GHOST_IOSBarKey GHOST_IOS_KEY_PAD[] = {
    {"X", GHOST_IOS_KEY_TEXT(GHOST_kKeyX, "x")},
    {"Y", GHOST_IOS_KEY_TEXT(GHOST_kKeyY, "y")},
    {"Z", GHOST_IOS_KEY_TEXT(GHOST_kKeyZ, "z")},
    {"⌫", GHOST_IOS_KEY(GHOST_kKeyBackSpace)},
    {"7", GHOST_IOS_KEY_TEXT(GHOST_kKey7, "7")},
    {"8", GHOST_IOS_KEY_TEXT(GHOST_kKey8, "8")},
    {"9", GHOST_IOS_KEY_TEXT(GHOST_kKey9, "9")},
    {"−", GHOST_IOS_KEY_TEXT(GHOST_kKeyMinus, "-")},
    {"4", GHOST_IOS_KEY_TEXT(GHOST_kKey4, "4")},
    {"5", GHOST_IOS_KEY_TEXT(GHOST_kKey5, "5")},
    {"6", GHOST_IOS_KEY_TEXT(GHOST_kKey6, "6")},
    {".", GHOST_IOS_KEY_TEXT(GHOST_kKeyPeriod, ".")},
    {"1", GHOST_IOS_KEY_TEXT(GHOST_kKey1, "1")},
    {"2", GHOST_IOS_KEY_TEXT(GHOST_kKey2, "2")},
    {"3", GHOST_IOS_KEY_TEXT(GHOST_kKey3, "3")},
    {"0", GHOST_IOS_KEY_TEXT(GHOST_kKey0, "0")},
    {"Esc", GHOST_IOS_KEY(GHOST_kKeyEsc)},
    {"Return", GHOST_IOS_KEY(GHOST_kKeyEnter)},
};
constexpr int GHOST_IOS_KEY_PAD_COLUMNS = 4;
constexpr int GHOST_IOS_KEY_PAD_COUNT = GHOST_IOS_ARRAY_LEN(GHOST_IOS_KEY_PAD);

inline bool GHOST_IOS_hasPrefix(const char *text, const char *prefix)
{
  return std::strncmp(text, prefix, std::strlen(prefix)) == 0;
}

inline bool GHOST_IOS_hasSuffix(const char *text, const char *suffix)
{
  const size_t text_len = std::strlen(text);
  const size_t suffix_len = std::strlen(suffix);
  return text_len >= suffix_len && std::strcmp(text + text_len - suffix_len, suffix) == 0;
}

/** Whether the running modal operator takes typed axes and numbers, like `G Z 2 Enter`. */
inline bool GHOST_IOSCommandBar_wantsKeyPad(const GHOST_IOSInteractionContext &context)
{
  const char *idname = context.modal_operator;
  if (idname[0] == '\0') {
    return false;
  }
  /* Transform, and the macros that end in a transform (extrude, duplicate, rip). */
  if (GHOST_IOS_hasPrefix(idname, "TRANSFORM_OT_") || GHOST_IOS_hasSuffix(idname, "_move")) {
    return true;
  }
  static const char *const numeric_modals[] = {
      "MESH_OT_bevel",
      "MESH_OT_inset",
      "MESH_OT_loopcut_slide",
      "MESH_OT_screw",
      "MESH_OT_spin",
  };
  for (const char *candidate : numeric_modals) {
    if (std::strcmp(idname, candidate) == 0) {
      return true;
    }
  }
  return false;
}

/** \} */
