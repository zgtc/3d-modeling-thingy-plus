#!/usr/bin/env python3

from pathlib import Path
import subprocess
import tempfile
import textwrap
import unittest


REPOSITORY = Path(__file__).resolve().parents[3]
GHOST = REPOSITORY / "intern" / "ghost"
GHOST_INTERN = GHOST / "intern"
SETTINGS_HEADER = GHOST_INTERN / "GHOST_IOSPencilSettings.hh"
KEYS_HEADER = GHOST_INTERN / "GHOST_IOSTouchKeys.hh"
PALETTE_HEADER = GHOST_INTERN / "GHOST_IOSPencilPalette.hh"
PALETTE_SOURCE = GHOST_INTERN / "GHOST_IOSPencilPalette.mm"
WINDOW_SOURCE = GHOST_INTERN / "GHOST_WindowIOS.mm"
GHOST_CMAKE = REPOSITORY / "build_files" / "ios" / "cmake" / "ghost_ios.cmake"


def run_harness(test: unittest.TestCase, body: str) -> None:
    """Compile a C++ program against the platform-neutral Pencil headers and run it."""
    harness = textwrap.dedent(
        """
        #include "GHOST_IOSPencilSettings.hh"
        #include "GHOST_IOSTouchKeys.hh"

        #include <cmath>
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

        using Choice = GHOST_IOSDoubleTapChoice;
        using Squeeze = GHOST_IOSSqueezeChoice;
        using System = GHOST_IOSSystemPencilAction;
        using Action = GHOST_IOSPencilAction;

        [[maybe_unused]] static GHOST_IOSInteractionContext context(GHOST_IOSEditor editor,
                                                   GHOST_IOSMode mode,
                                                   bool main_region = true,
                                                   const char *tool = "")
        {
          GHOST_IOSInteractionContext c;
          c.editor = editor;
          c.mode = mode;
          c.over_main_region = main_region;
          std::strcpy(c.tool, tool);
          return c;
        }

        int main()
        {
        """
    ) + textwrap.indent(textwrap.dedent(body), "  ") + "\n  return 0;\n}\n"

    with tempfile.TemporaryDirectory() as directory:
        temporary = Path(directory)
        source = temporary / "pencil_palette_test.cc"
        executable = temporary / "pencil_palette_test"
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


