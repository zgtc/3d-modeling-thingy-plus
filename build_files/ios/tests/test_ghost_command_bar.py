#!/usr/bin/env python3

from pathlib import Path
import subprocess
import tempfile
import textwrap
import unittest


REPOSITORY = Path(__file__).resolve().parents[3]
GHOST = REPOSITORY / "intern" / "ghost"
GHOST_INTERN = GHOST / "intern"
BELT_HEADER = GHOST_INTERN / "GHOST_IOSModifierBelt.hh"
BELT_SOURCE = GHOST_INTERN / "GHOST_IOSModifierBelt.mm"
WINDOW_SOURCE = GHOST_INTERN / "GHOST_WindowIOS.mm"
INTERACTION_HEADER = GHOST / "GHOST_IOSInteraction.hh"
WM_SOURCE = REPOSITORY / "source" / "blender" / "windowmanager" / "intern" / "wm.cc"
WM_SHIM = REPOSITORY / "source" / "blender" / "windowmanager" / "intern" / "wm_platform_ios.cc"
WM_CMAKE = REPOSITORY / "build_files" / "ios" / "cmake" / "windowmanager_ios.cmake"


def run_harness(test: unittest.TestCase, body: str) -> None:
    """Compile a C++ program against the platform-neutral command bar headers and run it."""
    harness = textwrap.dedent(
        """
        #include "GHOST_IOSModifierBelt.hh"
        #include "GHOST_IOSModifierLatch.hh"
        #include "GHOST_IOSTouchKeys.hh"

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

        using Mod = GHOST_IOSModifier;
        using Mode = GHOST_IOSModifierMode;

        [[maybe_unused]] static GHOST_IOSInteractionContext context(
            GHOST_IOSEditor editor, GHOST_IOSMode mode, const char *modal = "")
        {
          GHOST_IOSInteractionContext c;
          c.editor = editor;
          c.mode = mode;
          std::strcpy(c.modal_operator, modal);
          return c;
        }

        [[maybe_unused]] static bool has_title(const GHOST_IOSBarKey *keys, int count,
                                               const char *title)
        {
          for (int i = 0; i < count; i++) {
            if (std::strcmp(keys[i].title, title) == 0) {
              return true;
            }
          }
          return false;
        }

        int main()
        {
        """
    ) + textwrap.indent(textwrap.dedent(body), "  ") + "\n  return 0;\n}\n"

    with tempfile.TemporaryDirectory() as directory:
        temporary = Path(directory)
        source = temporary / "command_bar_test.cc"
        executable = temporary / "command_bar_test"
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


