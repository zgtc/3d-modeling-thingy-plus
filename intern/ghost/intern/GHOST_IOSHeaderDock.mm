/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * The iPad's native header and tool dock.
 *
 * The header sits above Blender's canvas in place of Blender's top bar: one menu button with
 * Blender's top bar menus and the menus of the editor under the pointer, and the workspace tabs
 * in a strip that scrolls sideways. The mode stays in the 3D Viewport's own mode menu. The dock sits
 * below the canvas with the tools of the current 3D Viewport mode, so the tool shelf on the left
 * of the viewport can stay hidden. The canvas is inset for both, so they never cover it.
 */

#import <UIKit/UIKit.h>

#include "GHOST_IOSHeaderDock.hh"
#include "GHOST_IOSMenuItems.hh"
#include "GHOST_IOSModifierBelt.hh"
#include "GHOST_IOSStyle.hh"

#include <algorithm>

static constexpr CGFloat padding = 8.0f;
static constexpr CGFloat tab_height = 32.0f;
static constexpr CGFloat tool_width = 64.0f;

@interface GHOSTIOSHeaderDock : NSObject
{
  UIView *canvas_;
  id<GHOSTIOSHeaderDockDelegate> delegate_;
  id<GHOSTIOSCanvasInsetting> host_;

  UIView *header_;
  UIButton *menu_button_;
  UIScrollView *tab_scroll_;
  UIStackView *tab_stack_;

  UIView *dock_;
  UIScrollView *tool_scroll_;
  UIStackView *tool_stack_;
  NSMutableArray<UIButton *> *tool_buttons_;

  GHOST_IOSInteractionContext context_;
  /** Tools last shown; kept while the pointer is outside the 3D Viewport. */
  const GHOST_IOSToolItem *tools_;
  int tools_count_;
}
- (instancetype)initWithCanvas:(UIView *)canvas delegate:(id<GHOSTIOSHeaderDockDelegate>)delegate;
- (void)teardown;
- (void)setContext:(const GHOST_IOSInteractionContext &)context;
- (BOOL)containsView:(UIView *)view;
@end

@implementation GHOSTIOSHeaderDock

static UIView *GHOST_IOSHeaderDock_bar(UIView *host, NSString *identifier)
{
  UIView *bar = [[UIView alloc] init];
  bar.translatesAutoresizingMaskIntoConstraints = NO;
  bar.overrideUserInterfaceStyle = UIUserInterfaceStyleDark;
  bar.accessibilityIdentifier = identifier;
  UIVisualEffectView *background = GHOST_IOSStyle_createMaterialView(0.0);
  [bar addSubview:background];
  [NSLayoutConstraint activateConstraints:@[
    [background.leadingAnchor constraintEqualToAnchor:bar.leadingAnchor],
    [background.trailingAnchor constraintEqualToAnchor:bar.trailingAnchor],
    [background.topAnchor constraintEqualToAnchor:bar.topAnchor],
    [background.bottomAnchor constraintEqualToAnchor:bar.bottomAnchor],
  ]];
  [host addSubview:bar];
  return bar;
}

