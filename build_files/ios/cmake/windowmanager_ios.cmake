# SPDX-FileCopyrightText: 2026 Blender Authors
#
# SPDX-License-Identifier: GPL-2.0-or-later

list(APPEND SRC intern/wm_platform_ios.cc)
add_definitions(-DBLENDER_PLATFORM_INIT_AFTER_HOMEFILE=wm_init_ios_cycles_smoke)
add_definitions(-DBLENDER_PLATFORM_MAIN_LOOP_HOOK=wm_ios_touch_channel_update)
add_definitions(-DBLENDER_PLATFORM_SPLASH_FIT)
