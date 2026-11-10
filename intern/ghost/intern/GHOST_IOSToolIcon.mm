/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * Draw Blender's own tool icons (see #GHOST_IOSToolIcon_parse) for the iPad's rail.
 */

#import <UIKit/UIKit.h>

#include "GHOST_IOSToolIcon.hh"

#include <algorithm>

/** The app's `datafiles/icons`, under `Assets/<version>`, or nil. Found once. */
static NSString *GHOST_IOSToolIcon_directory()
{
  static NSString *directory = nil;
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    NSString *assets = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"Assets"];
    NSFileManager *files = NSFileManager.defaultManager;
    for (NSString *version in [files contentsOfDirectoryAtPath:assets error:nil]) {
      NSString *candidate = [[assets stringByAppendingPathComponent:version]
          stringByAppendingPathComponent:@"datafiles/icons"];
      BOOL is_directory = NO;
      if ([files fileExistsAtPath:candidate isDirectory:&is_directory] && is_directory) {
        directory = [candidate retain];
        break;
      }
    }
  });
  return directory;
}

UIImage *GHOST_IOSToolIcon_image(const char *name, const double points)
{
  if (name == nullptr || name[0] == '\0') {
    return nil;
  }
  static NSCache<NSString *, UIImage *> *cache = [[NSCache alloc] init];
  NSString *key = [NSString stringWithFormat:@"%s@%.1f", name, points];
  if (UIImage *cached = [cache objectForKey:key]) {
    return cached;
  }
  NSString *directory = GHOST_IOSToolIcon_directory();
  if (directory == nil) {
    return nil;
  }
  NSString *path = [directory
      stringByAppendingPathComponent:[NSString stringWithFormat:@"%s.dat", name]];
  NSData *data = [NSData dataWithContentsOfFile:path];
  GHOST_IOSToolIconGeometry icon;
  if (data == nil ||
      !GHOST_IOSToolIcon_parse(static_cast<const uint8_t *>(data.bytes), data.length, &icon))
  {
    return nil;
  }
  const CGFloat scale = CGFloat(points) / CGFloat(std::max(icon.width, icon.height));
  UIGraphicsImageRenderer *renderer = [[[UIGraphicsImageRenderer alloc]
      initWithSize:CGSizeMake(points, points)] autorelease];
  UIImage *image = [renderer imageWithActions:^(UIGraphicsImageRendererContext *renderer_context) {
    CGContextRef context = renderer_context.CGContext;
    CGContextScaleCTM(context, scale, scale);
    for (int i = 0; i < icon.triangles; i++) {
      GHOST_IOSToolIconTriangle triangle;
      GHOST_IOSToolIcon_triangle(icon, i, &triangle);
      if (triangle.rgba[3] == 0) {
        continue;
      }
      CGContextSetRGBFillColor(context,
                               triangle.rgba[0] / 255.0,
                               triangle.rgba[1] / 255.0,
                               triangle.rgba[2] / 255.0,
                               triangle.rgba[3] / 255.0);
      CGContextMoveToPoint(context, triangle.x[0], triangle.y[0]);
      CGContextAddLineToPoint(context, triangle.x[1], triangle.y[1]);
      CGContextAddLineToPoint(context, triangle.x[2], triangle.y[2]);
      CGContextClosePath(context);
      CGContextFillPath(context);
    }
  }];
  image = [image imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
  [cache setObject:image forKey:key];
  return image;
}
