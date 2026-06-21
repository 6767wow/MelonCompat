use serde::{Deserialize, Serialize};
use std::{
    fs,
    io::{BufRead, BufReader},
    path::{Path, PathBuf},
    process::{Command, Stdio},
    thread,
};
use tauri::{Emitter, Window};

#[cfg(windows)]
use std::os::windows::process::CommandExt;

const CREATE_NO_WINDOW: u32 = 0x08000000;

#[derive(Clone, Debug, Serialize)]
#[serde(rename_all = "camelCase")]
struct OpenedFile {
    path: String,
    name: String,
    contents: String,
}

#[derive(Clone, Debug, Serialize)]
#[serde(rename_all = "camelCase")]
struct FileEntry {
    path: String,
    name: String,
    kind: String,
    depth: usize,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
struct SaveRequest {
    path: Option<String>,
    contents: String,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
struct CompileRequest {
    source_path: Option<String>,
    source: String,
    output_path: Option<String>,
}

#[derive(Clone, Debug, Serialize)]
#[serde(rename_all = "camelCase")]
struct CompileResult {
    ok: bool,
    source_path: String,
    output_path: String,
    output: String,
}

#[tauri::command]
fn open_file() -> Result<Option<OpenedFile>, String> {
    let Some(path) = rfd::FileDialog::new()
        .set_title("Open RoseV source")
        .add_filter("RoseV", &["rosev"])
        .add_filter("Code", &["rosev", "cs", "md", "txt"])
        .pick_file()
    else {
        return Ok(None);
    };

    read_opened_file(path).map(Some)
}

#[tauri::command]
fn open_folder() -> Result<Vec<FileEntry>, String> {
    let Some(path) = rfd::FileDialog::new()
        .set_title("Open RoseV workspace")
        .pick_folder()
    else {
        return Ok(Vec::new());
    };

    list_workspace(&path)
}

#[tauri::command]
fn read_file(path: String) -> Result<OpenedFile, String> {
    read_opened_file(PathBuf::from(path))
}

#[tauri::command]
fn save_file(request: SaveRequest) -> Result<OpenedFile, String> {
    let path = match request.path {
        Some(path) if !path.trim().is_empty() => PathBuf::from(path),
        _ => rfd::FileDialog::new()
            .set_title("Save RoseV source")
            .add_filter("RoseV", &["rosev"])
            .save_file()
            .ok_or_else(|| "Save cancelled.".to_string())?,
    };

    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent).map_err(|err| format!("Could not create {}: {err}", parent.display()))?;
    }

    fs::write(&path, request.contents).map_err(|err| format!("Could not save {}: {err}", path.display()))?;
    read_opened_file(path)
}

#[tauri::command]
fn sample_source() -> String {
    include_str!("../../src/samples/everything.rosev").to_string()
}

#[tauri::command]
fn compile_rosev(window: Window, request: CompileRequest) -> Result<CompileResult, String> {
    let compiler = find_rosev_compiler()?;
    let source_path = match request.source_path {
        Some(path) if !path.trim().is_empty() => PathBuf::from(path),
        _ => std::env::temp_dir().join("RoseVIDE").join("UnsavedMod.rosev"),
    };

    if let Some(parent) = source_path.parent() {
        fs::create_dir_all(parent).map_err(|err| format!("Could not create {}: {err}", parent.display()))?;
    }
    fs::write(&source_path, request.source).map_err(|err| format!("Could not write {}: {err}", source_path.display()))?;

    let output_path = match request.output_path {
        Some(path) if !path.trim().is_empty() => PathBuf::from(path),
        _ => source_path.with_extension("generated.cs"),
    };

    if let Some(parent) = output_path.parent() {
        fs::create_dir_all(parent).map_err(|err| format!("Could not create {}: {err}", parent.display()))?;
    }

    let _ = window.emit("rosev-output", format!("RoseV compiler: {}", compiler.display()));
    let _ = window.emit("rosev-output", format!("Compiling {} -> {}", source_path.display(), output_path.display()));

    let mut command = Command::new(&compiler);
    command
        .arg("compile")
        .arg(&source_path)
        .arg("-o")
        .arg(&output_path)
        .stdout(Stdio::piped())
        .stderr(Stdio::piped());

    #[cfg(windows)]
    command.creation_flags(CREATE_NO_WINDOW);

    let mut child = command.spawn().map_err(|err| format!("Could not start RoseV compiler: {err}"))?;
    let stdout = child.stdout.take();
    let stderr = child.stderr.take();

    let out_window = window.clone();
    let out_thread = thread::spawn(move || read_stream(stdout, out_window));
    let err_window = window.clone();
    let err_thread = thread::spawn(move || read_stream(stderr, err_window));

    let status = child.wait().map_err(|err| format!("Could not wait for RoseV compiler: {err}"))?;
    let mut output = String::new();
    output.push_str(&out_thread.join().unwrap_or_default());
    output.push_str(&err_thread.join().unwrap_or_default());

    Ok(CompileResult {
        ok: status.success(),
        source_path: path_to_string(source_path),
        output_path: path_to_string(output_path),
        output,
    })
}

