/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * Native Apple Pencil palette, double tap prompt and touch settings for iPad.
 *
 * The palette is a ring of eight slots that opens under the Pencil tip. Touching or hovering a
 * slot highlights it with a Pencil haptic; lifting on a slot runs it and closes the palette, and
 * lifting anywhere else closes it. The slot sets and the ring geometry live in
 * #GHOST_IOSTouchKeys.hh, so this file is only the view.
 */

#import <UIKit/UIKit.h>
#import <os/log.h>

#include "GHOST_IOSPencilPalette.hh"
#include "GHOST_IOSStyle.hh"

#include <algorithm>

static constexpr CGFloat slot_size = 64.0f;
/** Distance from the palette center to each slot center. */
static constexpr CGFloat ring_radius = 88.0f;
static constexpr CGFloat inner_radius = 30.0f;
static constexpr CGFloat outer_radius = ring_radius + slot_size * 0.5f + 8.0f;
static constexpr CGFloat screen_margin = 12.0f;

static NSString *const GHOST_IOSPencilDoubleTapKey = @"GHOST_IOSPencilDoubleTap";
static NSString *const GHOST_IOSPencilSqueezeKey = @"GHOST_IOSPencilSqueeze";
static NSString *const GHOST_IOSPencilOnlyDrawingKey = @"GHOST_IOSPencilOnlyDrawing";
static NSString *const GHOST_IOSTapGestureKeys[GHOST_IOS_TAP_GESTURE_COUNT] = {
    @"GHOST_IOSTapGesture2",
    @"GHOST_IOSTapGesture3",
    @"GHOST_IOSTapGesture4",
};

static os_log_t GHOST_IOSPencilPaletteLog()
{
  static os_log_t log = os_log_create("org.blender.ios", "pencil-palette");
  return log;
}

/* -------------------------------------------------------------------- */
/** \name Slot
 * \{ */

@interface GHOSTIOSPaletteSlotView : UIView
{
  UIVisualEffectView *background_;
  UIView *highlight_;
  UIImageView *icon_;
  UILabel *label_;
}
- (void)setSlot:(const GHOST_IOSPaletteSlot &)slot;
- (void)setHighlighted:(BOOL)highlighted;
@end

@implementation GHOSTIOSPaletteSlotView

- (instancetype)init
{
  self = [super initWithFrame:CGRectMake(0.0f, 0.0f, slot_size, slot_size)];
  if (self == nil) {
    return nil;
  }
  self.userInteractionEnabled = NO;
  self.isAccessibilityElement = YES;
  self.accessibilityTraits = UIAccessibilityTraitButton;

  background_ = [GHOST_IOSStyle_createMaterialView(slot_size * 0.5f) retain];
  background_.translatesAutoresizingMaskIntoConstraints = YES;
  background_.frame = self.bounds;
  [self addSubview:background_];

  highlight_ = [[UIView alloc] initWithFrame:self.bounds];
  highlight_.layer.cornerRadius = slot_size * 0.5f;
  highlight_.backgroundColor = GHOST_IOSStyle_accentColor();
  highlight_.alpha = 0.0f;
  [self addSubview:highlight_];

  icon_ = [[UIImageView alloc] initWithFrame:CGRectMake(0.0f, 10.0f, slot_size, 26.0f)];
  icon_.contentMode = UIViewContentModeCenter;
  icon_.tintColor = UIColor.whiteColor;
  [self addSubview:icon_];

  label_ = [[UILabel alloc] initWithFrame:CGRectMake(4.0f, 38.0f, slot_size - 8.0f, 14.0f)];
  label_.font = [UIFont systemFontOfSize:10.0f weight:UIFontWeightSemibold];
  label_.textColor = UIColor.whiteColor;
  label_.textAlignment = NSTextAlignmentCenter;
  label_.adjustsFontSizeToFitWidth = YES;
  label_.minimumScaleFactor = 0.7f;
  [self addSubview:label_];
  return self;
}

- (void)dealloc
{
  [background_ release];
  [highlight_ release];
  [icon_ release];
  [label_ release];
  [super dealloc];
}

