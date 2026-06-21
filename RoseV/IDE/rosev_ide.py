#!/usr/bin/env python3
from __future__ import annotations

import os
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Callable

try:
    from PyQt6.QtCore import QObject, QRect, QSize, QThread, QTimer, Qt, pyqtSignal
    from PyQt6.QtGui import (
        QAction,
        QColor,
        QFont,
        QIcon,
        QKeySequence,
        QPainter,
        QPixmap,
        QSyntaxHighlighter,
        QTextCharFormat,
        QTextCursor,
        QTextDocument,
        QTextFormat,
    )
    from PyQt6.QtWidgets import (
        QApplication,
        QDialog,
        QFileDialog,
        QFrame,
        QGridLayout,
        QHBoxLayout,
        QLabel,
        QLineEdit,
        QListWidget,
        QListWidgetItem,
        QMainWindow,
        QMessageBox,
        QPushButton,
        QPlainTextEdit,
        QSizePolicy,
        QSplitter,
        QStatusBar,
        QTabWidget,
        QTextBrowser,
        QTextEdit,
        QTreeWidget,
        QTreeWidgetItem,
        QVBoxLayout,
        QWidget,
    )
except ModuleNotFoundError as exc:
    raise SystemExit(
        "RoseV IDE now uses PyQt6. Install it with:\n"
        "  python -m pip install -r RoseV\\IDE\\requirements.txt"
    ) from exc


APP_NAME = "RoseV IDE"
SUPPORTED_FILE_EXTENSIONS = {".rosev", ".cs", ".csproj", ".md", ".txt"}
SKIP_DIRS = {"bin", "obj", ".git", "node_modules", "target", "dist", "__pycache__"}


SAMPLE_SOURCE = '''rosev "My RoseV Mod" id "com.example.myrosevmod" version "1.0.0" author "Me"
namespace MyMods
class MyRoseVMod

import csharp
import unity.core
import melonloader.core
import bepinex.core
import rosemod

use unity
use melonloader
use bepinex
use rosemod

when load {
  say "{mod} loaded on {loader}"
}
'''


APP_STYLE = """
QMainWindow, QWidget {
  background: #0f1318;
  color: #e9edf2;
  font-family: Segoe UI;
  font-size: 10pt;
}
QFrame#TopBar {
  background: #151b22;
  border-bottom: 1px solid #2a323d;
}
QFrame#Sidebar, QTabWidget::pane {
  background: #121820;
  border: 1px solid #232b35;
}
QFrame#ProfileCard {
  background: #1a222c;
  border: 1px solid #2d3946;
  border-radius: 8px;
}
QLabel#BrandTitle {
  color: #ffffff;
  font-size: 13pt;
  font-weight: 700;
}
QLabel#Muted, QLabel#FilePath, QLabel#CountLabel {
  color: #93a0ae;
}
QPushButton {
  min-height: 26px;
  padding: 4px 10px;
  background: #202936;
  color: #f2f5f7;
  border: 1px solid #344252;
  border-radius: 6px;
}
QPushButton:hover {
  background: #293545;
  border-color: #466074;
}
QPushButton:pressed {
  background: #1b232e;
}
QPushButton#PrimaryButton {
  background: #52a63f;
  color: #071007;
  border-color: #79c767;
  font-weight: 700;
}
QPushButton#PrimaryButton:hover {
  background: #65bb50;
}
QLineEdit {
  min-height: 26px;
  padding: 3px 8px;
  background: #0d1117;
  color: #eef2f5;
  border: 1px solid #303a47;
  border-radius: 6px;
  selection-background-color: #7a3fb4;
}
QPlainTextEdit, QTextBrowser {
  background: #0d1117;
  color: #e9edf2;
  border: 1px solid #232b35;
  border-radius: 6px;
  selection-background-color: #6f3ba6;
  selection-color: #ffffff;
}
QTreeWidget, QListWidget {
  background: #10161d;
  color: #d9e1e8;
  border: 1px solid #232b35;
  border-radius: 6px;
  alternate-background-color: #151c24;
}
QTreeWidget::item, QListWidget::item {
  min-height: 24px;
  padding: 3px 6px;
}
QTreeWidget::item:selected, QListWidget::item:selected {
  background: #28405a;
  color: #ffffff;
}
QTabBar::tab {
  min-height: 28px;
  padding: 5px 10px;
  background: #151b22;
  color: #aab5c2;
  border: 1px solid #232b35;
  border-bottom: 0;
  border-top-left-radius: 6px;
  border-top-right-radius: 6px;
}
QTabBar::tab:selected {
  background: #202936;
  color: #ffffff;
}
QSplitter::handle {
  background: #232b35;
}
QStatusBar {
  background: #111720;
  color: #aab5c2;
  border-top: 1px solid #232b35;
}
QScrollBar:vertical, QScrollBar:horizontal {
  background: #0f1318;
  border: 0;
  margin: 0;
}
QScrollBar:vertical {
  width: 13px;
}
QScrollBar:horizontal {
  height: 13px;
}
QScrollBar::handle {
  background: #344252;
  border-radius: 6px;
}
QScrollBar::handle:hover {
  background: #48586b;
}
QScrollBar::add-line, QScrollBar::sub-line {
  width: 0;
  height: 0;
}
"""


@dataclass
class Diagnostic:
    level: str
    line: int
    message: str


@dataclass
class CommandEntry:
    name: str
    detail: str
    run: Callable[[], None]


class LineNumberArea(QWidget):
    def __init__(self, editor: "CodeEditor") -> None:
        super().__init__(editor)
        self.editor = editor

    def sizeHint(self) -> QSize:
        return QSize(self.editor.line_number_area_width(), 0)

    def paintEvent(self, event) -> None:
        self.editor.line_number_area_paint_event(event)


