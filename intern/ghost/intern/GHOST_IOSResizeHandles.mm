/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * Visible, finger-sized grab handles on the resizable edges of Blender's sidebars and toolbars.
 */

#import <UIKit/UIKit.h>

#include "GHOST_IOSResizeHandles.hh"
#include "GHOST_IOSStyle.hh"

/* -------------------------------------------------------------------- */
/** \name Handle
 * \{ */

@interface GHOSTIOSResizeHandleView : UIView
{
  id<GHOSTIOSResizeHandlesDelegate> delegate_;
  UIView *pill_;
  UILongPressGestureRecognizer *press_;
  GHOST_IOSResizeEdge edge_;
  double scale_;
  /* Fixed for the whole drag: Blender moves the edge as the region resizes. */
  CGPoint press_client_;
  CGPoint press_location_;
}
- (instancetype)initWithDelegate:(id<GHOSTIOSResizeHandlesDelegate>)delegate;
- (void)setEdge:(const GHOST_IOSResizeEdge &)edge
          frame:(const GHOST_IOSHandleFrame &)frame
          scale:(double)scale;
- (void)forgetDelegate;
@end

@implementation GHOSTIOSResizeHandleView

- (instancetype)initWithDelegate:(id<GHOSTIOSResizeHandlesDelegate>)delegate
{
  self = [super initWithFrame:CGRectMake(0.0f,
                                         0.0f,
                                         GHOST_IOS_RESIZE_HANDLE_WIDTH,
                                         GHOST_IOS_RESIZE_HANDLE_HEIGHT)];
  if (self == nil) {
    return nil;
  }
  delegate_ = delegate;
  scale_ = 1.0;
  self.backgroundColor = UIColor.clearColor;
  self.isAccessibilityElement = YES;
  self.accessibilityLabel = @"Resize panel";
  self.accessibilityHint = @"Drag to resize the panel";
  self.accessibilityIdentifier = @"blender_resize_handle";

  pill_ = [[UIView alloc]
      initWithFrame:CGRectMake((GHOST_IOS_RESIZE_HANDLE_WIDTH - GHOST_IOS_RESIZE_PILL_WIDTH) * 0.5,
                               (GHOST_IOS_RESIZE_HANDLE_HEIGHT - GHOST_IOS_RESIZE_PILL_HEIGHT) *
                                   0.5,
                               GHOST_IOS_RESIZE_PILL_WIDTH,
                               GHOST_IOS_RESIZE_PILL_HEIGHT)];
  pill_.userInteractionEnabled = NO;
  pill_.layer.cornerRadius = GHOST_IOS_RESIZE_PILL_WIDTH * 0.5;
  pill_.layer.cornerCurve = kCACornerCurveContinuous;
  /* Readable on light and dark editors alike, like the system's split view divider handle. */
  pill_.layer.borderWidth = 1.0;
  pill_.layer.borderColor = [UIColor colorWithWhite:0.0f alpha:0.35f].CGColor;
  [self setActive:NO];
  [self addSubview:pill_];

  /* Starts on touch down, so the drag follows the finger from the first point. */
  press_ = [[UILongPressGestureRecognizer alloc] initWithTarget:self
                                                         action:@selector(handlePress:)];
  press_.minimumPressDuration = 0.0;
  press_.allowableMovement = CGFLOAT_MAX;
  [self addGestureRecognizer:press_];
  return self;
}

- (void)dealloc
{
  [pill_ release];
  [press_ release];
  [super dealloc];
}

- (void)forgetDelegate
{
  delegate_ = nil;
  [self removeGestureRecognizer:press_];
}

- (void)setActive:(BOOL)active
{
  pill_.backgroundColor = active ? GHOST_IOSStyle_accentColor() :
                                   [UIColor colorWithWhite:1.0f alpha:0.7f];
  pill_.transform = active ? CGAffineTransformMakeScale(1.3f, 1.1f) : CGAffineTransformIdentity;
}

- (void)setEdge:(const GHOST_IOSResizeEdge &)edge
          frame:(const GHOST_IOSHandleFrame &)frame
          scale:(double)scale
{
  edge_ = edge;
  scale_ = scale;
  self.frame = CGRectMake(frame.x, frame.y, frame.width, frame.height);
}

