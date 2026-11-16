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

static_assert(GHOST_IOS_PALETTE_SLOT_COUNT == GHOST_IOS_PALETTE_LAYOUT_SLOTS,
              "the stored palette layout needs one entry per slot");

/* -------------------------------------------------------------------- */
/** \name Editable palette
 *
 * Every action a palette slot can be set to. The user's layout stores indices into this table,
 * so only ever append to it.
 * \{ */

enum : uint8_t {
  GHOST_IOS_PALETTE_FOR_OBJECT = 1 << 0,
  GHOST_IOS_PALETTE_FOR_EDIT = 1 << 1,
  GHOST_IOS_PALETTE_FOR_SCULPT = 1 << 2,
  GHOST_IOS_PALETTE_FOR_PAINT = 1 << 3,
  GHOST_IOS_PALETTE_FOR_ALL = 0xF,
};

struct GHOST_IOSPaletteAction {
  GHOST_IOSPaletteSlot slot;
  /** Modes the action suits, as #GHOST_IOS_PALETTE_FOR_OBJECT and friends. */
  uint8_t modes;
};

inline constexpr GHOST_IOSPaletteAction GHOST_IOS_PALETTE_CATALOG[] = {
    /* General. */
    {{"Undo",
      "arrow.uturn.backward",
      GHOST_IOSSlotKind::Key,
      GHOST_IOS_KEY_MOD(GHOST_kKeyZ, GHOST_IOS_STROKE_CTRL)},
     GHOST_IOS_PALETTE_FOR_ALL},
    {{"Redo",
      "arrow.uturn.forward",
      GHOST_IOSSlotKind::Key,
      GHOST_IOS_KEY_MOD(GHOST_kKeyZ, GHOST_IOS_STROKE_CTRL | GHOST_IOS_STROKE_SHIFT)},
     GHOST_IOS_PALETTE_FOR_ALL},
    {{"Frame Selected",
      "viewfinder",
      GHOST_IOSSlotKind::Key,
      GHOST_IOS_KEY(GHOST_kKeyNumpadPeriod)},
     GHOST_IOS_PALETTE_FOR_ALL},
    {{"X-Ray",
      "square.on.square.dashed",
      GHOST_IOSSlotKind::Key,
      GHOST_IOS_KEY_MOD(GHOST_kKeyZ, GHOST_IOS_STROKE_ALT)},
     GHOST_IOS_PALETTE_FOR_OBJECT | GHOST_IOS_PALETTE_FOR_EDIT},
    /* Object and edit. */
    {{"Move",
      "arrow.up.and.down.and.arrow.left.and.right",
      GHOST_IOSSlotKind::Key,
      GHOST_IOS_KEY(GHOST_kKeyG)},
     GHOST_IOS_PALETTE_FOR_OBJECT | GHOST_IOS_PALETTE_FOR_EDIT},
    {{"Rotate",
      "arrow.trianglehead.2.clockwise.rotate.90",
      GHOST_IOSSlotKind::Key,
      GHOST_IOS_KEY(GHOST_kKeyR)},
     GHOST_IOS_PALETTE_FOR_OBJECT | GHOST_IOS_PALETTE_FOR_EDIT},
    {{"Scale",
      "arrow.up.left.and.arrow.down.right",
      GHOST_IOSSlotKind::Key,
      GHOST_IOS_KEY(GHOST_kKeyS)},
     GHOST_IOS_PALETTE_FOR_OBJECT | GHOST_IOS_PALETTE_FOR_EDIT},
    {{"Delete", "trash", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyX)},
     GHOST_IOS_PALETTE_FOR_OBJECT | GHOST_IOS_PALETTE_FOR_EDIT},
    {{"Box Select", "rectangle.dashed", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyB)},
     GHOST_IOS_PALETTE_FOR_OBJECT | GHOST_IOS_PALETTE_FOR_EDIT},
    {{"Select All", "checkmark.rectangle", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyA)},
     GHOST_IOS_PALETTE_FOR_OBJECT | GHOST_IOS_PALETTE_FOR_EDIT},
    {{"Hide", "eye.slash", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyH)},
     GHOST_IOS_PALETTE_FOR_OBJECT | GHOST_IOS_PALETTE_FOR_EDIT},
    {{"Reveal",
      "eye",
      GHOST_IOSSlotKind::Key,
      GHOST_IOS_KEY_MOD(GHOST_kKeyH, GHOST_IOS_STROKE_ALT)},
     GHOST_IOS_PALETTE_FOR_OBJECT | GHOST_IOS_PALETTE_FOR_EDIT},
    /* Object. */
    {{"Add",
      "plus",
      GHOST_IOSSlotKind::Key,
      GHOST_IOS_KEY_MOD(GHOST_kKeyA, GHOST_IOS_STROKE_SHIFT)},
     GHOST_IOS_PALETTE_FOR_OBJECT},
    {{"Duplicate",
      "plus.square.on.square",
      GHOST_IOSSlotKind::Key,
      GHOST_IOS_KEY_MOD(GHOST_kKeyD, GHOST_IOS_STROKE_SHIFT)},
     GHOST_IOS_PALETTE_FOR_OBJECT},
    {{"Apply",
      "checkmark.circle",
      GHOST_IOSSlotKind::Key,
      GHOST_IOS_KEY_MOD(GHOST_kKeyA, GHOST_IOS_STROKE_CTRL)},
     GHOST_IOS_PALETTE_FOR_OBJECT},
    {{"Parent",
      "link",
      GHOST_IOSSlotKind::Key,
      GHOST_IOS_KEY_MOD(GHOST_kKeyP, GHOST_IOS_STROKE_CTRL)},
     GHOST_IOS_PALETTE_FOR_OBJECT},
    {{"Join",
      "rectangle.on.rectangle",
      GHOST_IOSSlotKind::Key,
      GHOST_IOS_KEY_MOD(GHOST_kKeyJ, GHOST_IOS_STROKE_CTRL)},
     GHOST_IOS_PALETTE_FOR_OBJECT},
    /* Edit. */
    {{"Vertex", "circle.grid.2x2", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKey1)},
     GHOST_IOS_PALETTE_FOR_EDIT},
    {{"Edge", "line.diagonal", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKey2)},
     GHOST_IOS_PALETTE_FOR_EDIT},
    {{"Face", "square", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKey3)},
     GHOST_IOS_PALETTE_FOR_EDIT},
    {{"Extrude", "square.and.arrow.up", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyE)},
     GHOST_IOS_PALETTE_FOR_EDIT},
    {{"Inset", "square.inset.filled", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyI)},
     GHOST_IOS_PALETTE_FOR_EDIT},
    {{"Bevel",
      "square.on.circle",
      GHOST_IOSSlotKind::Key,
      GHOST_IOS_KEY_MOD(GHOST_kKeyB, GHOST_IOS_STROKE_CTRL)},
     GHOST_IOS_PALETTE_FOR_EDIT},
    {{"Loop Cut",
      "rectangle.split.2x1",
      GHOST_IOSSlotKind::Key,
      GHOST_IOS_KEY_MOD(GHOST_kKeyR, GHOST_IOS_STROKE_CTRL)},
     GHOST_IOS_PALETTE_FOR_EDIT},
    {{"Knife", "scissors", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyK)},
     GHOST_IOS_PALETTE_FOR_EDIT},
    {{"Merge", "arrow.triangle.merge", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyM)},
     GHOST_IOS_PALETTE_FOR_EDIT},
    {{"Fill", "paintbucket.classic", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyF)},
     GHOST_IOS_PALETTE_FOR_EDIT},
    /* Sculpt and paint. */
    {{"Radius", "circle.dashed", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyF)},
     GHOST_IOS_PALETTE_FOR_SCULPT | GHOST_IOS_PALETTE_FOR_PAINT},
    {{"Strength",
      "dial.medium",
      GHOST_IOSSlotKind::Key,
      GHOST_IOS_KEY_MOD(GHOST_kKeyF, GHOST_IOS_STROKE_SHIFT)},
     GHOST_IOS_PALETTE_FOR_SCULPT | GHOST_IOS_PALETTE_FOR_PAINT},
    {{"Invert",
      "plusminus",
      GHOST_IOSSlotKind::InvertBrush,
      GHOST_IOS_KEY(GHOST_kKeyUnknown)},
     GHOST_IOS_PALETTE_FOR_SCULPT | GHOST_IOS_PALETTE_FOR_PAINT},
    /* Sculpt. */
    {{"Draw", "paintbrush.pointed", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyV)},
     GHOST_IOS_PALETTE_FOR_SCULPT},
    {{"Smooth", "drop", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyS)},
     GHOST_IOS_PALETTE_FOR_SCULPT},
    {{"Mask", "theatermasks", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyA)},
     GHOST_IOS_PALETTE_FOR_SCULPT},
    /* Paint. */
    {{"Swap Colors",
      "arrow.left.arrow.right",
      GHOST_IOSSlotKind::Key,
      GHOST_IOS_KEY(GHOST_kKeyX)},
     GHOST_IOS_PALETTE_FOR_PAINT},
    {{"Stroke", "scribble", GHOST_IOSSlotKind::Key, GHOST_IOS_KEY(GHOST_kKeyE)},
     GHOST_IOS_PALETTE_FOR_PAINT},
};