class PencilSettingsTests(unittest.TestCase):
    def test_first_double_tap_asks_and_then_follows_the_choice(self) -> None:
        run_harness(
            self,
            """
            GHOST_IOSTouchSettings settings;
            CHECK(settings.double_tap == Choice::Unset);
            /* Whatever the system says, the first tap only asks. */
            CHECK(GHOST_IOSPencil_doubleTapAction(settings.double_tap, System::SwitchEraser) ==
                  Action::AskUser);
            CHECK(GHOST_IOSPencil_doubleTapAction(Choice::RightClick, System::SwitchEraser) ==
                  Action::RightClick);
            CHECK(GHOST_IOSPencil_doubleTapAction(Choice::PreviousTool, System::Ignore) ==
                  Action::PreviousTool);
            CHECK(GHOST_IOSPencil_doubleTapAction(Choice::Palette, System::Ignore) ==
                  Action::Palette);
            CHECK(GHOST_IOSPencil_doubleTapAction(Choice::Eraser, System::Ignore) ==
                  Action::Eraser);
            """,
        )

    def test_follow_system_maps_every_preferred_action(self) -> None:
        run_harness(
            self,
            """
            CHECK(GHOST_IOSPencil_doubleTapAction(Choice::System, System::Ignore) == Action::None);
            CHECK(GHOST_IOSPencil_doubleTapAction(Choice::System, System::SwitchEraser) ==
                  Action::Eraser);
            CHECK(GHOST_IOSPencil_doubleTapAction(Choice::System, System::SwitchPrevious) ==
                  Action::PreviousTool);
            CHECK(GHOST_IOSPencil_doubleTapAction(Choice::System, System::ShowColorPalette) ==
                  Action::Palette);
            CHECK(GHOST_IOSPencil_doubleTapAction(Choice::System, System::ShowInkAttributes) ==
                  Action::Palette);
            CHECK(GHOST_IOSPencil_doubleTapAction(Choice::System, System::ShowContextualPalette) ==
                  Action::Palette);
            /* iPadOS runs a system shortcut itself; the app must not act twice. */
            CHECK(GHOST_IOSPencil_doubleTapAction(Choice::System, System::RunSystemShortcut) ==
                  Action::None);
            """,
        )

    def test_squeeze_follows_the_system_unless_overridden(self) -> None:
        run_harness(
            self,
            """
            GHOST_IOSTouchSettings settings;
            CHECK(settings.squeeze == Squeeze::System);
            CHECK(GHOST_IOSPencil_squeezeAction(Squeeze::System, System::ShowContextualPalette) ==
                  Action::Palette);
            CHECK(GHOST_IOSPencil_squeezeAction(Squeeze::System, System::Ignore) == Action::None);
            CHECK(GHOST_IOSPencil_squeezeAction(Squeeze::Palette, System::Ignore) ==
                  Action::Palette);
            CHECK(GHOST_IOSPencil_squeezeAction(Squeeze::Off, System::ShowContextualPalette) ==
                  Action::None);
            /* A system shortcut wins even over the app override. */
            CHECK(GHOST_IOSPencil_squeezeAction(Squeeze::Palette, System::RunSystemShortcut) ==
                  Action::None);
            """,
        )

    def test_stored_values_out_of_range_fall_back_to_defaults(self) -> None:
        run_harness(
            self,
            """
            CHECK(GHOST_IOSDoubleTapChoice_fromStored(-1) == Choice::Unset);
            CHECK(GHOST_IOSDoubleTapChoice_fromStored(99) == Choice::Unset);
            CHECK(GHOST_IOSDoubleTapChoice_fromStored(3) == Choice::Palette);
            CHECK(GHOST_IOSSqueezeChoice_fromStored(7) == Squeeze::System);
            CHECK(GHOST_IOSSqueezeChoice_fromStored(2) == Squeeze::Off);
            CHECK(GHOST_IOSGestureAction_fromStored(9, GHOST_IOSGestureAction::Redo) ==
                  GHOST_IOSGestureAction::Redo);
            CHECK(GHOST_IOSGestureAction_fromStored(0, GHOST_IOSGestureAction::Redo) ==
                  GHOST_IOSGestureAction::None);
            """,
        )

    def test_pencil_only_drawing_blocks_fingers_only_in_the_canvas(self) -> None:
        run_harness(
            self,
            """
            GHOST_IOSTouchSettings settings;
            const auto viewport = context(GHOST_IOSEditor::View3D, GHOST_IOSMode::Sculpt);
            CHECK(GHOST_IOSFinger_mayClick(settings, viewport));

            settings.pencil_only_drawing = true;
            CHECK(!GHOST_IOSFinger_mayClick(settings, viewport));
            CHECK(!GHOST_IOSFinger_mayClick(
                settings, context(GHOST_IOSEditor::Image, GHOST_IOSMode::Paint)));
            /* Headers, toolbars and other editors keep working with a finger. */
            CHECK(GHOST_IOSFinger_mayClick(
                settings, context(GHOST_IOSEditor::View3D, GHOST_IOSMode::Sculpt, false)));
            CHECK(GHOST_IOSFinger_mayClick(
                settings, context(GHOST_IOSEditor::Node, GHOST_IOSMode::Other)));
            CHECK(GHOST_IOSFinger_mayClick(
                settings, context(GHOST_IOSEditor::Other, GHOST_IOSMode::Object)));
            """,
        )

    def test_previous_tool_is_tracked_per_editor_and_mode(self) -> None:
        run_harness(
            self,
            """
            GHOST_IOSToolHistory history;
            const auto V = GHOST_IOSEditor::View3D;
            history.update(context(V, GHOST_IOSMode::Object, true, "builtin.select_box"));
            CHECK(history.previous()[0] == '\\0');
            history.update(context(V, GHOST_IOSMode::Object, true, "builtin.move"));
            CHECK(std::strcmp(history.previous(), "builtin.select_box") == 0);
            /* Reports without a change keep the history. */
            history.update(context(V, GHOST_IOSMode::Object, false, "builtin.move"));
            history.update(context(V, GHOST_IOSMode::Object, true, ""));
            CHECK(std::strcmp(history.previous(), "builtin.select_box") == 0);
            history.update(context(V, GHOST_IOSMode::Object, true, "builtin.select_box"));
            CHECK(std::strcmp(history.previous(), "builtin.move") == 0);

            /* A tool from another mode cannot be set there. */
            history.update(context(V, GHOST_IOSMode::Sculpt, true, "builtin_brush.draw"));
            CHECK(history.previous()[0] == '\\0');
            """,
        )

    def test_operator_calls_are_formatted_for_the_window_manager(self) -> None:
        run_harness(
            self,
            """
            char command[GHOST_IOS_COMMAND_MAX];
            CHECK(GHOST_IOSCommand_format(command, "wm.tool_set_by_id", "name", "builtin.move"));
            CHECK(std::strcmp(command, "wm.tool_set_by_id\\nname=builtin.move") == 0);
            CHECK(GHOST_IOSCommand_format(command, "screen.redo_last"));
            CHECK(std::strcmp(command, "screen.redo_last") == 0);

            char long_value[GHOST_IOS_COMMAND_MAX + 8];
            std::memset(long_value, 'x', sizeof(long_value) - 1);
            long_value[sizeof(long_value) - 1] = '\\0';
            CHECK(!GHOST_IOSCommand_format(command, "wm.tool_set_by_id", "name", long_value));
            CHECK(command[0] == '\\0');
            CHECK(!GHOST_IOSCommand_format(command, ""));
            """,
        )


