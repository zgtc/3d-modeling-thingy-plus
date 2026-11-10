/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * Native on-screen command bar for touch and Apple Pencil.
 *
 * The bar stays open on the left or right screen edge, beside Blender's canvas, which is inset so
 * the bar never covers it. From the handle it shows Shift, Ctrl, Alt and Cmd, then up to six
 * shortcut keys for the editor and mode under the pointer, then a settings button. Drag the handle
 * to another corner, or tap it to switch sides. While a modal operator takes typed values, a key
 * pad with axes and numbers shows next to the bar, so `G Z 2 Enter` works without a keyboard.
 *
 * Each modifier can be held like a hardware key, tapped to apply to the next click, or double
 * tapped to stay on. The key logic lives in #GHOST_IOSModifierLatch and the key sets in
 * #GHOST_IOSTouchKeys.hh; this file is only the view.
 */

#import <GameController/GameController.h>
#import <UIKit/UIKit.h>
#import <os/log.h>

#include "GHOST_IOSModifierBelt.hh"
#include "GHOST_IOSStyle.hh"

#include <algorithm>
#include <cstdlib>
#include <cstring>

/** Corners, stored in user defaults. */
enum {
  GHOST_IOS_BELT_TOP_LEFT = 0,
  GHOST_IOS_BELT_TOP_RIGHT = 1,
  GHOST_IOS_BELT_BOTTOM_LEFT = 2,
  GHOST_IOS_BELT_BOTTOM_RIGHT = 3,
};

static NSString *const GHOST_IOSModifierBeltCorner = @"GHOST_IOSModifierBeltCorner";

static constexpr CGFloat belt_padding = 4.0f;
static constexpr CGFloat handle_size = GHOST_IOS_CONTROL_SIZE;
static constexpr CGFloat key_width = 58.0f;
/** The bar is vertical: one key wide. */
static constexpr CGFloat belt_width = key_width + belt_padding * 2.0f;
static constexpr CGFloat command_key_height = 36.0f;
static constexpr CGFloat pad_key_width = 52.0f;
static constexpr CGFloat pad_key_height = 40.0f;
static constexpr CGFloat pad_gap = 8.0f;
static constexpr CGFloat edge_inset = 12.0f;
/** Keep the bottom corners clear of Blender's status bar. */
static constexpr CGFloat bottom_inset = 40.0f;
/** Opacity while the app's finger cursor is over the belt. */
static constexpr CGFloat cursor_over_alpha = 0.3f;

struct GHOST_IOSModifierKeyInfo {
  GHOST_IOSModifier modifier;
  NSString *symbol;
  NSString *title;
  NSString *identifier;
};

static const GHOST_IOSModifierKeyInfo GHOST_IOS_MODIFIER_KEYS[GHOST_IOS_MODIFIER_COUNT] = {
    {GHOST_IOSModifier::Shift, @"shift", @"Shift", @"blender_modifier_shift"},
    {GHOST_IOSModifier::Control, @"control", @"Ctrl", @"blender_modifier_control"},
    {GHOST_IOSModifier::Alt, @"option", @"Alt", @"blender_modifier_alt"},
    {GHOST_IOSModifier::OS, @"command", @"Cmd", @"blender_modifier_os"},
};

static os_log_t GHOST_IOSModifierBeltLog()
{
  static os_log_t log = os_log_create("org.blender.ios", "modifier-belt");
  return log;
}

static NSString *GHOST_IOSModifierModeName(const GHOST_IOSModifierMode mode)
{
  switch (mode) {
    case GHOST_IOSModifierMode::Held:
      return @"held";
    case GHOST_IOSModifierMode::Latched:
      return @"latched";
    case GHOST_IOSModifierMode::Locked:
      return @"locked";
    case GHOST_IOSModifierMode::Off:
      break;
  }
  return @"off";
}

static UIImage *GHOST_IOSModifierSymbol(NSString *name, const CGFloat size)
{
  return GHOST_IOSStyle_symbol(name, size);
}

/* -------------------------------------------------------------------- */
/** \name Key
 * \{ */

@interface GHOSTIOSModifierKeyView : UIView {
  GHOST_IOSModifier modifier_;
  GHOST_IOSModifierMode mode_;
  id<GHOSTIOSModifierBeltDelegate> delegate_;
  UIImageView *icon_;
  UILabel *label_;
  UIView *lock_bar_;
  /** Not retained: only compared against the touches UIKit delivers. */
  UITouch *tracked_touch_;
}
- (instancetype)initWithKey:(const GHOST_IOSModifierKeyInfo &)info
                   delegate:(id<GHOSTIOSModifierBeltDelegate>)delegate;
- (void)setMode:(GHOST_IOSModifierMode)mode;
- (GHOST_IOSModifier)modifier;
@end

@implementation GHOSTIOSModifierKeyView