inline constexpr int GHOST_IOS_PALETTE_CATALOG_COUNT = int(
    sizeof(GHOST_IOS_PALETTE_CATALOG) / sizeof(GHOST_IOS_PALETTE_CATALOG[0]));

/** Index into a layout for \a mode. #GHOST_IOSMode::Other uses the Object palette. */
inline int GHOST_IOSPalette_modeIndex(const GHOST_IOSMode mode)
{
  switch (mode) {
    case GHOST_IOSMode::Edit:
      return 1;
    case GHOST_IOSMode::Sculpt:
      return 2;
    case GHOST_IOSMode::Paint:
      return 3;
    case GHOST_IOSMode::Object:
    case GHOST_IOSMode::Other:
      break;
  }
  return 0;
}

/** The mode bit of #GHOST_IOSPaletteAction::modes for a layout index. */
inline uint8_t GHOST_IOSPalette_modeBit(const int mode_index)
{
  return uint8_t(1u << mode_index);
}

/** \} */

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

/**
 * The slots to show for \a mode: the user's choices from \a layout, and the built-in slot where
 * a choice is unset or out of range.
 */
inline void GHOST_IOSPalette_resolve(
    const GHOST_IOSMode mode,
    const int8_t layout[GHOST_IOS_PALETTE_MODE_COUNT][GHOST_IOS_PALETTE_LAYOUT_SLOTS],
    GHOST_IOSPaletteSlot r_slots[GHOST_IOS_PALETTE_SLOT_COUNT])
{
  GHOST_IOSInteractionContext context;
  context.mode = mode;
  const GHOST_IOSPaletteSlot *defaults = GHOST_IOSPalette_slots(context);
  const int mode_index = GHOST_IOSPalette_modeIndex(mode);
  for (int i = 0; i < GHOST_IOS_PALETTE_SLOT_COUNT; i++) {
    const int choice = layout[mode_index][i];
    r_slots[i] = (choice >= 0 && choice < GHOST_IOS_PALETTE_CATALOG_COUNT) ?
                     GHOST_IOS_PALETTE_CATALOG[choice].slot :
                     defaults[i];
  }
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
  /** The key as the button shows it, like "R" or "⇧F". */
  const char *title;
  GHOST_IOSKeyStroke stroke;
  /** What the key does here, shown under the key. Null shows the key alone. */
  const char *name;
};