- (void)setSlot:(const GHOST_IOSPaletteSlot &)slot
{
  NSString *title = [NSString stringWithUTF8String:slot.title];
  icon_.image = GHOST_IOSStyle_symbol([NSString stringWithUTF8String:slot.symbol], 19.0f);
  label_.text = title;
  self.accessibilityLabel = title;
}

- (void)setHighlighted:(BOOL)highlighted
{
  highlight_.alpha = highlighted ? 1.0f : 0.0f;
  self.transform = highlighted ? CGAffineTransformMakeScale(1.12f, 1.12f) :
                                 CGAffineTransformIdentity;
}

@end

/** \} */

/* -------------------------------------------------------------------- */
/** \name Palette
 * \{ */

@interface GHOSTIOSPencilPaletteView : UIView
{
  id<GHOSTIOSPencilPaletteDelegate> delegate_;
  NSMutableArray<GHOSTIOSPaletteSlotView *> *slot_views_;
  UIView *center_;
  UIHoverGestureRecognizer *hover_;
  UICanvasFeedbackGenerator *feedback_;
  const GHOST_IOSPaletteSlot *slots_;
  int count_;
  CGPoint center_point_;
  int highlighted_;
}
- (instancetype)initWithInputView:(UIView *)input_view
                         delegate:(id<GHOSTIOSPencilPaletteDelegate>)delegate;
- (void)teardown;
- (void)showAt:(CGPoint)point slots:(const GHOST_IOSPaletteSlot *)slots count:(int)count;
- (void)hide;
@end

@implementation GHOSTIOSPencilPaletteView

- (instancetype)initWithInputView:(UIView *)input_view
                         delegate:(id<GHOSTIOSPencilPaletteDelegate>)delegate
{
  self = [super initWithFrame:input_view.bounds];
  if (self == nil) {
    return nil;
  }
  delegate_ = delegate;
  slots_ = nullptr;
  count_ = 0;
  highlighted_ = -1;
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  self.backgroundColor = UIColor.clearColor;
  self.overrideUserInterfaceStyle = UIUserInterfaceStyleDark;
  self.accessibilityIdentifier = @"blender_pencil_palette";
  self.hidden = YES;
  /* The palette takes a Pencil touch even when the system routes Pencil touches as a mouse. */
  self.multipleTouchEnabled = NO;

  slot_views_ = [[NSMutableArray alloc] init];
  for (int i = 0; i < GHOST_IOS_PALETTE_SLOT_COUNT; i++) {
    GHOSTIOSPaletteSlotView *slot = [[[GHOSTIOSPaletteSlotView alloc] init] autorelease];
    slot.accessibilityIdentifier = [NSString
        stringWithFormat:@"blender_pencil_palette_slot_%d", i];
    [slot_views_ addObject:slot];
    [self addSubview:slot];
  }

  center_ = [[UIImageView alloc] initWithImage:GHOST_IOSStyle_symbol(@"xmark", 14.0f)];
  center_.frame = CGRectMake(0.0f, 0.0f, inner_radius * 2.0f, inner_radius * 2.0f);
  center_.contentMode = UIViewContentModeCenter;
  center_.tintColor = [UIColor colorWithWhite:1.0f alpha:0.8f];
  center_.userInteractionEnabled = NO;
  [self addSubview:center_];

  hover_ = [[UIHoverGestureRecognizer alloc] initWithTarget:self action:@selector(handleHover:)];
  [self addGestureRecognizer:hover_];
  feedback_ = [[UICanvasFeedbackGenerator feedbackGeneratorForView:self] retain];

  [input_view addSubview:self];
  return self;
}

- (void)teardown
{
  [self removeGestureRecognizer:hover_];
  delegate_ = nil;
  slots_ = nullptr;
  [self removeFromSuperview];
}

- (void)dealloc
{
  [slot_views_ release];
  [center_ release];
  [hover_ release];
  [feedback_ release];
  [super dealloc];
}

