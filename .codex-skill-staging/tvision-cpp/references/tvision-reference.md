# TVision Reference

Use this file when the task requires framework-specific detail rather than general C++ guidance.

## Primary Sources

- Official project: `https://github.com/magiblot/tvision`
- Bundled user's guide: `references/borland-turbo-vision-guide.txt`
- Bundled programming guide: `references/turbo-vision-programming-guide-2.0.txt`

Use the GitHub README for current build and platform behavior.

Use the bundled user's guide for:

- the original application model
- class relationships
- the event-driven structure of `TApplication`, `TProgram`, `TGroup`, and `TView`
- the classic menu, status line, and dialog workflow

Use the bundled programming guide for:

- streams and resources
- validators and data-entry patterns
- collections
- editor and text views
- detailed object reference topics

Both bundled guides are large. Search first with `rg` and then open only the matching region.

Good search terms for the user's guide include:

- `TApplication`
- `TProgram`
- `TView`
- `TDialog`
- `TWindow`
- `palette`
- `handleEvent`
- `messageBox`
- `Chapter 13`

Good search terms for the programming guide include:

- `streams`
- `resources`
- `validator`
- `collection`
- `TScroller`
- `TListBox`
- `TEditor`
- `palette`
- `Chapter 17`
- `Chapter 19`

## What TVision Is

Turbo Vision is an event-driven text UI framework. The application is built from views owned by groups, with a central event loop dispatching keyboard, mouse, command, and broadcast events.

The bundled user's guide emphasizes this core model:

- derive your application from `TApplication`
- define menus, status line, and desktop setup in the application skeleton
- let the framework own the main event loop
- handle commands by overriding `handleEvent`

Do not treat a TVision app like a print-to-terminal program with a few key handlers bolted on top.

## Structural Model

Important classes to recognize quickly:

- `TApplication`: top-level application skeleton; usually the main entry point for app-specific behavior
- `TProgram`: provides the main run/execute loop
- `TGroup`: owns child views and routes events
- `TView`: base class for visible and interactive UI elements
- `TWindow`: standard window container
- `TDialog`: modal dialog container
- `TMenuBar`: top menu bar
- `TStatusLine`: bottom status line and shortcuts
- `TScrollBar` and `TScroller`: scrolling infrastructure

Useful mental model:

- `TApplication` is the shell of the app
- `TGroup` manages ownership and dispatch
- `TView` is the unit of drawing and interaction
- command IDs connect menu items, shortcuts, and `handleEvent`

## Event Loop

The guide's key pattern is that `run` comes from `TProgram`, which drives an event loop that repeatedly:

1. gets an event
2. routes the event
3. lets views or the application handle it

When modifying behavior:

- inspect `handleEvent` before changing `draw`
- clear events after consuming them
- check whether the event is positional, focused, command, broadcast, or keyboard input
- preserve the existing routing style if the project already centralizes command handling

If a UI action starts from a menu or status shortcut, trace:

1. command ID definition
2. menu/status item creation
3. `handleEvent` switch or dispatch table
4. dialog/window/view code opened by that command

## Application Skeleton Pattern

Typical TVision applications derive a custom class from `TApplication` and customize some combination of:

- `handleEvent`
- `initMenuBar`
- `initStatusLine`
- `initDeskTop`

When creating a new app or a major feature, mirror that pattern instead of inventing a separate control loop.

## Windows, Dialogs, And Views

### `TWindow`

Use for non-modal or document-style UI containers. Expect window flags, palette behavior, and ownership semantics to matter.

### `TDialog`

Use for modal interactions, forms, confirmations, or short workflows. Dialogs usually contain controls rather than behavior-heavy custom drawing.

### `TView`

Subclass when you need custom drawing or direct input handling. Review at least:

- `draw`
- `handleEvent`
- size and bounds logic
- focus/selectability flags

### `TGroup`

Use to understand ownership, insertion order, event routing, and clipping. Many bugs that look like drawing bugs are really group or event-routing bugs.

## Menus And Status Lines

TVision commonly wires commands through both menus and the status line.

When adding a feature:

1. add the command constant
2. expose it in `TMenuBar` and optionally `TStatusLine`
3. handle it in `handleEvent`
4. launch the dialog, mutate state, or broadcast the action

For small prompts, `messageBox` can be simpler than creating a dialog.

## Geometry And Layout

TVision layout is rectangle-based.

- `TRect` drives bounds
- child views are inserted into owning groups
- insertion order and focus order both matter
- resizing rules may depend on flags and `sizeLimits`

Before changing layout, inspect how the existing code computes bounds. Do not replace that with unrelated geometry abstractions unless the project already uses them.

## Colors And Palettes

Original Turbo Vision used palette indices heavily, and the old guide documents palette behavior in detail. Modern `magiblot/tvision` still supports palette-driven coloring while extending color capabilities.

Prefer:

- using palette lookups and existing color helpers
- preserving local color index conventions
- reading the relevant header comments when a widget's palette is unclear

Avoid:

- hardcoding attributes without checking the widget's palette model
- assuming all terminals render extended colors identically

## Unicode And Modern TVision

`magiblot/tvision` extends the original model with UTF-8 and richer color support.

Important modern points from the README:

- the project targets modern platforms while keeping strong source compatibility
- Unicode-aware APIs exist for input and drawing
- many standard views already support Unicode text correctly
- not every historical shortcut/highlight path is fully Unicode-capable

When debugging text issues:

- verify whether the view is Unicode-aware
- preserve UTF-8 assumptions in modern code
- avoid reintroducing single-byte assumptions into drawing or input code

## Build And Integration Notes

For current `magiblot/tvision`:

- build with CMake
- expect a C++14-capable compiler
- expect `ncursesw` on Linux
- `gpm` may be needed for mouse support on Linux builds
- some systems may also need `tinfow`
- vcpkg can install `tvision`

Common integration clues:

- include style may be `<tvision/tv.h>` in modern code
- legacy compatibility mode may still use `<tv.h>` and Borland compatibility headers
- compatibility include paths may be needed when old headers such as `dir.h` or `iostream.h` appear

Preserve the project's existing integration style unless you are explicitly modernizing it.

## Practical Debugging Heuristics

- If a menu item appears but does nothing, inspect command routing before layout.
- If a view redraws incorrectly, inspect palette mapping and ownership before changing rendering code.
- If keyboard input behaves strangely, inspect `TEvent` handling and command/shortcut collisions.
- If scrolling is broken, inspect both the scroller and the associated scroll bars.
- If the app seems frozen in a long operation, check whether the code needs framework-friendly redraw or event handling rather than raw terminal writes.

## What To Read Next

For deeper source-level detail:

- read the project's headers for class declarations and palette indices
- inspect sample apps such as `hello`, `tvdemo`, `tvedit`, and `palette`
- consult the bundled user's guide when you need the original conceptual model or class descriptions
- consult the bundled programming guide when the task touches validators, resources, streams, collections, or editor-related classes