class CommandBarKeyTests(unittest.TestCase):
    def test_context_keys_follow_the_editor_and_mode(self) -> None:
        run_harness(
            self,
            """
            const auto V = GHOST_IOSEditor::View3D;
            int count = 0;
            const GHOST_IOSBarKey *object = GHOST_IOSCommandBar_keys(
                context(V, GHOST_IOSMode::Object), &count);
            CHECK(count > 0 && count <= GHOST_IOS_BAR_KEY_MAX);
            CHECK(std::strcmp(object[0].title, "Tab") == 0);
            CHECK(has_title(object, count, "G") && has_title(object, count, "R") &&
                  has_title(object, count, "S") && has_title(object, count, "X"));

            const GHOST_IOSBarKey *edit = GHOST_IOSCommandBar_keys(
                context(V, GHOST_IOSMode::Edit), &count);
            CHECK(count <= GHOST_IOS_BAR_KEY_MAX);
            CHECK(has_title(edit, count, "E"));

            const GHOST_IOSBarKey *sculpt = GHOST_IOSCommandBar_keys(
                context(V, GHOST_IOSMode::Sculpt), &count);
            CHECK(count <= GHOST_IOS_BAR_KEY_MAX && has_title(sculpt, count, "F"));

            const GHOST_IOSBarKey *node = GHOST_IOSCommandBar_keys(
                context(GHOST_IOSEditor::Node, GHOST_IOSMode::Other), &count);
            CHECK(count <= GHOST_IOS_BAR_KEY_MAX && has_title(node, count, "Add"));

            const GHOST_IOSBarKey *other = GHOST_IOSCommandBar_keys(
                context(GHOST_IOSEditor::Other, GHOST_IOSMode::Object), &count);
            CHECK(count <= GHOST_IOS_BAR_KEY_MAX);
            CHECK(has_title(other, count, "Esc") && has_title(other, count, "Delete"));
            CHECK(object != edit && edit != sculpt && sculpt != node && node != other);
            """,
        )

    def test_letter_keys_type_text_but_shortcuts_do_not(self) -> None:
        run_harness(
            self,
            """
            int count = 0;
            const GHOST_IOSBarKey *object = GHOST_IOSCommandBar_keys(
                context(GHOST_IOSEditor::View3D, GHOST_IOSMode::Object), &count);
            for (int i = 0; i < count; i++) {
              CHECK(object[i].stroke.key != GHOST_kKeyUnknown);
              if (object[i].stroke.key == GHOST_kKeyG) {
                CHECK(std::strcmp(object[i].stroke.utf8, "g") == 0);
              }
              if (object[i].stroke.key == GHOST_kKeyTab) {
                CHECK(object[i].stroke.utf8 == nullptr);
              }
            }
            """,
        )

    def test_key_pad_types_axes_and_numbers(self) -> None:
        run_harness(
            self,
            """
            CHECK(GHOST_IOS_KEY_PAD_COLUMNS == 4);
            const char *const expected[] = {"X", "Y", "Z", "0", "1", "2", "3", "4", "5", "6",
                                            "7", "8", "9", ".", "Esc", "Return"};
            for (const char *title : expected) {
              CHECK(has_title(GHOST_IOS_KEY_PAD, GHOST_IOS_KEY_PAD_COUNT, title));
            }
            for (int i = 0; i < GHOST_IOS_KEY_PAD_COUNT; i++) {
              const GHOST_IOSKeyStroke &stroke = GHOST_IOS_KEY_PAD[i].stroke;
              CHECK(stroke.modifiers == 0);
              /* Digits must type, so number input works in both simple and advanced mode. */
              if (stroke.key >= GHOST_kKey0 && stroke.key <= GHOST_kKey9) {
                CHECK(stroke.utf8 != nullptr && stroke.utf8[0] == char('0' + (stroke.key -
                                                                                GHOST_kKey0)));
              }
            }
            """,
        )

    def test_key_pad_shows_only_for_operators_that_take_typed_values(self) -> None:
        run_harness(
            self,
            """
            const auto V = GHOST_IOSEditor::View3D;
            const auto O = GHOST_IOSMode::Object;
            CHECK(!GHOST_IOSCommandBar_wantsKeyPad(context(V, O)));
            CHECK(GHOST_IOSCommandBar_wantsKeyPad(context(V, O, "TRANSFORM_OT_translate")));
            CHECK(GHOST_IOSCommandBar_wantsKeyPad(context(V, O, "TRANSFORM_OT_resize")));
            CHECK(GHOST_IOSCommandBar_wantsKeyPad(context(V, O, "MESH_OT_extrude_region_move")));
            CHECK(GHOST_IOSCommandBar_wantsKeyPad(context(V, O, "OBJECT_OT_duplicate_move")));
            CHECK(GHOST_IOSCommandBar_wantsKeyPad(context(V, O, "MESH_OT_bevel")));
            CHECK(GHOST_IOSCommandBar_wantsKeyPad(context(V, O, "MESH_OT_inset")));
            /* Navigation and brush strokes take no typed values. */
            CHECK(!GHOST_IOSCommandBar_wantsKeyPad(context(V, O, "VIEW3D_OT_rotate")));
            CHECK(!GHOST_IOSCommandBar_wantsKeyPad(context(V, O, "SCULPT_OT_brush_stroke")));
            CHECK(!GHOST_IOSCommandBar_wantsKeyPad(context(V, O, "WM_OT_radial_control")));
            """,
        )


