/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup wm
 *
 * The keyboard shortcuts of a running modal tool, turned into keys for the iPad's quick bar.
 * The status line lists these with mouse buttons, which are of no use on a touch screen; the
 * quick bar shows the keyboard ones as keys to tap instead. Kept free of Blender's runtime so it
 * can be tested on its own.
 */

#pragma once

#include "GHOST_IOSInteraction.hh"
#include "GHOST_Types.hh"

#include "wm_event_types.hh"

#include <cstdio>
#include <cstring>

namespace blender {

/** The GHOST key for a Blender keyboard event, or #GHOST_kKeyUnknown for anything else. */
inline GHOST_TKey wm_ios_ghost_key(const int type)
{
  if (type >= EVT_AKEY && type <= EVT_ZKEY) {
    return GHOST_TKey(GHOST_kKeyA + (type - EVT_AKEY));
  }
  if (type >= EVT_ZEROKEY && type <= EVT_NINEKEY) {
    return GHOST_TKey(GHOST_kKey0 + (type - EVT_ZEROKEY));
  }
  if (type >= EVT_PAD0 && type <= EVT_PAD9) {
    return GHOST_TKey(GHOST_kKeyNumpad0 + (type - EVT_PAD0));
  }
  if (type >= EVT_F1KEY && type <= EVT_F24KEY) {
    return GHOST_TKey(GHOST_kKeyF1 + (type - EVT_F1KEY));
  }
  switch (type) {
    case EVT_ESCKEY:
      return GHOST_kKeyEsc;
    case EVT_RETKEY:
      return GHOST_kKeyEnter;
    case EVT_PADENTER:
      return GHOST_kKeyNumpadEnter;
    case EVT_TABKEY:
      return GHOST_kKeyTab;
    case EVT_SPACEKEY:
      return GHOST_kKeySpace;
    case EVT_BACKSPACEKEY:
      return GHOST_kKeyBackSpace;
    case EVT_DELKEY:
      return GHOST_kKeyDelete;
    case EVT_INSERTKEY:
      return GHOST_kKeyInsert;
    case EVT_HOMEKEY:
      return GHOST_kKeyHome;
    case EVT_ENDKEY:
      return GHOST_kKeyEnd;
    case EVT_PAGEUPKEY:
      return GHOST_kKeyUpPage;
    case EVT_PAGEDOWNKEY:
      return GHOST_kKeyDownPage;
    case EVT_LEFTARROWKEY:
      return GHOST_kKeyLeftArrow;
    case EVT_RIGHTARROWKEY:
      return GHOST_kKeyRightArrow;
    case EVT_UPARROWKEY:
      return GHOST_kKeyUpArrow;
    case EVT_DOWNARROWKEY:
      return GHOST_kKeyDownArrow;
    case EVT_MINUSKEY:
      return GHOST_kKeyMinus;
    case EVT_EQUALKEY:
      return GHOST_kKeyEqual;
    case EVT_COMMAKEY:
      return GHOST_kKeyComma;
    case EVT_PERIODKEY:
      return GHOST_kKeyPeriod;
    case EVT_PADPERIOD:
      return GHOST_kKeyNumpadPeriod;
    case EVT_PADPLUSKEY:
      return GHOST_kKeyNumpadPlus;
    case EVT_PADMINUS:
      return GHOST_kKeyNumpadMinus;
    case EVT_PADASTERKEY:
      return GHOST_kKeyNumpadAsterisk;
    case EVT_PADSLASHKEY:
      return GHOST_kKeyNumpadSlash;
    default:
      /* Mouse buttons, and Shift, Ctrl, Alt and Cmd, which are on the quick bar already. */
      return GHOST_kKeyUnknown;
  }
}

/**
 * Add the shortcut for modal item \a name, bound to event \a type with the given modifiers, to
 * the context's quick-bar keys. Mouse bindings, items already listed and keys past
 * #GHOST_IOS_MODAL_KEY_MAX are left out. Returns true when the key was added.
 */
inline bool wm_ios_modal_key_add(GHOST_IOSInteractionContext &context,
                                 const char *name,
                                 const int type,
                                 const bool shift,
                                 const bool ctrl,
                                 const bool alt,
                                 const bool oskey)
{
  if (name == nullptr || name[0] == '\0' || context.modal_keys_num >= GHOST_IOS_MODAL_KEY_MAX) {
    return false;
  }
  const GHOST_TKey key = wm_ios_ghost_key(type);
  if (key == GHOST_kKeyUnknown) {
    return false;
  }
  for (int i = 0; i < context.modal_keys_num; i++) {
    if (std::strcmp(context.modal_keys[i].title, name) == 0) {
      return false;
    }
  }
  GHOST_IOSModalKey &entry = context.modal_keys[context.modal_keys_num++];
  std::snprintf(entry.title, sizeof(entry.title), "%s", name);
  entry.key = int(key);
  entry.modifiers = uint8_t((shift ? GHOST_IOS_MODAL_KEY_SHIFT : 0) |
                            (ctrl ? GHOST_IOS_MODAL_KEY_CTRL : 0) |
                            (alt ? GHOST_IOS_MODAL_KEY_ALT : 0) |
                            (oskey ? GHOST_IOS_MODAL_KEY_OS : 0));
  return true;
}

}  // namespace blender