- (void)showAt:(CGPoint)point slots:(const GHOST_IOSPaletteSlot *)slots count:(int)count
{
  UIView *parent = self.superview;
  if (parent == nil || slots == nullptr || count <= 0) {
    return;
  }
  slots_ = slots;
  count_ = std::min(count, int(slot_views_.count));
  self.frame = parent.bounds;
  const GHOST_IOSPalettePoint center = GHOST_IOSPalette_clampCenter({point.x, point.y},
                                                                    outer_radius,
                                                                    screen_margin,
                                                                    parent.bounds.size.width,
                                                                    parent.bounds.size.height);
  center_point_ = CGPointMake(center.x, center.y);
  center_.center = center_point_;

  for (int i = 0; i < int(slot_views_.count); i++) {
    GHOSTIOSPaletteSlotView *view = slot_views_[i];
    view.hidden = i >= count_;
    if (i >= count_) {
      continue;
    }
    [view setSlot:slots_[i]];
    [view setHighlighted:NO];
    const GHOST_IOSPalettePoint slot_center = GHOST_IOSPalette_slotCenter(
        center, ring_radius, i, count_);
    view.center = CGPointMake(slot_center.x, slot_center.y);
  }
  highlighted_ = -1;
  os_log_info(GHOST_IOSPencilPaletteLog(), "Pencil palette opened");

  [parent bringSubviewToFront:self];
  self.hidden = NO;
  self.alpha = 0.0f;
  self.transform = CGAffineTransformMakeScale(0.85f, 0.85f);
  [feedback_ prepare];
  [UIView animateWithDuration:0.22
                        delay:0.0
       usingSpringWithDamping:0.82f
        initialSpringVelocity:0.0f
                      options:UIViewAnimationOptionAllowUserInteraction
                   animations:^{
                     self.alpha = 1.0f;
                     self.transform = CGAffineTransformIdentity;
                   }
                   completion:nil];
}

- (void)hide
{
  if (self.hidden) {
    return;
  }
  [self highlight:-1];
  slots_ = nullptr;
  [UIView animateWithDuration:0.12
      delay:0.0
      options:UIViewAnimationOptionBeginFromCurrentState
      animations:^{
        self.alpha = 0.0f;
      }
      completion:^(BOOL) {
        if (self.alpha == 0.0f) {
          self.hidden = YES;
        }
      }];
}

- (int)slotAtPoint:(CGPoint)point
{
  return GHOST_IOSPalette_slotAt(
      {center_point_.x, center_point_.y}, inner_radius, outer_radius, count_, {point.x, point.y});
}

- (void)highlight:(int)index
{
  if (index == highlighted_) {
    return;
  }
  if (highlighted_ >= 0) {
    [slot_views_[highlighted_] setHighlighted:NO];
  }
  highlighted_ = index;
  if (index >= 0) {
    GHOSTIOSPaletteSlotView *view = slot_views_[index];
    [view setHighlighted:YES];
    /* The Pencil Pro taps in the hand; other Pencils fall back to the device haptic. */
    [feedback_ alignmentOccurredAtLocation:view.center];
  }
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
  (void)event;
  [self highlight:[self slotAtPoint:[touches.anyObject locationInView:self]]];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
  (void)event;
  [self highlight:[self slotAtPoint:[touches.anyObject locationInView:self]]];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
  (void)event;
  const int index = [self slotAtPoint:[touches.anyObject locationInView:self]];
  [self hide];
  if (index >= 0) {
    [delegate_ pencilPaletteChoseSlot:index];
  }
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
  (void)touches;
  (void)event;
  [self highlight:-1];
}

- (void)handleHover:(UIHoverGestureRecognizer *)sender
{
  if (sender.state == UIGestureRecognizerStateBegan ||
      sender.state == UIGestureRecognizerStateChanged)
  {
    [self highlight:[self slotAtPoint:[sender locationInView:self]]];
  }
  else {
    [self highlight:-1];
  }
}

- (BOOL)accessibilityPerformEscape
{
  [self hide];
  return YES;
}

@end

/** \} */

/* -------------------------------------------------------------------- */
/** \name Settings
 * \{ */

