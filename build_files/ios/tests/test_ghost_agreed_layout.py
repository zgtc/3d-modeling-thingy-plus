#!/usr/bin/env python3

from pathlib import Path
import subprocess
import tempfile
import textwrap
import unittest


REPOSITORY = Path(__file__).resolve().parents[3]
GHOST = REPOSITORY / "intern" / "ghost"
GHOST_INTERN = GHOST / "intern"
BELT_SOURCE = GHOST_INTERN / "GHOST_IOSModifierBelt.mm"
HEADER_SOURCE = GHOST_INTERN / "GHOST_IOSHeaderDock.mm"
WINDOW_SOURCE = GHOST_INTERN / "GHOST_WindowIOS.mm"
WM_SHIM = REPOSITORY / "source" / "blender" / "windowmanager" / "intern" / "wm_platform_ios.cc"


def run_harness(test: unittest.TestCase, body: str) -> None:
    """Compile a C++ program against the platform-neutral rail and quick bar headers and run it."""
    harness = textwrap.dedent(
        """
        #include "GHOST_IOSModifierBelt.hh"
        #include "GHOST_IOSSettingsFile.hh"

        #include <cstdio>
        #include <cstdlib>
        #include <cstring>
        #include <string>

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
        temporary = Path(directory)
        source = temporary / "agreed_layout_test.cc"
        executable = temporary / "agreed_layout_test"
        source.write_text(harness)
        compile_result = subprocess.run(
            ["xcrun", "clang++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
             "-I", str(GHOST), "-I", str(GHOST_INTERN),
             "-I", str(REPOSITORY / "intern" / "guardedalloc"),
             str(source), "-o", str(executable)],
            capture_output=True,
            text=True,
        )
        test.assertEqual(compile_result.returncode, 0, compile_result.stderr)
        result = subprocess.run([str(executable)], capture_output=True, text=True)
        test.assertEqual(result.returncode, 0, result.stdout + result.stderr)


class AgreedLayoutTableTests(unittest.TestCase):
    def test_canvas_leaves_room_for_the_rail_and_quick_bar(self) -> None:
        run_harness(
            self,
            """
            double leading = 0.0, bottom = 0.0;
            GHOST_IOSRail_canvasInsets(true, 60.0, &leading, &bottom);
            CHECK(leading == GHOST_IOS_RAIL_WIDTH + 2.0 * GHOST_IOS_RAIL_MARGIN);
            CHECK(bottom == 60.0 + GHOST_IOS_QUICK_BAR_DIVIDER + GHOST_IOS_QUICK_BAR_GAP);
            /* A hardware keyboard hides the quick bar; the rail's tools stay. */
            GHOST_IOSRail_canvasInsets(false, 60.0, &leading, &bottom);
            CHECK(leading > 0.0 && bottom == 0.0);
            /* The divider cannot make the quick bar unusable or take over the screen. */
            CHECK(GHOST_IOSQuickBar_clampHeight(10.0) == GHOST_IOS_QUICK_BAR_MIN_HEIGHT);
            CHECK(GHOST_IOSQuickBar_clampHeight(500.0) == GHOST_IOS_QUICK_BAR_MAX_HEIGHT);
            CHECK(GHOST_IOSQuickBar_clampHeight(80.0) == 80.0);
            CHECK(GHOST_IOS_QUICK_BAR_MIN_HEIGHT >= 44.0);
            """,
        )

    def test_object_mode_quick_keys_match_the_design(self) -> None:
        run_harness(
            self,
            """
            GHOST_IOSInteractionContext context;
            context.editor = GHOST_IOSEditor::View3D;
            context.mode = GHOST_IOSMode::Object;
            int count = 0;
            const GHOST_IOSBarKey *keys = GHOST_IOSCommandBar_keys(context, &count);
            const char *const expected[] = {"Esc", "X", "S", "R", "G", "Tab"};
            CHECK(count == 6);
            for (int i = 0; i < 6; i++) {
              CHECK(std::strcmp(keys[i].title, expected[i]) == 0);
            }
            """,
        )

    def test_every_key_set_starts_with_esc_and_ends_with_tab(self) -> None:
        # Device feedback on #5, item 11: six keys each, Esc first, Tab last, mode keys between.
        run_harness(
            self,
            """
            struct Expected {
              GHOST_IOSEditor editor;
              GHOST_IOSMode mode;
              const char *set;
              const char *titles[6];
            };
            const Expected sets[] = {
                {GHOST_IOSEditor::View3D, GHOST_IOSMode::Object, "object",
                 {"Esc", "X", "S", "R", "G", "Tab"}},
                {GHOST_IOSEditor::View3D, GHOST_IOSMode::Edit, "edit",
                 {"Esc", "E", "X", "R", "G", "Tab"}},
                {GHOST_IOSEditor::View3D, GHOST_IOSMode::Sculpt, "sculpt",
                 {"Esc", "F", "\u21e7F", "H", "\u2325H", "Tab"}},
                {GHOST_IOSEditor::View3D, GHOST_IOSMode::Paint, "paint",
                 {"Esc", "F", "\u21e7F", "X", "E", "Tab"}},
                {GHOST_IOSEditor::Node, GHOST_IOSMode::Other, "node",
                 {"Esc", "\u21e7A", "X", "G", "H", "Tab"}},
            };
            for (const Expected &expected : sets) {
              GHOST_IOSInteractionContext context;
              context.editor = expected.editor;
              context.mode = expected.mode;
              int count = 0;
              const GHOST_IOSBarKey *keys = GHOST_IOSCommandBar_keys(context, &count);
              CHECK(count == 6);
              CHECK(std::strcmp(GHOST_IOSCommandBar_setName(context), expected.set) == 0);
              for (int i = 0; i < 6; i++) {
                CHECK(std::strcmp(keys[i].title, expected.titles[i]) == 0);
              }
            }
            /* The generic fallback keeps its four keys. */
            GHOST_IOSInteractionContext other;
            other.editor = GHOST_IOSEditor::Other;
            int count = 0;
            const GHOST_IOSBarKey *keys = GHOST_IOSCommandBar_keys(other, &count);
            const char *const fallback[] = {"Tab", "Esc", "Delete", "Return"};
            CHECK(count == 4);
            for (int i = 0; i < 4; i++) {
              CHECK(std::strcmp(keys[i].title, fallback[i]) == 0);
            }
            """,
        )

    def test_timeline_starts_minimized_and_the_choice_is_saved(self) -> None:
        run_harness(
            self,
            """
            GHOST_IOSTouchSettings settings;
            CHECK(settings.timeline_minimized);
            GHOST_IOSSettingsFile file;
            file.touch.timeline_minimized = false;
            const std::string json = GHOST_IOSSettingsFile_write(file);
            GHOST_IOSSettingsFile read;
            CHECK(GHOST_IOSSettingsFile_read(json.c_str(), &read));
            CHECK(!read.touch.timeline_minimized);
            char command[GHOST_IOS_COMMAND_MAX];
            CHECK(GHOST_IOSCommand_format(command, "ios.timeline", "minimized", "1"));
            CHECK(std::strcmp(command, "ios.timeline\\nminimized=1") == 0);
            """,
        )


class AgreedLayoutSourceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.belt = BELT_SOURCE.read_text()
        self.window = WINDOW_SOURCE.read_text()
        self.header = HEADER_SOURCE.read_text()
        self.shim = WM_SHIM.read_text()

    def test_rail_is_one_tool_panel_and_modifiers_live_on_the_quick_bar(self) -> None:
        # Device feedback on #5: Shift, Ctrl, Alt and Cmd showed twice, on the rail and the bar.
        self.assertIn('@"blender_rail_tools"', self.belt)
        self.assertNotIn('@"blender_rail_modifiers"', self.belt)
        self.assertNotIn("modifier_panel_", self.belt)
        self.assertNotIn("rail_modifier_keys_", self.belt)
        self.assertIn("- (void)setTools:(const GHOST_IOSToolItem *)tools", self.belt)
        # The tool panel runs the full height of the canvas.
        self.assertIn("[tool_panel_.bottomAnchor constraintEqualToAnchor:input_view.bottomAnchor", self.belt)
        self.assertIn("quick_modifier_keys_ = [GHOST_IOSBelt_modifierKeys(delegate, @\"\") retain];", self.belt)
        # The old single bar's handle, corners and divider line are gone.
        for gone in ("GHOST_IOSModifierBeltCorner", "handle_icon_", "separator_", "UISwipeGestureRecognizer"):
            self.assertNotIn(gone, self.belt)

    def test_quick_bar_runs_along_the_bottom(self) -> None:
        self.assertIn('@"blender_quick_bar"', self.belt)
        self.assertIn("quick_stack_.axis = UILayoutConstraintAxisHorizontal;", self.belt)
        self.assertIn('@"blender_command_bar_settings"', self.belt)
        build = self.belt[self.belt.index("- (void)arrangeQuickBar") :]
        build = build[: build.index("\n}\n")]
        # Modifiers, then the shortcut keys, then the settings button at the end.
        self.assertLess(build.index("quick_modifier_keys_"), build.index("context_keys_"))
        self.assertLess(build.index("context_keys_"), build.index("settings_button_"))

    def test_divider_sets_the_quick_bar_height_and_remembers_it(self) -> None:
        self.assertIn('@"blender_quick_bar_divider"', self.belt)
        drag = self.belt[self.belt.index("- (void)handleDivider:(UIPanGestureRecognizer *)sender") :]
        drag = drag[: drag.index("\n}\n")]
        self.assertIn("GHOST_IOSQuickBar_clampHeight(", drag)
        self.assertIn("GHOST_IOSQuickBarHeightKey", drag)
        self.assertIn("[self updateCanvasInsets];", drag)

    def test_key_pad_opens_above_the_quick_bar(self) -> None:
        pad = self.belt[self.belt.index("- (void)createKeyPadInView:") :]
        pad = pad[: pad.index("\n}\n")]
        self.assertIn("[key_pad_.bottomAnchor constraintEqualToAnchor:divider_.topAnchor", pad)

    def test_tools_moved_from_the_dock_to_the_rail(self) -> None:
        self.assertNotIn("tool_scroll_", self.header)
        self.assertNotIn("blender_tool_dock", self.header)
        self.assertIn("[host_ setCanvasInsetTop:GHOST_IOS_HEADER_HEIGHT];", self.header)
        setter = self.window[self.window.index("- (void)updateCommandBarForContext") :]
        setter = setter[: setter.index("\n}\n")]
        self.assertIn("GHOST_IOSModifierBelt_setTools(", setter)
        tool = self.window[self.window.index("- (void)commandBarTool:(int)index") :]
        tool = tool[: tool.index("\n}\n")]
        self.assertIn("GHOST_IOSTool_command(", tool)
        self.assertIn("[self queueCommand:command];", tool)

    def test_timeline_minimize_control_is_saved_and_runs_in_blender(self) -> None:
        self.assertIn('@"blender_timeline_minimize"', self.window)
        toggle = self.window[self.window.index("- (void)toggleTimeline") :]
        toggle = toggle[: toggle.index("\n}\n")]
        self.assertIn("touch_settings.timeline_minimized = !touch_settings.timeline_minimized;", toggle)
        self.assertIn("GHOST_IOSTouchSettings_save(touch_settings);", toggle)
        self.assertIn("[self applyTimeline];", toggle)
        builtin = self.shim[self.shim.index("static bool wm_ios_run_builtin(") :]
        builtin = builtin[: builtin.index("\n}\n")]
        self.assertIn('STREQ(idname, "ios.timeline")', builtin)
        timeline = self.shim[self.shim.index("static void wm_ios_set_timeline_minimized(") :]
        timeline = timeline[: timeline.index("\n}\n")]
        self.assertIn("wm_ios_bottom_timeline(", timeline)
        finder = self.shim[self.shim.index("static ScrArea *wm_ios_bottom_timeline(") :]
        self.assertIn("SACTCONT_TIMELINE", finder[: finder.index("\n}\n")])
        self.assertIn("ED_area_headersize()", timeline)
        self.assertIn("do_refresh = true", timeline)
        self.assertIn("context.timeline_", self.shim)


if __name__ == "__main__":
    unittest.main()


class HeaderActionTests(unittest.TestCase):
    """Item 4 of the device feedback on #5: Undo, Redo, Save and Search in the top bar, beside
    the menu button and the workspace tabs."""

    def test_header_actions_run_blenders_own_operators(self) -> None:
        run_harness(
            self,
            """
            const char *const titles[] = {"Undo", "Redo", "Save", "Search"};
            const char *const commands[] = {"ed.undo", "ed.redo", "wm.save_mainfile",
                                            "wm.search_menu"};
            CHECK(GHOST_IOS_ARRAY_LEN(GHOST_IOS_HEADER_ACTIONS) == 4);
            for (int i = 0; i < 4; i++) {
              const GHOST_IOSHeaderAction &action = GHOST_IOS_HEADER_ACTIONS[i];
              CHECK(std::strcmp(action.title, titles[i]) == 0);
              char command[GHOST_IOS_COMMAND_MAX];
              CHECK(GHOST_IOSHeaderAction_command(action, command));
              CHECK(std::strcmp(command, commands[i]) == 0);
              CHECK(action.symbol != nullptr && action.identifier != nullptr);
            }
            """,
        )

    def test_header_shows_the_actions_after_the_tabs(self) -> None:
        header = HEADER_SOURCE.read_text()
        self.assertIn("for (const GHOST_IOSHeaderAction &action : GHOST_IOS_HEADER_ACTIONS)", header)
        self.assertIn("- (void)handleAction:(UIButton *)sender", header)
        # The menu button and tabs stay; the tabs now end where the actions begin.
        self.assertIn('@"blender_native_menu"', header)
        self.assertIn('@"blender_workspace_tabs"', header)
        self.assertIn("[tab_scroll_.trailingAnchor constraintEqualToAnchor:action_stack_.leadingAnchor",
                      header)
        self.assertIn("[action_stack_.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor",
                      header)
        handle = header[header.index("- (void)handleAction:(UIButton *)sender") :]
        handle = handle[: handle.index("\n}\n")]
        self.assertIn("GHOST_IOSHeaderAction_command(", handle)
        self.assertIn("[self queue:command];", handle)
