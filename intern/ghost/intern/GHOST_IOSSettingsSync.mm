/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * Storage of the touch layer's settings file, and its sync through a folder the user picks.
 */

#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <os/log.h>

#include "GHOST_IOSSettingsSync.hh"

NSString *const GHOST_IOSTouchSettingsDidChangeNotification =
    @"GHOST_IOSTouchSettingsDidChangeNotification";

static NSString *const GHOST_IOSSettingsFileName = @"touch-settings.json";
static NSString *const GHOST_IOSSyncBookmarkKey = @"GHOST_IOSTouchSettingsSyncBookmark";

/* Keys of the settings before they moved to the file. */
static NSString *const GHOST_IOSLegacyDoubleTapKey = @"GHOST_IOSPencilDoubleTap";
static NSString *const GHOST_IOSLegacySqueezeKey = @"GHOST_IOSPencilSqueeze";
static NSString *const GHOST_IOSLegacyPencilOnlyKey = @"GHOST_IOSPencilOnlyDrawing";
static NSString *const GHOST_IOSLegacyTapKeys[GHOST_IOS_TAP_GESTURE_COUNT] = {
    @"GHOST_IOSTapGesture2",
    @"GHOST_IOSTapGesture3",
    @"GHOST_IOSTapGesture4",
};

static os_log_t GHOST_IOSSettingsSyncLog()
{
  static os_log_t log = os_log_create("org.blender.ios", "touch-settings");
  return log;
}

/* -------------------------------------------------------------------- */
/** \name Files
 * \{ */

static NSURL *GHOST_IOSSettings_localURL()
{
  NSFileManager *file_manager = NSFileManager.defaultManager;
  NSURL *documents =
      [file_manager URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask].firstObject;
  NSURL *folder = [documents URLByAppendingPathComponent:@"Settings" isDirectory:YES];
  [file_manager createDirectoryAtURL:folder
         withIntermediateDirectories:YES
                          attributes:nil
                               error:nil];
  return [folder URLByAppendingPathComponent:GHOST_IOSSettingsFileName];
}

static bool GHOST_IOSSettings_readURL(NSURL *url, GHOST_IOSSettingsFile *r_file, bool coordinated)
{
  __block NSData *data = nil;
  if (coordinated) {
    NSFileCoordinator *coordinator = [[[NSFileCoordinator alloc] initWithFilePresenter:nil]
        autorelease];
    NSError *error = nil;
    [coordinator coordinateReadingItemAtURL:url
                                    options:NSFileCoordinatorReadingWithoutChanges
                                      error:&error
                                 byAccessor:^(NSURL *reading_url) {
                                   data = [[NSData dataWithContentsOfURL:reading_url] retain];
                                 }];
    [data autorelease];
  }
  else {
    data = [NSData dataWithContentsOfURL:url];
  }
  if (data.length == 0) {
    return false;
  }
  std::string json(static_cast<const char *>(data.bytes), data.length);
  return GHOST_IOSSettingsFile_read(json.c_str(), r_file);
}

static bool GHOST_IOSSettings_writeURL(NSURL *url,
                                       const GHOST_IOSSettingsFile &file,
                                       bool coordinated)
{
  const std::string json = GHOST_IOSSettingsFile_write(file);
  NSData *data = [NSData dataWithBytes:json.data() length:json.size()];
  __block BOOL ok = NO;
  if (coordinated) {
    NSFileCoordinator *coordinator = [[[NSFileCoordinator alloc] initWithFilePresenter:nil]
        autorelease];
    NSError *error = nil;
    [coordinator coordinateWritingItemAtURL:url
                                    options:NSFileCoordinatorWritingForReplacing
                                      error:&error
                                 byAccessor:^(NSURL *writing_url) {
                                   ok = [data writeToURL:writing_url atomically:YES];
                                 }];
  }
  else {
    ok = [data writeToURL:url atomically:YES];
  }
  return ok;
}

