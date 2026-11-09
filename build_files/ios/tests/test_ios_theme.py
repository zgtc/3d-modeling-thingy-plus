#!/usr/bin/env python3

from pathlib import Path
import shutil
import subprocess
import tempfile
import textwrap
import unittest


REPOSITORY = Path(__file__).resolve().parents[3]
USERDEF = REPOSITORY / "release" / "datafiles" / "userdef"
DEFAULT_THEME = USERDEF / "userdef_default_theme.c"
PLATFORM_THEME = USERDEF / "userdef_platform_theme.cc"
BLENDFILE_SOURCE = REPOSITORY / "source" / "blender" / "blenkernel" / "intern" / "blendfile.cc"
RESOURCES_SOURCE = REPOSITORY / "source" / "blender" / "editors" / "interface" / "resources.cc"
BLENLOADER_CMAKE = REPOSITORY / "source" / "blender" / "blenloader" / "CMakeLists.txt"
IOS_PLATFORM = REPOSITORY / "build_files" / "ios" / "cmake" / "platform_ios.cmake"
INCLUDES = [
    REPOSITORY / "source" / "blender" / "blenloader",
    REPOSITORY / "source" / "blender" / "makesdna",
    REPOSITORY / "source" / "blender" / "blenlib",
    REPOSITORY / "intern" / "guardedalloc",
]


def compiler(language: str) -> list[str]:
    tool = "clang++" if language == "c++" else "clang"
    if shutil.which("xcrun"):
        return ["xcrun", tool]
    return [shutil.which(tool) or ("c++" if language == "c++" else "cc")]


def run_theme_harness(test: unittest.TestCase, body: str, defines: list[str]) -> None:
    """Build the default theme plus the platform pass and run checks against the result."""
    harness = textwrap.dedent(
        """
        #include "BLO_userdef_default.h"

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

        using namespace blender;

        [[maybe_unused]] static bool rgba_eq(const unsigned char c[4], unsigned int v)
        {
          return c[0] == ((v >> 24) & 0xff) && c[1] == ((v >> 16) & 0xff) &&
                 c[2] == ((v >> 8) & 0xff) && c[3] == (v & 0xff);
        }

        int main()
        {
          static bTheme theme;
          std::memcpy(&theme, &U_theme_default, sizeof(theme));
          BLO_userdef_theme_apply_platform(&theme);
          const bTheme &def = U_theme_default;
          (void)def;
        """
    ) + textwrap.indent(textwrap.dedent(body), "  ") + "\n  return 0;\n}\n"

    include_flags = [f"-I{path}" for path in INCLUDES]
    with tempfile.TemporaryDirectory() as directory:
        temporary = Path(directory)
        main = temporary / "theme_test.cc"
        main.write_text(harness)
        default_object = temporary / "default_theme.o"
        executable = temporary / "theme_test"
        steps = [
            compiler("c") + ["-std=c11", "-c", *include_flags, str(DEFAULT_THEME),
                             "-o", str(default_object)],
            compiler("c++") + ["-std=c++20", "-Wall", "-Werror", *defines, *include_flags,
                               str(PLATFORM_THEME), str(main), str(default_object),
                               "-o", str(executable)],
        ]
        for step in steps:
            result = subprocess.run(step, capture_output=True, text=True)
            test.assertEqual(result.returncode, 0, result.stderr)
        result = subprocess.run([str(executable)], capture_output=True, text=True)
        test.assertEqual(result.returncode, 0, result.stdout + result.stderr)


class IOSThemeTests(unittest.TestCase):
    def test_ios_theme_uses_system_colors_and_rounded_controls(self) -> None:
        run_theme_harness(
            self,
            """
            CHECK(rgba_eq(theme.tui.wcol_regular.inner_sel, 0x0a84ffff));
            CHECK(rgba_eq(theme.tui.wcol_regular.inner, 0x3a3a3cff));
            CHECK(theme.tui.wcol_regular.roundness >= 0.5f);
            CHECK(theme.tui.wcol_regular.shaded == 0);
            CHECK(rgba_eq(theme.tui.panel_back, 0x1c1c1eff));
            CHECK(rgba_eq(theme.tui.widget_text_cursor, 0x0a84ffff));
            CHECK(rgba_eq(theme.tui.editor_border, 0x000000ff));
            CHECK(rgba_eq(theme.space_topbar.header, 0x000000ff));
            """,
            ["-DBLENDER_PLATFORM_IOS_THEME"],
        )

    def test_ios_theme_keeps_modeling_colors(self) -> None:
        run_theme_harness(
            self,
            """
            CHECK(std::memcmp(theme.space_view3d.back, def.space_view3d.back, 4) == 0);
            CHECK(std::memcmp(theme.space_view3d.select, def.space_view3d.select, 4) == 0);
            CHECK(std::memcmp(theme.space_view3d.active, def.space_view3d.active, 4) == 0);
            CHECK(std::memcmp(theme.space_view3d.vertex_select, def.space_view3d.vertex_select,
                              4) == 0);
            CHECK(std::memcmp(theme.tui.xaxis, def.tui.xaxis, 4) == 0);
            CHECK(std::memcmp(theme.space_node.back, def.space_node.back, 4) == 0);
            CHECK(std::memcmp(theme.common.anim.keyframe_selected,
                              def.common.anim.keyframe_selected, 4) == 0);
            CHECK(std::memcmp(theme.tarm, def.tarm, sizeof(theme.tarm)) == 0);
            """,
            ["-DBLENDER_PLATFORM_IOS_THEME"],
        )

    def test_other_platforms_keep_the_stock_theme(self) -> None:
        run_theme_harness(
            self,
            """
            CHECK(std::memcmp(&theme, &def, sizeof(theme)) == 0);
            """,
            [],
        )

    def test_factory_and_reset_paths_apply_the_platform_theme(self) -> None:
        blendfile = BLENDFILE_SOURCE.read_text()
        defaults = blendfile[
            blendfile.index("UserDef *BKE_blendfile_userdef_from_defaults()") : blendfile.index(
                "bool BKE_blendfile_userdef_write("
            )
        ]
        self.assertIn("BLO_userdef_theme_apply_platform(btheme);", defaults)

        resources = RESOURCES_SOURCE.read_text()
        reset = resources[resources.index("void init_default()") : resources.index(
            "void theme_set(int spacetype"
        )]
        copy = reset.index("MEMCPY_STRUCT_AFTER(btheme, &U_theme_default, name);")
        apply = reset.index("BLO_userdef_theme_apply_platform(btheme);")
        self.assertLess(copy, apply)

        self.assertIn("userdef_platform_theme.cc", BLENLOADER_CMAKE.read_text())
        self.assertIn("-DBLENDER_PLATFORM_IOS_THEME", IOS_PLATFORM.read_text())


if __name__ == "__main__":
    unittest.main()