- (instancetype)initWithCanvas:(UIView *)canvas delegate:(id<GHOSTIOSHeaderDockDelegate>)delegate
{
  self = [super init];
  if (self == nil) {
    return nil;
  }
  UIView *host = canvas.superview;
  canvas_ = canvas;
  delegate_ = delegate;
  host_ = (id<GHOSTIOSCanvasInsetting>)host;
  tools_ = nullptr;
  tools_count_ = 0;
  tool_buttons_ = [[NSMutableArray alloc] init];

  /* Header. */
  header_ = GHOST_IOSHeaderDock_bar(host, @"blender_native_header");

  menu_button_ = [[UIButton buttonWithType:UIButtonTypeSystem] retain];
  menu_button_.translatesAutoresizingMaskIntoConstraints = NO;
  [menu_button_ setImage:GHOST_IOSStyle_symbol(@"line.3.horizontal.circle", 20.0)
                forState:UIControlStateNormal];
  menu_button_.tintColor = UIColor.whiteColor;
  menu_button_.showsMenuAsPrimaryAction = YES;
  menu_button_.accessibilityLabel = @"Menu";
  menu_button_.accessibilityIdentifier = @"blender_native_menu";
  /* Built when opened, so the editor menus match what is under the pointer. Not retained by
   * the block, which the button keeps. */
  __block GHOSTIOSHeaderDock *header_dock = self;
  menu_button_.menu = [UIMenu
      menuWithChildren:@[ [UIDeferredMenuElement elementWithUncachedProvider:^(void (^completion)(
                           NSArray<UIMenuElement *> *)) {
        completion([header_dock menuElements]);
      }] ]];
  [header_ addSubview:menu_button_];

  tab_scroll_ = [[UIScrollView alloc] init];
  tab_scroll_.translatesAutoresizingMaskIntoConstraints = NO;
  tab_scroll_.showsHorizontalScrollIndicator = NO;
  tab_scroll_.alwaysBounceHorizontal = YES;
  tab_scroll_.accessibilityIdentifier = @"blender_workspace_tabs";
  tab_stack_ = [[UIStackView alloc] init];
  tab_stack_.translatesAutoresizingMaskIntoConstraints = NO;
  tab_stack_.axis = UILayoutConstraintAxisHorizontal;
  tab_stack_.spacing = 6.0f;
  [tab_scroll_ addSubview:tab_stack_];
  [header_ addSubview:tab_scroll_];

  /* Dock. */
  dock_ = GHOST_IOSHeaderDock_bar(host, @"blender_tool_dock");
  tool_scroll_ = [[UIScrollView alloc] init];
  tool_scroll_.translatesAutoresizingMaskIntoConstraints = NO;
  tool_scroll_.showsHorizontalScrollIndicator = NO;
  tool_scroll_.alwaysBounceHorizontal = YES;
  tool_stack_ = [[UIStackView alloc] init];
  tool_stack_.translatesAutoresizingMaskIntoConstraints = NO;
  tool_stack_.axis = UILayoutConstraintAxisHorizontal;
  tool_stack_.spacing = 2.0f;
  [tool_scroll_ addSubview:tool_stack_];
  [dock_ addSubview:tool_scroll_];

  UILayoutGuide *safe = host.safeAreaLayoutGuide;
  [NSLayoutConstraint activateConstraints:@[
    /* The header covers the status bar area too; its controls sit below it. */
    [header_.topAnchor constraintEqualToAnchor:host.topAnchor],
    [header_.leadingAnchor constraintEqualToAnchor:host.leadingAnchor],
    [header_.trailingAnchor constraintEqualToAnchor:host.trailingAnchor],
    [header_.bottomAnchor constraintEqualToAnchor:safe.topAnchor constant:GHOST_IOS_HEADER_HEIGHT],
    [menu_button_.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:padding],
    [menu_button_.centerYAnchor constraintEqualToAnchor:safe.topAnchor
                                               constant:GHOST_IOS_HEADER_HEIGHT * 0.5],
    [menu_button_.widthAnchor constraintEqualToConstant:GHOST_IOS_CONTROL_SIZE],
    [menu_button_.heightAnchor constraintEqualToConstant:GHOST_IOS_CONTROL_SIZE],
    [tab_scroll_.leadingAnchor constraintEqualToAnchor:menu_button_.trailingAnchor
                                              constant:padding * 2.0f],
    [tab_scroll_.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-padding],
    [tab_scroll_.centerYAnchor constraintEqualToAnchor:menu_button_.centerYAnchor],
    [tab_scroll_.heightAnchor constraintEqualToConstant:tab_height],
    [tab_stack_.leadingAnchor
        constraintEqualToAnchor:tab_scroll_.contentLayoutGuide.leadingAnchor],
    [tab_stack_.trailingAnchor
        constraintEqualToAnchor:tab_scroll_.contentLayoutGuide.trailingAnchor],
    [tab_stack_.topAnchor constraintEqualToAnchor:tab_scroll_.contentLayoutGuide.topAnchor],
    [tab_stack_.bottomAnchor constraintEqualToAnchor:tab_scroll_.contentLayoutGuide.bottomAnchor],
    [tab_stack_.heightAnchor constraintEqualToAnchor:tab_scroll_.frameLayoutGuide.heightAnchor],

    /* The dock covers the home indicator area too. */
    [dock_.bottomAnchor constraintEqualToAnchor:host.bottomAnchor],
    [dock_.leadingAnchor constraintEqualToAnchor:host.leadingAnchor],
    [dock_.trailingAnchor constraintEqualToAnchor:host.trailingAnchor],
    [dock_.topAnchor constraintEqualToAnchor:safe.bottomAnchor constant:-GHOST_IOS_DOCK_HEIGHT],
    [tool_scroll_.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:padding],
    [tool_scroll_.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-padding],
    [tool_scroll_.topAnchor constraintEqualToAnchor:dock_.topAnchor constant:4.0f],
    [tool_scroll_.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor constant:-4.0f],
    [tool_stack_.leadingAnchor
        constraintEqualToAnchor:tool_scroll_.contentLayoutGuide.leadingAnchor],
    [tool_stack_.trailingAnchor
        constraintEqualToAnchor:tool_scroll_.contentLayoutGuide.trailingAnchor],
    [tool_stack_.topAnchor constraintEqualToAnchor:tool_scroll_.contentLayoutGuide.topAnchor],
    [tool_stack_.bottomAnchor
        constraintEqualToAnchor:tool_scroll_.contentLayoutGuide.bottomAnchor],
    [tool_stack_.heightAnchor constraintEqualToAnchor:tool_scroll_.frameLayoutGuide.heightAnchor],
  ]];

  [host_ setCanvasInsetsTop:GHOST_IOS_HEADER_HEIGHT bottom:GHOST_IOS_DOCK_HEIGHT];
  [self setContext:GHOST_IOSInteractionContext()];
  return self;
}