class CommandBarLatchTests(unittest.TestCase):
    def test_cmd_behaves_like_the_other_modifiers(self) -> None:
        run_harness(
            self,
            """
            CHECK(GHOST_IOS_MODIFIER_COUNT == 4);
            GHOST_IOSModifierLatch latch;
            latch.keyDown(Mod::OS, 1.00);
            latch.keyUp(Mod::OS, 1.05);
            CHECK(latch.mode(Mod::OS) == Mode::Latched);
            latch.pointerPressed();
            CHECK(latch.wantsDown(Mod::OS));
            latch.pointerReleased();
            CHECK(latch.mode(Mod::OS) == Mode::Off);
            """,
        )

    def test_invert_lock_applies_to_strokes_until_toggled_off(self) -> None:
        run_harness(
            self,
            """
            GHOST_IOSModifierLatch latch;
            latch.toggleLock(Mod::Control);
            CHECK(latch.mode(Mod::Control) == Mode::Locked);
            /* Navigation between strokes stays unmodified. */
            CHECK(!latch.wantsDown(Mod::Control));
            for (int i = 0; i < 3; i++) {
              latch.pointerPressed();
              CHECK(latch.wantsDown(Mod::Control));
              latch.pointerReleased();
              CHECK(latch.mode(Mod::Control) == Mode::Locked);
            }
            latch.toggleLock(Mod::Control);
            CHECK(latch.mode(Mod::Control) == Mode::Off);
            latch.pointerPressed();
            CHECK(!latch.wantsDown(Mod::Control));
            latch.pointerReleased();

            GHOST_IOSModifierChange changes[GHOST_IOS_MODIFIER_COUNT];
            latch.toggleLock(Mod::Control);
            latch.pointerPressed();
            CHECK(latch.takeChanges(changes) == 1 && changes[0].down);
            latch.pointerReleased();
            CHECK(latch.takeChanges(changes) == 1 && !changes[0].down);
            """,
        )


class CommandBarSourceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.belt = BELT_SOURCE.read_text()
        self.window = WINDOW_SOURCE.read_text()

    def test_bar_docks_vertically_with_cmd_shortcuts_and_settings(self) -> None:
        self.assertIn("stack_.axis = UILayoutConstraintAxisVertical;", self.belt)
        self.assertIn('@"blender_modifier_os"', self.belt)
        self.assertIn("GHOST_IOSModifier::OS", self.belt)
        # Pinned open: no swipe to tuck the bar away (see PinnedCommandBarTests).
        self.assertIn("settings_button_.showsMenuAsPrimaryAction = YES;", self.belt)
        self.assertIn('@"blender_command_bar_settings"', self.belt)
        self.assertIn('@"blender_command_bar_key_pad"', self.belt)
        self.assertIn("blender_command_key_%d", self.belt)
        header = BELT_HEADER.read_text()
        for name in ("GHOST_IOSModifierBelt_setContextKeys", "GHOST_IOSModifierBelt_setKeyPadVisible",
                     "GHOST_IOSModifierBelt_setSettingsMenu", "commandBarKey:"):
            self.assertIn(name, header)

    def test_key_pad_touches_belong_to_the_bar(self) -> None:
        contains = self.belt[self.belt.index("- (BOOL)containsView:(UIView *)view\n{") :]
        contains = contains[: contains.index("\n}\n")]
        self.assertIn("isDescendantOfView:key_pad_", contains)
        api = self.belt[self.belt.index("bool GHOST_IOSModifierBelt_containsView(") :]
        api = api[: api.index("\n}\n")]
        self.assertIn("containsView:view", api)

    def test_key_strokes_honor_latched_modifiers(self) -> None:
        stroke = self.window[self.window.index("- (void)sendKeyStroke:") :]
        stroke = stroke[: stroke.index("\n}\n")]
        pressed = stroke.index("modifier_latch->pointerPressed();")
        key_down = stroke.index("[self generateKeyEvent:stroke.key down:true utf8:utf8];")
        key_up = stroke.index("[self generateKeyEvent:stroke.key down:false utf8:nullptr];")
        released = stroke.index("modifier_latch->pointerReleased();")
        self.assertLess(pressed, key_down)
        self.assertLess(key_down, key_up)
        self.assertLess(key_up, released)
        mapping = self.window[self.window.index("static GHOST_TModifierKey IOS_belt_modifier_key(") :]
        mapping = mapping[: mapping.index("\n}\n")]
        self.assertIn("case GHOST_IOSModifier::OS:\n      return GHOST_kModifierKeyLeftOS;", mapping)

    def test_context_changes_refresh_keys_and_key_pad(self) -> None:
        setter = self.window[self.window.index("- (void)setInteractionContext:(const GHOST_IOSInteractionContext &)context\n{") :]
        setter = setter[: setter.index("\n}\n")]
        self.assertIn("if (context == interaction_context)", setter)
        self.assertIn("tool_history.update(context);", setter)
        self.assertIn("[self updateCommandBarForContext];", setter)
        update = self.window[self.window.index("- (void)updateCommandBarForContext") :]
        update = update[: update.index("\n}\n")]
        self.assertIn("GHOST_IOSModifierBelt_setContextKeys(", update)
        self.assertIn("GHOST_IOSCommandBar_wantsKeyPad(interaction_context)", update)

    def test_queued_commands_wake_the_main_loop(self) -> None:
        queue = self.window[self.window.index("- (void)queueCommand:") :]
        queue = queue[: queue.index("\n}\n")]
        self.assertIn("GHOST_kEventWindowUpdate", queue)
        self.assertIn("void GHOST_IOS_SetInteractionContext(", self.window)
        self.assertIn("bool GHOST_IOS_TakeCommand(", self.window)


