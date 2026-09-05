# GhostView 👻

<p align="center">
  <b>A floating, hardware-accelerated, ultra-lightweight modern C++ photo and animation viewer.</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat&logo=c%2B%2B" alt="C++17" />
  <img src="https://img.shields.io/badge/Graphics-Direct2D%20%7C%20DirectComposition-7B42BC?style=flat" alt="Direct2D" />
  <img src="https://img.shields.io/badge/Platform-Windows%2010%20%2F%2011-0078D6?style=flat&logo=windows" alt="Windows" />
  <img src="https://img.shields.io/badge/Size-~360%20KB%20(Single%20Binary)-brightgreen?style=flat" alt="Size" />
  <img src="https://img.shields.io/badge/Methodology-Vibe%20Coding-ff69b4?style=flat" alt="Vibe Coding" />
  <img src="https://img.shields.io/badge/Language-English%20%7C%20Turkish-blue?style=flat" alt="Bilingual" />
</p>

---

> 💡 **Development Philosophy:**  
> This project was brought to life from scratch using the **vibe coding** philosophy and an AI-augmented concurrent programming approach. Architectural decisions, Direct2D/DirectComposition GPU optimizations, interactive tooling, and UX flow loops were engineered end-to-end through interactive AI pair programming.

---

## 🌟 Highlights & Key Features

- 👻 **Ghost Viewport (Frameless Floating Canvas):**  
  No clunky window borders, titlebars, or sluggish menu strips. Your desktop is dimmed (75–80%) across fullscreen while your photo floats suspended with subtle drop-shadow depth.
- ⚡ **Direct GPU Acceleration (Direct2D + DirectComposition + WIC):**  
  Harnesses the direct power of your GPU for rock-solid 60 FPS smooth zooming, panning, and rotation with virtually instant (0 ms) cold startup.
- 🎞️ **Animated GIF Support:**  
  Precise multi-frame delay timing and buttery-smooth animated GIF playback.
- ✂️ **Interactive Crop Tool (`C`):**  
  Intuitive 8-way crop handles, Rule-of-Thirds overlay guide grid, and freehand crop rectangle repositioning.
- 🪄 **Magic Color & Background Eraser (`E`):**  
  High-performance flood-fill eraser algorithm to make clicked color regions or objects instantly transparent.
  - **Shift + Left Click:** Erases the sampled color globally across the entire image.
  - **`B` Key (Auto Background Remover):** Automatically samples outer corners to isolate and remove solid backgrounds in a single keystroke.
- ↩️ **Undo (`Ctrl+Z`) & 💾 Save As (`Ctrl+S`):**  
  15-step undo history buffer with 32-bit PNG (preserving alpha transparency) and JPEG export capabilities.
- 🌐 **Intelligent Bilingual System (English & Turkish):**  
  Auto-detects Windows system display language (`GetUserDefaultUILanguage`). Toggle dynamically at any time with the `[EN]` / `[TR]` HUD button or `T` key, persisting your preference in the Windows Registry.
- 🖼️ **Picasa-Inspired Async Filmstrip:**  
  Seamless thumbnail navigation bar at the bottom. Powered by an independent background worker thread ensuring zero UI stutter even in directories with thousands of high-resolution images.
- 📦 **100% Portable Single Binary:**  
  A single standalone `GhostView.exe` executable (~360 KB). Zero dependencies—no .NET Runtime, WebView2, or heavy Qt runtimes needed.

---

## ⌨️ Shortcuts & Controls

| Shortcut / Action | Function |
|---|---|
| **Mouse Cursor** | Transforms into 4-way move cursor (`IDC_SIZEALL`) when hovering over image |
| **Left Click + Drag** | Freely pan / move the image |
| **Mouse Wheel** | Smooth, cursor-centered focal zoom |
| **Double Click (Image)** | Toggle 1:1 Actual Pixel Size |
| **Double Click (Empty Canvas)** | Toggle Fullscreen / Windowed Mode |
| **T** | Toggle Language (English / Turkish) |
| **C** | Toggle Interactive Crop Tool |
| **Enter** | Apply Crop / Toggle Fullscreen |
| **ESC** | Cancel Crop / Exit Application |
| **E** | Toggle Magic Eraser Tool |
| **Left Click (Eraser)** | Make clicked contiguous color region transparent |
| **Shift + Left Click** | Erase sampled color globally across image |
| **B** | Auto-remove background (sampled from corners) |
| **Ctrl + Z** | Undo last edit action |
| **Ctrl + S** | Save As (PNG with transparency / JPEG) |
| **M** | Cycle Aspect Ratio Modes (Fit / Fill / Stretch / 1:1) |
| **Left / Right Arrow, A / D** | Previous / Next Image |
| **R / L** | Rotate 90° Clockwise / Counter-Clockwise |
| **F11** | Toggle Fullscreen / Windowed Mode |
| **O** | Open File Dialog |
| **[ / ]** | Adjust desktop background dimming opacity |
| **Drag & Drop** | Drop any image directly onto the window |

---

## 🚀 Quick Start

1. Simply run the standalone [`GhostView.exe`](GhostView.exe) binary included in this repo. No installation required.
2. **To Set as Windows Default Photo Viewer:**
   - Right-click [`set_default.bat`](set_default.bat) and run as Administrator (or double-click it).
   - In the Windows Default Apps settings window that appears, select **GhostView** as your default photo viewer.

---

## 🛠️ Building from Source

Building with MSVC (Visual Studio 2022 C++ build tools) is straightforward:

```cmd
build.bat
```

Or using CMake:

```cmd
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

Output: `GhostView.exe` (~360 KB standalone portable executable).

---

## 📄 License

MIT License © 2026 GhostView Contributors
