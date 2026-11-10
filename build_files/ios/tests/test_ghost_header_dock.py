#!/usr/bin/env python3

from pathlib import Path
import re
import subprocess
import tempfile
import textwrap
import unittest


REPOSITORY = Path(__file__).resolve().parents[3]
GHOST = REPOSITORY / "intern" / "ghost"
GHOST_INTERN = GHOST / "intern"
CHROME = GHOST_INTERN / "GHOST_IOSChrome.hh"
VIEWS = GHOST_INTERN / "GHOST_IOSHeaderDock.mm"
WINDOW_SOURCE = GHOST_INTERN / "GHOST_WindowIOS.mm"
BELT_SOURCE = GHOST_INTERN / "GHOST_IOSModifierBelt.mm"
PALETTE_SOURCE = GHOST_INTERN / "GHOST_IOSPencilPalette.mm"
WM_SHIM = REPOSITORY / "source" / "blender" / "windowmanager" / "intern" / "wm_platform_ios.cc"
GHOST_CMAKE = REPOSITORY / "build_files" / "ios" / "cmake" / "ghost_ios.cmake"
TOOLBAR = REPOSITORY / "scripts" / "startup" / "bl_ui" / "space_toolsystem_toolbar.py"
DOCS = REPOSITORY / "docs-site" / "src" / "content" / "docs" / "controls" / "touch.md"


def run_harness(test: unittest.TestCase, body: str) -> None:
    """Compile a C++ program against the platform-neutral header and dock tables and run it."""
    harness = textwrap.dedent(
        """
        #include "GHOST_IOSChrome.hh"

        #include <cstdio>
        #include <cstdlib>
        #include <cstring>

        #define CHECK(expr) \\
          do { \\
            if (!(expr)) { \\
              std::fprintf(stderr, "%s:%d: CHECK(%s) failed\\n", __FILE__, __LINE__, #expr); \\
              std::exit(1); \\
            } \\
          } while (0)

        [[maybe_unused]] static GHOST_IOSInteractionContext context(GHOST_IOSEditor editor,
                                                                    GHOST_IOSMode mode)
        {
          GHOST_IOSInteractionContext c;
          c.editor = editor;
          c.mode = mode;
          return c;
        }

        int main()
        {
        """
    ) + textwrap.indent(textwrap.dedent(body), "  ") + "\n  return 0;\n}\n"

    with tempfile.TemporaryDirectory() as directory:
        temporary = Path(directory)
        source = temporary / "header_dock_test.cc"
        executable = temporary / "header_dock_test"
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


class HeaderTableTests(unittest.TestCase):
    def test_workspace_tabs_switch_by_name_and_compare_in_context(self) -> None:
        run_harness(
            self,
            """
            char command[GHOST_IOS_COMMAND_MAX];
            CHECK(GHOST_IOSWorkspace_command("Sculpting", command));
            CHECK(std::strcmp(command, "ios.workspace\\nname=Sculpting") == 0);
            CHECK(!GHOST_IOSWorkspace_command("", command));

            GHOST_IOSInteractionContext a, b;
            std::strcpy(a.workspaces[0], "Layout");
            std::strcpy(a.workspaces[1], "Modeling");
            a.workspaces_num = 2;
            a.active_workspace = 0;
            b = a;
            CHECK(a == b && a.sameWorkspaces(b));
            b.active_workspace = 1;
            CHECK(a != b);
            b = a;
            std::strcpy(b.workspaces[1], "Shading");
            CHECK(!a.sameWorkspaces(b));
            """,
        )


