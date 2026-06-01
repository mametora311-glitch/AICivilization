# AICivilization

AICivilization is a Windows desktop civilization-observation simulator built
with C++20, raylib, and raygui.

Agents live inside a closed world, consume resources, reproduce, mutate, create
concepts, and gradually form laws, religions, structures, and civilization
stages. The simulation also supports anomaly events such as BlackHole, Gravity,
WhiteHole, and UnknownStress, including the BH/GV/HH civilization transfer
experiment.

The UI and in-game notifications are Japanese-localized.

## Features

- Real-time closed-world civilization simulation
- Agent lifecycle: movement, resource use, death, birth, mutation, memory, faith
- Concept generation and bounded multi-stage concept composition
- Civilization systems: laws, religions, structures, stability, collapse risk
- Civilization stages from Primitive to PostSingularity
- Anomaly lab for manual BlackHole / Gravity / WhiteHole / UnknownStress events
- Scenario selection with long-form 18,000-cycle default play
- BGM, sound effects, optional VOICEVOX TTS, and Windows notifications
- Save/load, snapshots, CSV export, logs, timeline, and final report output
- Headless tests for simulation, concepts, anomalies, agents, and save/load

## Requirements

- Windows
- Visual Studio with the MSVC C++ toolchain
- Network access during the first CMake configure step

No separate CMake or Ninja install is required. `build.bat` locates Visual
Studio with `vswhere` and uses the bundled Visual Studio CMake/Ninja tools.

VOICEVOX is optional. If VOICEVOX ENGINE is running locally at
`http://127.0.0.1:50021`, the app can play TTS notifications.

## Build

```bat
build.bat
```

The executable is generated at:

```text
build\AICivilization.exe
```

Third-party dependencies are fetched and cached by CMake:

- raylib
- raygui
- nlohmann/json
- cpp-httplib

## Run

```bat
run.bat
```

`run.bat` starts the executable from the project root so that `assets`,
`config`, `data`, `logs`, and `cache` resolve correctly.

## Controls

- Mouse wheel: zoom the world view
- Left drag: pan the world view
- Left click: select an agent, concept, or anomaly
- Right click: open the context panel for the selected object
- Space: pause/resume
- R: reset
- S: snapshot
- L: export CSV/report/screenshot
- F1: help overlay

Most actions are also available through the in-app buttons and tabs.

## Scenarios

Scenario presets live in `config/scenarios`.

- `primitive_seed`: standard long-form primitive civilization
- `closed_box`: closed observation box
- `memory_collapse`: memory-loss pressure scenario
- `religion_bloom`: religion emergence scenario
- `unknown_stress`: UnknownStress anomaly scenario
- `bh_gv_hh_experiment`: BlackHole -> Gravity -> WhiteHole experiment
- `total_random`: randomized observation scenario

## Project Layout

```text
.
├─ assets/          BGM, sound effects, placeholder texture/font assets
├─ config/          app settings, anomaly rules, scenario presets
├─ src/             application source
├─ tests/           headless verification programs
├─ build.bat        Release build helper
├─ run.bat          run helper
└─ CMakeLists.txt   CMake project definition
```

Generated folders such as `build`, `cache`, `logs`, `data/saves`,
`data/snapshots`, and `data/exports` are intentionally not committed.

## Validation

Current local validation:

```bat
build.bat
build\tests\test_agent.exe
build\tests\test_anomaly.exe
build\tests\test_concept.exe
build\tests\test_save_load.exe
build\tests\test_simulation.exe
```

The long-form primitive scenario reaches 18,000 cycles, keeps a living
population, avoids automatic collapse, and continues generating late events.
