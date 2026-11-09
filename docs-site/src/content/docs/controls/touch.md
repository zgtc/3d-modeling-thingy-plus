---
title: Touch and gestures
description: Move the cursor, select objects, and navigate with touch gestures.
---

Watch the [short video guide](/3d-modeling-thingy-ios/controls/video-guide/)
for demonstrations of cursor targeting, navigation, and gesture edge cases.

Use the screen like a trackpad. Drag one finger to move the cursor, then tap
anywhere to click at the cursor. To select a button, move the cursor over it
first. A tap directly on the button only works if the cursor is already there.

![The virtual cursor over the 3D Viewport](/3d-modeling-thingy-ios/touch/virtual-cursor.webp)

## Pointer and clicks

| Gesture | What it does |
| --- | --- |
| One-finger drag | Moves the virtual cursor. Use it to aim at a button, object, field, or node socket. |
| One-finger tap | Left click at the cursor. |
| Tap, then hold and drag | Left-button drag. Tap once, put the same finger down again, hold briefly, then move. Use this for sliders, nodes, gizmos, and box selection. |
| One-finger triple tap | Double-click. Useful for renaming items and other controls that need a desktop double-click. |
| Two-finger hold | Right click. Hold for about 0.3 seconds, then release to open a context menu. Keep holding and move for a right-button drag. |

The cursor moves farther when you swipe quickly. Slow down for small targets.

It also wraps around the screen. Push it past the right edge and it comes back
on the left; the top and bottom work the same way. That lets you keep moving
during a long drag or transform without running out of screen.

Some tools hide the cursor while you're using them. It comes back when
you finish the operation.

## Command bar

The keyboard button at the left or right edge of the screen opens the command
bar. Tap it again, or swipe toward the edge, to tuck the bar away.

From the handle outward, the bar has:

- **Shift, Ctrl, Alt, and Cmd.**
- **Up to six shortcut keys** for the editor and mode under the cursor, such as
  `Tab`, `G`, `R`, `S`, and `X` in Object Mode, or `E` for extrude in Edit Mode.
- **Focus mode**, **Properties**, and **Outliner** buttons.
- **The settings button.**

| Modifier key action | What it does |
| --- | --- |
| Hold | The key stays down while your finger is on it, like a keyboard key. |
| Tap | The key applies to your next click or shortcut key, then turns off. |
| Double tap | The key stays on for every click until you tap it again. A bar under the key shows it's locked. |

To select several objects, faces, or Outliner items, tap **Shift**, then click
the next item. Double tap **Shift** to keep adding items. In the Outliner,
**Ctrl** adds items and **Shift** selects a range. Tap **Shift**, then the `D`
shortcut key, for Shift+D.

Tapped and locked keys only apply to clicks, drags, and shortcut keys.
Two-finger orbit and other navigation gestures keep working normally between
clicks.

Drag the keyboard button to move the bar. It docks to the left or right edge,
in the top or bottom half, and stays there next time. The bar fades while the
cursor is over it, so you can see what's underneath. It hides while a hardware
keyboard is connected.

### Typing values without a keyboard

While a tool takes typed values, such as Move, Rotate, Scale, Extrude, Bevel,
or Inset, a key pad appears next to the bar. It has `X`, `Y`, `Z`, digits, a
minus sign, a decimal point, backspace, **Return**, and **Esc**. For example,
tap `G` on the bar, then `Z`, `2`, and **Return** on the key pad to move the
selection 2 m up.

### Settings

The settings button opens **Touch and Pencil** settings:

