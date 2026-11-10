#!/usr/bin/env python3

from pathlib import Path
import subprocess
import tempfile
import textwrap
import unittest


REPOSITORY = Path(__file__).resolve().parents[3]
WM_INTERN = REPOSITORY / "source" / "blender" / "windowmanager" / "intern"
LAYOUT_HEADER = WM_INTERN / "wm_ios_layout.hh"
SHIM = WM_INTERN / "wm_platform_ios.cc"
INTERFACE_LAYOUT = REPOSITORY / "source" / "blender" / "editors" / "interface" / "interface_layout.cc"
PLATFORM_CMAKE = REPOSITORY / "build_files" / "ios" / "cmake" / "platform_ios.cmake"


def run_harness(test: unittest.TestCase, body: str) -> None:
    harness = textwrap.dedent(
        """
        #include "wm_ios_layout.hh"
        #include <cstdio>
        #include <cstdlib>

        #define CHECK(expr) \\
          do { \\
            if (!(expr)) { \\
              std::fprintf(stderr, "%s:%d: CHECK(%s) failed\\n", __FILE__, __LINE__, #expr); \\
              std::exit(1); \\
            } \\
          } while (0)

        int main()
        {
        """
    ) + textwrap.indent(textwrap.dedent(body), "  ") + "\n  return 0;\n}\n"
    with tempfile.TemporaryDirectory() as directory:
        source = Path(directory) / "layout_test.cc"
        executable = Path(directory) / "layout_test"
        source.write_text(harness)
        result = subprocess.run(
            ["xcrun", "clang++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
             "-I", str(WM_INTERN), str(source), "-o", str(executable)],
            capture_output=True, text=True)
        test.assertEqual(result.returncode, 0, result.stderr)
        result = subprocess.run([str(executable)], capture_output=True, text=True)
        test.assertEqual(result.returncode, 0, result.stdout + result.stderr)