- (void)handlePress:(UILongPressGestureRecognizer *)sender
{
  const CGPoint location = [sender locationInView:self.superview];
  switch (sender.state) {
    case UIGestureRecognizerStateBegan: {
      int x = 0, y = 0;
      GHOST_IOSResizeHandle_pressPoint(edge_, &x, &y);
      press_client_ = CGPointMake(x, y);
      press_location_ = location;
      [self setActive:YES];
      GHOST_IOSStyle_selectionFeedback();
      [delegate_ resizeHandlePressAt:press_client_];
      break;
    }
    case UIGestureRecognizerStateChanged: {
      /* Only the horizontal motion resizes a sidebar. */
      const CGFloat dx = (location.x - press_location_.x) * scale_;
      [delegate_ resizeHandleDragTo:CGPointMake(press_client_.x + dx, press_client_.y)];
      break;
    }
    case UIGestureRecognizerStateEnded:
    case UIGestureRecognizerStateCancelled:
    case UIGestureRecognizerStateFailed: {
      const CGFloat dx = (location.x - press_location_.x) * scale_;
      [self setActive:NO];
      [delegate_ resizeHandleReleaseAt:CGPointMake(press_client_.x + dx, press_client_.y)];
      break;
    }
    default:
      break;
  }
}

@end

/** \} */

/* -------------------------------------------------------------------- */
/** \name Handle set
 * \{ */

@interface GHOSTIOSResizeHandles : NSObject
{
  UIView *input_view_;
  id<GHOSTIOSResizeHandlesDelegate> delegate_;
  NSMutableArray<GHOSTIOSResizeHandleView *> *handles_;
}
- (instancetype)initWithInputView:(UIView *)input_view
                         delegate:(id<GHOSTIOSResizeHandlesDelegate>)delegate;
- (void)teardown;
- (void)setEdges:(const GHOST_IOSResizeEdge *)edges count:(int)count scale:(double)scale;
- (BOOL)containsView:(UIView *)view;
@end

@implementation GHOSTIOSResizeHandles

- (instancetype)initWithInputView:(UIView *)input_view
                         delegate:(id<GHOSTIOSResizeHandlesDelegate>)delegate
{
  self = [super init];
  if (self == nil) {
    return nil;
  }
  input_view_ = input_view;
  delegate_ = delegate;
  handles_ = [[NSMutableArray alloc] init];
  return self;
}

- (void)dealloc
{
  [handles_ release];
  [super dealloc];
}

- (void)teardown
{
  for (GHOSTIOSResizeHandleView *handle in handles_) {
    [handle forgetDelegate];
    [handle removeFromSuperview];
  }
  [handles_ removeAllObjects];
  delegate_ = nil;
  input_view_ = nil;
}

- (void)setEdges:(const GHOST_IOSResizeEdge *)edges count:(int)count scale:(double)scale
{
  if (input_view_ == nil) {
    return;
  }
  int used = 0;
  for (int i = 0; i < count; i++) {
    GHOST_IOSHandleFrame frame;
    if (!GHOST_IOSResizeHandle_frame(edges[i], scale, &frame)) {
      continue;
    }
    if (used == int(handles_.count)) {
      GHOSTIOSResizeHandleView *handle = [[[GHOSTIOSResizeHandleView alloc]
          initWithDelegate:delegate_] autorelease];
      [handles_ addObject:handle];
      [input_view_ addSubview:handle];
    }
    GHOSTIOSResizeHandleView *handle = handles_[used++];
    [handle setEdge:edges[i] frame:frame scale:scale];
    handle.hidden = NO;
  }
  for (int i = used; i < int(handles_.count); i++) {
    handles_[i].hidden = YES;
  }
}

- (BOOL)containsView:(UIView *)view
{
  for (GHOSTIOSResizeHandleView *handle in handles_) {
    if (!handle.hidden && [view isDescendantOfView:handle]) {
      return YES;
    }
  }
  return NO;
}

@end

/** \} */

/* -------------------------------------------------------------------- */
/** \name C API
 * \{ */

NSObject *GHOST_IOSResizeHandles_create(UIView *input_view,
                                        id<GHOSTIOSResizeHandlesDelegate> delegate)
{
  if (input_view == nil) {
    return nil;
  }
  return [[GHOSTIOSResizeHandles alloc] initWithInputView:input_view delegate:delegate];
}

void GHOST_IOSResizeHandles_destroy(NSObject *handles)
{
  if (handles == nil) {
    return;
  }
  [(GHOSTIOSResizeHandles *)handles teardown];
  [handles release];
}

void GHOST_IOSResizeHandles_setEdges(NSObject *handles,
                                     const GHOST_IOSResizeEdge *edges,
                                     int count,
                                     double scale)
{
  [(GHOSTIOSResizeHandles *)handles setEdges:edges count:count scale:scale];
}

bool GHOST_IOSResizeHandles_containsView(NSObject *handles, UIView *view)
{
  if (handles == nil || view == nil) {
    return false;
  }
  return [(GHOSTIOSResizeHandles *)handles containsView:view];
}

/** \} */