- (instancetype)initWithKey:(const GHOST_IOSModifierKeyInfo &)info
                   delegate:(id<GHOSTIOSModifierBeltDelegate>)delegate
{
  self = [super initWithFrame:CGRectZero];
  if (self == nil) {
    return nil;
  }
  modifier_ = info.modifier;
  mode_ = GHOST_IOSModifierMode::Off;
  delegate_ = delegate;
  tracked_touch_ = nil;

  self.translatesAutoresizingMaskIntoConstraints = NO;
  /* Capsule keys, matching the glass belt they sit in. */
  self.layer.cornerRadius = handle_size * 0.5f;
  self.layer.cornerCurve = kCACornerCurveContinuous;
  self.isAccessibilityElement = YES;
  self.accessibilityTraits = UIAccessibilityTraitButton;
  self.accessibilityLabel = info.title;
  self.accessibilityIdentifier = info.identifier;
  self.accessibilityHint = @"Hold, tap for the next click, or double tap to keep on";

  icon_ = [[UIImageView alloc] initWithImage:GHOST_IOSModifierSymbol(info.symbol, 15.0f)];
  icon_.translatesAutoresizingMaskIntoConstraints = NO;
  icon_.contentMode = UIViewContentModeCenter;
  icon_.tintColor = UIColor.whiteColor;

  label_ = [[UILabel alloc] init];
  label_.translatesAutoresizingMaskIntoConstraints = NO;
  label_.text = info.title;
  label_.font = [UIFont systemFontOfSize:11.0f weight:UIFontWeightSemibold];
  label_.textColor = UIColor.whiteColor;
  label_.textAlignment = NSTextAlignmentCenter;

  /* Like the caps lock bar on the software keyboard. */
  lock_bar_ = [[UIView alloc] init];
  lock_bar_.translatesAutoresizingMaskIntoConstraints = NO;
  lock_bar_.backgroundColor = UIColor.whiteColor;
  lock_bar_.layer.cornerRadius = 1.0f;
  lock_bar_.hidden = YES;

  [self addSubview:icon_];
  [self addSubview:label_];
  [self addSubview:lock_bar_];
  [NSLayoutConstraint activateConstraints:@[
    [self.widthAnchor constraintEqualToConstant:key_width],
    [self.heightAnchor constraintEqualToConstant:handle_size],
    [icon_.centerXAnchor constraintEqualToAnchor:self.centerXAnchor],
    [icon_.topAnchor constraintEqualToAnchor:self.topAnchor constant:5.0f],
    [label_.centerXAnchor constraintEqualToAnchor:self.centerXAnchor],
    [label_.topAnchor constraintEqualToAnchor:icon_.bottomAnchor constant:1.0f],
    [lock_bar_.centerXAnchor constraintEqualToAnchor:self.centerXAnchor],
    [lock_bar_.bottomAnchor constraintEqualToAnchor:self.bottomAnchor constant:-3.0f],
    [lock_bar_.widthAnchor constraintEqualToConstant:14.0f],
    [lock_bar_.heightAnchor constraintEqualToConstant:2.0f],
  ]];
  [self setMode:GHOST_IOSModifierMode::Off];
  return self;
}

- (void)dealloc
{
  [icon_ release];
  [label_ release];
  [lock_bar_ release];
  [super dealloc];
}

- (GHOST_IOSModifier)modifier
{
  return modifier_;
}

- (void)setMode:(GHOST_IOSModifierMode)mode
{
  /* Holding is already felt through the finger; only tap and lock changes need a haptic. */
  const bool tactile_change = mode != mode_ && mode != GHOST_IOSModifierMode::Held &&
                              mode_ != GHOST_IOSModifierMode::Held;
  mode_ = mode;
  UIColor *accent = GHOST_IOSStyle_accentColor();
  switch (mode) {
    case GHOST_IOSModifierMode::Off:
      /* Glass already reads as a surface, so an idle key only needs a faint fill. */
      self.backgroundColor = [UIColor colorWithWhite:1.0f
                                               alpha:GHOST_IOSStyle_hasGlass() ? 0.06f : 0.10f];
      break;
    case GHOST_IOSModifierMode::Latched:
      self.backgroundColor = [accent colorWithAlphaComponent:0.5f];
      break;
    case GHOST_IOSModifierMode::Held:
    case GHOST_IOSModifierMode::Locked:
      self.backgroundColor = accent;
      break;
  }
  if (tactile_change) {
    GHOST_IOSStyle_selectionFeedback();
  }
  lock_bar_.hidden = mode != GHOST_IOSModifierMode::Locked;
  self.accessibilityValue = GHOST_IOSModifierModeName(mode);
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
  (void)event;
  if (tracked_touch_ != nil) {
    return;
  }
  tracked_touch_ = touches.anyObject;
  [delegate_ modifierBeltKey:modifier_ down:YES timestamp:tracked_touch_.timestamp];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
  (void)event;
  if (tracked_touch_ == nil || ![touches containsObject:tracked_touch_]) {
    return;
  }
  const NSTimeInterval timestamp = tracked_touch_.timestamp;
  tracked_touch_ = nil;
  [delegate_ modifierBeltKey:modifier_ down:NO timestamp:timestamp];
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
  (void)event;
  if (tracked_touch_ == nil || ![touches containsObject:tracked_touch_]) {
    return;
  }
  tracked_touch_ = nil;
  [delegate_ modifierBeltKeyCancelled:modifier_];
}

- (void)cancelTracking
{
  if (tracked_touch_ != nil) {
    tracked_touch_ = nil;
    [delegate_ modifierBeltKeyCancelled:modifier_];
  }
}

- (BOOL)accessibilityActivate
{
  /* VoiceOver activation is a tap: apply the key to the next click. */
  const NSTimeInterval now = NSProcessInfo.processInfo.systemUptime;
  [delegate_ modifierBeltKey:modifier_ down:YES timestamp:now];
  [delegate_ modifierBeltKey:modifier_ down:NO timestamp:now];
  return YES;
}

@end

/** \} */

/* -------------------------------------------------------------------- */
/** \name Shortcut and key pad keys
 * \{ */

static UIColor *GHOST_IOSCommandKeyIdleColor()
{
  return [UIColor colorWithWhite:1.0f alpha:GHOST_IOSStyle_hasGlass() ? 0.06f : 0.10f];
}

/** A key that sends one key stroke when tapped. */
@interface GHOSTIOSCommandKeyButton : UIButton {
  GHOST_IOSBarKey key_;
  id<GHOSTIOSModifierBeltDelegate> delegate_;
  /** Index among the shortcut keys for editing, or -1 for key pad keys. */
  int edit_index_;
  BOOL edit_started_;
}
+ (instancetype)keyWithInfo:(const GHOST_IOSBarKey &)info
                   delegate:(id<GHOSTIOSModifierBeltDelegate>)delegate
                      width:(CGFloat)width
                     height:(CGFloat)height
                 identifier:(NSString *)identifier;