- What Apple Pencil double tap and squeeze do. See [Apple Pencil](/3d-modeling-thingy-ios/controls/keyboard-mouse-pencil/#apple-pencil).
- **Only Draw with Apple Pencil.**
- What the two-, three-, and four-finger taps do.
- **Touch Spacing**, which adds room between Blender's buttons, rows, and
  panels so a finger lands on the one you mean. It doesn't change the UI scale.
- Settings sync and resetting your custom keys.

### Customizing keys and the Pencil palette

Hold a shortcut key on the bar, or a slot in the Pencil palette, for about half
a second to replace it. Type one of these:

| Type | Example | What it does |
| --- | --- | --- |
| A key | `Shift+D`, `Ctrl+R`, `F3`, `Tab` | Sends that key, with modifiers. |
| An operator | `mesh.subdivide`, `object.shade_smooth keep_sharp_edges=1` | Runs the operator, optionally with one setting. |
| A menu or pie | `VIEW3D_MT_add`, `VIEW3D_MT_pivot_pie` | Opens the menu or pie. |

Each editor and mode keeps its own set. **Reset to Default** restores one key;
**Reset Custom Slots and Keys** in settings restores them all.

## Panels

| Action | How |
| --- | --- |
| Show or hide the toolbar (`T`) | Swipe in from the left edge of the screen. |
| Show or hide the sidebar (`N`) | Swipe in from the right edge of the screen. |
| Resize a toolbar or sidebar | Drag the white grab handle on its edge. |
| Focus mode | Tap the Focus button on the command bar, or choose **Window > Focus Mode**. The editor under the cursor fills the screen with its panels hidden. Do it again to go back. |
| Properties or Outliner | Tap its button on the command bar, or choose it from the **Window** menu, to open it in a floating window. |

Edge swipes act on the editor where the swipe ends. They only start at the very
edge of the screen, so they don't get in the way of Blender's own controls.

Every visible toolbar and sidebar has a grab handle on its resizable edge. The
handle is a full fingertip wide and highlights while you drag. It resizes the
panel the same way Blender's own edge does, so panel sizes are saved with your
file.

## Menu bar

Swipe down from the top of the screen, or hold `Cmd` on a hardware keyboard,
to show the iPadOS menu bar:

- **File**, **Edit**, **Render**, **Window**, and **Help**, the same as
  Blender's top bar.
- In the 3D Viewport, a **Viewport** menu and an **Object**, **Mesh**, or
  **Sculpt** menu for the current mode.

The menu items have no keyboard shortcuts of their own, so every key still goes
to Blender.

## Settings sync

Touch and Pencil settings, including your custom keys and palette slots, are
saved in `touch-settings.json` in the app's **Settings** folder. Blender's own
preferences stay separate.

To use the same settings on another iPad, choose **Sync Through a Folder…** in
settings and pick a folder, for example in iCloud Drive. Pick the same folder
on the other iPad. Each time the app opens or comes back to the front, it uses
whichever copy was saved most recently.

Changes made on one iPad show up on the other the next time the app opens or
returns to the front, not while both are open. Without a sync folder, or when
the folder can't be reached, the iPad's own copy is used.

## Moving around an editor

First, move the cursor into the editor you want to use. Gestures go to the
editor under the cursor.

| Gesture | 3D Viewport | Shader Editor, Geometry Nodes, and other 2D editors |
| --- | --- | --- |
| Two-finger drag | Orbit | Pan the canvas or scroll |
| Pinch | Zoom | Zoom |
| Three-finger drag | Pan | Use two fingers for ordinary 2D panning |

You can drag with two fingers and pinch at the same time to orbit and zoom.

In the Shader Editor, move the cursor over the nodes, then use two fingers to
pan and pinch to zoom. To move a node or connect sockets, aim the cursor and use
tap, then hold and drag.

The same cursor rule applies to the Outliner, timeline, and Properties. Move
the cursor there before trying to scroll or zoom.

## Undo, redo, and search

| Gesture | Command |
| --- | --- |
| Two-finger tap | Undo |
| Three-finger tap | Redo |
| Four-finger tap | Search, the same as `F3` |

You can change each one in settings to Undo, Redo, Search, Focus Mode, or
Nothing. Keep these taps quick and still. Holding two fingers starts a right click;
moving them starts navigation.
