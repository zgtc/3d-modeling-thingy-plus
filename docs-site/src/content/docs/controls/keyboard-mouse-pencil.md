---
title: Keyboard, mouse, and Apple Pencil
description: Use a keyboard, mouse, trackpad, or Apple Pencil with 3D Modeling Thingy.
---

## Mouse and trackpad

The left, middle, and right buttons use Blender's usual controls. Drag with the
middle button to orbit, hold Shift and drag with the middle button to pan, and
right click for a context menu.

Point at the editor you want to use before scrolling or pinching on a trackpad.
The app draws its own cursor, which can wrap around the window edges during
continuous movement.

## Hardware keyboard

Use your usual Blender shortcuts: `G` to move, `R` to rotate, `S` to scale,
`Tab` to change modes, and `F3` to search. Number-row keys, arrows, modifiers,
function keys, and numpad keys are supported when your keyboard has them.

Press `Command-W` to close a secondary window, such as a file browser or
render view. You can also use the close button at the top right.

## Typing into fields

Editing a field opens the iOS text editor. You can type a name, a number, or an
expression such as `3+4`. Tap **Done** to keep the edit or **Cancel** to restore
the old value.

After you finish, hardware keyboard shortcuts control the app again.

This clip shows entering `3+4` in a numeric field and confirming the result, `7 m`.

<video controls playsinline preload="metadata" aria-label="Enter a numeric expression with the iPad keyboard" poster="/3d-modeling-thingy-ios/text/text-entry.webp">
  <source src="/3d-modeling-thingy-ios/text/text-entry.mp4" type="video/mp4" />
  <a href="/3d-modeling-thingy-ios/text/text-entry.mp4">Watch the text entry demo.</a>
</video>

## Apple Pencil

Pencil points directly where its tip touches the screen. Tap to left click, or
keep the tip down to draw or drag. The tools receive pressure and tilt.

Hover moves the cursor on iPads that support it. In Sculpt and paint modes,
Blender's brush circle follows the hovering tip.

You can use finger gestures to navigate while working with Pencil.

### Squeeze palette

On Apple Pencil Pro, squeeze and release to open a ring of eight commands
under the tip. Touch or hover over a command to highlight it, with a haptic tap
in the Pencil, and lift to run it. Lift in the middle or outside the ring to
close it.

| Mode | Commands |
| --- | --- |
| Object | Move, Rotate, Scale, Add, Delete, Duplicate, Undo, Redo |
| Edit | Vertex, Edge, Face, Extrude, Inset, Loop Cut, Knife, Merge |
| Sculpt | Radius, Strength, Draw, Smooth, Mask, Invert, Undo, Redo |
| Paint | Radius, Strength, Invert, Swap Colors, Stroke, Frame, Undo, Redo |

**Radius** and **Strength** follow the Pencil as you drag. **Invert** keeps
**Ctrl** on for your next strokes, which inverts the brush, until you choose it
again.

Squeeze follows your system setting under **Settings > Apple Pencil**. In the
command bar settings you can choose **Show Palette** to always open it, or
**Off**. A system shortcut chosen for squeeze always runs instead.

Hold a palette command for about half a second to replace it. See
[Customizing keys and the Pencil palette](/3d-modeling-thingy-ios/controls/touch/#customizing-keys-and-the-pencil-palette).

### Double tap

The first time you double tap Apple Pencil 2 or Pro, the app asks what double
tap should do:

- **Right Click**
- **Previous Tool**
- **Show Palette**
- **Invert Brush**
- **Follow System Setting**

Nothing else happens on that first tap. Your answer is saved, and you can change
it later in the command bar settings.

### Only Draw with Apple Pencil

When **Only Draw with Apple Pencil** is on, fingers can't click, select, or
paint in the 3D Viewport or the Image Editor. A finger drag there orbits the
view instead. Headers, toolbars, and other editors still work with a finger.
This setting starts out matching your iPad's own Apple Pencil setting.

Hold an on-screen modifier key with a finger while you draw with Pencil. For
example, hold **Shift** to smooth while sculpting, or **Ctrl** to invert a
brush. See [Command bar](/3d-modeling-thingy-ios/controls/touch/#command-bar).
The on-screen keys hide while a hardware keyboard is connected; use its own
modifier keys instead.
