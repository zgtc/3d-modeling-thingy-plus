#!/usr/bin/env python3

from pathlib import Path
import unittest


REPOSITORY = Path(__file__).resolve().parents[3]
GHOST_INTERN = REPOSITORY / "intern" / "ghost" / "intern"
STYLE_HEADER = GHOST_INTERN / "GHOST_IOSStyle.hh"
STYLE_SOURCE = GHOST_INTERN / "GHOST_IOSStyle.mm"
BELT_SOURCE = GHOST_INTERN / "GHOST_IOSModifierBelt.mm"
WINDOW_SOURCE = GHOST_INTERN / "GHOST_WindowIOS.mm"
ACCESS_SOURCE = GHOST_INTERN / "GHOST_IOSFileAccess.mm"
GHOST_CMAKE = REPOSITORY / "build_files" / "ios" / "cmake" / "ghost_ios.cmake"


class GhostNativeStyleTests(unittest.TestCase):
    def test_style_module_is_built(self) -> None:
        cmake = GHOST_CMAKE.read_text()
        self.assertIn("intern/GHOST_IOSStyle.hh", cmake)
        self.assertIn("intern/GHOST_IOSStyle.mm", cmake)

    def test_liquid_glass_is_gated_with_a_pre_ios_26_fallback(self) -> None:
        source = STYLE_SOURCE.read_text()
        material = source[source.index("GHOST_IOSStyle_createMaterialView") :]
        material = material[: material.index("\n}\n")]
        self.assertIn("@available(iOS 26.0, *)", material)
        self.assertIn("UIGlassEffect effectWithStyle:UIGlassEffectStyleRegular", material)
        self.assertIn("UIBlurEffectStyleSystemThinMaterialDark", material)

        button = source[source.index("void GHOST_IOSStyle_applyFloatingButton") :]
        button = button[: button.index("\n}\n")]
        self.assertIn("@available(iOS 26.0, *)", button)
        self.assertIn("glassButtonConfiguration", button)
        self.assertIn("UIButtonConfigurationCornerStyleCapsule", button)

    def test_accent_follows_the_system_tint(self) -> None:
        self.assertIn("return UIColor.tintColor;", STYLE_SOURCE.read_text())
        self.assertNotIn("systemBlueColor", BELT_SOURCE.read_text())

    def test_every_floating_control_shares_the_style(self) -> None:
        window = WINDOW_SOURCE.read_text()
        self.assertIn('GHOST_IOSStyle_applyFloatingButton(close_window_button, @"xmark")', window)
        self.assertNotIn("close_window_button.backgroundColor", window)
        self.assertIn("GHOST_IOSStyle_applyFloatingButton(button_", ACCESS_SOURCE.read_text())
        belt = BELT_SOURCE.read_text()
        self.assertIn("GHOST_IOSStyle_createMaterialView(radius)", belt)
        self.assertIn("tool_size = GHOST_IOS_CONTROL_SIZE", belt)

    def test_text_entry_done_is_prominent_on_ios_26(self) -> None:
        window = WINDOW_SOURCE.read_text()
        start = window.index("- (void)initToolbar\n{")
        toolbar = window[start : window.index("- (void)generateKeyboardReturnEvent", start)]
        self.assertIn("@available(iOS 26.0, *)", toolbar)
        self.assertIn("toolbar_done_editing_item.style = UIBarButtonItemStyleProminent;", toolbar)

    def test_modifier_changes_give_haptic_feedback(self) -> None:
        belt = BELT_SOURCE.read_text()
        self.assertGreaterEqual(belt.count("GHOST_IOSStyle_selectionFeedback();"), 3)
        self.assertIn("UISelectionFeedbackGenerator", STYLE_SOURCE.read_text())

    def test_style_header_is_objc_only(self) -> None:
        header = STYLE_HEADER.read_text()
        self.assertIn("#ifdef __OBJC__", header)
        self.assertIn("GHOST_IOS_CONTROL_SIZE = 44.0", header)


if __name__ == "__main__":
    unittest.main()