- (void)setEditIndex:(int)index;
- (void)forgetDelegate;
@end

@implementation GHOSTIOSCommandKeyButton

+ (instancetype)keyWithInfo:(const GHOST_IOSBarKey &)info
                   delegate:(id<GHOSTIOSModifierBeltDelegate>)delegate
                      width:(CGFloat)width
                     height:(CGFloat)height
                 identifier:(NSString *)identifier
{
  GHOSTIOSCommandKeyButton *key = [self buttonWithType:UIButtonTypeCustom];
  key->key_ = info;
  key->delegate_ = delegate;
  key->edit_index_ = -1;
  key->edit_started_ = NO;
  NSString *title = [NSString stringWithUTF8String:info.title];
  key.translatesAutoresizingMaskIntoConstraints = NO;
  [key setTitle:title forState:UIControlStateNormal];
  [key setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
  key.titleLabel.font = [UIFont systemFontOfSize:13.0f weight:UIFontWeightSemibold];
  key.titleLabel.adjustsFontSizeToFitWidth = YES;
  key.titleLabel.minimumScaleFactor = 0.7f;
  key.backgroundColor = GHOST_IOSCommandKeyIdleColor();
  key.layer.cornerRadius = height * 0.5f;
  key.layer.cornerCurve = kCACornerCurveContinuous;
  key.accessibilityLabel = title;
  key.accessibilityIdentifier = identifier;
  [NSLayoutConstraint activateConstraints:@[
    [key.widthAnchor constraintEqualToConstant:width],
    [key.heightAnchor constraintEqualToConstant:height],
  ]];
  [key addTarget:key action:@selector(handlePress) forControlEvents:UIControlEventTouchUpInside];
  return key;
}

- (void)setHighlighted:(BOOL)highlighted
{
  [super setHighlighted:highlighted];
  self.backgroundColor = highlighted ? GHOST_IOSStyle_accentColor() :
                                       GHOST_IOSCommandKeyIdleColor();
}

- (void)handlePress
{
  if (edit_started_) {
    edit_started_ = NO;
    return;
  }
  GHOST_IOSStyle_selectionFeedback();
  [delegate_ commandBarKey:key_];
}

- (void)setEditIndex:(int)index
{
  edit_index_ = index;
  UILongPressGestureRecognizer *hold = [[[UILongPressGestureRecognizer alloc]
      initWithTarget:self
              action:@selector(handleHold:)] autorelease];
  hold.minimumPressDuration = 0.6;
  [self addGestureRecognizer:hold];
  self.accessibilityHint = @"Hold to replace this key";
}

- (void)handleHold:(UILongPressGestureRecognizer *)sender
{
  if (sender.state == UIGestureRecognizerStateBegan && edit_index_ >= 0) {
    edit_started_ = YES;
    GHOST_IOSStyle_selectionFeedback();
    [delegate_ commandBarEditKey:edit_index_];
  }
}

- (void)forgetDelegate
{
  delegate_ = nil;
}

@end

/** \} */

/* -------------------------------------------------------------------- */
/** \name Belt
 * \{ */

@interface GHOSTIOSModifierBeltView : UIView {
  id<GHOSTIOSModifierBeltDelegate> delegate_;
  UIVisualEffectView *background_;
  UIStackView *stack_;
  UIView *handle_;
  UIImageView *handle_icon_;
  NSArray<GHOSTIOSModifierKeyView *> *keys_;
  NSMutableArray<GHOSTIOSCommandKeyButton *> *context_keys_;
  GHOST_IOSBarKey context_key_info_[GHOST_IOS_BAR_KEY_MAX];
  /* The bar's own copies, so keys stay valid when the caller reuses its buffers. */
  char context_key_titles_[GHOST_IOS_BAR_KEY_MAX][32];
  char context_key_commands_[GHOST_IOS_BAR_KEY_MAX][GHOST_IOS_COMMAND_MAX];
  int context_key_count_;
  UIView *separator_;
  NSArray<UIButton *> *action_buttons_;
  UIButton *settings_button_;
  /** Sibling of the bar in the input view, so it can sit beside the bar. */
  UIView *key_pad_;
  NSMutableArray<GHOSTIOSCommandKeyButton *> *key_pad_keys_;
  BOOL key_pad_wanted_;
  NSArray<NSLayoutConstraint *> *corner_constraints_;
  UIPanGestureRecognizer *pan_;
  UITapGestureRecognizer *tap_;
  /** Blender's canvas. Not retained. */
  UIView *input_view_;
  id keyboard_connect_observer_;
  id keyboard_disconnect_observer_;
  NSInteger corner_;
  GHOST_IOSModifierMode modes_[GHOST_IOS_MODIFIER_COUNT];
  BOOL expanded_;
  BOOL cursor_over_;
}
- (instancetype)initWithInputView:(UIView *)input_view
                         delegate:(id<GHOSTIOSModifierBeltDelegate>)delegate;
- (void)teardown;
- (void)setModes:(const GHOST_IOSModifierMode *)modes;
- (void)setCursor:(CGPoint)point visible:(bool)visible;
- (void)setContextKeys:(const GHOST_IOSBarKey *)keys count:(int)count;
- (void)setKeyPadVisible:(BOOL)visible;
- (void)setSettingsMenu:(UIMenu *)menu;
- (BOOL)containsView:(UIView *)view;
@end

@implementation GHOSTIOSModifierBeltView

- (instancetype)initWithInputView:(UIView *)input_view
                         delegate:(id<GHOSTIOSModifierBeltDelegate>)delegate
{
  self = [super initWithFrame:CGRectZero];
  if (self == nil) {
    return nil;
  }
  delegate_ = delegate;
  input_view_ = input_view;
  /* Pinned open: a tucked-away bar was hard to find and slow to reach. */
  expanded_ = YES;
  cursor_over_ = NO;
  context_key_count_ = 0;
  key_pad_wanted_ = NO;
  context_keys_ = [[NSMutableArray alloc] init];
  key_pad_keys_ = [[NSMutableArray alloc] init];
  for (GHOST_IOSModifierMode &mode : modes_) {
    mode = GHOST_IOSModifierMode::Off;
  }
  corner_constraints_ = nil;

  NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
  corner_ = [defaults objectForKey:GHOST_IOSModifierBeltCorner] != nil ?
                [defaults integerForKey:GHOST_IOSModifierBeltCorner] :
                GHOST_IOS_BELT_BOTTOM_RIGHT;
  corner_ = MAX(GHOST_IOS_BELT_TOP_LEFT, MIN(GHOST_IOS_BELT_BOTTOM_RIGHT, corner_));

  self.translatesAutoresizingMaskIntoConstraints = NO;
  self.overrideUserInterfaceStyle = UIUserInterfaceStyleDark;
  self.accessibilityIdentifier = @"blender_modifier_belt";
  if (!GHOST_IOSStyle_hasGlass()) {
    /* Liquid Glass draws its own depth; the blur fallback needs a shadow to lift it. */
    self.layer.shadowColor = UIColor.blackColor.CGColor;
    self.layer.shadowOpacity = 0.3f;
    self.layer.shadowRadius = 8.0f;
    self.layer.shadowOffset = CGSizeMake(0.0f, 2.0f);
  }

  background_ = GHOST_IOSStyle_createMaterialView(belt_width * 0.5f);
  [self addSubview:background_];

  handle_ = [[UIView alloc] init];
  handle_.translatesAutoresizingMaskIntoConstraints = NO;
  handle_.layer.cornerRadius = handle_size * 0.5f;
  handle_.layer.cornerCurve = kCACornerCurveContinuous;
  handle_.isAccessibilityElement = YES;
  handle_.accessibilityTraits = UIAccessibilityTraitButton;
  handle_.accessibilityLabel = @"Modifier keys";
  handle_.accessibilityHint = @"Tap to switch sides. Drag to move to another corner.";
  handle_.accessibilityIdentifier = @"blender_modifier_belt_handle";
  handle_icon_ = [[UIImageView alloc] init];
  handle_icon_.translatesAutoresizingMaskIntoConstraints = NO;
  handle_icon_.contentMode = UIViewContentModeCenter;
  handle_icon_.tintColor = UIColor.whiteColor;
  [handle_ addSubview:handle_icon_];

  NSMutableArray<GHOSTIOSModifierKeyView *> *keys = [NSMutableArray array];
  for (const GHOST_IOSModifierKeyInfo &info : GHOST_IOS_MODIFIER_KEYS) {
    GHOSTIOSModifierKeyView *key = [[[GHOSTIOSModifierKeyView alloc] initWithKey:info
                                                                         delegate:delegate]
        autorelease];
    key.hidden = !expanded_;
    [keys addObject:key];
  }
  keys_ = [keys copy];

  separator_ = [[UIView alloc] init];
  separator_.translatesAutoresizingMaskIntoConstraints = NO;
  separator_.backgroundColor = [UIColor colorWithWhite:1.0f alpha:0.25f];
  separator_.hidden = !expanded_;

  struct ActionInfo {
    GHOST_IOSBarAction action;
    NSString *symbol;
    NSString *title;
    NSString *identifier;
  };
  static const ActionInfo action_infos[] = {
      {GHOST_IOSBarAction::Focus, @"arrow.up.left.and.arrow.down.right", @"Focus mode",
       @"blender_command_bar_focus"},
      {GHOST_IOSBarAction::Properties, @"slider.horizontal.3", @"Properties",
       @"blender_command_bar_properties"},
      {GHOST_IOSBarAction::Outliner, @"list.bullet.indent", @"Outliner",
       @"blender_command_bar_outliner"},
  };
  NSMutableArray<UIButton *> *action_buttons = [NSMutableArray array];
  for (const ActionInfo &info : action_infos) {
    UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
    button.translatesAutoresizingMaskIntoConstraints = NO;
    [button setImage:GHOST_IOSModifierSymbol(info.symbol, 16.0f) forState:UIControlStateNormal];
    button.tintColor = UIColor.whiteColor;
    button.tag = NSInteger(info.action);
    button.accessibilityLabel = info.title;
    button.accessibilityIdentifier = info.identifier;
    button.hidden = YES;
    [button addTarget:self
                  action:@selector(handleActionButton:)
        forControlEvents:UIControlEventTouchUpInside];
    [NSLayoutConstraint activateConstraints:@[
      [button.widthAnchor constraintEqualToConstant:key_width],
      [button.heightAnchor constraintEqualToConstant:command_key_height],
    ]];
    [action_buttons addObject:button];
  }
  action_buttons_ = [action_buttons copy];

  settings_button_ = [[UIButton buttonWithType:UIButtonTypeSystem] retain];
  settings_button_.translatesAutoresizingMaskIntoConstraints = NO;
  [settings_button_ setImage:GHOST_IOSModifierSymbol(@"gearshape", 17.0f)
                    forState:UIControlStateNormal];
  settings_button_.tintColor = UIColor.whiteColor;
  settings_button_.showsMenuAsPrimaryAction = YES;
  settings_button_.accessibilityLabel = @"Touch and Pencil settings";
  settings_button_.accessibilityIdentifier = @"blender_command_bar_settings";
  settings_button_.hidden = !expanded_;

  stack_ = [[UIStackView alloc] init];
  stack_.translatesAutoresizingMaskIntoConstraints = NO;
  stack_.axis = UILayoutConstraintAxisVertical;
  stack_.alignment = UIStackViewAlignmentCenter;
  stack_.spacing = belt_padding;
  [self addSubview:stack_];

  [NSLayoutConstraint activateConstraints:@[
    [background_.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
    [background_.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
    [background_.topAnchor constraintEqualToAnchor:self.topAnchor],
    [background_.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
    [stack_.topAnchor constraintEqualToAnchor:self.topAnchor constant:belt_padding],
    [stack_.bottomAnchor constraintEqualToAnchor:self.bottomAnchor constant:-belt_padding],
    [stack_.centerXAnchor constraintEqualToAnchor:self.centerXAnchor],
    [self.widthAnchor constraintEqualToConstant:belt_width],
    [separator_.widthAnchor constraintEqualToConstant:28.0f],
    [separator_.heightAnchor constraintEqualToConstant:1.0f],
    [settings_button_.widthAnchor constraintEqualToConstant:key_width],
    [settings_button_.heightAnchor constraintEqualToConstant:command_key_height],
    [handle_.widthAnchor constraintEqualToConstant:handle_size],
    [handle_.heightAnchor constraintEqualToConstant:handle_size],
    [handle_icon_.centerXAnchor constraintEqualToAnchor:handle_.centerXAnchor],
    [handle_icon_.centerYAnchor constraintEqualToAnchor:handle_.centerYAnchor],
  ]];

  tap_ = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTap:)];
  pan_ = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePan:)];
  pan_.maximumNumberOfTouches = 1;
  [handle_ addGestureRecognizer:tap_];
  [handle_ addGestureRecognizer:pan_];

  [[self hostView] addSubview:self];
  [self createKeyPadInView:input_view];
  [self arrangeForCorner];
  [self activateCornerConstraints];
  [self updateHandle];

  NSNotificationCenter *center = NSNotificationCenter.defaultCenter;
  GHOSTIOSModifierBeltView *belt = self;
  void (^keyboard_changed)(NSNotification *) = ^(NSNotification *) {
    [belt updateKeyboardVisibility];
  };
  keyboard_connect_observer_ = [[center addObserverForName:GCKeyboardDidConnectNotification
                                                    object:nil
                                                     queue:NSOperationQueue.mainQueue
                                                usingBlock:keyboard_changed] retain];
  keyboard_disconnect_observer_ = [[center addObserverForName:GCKeyboardDidDisconnectNotification
                                                       object:nil
                                                        queue:NSOperationQueue.mainQueue
                                                   usingBlock:keyboard_changed] retain];
  [self updateKeyboardVisibility];
  return self;
}

