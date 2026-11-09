/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 */

#import <UIKit/UIKit.h>

#include "GHOST_IOSStyle.hh"

bool GHOST_IOSStyle_hasGlass()
{
  if (@available(iOS 26.0, *)) {
    return true;
  }
  return false;
}

UIColor *GHOST_IOSStyle_accentColor()
{
  return UIColor.tintColor;
}

UIImage *GHOST_IOSStyle_symbol(NSString *name, const double point_size)
{
  UIImageSymbolConfiguration *config = [UIImageSymbolConfiguration
      configurationWithPointSize:CGFloat(point_size)
                          weight:UIImageSymbolWeightSemibold];
  return [UIImage systemImageNamed:name withConfiguration:config];
}

UIVisualEffectView *GHOST_IOSStyle_createMaterialView(const double corner_radius)
{
  UIVisualEffect *effect = nil;
  if (@available(iOS 26.0, *)) {
    effect = [UIGlassEffect effectWithStyle:UIGlassEffectStyleRegular];
  }
  else {
    effect = [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemThinMaterialDark];
  }
  UIVisualEffectView *view = [[UIVisualEffectView alloc] initWithEffect:effect];
  view.translatesAutoresizingMaskIntoConstraints = NO;
  view.layer.cornerRadius = CGFloat(corner_radius);
  view.layer.cornerCurve = kCACornerCurveContinuous;
  view.clipsToBounds = YES;
  view.userInteractionEnabled = NO;
  return view;
}

void GHOST_IOSStyle_applyFloatingButton(UIButton *button, NSString *symbol)
{
  UIImage *image = GHOST_IOSStyle_symbol(symbol, 14.0);
  button.tintColor = UIColor.whiteColor;
  if (@available(iOS 26.0, *)) {
    UIButtonConfiguration *configuration = [UIButtonConfiguration glassButtonConfiguration];
    configuration.cornerStyle = UIButtonConfigurationCornerStyleCapsule;
    configuration.baseForegroundColor = UIColor.whiteColor;
    configuration.image = image;
    button.configuration = configuration;
  }
  else {
    button.backgroundColor = [UIColor colorWithWhite:0.0f alpha:0.65f];
    button.layer.cornerRadius = CGFloat(GHOST_IOS_CONTROL_SIZE * 0.5);
    button.layer.cornerCurve = kCACornerCurveContinuous;
    [button setImage:image forState:UIControlStateNormal];
  }
}

void GHOST_IOSStyle_selectionFeedback()
{
  UISelectionFeedbackGenerator *generator = [[UISelectionFeedbackGenerator alloc] init];
  [generator selectionChanged];
  [generator release];
}
