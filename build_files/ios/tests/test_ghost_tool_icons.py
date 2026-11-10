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
ICON_HEADER = GHOST_INTERN / "GHOST_IOSToolIcon.hh"
CHROME_HEADER = GHOST_INTERN / "GHOST_IOSChrome.hh"
BELT_SOURCE = GHOST_INTERN / "GHOST_IOSModifierBelt.mm"
ICON_SOURCE = GHOST_INTERN / "GHOST_IOSToolIcon.mm"
GHOST_CMAKE = REPOSITORY / "build_files" / "ios" / "cmake" / "ghost_ios.cmake"
DATAFILES_CMAKE = REPOSITORY / "source" / "blender" / "editors" / "datafiles" / "CMakeLists.txt"


def run_harness(test: unittest.TestCase, body: str) -> None:
    harness = textwrap.dedent(
        """
        #include "GHOST_IOSChrome.hh"
        #include "GHOST_IOSToolIcon.hh"
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
        source = Path(directory) / "tool_icon_test.cc"
        executable = Path(directory) / "tool_icon_test"
        source.write_text(harness)
        result = subprocess.run(
            ["xcrun", "clang++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
             "-I", str(GHOST), "-I", str(GHOST_INTERN),
             "-I", str(REPOSITORY / "intern" / "guardedalloc"),
             str(source), "-o", str(executable)],
            capture_output=True, text=True)
        test.assertEqual(result.returncode, 0, result.stderr)
        result = subprocess.run([str(executable)], capture_output=True, text=True)
        test.assertEqual(result.returncode, 0, result.stdout + result.stderr)


class ToolIconGeometryTests(unittest.TestCase):
    """Item 14 of the device feedback on #5: the rail shows Blender's own tool icons."""

    def test_icon_geometry_reads_blenders_triangles(self) -> None:
        run_harness(self, """
            /* "VCO", width 96, height 96, start 0 0, one triangle: three corners then RGBA each. */
            const uint8_t data[] = {'V', 'C', 'O', 0, 96, 96, 0, 0,
                                    10, 20, 30, 40, 50, 60,
                                    255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255};
            GHOST_IOSToolIconGeometry icon;
            CHECK(GHOST_IOSToolIcon_parse(data, sizeof(data), &icon));
            CHECK(icon.width == 96 && icon.height == 96 && icon.triangles == 1);
            GHOST_IOSToolIconTriangle tri;
            GHOST_IOSToolIcon_triangle(icon, 0, &tri);
            /* Blender's y runs up; the triangle comes out with y running down. */
            CHECK(tri.x[0] == 10 && tri.y[0] == 96 - 20);
            CHECK(tri.x[2] == 50 && tri.y[2] == 96 - 60);
            CHECK(tri.rgba[0] == 255 && tri.rgba[1] == 0 && tri.rgba[3] == 255);
            /* Anything that is not icon geometry is refused. */
            CHECK(!GHOST_IOSToolIcon_parse(data, 7, &icon));
            const uint8_t pointer[] = "version https://git-lfs.github.com/spec/v1";
            CHECK(!GHOST_IOSToolIcon_parse(pointer, sizeof(pointer), &icon));
            const uint8_t short_body[] = {'V', 'C', 'O', 0, 96, 96, 0, 0, 1, 2, 3};
            CHECK(!GHOST_IOSToolIcon_parse(short_body, sizeof(short_body), &icon));
        """)

    def test_every_rail_tool_names_an_icon_blender_ships(self) -> None:
        shipped = set(re.findall(r"^\s+([a-z_.0-9]+)$",
                                 DATAFILES_CMAKE.read_text().split("# BEGIN ICON_GEOM_NAMES")[1]
                                 .split("# END ICON_GEOM_NAMES")[0], re.M))
        self.assertIn("ops.transform.translate", shipped)
        chrome = CHROME_HEADER.read_text()
        entries = re.findall(
            r'\{"([^"]+)",\s*"([^"]+)",\s*"(builtin\.[a-z_]+)",\s*"([a-z_.0-9]+)"\}', chrome)
        self.assertGreaterEqual(len(entries), 30)
        for title, symbol, idname, icon in entries:
            self.assertIn(icon, shipped, f"{title} ({idname}) names {icon}")
        expected = {"builtin.move": "ops.transform.translate", "builtin.bevel": "ops.mesh.bevel",
                    "builtin.select_box": "ops.generic.select_box", "builtin.brush": "brush.sculpt"}
        mapping = {idname: icon for _, _, idname, icon in entries}
        for idname, icon in expected.items():
            self.assertEqual(mapping[idname], icon)

    def test_rail_draws_the_shipped_icons_with_a_symbol_fallback(self) -> None:
        self.assertIn("GHOST_IOSToolIcon.mm", GHOST_CMAKE.read_text())
        icon = ICON_SOURCE.read_text()
        self.assertIn('@"datafiles/icons"', icon)
        self.assertIn("GHOST_IOSToolIcon_parse(", icon)
        self.assertIn("CGContextFillPath", icon)
        self.assertIn("NSCache", icon)
        belt = BELT_SOURCE.read_text()
        tools = belt[belt.index("- (void)setTools:(const GHOST_IOSToolItem *)tools count:(int)count active:(int)active\n{") :]
        tools = tools[: tools.index("\n}\n")]
        self.assertIn("GHOST_IOSToolIcon_image(tools[i].icon", tools)
        self.assertIn("GHOST_IOSStyle_symbol(@(tools[i].symbol)", tools)


if __name__ == "__main__":
    unittest.main()
