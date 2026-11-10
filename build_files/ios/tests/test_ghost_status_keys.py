#!/usr/bin/env python3

from pathlib import Path
import subprocess
import tempfile
import textwrap
import unittest


REPOSITORY = Path(__file__).resolve().parents[3]
GHOST = REPOSITORY / "intern" / "ghost"
GHOST_INTERN = GHOST / "intern"
WM = REPOSITORY / "source" / "blender" / "windowmanager"
WM_INTERN = WM / "intern"
STATUS_KEYS = WM_INTERN / "wm_ios_status_keys.hh"
SHIM = WM_INTERN / "wm_platform_ios.cc"
STATUS_TEMPLATE = (REPOSITORY / "source" / "blender" / "editors" / "interface" / "templates"
                   / "interface_template_status.cc")
EVENT_TEMPLATE = (REPOSITORY / "source" / "blender" / "editors" / "interface" / "templates"
                  / "interface_template_event.cc")
PLATFORM_CMAKE = REPOSITORY / "build_files" / "ios" / "cmake" / "platform_ios.cmake"


def run_harness(test: unittest.TestCase, includes: str, body: str) -> None:
    harness = includes + textwrap.dedent(
        """
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

        int main()
        {
        """
    ) + textwrap.indent(textwrap.dedent(body), "  ") + "\n  return 0;\n}\n"
    with tempfile.TemporaryDirectory() as directory:
        source = Path(directory) / "status_keys_test.cc"
        executable = Path(directory) / "status_keys_test"
        source.write_text(harness)
        result = subprocess.run(
            ["xcrun", "clang++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
             "-I", str(GHOST), "-I", str(GHOST_INTERN), "-I", str(WM), "-I", str(WM_INTERN),
             "-I", str(REPOSITORY / "intern" / "guardedalloc"),
             str(source), "-o", str(executable)],
            capture_output=True, text=True)
        test.assertEqual(result.returncode, 0, result.stderr)
        result = subprocess.run([str(executable)], capture_output=True, text=True)
        test.assertEqual(result.returncode, 0, result.stdout + result.stderr)