class CodeEditor(QPlainTextEdit):
    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.line_number_area = LineNumberArea(self)
        self.setLineWrapMode(QPlainTextEdit.LineWrapMode.NoWrap)
        self.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        self.setTabStopDistance(self.fontMetrics().horizontalAdvance(" ") * 2)

        font = QFont("Consolas", 10)
        font.setFixedPitch(True)
        self.setFont(font)

        self.blockCountChanged.connect(self.update_line_number_area_width)
        self.updateRequest.connect(self.update_line_number_area)
        self.cursorPositionChanged.connect(self.highlight_current_line)
        self.update_line_number_area_width(0)
        self.highlight_current_line()

    def line_number_area_width(self) -> int:
        digits = len(str(max(1, self.blockCount())))
        return 14 + self.fontMetrics().horizontalAdvance("9") * digits

    def update_line_number_area_width(self, _block_count: int) -> None:
        self.setViewportMargins(self.line_number_area_width(), 0, 0, 0)

    def update_line_number_area(self, rect: QRect, dy: int) -> None:
        if dy:
            self.line_number_area.scroll(0, dy)
        else:
            self.line_number_area.update(0, rect.y(), self.line_number_area.width(), rect.height())
        if rect.contains(self.viewport().rect()):
            self.update_line_number_area_width(0)

    def resizeEvent(self, event) -> None:
        super().resizeEvent(event)
        rect = self.contentsRect()
        self.line_number_area.setGeometry(QRect(rect.left(), rect.top(), self.line_number_area_width(), rect.height()))

    def line_number_area_paint_event(self, event) -> None:
        painter = QPainter(self.line_number_area)
        painter.fillRect(event.rect(), QColor("#121820"))
        painter.setPen(QColor("#647385"))

        block = self.firstVisibleBlock()
        block_number = block.blockNumber()
        top = int(self.blockBoundingGeometry(block).translated(self.contentOffset()).top())
        bottom = top + int(self.blockBoundingRect(block).height())

        while block.isValid() and top <= event.rect().bottom():
            if block.isVisible() and bottom >= event.rect().top():
                number = str(block_number + 1)
                painter.drawText(
                    0,
                    top,
                    self.line_number_area.width() - 6,
                    self.fontMetrics().height(),
                    int(Qt.AlignmentFlag.AlignRight),
                    number,
                )

            block = block.next()
            top = bottom
            bottom = top + int(self.blockBoundingRect(block).height())
            block_number += 1

    def highlight_current_line(self) -> None:
        selection = QTextEdit.ExtraSelection()
        selection.format.setBackground(QColor("#151d27"))
        selection.format.setProperty(QTextFormat.Property.FullWidthSelection, True)
        selection.cursor = self.textCursor()
        selection.cursor.clearSelection()
        self.setExtraSelections([selection])

    def keyPressEvent(self, event) -> None:
        if event.key() == Qt.Key.Key_Tab:
            self.indent_selection(1)
            return
        if event.key() == Qt.Key.Key_Backtab:
            self.indent_selection(-1)
            return
        if event.key() in (Qt.Key.Key_Return, Qt.Key.Key_Enter):
            self.insert_smart_newline()
            return
        super().keyPressEvent(event)

    def indent_selection(self, direction: int) -> None:
        cursor = self.textCursor()
        text = self.toPlainText()
        start = cursor.selectionStart()
        end = cursor.selectionEnd()

        if direction > 0 and start == end:
            cursor.insertText("  ")
            return

        line_start = text.rfind("\n", 0, start) + 1
        if start == end:
            line_end = text.find("\n", line_start)
            if line_end == -1:
                line_end = len(text)
            line = text[line_start:line_end]
            remove_count = 2 if line.startswith("  ") else 1 if line.startswith((" ", "\t")) else 0
            if remove_count:
                cursor.setPosition(line_start)
                cursor.setPosition(line_start + remove_count, QTextCursor.MoveMode.KeepAnchor)
                cursor.removeSelectedText()
            return

        selected_end = end - 1 if end > start and text[end - 1] == "\n" else end
        line_end = text.find("\n", selected_end)
        if line_end == -1:
            line_end = len(text)

        block = text[line_start:line_end]
        lines = block.split("\n")
        replacement_lines = []
        for line in lines:
            if direction > 0:
                replacement_lines.append(f"  {line}")
            elif line.startswith("  "):
                replacement_lines.append(line[2:])
            elif line.startswith((" ", "\t")):
                replacement_lines.append(line[1:])
            else:
                replacement_lines.append(line)
        replacement = "\n".join(replacement_lines)

        cursor.beginEditBlock()
        cursor.setPosition(line_start)
        cursor.setPosition(line_end, QTextCursor.MoveMode.KeepAnchor)
        cursor.insertText(replacement)
        cursor.setPosition(line_start)
        cursor.setPosition(line_start + len(replacement), QTextCursor.MoveMode.KeepAnchor)
        self.setTextCursor(cursor)
        cursor.endEditBlock()

    def insert_smart_newline(self) -> None:
        cursor = self.textCursor()
        text = self.toPlainText()
        start = cursor.selectionStart()
        line_start = text.rfind("\n", 0, start) + 1
        before_cursor = text[line_start:start]
        indent = re.match(r"\s*", before_cursor).group(0)
        extra = "  " if before_cursor.strip().endswith("{") else ""
        cursor.insertText(f"\n{indent}{extra}")

    def goto_line(self, line: int) -> None:
        block = self.document().findBlockByNumber(max(0, line - 1))
        if not block.isValid():
            return
        cursor = QTextCursor(block)
        self.setTextCursor(cursor)
        self.centerCursor()
        self.setFocus()


class RoseVSyntaxHighlighter(QSyntaxHighlighter):
    def __init__(self, document: QTextDocument) -> None:
        super().__init__(document)
        self.keyword_format = self.make_format("#8fd07d", bold=True)
        self.block_format = self.make_format("#d38cff", bold=True)
        self.string_format = self.make_format("#f2ca72")
        self.comment_format = self.make_format("#6d7886", italic=True)
        self.type_format = self.make_format("#7ab7ff")
        self.number_format = self.make_format("#f59a8d")

        keywords = "rosev namespace class import use setting field native synvert members csharp cs unity"
        blocks = "when every key make if repeat while try"
        actions = "say warn error emit let set add sub mul div call return throw atleast is"
        self.keyword_pattern = re.compile(rf"\b({'|'.join(keywords.split())})\b")
        self.block_pattern = re.compile(rf"\b({'|'.join(blocks.split())})\b")
        self.action_pattern = re.compile(rf"\b({'|'.join(actions.split())})\b")
        self.string_pattern = re.compile(r'"(?:\\.|[^"\\])*"')
        self.comment_pattern = re.compile(r"#.*$")
        self.number_pattern = re.compile(r"\b\d+(?:\.\d+)?\b")

    @staticmethod
    def make_format(color: str, bold: bool = False, italic: bool = False) -> QTextCharFormat:
        fmt = QTextCharFormat()
        fmt.setForeground(QColor(color))
        if bold:
            fmt.setFontWeight(QFont.Weight.Bold)
        if italic:
            fmt.setFontItalic(True)
        return fmt

    def highlightBlock(self, text: str) -> None:
        for pattern, fmt in (
            (self.keyword_pattern, self.keyword_format),
            (self.block_pattern, self.block_format),
            (self.action_pattern, self.type_format),
            (self.number_pattern, self.number_format),
            (self.string_pattern, self.string_format),
            (self.comment_pattern, self.comment_format),
        ):
            for match in pattern.finditer(text):
                self.setFormat(match.start(), match.end() - match.start(), fmt)