/** The picked sync folder with access started, or nil. Call #GHOST_IOSSettings_endSync after. */
static NSURL *GHOST_IOSSettings_beginSync()
{
  NSData *bookmark = [NSUserDefaults.standardUserDefaults dataForKey:GHOST_IOSSyncBookmarkKey];
  if (bookmark == nil) {
    return nil;
  }
  BOOL stale = NO;
  NSError *error = nil;
  NSURL *folder = [NSURL URLByResolvingBookmarkData:bookmark
                                            options:0
                                      relativeToURL:nil
                                bookmarkDataIsStale:&stale
                                              error:&error];
  if (folder == nil || ![folder startAccessingSecurityScopedResource]) {
    os_log_info(GHOST_IOSSettingsSyncLog(), "Sync folder unavailable; using local settings");
    return nil;
  }
  if (stale) {
    NSData *refreshed = [folder bookmarkDataWithOptions:0
                         includingResourceValuesForKeys:nil
                                          relativeToURL:nil
                                                  error:nil];
    if (refreshed != nil) {
      [NSUserDefaults.standardUserDefaults setObject:refreshed forKey:GHOST_IOSSyncBookmarkKey];
    }
  }
  return folder;
}

static void GHOST_IOSSettings_endSync(NSURL *folder)
{
  [folder stopAccessingSecurityScopedResource];
}

