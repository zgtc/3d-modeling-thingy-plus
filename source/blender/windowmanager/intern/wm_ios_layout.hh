/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup wm
 *
 * Sizes for the iPad's factory layout and touch spacing: how wide the Outliner and Properties
 * column on the right is, how much of it the Outliner gets, and how far apart buttons, rows and
 * panels are with Touch Spacing on. Kept free of Blender's runtime so it can be tested on its own.
 */

#pragma once

#include <algorithm>

/** Spacing in #uiStyle, saved before the touch preset changes it so it can be restored exactly. */
struct WMIOSStyleSpacing {
  bool saved = false;
  short columnspace = 0, templatespace = 0, boxspace = 0, buttonspacex = 0, buttonspacey = 0,
        panelspace = 0, panelouter = 0;
  float widget_points = 0.0f, grouplabel_points = 0.0f, paneltitle_points = 0.0f;
};

/**
 * Touch spacing: twice the room beside buttons so a finger lands on the intended one, and
 * slightly larger text. Rows and panels only gain a little, so the Properties editor does not
 * spread out vertically. With \a enabled false, \a defaults come back unchanged.
 */
inline WMIOSStyleSpacing wm_ios_touch_spacing(const WMIOSStyleSpacing &defaults,
                                              const bool enabled)
{
  if (!enabled) {
    return defaults;
  }
  WMIOSStyleSpacing touch = defaults;
  touch.columnspace = short(defaults.columnspace * 2);
  touch.buttonspacex = short(defaults.buttonspacex * 2);
  touch.buttonspacey = short(defaults.buttonspacey + 2);
  touch.templatespace = short(defaults.templatespace * 5 / 4);
  touch.boxspace = short(defaults.boxspace * 5 / 4);
  touch.panelspace = short(defaults.panelspace * 5 / 4);
  touch.panelouter = short(defaults.panelouter * 3 / 2);
  touch.widget_points = defaults.widget_points * 1.15f;
  touch.grouplabel_points = defaults.grouplabel_points * 1.15f;
  touch.paneltitle_points = defaults.paneltitle_points * 1.15f;
  return touch;
}

/**
 * The new left edge of the Outliner and Properties column, whose right edge is \a screen_right:
 * a third wider, at least 32% of the screen, never more than 45% so the viewport stays the main
 * area. A column already wider is left alone.
 */
inline int wm_ios_right_column_left(const int screen_right, const int column_left)
{
  const int width = screen_right - column_left;
  const int wanted = std::max(width * 4 / 3, screen_right * 32 / 100);
  const int new_left = std::max(screen_right - wanted, screen_right - screen_right * 45 / 100);
  return std::min(new_left, column_left);
}

/**
 * The new bottom edge of the Outliner at the top of the column from \a column_top down to
 * \a column_bottom: it gets at least two fifths of the column, the Properties editor the rest.
 */
inline int wm_ios_outliner_bottom(const int column_top,
                                  const int column_bottom,
                                  const int outliner_bottom)
{
  const int wanted = column_top - (column_top - column_bottom) * 2 / 5;
  return std::min(outliner_bottom, wanted);
}
