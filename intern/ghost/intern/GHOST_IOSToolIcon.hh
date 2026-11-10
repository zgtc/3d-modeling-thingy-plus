/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * Blender's own tool icons for the iPad's rail.
 *
 * Blender draws its toolbar icons from triangle geometry in `datafiles/icons/<name>.dat`
 * (written by `release/datafiles/blender_icons_geom.py`): the bytes `VCO\0`, the icon's width and
 * height, two bytes of origin, then every triangle's three corners as one byte x and y each, and
 * after all corners every corner's RGBA. This reads that format; the rail draws it natively, so
 * the icons match Blender's toolbar exactly. Platform-neutral so it can be tested on its own.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

struct GHOST_IOSToolIconGeometry {
  int width = 0;
  int height = 0;
  int triangles = 0;
  /** Corner coordinates, then corner colours, inside the file's bytes. */
  const uint8_t *coords = nullptr;
  const uint8_t *colors = nullptr;
};

struct GHOST_IOSToolIconTriangle {
  /** Corners in icon units, y running down. */
  float x[3];
  float y[3];
  /** The average of the corners' colours. */
  uint8_t rgba[4];
};

/** Read icon geometry from \a data. False for anything that is not a complete icon. */
inline bool GHOST_IOSToolIcon_parse(const uint8_t *data,
                                    const size_t size,
                                    GHOST_IOSToolIconGeometry *r_icon)
{
  constexpr size_t header_size = 8;
  constexpr size_t triangle_size = 3 * 2 + 3 * 4;
  if (data == nullptr || size < header_size || std::memcmp(data, "VCO\0", 4) != 0) {
    return false;
  }
  const size_t body = size - header_size;
  if (body == 0 || body % triangle_size != 0 || data[4] == 0 || data[5] == 0) {
    return false;
  }
  r_icon->width = data[4];
  r_icon->height = data[5];
  r_icon->triangles = int(body / triangle_size);
  r_icon->coords = data + header_size;
  r_icon->colors = data + header_size + size_t(r_icon->triangles) * 3 * 2;
  return true;
}

inline void GHOST_IOSToolIcon_triangle(const GHOST_IOSToolIconGeometry &icon,
                                       const int index,
                                       GHOST_IOSToolIconTriangle *r_triangle)
{
  int sum[4] = {0, 0, 0, 0};
  for (int corner = 0; corner < 3; corner++) {
    const int vertex = index * 3 + corner;
    r_triangle->x[corner] = float(icon.coords[vertex * 2]);
    r_triangle->y[corner] = float(icon.height - icon.coords[vertex * 2 + 1]);
    for (int channel = 0; channel < 4; channel++) {
      sum[channel] += icon.colors[vertex * 4 + channel];
    }
  }
  for (int channel = 0; channel < 4; channel++) {
    r_triangle->rgba[channel] = uint8_t((sum[channel] + 1) / 3);
  }
}

#ifdef __OBJC__
@class UIImage;
/**
 * Blender's icon \a name drawn \a points square, read from the app's `datafiles/icons`, or nil
 * when the app does not ship it. Images are cached.
 */
UIImage *GHOST_IOSToolIcon_image(const char *name, double points);
#endif
