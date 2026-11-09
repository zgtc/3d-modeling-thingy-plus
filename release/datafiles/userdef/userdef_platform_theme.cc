/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup blenloader
 *
 * Platform adjustments applied on top of #U_theme_default.
 *
 * The iOS build defines `BLENDER_PLATFORM_IOS_THEME` so the factory theme follows the iOS dark
 * appearance: system grays, the system blue accent, rounded capsule controls, grouped panels
 * on a black background and softer, deeper menu shadows. Colors that carry modeling meaning
 * (selection, active object, axes, keyframes, node sockets) keep Blender's values so tutorials
 * and muscle memory still apply. The stock look stays one click away through the bundled
 * "Blender Dark" preset.
 */

#include "BLO_userdef_default.h"

#include "DNA_theme_types.h"

namespace blender {

#ifdef BLENDER_PLATFORM_IOS_THEME

/** iOS dark appearance palette (UIKit dynamic colors resolved for dark mode). */
enum : unsigned int {
  IOS_BACKGROUND = 0x000000ff,       /* systemBackground */
  IOS_GROUPED_CELL = 0x1c1c1eff,     /* secondarySystemBackground */
  IOS_ELEVATED = 0x2c2c2eff,         /* tertiarySystemBackground, systemGray5 */
  IOS_FILL = 0x3a3a3cff,             /* systemGray4 */
  IOS_FILL_RAISED = 0x48484aff,      /* systemGray3 */
  IOS_SECONDARY_FILL = 0x636366ff,   /* systemGray2 */
  IOS_LABEL = 0xffffffff,            /* label */
  IOS_SECONDARY_LABEL = 0xebebf599,  /* secondaryLabel */
  IOS_TERTIARY_LABEL = 0xebebf54d,   /* tertiaryLabel */
  IOS_BLUE = 0x0a84ffff,             /* systemBlue */
};

static void set_rgba(unsigned char dst[4], const unsigned int c)
{
  dst[0] = (c >> 24) & 0xff;
  dst[1] = (c >> 16) & 0xff;
  dst[2] = (c >> 8) & 0xff;
  dst[3] = c & 0xff;
}

/** Same color with a new alpha, for translucent regions that overlap the viewport. */
static unsigned int with_alpha(const unsigned int c, const unsigned int alpha)
{
  return (c & 0xffffff00u) | (alpha & 0xffu);
}

static void style_widget(uiWidgetColors &wcol,
                         const unsigned int inner,
                         const unsigned int inner_sel,
                         const float roundness)
{
  set_rgba(wcol.outline, IOS_BACKGROUND & 0xffffff00u);
  set_rgba(wcol.outline_sel, IOS_BACKGROUND & 0xffffff00u);
  set_rgba(wcol.inner, inner);
  set_rgba(wcol.inner_sel, inner_sel);
  set_rgba(wcol.text, IOS_LABEL);
  set_rgba(wcol.text_sel, IOS_LABEL);
  /* iOS controls are flat fills, not gradients. */
  wcol.shaded = 0;
  wcol.shadetop = 0;
  wcol.shadedown = 0;
  wcol.roundness = roundness;
}

static void style_space(ThemeSpace &space, const unsigned int back, const unsigned int header)
{
  set_rgba(space.back, back);
  set_rgba(space.header, header);
  set_rgba(space.title, IOS_LABEL);
  set_rgba(space.text, IOS_LABEL);
  set_rgba(space.text_hi, IOS_LABEL);
  set_rgba(space.header_text, IOS_LABEL);
  set_rgba(space.header_text_hi, IOS_LABEL);
}

void BLO_userdef_theme_apply_platform(bTheme *btheme)
{
  ThemeUI &tui = btheme->tui;

  /* Buttons and fields: capsule-like controls on gray fills, blue when on. */
  style_widget(tui.wcol_regular, IOS_FILL, IOS_BLUE, 0.5f);
  style_widget(tui.wcol_tool, IOS_FILL, IOS_BLUE, 0.5f);
  style_widget(tui.wcol_toolbar_item, IOS_ELEVATED, IOS_BLUE, 0.45f);
  style_widget(tui.wcol_radio, IOS_FILL, IOS_BLUE, 0.5f);
  style_widget(tui.wcol_option, IOS_FILL, IOS_BLUE, 0.5f);
  style_widget(tui.wcol_toggle, IOS_FILL, IOS_BLUE, 0.5f);
  style_widget(tui.wcol_num, IOS_FILL, IOS_FILL_RAISED, 0.5f);
  style_widget(tui.wcol_numslider, IOS_FILL, IOS_FILL_RAISED, 0.5f);
  style_widget(tui.wcol_text, IOS_GROUPED_CELL, IOS_ELEVATED, 0.5f);
  style_widget(tui.wcol_tab, IOS_GROUPED_CELL, IOS_FILL, 0.5f);
  style_widget(tui.wcol_menu, IOS_FILL, IOS_FILL_RAISED, 0.5f);
  style_widget(tui.wcol_pulldown, IOS_BACKGROUND & 0xffffff00u, IOS_FILL, 0.5f);
  style_widget(tui.wcol_box, with_alpha(IOS_GROUPED_CELL, 0x80), IOS_ELEVATED, 0.4f);
  style_widget(tui.wcol_list_item, IOS_BACKGROUND & 0xffffff00u, with_alpha(IOS_BLUE, 0x66), 0.4f);
  style_widget(tui.wcol_progress, IOS_FILL, IOS_FILL, 1.0f);
  style_widget(tui.wcol_scroll, IOS_BACKGROUND & 0xffffff00u, IOS_SECONDARY_FILL, 1.0f);

  /* Menus and popovers: translucent elevated surfaces with large rounded corners. */
  style_widget(tui.wcol_menu_back, with_alpha(IOS_GROUPED_CELL, 0xf5), IOS_ELEVATED, 0.35f);
  style_widget(tui.wcol_menu_item, IOS_GROUPED_CELL & 0xffffff00u, IOS_BLUE, 0.4f);
  style_widget(tui.wcol_pie_menu, with_alpha(IOS_GROUPED_CELL, 0xf5), IOS_BLUE, 0.5f);
  style_widget(tui.wcol_tooltip, with_alpha(IOS_ELEVATED, 0xf5), IOS_ELEVATED, 0.5f);

  /* Accent items: slider fill, check marks, progress fill and menu arrows. */
  set_rgba(tui.wcol_num.item, IOS_BLUE);
  set_rgba(tui.wcol_numslider.item, IOS_BLUE);
  set_rgba(tui.wcol_progress.item, IOS_BLUE);
  set_rgba(tui.wcol_option.item, IOS_LABEL);
  set_rgba(tui.wcol_radio.item, IOS_LABEL);
  set_rgba(tui.wcol_toggle.item, IOS_LABEL);
  set_rgba(tui.wcol_menu.item, IOS_SECONDARY_LABEL);
  set_rgba(tui.wcol_menu_item.item, IOS_SECONDARY_LABEL);
  set_rgba(tui.wcol_pulldown.item, IOS_SECONDARY_LABEL);
  set_rgba(tui.wcol_menu_back.item, IOS_SECONDARY_LABEL);
  set_rgba(tui.wcol_tooltip.item, IOS_SECONDARY_LABEL);
  set_rgba(tui.wcol_list_item.item, IOS_TERTIARY_LABEL);
  set_rgba(tui.wcol_text.item, IOS_TERTIARY_LABEL);
  set_rgba(tui.wcol_tab.item, IOS_GROUPED_CELL);
  set_rgba(tui.wcol_pie_menu.item, IOS_FILL);

  set_rgba(tui.widget_emboss, 0x00000000);
  set_rgba(tui.widget_text_cursor, IOS_BLUE);
  tui.menu_shadow_fac = 0.35f;
  tui.menu_shadow_width = 12;

  /* Editors sit on black with hairline separators, like split views. */
  set_rgba(tui.editor_border, IOS_BACKGROUND);
  set_rgba(tui.editor_outline, 0xffffff12);
  set_rgba(tui.editor_outline_active, with_alpha(IOS_BLUE, 0x80));

  /* Panels read as inset grouped cells. */
  tui.panel_roundness = 0.6f;
  set_rgba(tui.panel_header, IOS_GROUPED_CELL);
  set_rgba(tui.panel_back, IOS_GROUPED_CELL);
  set_rgba(tui.panel_sub_back, 0x0000001f);
  set_rgba(tui.panel_outline, 0x00000000);
  set_rgba(tui.panel_title, IOS_LABEL);
  set_rgba(tui.panel_text, IOS_LABEL);
  set_rgba(tui.panel_active, IOS_BLUE);

  /* Translucent region backgrounds overlap content, like bars over a scroll view. */
  const unsigned int bar = with_alpha(IOS_GROUPED_CELL, 0xcc);
  const unsigned int clear_black = IOS_BACKGROUND & 0xffffff00u;
  style_space(btheme->space_properties, clear_black, bar);
  style_space(btheme->space_file, clear_black, bar);
  style_space(btheme->space_graph, with_alpha(IOS_GROUPED_CELL, 0x00), bar);
  style_space(btheme->space_info, clear_black, bar);
  style_space(btheme->space_action, with_alpha(IOS_GROUPED_CELL, 0x00), bar);
  style_space(btheme->space_nla, with_alpha(IOS_GROUPED_CELL, 0x00), bar);
  style_space(btheme->space_image, with_alpha(IOS_GROUPED_CELL, 0x00), bar);
  style_space(btheme->space_text, with_alpha(IOS_GROUPED_CELL, 0x00), bar);
  style_space(btheme->space_outliner, with_alpha(IOS_GROUPED_CELL, 0x00), bar);
  style_space(btheme->space_preferences, clear_black, bar);
  style_space(btheme->space_console, clear_black, bar);
  style_space(btheme->space_clip, with_alpha(IOS_GROUPED_CELL, 0x00), bar);
  style_space(btheme->space_spreadsheet, with_alpha(IOS_GROUPED_CELL, 0x00), bar);
  style_space(btheme->space_topbar, clear_black, IOS_BACKGROUND);
  style_space(btheme->space_statusbar, clear_black, IOS_BACKGROUND);
  /* The 3D viewport, sequencer and node editor keep their canvas colors and only restyle the
   * header, since their backgrounds carry contrast for geometry, strips and node wires. */
  set_rgba(btheme->space_view3d.header, bar);
  set_rgba(btheme->space_sequencer.header, bar);
  set_rgba(btheme->space_node.header, with_alpha(IOS_BACKGROUND, 0xcc));

  set_rgba(btheme->space_file.hilite, IOS_BLUE);
  set_rgba(btheme->space_properties.match, IOS_BLUE);
  set_rgba(btheme->space_outliner.active, with_alpha(IOS_BLUE, 0x66));
  set_rgba(btheme->space_spreadsheet.active, with_alpha(IOS_BLUE, 0x66));

  set_rgba(btheme->regions.sidebars.back, clear_black);
  set_rgba(btheme->regions.sidebars.tab_back, clear_black);
  set_rgba(btheme->regions.asset_shelf.back, with_alpha(IOS_GROUPED_CELL, 0xe6));
  set_rgba(btheme->regions.asset_shelf.header_back, IOS_GROUPED_CELL);
  set_rgba(btheme->regions.channels.back, IOS_GROUPED_CELL);
  set_rgba(btheme->regions.scrubbing.back, IOS_GROUPED_CELL);

  set_rgba(btheme->common.anim.playhead, IOS_BLUE);
}

#else

void BLO_userdef_theme_apply_platform(bTheme * /*btheme*/) {}

#endif

}  // namespace blender
