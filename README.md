# CAD

A 3D CAD application for geometric modeling, developed as part of a graduate course in computational methods for geometric modeling.

## About

The application lets you create and manipulate geometric objects in 3D space — including parametric surfaces, Bezier curves, and interpolating splines. It supports stereoscopic 3D rendering and interactive scene editing via an immediate-mode GUI.

## Built With

- **C++17** — core language
- **OpenGL 4.0** (Core Profile) — rendering, with tessellation shader support
- **GLFW** — window management and input
- **GLAD** — OpenGL loader
- **ImGui** — immediate-mode UI

## Building

The project uses MSVC. Open a Developer Command Prompt and run the build script:

```bat
build.bat
```

Dependencies (GLFW, GLAD, ImGui) are included in the repository.

## Status

Active development — features and UI are subject to change.