- (void)teardown
{
  NSNotificationCenter *center = NSNotificationCenter.defaultCenter;
  if (keyboard_connect_observer_ != nil) {
    [center removeObserver:keyboard_connect_observer_];
    [keyboard_connect_observer_ release];
    keyboard_connect_observer_ = nil;
  }
  if (keyboard_disconnect_observer_ != nil) {
    [center removeObserver:keyboard_disconnect_observer_];
    [keyboard_disconnect_observer_ release];
    keyboard_disconnect_observer_ = nil;
  }
  [handle_ removeGestureRecognizer:tap_];
  [handle_ removeGestureRecognizer:pan_];
  if (id<GHOSTIOSCanvasInsetting> host = [self insettingHost]) {
    [host setCanvasInsetsLeading:0.0f trailing:0.0f];
  }
  for (GHOSTIOSCommandKeyButton *key in context_keys_) {
    [key forgetDelegate];
  }
  for (GHOSTIOSCommandKeyButton *key in key_pad_keys_) {
    [key forgetDelegate];
  }
  delegate_ = nil;
  [key_pad_ removeFromSuperview];
  [self removeFromSuperview];
}

- (void)dealloc
{
  [background_ release];
  [stack_ release];
  [handle_ release];
  [handle_icon_ release];
  [keys_ release];
  [context_keys_ release];
  [separator_ release];
  [action_buttons_ release];
  [settings_button_ release];
  [key_pad_ release];
  [key_pad_keys_ release];
  [corner_constraints_ release];
  [pan_ release];
  [tap_ release];
  [super dealloc];
}