constexpr int GHOST_IOS_BAR_KEY_MAX = 6;

inline constexpr GHOST_IOSBarKey GHOST_IOS_BAR_OBJECT[] = {
    {"Tab", GHOST_IOS_KEY(GHOST_kKeyTab), "Edit Mode"},
    {"G", GHOST_IOS_KEY_TEXT(GHOST_kKeyG, "g"), "Move"},
    {"R", GHOST_IOS_KEY_TEXT(GHOST_kKeyR, "r"), "Rotate"},
    {"S", GHOST_IOS_KEY_TEXT(GHOST_kKeyS, "s"), "Scale"},
    {"X", GHOST_IOS_KEY_TEXT(GHOST_kKeyX, "x"), "Delete"},
};

inline constexpr GHOST_IOSBarKey GHOST_IOS_BAR_EDIT[] = {
    {"Tab", GHOST_IOS_KEY(GHOST_kKeyTab), "Object Mode"},
    {"G", GHOST_IOS_KEY_TEXT(GHOST_kKeyG, "g"), "Move"},
    {"R", GHOST_IOS_KEY_TEXT(GHOST_kKeyR, "r"), "Rotate"},
    {"S", GHOST_IOS_KEY_TEXT(GHOST_kKeyS, "s"), "Scale"},
    {"E", GHOST_IOS_KEY_TEXT(GHOST_kKeyE, "e"), "Extrude"},
    {"X", GHOST_IOS_KEY_TEXT(GHOST_kKeyX, "x"), "Delete"},
};

