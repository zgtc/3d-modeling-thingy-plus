/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * Blender's menus in the iPadOS menu bar.
 *
 * Items carry no key equivalents, so the menu bar never takes a key press away from Blender's
 * keymap. They only queue operator calls, like the command bar does.
 */

#import <UIKit/UIKit.h>

#include "GHOST_IOSMenuBar.hh"

static GHOST_IOSInteractionContext GHOST_IOSMenuBar_context;

static UIMenu *GHOST_IOSMenuBar_menu(const GHOST_IOSMenu &menu, const bool inline_items)
{
  NSMutableArray<UIMenuElement *> *children = [NSMutableArray array];
  for (int i = 0; i < menu.count; i++) {
    char command[GHOST_IOS_COMMAND_MAX];
    if (!GHOST_IOSMenuItem_command(menu.items[i], command)) {
      continue;
    }
    [children
        addObject:[UICommand commandWithTitle:[NSString stringWithUTF8String:menu.items[i].title]
                                        image:nil
                                       action:NSSelectorFromString(@"blenderMenuCommand:")
                                 propertyList:[NSString stringWithUTF8String:command]]];
  }
  NSString *identifier = [NSString stringWithFormat:@"org.blender.menu.%s", menu.id];
  return [UIMenu menuWithTitle:[NSString stringWithUTF8String:menu.title]
                         image:nil
                    identifier:identifier
                       options:inline_items ? UIMenuOptionsDisplayInline : 0
                      children:children];
}

static const GHOST_IOSMenu *GHOST_IOSMenuBar_find(const char *id)
{
  const GHOST_IOSMenu *menus = nullptr;
  const int count = GHOST_IOSMenuBar_topMenus(&menus);
  for (int i = 0; i < count; i++) {
    if (std::strcmp(menus[i].id, id) == 0) {
      return &menus[i];
    }
  }
  return nullptr;
}

void GHOST_IOSMenuBar_build(id<UIMenuBuilder> builder)
{
  /* File and Edit replace the system's own items, which do not apply to Blender's canvas. */
  UIMenu *file = GHOST_IOSMenuBar_menu(*GHOST_IOSMenuBar_find("file"), true);
  [builder replaceChildrenOfMenuForIdentifier:UIMenuFile
                            fromChildrenBlock:^(NSArray<UIMenuElement *> *) {
                              return @[ file ];
                            }];
  UIMenu *edit = GHOST_IOSMenuBar_menu(*GHOST_IOSMenuBar_find("edit"), true);
  [builder replaceChildrenOfMenuForIdentifier:UIMenuEdit
                            fromChildrenBlock:^(NSArray<UIMenuElement *> *) {
                              return @[ edit ];
                            }];
  if ([builder menuForIdentifier:UIMenuFormat] != nil) {
    [builder removeMenuForIdentifier:UIMenuFormat];
  }

  /* Render, then the menus of the editor and mode under the pointer. */
  UIMenu *render = GHOST_IOSMenuBar_menu(*GHOST_IOSMenuBar_find("render"), false);
  [builder insertSiblingMenu:render afterMenuForIdentifier:UIMenuEdit];
  const GHOST_IOSMenu *editor_menus = nullptr;
  const int editor_count = GHOST_IOSMenuBar_editorMenus(GHOST_IOSMenuBar_context, &editor_menus);
  UIMenuIdentifier after = render.identifier;
  for (int i = 0; i < editor_count; i++) {
    UIMenu *menu = GHOST_IOSMenuBar_menu(editor_menus[i], false);
    [builder insertSiblingMenu:menu afterMenuForIdentifier:after];
    after = menu.identifier;
  }

  /* Window and Help keep the system's items and gain Blender's at the top. */
  [builder insertChildMenu:GHOST_IOSMenuBar_menu(*GHOST_IOSMenuBar_find("window"), true)
      atStartOfMenuForIdentifier:UIMenuWindow];
  [builder insertChildMenu:GHOST_IOSMenuBar_menu(*GHOST_IOSMenuBar_find("help"), true)
      atStartOfMenuForIdentifier:UIMenuHelp];
}

void GHOST_IOSMenuBar_setContext(const GHOST_IOSInteractionContext &context)
{
  if (!GHOST_IOSMenuBar_needsRebuild(GHOST_IOSMenuBar_context, context)) {
    GHOST_IOSMenuBar_context = context;
    return;
  }
  GHOST_IOSMenuBar_context = context;
  [UIMenuSystem.mainSystem setNeedsRebuild];
}
