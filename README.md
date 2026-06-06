# OCCT-ImGui

![Windows](https://github.com/Xiaoyanglib/OCCT-ImGui/actions/workflows/build-windows.yml/badge.svg)
![macOS](https://github.com/Xiaoyanglib/OCCT-ImGui/actions/workflows/build-macos.yml/badge.svg)

A lightweight CAD viewer built on [OpenCASCADE](https://dev.opencascade.org/) (OCCT) and [Dear ImGui](https://github.com/ocornut/imgui), with [GLFW](https://www.glfw.org/) for windowing and OpenGL rendering.

**Supported platforms:** 💻 Windows · 🍎 macOS ·  *(🐧 Linux is not supported)*

## ✨ Features

- **3D Shape Viewer** — display, rotate, pan, and zoom CAD models (STEP, IGES, BREP)
- **Object Panel** — toggle visibility, change colors, add primitives, delete shapes
- **Selection** — click to select/deselect, Shift+drag for rectangle selection
- **Import/Export** — load and save CAD files via native file dialogs

## 📁 Project Structure

```
OCCT-ImGui/
├── CMakeLists.txt              # CMake build configuration
├── README.md
├── cmake/                      # CMake dependency modules
│   ├── Dependencies.cmake      # Top-level dependency loader
│   ├── GLFW.cmake              # GLFW via add_subdirectory
│   ├── ImGui.cmake             # ImGui (docking branch) via add_subdirectory
│   └── OpenCASCADE.cmake       # Finds installed OCCT
├── third_party/                # Git submodules + glad
│   ├── glfw/                   #   glfw 3.3.8 (submodule)
│   ├── imgui/                  #   imgui docking (submodule)
│   ├── imoguizmo/              #   imoguizmo (submodule)
│   ├── googletest/             #   googletest v1.14.0 (submodule)
│   └── glad/                   #   glad GL 3.2 core (generated source)
├── tests/
│   ├── CMakeLists.txt          # GTest via add_subdirectory
│   └── test_occt_imgui.cpp     # Viewer API tests
└── src/
    ├── main.cpp                # DemoApp: demo shapes
    ├── Application.cpp/.h      # Framework: window, ImGui, docking, style,
    │                           #   menu/status bars, Viewer panel, rect select
    ├── OcctImGui.cpp/.h        # Library: shape mgmt, Objects panel, section,
    │                           #   import/export, orientation gizmo, GL overlay
    ├── OcctViewer.cpp/.h       # OCCT 3D viewer: camera, selection, clip planes
    ├── OcctWindow.cpp/.h       # GLFW-to-OCCT window bridge
    ├── OcctImConfig.h          # ImGui user config header
    └── fonts/                  # Roboto font files (SemiBold, Regular, etc.)
```

## 📦 Dependencies

| Dependency | Version | Purpose |
|---|---|---|
| OpenCASCADE | 8.0+ | CAD kernel |
| Dear ImGui | docking branch | UI rendering |
| GLFW | 3.3.x | Window + OpenGL context |
| ImGuizmo | latest | Orientation gizmo |
| Google Test | 1.14.x | Unit testing |
| glad | GL 3.2 core | OpenGL loader |
| CMake | 3.20+ | Build system |

GLFW, Dear ImGui, ImGuizmo, and Google Test are included as git submodules. glad source is committed directly. OpenCASCADE must be installed separately.

## 🔨 Build

### Prerequisites

Install OpenCASCADE 8.0+:

**macOS:**
Coming soon.

**Windows:**
Use pre-built binaries from [OCCT Releases](https://github.com/Open-Cascade-SAS/OCCT/releases), or build from source using CMake.

### Configure & Build

```bash
# Clone with all submodules
git clone --recurse-submodules https://github.com/Xiaoyanglib/OCCT-ImGui.git
cd OCCT-ImGui

# Configure and build
cmake -B build -S . -DOpenCASCADE_DIR="<occt-install>/lib/cmake/opencascade"
cmake --build build
```

Set `OpenCASCADE_DIR` to the directory containing `OpenCASCADEConfig.cmake` (typically `<occt-install>/lib/cmake/opencascade`).

## 🎮 Controls

| Action | Input |
|---|---|
| Rotate | Right mouse drag |
| Pan | Left mouse drag |
| Zoom | Scroll wheel / pinch / middle mouse drag |
| Fit all | `F` key |
| Select | Click shape / drag rectangle |
| Toggle selection | Shift + click |
| Toggle Viewer panel | `F7` |
| Toggle Objects panel | `F8` |
| Import file | Drag & drop / File → Import |

## 🙏 Acknowledgements

Built with and inspired by these excellent projects:

- [OpenCASCADE](https://dev.opencascade.org/) — CAD kernel
- [Dear ImGui](https://github.com/ocornut/imgui) — immediate-mode GUI
- [GLFW](https://www.glfw.org/) — cross-platform windowing
- [Geogram](https://github.com/BrunoLevy/geogram) — reference for ImGui + 3D viewer integration
- [OcctImgui](https://github.com/eryar/OcctImgui) — reference OCCT + ImGui integration
- [OCC-QT-Demo](https://github.com/ajune-wang/OCC-QT-Demo) — reference for OCCT application patterns

## 📄 License

MIT License — Copyright (c) 2026 Xiaoyang Yu