class DockTableTests(unittest.TestCase):
    def test_dock_tools_follow_the_viewport_mode(self) -> None:
        run_harness(
            self,
            """
            const auto V = GHOST_IOSEditor::View3D;
            int count = 0;
            const GHOST_IOSToolItem *object = GHOST_IOSDock_tools(context(V, GHOST_IOSMode::Object),
                                                                  &count);
            CHECK(object != nullptr && count >= 8);
            CHECK(GHOST_IOSDock_activeIndex(object, count, "builtin.move") == 2);
            CHECK(GHOST_IOSDock_activeIndex(object, count, "builtin.knife") == -1);
            const GHOST_IOSToolItem *edit = GHOST_IOSDock_tools(context(V, GHOST_IOSMode::Edit),
                                                                &count);
            CHECK(edit != object && GHOST_IOSDock_activeIndex(edit, count, "builtin.knife") >= 0);
            const GHOST_IOSToolItem *sculpt = GHOST_IOSDock_tools(
                context(V, GHOST_IOSMode::Sculpt), &count);
            CHECK(std::strcmp(sculpt[0].idname, "builtin.brush") == 0);
            CHECK(GHOST_IOSDock_tools(context(V, GHOST_IOSMode::Paint), &count) == nullptr &&
                  count == 0);
            CHECK(GHOST_IOSDock_tools(context(GHOST_IOSEditor::Node, GHOST_IOSMode::Object),
                                      &count) == nullptr);

            char command[GHOST_IOS_COMMAND_MAX];
            CHECK(GHOST_IOSTool_command(object[2], command));
            CHECK(std::strcmp(command, "wm.tool_set_by_id\\nname=builtin.move") == 0);
            """,
        )

    def test_palette_tools_page_holds_the_first_tools(self) -> None:
        run_harness(
            self,
            """
            GHOST_IOSPaletteSlot slots[GHOST_IOS_PALETTE_SLOT_COUNT];
            char commands[GHOST_IOS_PALETTE_SLOT_COUNT][GHOST_IOS_COMMAND_MAX];
            const int count = GHOST_IOSPalette_toolSlots(
                context(GHOST_IOSEditor::View3D, GHOST_IOSMode::Edit), slots, commands);
            CHECK(count == GHOST_IOS_PALETTE_SLOT_COUNT);
            CHECK(slots[5].kind == GHOST_IOSSlotKind::Command);
            CHECK(std::strcmp(slots[5].command, "wm.tool_set_by_id\\nname=builtin.extrude_region") ==
                  0);
            CHECK(std::strcmp(slots[5].title, "Extrude") == 0);
            CHECK(GHOST_IOSPalette_toolSlots(
                      context(GHOST_IOSEditor::Node, GHOST_IOSMode::Other), slots, commands) == 0);
            """,
        )

    def test_every_dock_tool_exists_in_blenders_toolbar(self) -> None:
        tools = set(re.findall(r'"(builtin\.\w+)"', CHROME.read_text()))
        self.assertGreater(len(tools), 20)
        toolbar = TOOLBAR.read_text()
        for tool in tools:
            self.assertIn(f'idname="{tool}"', toolbar, tool)


class HeaderDockSourceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.views = VIEWS.read_text()
        self.window = WINDOW_SOURCE.read_text()
        self.shim = WM_SHIM.read_text()

    def test_build_lists_the_new_sources(self) -> None:
        cmake = GHOST_CMAKE.read_text()
        for name in ("GHOST_IOSChrome.hh", "GHOST_IOSHeaderDock.hh", "GHOST_IOSHeaderDock.mm"):
            self.assertIn(f"intern/{name}", cmake)

    def test_header_has_one_menu_button_and_scrolling_tabs(self) -> None:
        self.assertIn("menu_button_.showsMenuAsPrimaryAction = YES;", self.views)
        self.assertIn("UIDeferredMenuElement", self.views)
        self.assertIn("GHOST_IOSMenuBar_topMenus(", self.views)
        # No mode switch: the 3D Viewport's own mode menu already does this (design review).
        self.assertNotIn("UISegmentedControl", self.views)
        self.assertNotIn("GHOST_IOSModeSwitch", CHROME.read_text())
        tabs = self.views[self.views.index("[tab_scroll_.leadingAnchor") :]
        tabs = tabs[: tabs.index("],")]
        self.assertIn("menu_button_.trailingAnchor", tabs)
        self.assertIn("tab_scroll_.showsHorizontalScrollIndicator = NO;", self.views)
        self.assertIn("scrollRectToVisible:", self.views)
        self.assertIn("[host_ setCanvasInsetTop:GHOST_IOS_HEADER_HEIGHT];", self.views)
        self.assertNotIn("UIGlassEffect", self.views)
        self.assertNotIn("contentEdgeInsets", self.views)

    def test_header_and_dock_are_ipad_only_and_main_window_only(self) -> None:
        create = self.window[self.window.index("header_dock = GHOST_IOSHeaderDock_create(") - 400 :]
        create = create[: create.index("}\n  }") + 4]
        self.assertIn("window->isMainWindow()", create)
        self.assertIn("UIUserInterfaceIdiomPad", create)
        self.assertIn('"ios.native_chrome", "enabled", "1"', create)
        setter = self.window[self.window.index("- (void)setInteractionContext:(const GHOST_IOSInteractionContext &)context\n{") :]
        setter = setter[: setter.index("\n}\n")]
        self.assertIn("GHOST_IOSHeaderDock_setContext(header_dock, context);", setter)

    def test_shim_hides_blenders_top_bar_and_tool_shelf_and_switches_workspaces(self) -> None:
        topbar = self.shim[self.shim.index("static void wm_ios_hide_topbar(") :]
        topbar = topbar[: topbar.index("\n}\n")]
        self.assertIn("SPACE_TOPBAR", topbar)
        self.assertIn("GLOBAL_AREA_IS_HIDDEN", topbar)
        self.assertIn("do_refresh = true", topbar)
        shelves = self.shim[self.shim.index("static void wm_ios_hide_tool_shelves(") :]
        shelves = shelves[: shelves.index("\n}\n")]
        self.assertIn("RGN_TYPE_TOOLS", shelves)
        self.assertIn("SPACE_VIEW3D", shelves)
        builtin = self.shim[self.shim.index("static bool wm_ios_run_builtin(") :]
        builtin = builtin[: builtin.index("\n}\n")]
        self.assertIn('STREQ(idname, "ios.workspace")', builtin)
        self.assertIn("WM_window_set_active_workspace(C, win, workspace);", builtin)
        # The tool shelf is hidden once, when the dock appears, not on every pass.
        self.assertIn("if (enabled && !wm_ios_native_chrome)", builtin)
        update = self.shim[self.shim.index("void wm_ios_touch_channel_update(bContext *C)") :]
        update = update[: update.index("\n}\n")]
        self.assertIn("wm_ios_hide_topbar(win);", update)
        self.assertIn("context.workspaces", self.shim)

    def test_canvas_and_rail_make_room_for_header_and_dock(self) -> None:
        layout = self.window[self.window.index("@implementation GHOSTIOSCanvasContainer") :]
        layout = layout[layout.index("- (void)layoutSubviews") :]
        layout = layout[: layout.index("\n}\n")]
        self.assertIn("self.safeAreaInsets.top", layout)
        self.assertIn("self.safeAreaInsets.bottom", layout)
        self.assertLess(layout.index("canvas_.frame ="), layout.index("[super layoutSubviews];"))
        belt = BELT_SOURCE.read_text()
        self.assertIn("UILayoutGuide *safe = host_.safeAreaLayoutGuide;", belt)
        self.assertIn("[tool_panel_.topAnchor constraintEqualToAnchor:input_view.topAnchor", belt)
        self.assertIn("[quick_bar_.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor", belt)

    def test_squeeze_palette_has_a_tools_page(self) -> None:
        palette = PALETTE_SOURCE.read_text()
        self.assertIn("pencilPaletteSwitchPage", palette)
        switch = self.window[self.window.index("- (void)pencilPaletteSwitchPage") :]
        switch = switch[: switch.index("\n}\n")]
        self.assertIn("palette_tools_page = !palette_tools_page;", switch)
        page = self.window[self.window.index("- (void)showPalettePage") :]
        page = page[: page.index("\n}\n")]
        self.assertIn("GHOST_IOSPalette_toolSlots(", page)

    def test_guide_describes_the_header_and_dock(self) -> None:
        touch = DOCS.read_text()
        self.assertIn("## Header and tool dock", touch)


if __name__ == "__main__":
    unittest.main()