- (void)teardown
{
  menu_button_.menu = nil;
  for (UIButton *button in tool_buttons_) {
    [button removeTarget:self action:nil forControlEvents:UIControlEventAllEvents];
  }
  for (UIView *view in tab_stack_.arrangedSubviews) {
    if ([view isKindOfClass:[UIButton class]]) {
      [(UIButton *)view removeTarget:self action:nil forControlEvents:UIControlEventAllEvents];
    }
  }
  [host_ setCanvasInsetsTop:0.0f bottom:0.0f];
  [header_ removeFromSuperview];
  [dock_ removeFromSuperview];
  delegate_ = nil;
  host_ = nil;
}

- (void)dealloc
{
  [header_ release];
  [menu_button_ release];
  [tab_scroll_ release];
  [tab_stack_ release];
  [dock_ release];
  [tool_scroll_ release];
  [tool_stack_ release];
  [tool_buttons_ release];
  [super dealloc];
}

- (BOOL)containsView:(UIView *)view
{
  return [view isDescendantOfView:header_] || [view isDescendantOfView:dock_];
}

- (void)queue:(const char *)command
{
  [delegate_ headerDockQueueCommand:command];
}

/* -------------------------------------------------------------------- */
/** \name Menu
 * \{ */

- (UIMenu *)menuFor:(const GHOST_IOSMenu &)menu
{
  NSMutableArray<UIMenuElement *> *children = [NSMutableArray array];
  for (int i = 0; i < menu.count; i++) {
    char command[GHOST_IOS_COMMAND_MAX];
    if (!GHOST_IOSMenuItem_command(menu.items[i], command)) {
      continue;
    }
    NSString *call = [NSString stringWithUTF8String:command];
    __block GHOSTIOSHeaderDock *header_dock = self;
    [children
        addObject:[UIAction actionWithTitle:[NSString stringWithUTF8String:menu.items[i].title]
                                      image:nil
                                 identifier:nil
                                    handler:^(UIAction *) {
                                      [header_dock queue:call.UTF8String];
                                    }]];
  }
  return [UIMenu menuWithTitle:[NSString stringWithUTF8String:menu.title] children:children];
}

