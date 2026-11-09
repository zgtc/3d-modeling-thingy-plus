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
WINDOW_SOURCE = GHOST_INTERN / "GHOST_WindowIOS.mm"
SYSTEM_SOURCE = GHOST_INTERN / "GHOST_SystemIOS.mm"
BELT_SOURCE = GHOST_INTERN / "GHOST_IOSModifierBelt.mm"
HANDLES_SOURCE = GHOST_INTERN / "GHOST_IOSResizeHandles.mm"
MENU_SOURCE = GHOST_INTERN / "GHOST_IOSMenuBar.mm"
MENU_ITEMS = GHOST_INTERN / "GHOST_IOSMenuItems.hh"
SYNC_SOURCE = GHOST_INTERN / "GHOST_IOSSettingsSync.mm"
PALETTE_SOURCE = GHOST_INTERN / "GHOST_IOSPencilPalette.mm"
WM_SHIM = REPOSITORY / "source" / "blender" / "windowmanager" / "intern" / "wm_platform_ios.cc"
GHOST_CMAKE = REPOSITORY / "build_files" / "ios" / "cmake" / "ghost_ios.cmake"
STARTUP_UI = REPOSITORY / "scripts" / "startup" / "bl_ui"
DOCS = REPOSITORY / "docs-site" / "src" / "content" / "docs" / "controls"