GHOST_IOSTouchSettings GHOST_IOSTouchSettings_load()
{
  NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
  GHOST_IOSTouchSettings settings;
  settings.double_tap = GHOST_IOSDoubleTapChoice_fromStored(
      [defaults integerForKey:GHOST_IOSPencilDoubleTapKey]);
  settings.squeeze = GHOST_IOSSqueezeChoice_fromStored(
      [defaults integerForKey:GHOST_IOSPencilSqueezeKey]);
  /* Until the user picks, follow the system "Only Draw with Apple Pencil" setting. */
  settings.pencil_only_drawing = [defaults objectForKey:GHOST_IOSPencilOnlyDrawingKey] != nil ?
                                     [defaults boolForKey:GHOST_IOSPencilOnlyDrawingKey] :
                                     UIPencilInteraction.prefersPencilOnlyDrawing;
  for (int i = 0; i < GHOST_IOS_TAP_GESTURE_COUNT; i++) {
    if ([defaults objectForKey:GHOST_IOSTapGestureKeys[i]] != nil) {
      settings.tap_gestures[i] = GHOST_IOSGestureAction_fromStored(
          [defaults integerForKey:GHOST_IOSTapGestureKeys[i]], settings.tap_gestures[i]);
    }
  }
  return settings;
}

void GHOST_IOSTouchSettings_save(const GHOST_IOSTouchSettings &settings)
{
  NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
  [defaults setInteger:NSInteger(settings.double_tap) forKey:GHOST_IOSPencilDoubleTapKey];
  [defaults setInteger:NSInteger(settings.squeeze) forKey:GHOST_IOSPencilSqueezeKey];
  [defaults setBool:settings.pencil_only_drawing forKey:GHOST_IOSPencilOnlyDrawingKey];
  for (int i = 0; i < GHOST_IOS_TAP_GESTURE_COUNT; i++) {
    [defaults setInteger:NSInteger(settings.tap_gestures[i]) forKey:GHOST_IOSTapGestureKeys[i]];
  }
}

static GHOST_IOSSystemPencilAction GHOST_IOSPencil_systemAction(
    const UIPencilPreferredAction action)
{
  switch (action) {
    case UIPencilPreferredActionIgnore:
      return GHOST_IOSSystemPencilAction::Ignore;
    case UIPencilPreferredActionSwitchEraser:
      return GHOST_IOSSystemPencilAction::SwitchEraser;
    case UIPencilPreferredActionSwitchPrevious:
      return GHOST_IOSSystemPencilAction::SwitchPrevious;
    case UIPencilPreferredActionShowColorPalette:
      return GHOST_IOSSystemPencilAction::ShowColorPalette;
    case UIPencilPreferredActionShowInkAttributes:
      return GHOST_IOSSystemPencilAction::ShowInkAttributes;
    case UIPencilPreferredActionShowContextualPalette:
      return GHOST_IOSSystemPencilAction::ShowContextualPalette;
    case UIPencilPreferredActionRunSystemShortcut:
      return GHOST_IOSSystemPencilAction::RunSystemShortcut;
  }
  return GHOST_IOSSystemPencilAction::Ignore;
}

GHOST_IOSSystemPencilAction GHOST_IOSPencil_systemTapAction()
{
  return GHOST_IOSPencil_systemAction(UIPencilInteraction.preferredTapAction);
}

GHOST_IOSSystemPencilAction GHOST_IOSPencil_systemSqueezeAction()
{
  return GHOST_IOSPencil_systemAction(UIPencilInteraction.preferredSqueezeAction);
}

struct GHOST_IOSDoubleTapOption {
  GHOST_IOSDoubleTapChoice choice;
  NSString *title;
  NSString *symbol;
};

static const GHOST_IOSDoubleTapOption GHOST_IOS_DOUBLE_TAP_OPTIONS[] = {
    {GHOST_IOSDoubleTapChoice::RightClick, @"Right Click", @"cursorarrow.click"},
    {GHOST_IOSDoubleTapChoice::PreviousTool, @"Previous Tool", @"arrow.uturn.left"},
    {GHOST_IOSDoubleTapChoice::Palette, @"Show Palette", @"circle.grid.cross"},
    {GHOST_IOSDoubleTapChoice::Eraser, @"Invert Brush", @"eraser"},
    {GHOST_IOSDoubleTapChoice::System, @"Follow System Setting", @"gear"},
};