- (NSArray<UIMenuElement *> *)menuElements
{
  NSMutableArray<UIMenuElement *> *elements = [NSMutableArray array];
  const GHOST_IOSMenu *menus = nullptr;
  const int top = GHOST_IOSMenuBar_topMenus(&menus);
  for (int i = 0; i < top; i++) {
    [elements addObject:[self menuFor:menus[i]]];
  }
  const int editor = GHOST_IOSMenuBar_editorMenus(context_, &menus);
  if (editor > 0) {
    NSMutableArray<UIMenuElement *> *editor_menus = [NSMutableArray array];
    for (int i = 0; i < editor; i++) {
      [editor_menus addObject:[self menuFor:menus[i]]];
    }
    [elements addObject:[UIMenu menuWithTitle:@""
                                        image:nil
                                   identifier:nil
                                      options:UIMenuOptionsDisplayInline
                                     children:editor_menus]];
  }
  return elements;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Workspace tabs
 * \{ */

- (void)handleTab:(UIButton *)sender
{
  const int index = int(sender.tag);
  if (index < 0 || index >= context_.workspaces_num) {
    return;
  }
  char command[GHOST_IOS_COMMAND_MAX];
  if (GHOST_IOSWorkspace_command(context_.workspaces[index], command)) {
    GHOST_IOSStyle_selectionFeedback();
    [self queue:command];
  }
}

- (void)rebuildTabs:(const GHOST_IOSInteractionContext &)context
{
  for (UIView *view in [[tab_stack_.arrangedSubviews copy] autorelease]) {
    [tab_stack_ removeArrangedSubview:view];
    [view removeFromSuperview];
  }
  UIButton *active = nil;
  for (int i = 0; i < context.workspaces_num; i++) {
    UIButtonConfiguration *configuration = [UIButtonConfiguration plainButtonConfiguration];
    configuration.title = [NSString stringWithUTF8String:context.workspaces[i]];
    configuration.baseForegroundColor = UIColor.whiteColor;
    configuration.contentInsets = NSDirectionalEdgeInsetsMake(0.0f, 14.0f, 0.0f, 14.0f);
    configuration.titleTextAttributesTransformer = ^(NSDictionary *attributes) {
      NSMutableDictionary *result = [[attributes mutableCopy] autorelease];
      result[NSFontAttributeName] = [UIFont systemFontOfSize:14.0f weight:UIFontWeightSemibold];
      return (NSDictionary *)result;
    };
    UIButton *tab = [UIButton buttonWithConfiguration:configuration primaryAction:nil];
    tab.translatesAutoresizingMaskIntoConstraints = NO;
    tab.tag = i;
    tab.layer.cornerRadius = tab_height * 0.5f;
    tab.layer.cornerCurve = kCACornerCurveContinuous;
    const bool is_active = i == context.active_workspace;
    tab.backgroundColor = is_active ? GHOST_IOSStyle_accentColor() :
                                      [UIColor colorWithWhite:1.0f alpha:0.08f];
    tab.accessibilityIdentifier = [NSString stringWithFormat:@"blender_workspace_tab_%d", i];
    tab.accessibilityTraits = is_active ?
                                  UIAccessibilityTraitButton | UIAccessibilityTraitSelected :
                                  UIAccessibilityTraitButton;
    [tab addTarget:self action:@selector(handleTab:) forControlEvents:UIControlEventTouchUpInside];
    [tab_stack_ addArrangedSubview:tab];
    if (is_active) {
      active = tab;
    }
  }
  if (active != nil) {
    [tab_scroll_ layoutIfNeeded];
    [tab_scroll_ scrollRectToVisible:CGRectInset(active.frame, -24.0f, 0.0f) animated:YES];
  }
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Tool dock
 * \{ */

- (void)handleTool:(UIButton *)sender
{
  const int index = int(sender.tag);
  if (tools_ == nullptr || index < 0 || index >= tools_count_) {
    return;
  }
  char command[GHOST_IOS_COMMAND_MAX];
  if (GHOST_IOSTool_command(tools_[index], command)) {
    GHOST_IOSStyle_selectionFeedback();
    [self queue:command];
  }
}

- (void)rebuildTools
{
  for (UIButton *button in tool_buttons_) {
    [button removeTarget:self action:nil forControlEvents:UIControlEventAllEvents];
    [tool_stack_ removeArrangedSubview:button];
    [button removeFromSuperview];
  }
  [tool_buttons_ removeAllObjects];
  for (int i = 0; i < tools_count_; i++) {
    UIButtonConfiguration *configuration = [UIButtonConfiguration plainButtonConfiguration];
    configuration.image = GHOST_IOSStyle_symbol([NSString stringWithUTF8String:tools_[i].symbol],
                                                20.0);
    configuration.title = [NSString stringWithUTF8String:tools_[i].title];
    configuration.imagePlacement = NSDirectionalRectEdgeTop;
    configuration.imagePadding = 3.0f;
    configuration.baseForegroundColor = UIColor.whiteColor;
    configuration.titleTextAttributesTransformer = ^(NSDictionary *attributes) {
      NSMutableDictionary *result = [[attributes mutableCopy] autorelease];
      result[NSFontAttributeName] = [UIFont systemFontOfSize:10.0f weight:UIFontWeightSemibold];
      return (NSDictionary *)result;
    };
    UIButton *button = [UIButton buttonWithConfiguration:configuration primaryAction:nil];
    button.translatesAutoresizingMaskIntoConstraints = NO;
    button.tag = i;
    button.layer.cornerRadius = 12.0f;
    button.layer.cornerCurve = kCACornerCurveContinuous;
    button.accessibilityIdentifier = [NSString stringWithFormat:@"blender_dock_tool_%d", i];
    [button.widthAnchor constraintEqualToConstant:tool_width].active = YES;
    [button addTarget:self
                  action:@selector(handleTool:)
        forControlEvents:UIControlEventTouchUpInside];
    [tool_buttons_ addObject:button];
    [tool_stack_ addArrangedSubview:button];
  }
}

- (void)highlightActiveTool:(const char *)tool
{
  const int active = GHOST_IOSDock_activeIndex(tools_, tools_count_, tool);
  for (int i = 0; i < int(tool_buttons_.count); i++) {
    UIButton *button = tool_buttons_[i];
    button.backgroundColor = i == active ?
                                 [GHOST_IOSStyle_accentColor() colorWithAlphaComponent:0.85f] :
                                 UIColor.clearColor;
    button.selected = i == active;
  }
}

/** \} */

- (void)setContext:(const GHOST_IOSInteractionContext &)context
{
  const bool tabs_changed = !context.sameWorkspaces(context_) ||
                            tab_stack_.arrangedSubviews.count == 0;
  context_ = context;
  if (tabs_changed) {
    [self rebuildTabs:context];
  }

  /* The dock follows the 3D Viewport; elsewhere it keeps its tools. */
  if (context.editor != GHOST_IOSEditor::View3D) {
    return;
  }

  int count = 0;
  const GHOST_IOSToolItem *tools = GHOST_IOSDock_tools(context, &count);
  if (tools != tools_) {
    tools_ = tools;
    tools_count_ = count;
    [self rebuildTools];
  }
  [self highlightActiveTool:context.tool];
}

@end

/* -------------------------------------------------------------------- */
/** \name C API
 * \{ */

NSObject *GHOST_IOSHeaderDock_create(UIView *canvas, id<GHOSTIOSHeaderDockDelegate> delegate)
{
  if (canvas == nil || ![canvas.superview conformsToProtocol:@protocol(GHOSTIOSCanvasInsetting)]) {
    return nil;
  }
  return [[GHOSTIOSHeaderDock alloc] initWithCanvas:canvas delegate:delegate];
}

void GHOST_IOSHeaderDock_destroy(NSObject *header_dock)
{
  if (header_dock == nil) {
    return;
  }
  [(GHOSTIOSHeaderDock *)header_dock teardown];
  [header_dock release];
}

void GHOST_IOSHeaderDock_setContext(NSObject *header_dock,
                                    const GHOST_IOSInteractionContext &context)
{
  [(GHOSTIOSHeaderDock *)header_dock setContext:context];
}

bool GHOST_IOSHeaderDock_containsView(NSObject *header_dock, UIView *view)
{
  if (header_dock == nil || view == nil) {
    return false;
  }
  return [(GHOSTIOSHeaderDock *)header_dock containsView:view];
}

/** \} */