fn read_opened_file(path: PathBuf) -> Result<OpenedFile, String> {
    let contents = fs::read_to_string(&path).map_err(|err| format!("Could not read {}: {err}", path.display()))?;
    let name = path.file_name().and_then(|name| name.to_str()).unwrap_or("Untitled").to_string();
    Ok(OpenedFile {
        path: path_to_string(path),
        name,
        contents,
    })
}

fn list_workspace(root: &Path) -> Result<Vec<FileEntry>, String> {
    let mut entries = Vec::new();
    visit_workspace(root, root, 0, &mut entries)?;
    entries.sort_by(|left, right| {
        left.path
            .to_lowercase()
            .cmp(&right.path.to_lowercase())
            .then_with(|| left.name.to_lowercase().cmp(&right.name.to_lowercase()))
    });
    Ok(entries)
}

fn visit_workspace(root: &Path, current: &Path, depth: usize, entries: &mut Vec<FileEntry>) -> Result<(), String> {
    if depth > 5 {
        return Ok(());
    }

    for entry in fs::read_dir(current).map_err(|err| format!("Could not read {}: {err}", current.display()))? {
        let entry = entry.map_err(|err| format!("Could not read workspace entry: {err}"))?;
        let path = entry.path();
        let name = entry.file_name().to_string_lossy().to_string();
        if should_skip(&name) {
            continue;
        }

        if path.is_dir() {
            entries.push(FileEntry {
                path: path_to_string(&path),
                name,
                kind: "folder".to_string(),
                depth,
            });
            visit_workspace(root, &path, depth + 1, entries)?;
        } else if is_supported_file(&path) {
            let relative = path.strip_prefix(root).unwrap_or(&path);
            entries.push(FileEntry {
                path: path_to_string(&path),
                name: relative.to_string_lossy().replace('\\', "/"),
                kind: "file".to_string(),
                depth,
            });
        }
    }

    Ok(())
}

fn should_skip(name: &str) -> bool {
    matches!(
        name.to_ascii_lowercase().as_str(),
        "bin" | "obj" | ".git" | "node_modules" | "target" | "dist"
    )
}

fn is_supported_file(path: &Path) -> bool {
    matches!(
        path.extension().and_then(|ext| ext.to_str()).unwrap_or("").to_ascii_lowercase().as_str(),
        "rosev" | "cs" | "csproj" | "md" | "txt"
    )
}

fn find_rosev_compiler() -> Result<PathBuf, String> {
    let mut starts = Vec::new();
    if let Ok(current) = std::env::current_dir() {
        starts.push(current);
    }
    if let Ok(exe) = std::env::current_exe() {
        if let Some(parent) = exe.parent() {
            starts.push(parent.to_path_buf());
        }
    }

    let candidates = [
        PathBuf::from("bin/RoseV/Release/RoseV.exe"),
        PathBuf::from("../../bin/RoseV/Release/RoseV.exe"),
        PathBuf::from("../../../bin/RoseV/Release/RoseV.exe"),
        PathBuf::from("RoseV/Native/bin/RoseV/Release/RoseV.exe"),
    ];

    for start in starts {
        for ancestor in start.ancestors() {
            for candidate in &candidates {
                let full = ancestor.join(candidate);
                if full.is_file() {
                    return Ok(full);
                }
            }
        }
    }

    Err("RoseV.exe was not found. Build RoseV\\Native\\RoseV.Native.vcxproj first.".to_string())
}

fn read_stream(stream: Option<impl std::io::Read + Send + 'static>, window: Window) -> String {
    let Some(stream) = stream else {
        return String::new();
    };

    let mut output = String::new();
    let reader = BufReader::new(stream);
    for line in reader.lines().map_while(Result::ok) {
        output.push_str(&line);
        output.push('\n');
        let _ = window.emit("rosev-output", line);
    }

    output
}

fn path_to_string(path: impl AsRef<Path>) -> String {
    path.as_ref().to_string_lossy().to_string()
}

fn main() {
    tauri::Builder::default()
        .invoke_handler(tauri::generate_handler![
            open_file,
            open_folder,
            read_file,
            save_file,
            sample_source,
            compile_rosev
        ])
        .run(tauri::generate_context!())
        .expect("error while running RoseV IDE");
}