class WindowManagerChannelTests(unittest.TestCase):
    def test_upstream_main_loop_has_only_a_platform_hook(self) -> None:
        source = WM_SOURCE.read_text()
        body = source[source.index("void WM_main_loop_body(bContext *C)") :]
        body = body[: body.index("\n}\n")]
        handlers = body.index("wm_event_do_handlers(C);")
        hook = body.index("BLENDER_PLATFORM_MAIN_LOOP_HOOK(C);")
        notifiers = body.index("wm_event_do_notifiers(C);")
        # After handlers, so a modal operator that just started is reported; before notifiers,
        # so the notes left by queued operators are handled in the same pass.
        self.assertLess(handlers, hook)
        self.assertLess(hook, notifiers)
        self.assertIn("#ifdef BLENDER_PLATFORM_MAIN_LOOP_HOOK", body)
        self.assertNotIn("GHOST_IOS", source)
        self.assertIn(
            "-DBLENDER_PLATFORM_MAIN_LOOP_HOOK=wm_ios_touch_channel_update", WM_CMAKE.read_text()
        )

    def test_shim_reports_context_and_runs_queued_operators(self) -> None:
        shim = WM_SHIM.read_text()
        update = shim[shim.index("void wm_ios_touch_channel_update(bContext *C)") :]
        update = update[: update.index("\n}\n")]
        self.assertIn("GHOST_IOS_TakeCommand(ghost_window, command)", update)
        self.assertIn("GHOST_IOS_SetInteractionContext(ghost_window,", update)
        self.assertLess(update.index("GHOST_IOS_TakeCommand"),
                        update.index("GHOST_IOS_SetInteractionContext"))
        run = shim[shim.index("static void wm_ios_run_command(") :]
        run = run[: run.index("\n}\n")]
        self.assertIn("WM_operatortype_find(line, true)", run)
        self.assertIn("WM_operator_name_call_ptr(C, ot, wm::OpCallContext::InvokeDefault", run)
        self.assertIn("WM_operator_properties_free(&ptr);", run)
        self.assertIn("CTX_wm_area_set(C, prev_area);", run)
        context = shim[shim.index("static GHOST_IOSInteractionContext wm_ios_interaction_context(") :]
        context = context[: context.index("\n}\n")]
        for needle in ("modalhandlers", "WM_HANDLER_TYPE_OP", "RGN_TYPE_WINDOW",
                       "WM_toolsystem_ref_find"):
            self.assertIn(needle, context)

    def test_channel_header_is_plain_cpp(self) -> None:
        header = INTERACTION_HEADER.read_text()
        self.assertNotIn("#import", header)
        self.assertNotIn("__OBJC__", header)
        self.assertIn("GHOST_IOS_COMMAND_MAX = 256", header)


if __name__ == "__main__":
    unittest.main()


