# Woosh - Audio Batch Editor
# Cursor AI Instructions & Project Guidelines

## Project Overview
Woosh is a Qt 6-based audio batch editor for game developers. It allows loading, 
previewing, trimming, normalizing, and compressing multiple audio files with a 
modern waveform editor interface.

## Version Management

### IMPORTANT: Bump Version for Each Build
Before each significant build or release, update the version number in:
- `CMakeLists.txt` line: `project(Woosh VERSION X.Y.Z LANGUAGES CXX)`

Version format: MAJOR.MINOR.PATCH
- MAJOR: Breaking changes or major feature releases
- MINOR: New features, non-breaking improvements  
- PATCH: Bug fixes, small improvements

Current version info is auto-generated to:
- `build/generated/Version.h` (C++ header)
- `build/woosh.rc` (Windows resource file with file properties)

## Technology Stack

- **Language**: C++20
- **Build System**: CMake 3.21+
- **GUI Framework**: Qt 6 (Widgets, Multimedia)
- **Audio I/O**: libsndfile (WAV), mpg123 (MP3 decode)
- **Package Manager**: vcpkg
- **Platforms**: Windows, macOS, Linux

## Architecture & Design Choices

### UI Architecture
- **MainWindow**: Central coordinator, owns all major components
- **ClipTableModel**: QAbstractTableModel for sortable file list
- **WaveformView**: Custom QWidget for waveform visualization and editing
- **AudioPlayer**: Qt Multimedia-based playback with position tracking
- **TransportPanel**: Play/pause/stop, zoom, trim controls
- **ProcessingPanel**: Normalize and compress parameter controls
- **OutputPanel**: Export folder and options

### Audio Processing
- All processing operates on in-memory float samples (-1.0 to 1.0 range)
- Original samples are preserved for undo support (AudioClip::saveOriginal)
- Playback converts to 16-bit PCM on-the-fly
- Supports resampling when device format differs from clip format

### Key Design Patterns
1. **RAII**: Use smart pointers and Qt parent-child ownership
2. **Signal/Slot**: Qt signals for loose coupling between components
3. **Model/View**: QAbstractTableModel + QSortFilterProxyModel for clip list
4. **Settings Persistence**: QSettings for user preferences

### Waveform Display
- Vertical lines per pixel column (traditional DAW style)
- Stereo: Top half = left channel, bottom half = right channel
- Min/max value caching per column for efficient rendering
- Zoom-to-cursor functionality (Ctrl+wheel)

### Audio Playback
- Uses QAudioSink for streaming playback
- Position timer (30ms interval) for playhead updates
- Playback region support for trim preview
- Format negotiation with fallback resampling

## Code Style Guidelines

### File Organization
- Keep files under 1000 lines where practical
- Use `#pragma once` for header guards
- Group includes: Qt, STL, project headers
- Use `Q_OBJECT` macro for QObject subclasses
- Use `Q_SLOTS` and `Q_SIGNALS` (not `slots`/`signals`) for Qt 6 compatibility

### Naming Conventions
- Classes: PascalCase (e.g., `AudioPlayer`)
- Methods: camelCase (e.g., `setPlaybackRegion`)
- Member variables: camelCase with trailing underscore (e.g., `audioPlayer_`)
- Constants: kPascalCase (e.g., `kMaxRecentItems`)

### Documentation
- Use Doxygen-style comments for public APIs
- File-level comment with @file, @brief
- Brief comments for non-obvious private members

## Common Pitfalls to Avoid

1. **Qt Destruction Order**: Disconnect signals in destructors before cleanup
2. **Model Reset**: Call `refreshModelPreservingSelection()` to keep selection
3. **Audio Thread Safety**: Don't access UI from audio callbacks
4. **Unicode Symbols**: Use appropriate fonts for transport button symbols

## Patterns and practices to prioritize

1. **RAII and smart ownership everywhere**

   * Use `std::unique_ptr`, `std::shared_ptr` or `QScopedPointer` for non-`QObject` types.
   * Let parent–child handle `QObject` lifetimes, avoid manual `delete`.

2. **Clear ownership for QObjects**

   * Set parents in constructors so ownership is explicit.
   * Avoid stack-allocating long-lived widgets or controllers with unclear lifetime.

