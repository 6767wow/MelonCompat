# Installer and Build Bug History

This page documents installer, packaging, build, GitHub, and release bugs from the project.

## 1. Installer Needed to Detect Existing Loaders

Initial risk:

- The installer could place files into the wrong game state.

Required detection:

- Unity game root.
- Game executable.
- Unity `_Data` folder.
- Mono or IL2CPP backend.
- x86 or x64 architecture.
- BepInEx install state.
- MelonLoader install state.
- RoseMod install state.

Main files:

- `Installer/UnityGameDetector`
- `Installer/BepInExInstall`
- `Installer/MelonLoaderInstall`
- `TauriInstaller/src-tauri/src/main.rs`

## 2. Normal Shim Must Block Missing BepInEx

Bug:

- The BepInEx-powered shim cannot work without BepInEx.

Expected behavior:

- If BepInEx is absent, ask to install BepInEx.
- If the user refuses, do not install the shim.
- If BepInEx is installed by the installer, run the game once before adding the shim.

Main code:

- `InstallerEngine.InstallAsync`
- Tauri `installSelected` request flow.

## 3. RoseMod Should Not Require BepInEx

Bug:

- Early RoseMod/UniWork behavior leaned too much on BepInEx state.

Expected behavior:

- RoseMod installs as a standalone loader.
- It may import existing BepInEx interop.
- It may optionally remove BepInEx after install.
- It must not need BepInEx to launch.

Main code:

- `InstallerEngine.DeployRoseModPayload`
- `Native/RoseMod.Native`
- `RoseMod/Core`

## 4. BepInEx Removal Access Denied

Observed error:

```text
Access to the path '...\BepInEx' is denied.
```

Cause:

- Steam games under `Program Files (x86)` often require elevated permission for moving folders.
- Files can also be locked if the game, Steam, or an editor still has handles open.

Correct installer behavior:

- Do not treat removal failure as proof that RoseMod failed to install.
- Tell the user the folder could not be moved.
- Leave the active RoseMod bootstrap in place.
- Let the user manually remove or move BepInEx later.

## 5. Program Compatibility Assistant Popup

Observed popup:

```text
This program might not have installed correctly
```

Cause:

- Windows detects installer-like behavior from a plain EXE.

Fix direction:

- Use an app manifest.
- Make the GUI show success/failure clearly.
- Avoid requiring users to run the CLI directly when the GUI is available.

## 6. Electron to Tauri Migration

Requirement change:

- Use Tauri instead of Electron.

Why:

- Smaller native shell.
- Better fit for a Windows installer-style GUI.
- Rust host can launch the C# backend without showing backend consoles.

Important files:

- `TauriInstaller/src-tauri/src/main.rs`
- `TauriInstaller/src/renderer.js`
- `TauriInstaller/src/index.html`
- `TauriInstaller/src/styles.css`

## 7. Backend Console Should Not Pop Up

Bug:

- Running backend commands from the GUI could show console windows.

Fix direction:

- Tauri backend process launch uses Windows `CREATE_NO_WINDOW`.
- Backend output is streamed into the GUI log area instead.

Main file:

- `TauriInstaller/src-tauri/src/main.rs`

## 8. Steam Icons and Game List

UI issue:

- The left-side game pictures should be Steam game icons, not repeated loader logos.

Fix direction:

- Scan Steam libraries.
- Read app metadata where available.
- Use Steam icon/image data for game entries.
- Use loader logo only as fallback.

Main file:

- `TauriInstaller/src-tauri/src/main.rs`

## 9. Logo Replacement and README Branding

Changes requested:

- Replace the old logo.
- Keep eyes white.
- Use the new transparent MelonCompat logo.
- Rename UniWork to RoseMod.
- Add RoseMod logo next to MelonCompat logo.

Current assets:

```text
docs/assets/meloncompat-logo.png
docs/assets/rosemod-logo.png
TauriInstaller/src/logo.png
TauriInstaller/src/rosemod-logo.png
```

## 10. Release ZIP Must Match Current Source

Bug class:

- The release ZIP can fall out of date after source changes.

Correct release process:

1. Build managed projects.
2. Build native RoseMod bootstrap.
3. Publish CLI backend.
4. Build Tauri frontend.
5. Recreate release folder.
6. Recreate ZIP.
7. Verify ZIP contents.
8. Push source and tag.

Important warning:

- A GitHub source upload replaces files only according to what you upload/commit. It does not magically delete old GitHub files unless the commit deletes them.

## 11. Source ZIP Stale UniWork Names

Problem:

- Older generated source ZIPs could still contain `UniWork.*` project names.

Fix direction:

- Regenerate the source package after the RoseMod rename.
- Do not trust older files under `dist/github-source` unless regenerated.

Status:

- The GitHub repo source is the authoritative source.

## 12. MSBuild Resolved Stale Facade DLLs

Observed errors:

```text
BepInEx.BaseUnityPlugin does not exist
PluginInfo.Type does not exist
```

Cause:

- MSBuild found older payload DLLs under generated `dist` folders before the fresh facade outputs.

Fix direction:

- Tighten references.
- Prefer explicit `HintPath`.
- Avoid letting generated release/source folders become reference candidates.

## 13. PluginInfo Instance Type Mismatch

Observed build error:

```text
Cannot implicitly convert type 'object' to 'BepInEx.BaseUnityPlugin'
```

Cause:

- Facade metadata type shape did not match code expectations.

Fix direction:

- Keep plugin instance properties strongly typed where the bridge expects them.
- Keep the public surface close to real BepInEx.

## 14. Unity Physics References Missing

Observed build issue:

- Unity physics types were missing while compiling the MelonLoader facade.

Cause:

- `RoseMod.MelonLoader.csproj` needs a real Unity reference set for compile-time Unity types.

Fix direction:

- Build with `UnityEngine.CoreModule.dll`.
- Include physics modules when facade code references them.
- Use generated IL2CPP references where needed.

## 15. Parallel Publish File Locks

Problem:

- Publishing/building multiple outputs in parallel can lock shared files.

Fix direction:

- Build sequentially when publishing final release artifacts.
- Clean only generated output folders, not user files.

## 16. GitHub Remote Moved

Observed during push:

```text
This repository moved. Please use the new location:
https://github.com/6767wow/MelonCompat-and-RoseMod.git
```

Fix:

- Update local `origin` to the moved repository URL.

## 17. GitHub Wiki Could Not Be Pushed While Empty

Observed error:

```text
Repository not found
```

Cause:

- GitHub does not create the backing `.wiki.git` repository until the first page is made in the web UI.

Fix:

1. Create the first wiki page in the browser.
2. Fetch the new wiki repo.
3. Merge local wiki content.
4. Push all pages.

Status:

- The real GitHub Wiki is now populated.

## 18. GitHub CLI Missing

Observed issue:

- `gh` was not installed.

Impact:

- Release asset upload through GitHub CLI could not be completed locally.

Workaround:

- Use normal `git` for source/wiki pushes.
- Upload release ZIP manually through GitHub web UI or install `gh`.

## 19. Main Repo Docs Mirror

Reason:

- The wiki was empty at first.
- Docs still needed to be visible on GitHub immediately.

Fix:

- Add `docs/wiki` to the main repo as a mirror of the wiki pages.

Current rule:

- The GitHub Wiki is the user-facing long-form docs.
- `docs/wiki` is a source-controlled mirror.
