#!/usr/bin/env python3

from pathlib import Path
import subprocess
import tempfile
import textwrap
import unittest


REPOSITORY = Path(__file__).resolve().parents[3]
WM_INTERN = REPOSITORY / "source" / "blender" / "windowmanager" / "intern"
FIT_HEADER = WM_INTERN / "wm_splash_fit.hh"
SPLASH_SOURCE = WM_INTERN / "wm_splash_screen.cc"
WM_IOS_CMAKE = REPOSITORY / "build_files" / "ios" / "cmake" / "windowmanager_ios.cmake"


def run_harness(test: unittest.TestCase, body: str) -> None:
    harness = textwrap.dedent(
        """
        #include "wm_splash_fit.hh"
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
        source = Path(directory) / "splash_fit_test.cc"
        executable = Path(directory) / "splash_fit_test"
        source.write_text(harness)
        result = subprocess.run(
            ["xcrun", "clang++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
             "-I", str(WM_INTERN), str(source), "-o", str(executable)],
            capture_output=True, text=True)
        test.assertEqual(result.returncode, 0, result.stderr)
        result = subprocess.run([str(executable)], capture_output=True, text=True)
        test.assertEqual(result.returncode, 0, result.stdout + result.stderr)


class SplashFitTests(unittest.TestCase):
    """The splash must fit the iPad's canvas: device feedback on #5 showed its top and bottom
    cut off once the header and quick bar took part of the screen's height."""

    def test_image_keeps_its_size_when_everything_fits(self) -> None:
        run_harness(self, """
            const WM_SplashImageFit fit = wm_splash_fit_image(1000, 1000, 500, 400, 2000, 20);
            CHECK(fit.width == 1000 && fit.height == 500 && fit.x == 0);
        """)

    def test_image_shrinks_and_centers_so_the_menu_stays_on_screen(self) -> None:
        run_harness(self, """
            /* 1280 px of canvas, 900 px of menu: the image gets what is left. */
            const WM_SplashImageFit fit = wm_splash_fit_image(1000, 1000, 500, 900, 1280, 20);
            CHECK(fit.height + 900 + 20 + WM_SPLASH_FIT_MARGIN * 2 <= 1280);
            CHECK(fit.height >= WM_SPLASH_FIT_MIN_IMAGE_HEIGHT && fit.height < 500);
            /* Aspect ratio kept, centered over the menu. */
            CHECK(fit.width == fit.height * 2);
            CHECK(fit.x == (1000 - fit.width) / 2);
        """)

    def test_image_is_dropped_when_only_the_menu_fits(self) -> None:
        run_harness(self, """
            const WM_SplashImageFit fit = wm_splash_fit_image(1000, 1000, 500, 1200, 1280, 20);
            CHECK(fit.width == 0 && fit.height == 0);
        """)

    def test_splash_measures_the_menu_before_sizing_the_image_on_ios(self) -> None:
        source = SPLASH_SOURCE.read_text()
        self.assertIn('#include "wm_splash_fit.hh"', source)
        body = source[source.index("static ui::Block *wm_block_splash_create(") :]
        body = body[: body.index("\n}\n")]
        self.assertIn("#ifdef BLENDER_PLATFORM_SPLASH_FIT", body)
        fit = body[body.index("#ifdef BLENDER_PLATFORM_SPLASH_FIT") :]
        self.assertLess(fit.index("wm_block_splash_add_menu("), fit.index("ui::block_layout_resolve(block)"))
        self.assertLess(fit.index("ui::block_layout_resolve(block)"), fit.index("wm_splash_fit_image("))
        self.assertIn("WM_window_native_pixel_y(", fit)
        self.assertIn("-DBLENDER_PLATFORM_SPLASH_FIT", WM_IOS_CMAKE.read_text())


if __name__ == "__main__":
    unittest.main()