/** Settings from before the file existed. */
static GHOST_IOSSettingsFile GHOST_IOSSettings_legacy()
{
  NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
  GHOST_IOSSettingsFile file;
  GHOST_IOSTouchSettings &touch = file.touch;
  touch.double_tap = GHOST_IOSDoubleTapChoice_fromStored(
      [defaults integerForKey:GHOST_IOSLegacyDoubleTapKey]);
  touch.squeeze = GHOST_IOSSqueezeChoice_fromStored(
      [defaults integerForKey:GHOST_IOSLegacySqueezeKey]);
  /* Until the user picks, follow the system "Only Draw with Apple Pencil" setting. */
  touch.pencil_only_drawing = [defaults objectForKey:GHOST_IOSLegacyPencilOnlyKey] != nil ?
                                  [defaults boolForKey:GHOST_IOSLegacyPencilOnlyKey] :
                                  UIPencilInteraction.prefersPencilOnlyDrawing;
  for (int i = 0; i < GHOST_IOS_TAP_GESTURE_COUNT; i++) {
    if ([defaults objectForKey:GHOST_IOSLegacyTapKeys[i]] != nil) {
      touch.tap_gestures[i] = GHOST_IOSGestureAction_fromStored(
          [defaults integerForKey:GHOST_IOSLegacyTapKeys[i]], touch.tap_gestures[i]);
    }
  }
  return file;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Shared settings
 * \{ */

static GHOST_IOSSettingsFile *GHOST_IOSSettings_storage = nullptr;

/** Use the synced copy when it is newer, or push ours when it is older. Returns true on change. */
static bool GHOST_IOSSettings_mergeSyncFolder()
{
  NSURL *folder = GHOST_IOSSettings_beginSync();
  if (folder == nil) {
    return false;
  }
  GHOST_IOSSettingsFile &local = *GHOST_IOSSettings_storage;
  NSURL *remote_url = [folder URLByAppendingPathComponent:GHOST_IOSSettingsFileName];
  GHOST_IOSSettingsFile remote;
  bool changed = false;
  if (GHOST_IOSSettings_readURL(remote_url, &remote, true) && remote.saved_at > local.saved_at) {
    local = remote;
    GHOST_IOSSettings_writeURL(GHOST_IOSSettings_localURL(), local, false);
    os_log_info(GHOST_IOSSettingsSyncLog(), "Using newer touch settings from the sync folder");
    changed = true;
  }
  else if (remote.saved_at < local.saved_at || ![NSFileManager.defaultManager
                                                   fileExistsAtPath:remote_url.path])
  {
    GHOST_IOSSettings_writeURL(remote_url, local, true);
  }
  GHOST_IOSSettings_endSync(folder);
  return changed;
}

static void GHOST_IOSSettings_notifyChanged()
{
  [NSNotificationCenter.defaultCenter
      postNotificationName:GHOST_IOSTouchSettingsDidChangeNotification
                    object:nil];
}

GHOST_IOSSettingsFile &GHOST_IOSSettings_shared()
{
  if (GHOST_IOSSettings_storage != nullptr) {
    return *GHOST_IOSSettings_storage;
  }
  GHOST_IOSSettings_storage = new GHOST_IOSSettingsFile();
  NSURL *local_url = GHOST_IOSSettings_localURL();
  if (!GHOST_IOSSettings_readURL(local_url, GHOST_IOSSettings_storage, false)) {
    /* First run with the file: keep what the user already chose. */
    *GHOST_IOSSettings_storage = GHOST_IOSSettings_legacy();
    GHOST_IOSSettings_writeURL(local_url, *GHOST_IOSSettings_storage, false);
  }
  GHOST_IOSSettings_mergeSyncFolder();

  /* Another iPad may have saved newer settings while this app was in the background. */
  [NSNotificationCenter.defaultCenter addObserverForName:UIApplicationDidBecomeActiveNotification
                                                  object:nil
                                                   queue:NSOperationQueue.mainQueue
                                              usingBlock:^(NSNotification *) {
                                                if (GHOST_IOSSettings_mergeSyncFolder()) {
                                                  GHOST_IOSSettings_notifyChanged();
                                                }
                                              }];
  return *GHOST_IOSSettings_storage;
}

void GHOST_IOSSettings_save()
{
  GHOST_IOSSettingsFile &file = GHOST_IOSSettings_shared();
  file.version = GHOST_IOS_SETTINGS_VERSION;
  file.saved_at = NSDate.date.timeIntervalSince1970;
  GHOST_IOSSettings_writeURL(GHOST_IOSSettings_localURL(), file, false);
  if (NSURL *folder = GHOST_IOSSettings_beginSync()) {
    GHOST_IOSSettings_writeURL(
        [folder URLByAppendingPathComponent:GHOST_IOSSettingsFileName], file, true);
    GHOST_IOSSettings_endSync(folder);
  }
}

NSString *GHOST_IOSSettings_syncFolderName()
{
  NSURL *folder = GHOST_IOSSettings_beginSync();
  if (folder == nil) {
    return nil;
  }
  NSString *name = folder.lastPathComponent;
  GHOST_IOSSettings_endSync(folder);
  return name;
}

void GHOST_IOSSettings_stopSync()
{
  [NSUserDefaults.standardUserDefaults removeObjectForKey:GHOST_IOSSyncBookmarkKey];
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Folder picker
 * \{ */

@interface GHOSTIOSSyncFolderPicker : NSObject <UIDocumentPickerDelegate>
@end

@implementation GHOSTIOSSyncFolderPicker

- (void)documentPicker:(UIDocumentPickerViewController *)controller
    didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls
{
  (void)controller;
  NSURL *folder = urls.firstObject;
  if (folder != nil && [folder startAccessingSecurityScopedResource]) {
    NSData *bookmark = [folder bookmarkDataWithOptions:0
                        includingResourceValuesForKeys:nil
                                         relativeToURL:nil
                                                 error:nil];
    [folder stopAccessingSecurityScopedResource];
    if (bookmark != nil) {
      [NSUserDefaults.standardUserDefaults setObject:bookmark forKey:GHOST_IOSSyncBookmarkKey];
      os_log_info(GHOST_IOSSettingsSyncLog(), "Touch settings now sync through a picked folder");
      GHOST_IOSSettings_shared();
      GHOST_IOSSettings_mergeSyncFolder();
      GHOST_IOSSettings_notifyChanged();
    }
  }
  [self release];
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller
{
  (void)controller;
  [self release];
}

@end

void GHOST_IOSSettings_chooseSyncFolder(UIViewController *presenter)
{
  if (presenter == nil || presenter.presentedViewController != nil) {
    return;
  }
  UIDocumentPickerViewController *picker = [[[UIDocumentPickerViewController alloc]
      initForOpeningContentTypes:@[ UTTypeFolder ]] autorelease];
  /* Released by itself when the picker finishes. */
  GHOSTIOSSyncFolderPicker *delegate = [[GHOSTIOSSyncFolderPicker alloc] init];
  picker.delegate = delegate;
  picker.allowsMultipleSelection = NO;
  [presenter presentViewController:picker animated:YES completion:nil];
}

/** \} */
