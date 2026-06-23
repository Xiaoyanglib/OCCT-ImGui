# OCCT-ImGui

![Windows](https://github.com/Xiaoyanglib/OCCT-ImGui/actions/workflows/build-windows.yml/badge.svg)
![macOS](https://github.com/Xiaoyanglib/OCCT-ImGui/actions/workflows/build-macos.yml/badge.svg)

A lightweight CAD viewer built on [OpenCASCADE](https://dev.opencascade.org/) (OCCT) and [Dear ImGui](https://github.com/ocornut/imgui).

**Supported platforms:** 💻 Windows · 🍎 macOS ·  *(🐧 Linux is not supported)*

**Supported formats:** [.step(.stp)](https://en.wikipedia.org/wiki/ISO_10303-21) · [.iges(.igs)](https://en.wikipedia.org/wiki/IGES) · [.brep](https://dev.opencascade.org/doc/overview/html/specification__brep_format.html)

## ✨ Features

- **3D Shape Viewer** — rotate/pan/zoom, section clipping, orientation gizmo for CAD models (STEP, IGES, BREP)
- **Object Panel** — toggle visibility, change colors, import/export
- **Selection** — click select, Shift+drag rectangle select

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

Install OpenCASCADE (8.0 or later recommended):

**Windows:**
Use pre-built binaries from [OCCT Releases](https://github.com/Open-Cascade-SAS/OCCT/releases), or build from source using CMake.

**macOS:**
Use Homebrew (may not be the latest version):
```bash
brew install opencascade
```

**Source build:**
To build from source instead (replace `V8_0_0` with the desired version tag):
```bash
git clone --depth 1 --branch V8_0_0 https://github.com/Open-Cascade-SAS/OCCT.git
cmake -B build -S OCCT -DCMAKE_INSTALL_PREFIX=./installed
cmake --build build -j$(sysctl -n hw.logicalcpu)
cmake --install build
```

### Configure & Build

```bash
# Clone with all submodules
git clone --recurse-submodules https://github.com/Xiaoyanglib/OCCT-ImGui.git
cd OCCT-ImGui

# Configure and build
cmake -B build -S . -DOpenCASCADE_DIR="<occt-install>/lib/cmake/opencascade"
cmake --build build
```

Set `OpenCASCADE_DIR` to the directory containing `OpenCASCADEConfig.cmake`:
- **Pre-built binaries (Windows):** `<occt-install>/cmake`
- **Homebrew (macOS):** `$(brew --prefix opencascade)/lib/cmake/opencascade`

### Integration

To use this project as a **submodule** in your own project, follow these steps:
1. Add the submodule:

    ```bash
   git submodule add https://github.com/Xiaoyanglib/OCCT-ImGui.git [SubmoduleDir]
   git submodule update --init --recursive
    ```
   
2. Configure and link the library:
    ```bash
    add_subdirectory([SubmoduleDir])
   target_link_libraries([Project] PRIVATE OcctImGui)
    ```

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
- [ImGuizmo](https://github.com/fknfilewalker/imoguizmo) — orientation gizmo
- [glad](https://glad.dav1d.de/) — OpenGL loader
- [Geogram](https://github.com/BrunoLevy/geogram) — section view and dock layout reference
- [OcctImGui](https://github.com/eryar/OcctImgui) — OCCT + ImGui integration reference
- [OCC-QT-Demo](https://github.com/ajune-wang/OCC-QT-Demo) — OCCT application patterns reference

## 📄 License

MIT License — Copyright (c) 2026 Xiaoyang Yu