class RightColumnTests(unittest.TestCase):
    """Device feedback on #5: the Outliner was cramped, Properties labels were cut off and the
    right column had too much vertical spacing."""

    def test_right_column_is_a_third_of_the_screen_at_most_45_percent(self) -> None:
        run_harness(self, """
            /* Blender's default column, about a fifth of a 2000 px screen, grows to 32%. */
            CHECK(wm_ios_right_column_left(2000, 1600) == 2000 - 640);
            /* An already wide column grows by a third, but never past 45%. */
            CHECK(wm_ios_right_column_left(2000, 1300) == 2000 - 900);
            /* A column wider than that is left alone. */
            CHECK(wm_ios_right_column_left(2000, 1000) == 1000);
        """)

    def test_outliner_gets_at_least_two_fifths_of_the_column(self) -> None:
        run_harness(self, """
            /* Column from y=1000 (top) to y=0: the Outliner's bottom edge moves down to 600. */
            CHECK(wm_ios_outliner_bottom(1000, 0, 800) == 600);
            /* A taller Outliner keeps its size. */
            CHECK(wm_ios_outliner_bottom(1000, 0, 500) == 500);
        """)

    def test_touch_spacing_keeps_rows_tight_and_restores_defaults(self) -> None:
        run_harness(self, """
            WMIOSStyleSpacing defaults;
            defaults.saved = true;
            defaults.columnspace = 8;
            defaults.templatespace = 5;
            defaults.boxspace = 5;
            defaults.buttonspacex = 8;
            defaults.buttonspacey = 2;
            defaults.panelspace = 8;
            defaults.panelouter = 4;
            defaults.widget_points = 11.0f;
            defaults.grouplabel_points = 12.0f;
            defaults.paneltitle_points = 12.0f;
            const WMIOSStyleSpacing touch = wm_ios_touch_spacing(defaults, true);
            /* Horizontal room doubles so fingers land on the intended button... */
            CHECK(touch.columnspace == 16 && touch.buttonspacex == 16);
            /* ...but rows and panels stay close: at most 1.5x, plus 2 px between rows. */
            CHECK(touch.buttonspacey <= defaults.buttonspacey + 2);
            CHECK(touch.panelspace <= defaults.panelspace * 3 / 2);
            CHECK(touch.panelouter <= defaults.panelouter * 3 / 2);
            CHECK(touch.boxspace <= defaults.boxspace * 3 / 2);
            CHECK(touch.templatespace <= defaults.templatespace * 3 / 2);
            CHECK(touch.widget_points > defaults.widget_points);
            const WMIOSStyleSpacing off = wm_ios_touch_spacing(defaults, false);
            CHECK(off.buttonspacey == defaults.buttonspacey && off.panelspace == defaults.panelspace &&
                  off.columnspace == defaults.columnspace && off.widget_points == defaults.widget_points);
        """)

    def test_properties_labels_get_half_the_row_on_ios(self) -> None:
        self.assertIn("-DBLENDER_PLATFORM_PROP_SEP_DIVIDE=0.5f", PLATFORM_CMAKE.read_text())
        layout = INTERFACE_LAYOUT.read_text()
        self.assertIn("#ifdef BLENDER_PLATFORM_PROP_SEP_DIVIDE\n"
                      "#  define UI_ITEM_PROP_SEP_DIVIDE BLENDER_PLATFORM_PROP_SEP_DIVIDE\n"
                      "#else\n"
                      "#  define UI_ITEM_PROP_SEP_DIVIDE 0.4f\n"
                      "#endif", layout)

    def test_shim_uses_the_layout_rules(self) -> None:
        shim = SHIM.read_text()
        self.assertIn('#include "wm_ios_layout.hh"', shim)
        column = shim[shim.index("static void wm_ios_widen_right_column(") :]
        column = column[: column.index("\n}\n")]
        self.assertIn("wm_ios_right_column_left(", column)
        outliner = shim[shim.index("static void wm_ios_grow_outliner(") :]
        outliner = outliner[: outliner.index("\n}\n")]
        self.assertIn("SPACE_OUTLINER", outliner)
        self.assertIn("wm_ios_outliner_bottom(", outliner)
        update = shim[shim.index("void wm_ios_update_startup_defaults(") :]
        update = update[: update.index("\n}\n")]
        self.assertLess(update.index("wm_ios_widen_right_column("), update.index("wm_ios_grow_outliner("))
        spacing = shim[shim.index("static void wm_ios_set_touch_spacing(") :]
        spacing = spacing[: spacing.index("\n}\n")]
        self.assertIn("wm_ios_touch_spacing(spacing, enabled)", spacing)


if __name__ == "__main__":
    unittest.main()


class OutlinerHighlightTests(unittest.TestCase):
    """Device feedback on #5: the selected object should have an outline and the active item a
    colour, so both read at a glance on a touch screen without hover."""

    OUTLINER_DRAW = (REPOSITORY / "source" / "blender" / "editors" / "space_outliner"
                     / "outliner_draw.cc")

    def test_selected_rows_are_outlined_and_the_active_row_is_filled(self) -> None:
        self.assertIn("-DBLENDER_PLATFORM_TOUCH_OUTLINER", PLATFORM_CMAKE.read_text())
        source = self.OUTLINER_DRAW.read_text()
        body = source[source.index("static void outliner_draw_highlights(const ARegion *region,") :]
        body = body[: body.index("\n}\n")]
        self.assertIn("#ifdef BLENDER_PLATFORM_TOUCH_OUTLINER", body)
        touch = body[body.index("#ifdef BLENDER_PLATFORM_TOUCH_OUTLINER") : body.index("#else")]
        selected = touch[touch.index("else if (tselem->flag & TSE_SELECTED)") :]
        # Selected: an outline only (no fill), in the active colour, two pixels wide.
        self.assertIn("const float outline_width = 2.0f * U.pixelsize;", touch)
        self.assertIn("&rect, nullptr, nullptr, 1.0f, col_active_outline, outline_width, radius);",
                      selected)
        active = touch[: touch.index("else if (tselem->flag & TSE_SELECTED)")]
        # Active: a solid colour fill with the same outline.
        self.assertIn("&rect, col_active, nullptr, 1.0f, col_active_outline, outline_width, radius);",
                      active)
