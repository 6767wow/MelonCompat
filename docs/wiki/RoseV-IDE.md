# RoseV IDE

RoseV IDE is a VS Code-style editor for `.rosev` files.

It is separate from the MelonCompat installer. The installer installs loaders into games. The IDE writes RoseV source and runs the RoseV compiler.

## Location

```text
RoseV/IDE
```

## Goals

The IDE is designed for:

- Fast beginner onboarding.
- Editing `.rosev` files.
- Opening RoseV workspaces.
- Seeing a file outline.
- Inserting common snippets.
- Inserting full C# statement blocks and class member blocks.
- Declaring native C/C++/ASM companion bridges.
- Viewing diagnostics.
- Running the native RoseV compiler.
- Viewing generated C#.

## Layout

The IDE uses a familiar workbench layout:

- Activity bar on the far left.
- Explorer/sidebar next to it.
- Main editor in the center.
- Diagnostics/output/generated-code panel at the bottom.
- Command palette through `Ctrl+Shift+P`.

## Main Commands

Buttons:

- `New`: create a new `.rosev` source.
- `Open`: open a `.rosev` file.
- `Folder`: open a workspace folder.
- `Sample`: load a complete sample.
- `Save`: save the current file.
- `Compile`: run `RoseV.exe` and generate C#.
- `Commands`: open the command palette.

Useful snippets:

- `import all`: built-in C#/.NET, Unity, MelonLoader, BepInEx, and RoseMod imports.
- `C# block`: full C# inside an event or function.
- `C# members`: full C# fields, properties, methods, or nested classes on the generated class.
- `native`: C/C++/ASM companion declaration plus a native call function.

Keyboard:

```text
Ctrl+S        Save
Ctrl+Shift+P  Command palette
Esc           Close command palette
Tab           Insert two spaces
```

## Diagnostics

The IDE has lightweight live checks:

- Missing `rosev` metadata.
- Unknown commands.
- Unclosed quoted text.
- Missing `{` on block commands.
- Extra `}`.
- Missing closing `}`.
- New all-in-one commands such as `members`, `csharp`, `repeat`, `try`, and `native`.

The native compiler is still the final source of truth.

## Compile Behavior

Compile runs:

```text
RoseV.exe compile <source.rosev> -o <source.generated.cs>
```

If the file is unsaved, the IDE writes a temp `.rosev` source before compile.

The generated C# appears in the bottom `Generated` tab.

## Browser Preview

The IDE frontend can be previewed without Tauri:

```text
npm run preview
```

Browser preview can edit and validate syntax, but cannot run `RoseV.exe`.

The desktop Tauri build is required for open/save/compile integration.

## Tauri Build

```text
cd RoseV/IDE
npm install
npm run build
```

During development:

```text
npm run dev
```

## Native Compiler Requirement

The IDE expects the native compiler at a normal repo build path:

```text
bin/RoseV/Release/RoseV.exe
```

Build it first:

```text
msbuild RoseV/Native/RoseV.Native.vcxproj /p:Configuration=Release /p:Platform=x64
```

## Current Limitations

The IDE is intentionally lightweight right now.

Missing features that can be added later:

- Rich syntax highlighting.
- IntelliSense-style completions.
- Automatic DLL packaging.
- Project templates.
- Built-in loader wrapper generation.
- Automatic native DLL compilation.
- Debug log viewer tied to a selected game.
- One-click install into `RoseMod/MelonMods`.