void GHOST_IOSPencil_askDoubleTap(UIViewController *presenter,
                                  UIView *source_view,
                                  CGPoint point,
                                  void (^chosen)(GHOST_IOSDoubleTapChoice choice))
{
  if (presenter == nil || presenter.presentedViewController != nil) {
    return;
  }
  UIAlertController *sheet = [UIAlertController
      alertControllerWithTitle:@"Apple Pencil Double Tap"
                       message:
                           @"Choose what double tap does in Blender. You can change it later "
                           @"in the command bar settings."
                preferredStyle:UIAlertControllerStyleActionSheet];
  for (const GHOST_IOSDoubleTapOption &option : GHOST_IOS_DOUBLE_TAP_OPTIONS) {
    const GHOST_IOSDoubleTapChoice choice = option.choice;
    UIAlertAction *action = [UIAlertAction actionWithTitle:option.title
                                                     style:UIAlertActionStyleDefault
                                                   handler:^(UIAlertAction *) {
                                                     chosen(choice);
                                                   }];
    [sheet addAction:action];
  }
  [sheet addAction:[UIAlertAction actionWithTitle:@"Not Now"
                                            style:UIAlertActionStyleCancel
                                          handler:nil]];
  sheet.view.accessibilityIdentifier = @"blender_pencil_double_tap_prompt";
  UIPopoverPresentationController *popover = sheet.popoverPresentationController;
  if (popover != nil) {
    popover.sourceView = source_view;
    popover.sourceRect = CGRectMake(point.x - 1.0f, point.y - 1.0f, 2.0f, 2.0f);
  }
  [presenter presentViewController:sheet animated:YES completion:nil];
}

static UIMenu *GHOST_IOSGestureMenu(NSString *title,
                                    const int gesture,
                                    const GHOST_IOSTouchSettings &settings,
                                    void (^apply)(GHOST_IOSTouchSettings settings))
{
  struct Option {
    GHOST_IOSGestureAction action;
    NSString *title;
  };
  static const Option options[] = {
      {GHOST_IOSGestureAction::Undo, @"Undo"},
      {GHOST_IOSGestureAction::Redo, @"Redo"},
      {GHOST_IOSGestureAction::Search, @"Search"},
      {GHOST_IOSGestureAction::None, @"Nothing"},
  };
  NSMutableArray<UIMenuElement *> *children = [NSMutableArray array];
  for (const Option &option : options) {
    GHOST_IOSTouchSettings next = settings;
    next.tap_gestures[gesture] = option.action;
    UIAction *action = [UIAction actionWithTitle:option.title
                                           image:nil
                                      identifier:nil
                                         handler:^(UIAction *) {
                                           apply(next);
                                         }];
    action.state = settings.tap_gestures[gesture] == option.action ? UIMenuElementStateOn :
                                                                     UIMenuElementStateOff;
    [children addObject:action];
  }
  return [UIMenu menuWithTitle:title
                         image:nil
                    identifier:nil
                       options:UIMenuOptionsSingleSelection
                      children:children];
}

