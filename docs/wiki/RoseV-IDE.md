# RoseV IDE

RoseV IDE is a native PyQt editor for `.rosev` files.

It is separate from the MelonCompat installer. The installer installs loaders into games. The IDE writes RoseV source and runs the RoseV compiler.

## Location

```text
RoseV/IDE
```

## Goals

The IDE is designed for:

- Fast beginner onboarding.
- Editing `.rosev` files.
- Native code scrolling with line numbers.
- Opening RoseV workspaces.
- Seeing a file outline.
- Inserting common snippets.
- Inserting full C# statement blocks and class member blocks.
- Declaring native C/C++/ASM companion bridges.
- Finding text in the current file.
- Formatting RoseV indentation.
- Viewing diagnostics.
- Running the native RoseV compiler.
- Viewing generated C#.

## Layout

The IDE uses a native desktop workbench layout:

- RoseMod profile card and file/code/help tabs on the left.
- Main editor in the center.
- Diagnostics/output/generated-code panel at the bottom.
- Command palette through `Ctrl+Shift+P`.

## Main Commands

Buttons:

- `New`: create a new `.rosev` source.
- `Open`: open a `.rosev` file.
- `Folder`: open a workspace folder.
- `Sample`: load a complete sample.
- `Format`: normalize indentation.
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
Ctrl+O        Open
Ctrl+Enter    Compile
Ctrl+Shift+F  Format document
Ctrl+Shift+P  Command palette
Ctrl+F        Find in file
Alt+Z         Toggle word wrap
Esc           Close command palette
Tab           Indent selection or insert two spaces
Shift+Tab     Unindent selection
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

## PyQt Run

Install PyQt once:

```powershell
cd RoseV/IDE
python -m pip install -r requirements.txt
```

Run the IDE:

```powershell
python rosev_ide.py
```

Or use the Windows launcher:

```text
RoseV/IDE/run_rosev_ide.bat
```

## PyInstaller Build

To build a standalone Windows executable:

```powershell
cd RoseV/IDE
.\build_pyqt_exe.ps1
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

- IntelliSense-style completions.
- Automatic DLL packaging.
- Project templates.
- Built-in loader wrapper generation.
- Automatic native DLL compilation.
- Debug log viewer tied to a selected game.
- One-click install into `RoseMod/MelonMods`.
