---
name: tvision-cpp
description: Work effectively in C++ projects that use the Turbo Vision or TVision text UI framework, including magiblot/tvision. Use when Codex needs to create, modify, debug, or explain terminal UI code built around classes such as TApplication, TProgram, TView, TGroup, TWindow, TDialog, TMenuBar, TStatusLine, TScroller, or TScrollBar, or when the task involves TVision-specific build setup, event handling, palettes, Unicode, dialogs, menus, and status lines.
---

# TVision C++

## Overview

Use this skill when working inside a Turbo Vision codebase or when building a new TVision-based terminal UI in C++. Start by identifying the application's structure and build conventions, then apply framework-specific patterns instead of treating it like a generic CLI program.

## Workflow

1. Inspect the codebase before editing.
2. Identify how the project integrates TVision.
3. Map the application's view tree and command flow.
4. Make framework-aligned changes.
5. Build or test with the project's existing commands.

## Inspect The Codebase

Locate the integration points first:

- Find the build system entry points such as `CMakeLists.txt`, `Makefile`, or vcpkg manifests.
- Find the application class derived from `TApplication` or `TProgram`.
- Find where menus, status lines, dialogs, windows, and custom views are created.
- Find command constants such as `cmFoo` and where they are handled in `handleEvent`.
- Check whether the project uses modern includes like `<tvision/tv.h>` or compatibility headers such as `<tv.h>`.

If the task is ambiguous, answer these questions before changing code:

- Is this a modern `magiblot/tvision` codebase or an older Borland-compatible layout?
- Are we extending an existing view hierarchy or adding a new window/dialog flow?
- Does the project already have a command-routing pattern for menu actions, broadcasts, and dialogs?

## Follow The Project's TVision Style

Preserve local conventions instead of imposing a new structure:

- Keep the existing include style.
- Reuse the project's command IDs, palette conventions, and event-routing approach.
- Prefer extending existing `TView`, `TWindow`, or `TDialog` subclasses over inserting unrelated abstractions.
- Keep layout logic in terms of `TRect`, insertion order, and the owning `TGroup`.
- Avoid bypassing the framework with ad hoc terminal I/O unless the codebase already does so intentionally.

## Implement Common Changes

### Add UI Commands

When adding a feature reachable from the UI:

1. Define or reuse a command ID.
2. Add the menu item or status item.
3. Handle the command in `handleEvent`.
4. Clear the event after consuming it.
5. Open a dialog/window or update the relevant view.

### Add A Dialog Or Window

When adding modal UI:

- Create the `TDialog` or `TWindow` with an explicit `TRect`.
- Insert child controls in a deliberate order.
- Use existing validators, histories, scroll bars, and buttons where possible.
- Prefer `messageBox` only for simple prompts or confirmations.
- If the dialog returns a value, keep the result handling close to the `execute` call site.

### Add Or Fix Custom Views

When touching a custom `TView` subclass:

- Check `draw`, `handleEvent`, `sizeLimits`, and focus behavior together.
- Verify palette usage before hardcoding attributes.
- Keep screen updates inside the framework's drawing model.
- Respect scrolling, clipping, and ownership rules from the containing `TGroup`.

## Build And Debug

- Use the project's existing build commands first.
- For Linux builds, expect `ncursesw` and sometimes `gpm` or `tinfow` in the link line.
- For CMake-based setups, keep compile requirements aligned with TVision's C++14 baseline unless the project has raised it.
- If behavior looks wrong, inspect event flow before assuming drawing bugs.
- When a busy loop must visibly refresh the screen, check whether `TScreen::flushScreen()` is the intended fix.

## Reference Files

Read [references/tvision-reference.md](references/tvision-reference.md) when you need:

- the TVision object model and event loop
- build and integration details for `magiblot/tvision`
- guidance on menus, dialogs, palettes, Unicode, and common classes
- deep reference material for streams, resources, validators, collections, editors, and custom views
- the bundled Borland Turbo Vision guide for deeper lookup

For the bundled historical guides, search first and then open only the relevant chunk.

- Use [references/borland-turbo-vision-guide.txt](references/borland-turbo-vision-guide.txt) for application structure, menus, dialogs, event flow, and the classic object model.
- Use [references/turbo-vision-programming-guide-2.0.txt](references/turbo-vision-programming-guide-2.0.txt) for streams, resources, validators, collections, editors, palettes, and detailed reference material.

## Deliverables

When finishing work in a TVision codebase, report:

- what part of the application structure you changed
- which commands, views, or dialogs were affected
- what build or test command you ran
- any remaining uncertainty around event routing, palette behavior, or terminal-specific behavior
