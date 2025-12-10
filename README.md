# Woosh

A Qt 6 desktop utility for game audio teams to batch-trim, normalize, and lightly compress WAV/MP3 assets. First iteration focuses on non-destructive edits and WAV export; MP3 export will be added later.

## Features (current)
- Open WAV and MP3 files (decode via libsndfile + mpg123).
- View basic metadata (duration, sample rate, channels, peak/RMS).
- Apply trim, peak/RMS normalize, and a simple compressor in-memory.
- Batch process folders; export processed clips as WAV with `_woosh` suffix.
- Minimal GUI: file list, placeholder waveform, batch dialog, and per-clip controls.

## Roadmap / TODO
- Add MP3 export via LAME.
- Add OGG/FLAC support (libogg/libvorbis/libFLAC).
- Replace placeholder waveform with real visualization.
- Loudness normalization (EBU R128/LUFS).
- More robust test coverage and headless pipelines.

## Dependencies
Built with CMake and vcpkg (assumes `vcpkg integrate install` already run).

```
vcpkg install qtbase qttools qt6-multimedia libsndfile mpg123
```

Triplet: `x64-windows` (adjust as needed).

## Building
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Debug
```

Notes:
- Ensure `VCPKG_ROOT` is set; CMake will auto-detect if already integrated.
- On Windows, `Woosh` is built as a GUI (no console). Use `windeployqt` later for redistribution.

## Running
- From CMake build dir: `build\Debug\Woosh.exe` (or `Release`).
- Drop a few WAV/MP3 files into a `samples/` folder and open them via File → Open Folder.
- Exported files are saved alongside originals with `_woosh` suffix by default.

## Tests
Simple assert-style tests (no external framework):
```
cmake --build build --config Debug --target WooshTests
ctest --test-dir build
```

## Limitations (current)
- MP3 export not implemented (decode only).
- Waveform view is a placeholder.
- Preview playback uses Qt Multimedia; may need extra codecs on some systems.

## Structure
```
Woosh/
  src/
    ui/
    audio/
    utils/
  resources/
  tests/
```