inline constexpr GHOST_IOSBarKey GHOST_IOS_BAR_SCULPT[] = {
    {"F", GHOST_IOS_KEY_TEXT(GHOST_kKeyF, "f"), "Radius"},
    {"⇧F", GHOST_IOS_KEY_MOD(GHOST_kKeyF, GHOST_IOS_STROKE_SHIFT), "Strength"},
    {"H", GHOST_IOS_KEY_TEXT(GHOST_kKeyH, "h"), "Hide"},
    {"⌥H", GHOST_IOS_KEY_MOD(GHOST_kKeyH, GHOST_IOS_STROKE_ALT), "Reveal"},
    {"Tab", GHOST_IOS_KEY(GHOST_kKeyTab), "Object Mode"},
};

inline constexpr GHOST_IOSBarKey GHOST_IOS_BAR_NODE[] = {
    {"⇧A", GHOST_IOS_KEY_MOD(GHOST_kKeyA, GHOST_IOS_STROKE_SHIFT), "Add"},
    {"G", GHOST_IOS_KEY_TEXT(GHOST_kKeyG, "g"), "Move"},
    {"X", GHOST_IOS_KEY_TEXT(GHOST_kKeyX, "x"), "Delete"},
    {"Tab", GHOST_IOS_KEY(GHOST_kKeyTab), "Edit Group"},
    {"H", GHOST_IOS_KEY_TEXT(GHOST_kKeyH, "h"), "Collapse"},
};

inline constexpr GHOST_IOSBarKey GHOST_IOS_BAR_OTHER[] = {
    {"Tab", GHOST_IOS_KEY(GHOST_kKeyTab), nullptr},
    {"Delete", GHOST_IOS_KEY(GHOST_kKeyDelete), nullptr},
    {"Return", GHOST_IOS_KEY(GHOST_kKeyEnter), "Confirm"},
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
    {"X", GHOST_IOS_KEY_TEXT(GHOST_kKeyX, "x"), nullptr},
    {"Y", GHOST_IOS_KEY_TEXT(GHOST_kKeyY, "y"), nullptr},
    {"Z", GHOST_IOS_KEY_TEXT(GHOST_kKeyZ, "z"), nullptr},
    {"⌫", GHOST_IOS_KEY(GHOST_kKeyBackSpace), nullptr},
    {"7", GHOST_IOS_KEY_TEXT(GHOST_kKey7, "7"), nullptr},
    {"8", GHOST_IOS_KEY_TEXT(GHOST_kKey8, "8"), nullptr},
    {"9", GHOST_IOS_KEY_TEXT(GHOST_kKey9, "9"), nullptr},
    {"−", GHOST_IOS_KEY_TEXT(GHOST_kKeyMinus, "-"), nullptr},
    {"4", GHOST_IOS_KEY_TEXT(GHOST_kKey4, "4"), nullptr},
    {"5", GHOST_IOS_KEY_TEXT(GHOST_kKey5, "5"), nullptr},
    {"6", GHOST_IOS_KEY_TEXT(GHOST_kKey6, "6"), nullptr},
    {".", GHOST_IOS_KEY_TEXT(GHOST_kKeyPeriod, "."), nullptr},
    {"1", GHOST_IOS_KEY_TEXT(GHOST_kKey1, "1"), nullptr},
    {"2", GHOST_IOS_KEY_TEXT(GHOST_kKey2, "2"), nullptr},
    {"3", GHOST_IOS_KEY_TEXT(GHOST_kKey3, "3"), nullptr},
    {"0", GHOST_IOS_KEY_TEXT(GHOST_kKey0, "0"), nullptr},
    {"Esc", GHOST_IOS_KEY(GHOST_kKeyEsc), nullptr},
    {"Return", GHOST_IOS_KEY(GHOST_kKeyEnter), nullptr},
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