/** The canvas' superview when it can make room for the bar. */
- (id<GHOSTIOSCanvasInsetting>)insettingHost
{
  UIView *host = input_view_.superview;
  if ([host conformsToProtocol:@protocol(GHOSTIOSCanvasInsetting)]) {
    return (id<GHOSTIOSCanvasInsetting>)host;
  }
  return nil;
}

- (UIView *)hostView
{
  return [self insettingHost] != nil ? input_view_.superview : input_view_;
}

- (void)updateCanvasInsets
{
  id<GHOSTIOSCanvasInsetting> host = [self insettingHost];
  if (host == nil) {
    return;
  }
  double leading = 0.0, trailing = 0.0;
  GHOST_IOSCommandBar_canvasInsets(
      !self.hidden, [self isLeftCorner], belt_width, edge_inset, &leading, &trailing);
  [host setCanvasInsetsLeading:leading trailing:trailing];
}

- (BOOL)isLeftCorner
{
  return corner_ == GHOST_IOS_BELT_TOP_LEFT || corner_ == GHOST_IOS_BELT_BOTTOM_LEFT;
}

- (BOOL)isTopCorner
{
  return corner_ == GHOST_IOS_BELT_TOP_LEFT || corner_ == GHOST_IOS_BELT_TOP_RIGHT;
}

/** Everything the handle shows and hides. */
- (NSArray<UIView *> *)expandableViews
{
  NSMutableArray<UIView *> *views = [NSMutableArray arrayWithArray:keys_];
  [views addObject:separator_];
  [views addObjectsFromArray:context_keys_];
  [views addObjectsFromArray:action_buttons_];
  [views addObject:settings_button_];
  return views;
}