class RunningToolKeyTests(unittest.TestCase):
    """While a tool runs, its keyboard shortcuts go on the quick bar (device feedback on #5:
    the status line's mouse hints are of no use on iPad)."""

    def test_blender_keys_map_to_ghost_keys(self) -> None:
        run_harness(self, '#include "wm_ios_status_keys.hh"\nusing namespace blender;\n', """
            CHECK(wm_ios_ghost_key(EVT_AKEY) == GHOST_kKeyA);
            CHECK(wm_ios_ghost_key(EVT_ZKEY) == GHOST_kKeyZ);
            CHECK(wm_ios_ghost_key(EVT_ZEROKEY) == GHOST_kKey0);
            CHECK(wm_ios_ghost_key(EVT_NINEKEY) == GHOST_kKey9);
            CHECK(wm_ios_ghost_key(EVT_PAD0) == GHOST_kKeyNumpad0);
            CHECK(wm_ios_ghost_key(EVT_F1KEY) == GHOST_kKeyF1);
            CHECK(wm_ios_ghost_key(EVT_ESCKEY) == GHOST_kKeyEsc);
            CHECK(wm_ios_ghost_key(EVT_RETKEY) == GHOST_kKeyEnter);
            CHECK(wm_ios_ghost_key(EVT_PADENTER) == GHOST_kKeyNumpadEnter);
            CHECK(wm_ios_ghost_key(EVT_TABKEY) == GHOST_kKeyTab);
            CHECK(wm_ios_ghost_key(EVT_SPACEKEY) == GHOST_kKeySpace);
            CHECK(wm_ios_ghost_key(EVT_BACKSPACEKEY) == GHOST_kKeyBackSpace);
            CHECK(wm_ios_ghost_key(EVT_DELKEY) == GHOST_kKeyDelete);
            CHECK(wm_ios_ghost_key(EVT_UPARROWKEY) == GHOST_kKeyUpArrow);
            CHECK(wm_ios_ghost_key(EVT_PAGEUPKEY) == GHOST_kKeyUpPage);
            /* Mouse buttons and the modifier keys themselves have no quick-bar key. */
            CHECK(wm_ios_ghost_key(LEFTMOUSE) == GHOST_kKeyUnknown);
            CHECK(wm_ios_ghost_key(RIGHTMOUSE) == GHOST_kKeyUnknown);
            CHECK(wm_ios_ghost_key(EVT_LEFTSHIFTKEY) == GHOST_kKeyUnknown);
            CHECK(wm_ios_ghost_key(EVT_OSKEY) == GHOST_kKeyUnknown);
        """)

    def test_running_tool_keys_skip_mouse_duplicates_and_overflow(self) -> None:
        run_harness(self, '#include "wm_ios_status_keys.hh"\nusing namespace blender;\n', """
            GHOST_IOSInteractionContext context;
            CHECK(wm_ios_modal_key_add(context, "Confirm", EVT_RETKEY, false, false, false, false));
            CHECK(!wm_ios_modal_key_add(context, "Confirm", LEFTMOUSE, false, false, false, false));
            CHECK(!wm_ios_modal_key_add(context, "Cancel", RIGHTMOUSE, false, false, false, false));
            CHECK(wm_ios_modal_key_add(context, "Cancel", EVT_ESCKEY, false, false, false, false));
            /* A second binding for an item already shown is left out. */
            CHECK(!wm_ios_modal_key_add(context, "Confirm", EVT_SPACEKEY, false, false, false, false));
            CHECK(wm_ios_modal_key_add(context, "Snap Invert", EVT_TABKEY, false, true, false, false));
            CHECK(context.modal_keys_num == 3);
            CHECK(std::strcmp(context.modal_keys[0].title, "Confirm") == 0);
            CHECK(context.modal_keys[0].key == GHOST_kKeyEnter);
            CHECK(context.modal_keys[2].modifiers == GHOST_IOS_MODAL_KEY_CTRL);
            for (const char *name : {"X Axis", "Y Axis", "Z Axis", "Plane"}) {
              wm_ios_modal_key_add(context, name, EVT_XKEY, false, false, false, false);
            }
            CHECK(context.modal_keys_num == GHOST_IOS_MODAL_KEY_MAX);
            /* The context compares the keys, so the bar updates when the tool changes. */
            GHOST_IOSInteractionContext other = context;
            CHECK(other == context);
            other.modal_keys[1].key = GHOST_kKeyA;
            CHECK(!(other == context));
        """)

    def test_quick_bar_shows_the_running_tools_keys(self) -> None:
        run_harness(self, '#include "GHOST_IOSSettingsFile.hh"\n', """
            GHOST_IOSInteractionContext context;
            context.editor = GHOST_IOSEditor::View3D;
            context.mode = GHOST_IOSMode::Object;
            std::strcpy(context.modal_keys[0].title, "Confirm");
            context.modal_keys[0].key = GHOST_kKeyEnter;
            std::strcpy(context.modal_keys[1].title, "Snap Invert");
            context.modal_keys[1].key = GHOST_kKeyTab;
            context.modal_keys[1].modifiers = GHOST_IOS_MODAL_KEY_CTRL;
            context.modal_keys_num = 2;
            GHOST_IOSSettingsFile file;
            GHOST_IOSCustomBar bar;
            bar.build(context, file);
            CHECK(std::strcmp(bar.set, "modal") == 0);
            CHECK(bar.count == 2);
            CHECK(std::strcmp(bar.keys[0].title, "Confirm") == 0);
            CHECK(bar.keys[0].stroke.key == GHOST_kKeyEnter && bar.keys[0].command == nullptr);
            CHECK(bar.keys[1].stroke.modifiers == GHOST_IOS_STROKE_CTRL);
            /* With no tool running, the mode's own keys come back. */
            context.modal_keys_num = 0;
            bar.build(context, file);
            CHECK(std::strcmp(bar.set, "object") == 0);
        """)

    def test_shim_reads_the_running_tools_modal_keymap(self) -> None:
        shim = SHIM.read_text()
        self.assertIn('#include "wm_ios_status_keys.hh"', shim)
        fill = shim[shim.index("static void wm_ios_modal_keys(") :]
        fill = fill[: fill.index("\n}\n")]
        for needle in ("modal_items", "poll_modal_item", "propvalue", "wm_ios_modal_key_add("):
            self.assertIn(needle, fill)
        self.assertIn("wm_ios_modal_keys(", shim[shim.index("static GHOST_IOSInteractionContext wm_ios_interaction_context(") :])


class TouchStatusLineTests(unittest.TestCase):
    def test_status_line_leaves_out_mouse_hints_on_ios(self) -> None:
        self.assertIn("-DBLENDER_PLATFORM_TOUCH_STATUS", PLATFORM_CMAKE.read_text())
        status = STATUS_TEMPLATE.read_text()
        body = status[status.index("void uiTemplateInputStatus(Layout *layout, bContext *C)") :]
        body = body[: body.index("\n}\n")]
        self.assertIn("#ifdef BLENDER_PLATFORM_TOUCH_STATUS", body)
        touch = body[body.index("#ifdef BLENDER_PLATFORM_TOUCH_STATUS") :]
        self.assertIn("status_icon_is_mouse(", touch)
        # Idle mouse-button hints come after the modal keymap status and are skipped on iOS.
        idle = body[body.index("if (WM_window_modal_keymap_status_draw(C, win, *layout))") :]
        self.assertIn("#ifdef BLENDER_PLATFORM_TOUCH_STATUS\n  return;\n#endif", idle)
        event = EVENT_TEMPLATE.read_text()
        item = event[event.index("int template_status_bar_modal_item(") :]
        item = item[: item.index("\n}\n")]
        self.assertIn("#ifdef BLENDER_PLATFORM_TOUCH_STATUS", item)
        self.assertIn("ISMOUSE(kmi->type)", item)


if __name__ == "__main__":
    unittest.main()