def run_harness(test: unittest.TestCase, body: str) -> None:
    """Compile a C++ program against the platform-neutral panel and menu headers and run it."""
    harness = textwrap.dedent(
        """
        #include "GHOST_IOSMenuItems.hh"
        #include "GHOST_IOSResizeHandles.hh"
        #include "GHOST_IOSSettingsFile.hh"

        #include <cmath>
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
        source = temporary / "panels_menus_test.cc"
        executable = temporary / "panels_menus_test"
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


class SettingsFileTests(unittest.TestCase):
    def test_settings_round_trip_through_json(self) -> None:
        run_harness(
            self,
            """
            GHOST_IOSSettingsFile file;
            file.saved_at = 1794260000.5;
            file.touch.double_tap = GHOST_IOSDoubleTapChoice::Palette;
            file.touch.squeeze = GHOST_IOSSqueezeChoice::Off;
            file.touch.pencil_only_drawing = true;
            file.touch.touch_spacing = true;
            file.touch.tap_gestures[2] = GHOST_IOSGestureAction::Focus;
            CHECK(file.setCustomAction("palette.edit.3", "mesh.subdivide"));
            CHECK(file.setCustomAction("bar.object.5", "Shift+D"));
            CHECK(file.setCustomAction("bar.node.0", "say \\"hi\\"\\\\ there"));

            const std::string json = GHOST_IOSSettingsFile_write(file);
            GHOST_IOSSettingsFile read;
            CHECK(GHOST_IOSSettingsFile_read(json.c_str(), &read));
            CHECK(read.version == GHOST_IOS_SETTINGS_VERSION);
            CHECK(std::fabs(read.saved_at - 1794260000.5) < 1e-3);
            CHECK(read.touch.double_tap == GHOST_IOSDoubleTapChoice::Palette);
            CHECK(read.touch.squeeze == GHOST_IOSSqueezeChoice::Off);
            CHECK(read.touch.pencil_only_drawing && read.touch.touch_spacing);
            CHECK(read.touch.tap_gestures[0] == GHOST_IOSGestureAction::Undo);
            CHECK(read.touch.tap_gestures[2] == GHOST_IOSGestureAction::Focus);
            CHECK(read.custom_num == 3);
            CHECK(std::strcmp(read.customAction("palette.edit.3"), "mesh.subdivide") == 0);
            CHECK(std::strcmp(read.customAction("bar.object.5"), "Shift+D") == 0);
            CHECK(std::strcmp(read.customAction("bar.node.0"), "say \\"hi\\"\\\\ there") == 0);
            """,
        )

    def test_newer_or_damaged_files_are_read_safely(self) -> None:
        run_harness(
            self,
            """
            /* A newer schema: unknown fields of every type are skipped, known ones are read. */
            const char *newer =
                "{\\"version\\": 3, \\"saved_at\\": 10, \\"future\\": {\\"a\\": [1, {\\"b\\": null}]},"
                " \\"double_tap\\": 2, \\"flags\\": [true, false], \\"note\\": \\"x\\","
                " \\"tap_gestures\\": [0, 1, 2, 3, 4, 9], \\"custom\\": {}}";
            GHOST_IOSSettingsFile file;
            CHECK(GHOST_IOSSettingsFile_read(newer, &file));
            CHECK(file.version == 3);
            CHECK(file.touch.double_tap == GHOST_IOSDoubleTapChoice::PreviousTool);
            CHECK(file.touch.tap_gestures[0] == GHOST_IOSGestureAction::None);
            CHECK(file.touch.tap_gestures[2] == GHOST_IOSGestureAction::Redo);
            /* Missing fields keep their defaults. */
            CHECK(file.touch.squeeze == GHOST_IOSSqueezeChoice::System);

            GHOST_IOSSettingsFile untouched;
            untouched.touch.double_tap = GHOST_IOSDoubleTapChoice::Eraser;
            CHECK(!GHOST_IOSSettingsFile_read("{\\"double_tap\\": ", &untouched));
            CHECK(!GHOST_IOSSettingsFile_read("not json", &untouched));
            CHECK(untouched.touch.double_tap == GHOST_IOSDoubleTapChoice::Eraser);
            """,
        )

    def test_customizations_replace_and_reset(self) -> None:
        run_harness(
            self,
            """
            GHOST_IOSSettingsFile file;
            CHECK(file.setCustomAction("palette.object.0", "Shift+D"));
            CHECK(file.setCustomAction("palette.object.0", "object.join"));
            CHECK(file.custom_num == 1);
            CHECK(std::strcmp(file.customAction("palette.object.0"), "object.join") == 0);
            CHECK(file.setCustomAction("palette.object.0", ""));
            CHECK(file.custom_num == 0 && file.customAction("palette.object.0") == nullptr);
            /* Resetting a key that was never replaced is fine. */
            CHECK(file.setCustomAction("bar.edit.1", ""));
            char long_action[GHOST_IOS_CUSTOM_ACTION_MAX + 4];
            std::memset(long_action, 'a', sizeof(long_action) - 1);
            long_action[sizeof(long_action) - 1] = '\\0';
            CHECK(!file.setCustomAction("bar.edit.1", long_action));
            """,
        )


class CustomActionTests(unittest.TestCase):
    def test_keys_with_modifiers(self) -> None:
        run_harness(
            self,
            """
            GHOST_IOSCustomAction action;
            CHECK(GHOST_IOSCustomAction_parse("Shift+D", &action));
            CHECK(action.kind == GHOST_IOSCustomKind::Key);
            CHECK(action.stroke.key == GHOST_kKeyD);
            CHECK(action.stroke.modifiers == GHOST_IOS_STROKE_SHIFT);
            CHECK(GHOST_IOSCustomAction_parse("ctrl+shift+z", &action));
            CHECK(action.stroke.key == GHOST_kKeyZ);
            CHECK(action.stroke.modifiers == (GHOST_IOS_STROKE_CTRL | GHOST_IOS_STROKE_SHIFT));
            CHECK(GHOST_IOSCustomAction_parse("Cmd+Option+3", &action));
            CHECK(action.stroke.key == GHOST_kKey3);
            CHECK(action.stroke.modifiers == (GHOST_IOS_STROKE_OS | GHOST_IOS_STROKE_ALT));
            CHECK(GHOST_IOSCustomAction_parse(" F3 ", &action) && action.stroke.key == GHOST_kKeyF3);
            CHECK(GHOST_IOSCustomAction_parse("Tab", &action) && action.stroke.key == GHOST_kKeyTab);
            CHECK(GHOST_IOSCustomAction_parse("Numpad7", &action) &&
                  action.stroke.key == GHOST_kKeyNumpad7);
            CHECK(GHOST_IOSCustomAction_parse("k", &action));
            CHECK(action.stroke.utf8 != nullptr && std::strcmp(action.stroke.utf8, "k") == 0);
            CHECK(GHOST_IOSCustomAction_parse("7", &action));
            CHECK(std::strcmp(action.stroke.utf8, "7") == 0);
            """,
        )

    def test_operators_and_menus(self) -> None:
        run_harness(
            self,
            """
            GHOST_IOSCustomAction action;
            CHECK(GHOST_IOSCustomAction_parse("mesh.subdivide", &action));
            CHECK(action.kind == GHOST_IOSCustomKind::Command);
            CHECK(std::strcmp(action.command, "mesh.subdivide") == 0);
            CHECK(GHOST_IOSCustomAction_parse("object.shade_smooth keep_sharp_edges=1", &action));
            CHECK(std::strcmp(action.command, "object.shade_smooth\\nkeep_sharp_edges=1") == 0);
            CHECK(GHOST_IOSCustomAction_parse("VIEW3D_MT_add", &action));
            CHECK(std::strcmp(action.command, "wm.call_menu\\nname=VIEW3D_MT_add") == 0);
            CHECK(GHOST_IOSCustomAction_parse("VIEW3D_MT_pivot_pie", &action));
            CHECK(std::strcmp(action.command, "wm.call_menu_pie\\nname=VIEW3D_MT_pivot_pie") == 0);
            """,
        )

    def test_rejects_anything_else(self) -> None:
        run_harness(
            self,
            """
            GHOST_IOSCustomAction action;
            for (const char *text : {"", "   ", "Shift+", "Hyper+D", "Shift+Banana", "Mesh.Subdivide",
                                     ".subdivide", "mesh.", "mesh.subdivide a b", "mesh.x =1",
                                     "rm -rf"}) {
              CHECK(!GHOST_IOSCustomAction_parse(text, &action));
            }
            """,
        )

    def test_replacements_apply_to_the_palette_and_bar(self) -> None:
        run_harness(
            self,
            """
            GHOST_IOSSettingsFile file;
            file.setCustomAction("palette.edit.3", "mesh.subdivide");
            file.setCustomAction("palette.edit.4", "Ctrl+B");
            file.setCustomAction("palette.edit.5", "not a valid action");
            file.setCustomAction("bar.object.1", "VIEW3D_MT_add");
            const auto edit = context(GHOST_IOSEditor::View3D, GHOST_IOSMode::Edit);

            GHOST_IOSCustomPalette palette;
            palette.build(edit, file);
            CHECK(std::strcmp(palette.set, "edit") == 0);
            CHECK(palette.slots[3].kind == GHOST_IOSSlotKind::Command);
            CHECK(std::strcmp(palette.slots[3].command, "mesh.subdivide") == 0);
            CHECK(std::strcmp(palette.slots[3].title, "subdivide") == 0);
            CHECK(palette.slots[4].kind == GHOST_IOSSlotKind::Key);
            CHECK(palette.slots[4].stroke.key == GHOST_kKeyB);
            CHECK(palette.slots[4].stroke.modifiers == GHOST_IOS_STROKE_CTRL);
            /* Unparsable replacements keep the default slot. */
            CHECK(std::strcmp(palette.slots[5].title, "Loop Cut") == 0);
            CHECK(std::strcmp(palette.slots[0].title, "Vertex") == 0);

            GHOST_IOSCustomBar bar;
            bar.build(context(GHOST_IOSEditor::View3D, GHOST_IOSMode::Object), file);
            CHECK(std::strcmp(bar.set, "object") == 0);
            CHECK(bar.count == 6);
            CHECK(std::strcmp(bar.keys[1].command, "wm.call_menu\\nname=VIEW3D_MT_add") == 0);
            CHECK(bar.keys[0].command == nullptr && std::strcmp(bar.keys[0].title, "Tab") == 0);
            /* Each editor keeps its own set. */
            bar.build(context(GHOST_IOSEditor::Node, GHOST_IOSMode::Other), file);
            CHECK(std::strcmp(bar.set, "node") == 0);
            CHECK(bar.keys[1].command == nullptr);
            """,
        )


class PanelGeometryTests(unittest.TestCase):
    def test_targeted_commands_carry_the_swipe_point(self) -> None:
        run_harness(
            self,
            """
            char command[GHOST_IOS_COMMAND_MAX];
            CHECK(GHOST_IOSCommand_formatAt(command, 40, 812, "wm.context_toggle", "data_path",
                                            "space_data.show_region_toolbar"));
            CHECK(std::strcmp(command,
                              "wm.context_toggle\\n@40,812\\ndata_path=space_data.show_region_toolbar") ==
                  0);
            CHECK(GHOST_IOSCommand_formatAt(command, 1, 2, "screen.region_quadview"));
            CHECK(std::strcmp(command, "screen.region_quadview\\n@1,2") == 0);
            """,
        )

    def test_grab_handles_are_control_sized_and_centered_on_the_edge(self) -> None:
        run_harness(
            self,
            """
            CHECK(GHOST_IOS_RESIZE_HANDLE_HEIGHT == 44.0);
            GHOST_IOSResizeEdge edge;
            edge.x = 600;
            edge.y_min = 100;
            edge.y_max = 900;
            GHOST_IOSHandleFrame frame;
            CHECK(GHOST_IOSResizeHandle_frame(edge, 2.0, &frame));
            CHECK(frame.width == GHOST_IOS_RESIZE_HANDLE_WIDTH);
            CHECK(frame.height == GHOST_IOS_RESIZE_HANDLE_HEIGHT);
            CHECK(std::fabs(frame.x + frame.width * 0.5 - 300.0) < 1e-9);
            CHECK(std::fabs(frame.y + frame.height * 0.5 - 250.0) < 1e-9);
            int x = 0, y = 0;
            GHOST_IOSResizeHandle_pressPoint(edge, &x, &y);
            CHECK(x == 600 && y == 500);

            /* Too short for a handle, and a bad scale. */
            edge.y_max = 200;
            CHECK(!GHOST_IOSResizeHandle_frame(edge, 2.0, &frame));
            CHECK(!GHOST_IOSResizeHandle_frame(edge, 0.0, &frame));
            """,
        )

    def test_context_compares_resize_edges(self) -> None:
        run_harness(
            self,
            """
            GHOST_IOSInteractionContext a, b;
            CHECK(a == b);
            b.resize_edges_num = 1;
            b.resize_edges[0].x = 10;
            CHECK(a != b);
            a = b;
            CHECK(a == b);
            a.resize_edges[0].region_on_right = true;
            CHECK(!a.sameResizeEdges(b));
            """,
        )


class MenuBarTableTests(unittest.TestCase):
    def test_every_item_is_a_well_formed_operator_call(self) -> None:
        run_harness(
            self,
            """
            const GHOST_IOSMenu *menus = nullptr;
            const int top = GHOST_IOSMenuBar_topMenus(&menus);
            CHECK(top == 5);
            CHECK(std::strcmp(menus[0].title, "File") == 0);
            CHECK(std::strcmp(menus[4].title, "Help") == 0);
            for (int i = 0; i < top; i++) {
              CHECK(menus[i].count > 0);
              for (int j = 0; j < menus[i].count; j++) {
                char command[GHOST_IOS_COMMAND_MAX];
                CHECK(GHOST_IOSMenuItem_command(menus[i].items[j], command));
                CHECK(std::strchr(menus[i].items[j].idname, '.') != nullptr);
              }
            }
            """,
        )

    def test_editor_menus_follow_the_mode_and_rebuild_only_on_change(self) -> None:
        run_harness(
            self,
            """
            const GHOST_IOSMenu *menus = nullptr;
            const auto V = GHOST_IOSEditor::View3D;
            CHECK(GHOST_IOSMenuBar_editorMenus(context(V, GHOST_IOSMode::Object), &menus) == 2);
            CHECK(std::strcmp(menus[0].title, "Viewport") == 0);
            CHECK(std::strcmp(menus[1].title, "Object") == 0);
            CHECK(GHOST_IOSMenuBar_editorMenus(context(V, GHOST_IOSMode::Edit), &menus) == 2);
            CHECK(std::strcmp(menus[1].title, "Mesh") == 0);
            CHECK(GHOST_IOSMenuBar_editorMenus(context(V, GHOST_IOSMode::Sculpt), &menus) == 2);
            CHECK(std::strcmp(menus[1].title, "Sculpt") == 0);
            CHECK(GHOST_IOSMenuBar_editorMenus(context(V, GHOST_IOSMode::Paint), &menus) == 1);
            CHECK(GHOST_IOSMenuBar_editorMenus(
                      context(GHOST_IOSEditor::Node, GHOST_IOSMode::Other), &menus) == 0);

            auto object = context(V, GHOST_IOSMode::Object);
            auto moved = object;
            moved.over_main_region = true;
            std::strcpy(moved.tool, "builtin.move");
            CHECK(!GHOST_IOSMenuBar_needsRebuild(object, moved));
            CHECK(GHOST_IOSMenuBar_needsRebuild(object, context(V, GHOST_IOSMode::Edit)));
            CHECK(GHOST_IOSMenuBar_needsRebuild(
                object, context(GHOST_IOSEditor::Node, GHOST_IOSMode::Other)));
            """,
        )

    def test_menus_named_by_the_tables_exist_in_blender(self) -> None:
        tables = MENU_ITEMS.read_text() + (GHOST_INTERN / "GHOST_IOSTouchKeys.hh").read_text()
        names = set(re.findall(r'"((?:VIEW3D|TOPBAR)_MT_\w+)"', tables))
        self.assertTrue(names)
        ui = "".join(path.read_text() for path in STARTUP_UI.glob("*.py"))
        for name in names:
            self.assertIn(f"class {name}(", ui, name)


class PanelSourceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.window = WINDOW_SOURCE.read_text()
        self.shim = WM_SHIM.read_text()

    def test_build_lists_the_new_sources(self) -> None:
        cmake = GHOST_CMAKE.read_text()
        for name in ("GHOST_IOSMenuBar.mm", "GHOST_IOSMenuItems.hh", "GHOST_IOSResizeHandles.mm",
                     "GHOST_IOSSettingsFile.hh", "GHOST_IOSSettingsSync.mm"):
            self.assertIn(f"intern/{name}", cmake)

    def test_edge_swipes_toggle_regions_of_the_editor_under_the_swipe(self) -> None:
        swipe = self.window[self.window.index("- (void)handleEdgeSwipe:(UIScreenEdgePanGestureRecognizer *)gesture\n{") :]
        swipe = swipe[: swipe.index("\n}\n")]
        self.assertIn('"space_data.show_region_toolbar"', swipe)
        self.assertIn('"space_data.show_region_ui"', swipe)
        self.assertIn("GHOST_IOSCommand_formatAt(", swipe)
        self.assertIn('"wm.context_toggle"', swipe)
        self.assertNotIn("GHOST_EventTouch", swipe)

    def test_shim_targets_the_point_and_runs_builtin_commands(self) -> None:
        run = self.shim[self.shim.index("static void wm_ios_run_command(") :]
        run = run[: run.index("\n}\n")]
        self.assertIn("rest[0] == '@'", run)
        self.assertIn("wm_cursor_position_from_ghost_client_coords(&win, &x, &y)", run)
        self.assertIn('STRPREFIX(idname, "ios.")', run)
        builtin = self.shim[self.shim.index("static bool wm_ios_run_builtin(") :]
        builtin = builtin[: builtin.index("\n}\n")]
        self.assertIn("WM_window_open_temp(C, IFACE_(\"Properties\"), SPACE_PROPERTIES", builtin)
        self.assertIn("WM_window_open_temp(C, IFACE_(\"Outliner\"), SPACE_OUTLINER", builtin)

    def test_touch_spacing_restores_the_exact_defaults(self) -> None:
        spacing = self.shim[self.shim.index("static void wm_ios_set_touch_spacing(") :]
        spacing = spacing[: spacing.index("\n}\n")]
        self.assertIn("if (!spacing.saved)", spacing)
        self.assertIn("enabled ? 2.0f : 1.0f", spacing)
        self.assertIn("style->buttonspacey", spacing)
        self.assertNotIn("U.ui_scale", spacing)

    def test_resize_edges_match_blenders_region_resize_zones(self) -> None:
        edges = self.shim[self.shim.index("static void wm_ios_resize_edges(") :]
        edges = edges[: edges.index("\n}\n")]
        self.assertIn("RGN_TYPE_UI, RGN_TYPE_TOOLS", edges)
        self.assertIn("align == RGN_ALIGN_LEFT ? region.winrct.xmax : region.winrct.xmin", edges)
        self.assertIn("RGN_FLAG_HIDDEN | RGN_FLAG_TOO_SMALL", edges)
        self.assertIn("wm_cursor_position_to_ghost_client_coords", edges)
        area_cc = (REPOSITORY / "source" / "blender" / "editors" / "screen" / "area.cc").read_text()
        self.assertIn("az->x1 = region->winrct.xmin - pad_out;", area_cc)
        self.assertIn("az->x2 = region->winrct.xmax + pad_out;", area_cc)

    def test_grab_handle_drags_are_mouse_drags_on_the_edge(self) -> None:
        press = self.window[self.window.index("- (void)resizeHandlePressAt:") :]
        press = press[: press.index("\n}\n")]
        self.assertIn("virtual_pointer->button(GHOST_kButtonMaskLeft, true);", press)
        release = self.window[self.window.index("- (void)resizeHandleReleaseAt:") :]
        release = release[: release.index("\n}\n")]
        self.assertIn("virtual_pointer->button(GHOST_kButtonMaskLeft, false);", release)
        handles = HANDLES_SOURCE.read_text()
        self.assertIn("press_.minimumPressDuration = 0.0;", handles)
        # The press point is fixed for the drag, so a moving edge cannot make it overshoot.
        changed = handles[handles.index("case UIGestureRecognizerStateChanged:") :]
        changed = changed[: changed.index("break;")]
        self.assertIn("press_client_.x + dx", changed)
        should_receive = self.window[self.window.index("       shouldReceiveTouch:(UITouch *)touch\n{") :]
        should_receive = should_receive[: should_receive.index("\n}\n")]
        self.assertIn("GHOST_IOSResizeHandles_containsView(resize_handles, touch.view)",
                      should_receive)

    def test_command_bar_buttons_and_focus(self) -> None:
        belt = BELT_SOURCE.read_text()
        for identifier in ("blender_command_bar_focus", "blender_command_bar_properties",
                           "blender_command_bar_outliner"):
            self.assertIn(f'@"{identifier}"', belt)
        focus = self.window[self.window.index("- (void)toggleFocusMode") :]
        focus = focus[: focus.index("\n}\n")]
        self.assertIn('"screen.screen_full_area", "use_hide_panels", "1"', focus)
        gestures = self.window[self.window.index("- (void)runTapGesture:") :]
        self.assertIn("case GHOST_IOSGestureAction::Focus:", gestures)


class MenuBarAndSyncSourceTests(unittest.TestCase):
    def test_menu_bar_is_built_by_the_app_delegate_without_key_equivalents(self) -> None:
        system = SYSTEM_SOURCE.read_text()
        build = system[system.index("- (void)buildMenuWithBuilder:(id<UIMenuBuilder>)builder") :]
        build = build[: build.index("\n}\n")]
        self.assertIn("[super buildMenuWithBuilder:builder];", build)
        self.assertIn("builder.system == UIMenuSystem.mainSystem", build)
        self.assertIn("GHOST_IOSMenuBar_build(builder);", build)
        menu = MENU_SOURCE.read_text()
        self.assertNotIn("UIKeyCommand", menu)
        self.assertNotIn("keyInput", menu)
        self.assertIn("setNeedsRebuild", menu)
        window = WINDOW_SOURCE.read_text()
        handler = window[window.index("- (void)blenderMenuCommand:(UICommand *)sender") :]
        handler = handler[: handler.index("\n}\n")]
        self.assertIn("[self queueCommand:command];", handler)

    def test_sync_uses_a_picked_folder_not_the_icloud_entitlement(self) -> None:
        sync = SYNC_SOURCE.read_text()
        self.assertNotIn("URLForUbiquityContainerIdentifier", sync)
        self.assertNotIn("NSMetadataQuery", sync)
        self.assertIn("UTTypeFolder", sync)
        self.assertIn("startAccessingSecurityScopedResource", sync)
        self.assertIn("NSFileCoordinator", sync)
        self.assertIn("UIApplicationDidBecomeActiveNotification", sync)
        merge = sync[sync.index("static bool GHOST_IOSSettings_mergeSyncFolder()") :]
        merge = merge[: merge.index("\n}\n")]
        self.assertIn("remote.saved_at > local.saved_at", merge)
        self.assertIn('@"touch-settings.json"', sync)
        # Settings from before the file existed are kept on first launch.
        self.assertIn("GHOST_IOSSettings_legacy()", sync)
        palette = PALETTE_SOURCE.read_text()
        self.assertNotIn("NSUserDefaults", palette)

    def test_user_guide_covers_the_new_controls(self) -> None:
        touch = (DOCS / "touch.md").read_text()
        pencil = (DOCS / "keyboard-mouse-pencil.md").read_text()
        for heading in ("## Command bar", "### Typing values without a keyboard",
                        "### Customizing keys and the Pencil palette", "## Panels",
                        "## Menu bar", "## Settings sync"):
            self.assertIn(heading, touch)
        for heading in ("### Squeeze palette", "### Double tap",
                        "### Only Draw with Apple Pencil"):
            self.assertIn(heading, pencil)
        self.assertNotIn("#shift-ctrl-and-alt", pencil)
        self.assertIn("/controls/touch/#command-bar", pencil)


if __name__ == "__main__":
    unittest.main()