/** Keep the handle in the corner so the keys open towards the middle of the edge. */
- (void)arrangeForCorner
{
  for (UIView *view in [[stack_.arrangedSubviews copy] autorelease]) {
    [stack_ removeArrangedSubview:view];
    [view removeFromSuperview];
  }
  NSMutableArray<UIView *> *order = [NSMutableArray arrayWithObject:handle_];
  [order addObjectsFromArray:[self expandableViews]];
  NSEnumerator<UIView *> *views = [self isTopCorner] ? order.objectEnumerator :
                                                       order.reverseObjectEnumerator;
  for (UIView *view in views) {
    [stack_ addArrangedSubview:view];
  }
}

- (void)activateCornerConstraints
{
  UIView *parent = self.superview;
  if (parent == nil) {
    return;
  }
  if (corner_constraints_ != nil) {
    [NSLayoutConstraint deactivateConstraints:corner_constraints_];
    [corner_constraints_ release];
  }
  UILayoutGuide *safe_area = parent.safeAreaLayoutGuide;
  NSLayoutConstraint *horizontal =
      [self isLeftCorner] ?
          [self.leadingAnchor constraintEqualToAnchor:safe_area.leadingAnchor
                                             constant:edge_inset] :
          [self.trailingAnchor constraintEqualToAnchor:safe_area.trailingAnchor
                                              constant:-edge_inset];
  NSLayoutConstraint *vertical =
      [self isTopCorner] ?
          [self.topAnchor constraintEqualToAnchor:safe_area.topAnchor constant:edge_inset] :
          [self.bottomAnchor constraintEqualToAnchor:safe_area.bottomAnchor
                                            constant:-bottom_inset];
  /* The key pad opens towards the middle of the screen, level with the bar's handle end. */
  NSLayoutConstraint *pad_horizontal =
      [self isLeftCorner] ?
          [key_pad_.leadingAnchor constraintEqualToAnchor:self.trailingAnchor constant:pad_gap] :
          [key_pad_.trailingAnchor constraintEqualToAnchor:self.leadingAnchor constant:-pad_gap];
  NSLayoutConstraint *pad_vertical =
      [self isTopCorner] ? [key_pad_.topAnchor constraintEqualToAnchor:self.topAnchor] :
                           [key_pad_.bottomAnchor constraintEqualToAnchor:self.bottomAnchor];
  corner_constraints_ = [@[ horizontal, vertical, pad_horizontal, pad_vertical ] retain];
  [NSLayoutConstraint activateConstraints:corner_constraints_];
  [self updateCanvasInsets];
}

- (void)createKeyPadInView:(UIView *)input_view
{
  key_pad_ = [[UIView alloc] init];
  key_pad_.translatesAutoresizingMaskIntoConstraints = NO;
  key_pad_.overrideUserInterfaceStyle = UIUserInterfaceStyleDark;
  key_pad_.accessibilityIdentifier = @"blender_command_bar_key_pad";
  key_pad_.hidden = YES;
  UIVisualEffectView *background = GHOST_IOSStyle_createMaterialView(18.0f);
  [key_pad_ addSubview:background];

  UIStackView *rows = [[[UIStackView alloc] init] autorelease];
  rows.translatesAutoresizingMaskIntoConstraints = NO;
  rows.axis = UILayoutConstraintAxisVertical;
  rows.spacing = belt_padding;
  [key_pad_ addSubview:rows];

  const int columns = GHOST_IOS_KEY_PAD_COLUMNS;
  const int full_rows = GHOST_IOS_KEY_PAD_COUNT / columns;
  for (int row = 0; row * columns < GHOST_IOS_KEY_PAD_COUNT; row++) {
    UIStackView *line = [[[UIStackView alloc] init] autorelease];
    line.axis = UILayoutConstraintAxisHorizontal;
    line.spacing = belt_padding;
    const int first = row * columns;
    const int count = std::min(columns, GHOST_IOS_KEY_PAD_COUNT - first);
    /* A short last row (Esc, Return) stretches its keys across the pad. */
    const CGFloat width = row < full_rows ?
                              pad_key_width :
                              (pad_key_width * columns + belt_padding * (columns - count)) /
                                  count;
    for (int i = first; i < first + count; i++) {
      NSString *identifier = [NSString stringWithFormat:@"blender_key_pad_%d", i];
      GHOSTIOSCommandKeyButton *key = [GHOSTIOSCommandKeyButton keyWithInfo:GHOST_IOS_KEY_PAD[i]
                                                                   delegate:delegate_
                                                                      width:width
                                                                     height:pad_key_height
                                                                 identifier:identifier];
      [key_pad_keys_ addObject:key];
      [line addArrangedSubview:key];
    }
    [rows addArrangedSubview:line];
  }

  [input_view addSubview:key_pad_];
  [NSLayoutConstraint activateConstraints:@[
    [background.leadingAnchor constraintEqualToAnchor:key_pad_.leadingAnchor],
    [background.trailingAnchor constraintEqualToAnchor:key_pad_.trailingAnchor],
    [background.topAnchor constraintEqualToAnchor:key_pad_.topAnchor],
    [background.bottomAnchor constraintEqualToAnchor:key_pad_.bottomAnchor],
    [rows.leadingAnchor constraintEqualToAnchor:key_pad_.leadingAnchor constant:pad_gap],
    [rows.trailingAnchor constraintEqualToAnchor:key_pad_.trailingAnchor constant:-pad_gap],
    [rows.topAnchor constraintEqualToAnchor:key_pad_.topAnchor constant:pad_gap],
    [rows.bottomAnchor constraintEqualToAnchor:key_pad_.bottomAnchor constant:-pad_gap],
  ]];
}

