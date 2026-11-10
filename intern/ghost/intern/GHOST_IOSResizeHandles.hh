/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * Visible, finger-sized grab handles on the resizable edges of Blender's sidebars and toolbars.
 *
 * Blender's own resize zone is a few pixels wide and invisible until hovered. Each handle is a
 * pill on that edge with a full control-sized hit target; dragging it presses and drags on the
 * edge itself, so Blender's region resizing does the work and saved layouts are unchanged.
 */

#pragma once

#include "GHOST_IOSInteraction.hh"

#include <algorithm>

/** Hit target of one handle, in view points. */
struct GHOST_IOSHandleFrame {
  double x;
  double y;
  double width;
  double height;
};

/** Width of the hit target, centered on the edge. */
constexpr double GHOST_IOS_RESIZE_HANDLE_WIDTH = 28.0;
/** Height of the hit target; matches #GHOST_IOS_CONTROL_SIZE. */
constexpr double GHOST_IOS_RESIZE_HANDLE_HEIGHT = 44.0;
/** The visible pill inside the hit target. */
constexpr double GHOST_IOS_RESIZE_PILL_WIDTH = 6.0;
constexpr double GHOST_IOS_RESIZE_PILL_HEIGHT = 36.0;
/** Edges shorter than this, in points, get no handle. */
constexpr double GHOST_IOS_RESIZE_MIN_EDGE = 60.0;

/**
 * Where the handle for \a edge goes, in view points, given GHOST client pixels per point
 * (\a scale). Returns false when the edge is too short for a handle.
 */
inline bool GHOST_IOSResizeHandle_frame(const GHOST_IOSResizeEdge &edge,
                                        const double scale,
                                        GHOST_IOSHandleFrame *r_frame)
{
  if (scale <= 0.0) {
    return false;
  }
  const double x = edge.x / scale;
  const double top = std::min(edge.y_min, edge.y_max) / scale;
  const double bottom = std::max(edge.y_min, edge.y_max) / scale;
  if (bottom - top < GHOST_IOS_RESIZE_MIN_EDGE) {
    return false;
  }
  const double center_y = (top + bottom) * 0.5;
  r_frame->width = GHOST_IOS_RESIZE_HANDLE_WIDTH;
  r_frame->height = GHOST_IOS_RESIZE_HANDLE_HEIGHT;
  r_frame->x = x - GHOST_IOS_RESIZE_HANDLE_WIDTH * 0.5;
  r_frame->y = center_y - GHOST_IOS_RESIZE_HANDLE_HEIGHT * 0.5;
  return true;
}

/** Where to press so Blender starts resizing: on the edge, level with the pill. */
inline void GHOST_IOSResizeHandle_pressPoint(const GHOST_IOSResizeEdge &edge, int *r_x, int *r_y)
{
  *r_x = edge.x;
  *r_y = (edge.y_min + edge.y_max) / 2;
}

#ifdef __OBJC__

#  import <CoreGraphics/CoreGraphics.h>
#  import <Foundation/Foundation.h>

@class UIView;

/** Drags on a handle, in GHOST client pixels. */
@protocol GHOSTIOSResizeHandlesDelegate <NSObject>
- (void)resizeHandlePressAt:(CGPoint)client_point;
- (void)resizeHandleDragTo:(CGPoint)client_point;
- (void)resizeHandleReleaseAt:(CGPoint)client_point;
@end

NSObject *GHOST_IOSResizeHandles_create(UIView *input_view,
                                        id<GHOSTIOSResizeHandlesDelegate> delegate);
void GHOST_IOSResizeHandles_destroy(NSObject *handles);
/** Show one handle per edge. \a scale is GHOST client pixels per view point. */
void GHOST_IOSResizeHandles_setEdges(NSObject *handles,
                                     const GHOST_IOSResizeEdge *edges,
                                     int count,
                                     double scale);
bool GHOST_IOSResizeHandles_containsView(NSObject *handles, UIView *view);

#endif
