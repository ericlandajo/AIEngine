# AIEngine — Modular AI engine for NPCs

**English** | [Català](README.ca.md)

An artificial intelligence engine for non-player characters (NPCs), decoupled from the game engine. Behaviours are designed in a node-based visual editor, either as a **behaviour tree** (BT) or as a **finite state machine** (FSM). They are exported to a binary file using [FlatBuffers](https://flatbuffers.dev/), and the engine interprets and runs them during play. The reference integration is with Unity.

This repository contains the code of the Bachelor's thesis *Disseny i desenvolupament de l'arquitectura modular d'un motor d'IA per a NPC* (Design and development of the modular architecture of an AI engine for NPCs), Bachelor's Degree in Informatics Engineering, Barcelona School of Informatics (FIB), UPC.

## Repository structure

```
.
├── engine/              Engine core (C++)
│   ├── schema/          Data schema (AIBrainTemplate.fbs) and generated C++ code
│   ├── src/             Engine, custom nodes and C API for P/Invoke
│   ├── include/         Bundled third-party dependencies (aitoolkit, FlatBuffers)
│   ├── tests/           Test suite (doctest + CTest) and test fixtures
│   └── CMakeLists.txt
└── editor/              Visual editor (React + React Flow)
    ├── src/             Editor code and TypeScript code generated from the schema
    └── public/engine/   Unity installation package (plugin, shared types and instructions)
```

The schema `engine/schema/AIBrainTemplate.fbs` is the single source of truth for the data format: both the engine's C++ code and the editor's TypeScript code are generated from it.

## Requirements

- **Engine:** CMake 3.20 or later and a C++20 compiler (Visual Studio 2022, GCC or Clang).
- **Editor:** Node.js 20.19 or later (or 22.12 or later).
- **Integration:** Unity (tested on Windows).
- **Optional:** the `flatc` schema compiler, version 25.12.19, only if the schema is modified.

## Engine (C++)

All commands in this section are run from the `engine/` folder.

### Build

```bash
cd engine
cmake -S . -B build
cmake --build build --config Release
```

Three targets are built:

| Target | Description |
|---|---|
| `AIEngineCore` | Static library with the engine core, independent of any game engine. |
| `AIEnginePlugin` | Shared library (`AIEnginePlugin.dll` on Windows) used by Unity. |
| `AIEngineDemo` | Console program that loads a behaviour and simulates a few frames. |

To output the DLL directly into a Unity project when building:

```bash
cmake -S . -B build -DAIENGINE_UNITY_PLUGINS_DIR="C:/path/to/project/Assets/Plugins"
```

### Tests

```bash
ctest --test-dir build -C Release
```

The suite contains 155 test cases and runs without opening any game engine. The `.bin` files in `tests/fixtures/` are brains exported from the editor or generated for the performance tests; the `.json` files with the same name show their contents in readable form. To build without the tests: `-DAIENGINE_BUILD_TESTS=OFF`.

### Console program

With no arguments it loads `tests/fixtures/test_brain.bin`. Another brain can be passed as an argument:

```bash
./build/Release/AIEngineDemo tests/fixtures/editor_bt_basic.bin
```

(On Linux and macOS the executable is at `build/AIEngineDemo`.)

## Visual editor

```bash
cd editor
npm ci
npm run dev
```

The editor opens at `http://localhost:5173`. To build a static version: `npm run build` (output in `editor/dist/`). The application runs entirely in the browser and needs no server of its own.

From the editor you can:

- Model behaviours as a BT or as an FSM.
- Save and load the working project (an editor-specific `.json` file).
- Export a behaviour: this downloads `Name.bin` and the class `AIBrainBase_Name.cs`.
- Download the engine's installation package for Unity (**Engine Setup** button).

## Unity integration

1. Download the installation package from the editor (**Engine Setup**) and copy `AIEnginePlugin.dll` to `Assets/Plugins/` and `AIEngineTypes.cs` to `Assets/Scripts/`. This is needed only once per project.
2. Export a behaviour from the editor. Copy `Name.bin` to `Assets/StreamingAssets/` and `AIBrainBase_Name.cs` to `Assets/Scripts/`.
3. Create a class that inherits from `AIBrainBase_Name` and implement its abstract methods (one per action and one per variable defined in the editor).
4. Add the component to the NPC and enter the `.bin` file name in the **Brain File Name** field of the inspector.

Full instructions, with an example and troubleshooting, are in [`editor/public/engine/README.txt`](editor/public/engine/README.txt).

## Modifying the data schema

If `engine/schema/AIBrainTemplate.fbs` is modified, the code on **both** sides must be regenerated with the same `flatc` version (25.12.19). From the repository root:

```bash
flatc --cpp -o engine/schema engine/schema/AIBrainTemplate.fbs
flatc --ts  -o editor/src engine/schema/AIBrainTemplate.fbs
```

Then rebuild the plugin and replace `editor/public/engine/AIEnginePlugin.dll` with the new version.

## License

This project is released under the MIT License. See the [LICENSE](LICENSE) file.

It includes or uses third-party components, each under its own license:

| Component | Use | License |
|---|---|---|
| [aitoolkit](https://github.com/linkdd/aitoolkit) | Base behaviour tree nodes (`engine/include/aitoolkit`) | MIT |
| [FlatBuffers](https://github.com/google/flatbuffers) | Serialization (`engine/include/flatbuffers` and npm package) | Apache 2.0 |
| [doctest](https://github.com/doctest/doctest) | Test framework (downloaded at build time) | MIT |
| [React Flow](https://reactflow.dev/) (`@xyflow/react`) | Graph editor | MIT |
| [React](https://react.dev/) | Editor UI | MIT |
| [JSZip](https://stuk.github.io/jszip/) | Installation package | MIT or GPLv3 (used under MIT) |