class CompileWorker(QObject):
    line = pyqtSignal(str)
    finished = pyqtSignal(bool, str, str, str)

    def __init__(self, compiler: Path, source_path: Path, source: str, output_path: Path) -> None:
        super().__init__()
        self.compiler = compiler
        self.source_path = source_path
        self.source = source
        self.output_path = output_path

    def run(self) -> None:
        output_lines: list[str] = []
        try:
            self.source_path.parent.mkdir(parents=True, exist_ok=True)
            self.output_path.parent.mkdir(parents=True, exist_ok=True)
            self.source_path.write_text(self.source, encoding="utf-8")

            self.line.emit(f"RoseV compiler: {self.compiler}")
            self.line.emit(f"Compiling {self.source_path} -> {self.output_path}")
            command = [str(self.compiler), "compile", str(self.source_path), "-o", str(self.output_path)]

            creationflags = 0
            startupinfo = None
            if os.name == "nt":
                creationflags = getattr(subprocess, "CREATE_NO_WINDOW", 0)
                startupinfo = subprocess.STARTUPINFO()
                startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW

            process = subprocess.Popen(
                command,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                encoding="utf-8",
                errors="replace",
                creationflags=creationflags,
                startupinfo=startupinfo,
            )

            assert process.stdout is not None
            for raw_line in process.stdout:
                line = raw_line.rstrip()
                output_lines.append(line)
                self.line.emit(line)

            ok = process.wait() == 0
            self.finished.emit(ok, str(self.source_path), str(self.output_path), "\n".join(output_lines))
        except Exception as exc:
            output_lines.append(str(exc))
            self.line.emit(f"Compile failed: {exc}")
            self.finished.emit(False, str(self.source_path), str(self.output_path), "\n".join(output_lines))


class CommandPalette(QDialog):
    def __init__(self, parent: QWidget, commands: list[CommandEntry]) -> None:
        super().__init__(parent)
        self.setWindowTitle("Commands")
        self.commands = commands
        self.visible_commands: list[CommandEntry] = []
        self.setMinimumSize(540, 420)

        layout = QVBoxLayout(self)
        self.search = QLineEdit()
        self.search.setPlaceholderText("Type a command...")
        self.list_widget = QListWidget()
        layout.addWidget(self.search)
        layout.addWidget(self.list_widget)

        self.search.textChanged.connect(self.refresh)
        self.search.returnPressed.connect(self.run_selected)
        self.list_widget.itemActivated.connect(lambda _item: self.run_selected())
        self.refresh("")

    def refresh(self, query: str) -> None:
        query = query.strip().lower()
        self.list_widget.clear()
        self.visible_commands = [
            command
            for command in self.commands
            if query in f"{command.name} {command.detail}".lower()
        ]
        for index, command in enumerate(self.visible_commands):
            item = QListWidgetItem(f"{command.name}\n{command.detail}")
            item.setData(Qt.ItemDataRole.UserRole, index)
            self.list_widget.addItem(item)
        if self.visible_commands:
            self.list_widget.setCurrentRow(0)

    def run_selected(self) -> None:
        item = self.list_widget.currentItem()
        if not item:
            return
        index = item.data(Qt.ItemDataRole.UserRole)
        if index is None:
            return
        command = self.visible_commands[int(index)]
        self.accept()
        command.run()


