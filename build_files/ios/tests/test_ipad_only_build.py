#!/usr/bin/env python3

from pathlib import Path
import plistlib
import sys
import unittest


REPOSITORY = Path(__file__).resolve().parents[3]
IOS = REPOSITORY / "build_files" / "ios"
sys.path.insert(0, str(IOS))

import package_unsigned_ipa  # noqa: E402


class IPadOnlyBuildTests(unittest.TestCase):
    """The app is built for iPad only; iPhone is not a target (decision on #5)."""

    def test_app_info_plist_targets_ipad_only(self) -> None:
        with (REPOSITORY / "release" / "ios" / "Blender.app" / "Info.plist").open("rb") as handle:
            plist = plistlib.load(handle)
        self.assertEqual(plist["UIDeviceFamily"], [2])

    def test_xcode_build_setting_targets_ipad_only(self) -> None:
        cmake = (IOS / "cmake" / "creator_ios.cmake").read_text()
        self.assertIn('XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "2"', cmake)
        self.assertNotIn('XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"', cmake)

    def test_ipa_gate_requires_ipad_only(self) -> None:
        self.assertEqual(package_unsigned_ipa.EXPECTED_DEVICE_FAMILIES, {2})


if __name__ == "__main__":
    unittest.main()