class PencilPaletteLayoutTests(unittest.TestCase):
    def test_palette_follows_the_mode_under_the_tip(self) -> None:
        run_harness(
            self,
            """
            const auto V = GHOST_IOSEditor::View3D;
            const GHOST_IOSPaletteSlot *object = GHOST_IOSPalette_slots(
                context(V, GHOST_IOSMode::Object));
            CHECK(std::strcmp(object[0].title, "Move") == 0);
            CHECK(object[0].stroke.key == GHOST_kKeyG);
            CHECK(std::strcmp(object[7].title, "Redo") == 0);

            const GHOST_IOSPaletteSlot *edit = GHOST_IOSPalette_slots(
                context(V, GHOST_IOSMode::Edit));
            CHECK(std::strcmp(edit[0].title, "Vertex") == 0);
            CHECK(edit[0].stroke.key == GHOST_kKey1);
            CHECK(edit[5].stroke.key == GHOST_kKeyR);
            CHECK(edit[5].stroke.modifiers == GHOST_IOS_STROKE_CTRL);

            const GHOST_IOSPaletteSlot *sculpt = GHOST_IOSPalette_slots(
                context(V, GHOST_IOSMode::Sculpt));
            CHECK(std::strcmp(sculpt[0].title, "Radius") == 0);
            CHECK(sculpt[0].stroke.key == GHOST_kKeyF);
            CHECK(sculpt[1].stroke.modifiers == GHOST_IOS_STROKE_SHIFT);

            const GHOST_IOSPaletteSlot *paint = GHOST_IOSPalette_slots(
                context(V, GHOST_IOSMode::Paint));
            CHECK(paint != sculpt && paint != object && paint != edit);

            for (const GHOST_IOSPaletteSlot *slots : {object, edit, sculpt, paint}) {
              for (int i = 0; i < GHOST_IOS_PALETTE_SLOT_COUNT; i++) {
                CHECK(slots[i].title != nullptr && slots[i].title[0] != '\\0');
                CHECK(slots[i].symbol != nullptr && slots[i].symbol[0] != '\\0');
                /* Every key slot sends a real key; Invert has no key of its own. */
                CHECK((slots[i].kind == GHOST_IOSSlotKind::Key) ==
                      (slots[i].stroke.key != GHOST_kKeyUnknown));
              }
            }
            """,
        )

    def test_ring_slots_start_at_the_top_and_hit_test_back(self) -> None:
        run_harness(
            self,
            """
            const GHOST_IOSPalettePoint center = {200.0, 300.0};
            const GHOST_IOSPalettePoint top = GHOST_IOSPalette_slotCenter(center, 88.0, 0, 8);
            CHECK(std::fabs(top.x - 200.0) < 1e-6);
            CHECK(std::fabs(top.y - 212.0) < 1e-6);
            const GHOST_IOSPalettePoint right = GHOST_IOSPalette_slotCenter(center, 88.0, 2, 8);
            CHECK(std::fabs(right.x - 288.0) < 1e-6);
            CHECK(std::fabs(right.y - 300.0) < 1e-6);

            for (int i = 0; i < 8; i++) {
              const GHOST_IOSPalettePoint slot = GHOST_IOSPalette_slotCenter(center, 88.0, i, 8);
              CHECK(GHOST_IOSPalette_slotAt(center, 30.0, 128.0, 8, slot) == i);
            }
            /* The middle closes the palette, and so does anything outside the ring. */
            CHECK(GHOST_IOSPalette_slotAt(center, 30.0, 128.0, 8, center) == -1);
            CHECK(GHOST_IOSPalette_slotAt(center, 30.0, 128.0, 8, {200.0, 160.0}) == -1);
            /* Just counter-clockwise of the top slot still belongs to it. */
            CHECK(GHOST_IOSPalette_slotAt(center, 30.0, 128.0, 8, {190.0, 212.0}) == 0);
            CHECK(GHOST_IOSPalette_slotAt(center, 30.0, 128.0, 0, top) == -1);
            """,
        )

    def test_palette_moves_inwards_near_screen_edges(self) -> None:
        run_harness(
            self,
            """
            const GHOST_IOSPalettePoint corner = GHOST_IOSPalette_clampCenter(
                {5.0, 1170.0}, 128.0, 12.0, 820.0, 1180.0);
            CHECK(std::fabs(corner.x - 140.0) < 1e-6);
            CHECK(std::fabs(corner.y - 1040.0) < 1e-6);
            const GHOST_IOSPalettePoint middle = GHOST_IOSPalette_clampCenter(
                {400.0, 500.0}, 128.0, 12.0, 820.0, 1180.0);
            CHECK(middle.x == 400.0 && middle.y == 500.0);
            /* Too small to fit: centered. */
            const GHOST_IOSPalettePoint tiny = GHOST_IOSPalette_clampCenter(
                {10.0, 10.0}, 128.0, 12.0, 200.0, 1180.0);
            CHECK(tiny.x == 100.0);
            """,
        )


class PencilInteractionSourceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.window = WINDOW_SOURCE.read_text()
        self.palette = PALETTE_SOURCE.read_text()

    def test_build_lists_the_palette_sources(self) -> None:
        cmake = GHOST_CMAKE.read_text()
        for name in ("GHOST_IOSInteraction.hh", "intern/GHOST_IOSPencilPalette.hh",
                     "intern/GHOST_IOSPencilPalette.mm", "intern/GHOST_IOSPencilSettings.hh",
                     "intern/GHOST_IOSTouchKeys.hh"):
            self.assertIn(name, cmake)

    def test_window_uses_the_current_pencil_interaction_api(self) -> None:
        self.assertNotIn("pencilInteractionDidTap", self.window)
        tap = self.window[self.window.index("didReceiveTap:(UIPencilInteractionTap *)tap") :]
        tap = tap[: tap.index("\n}\n")]
        self.assertIn("GHOST_IOSPencil_doubleTapAction(", tap)
        self.assertIn("GHOST_IOSPencil_systemTapAction()", tap)
        self.assertIn("tap.hoverPose", tap)

        squeeze = self.window[
            self.window.index("didReceiveSqueeze:(UIPencilInteractionSqueeze *)squeeze") :
        ]
        squeeze = squeeze[: squeeze.index("\n}\n")]
        # Open on release, so a squeeze held to drag does not flash the palette.
        self.assertIn("squeeze.phase != UIPencilInteractionPhaseEnded", squeeze)
        self.assertIn("GHOST_IOSPencil_squeezeAction(", squeeze)
        self.assertIn("squeeze.hoverPose", squeeze)
        self.assertIn("UIPencilInteraction.preferredSqueezeAction", self.palette)
        self.assertIn("UIPencilInteraction.preferredTapAction", self.palette)

    def test_first_double_tap_asks_and_saves_the_answer(self) -> None:
        ask = self.window[self.window.index("case GHOST_IOSPencilAction::AskUser:") :]
        ask = ask[: ask.index("break;")]
        self.assertIn("GHOST_IOSPencil_askDoubleTap(", ask)
        self.assertNotIn("click(", ask)
        choose = self.window[self.window.index("- (void)chooseDoubleTap:") :]
        choose = choose[: choose.index("\n}\n")]
        self.assertIn("GHOST_IOSTouchSettings_save(touch_settings)", choose)
        self.assertIn("UIAlertControllerStyleActionSheet", self.palette)
        self.assertIn("popover.sourceRect", self.palette)

    def test_palette_uses_pencil_haptics_and_shared_style(self) -> None:
        self.assertIn("UICanvasFeedbackGenerator feedbackGeneratorForView:", self.palette)
        self.assertIn("alignmentOccurredAtLocation:", self.palette)
        self.assertIn("GHOST_IOSStyle_createMaterialView(", self.palette)
        self.assertIn("UIHoverGestureRecognizer", self.palette)
        # Liquid Glass is reached only through the style helper, which guards iOS 26.
        for source in (self.palette, BELT_SOURCE_TEXT):
            self.assertNotIn("UIGlassEffect", source)

    def test_palette_touches_never_reach_the_canvas(self) -> None:
        should_receive = self.window[
            self.window.index("       shouldReceiveTouch:(UITouch *)touch") :
            self.window.index("shouldRecognizeSimultaneouslyWithGestureRecognizer:\n        (UIGestureRecognizer *)otherGestureRecognizer\n{")
        ]
        self.assertIn("GHOST_IOSPencilPalette_containsView(pencil_palette, touch.view)",
                      should_receive)
        send_event = self.window[self.window.index("- (void)sendEvent:(UIEvent *)event") :]
        send_event = send_event[: send_event.index("[super sendEvent:event];")]
        self.assertIn("GHOST_IOSPencilPalette_containsView(pencil_palette, touch.view)",
                      send_event)

    def test_pencil_only_drawing_turns_finger_drags_into_navigation(self) -> None:
        tap = self.window[self.window.rindex("- (void)handleTap:") :]
        tap = tap[: tap.index("\n}\n")]
        self.assertIn("GHOST_IOSFinger_mayClick(touch_settings, interaction_context)", tap)
        drag = self.window[self.window.rindex("- (void)handleDoubleTapDrag:") :]
        drag = drag[: drag.index("\n}\n")]
        self.assertIn("GHOST_IOSFinger_mayClick(touch_settings, interaction_context)", drag)
        self.assertIn("PAN_GESTURE_TWO_FINGERS", drag)
        self.assertIn("UIPencilInteraction.prefersPencilOnlyDrawing", self.palette)

    def test_previous_tool_goes_through_the_window_manager(self) -> None:
        previous = self.window[self.window.index("case GHOST_IOSPencilAction::PreviousTool:") :]
        previous = previous[: previous.index("break;")]
        self.assertIn('"wm.tool_set_by_id", "name", tool_history.previous()', previous)
        self.assertIn("[self queueCommand:command]", previous)


BELT_SOURCE_TEXT = (GHOST_INTERN / "GHOST_IOSModifierBelt.mm").read_text()


if __name__ == "__main__":
    unittest.main()


class PencilHoverOverControlsTests(unittest.TestCase):
    """A Pencil hovering a native control must not move Blender's cursor (device feedback on #3).

    Otherwise a palette slot or key such as Duplicate starts its transform where the control
    was, and the item then trails the Pencil tip by that offset.
    """

    def test_hover_over_native_controls_keeps_the_cursor(self) -> None:
        window = WINDOW_SOURCE.read_text()
        hover = window[window.index("- (void)handleHover:(GHOSTUIHoverGestureRecognizer *)sender\n{") :]
        hover = hover[: hover.index("\n}\n")]
        hit = hover.index("hitTest:")
        move = hover.index("virtual_pointer->moveAbsolute(")
        self.assertLess(hit, move)
        self.assertIn("!= window->getView()", hover)