- (void)updateKeyPadVisibility
{
  const BOOL hide = !key_pad_wanted_ || self.hidden;
  if (key_pad_.hidden == hide) {
    return;
  }
  key_pad_.hidden = hide;
  [key_pad_.superview bringSubviewToFront:key_pad_];
}

- (void)updateHandle
{
  /* A grabber: the bar is always open, so the handle only moves it. */
  NSString *symbol = @"line.3.horizontal";
  bool any_active = false;
  if (!expanded_) {
    /* Show which key is on while the keys are tucked away. */
    for (const GHOST_IOSModifierKeyInfo &info : GHOST_IOS_MODIFIER_KEYS) {
      if (modes_[int(info.modifier)] != GHOST_IOSModifierMode::Off) {
        symbol = info.symbol;
        any_active = true;
        break;
      }
    }
  }
  handle_icon_.image = GHOST_IOSModifierSymbol(symbol, 18.0f);
  handle_.backgroundColor = any_active ? GHOST_IOSStyle_accentColor() : UIColor.clearColor;
  handle_.accessibilityValue = expanded_ ? @"expanded" : @"collapsed";
}

/** Tapping the handle moves the bar to the other side, keeping its half. */
- (void)handleTap:(UITapGestureRecognizer *)sender
{
  if (sender.state != UIGestureRecognizerStateEnded) {
    return;
  }
  GHOST_IOSStyle_selectionFeedback();
  switch (corner_) {
    case GHOST_IOS_BELT_TOP_LEFT:
      corner_ = GHOST_IOS_BELT_TOP_RIGHT;
      break;
    case GHOST_IOS_BELT_TOP_RIGHT:
      corner_ = GHOST_IOS_BELT_TOP_LEFT;
      break;
    case GHOST_IOS_BELT_BOTTOM_LEFT:
      corner_ = GHOST_IOS_BELT_BOTTOM_RIGHT;
      break;
    default:
      corner_ = GHOST_IOS_BELT_BOTTOM_LEFT;
      break;
  }
  [NSUserDefaults.standardUserDefaults setInteger:corner_ forKey:GHOST_IOSModifierBeltCorner];
  [self arrangeForCorner];
  [self activateCornerConstraints];
  [self updateHandle];
  [UIView animateWithDuration:0.25
                        delay:0.0
       usingSpringWithDamping:0.9f
        initialSpringVelocity:0.0f
                      options:UIViewAnimationOptionAllowUserInteraction
                   animations:^{
                     [self.superview layoutIfNeeded];
                   }
                   completion:nil];
}

- (void)handleActionButton:(UIButton *)sender
{
  GHOST_IOSStyle_selectionFeedback();
  [delegate_ commandBarAction:GHOST_IOSBarAction(sender.tag)];
}

- (void)handlePan:(UIPanGestureRecognizer *)sender
{
  UIView *parent = self.superview;
  if (parent == nil) {
    return;
  }
  const CGPoint translation = [sender translationInView:parent];
  if (sender.state == UIGestureRecognizerStateBegan ||
      sender.state == UIGestureRecognizerStateChanged)
  {
    self.transform = CGAffineTransformMakeTranslation(translation.x, translation.y);
    return;
  }
  if (sender.state != UIGestureRecognizerStateEnded &&
      sender.state != UIGestureRecognizerStateCancelled)
  {
    return;
  }

  /* Snap to the corner of the quarter the belt was dropped in. */
  const CGRect dropped = self.frame;
  const CGRect safe = parent.safeAreaLayoutGuide.layoutFrame;
  const CGPoint center = CGPointMake(CGRectGetMidX(dropped), CGRectGetMidY(dropped));
  const BOOL left = center.x < CGRectGetMidX(safe);
  const BOOL top = center.y < CGRectGetMidY(safe);
  corner_ = top ? (left ? GHOST_IOS_BELT_TOP_LEFT : GHOST_IOS_BELT_TOP_RIGHT) :
                  (left ? GHOST_IOS_BELT_BOTTOM_LEFT : GHOST_IOS_BELT_BOTTOM_RIGHT);
  [NSUserDefaults.standardUserDefaults setInteger:corner_ forKey:GHOST_IOSModifierBeltCorner];
  GHOST_IOSStyle_selectionFeedback();

  self.transform = CGAffineTransformIdentity;
  [self arrangeForCorner];
  [self activateCornerConstraints];
  [self updateHandle];
  [parent layoutIfNeeded];
  /* Start the animation where the finger let go. */
  const CGRect settled = self.frame;
  self.transform = CGAffineTransformMakeTranslation(CGRectGetMinX(dropped) - CGRectGetMinX(settled),
                                                    CGRectGetMinY(dropped) - CGRectGetMinY(settled));
  [UIView animateWithDuration:0.35
                        delay:0.0
       usingSpringWithDamping:0.82f
        initialSpringVelocity:0.0f
                      options:UIViewAnimationOptionAllowUserInteraction
                   animations:^{
                     self.transform = CGAffineTransformIdentity;
                   }
                   completion:nil];
}

- (void)setModes:(const GHOST_IOSModifierMode *)modes
{
  for (GHOSTIOSModifierKeyView *key in keys_) {
    const int index = int(key.modifier);
    modes_[index] = modes[index];
    [key setMode:modes_[index]];
  }
  [self updateHandle];
}

- (void)setCursor:(CGPoint)point visible:(bool)visible
{
  /* \a point is in canvas points; the bar may live beside the canvas. */
  point = [input_view_ convertPoint:point toView:self.superview];
  const BOOL over = visible && !self.hidden &&
                    CGRectContainsPoint(CGRectInset(self.frame, -6.0f, -6.0f), point);
  if (over == cursor_over_) {
    return;
  }
  cursor_over_ = over;
  [UIView animateWithDuration:0.15
                        delay:0.0
                      options:UIViewAnimationOptionAllowUserInteraction |
                              UIViewAnimationOptionBeginFromCurrentState
                   animations:^{
                     self.alpha = over ? cursor_over_alpha : 1.0f;
                   }
                   completion:nil];
}

