/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup wm
 *
 * Fit the splash screen into a short window. The menu under the image has a fixed height for the
 * UI scale, so on a screen where native bars take part of the height (the iPad's header and quick
 * bar) the image gives up the room instead of the popup running off the top and bottom.
 * Platform-neutral so it can be tested on its own.
 */

#pragma once

/** Room kept free above and below the splash, in pixels. */
constexpr int WM_SPLASH_FIT_MARGIN = 16;
/** Below this height the image is dropped rather than shown as a sliver, in pixels. */
constexpr int WM_SPLASH_FIT_MIN_IMAGE_HEIGHT = 96;

struct WM_SplashImageFit {
  /** Size of the image, 0 when it is dropped. */
  int width;
  int height;
  /** Horizontal offset that centers the image over a menu of the splash's full width. */
  int x;
};

/**
 * \param splash_width: Width of the splash's menu, and the image's width when it fits.
 * \param image_width, image_height: The image's size at \a splash_width.
 * \param content_height: Height of the menu under the image.
 * \param window_height: Height of the window the splash is centered in.
 * \param gap: Space between the image and the menu.
 */
inline WM_SplashImageFit wm_splash_fit_image(const int splash_width,
                                             const int image_width,
                                             const int image_height,
                                             const int content_height,
                                             const int window_height,
                                             const int gap)
{
  WM_SplashImageFit fit = {image_width, image_height, 0};
  if (image_width <= 0 || image_height <= 0) {
    fit.width = fit.height = 0;
    return fit;
  }
  const int available = window_height - content_height - gap - WM_SPLASH_FIT_MARGIN * 2;
  if (image_height <= available) {
    return fit;
  }
  if (available < WM_SPLASH_FIT_MIN_IMAGE_HEIGHT) {
    fit.width = fit.height = 0;
    return fit;
  }
  fit.height = available;
  fit.width = int((long long)available * image_width / image_height);
  fit.x = (splash_width - fit.width) / 2;
  return fit;
}