3. **Minimize work in the main (GUI) thread**

   * Use worker `QObject`s in `QThread`s for heavy CPU or blocking I/O.
   * Use queued connections or `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` for cross-thread calls.

4. **Leverage implicit sharing and value types**

   * Prefer Qt’s implicitly shared types (`QString`, `QByteArray`, `QImage`, etc.) where copying is common.
   * Don’t call `detach()` unless you explicitly need a deep copy.

5. **Use model/view and model/delegate patterns**

   * Implement `QAbstractItemModel` (and friends) instead of hand-wiring widgets to data.
   * Keep models free of UI logic to stay testable and reusable.

6. **Prefer signals and slots over tight coupling**

   * Use signals for state changes and events instead of direct calls across layers.
   * Use the new function pointer syntax for type-safe connections (no `SIGNAL`/`SLOT` macros).

7. **Avoid unnecessary heap churn and temporaries**

   * Reserve container capacity (`reserve`, `resize`) before filling in hot paths.
   * Reuse buffers and avoid naive `QString` concatenation in tight loops (consider `QStringBuilder` / `reserve()`).

8. **Profile and measure with Qt tools**

   * Use `QElapsedTimer` for quick checks and Qt Creator’s profiler or external tools for hotspots.
   * Only micro-optimize where measurements show it matters.

9. **Use modern C++ features consistently**

   * Apply `noexcept`, `final`, `override`, `[[nodiscard]]`, and move semantics where appropriate.
   * Use `std::span`, `std::optional`, `std::variant` to make APIs safer and cheaper.

10. **Design for deterministic resource management**

    * Wrap OS/library handles (file descriptors, sockets, GPU resources, etc.) in RAII types.
    * Ensure cleanup happens in destructors, not via fragile ordering of signals or event loop callbacks.


## Build & Test Commands

Woosh is set up for a CMake **preset-based** workflow that plays nicely with
Visual Studio 2022/2026 and a standalone Qt install under `C:\Qt`.

Key pieces:
- `CMakeLists.txt` uses C++20, Qt 6 (Widgets + Multimedia), libsndfile, mpg123.
- `vcpkg.json` declares only the audio libs (`libsndfile`, `mpg123`).
- `CMakePresets.json` selects the **Visual Studio 17 2022** generator and sets:
	- `CMAKE_TOOLCHAIN_FILE` to `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`.
	- `VCPKG_TARGET_TRIPLET` to `x64-windows`.
	- `CMAKE_PREFIX_PATH` to something like `C:/Qt/6.10.1/msvc2022_64`.

> Note: Visual Studio 2026 still uses the `Visual Studio 17 2022` CMake
> generator under the hood; the presets remain valid.

### Environment prerequisites

- Install Qt 6 with the **MSVC 2022 64-bit** kit, e.g. to
	`C:\Qt\6.10.1\msvc2022_64`.
- Install vcpkg and set `VCPKG_ROOT` so the presets and auto-detection work.
- Let vcpkg restore `libsndfile` and `mpg123` using `vcpkg.json` (either via
	Visual Studio integration or by running `vcpkg install` manually if needed).

### Recommended: CMake presets (CLI)

```bash
# Configure Debug
cmake --preset x64-debug

# Build Debug
cmake --build --preset x64-debug

# Configure Release
cmake --preset x64-release

# Build Release
cmake --build --preset x64-release
```

### Visual Studio 2022 / 2026

- Open the repository folder directly in Visual Studio.
- VS detects `CMakePresets.json` and offers `x64-Debug`, `x64-Release`, and
	`x64-RelWithDebInfo` configurations.
- Build the `Woosh` target from the CMake Targets View.

### Running tests

```bash
# Debug tests
cmake --build --preset x64-debug --target WooshTests
ctest --preset x64-debug

# Release tests
cmake --build --preset x64-release --target WooshTests
ctest --preset x64-release
```

### Packaging

CPack is configured in `CMakeLists.txt`. After configuring with a preset, you
can build the `package` target to create installers/archives:

```bash
cmake --build --preset x64-release --target package
```

## GitHub Workflow

The CI/CD pipeline builds for Windows, macOS, and Linux:
- Triggered on push to master or pull requests
- Creates release artifacts (installers/zips) on tagged releases
- Uses vcpkg for dependency management

## Author Information

- **Author**: Pedro G. Dias
- **Handle**: digitaldias
- **Repository**: https://github.com/digitaldias/woosh