- (void)updateKeyboardVisibility
{
  /* `GHOST_IOS_MODIFIER_BELT=always` keeps the keys with a keyboard attached. The Simulator
   * always reports one, so UI tests use this too. */
  const char *belt_mode = getenv("GHOST_IOS_MODIFIER_BELT");
  const bool always = belt_mode != nullptr && strcmp(belt_mode, "always") == 0;
  const BOOL hide = !always && GCKeyboard.coalescedKeyboard != nil;
  if (self.hidden == hide) {
    return;
  }
  os_log_info(GHOST_IOSModifierBeltLog(),
              "Modifier keys %{public}s: hardware keyboard %{public}s",
              hide ? "hidden" : "shown",
              hide ? "connected" : "disconnected");
  self.hidden = hide;
  [self updateKeyPadVisibility];
  [self updateCanvasInsets];
  if (hide) {
    /* Release any modifier still held; the bar opens again with the keyboard gone. */
    for (GHOSTIOSModifierKeyView *key in keys_) {
      [key cancelTracking];
    }
    [delegate_ modifierBeltDidHide];
  }
}

- (void)setContextKeys:(const GHOST_IOSBarKey *)keys count:(int)count
{
  count = std::max(0, std::min(count, GHOST_IOS_BAR_KEY_MAX));
  bool same = count == context_key_count_;
  for (int i = 0; same && i < count; i++) {
    const GHOST_IOSBarKey &old_key = context_key_info_[i];
    same = old_key.stroke.key == keys[i].stroke.key &&
           old_key.stroke.modifiers == keys[i].stroke.modifiers &&
           std::strcmp(old_key.title, keys[i].title) == 0 &&
           (old_key.command == nullptr) == (keys[i].command == nullptr) &&
           (old_key.command == nullptr || std::strcmp(old_key.command, keys[i].command) == 0);
  }
  if (same) {
    return;
  }
  for (GHOSTIOSCommandKeyButton *key in context_keys_) {
    [key forgetDelegate];
    [stack_ removeArrangedSubview:key];
    [key removeFromSuperview];
  }
  [context_keys_ removeAllObjects];
  context_key_count_ = count;
  for (int i = 0; i < count; i++) {
    GHOST_IOSBarKey &copy = context_key_info_[i];
    copy = keys[i];
    std::snprintf(context_key_titles_[i], sizeof(context_key_titles_[i]), "%s", keys[i].title);
    copy.title = context_key_titles_[i];
    if (keys[i].command != nullptr) {
      std::snprintf(context_key_commands_[i], GHOST_IOS_COMMAND_MAX, "%s", keys[i].command);
      copy.command = context_key_commands_[i];
    }
    NSString *identifier = [NSString stringWithFormat:@"blender_command_key_%d", i];
    GHOSTIOSCommandKeyButton *key = [GHOSTIOSCommandKeyButton keyWithInfo:copy
                                                                 delegate:delegate_
                                                                    width:key_width
                                                                   height:command_key_height
                                                               identifier:identifier];
    key.hidden = !expanded_;
    [key setEditIndex:i];
    [context_keys_ addObject:key];
  }
  [self arrangeForCorner];
}

- (void)setKeyPadVisible:(BOOL)visible
{
  key_pad_wanted_ = visible;
  [self updateKeyPadVisibility];
}

- (void)setSettingsMenu:(UIMenu *)menu
{
  settings_button_.menu = menu;
}

- (BOOL)containsView:(UIView *)view
{
  return [view isDescendantOfView:self] || [view isDescendantOfView:key_pad_];
}

@end

/** \} */

/* -------------------------------------------------------------------- */
/** \name C API
 * \{ */

NSObject *GHOST_IOSModifierBelt_create(UIView *input_view,
                                       id<GHOSTIOSModifierBeltDelegate> delegate)
{
  if (input_view == nil) {
    return nil;
  }
  return [[GHOSTIOSModifierBeltView alloc] initWithInputView:input_view delegate:delegate];
}

void GHOST_IOSModifierBelt_destroy(NSObject *belt)
{
  if (belt == nil) {
    return;
  }
  [(GHOSTIOSModifierBeltView *)belt teardown];
  [belt release];
}

bool GHOST_IOSModifierBelt_containsView(NSObject *belt, UIView *view)
{
  if (belt == nil || view == nil) {
    return false;
  }
  return [(GHOSTIOSModifierBeltView *)belt containsView:view];
}

void GHOST_IOSModifierBelt_setModes(NSObject *belt,
                                    const GHOST_IOSModifierMode modes[GHOST_IOS_MODIFIER_COUNT])
{
  [(GHOSTIOSModifierBeltView *)belt setModes:modes];
}

void GHOST_IOSModifierBelt_setCursor(NSObject *belt, CGPoint point, bool visible)
{
  [(GHOSTIOSModifierBeltView *)belt setCursor:point visible:visible];
}

void GHOST_IOSModifierBelt_setContextKeys(NSObject *belt, const GHOST_IOSBarKey *keys, int count)
{
  [(GHOSTIOSModifierBeltView *)belt setContextKeys:keys count:count];
}

void GHOST_IOSModifierBelt_setKeyPadVisible(NSObject *belt, bool visible)
{
  [(GHOSTIOSModifierBeltView *)belt setKeyPadVisible:visible];
}

void GHOST_IOSModifierBelt_setSettingsMenu(NSObject *belt, UIMenu *menu)
{
  [(GHOSTIOSModifierBeltView *)belt setSettingsMenu:menu];
}

/** \} */