class PinnedCommandBarTests(unittest.TestCase):
    """The bar stays open and Blender's canvas makes room for it (device feedback on #3)."""

    def setUp(self) -> None:
        self.belt = BELT_SOURCE.read_text()
        self.window = WINDOW_SOURCE.read_text()

    def test_canvas_insets_leave_room_for_the_docked_bar(self) -> None:
        run_harness(
            self,
            """
            double leading = -1.0, trailing = -1.0;
            GHOST_IOSCommandBar_canvasInsets(true, true, 66.0, 12.0, &leading, &trailing);
            CHECK(leading == 90.0 && trailing == 0.0);
            GHOST_IOSCommandBar_canvasInsets(true, false, 66.0, 12.0, &leading, &trailing);
            CHECK(leading == 0.0 && trailing == 90.0);
            /* With a hardware keyboard the bar hides and the canvas gets the full width back. */
            GHOST_IOSCommandBar_canvasInsets(false, true, 66.0, 12.0, &leading, &trailing);
            CHECK(leading == 0.0 && trailing == 0.0);
            """,
        )

    def test_bar_is_pinned_open(self) -> None:
        init = self.belt[self.belt.index("- (instancetype)initWithInputView:(UIView *)input_view\n                         delegate:(id<GHOSTIOSModifierBeltDelegate>)delegate\n{") :]
        init = init[: init.index("\n}\n")]
        self.assertIn("expanded_ = YES;", init)
        self.assertNotIn("UISwipeGestureRecognizer", self.belt)
        tap = self.belt[self.belt.index("- (void)handleTap:(UITapGestureRecognizer *)sender") :]
        tap = tap[: tap.index("\n}\n")]
        self.assertNotIn("setExpanded:", tap)

    def test_bar_sits_beside_the_canvas_not_over_it(self) -> None:
        self.assertIn("setCanvasInsetsLeading:", self.belt)
        self.assertIn("GHOST_IOSCommandBar_canvasInsets(", self.belt)
        # The view controller's root view holds the canvas and the bar side by side, while
        # getView() stays the Metal canvas so every touch coordinate is unchanged.
        self.assertIn("@interface GHOSTIOSCanvasContainer : UIView", self.window)
        self.assertIn("uiview_ = metal_view_;", self.window)
        layout = self.window[self.window.index("@implementation GHOSTIOSCanvasContainer") :]
        layout = layout[layout.index("- (void)layoutSubviews") :]
        layout = layout[: layout.index("\n}\n")]
        self.assertIn("leading_inset_", layout)
        self.assertIn("trailing_inset_", layout)
        load = self.window[self.window.index("- (void)viewDidLoad") :]
        load = load[: load.index("\n}\n")]
        self.assertNotIn("_view = (MTKView *)self.view;", load)


class TouchLayoutDefaultsTests(unittest.TestCase):
    """Factory layout for touch: a wider right column and a one-row timeline (device feedback)."""

    DEFAULTS = REPOSITORY / "source" / "blender" / "blenloader" / "intern" / "versioning_defaults.cc"
    PLATFORM = REPOSITORY / "build_files" / "ios" / "cmake" / "platform_ios.cmake"

    def test_factory_startup_has_only_a_platform_hook(self) -> None:
        source = self.DEFAULTS.read_text()
        body = source[source.index("void BLO_update_defaults_startup_blend(Main *bmain, const char *app_template)\n{") :]
        body = body[: body.index("\n}\n")]
        self.assertIn("#ifdef BLENDER_PLATFORM_STARTUP_DEFAULTS_HOOK", body)
        self.assertIn("BLENDER_PLATFORM_STARTUP_DEFAULTS_HOOK(bmain, app_template);", body)
        self.assertIn(
            "-DBLENDER_PLATFORM_STARTUP_DEFAULTS_HOOK=wm_ios_update_startup_defaults",
            self.PLATFORM.read_text(),
        )

    def test_shim_widens_the_right_column_and_collapses_the_timeline(self) -> None:
        shim = WM_SHIM.read_text()
        column = shim[shim.index("static void wm_ios_widen_right_column(") :]
        column = column[: column.index("\n}\n")]
        self.assertIn("SPACE_OUTLINER", column)
        self.assertIn("SPACE_PROPERTIES", column)
        # Never more than half the screen, so the viewport stays the main area.
        self.assertIn("screen_right / 2", column)
        timeline = shim[shim.index("static void wm_ios_collapse_timeline(") :]
        timeline = timeline[: timeline.index("\n}\n")]
        self.assertIn("SACTCONT_TIMELINE", timeline)
        self.assertIn("HEADERY", timeline)
        update = shim[shim.index("void wm_ios_update_startup_defaults(") :]
        update = update[: update.index("\n}\n")]
        self.assertIn("wm_ios_widen_right_column(", update)
        self.assertIn("wm_ios_collapse_timeline(", update)