class RoseVIDE(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.root = Path(__file__).resolve().parent
        self.logo_png = self.root / "assets" / "rosemod-logo.png"
        self.logo_ico = self.root / "assets" / "rosemod-logo.ico"
        self.sample_path = self.root / "samples" / "everything.rosev"

        self.current_path: Path | None = None
        self.current_name = "Untitled.rosev"
        self.dirty = False
        self.workspace_root: Path | None = None
        self.compiling = False
        self.thread: QThread | None = None
        self.worker: CompileWorker | None = None
        self.loading_source = False

        self.setWindowTitle(APP_NAME)
        self.resize(1240, 780)
        self.setMinimumSize(960, 620)
        self.setWindowIcon(QIcon(str(self.logo_ico if self.logo_ico.exists() else self.logo_png)))

        self.commands: list[CommandEntry] = []
        self.build_ui()
        self.install_actions()
        self.set_source(self.read_sample_source(), "EverythingSample.rosev", None, dirty=False)
        self.set_status("Ready")

    def build_ui(self) -> None:
        central = QWidget()
        layout = QVBoxLayout(central)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        layout.addWidget(self.build_topbar())

        body = QSplitter(Qt.Orientation.Horizontal)
        body.addWidget(self.build_sidebar())
        body.addWidget(self.build_editor_area())
        body.setSizes([330, 900])
        body.setChildrenCollapsible(False)
        layout.addWidget(body, 1)
        self.setCentralWidget(central)

        status = QStatusBar()
        self.status_label = QLabel("Ready")
        self.cursor_label = QLabel("Ln 1, Col 1")
        self.diagnostic_label = QLabel("No issues")
        self.mode_label = QLabel("PyQt native")
        status.addWidget(self.status_label, 1)
        status.addPermanentWidget(self.cursor_label)
        status.addPermanentWidget(self.diagnostic_label)
        status.addPermanentWidget(self.mode_label)
        self.setStatusBar(status)

        self.diagnostic_timer = QTimer(self)
        self.diagnostic_timer.setSingleShot(True)
        self.diagnostic_timer.timeout.connect(self.refresh_code_views)

    def build_topbar(self) -> QWidget:
        topbar = QFrame()
        topbar.setObjectName("TopBar")
        layout = QHBoxLayout(topbar)
        layout.setContentsMargins(12, 8, 12, 8)
        layout.setSpacing(10)

        logo = QLabel()
        logo.setPixmap(self.logo_pixmap(44))
        logo.setFixedSize(48, 48)
        logo.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(logo)

        brand = QVBoxLayout()
        title = QLabel(APP_NAME)
        title.setObjectName("BrandTitle")
        self.path_label = QLabel("Untitled.rosev")
        self.path_label.setObjectName("FilePath")
        brand.addWidget(title)
        brand.addWidget(self.path_label)
        layout.addLayout(brand, 1)

        self.find_box = QLineEdit()
        self.find_box.setPlaceholderText("Find in file")
        self.find_box.setFixedWidth(190)
        self.find_box.returnPressed.connect(self.find_next)
        layout.addWidget(self.find_box)
        layout.addWidget(self.make_button("Prev", self.find_previous))
        layout.addWidget(self.make_button("Next", self.find_next))
        layout.addWidget(self.make_button("Format", self.format_document))
        layout.addWidget(self.make_button("Save", self.save_file))
        layout.addWidget(self.make_button("Compile", self.compile_file, primary=True))
        layout.addWidget(self.make_button("Commands", self.show_command_palette))
        return topbar

    def build_sidebar(self) -> QWidget:
        sidebar = QFrame()
        sidebar.setObjectName("Sidebar")
        sidebar.setMinimumWidth(300)
        sidebar.setMaximumWidth(430)
        layout = QVBoxLayout(sidebar)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(10)

        profile = QFrame()
        profile.setObjectName("ProfileCard")
        profile_layout = QHBoxLayout(profile)
        profile_layout.setContentsMargins(10, 10, 10, 10)
        profile_layout.setSpacing(10)
        avatar = QLabel()
        avatar.setPixmap(self.logo_pixmap(54))
        avatar.setFixedSize(58, 58)
        avatar.setAlignment(Qt.AlignmentFlag.AlignCenter)
        profile_layout.addWidget(avatar)
        copy = QVBoxLayout()
        name = QLabel("RoseMod")
        name.setObjectName("BrandTitle")
        detail = QLabel("RoseV workspace")
        detail.setObjectName("Muted")
        copy.addWidget(name)
        copy.addWidget(detail)
        profile_layout.addLayout(copy, 1)
        layout.addWidget(profile)

        self.sidebar_tabs = QTabWidget()
        self.sidebar_tabs.addTab(self.build_explorer_tab(), "Files")
        self.sidebar_tabs.addTab(self.build_code_tab(), "Code")
        self.sidebar_tabs.addTab(self.build_help_tab(), "Help")
        layout.addWidget(self.sidebar_tabs, 1)
        return sidebar

    def build_explorer_tab(self) -> QWidget:
        tab = QWidget()
        layout = QVBoxLayout(tab)
        layout.setContentsMargins(8, 8, 8, 8)
        buttons = QGridLayout()
        buttons.addWidget(self.make_button("New", self.new_file), 0, 0)
        buttons.addWidget(self.make_button("Open", self.open_file), 0, 1)
        buttons.addWidget(self.make_button("Folder", self.open_folder), 1, 0)
        buttons.addWidget(self.make_button("Sample", self.load_sample), 1, 1)
        layout.addLayout(buttons)

        self.workspace_count = QLabel("0 files")
        self.workspace_count.setObjectName("CountLabel")
        layout.addWidget(self.workspace_count)
        self.workspace_tree = QTreeWidget()
        self.workspace_tree.setHeaderHidden(True)
        self.workspace_tree.itemActivated.connect(self.open_workspace_item)
        layout.addWidget(self.workspace_tree, 1)
        return tab

    def build_code_tab(self) -> QWidget:
        tab = QWidget()
        layout = QVBoxLayout(tab)
        layout.setContentsMargins(8, 8, 8, 8)
        self.outline_count = QLabel("0 symbols")
        self.outline_count.setObjectName("CountLabel")
        layout.addWidget(self.outline_count)
        self.outline_list = QListWidget()
        self.outline_list.itemActivated.connect(self.goto_outline_item)
        layout.addWidget(self.outline_list, 1)

        layout.addWidget(QLabel("Snippets"))
        grid = QGridLayout()
        snippets = [
            ("when load", "load"),
            ("when update", "update"),
            ("function", "function"),
            ("import", "import"),
            ("setting", "setting"),
            ("field", "field"),
            ("if", "if"),
            ("key press", "key"),
            ("unity escape", "unity"),
            ("C# block", "csharp"),
            ("C# members", "members"),
            ("Unity C#", "unitySynvert"),
            ("native", "native"),
        ]
        for index, (label, kind) in enumerate(snippets):
            button = self.make_button(label, lambda checked=False, snippet_kind=kind: self.insert_snippet(snippet_kind))
            grid.addWidget(button, index // 2, index % 2)
        layout.addLayout(grid)
        return tab

    def build_help_tab(self) -> QWidget:
        help_widget = QTextBrowser()
        help_widget.setOpenExternalLinks(False)
        help_widget.setHtml(
            """
            <h2>RoseV</h2>
            <p>Write when code runs, then what should happen.</p>
            <pre>when load {
  say "Loaded"
}</pre>
            <p><b>Editor</b>: scrolls vertically and horizontally, shows line numbers, colors RoseV syntax, auto-indents, and supports find.</p>
            <p><b>Shortcuts</b>: Ctrl+S save, Ctrl+O open, Ctrl+Enter compile, Ctrl+Shift+F format, Ctrl+Shift+P commands, Ctrl+F find, Alt+Z word wrap.</p>
            <p><b>Panels</b>: files, outline, snippets, diagnostics, compiler output, and generated C#.</p>
            """
        )
        return help_widget

    def build_editor_area(self) -> QWidget:
        splitter = QSplitter(Qt.Orientation.Vertical)
        self.editor = CodeEditor()
        self.editor.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.highlighter = RoseVSyntaxHighlighter(self.editor.document())
        self.editor.textChanged.connect(self.on_editor_text_changed)
        self.editor.cursorPositionChanged.connect(self.update_cursor_status)
        splitter.addWidget(self.editor)

        self.bottom_tabs = QTabWidget()
        self.diagnostics_list = QListWidget()
        self.diagnostics_list.itemActivated.connect(self.goto_diagnostic_item)
        self.output_box = QPlainTextEdit()
        self.output_box.setReadOnly(True)
        self.generated_box = QPlainTextEdit()
        self.generated_box.setReadOnly(True)
        self.generated_box.setLineWrapMode(QPlainTextEdit.LineWrapMode.NoWrap)
        self.bottom_tabs.addTab(self.diagnostics_list, "Diagnostics")
        self.bottom_tabs.addTab(self.output_box, "Output")
        self.bottom_tabs.addTab(self.generated_box, "Generated")
        splitter.addWidget(self.bottom_tabs)
        splitter.setSizes([560, 200])
        return splitter

    def install_actions(self) -> None:
        file_menu = self.menuBar().addMenu("File")
        edit_menu = self.menuBar().addMenu("Edit")
        view_menu = self.menuBar().addMenu("View")
        build_menu = self.menuBar().addMenu("Build")

        self.add_action(file_menu, "New", "Ctrl+N", self.new_file)
        self.add_action(file_menu, "Open", "Ctrl+O", self.open_file)
        self.add_action(file_menu, "Open Folder", "Ctrl+K", self.open_folder)
        self.add_action(file_menu, "Save", "Ctrl+S", self.save_file)
        self.add_action(file_menu, "Save As", "Ctrl+Shift+S", lambda: self.save_file(save_as=True))
        self.add_action(edit_menu, "Find", "Ctrl+F", self.focus_find)
        self.add_action(edit_menu, "Format Document", "Ctrl+Shift+F", self.format_document)
        self.add_action(edit_menu, "Command Palette", "Ctrl+Shift+P", self.show_command_palette)
        self.add_action(view_menu, "Toggle Word Wrap", "Alt+Z", self.toggle_word_wrap)
        self.add_action(view_menu, "Zoom In", "Ctrl+=", lambda: self.zoom_editor(1))
        self.add_action(view_menu, "Zoom Out", "Ctrl+-", lambda: self.zoom_editor(-1))
        self.add_action(build_menu, "Compile RoseV", "Ctrl+Return", self.compile_file)

        self.commands = [
            CommandEntry("New RoseV File", "Create an empty RoseV source", self.new_file),
            CommandEntry("Open File", "Open a .rosev file", self.open_file),
            CommandEntry("Open Folder", "Open a workspace folder", self.open_folder),
            CommandEntry("Save File", "Save the current file", self.save_file),
            CommandEntry("Compile RoseV", "Generate C# from the current RoseV file", self.compile_file),
            CommandEntry("Format Document", "Normalize indentation and trim trailing spaces", self.format_document),
            CommandEntry("Toggle Word Wrap", "Switch between horizontal scrolling and wrapped lines", self.toggle_word_wrap),
            CommandEntry("Load Everything Sample", "Replace editor contents with a full RoseV sample", self.load_sample),
            CommandEntry("Insert when load", "Add a load lifecycle block", lambda: self.insert_snippet("load")),
            CommandEntry("Insert when update", "Add an update lifecycle block", lambda: self.insert_snippet("update")),
            CommandEntry("Insert function", "Add a reusable make block", lambda: self.insert_snippet("function")),
            CommandEntry("Insert import", "Import common RoseV packs", lambda: self.insert_snippet("import")),
            CommandEntry("Insert setting", "Add a simple config setting", lambda: self.insert_snippet("setting")),
            CommandEntry("Insert C# block", "Embed full C# in an event or function", lambda: self.insert_snippet("csharp")),
            CommandEntry("Insert C# members", "Add full C# class members", lambda: self.insert_snippet("members")),
            CommandEntry("Insert native companion", "Declare a native DLL bridge", lambda: self.insert_snippet("native")),
        ]

    def add_action(self, menu, label: str, shortcut: str, callback: Callable[[], None]) -> QAction:
        action = QAction(label, self)
        if shortcut:
            action.setShortcut(QKeySequence(shortcut))
        action.triggered.connect(callback)
        menu.addAction(action)
        return action

    def make_button(self, text: str, callback: Callable[[], None], primary: bool = False) -> QPushButton:
        button = QPushButton(text)
        if primary:
            button.setObjectName("PrimaryButton")
        button.clicked.connect(callback)
        return button

    def logo_pixmap(self, size: int) -> QPixmap:
        pixmap = QPixmap(str(self.logo_png))
        if pixmap.isNull():
            fallback = QPixmap(size, size)
            fallback.fill(QColor("#52a63f"))
            return fallback
        return pixmap.scaled(
            size,
            size,
            Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation,
        )

    def read_sample_source(self) -> str:
        if self.sample_path.exists():
            return self.sample_path.read_text(encoding="utf-8")
        return SAMPLE_SOURCE

    def set_source(self, source: str, name: str, path: Path | None, dirty: bool) -> None:
        self.loading_source = True
        self.current_path = path
        self.current_name = name
        self.dirty = dirty
        self.editor.setPlainText(source)
        self.loading_source = False
        self.refresh_code_views()
        self.update_cursor_status()
        self.update_title()

    def on_editor_text_changed(self) -> None:
        if self.loading_source:
            return
        self.dirty = True
        self.update_title()
        self.diagnostic_timer.start(120)

    def update_title(self) -> None:
        marker = " *" if self.dirty else ""
        self.setWindowTitle(f"{self.current_name}{marker} - {APP_NAME}")
        self.path_label.setText(str(self.current_path) if self.current_path else "Unsaved file")

    def set_status(self, message: str) -> None:
        self.status_label.setText(message)

    def update_cursor_status(self) -> None:
        cursor = self.editor.textCursor()
        self.cursor_label.setText(f"Ln {cursor.blockNumber() + 1}, Col {cursor.positionInBlock() + 1}")

    def refresh_code_views(self) -> None:
        self.refresh_outline()
        self.refresh_diagnostics()

    def new_file(self) -> None:
        if not self.confirm_dirty():
            return
        text = '''rosev "New RoseV Mod" id "com.example.newmod" version "1.0.0" author "Me"
namespace MyMods
class NewRoseVMod

import csharp
import unity.core
import melonloader.core
import bepinex.core
import rosemod

use unity
use melonloader
use bepinex
use rosemod

when load {
  say "{mod} loaded"
}
'''
        self.set_source(text, "Untitled.rosev", None, dirty=False)
        self.set_status("New file ready")

    def open_file(self) -> None:
        if not self.confirm_dirty():
            return
        filename, _selected = QFileDialog.getOpenFileName(
            self,
            "Open RoseV source",
            str(self.workspace_root or self.root),
            "RoseV and Code (*.rosev *.cs *.md *.txt);;All Files (*.*)",
        )
        if not filename:
            return
        path = Path(filename)
        try:
            self.set_source(path.read_text(encoding="utf-8"), path.name, path, dirty=False)
            self.set_status(f"Opened {path.name}")
        except OSError as exc:
            QMessageBox.critical(self, APP_NAME, f"Could not open {path}:\n{exc}")

    def open_folder(self) -> None:
        folder = QFileDialog.getExistingDirectory(self, "Open RoseV workspace", str(self.workspace_root or self.root))
        if not folder:
            return
        self.workspace_root = Path(folder)
        self.populate_workspace()

    def populate_workspace(self) -> None:
        self.workspace_tree.clear()
        if not self.workspace_root:
            self.workspace_count.setText("0 files")
            return

        root_item = QTreeWidgetItem([self.workspace_root.name])
        root_item.setData(0, Qt.ItemDataRole.UserRole, str(self.workspace_root))
        self.workspace_tree.addTopLevelItem(root_item)

        folder_items: dict[Path, QTreeWidgetItem] = {self.workspace_root: root_item}
        file_count = 0
        for current, dirs, files in os.walk(self.workspace_root):
            current_path = Path(current)
            depth = len(current_path.relative_to(self.workspace_root).parts)
            if depth > 5:
                dirs[:] = []
                continue
            dirs[:] = sorted(d for d in dirs if d.lower() not in SKIP_DIRS)
            parent_item = folder_items.get(current_path, root_item)

            for dirname in dirs:
                folder_path = current_path / dirname
                item = QTreeWidgetItem([dirname])
                item.setData(0, Qt.ItemDataRole.UserRole, str(folder_path))
                parent_item.addChild(item)
                folder_items[folder_path] = item

            for filename in sorted(files):
                path = current_path / filename
                if path.suffix.lower() not in SUPPORTED_FILE_EXTENSIONS:
                    continue
                item = QTreeWidgetItem([filename])
                item.setData(0, Qt.ItemDataRole.UserRole, str(path))
                parent_item.addChild(item)
                file_count += 1

        root_item.setExpanded(True)
        self.workspace_count.setText(f"{file_count} {'file' if file_count == 1 else 'files'}")
        self.set_status(f"Opened workspace {self.workspace_root}")

    def open_workspace_item(self, item: QTreeWidgetItem) -> None:
        value = item.data(0, Qt.ItemDataRole.UserRole)
        if not value:
            return
        path = Path(str(value))
        if path.is_file() and path.suffix.lower() in SUPPORTED_FILE_EXTENSIONS:
            if not self.confirm_dirty():
                return
            try:
                self.set_source(path.read_text(encoding="utf-8"), path.name, path, dirty=False)
                self.set_status(f"Opened {path.name}")
            except OSError as exc:
                QMessageBox.critical(self, APP_NAME, f"Could not open {path}:\n{exc}")
        elif path.is_dir():
            item.setExpanded(not item.isExpanded())

    def save_file(self, save_as: bool = False) -> bool:
        path = self.current_path
        if save_as or path is None:
            filename, _selected = QFileDialog.getSaveFileName(
                self,
                "Save RoseV source",
                str((self.workspace_root or self.root) / self.current_name),
                "RoseV (*.rosev);;All Files (*.*)",
            )
            if not filename:
                return False
            path = Path(filename)
            if not path.suffix:
                path = path.with_suffix(".rosev")

        try:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(self.editor.toPlainText(), encoding="utf-8")
            self.current_path = path
            self.current_name = path.name
            self.dirty = False
            self.update_title()
            self.set_status(f"Saved {path.name}")
            return True
        except OSError as exc:
            QMessageBox.critical(self, APP_NAME, f"Could not save {path}:\n{exc}")
            self.set_status("Save failed")
            return False

    def load_sample(self) -> None:
        if not self.confirm_dirty():
            return
        self.set_source(self.read_sample_source(), "EverythingSample.rosev", None, dirty=False)
        self.set_status("Loaded sample")

    def confirm_dirty(self) -> bool:
        if not self.dirty:
            return True
        result = QMessageBox.question(
            self,
            APP_NAME,
            "Discard unsaved changes?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        return result == QMessageBox.StandardButton.Yes

    def refresh_outline(self) -> None:
        self.outline_list.clear()
        items: list[tuple[int, str]] = []
        for index, line in enumerate(self.editor.toPlainText().splitlines(), start=1):
            trimmed = line.strip()
            if re.match(r"^(rosev|namespace|class|setting|field|native|make|when|synvert)\b", trimmed):
                items.append((index, trimmed))

        for line, text in items:
            item = QListWidgetItem(f"{line}: {text}")
            item.setData(Qt.ItemDataRole.UserRole, line)
            self.outline_list.addItem(item)
        self.outline_count.setText(f"{len(items)} {'symbol' if len(items) == 1 else 'symbols'}")

    def goto_outline_item(self, item: QListWidgetItem) -> None:
        line = item.data(Qt.ItemDataRole.UserRole)
        if line:
            self.editor.goto_line(int(line))

    def refresh_diagnostics(self) -> None:
        diagnostics = validate_rosev(self.editor.toPlainText())
        self.diagnostics_list.clear()
        if not diagnostics:
            self.diagnostics_list.addItem("Info: No obvious RoseV syntax issues.")
            self.diagnostic_label.setText("No issues")
            return

        errors = sum(1 for item in diagnostics if item.level == "error")
        warnings = sum(1 for item in diagnostics if item.level == "warn")
        if errors:
            self.diagnostic_label.setText(f"{errors} {'error' if errors == 1 else 'errors'}")
        elif warnings:
            self.diagnostic_label.setText(f"{warnings} {'warning' if warnings == 1 else 'warnings'}")
        else:
            self.diagnostic_label.setText("No issues")

        for diagnostic in diagnostics:
            item = QListWidgetItem(f"{diagnostic.level.upper()} line {diagnostic.line}: {diagnostic.message}")
            item.setData(Qt.ItemDataRole.UserRole, diagnostic.line)
            self.diagnostics_list.addItem(item)

    def goto_diagnostic_item(self, item: QListWidgetItem) -> None:
        line = item.data(Qt.ItemDataRole.UserRole)
        if line:
            self.editor.goto_line(int(line))

    def format_document(self) -> None:
        cursor_position = self.editor.textCursor().position()
        formatted = format_rosev_source(self.editor.toPlainText())
        self.loading_source = True
        self.editor.setPlainText(formatted)
        self.loading_source = False
        cursor = self.editor.textCursor()
        cursor.setPosition(min(cursor_position, len(formatted)))
        self.editor.setTextCursor(cursor)
        self.dirty = True
        self.update_title()
        self.refresh_code_views()
        self.set_status("Formatted document")

    def insert_snippet(self, kind: str) -> None:
        snippets = {
            "load": '\nwhen load {\n  say "{mod} loaded on {loader}"\n}\n',
            "update": '\nwhen update {\n  every 300 {\n    say "Update alive"\n  }\n}\n',
            "function": '\nmake announce {\n  say "Reusable function called"\n}\n',
            "import": "\nimport csharp\nimport unity.core\nimport melonloader.core\nimport bepinex.core\nimport rosemod\n",
            "setting": '\nsetting enabled bool true "Enable this feature"\n',
            "field": "\nfield counter int = 0\n",
            "if": '\nif enabled is true {\n  say "Feature is enabled"\n}\n',
            "key": '\nkey F8 {\n  warn "F8 pressed"\n}\n',
            "unity": '\nunity "UnityEngine.Debug.Log(\\"RoseV Unity escape\\");"\n',
            "csharp": '\nsynvert = csharp\nvar message = $"Running on {Context.Loader}";\nLog.Info(message);\nsynvert = rosev\n',
            "members": '\nsynvert = csharp\nprivate int fullCSharpCounter;\n\nprivate void FullCSharpHelper()\n{\n  fullCSharpCounter++;\n  Log.Info($"Full C# helper ran {fullCSharpCounter} time(s).");\n}\nsynvert = rosev\n',
            "unitySynvert": '\nsynvert = unity\nUnityEngine.Debug.Log("Unity is C# here, using UnityEngine imports.");\nsynvert = rosev\n',
            "native": '\nnative c "Native/MyNativeCode.c" as MyNativeCode\n\nmake callNative {\n  native call MyNativeCode.MyNativeFunction\n}\n',
        }
        cursor = self.editor.textCursor()
        cursor.insertText(snippets.get(kind, ""))
        self.editor.setTextCursor(cursor)
        self.editor.setFocus()

    def compile_file(self) -> None:
        if self.compiling:
            return
        if self.dirty or self.current_path is None:
            if not self.save_file():
                return

        compiler = find_rosev_compiler(self.root)
        if compiler is None:
            QMessageBox.critical(
                self,
                APP_NAME,
                "RoseV.exe was not found. Build RoseV\\Native\\RoseV.Native.vcxproj first.",
            )
            self.set_status("Compiler not found")
            return

        assert self.current_path is not None
        output_path = self.current_path.with_suffix(".generated.cs")
        self.output_box.clear()
        self.generated_box.clear()
        self.bottom_tabs.setCurrentWidget(self.output_box)
        self.compiling = True
        self.set_status("Compiling...")

        self.thread = QThread(self)
        self.worker = CompileWorker(compiler, self.current_path, self.editor.toPlainText(), output_path)
        self.worker.moveToThread(self.thread)
        self.thread.started.connect(self.worker.run)
        self.worker.line.connect(self.append_output)
        self.worker.finished.connect(self.on_compile_finished)
        self.worker.finished.connect(self.thread.quit)
        self.worker.finished.connect(self.worker.deleteLater)
        self.thread.finished.connect(self.thread.deleteLater)
        self.thread.start()

    def append_output(self, line: str) -> None:
        self.output_box.appendPlainText(line)

    def on_compile_finished(self, ok: bool, _source_path: str, output_path: str, output: str) -> None:
        self.compiling = False
        self.thread = None
        self.worker = None
        if output and not self.output_box.toPlainText().strip():
            self.output_box.setPlainText(output)
        if ok:
            generated_path = Path(output_path)
            if generated_path.exists():
                self.generated_box.setPlainText(generated_path.read_text(encoding="utf-8", errors="replace"))
                self.bottom_tabs.setCurrentWidget(self.generated_box)
            self.set_status("Compile complete")
        else:
            self.bottom_tabs.setCurrentWidget(self.output_box)
            self.set_status("Compile failed")

    def focus_find(self) -> None:
        self.find_box.setFocus()
        self.find_box.selectAll()

    def find_next(self) -> None:
        self.find_text(backward=False)

    def find_previous(self) -> None:
        self.find_text(backward=True)

    def find_text(self, backward: bool = False) -> None:
        query = self.find_box.text()
        if not query:
            return
        flags = QTextDocument.FindFlag.FindBackward if backward else QTextDocument.FindFlag(0)
        if self.editor.find(query, flags):
            self.set_status(f"Found {query}")
            return

        cursor = self.editor.textCursor()
        cursor.movePosition(QTextCursor.MoveOperation.End if backward else QTextCursor.MoveOperation.Start)
        self.editor.setTextCursor(cursor)
        if not self.editor.find(query, flags):
            self.set_status(f"No match for {query}")

    def toggle_word_wrap(self) -> None:
        if self.editor.lineWrapMode() == QPlainTextEdit.LineWrapMode.NoWrap:
            self.editor.setLineWrapMode(QPlainTextEdit.LineWrapMode.WidgetWidth)
            self.set_status("Word wrap on")
        else:
            self.editor.setLineWrapMode(QPlainTextEdit.LineWrapMode.NoWrap)
            self.set_status("Word wrap off")

    def zoom_editor(self, delta: int) -> None:
        font = self.editor.font()
        next_size = max(8, min(22, font.pointSize() + delta))
        font.setPointSize(next_size)
        self.editor.setFont(font)
        self.editor.setTabStopDistance(self.editor.fontMetrics().horizontalAdvance(" ") * 2)
        self.editor.update_line_number_area_width(0)
        self.set_status(f"Editor font {next_size} pt")

    def show_command_palette(self) -> None:
        dialog = CommandPalette(self, self.commands)
        dialog.exec()

    def closeEvent(self, event) -> None:
        if self.confirm_dirty():
            event.accept()
        else:
            event.ignore()


def validate_rosev(source: str) -> list[Diagnostic]:
    diagnostics: list[Diagnostic] = []
    lines = source.splitlines() or [""]
    depth = 0
    has_metadata = False
    raw_synvert = False
    raw_depth = 0
    headers = re.compile(r"^(rosev|namespace|class|use|setting|field|import|native|synvert)\b")
    statements = re.compile(r"^(say|warn|error|emit|unity|cs|csharp|let|set|add|sub|mul|div|call|return|throw|native call|synvert)\b")
    blocks = re.compile(r"^(when|every|key|make|if|repeat|while|try|members|member|cs|csharp|synvert)\b")
    synvert_modes = {"rosev", "rose", "easy", "csharp", "cs", "c#", "unity", "melonloader", "melon", "bepinex", "rosemod", "il2cpp", "harmony"}

    for index, line in enumerate(lines, start=1):
        trimmed = strip_comment(line).strip()
        if not trimmed:
            continue
        synvert_match = re.match(r"^synvert\s*=\s*([A-Za-z0-9_#.-]+)", trimmed)

        if raw_synvert:
            if raw_depth == 0 and synvert_match and synvert_match.group(1).lower() in {"rosev", "rose", "easy"}:
                raw_synvert = False
                continue
            raw_depth += brace_delta(trimmed)
            if raw_depth <= 0 and trimmed == "}":
                raw_synvert = False
                raw_depth = 0
            elif raw_depth < 0:
                raw_depth = 0
            continue

        if trimmed.startswith("rosev "):
            has_metadata = True

        if synvert_match:
            mode = synvert_match.group(1).lower()
            if mode not in synvert_modes:
                diagnostics.append(Diagnostic("error", index, f"Unknown synvert language '{mode}'."))
                continue
            if mode not in {"rosev", "rose", "easy"}:
                raw_synvert = True
                raw_depth = 1 if trimmed.endswith("{") else 0
            continue

        if re.match(r"^(members|member|csharp|cs)\b.*\{$", trimmed):
            raw_synvert = True
            raw_depth = 1
            continue

        if trimmed == "}":
            depth -= 1
            if depth < 0:
                diagnostics.append(Diagnostic("error", index, "Closing brace has no matching block."))
                depth = 0
            continue

        csharp_statement = re.match(r"^(csharp|cs)\s+\"", trimmed)
        if blocks.search(trimmed) and not trimmed.endswith("{") and not csharp_statement:
            diagnostics.append(Diagnostic("error", index, "Block commands must end with {.")) 

        if not headers.search(trimmed) and not statements.search(trimmed) and not blocks.search(trimmed):
            diagnostics.append(Diagnostic("warn", index, "Unknown RoseV command. Use the Help panel for valid commands."))

        if trimmed.count('"') % 2 != 0:
            diagnostics.append(Diagnostic("error", index, "Quoted text is not closed."))

        if trimmed.endswith("{"):
            depth += 1

    if not has_metadata:
        diagnostics.insert(0, Diagnostic("warn", 1, "Add a rosev metadata line so the generated mod has a name, id, and version."))
    if raw_synvert:
        diagnostics.append(Diagnostic("warn", len(lines), "synvert raw syntax mode reaches the end of the file. Add synvert = rosev when you return to RoseV commands."))
    if depth > 0:
        diagnostics.append(Diagnostic("error", len(lines), f"{depth} block(s) are missing a closing }}."))
    return diagnostics


def brace_delta(text: str) -> int:
    in_string = False
    in_char = False
    escaped = False
    delta = 0
    for ch in text:
        if escaped:
            escaped = False
            continue
        if ch == "\\":
            escaped = True
            continue
        if not in_char and ch == '"':
            in_string = not in_string
            continue
        if not in_string and ch == "'":
            in_char = not in_char
            continue
        if in_string or in_char:
            continue
        if ch == "{":
            delta += 1
        elif ch == "}":
            delta -= 1
    return delta


def strip_comment(line: str) -> str:
    in_string = False
    escaped = False
    for index, ch in enumerate(line):
        if escaped:
            escaped = False
            continue
        if ch == "\\":
            escaped = True
            continue
        if ch == '"':
            in_string = not in_string
        if not in_string and ch == "#":
            return line[:index]
    return line


def format_rosev_source(source: str) -> str:
    depth = 0
    formatted: list[str] = []
    for line in source.splitlines():
        trimmed = line.rstrip().strip()
        if not trimmed:
            formatted.append("")
            continue
        if trimmed.startswith("}"):
            depth = max(0, depth - 1)
        formatted.append(f"{'  ' * depth}{trimmed}")
        depth = max(0, depth + brace_delta(trimmed))
    return "\n".join(formatted)


def find_rosev_compiler(ide_root: Path) -> Path | None:
    starts = [Path.cwd(), ide_root]
    if getattr(sys, "frozen", False):
        starts.append(Path(sys.executable).resolve().parent)
    else:
        starts.append(Path(__file__).resolve().parent)

    candidates = [
        Path("bin/RoseV/Release/RoseV.exe"),
        Path("../../bin/RoseV/Release/RoseV.exe"),
        Path("../../../bin/RoseV/Release/RoseV.exe"),
        Path("RoseV/Native/bin/RoseV/Release/RoseV.exe"),
    ]

    seen: set[Path] = set()
    for start in starts:
        for ancestor in [start, *start.parents]:
            if ancestor in seen:
                continue
            seen.add(ancestor)
            for candidate in candidates:
                full = ancestor / candidate
                if full.is_file():
                    return full
    return None


def main() -> int:
    app = QApplication(sys.argv)
    app.setApplicationName(APP_NAME)
    app.setStyleSheet(APP_STYLE)
    window = RoseVIDE()
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