UIMenu *GHOST_IOSTouchSettings_menu(const GHOST_IOSTouchSettings &settings,
                                    void (^changed)(const GHOST_IOSTouchSettings &settings))
{
  void (^apply)(GHOST_IOSTouchSettings) = ^(GHOST_IOSTouchSettings next) {
    GHOST_IOSTouchSettings_save(next);
    changed(next);
  };

  NSMutableArray<UIMenuElement *> *double_tap = [NSMutableArray array];
  for (const GHOST_IOSDoubleTapOption &option : GHOST_IOS_DOUBLE_TAP_OPTIONS) {
    GHOST_IOSTouchSettings next = settings;
    next.double_tap = option.choice;
    UIAction *action = [UIAction actionWithTitle:option.title
                                           image:GHOST_IOSStyle_symbol(option.symbol, 15.0f)
                                      identifier:nil
                                         handler:^(UIAction *) {
                                           apply(next);
                                         }];
    action.state = settings.double_tap == option.choice ? UIMenuElementStateOn :
                                                          UIMenuElementStateOff;
    [double_tap addObject:action];
  }

  struct SqueezeOption {
    GHOST_IOSSqueezeChoice choice;
    NSString *title;
  };
  static const SqueezeOption squeeze_options[] = {
      {GHOST_IOSSqueezeChoice::System, @"Follow System Setting"},
      {GHOST_IOSSqueezeChoice::Palette, @"Show Palette"},
      {GHOST_IOSSqueezeChoice::Off, @"Off"},
  };
  NSMutableArray<UIMenuElement *> *squeeze = [NSMutableArray array];
  for (const SqueezeOption &option : squeeze_options) {
    GHOST_IOSTouchSettings next = settings;
    next.squeeze = option.choice;
    UIAction *action = [UIAction actionWithTitle:option.title
                                           image:nil
                                      identifier:nil
                                         handler:^(UIAction *) {
                                           apply(next);
                                         }];
    action.state = settings.squeeze == option.choice ? UIMenuElementStateOn :
                                                       UIMenuElementStateOff;
    [squeeze addObject:action];
  }

  GHOST_IOSTouchSettings pencil_only = settings;
  pencil_only.pencil_only_drawing = !settings.pencil_only_drawing;
  UIAction *pencil_only_action = [UIAction
      actionWithTitle:@"Only Draw with Apple Pencil"
                image:GHOST_IOSStyle_symbol(@"applepencil", 15.0f)
           identifier:nil
              handler:^(UIAction *) {
                apply(pencil_only);
              }];
  pencil_only_action.state = settings.pencil_only_drawing ? UIMenuElementStateOn :
                                                            UIMenuElementStateOff;

  UIMenu *pencil = [UIMenu menuWithTitle:@""
                                   image:nil
                              identifier:nil
                                 options:UIMenuOptionsDisplayInline
                                children:@[
                                  [UIMenu menuWithTitle:@"Pencil Double Tap"
                                                  image:GHOST_IOSStyle_symbol(@"hand.tap", 15.0f)
                                             identifier:nil
                                                options:UIMenuOptionsSingleSelection
                                               children:double_tap],
                                  [UIMenu menuWithTitle:@"Pencil Squeeze"
                                                  image:GHOST_IOSStyle_symbol(@"hand.pinch", 15.0f)
                                             identifier:nil
                                                options:UIMenuOptionsSingleSelection
                                               children:squeeze],
                                  pencil_only_action,
                                ]];
  UIMenu *gestures = [UIMenu menuWithTitle:@""
                                     image:nil
                                identifier:nil
                                   options:UIMenuOptionsDisplayInline
                                  children:@[
                                    GHOST_IOSGestureMenu(@"Two-Finger Tap", 0, settings, apply),
                                    GHOST_IOSGestureMenu(@"Three-Finger Tap", 1, settings, apply),
                                    GHOST_IOSGestureMenu(@"Four-Finger Tap", 2, settings, apply),
                                  ]];
  return [UIMenu menuWithTitle:@"Touch and Pencil" children:@[ pencil, gestures ]];
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name C API
 * \{ */

NSObject *GHOST_IOSPencilPalette_create(UIView *input_view,
                                        id<GHOSTIOSPencilPaletteDelegate> delegate)
{
  if (input_view == nil) {
    return nil;
  }
  return [[GHOSTIOSPencilPaletteView alloc] initWithInputView:input_view delegate:delegate];
}

void GHOST_IOSPencilPalette_destroy(NSObject *palette)
{
  if (palette == nil) {
    return;
  }
  [(GHOSTIOSPencilPaletteView *)palette teardown];
  [palette release];
}

void GHOST_IOSPencilPalette_show(NSObject *palette,
                                 CGPoint point,
                                 const GHOST_IOSPaletteSlot *slots,
                                 int count)
{
  [(GHOSTIOSPencilPaletteView *)palette showAt:point slots:slots count:count];
}

void GHOST_IOSPencilPalette_hide(NSObject *palette)
{
  [(GHOSTIOSPencilPaletteView *)palette hide];
}

bool GHOST_IOSPencilPalette_isVisible(NSObject *palette)
{
  return palette != nil && !((GHOSTIOSPencilPaletteView *)palette).hidden;
}

bool GHOST_IOSPencilPalette_containsView(NSObject *palette, UIView *view)
{
  if (palette == nil || view == nil) {
    return false;
  }
  return [view isDescendantOfView:(GHOSTIOSPencilPaletteView *)palette];
}

/** \} */
