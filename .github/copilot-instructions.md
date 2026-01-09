# VRM4U — Copilot instructions

Purpose: short, focused guidance so an AI coding agent can be productive immediately in this repo.

## Quick architecture snapshot 🔧
- This is an Unreal Engine plugin (see `VRM4U.uplugin`). Key modules:
  - `VRM4U` (Runtime core)
  - `VRM4ULoader` (runtime loader; Windows/Mac whitelisted)
  - `VRM4URender` (render helpers)
  - `VRM4UImporter` (UncookedOnly import-time code; Windows/Mac)
  - `VRM4UEditor`, `VRM4UMisc` (editor tools)
- Content assets (sample maps, materials, example blueprints) are in `Content/` (look at `Content/Maps/` and `Content/MaterialUtil/`).
- ThirdParty libraries: `ThirdParty/assimp` (prebuilt Windows libs included), `ThirdParty/rapidjson`.

## Developer workflows — build, package, debug 🚀
- Release packaging is scripted in `Source/ReleaseScript/`:
  - Use `build_5.bat` to build the common UE5 targets.
  - `build_all.bat` runs `build_5.bat` then legacy builds.
  - `build_ver2.bat <UEVER> <Platform> <Config> <ZipName>` runs UAT BuildPlugin and then compresses output.
    - Example: `call Source\ReleaseScript\build_ver2.bat 5.7 Win64 Shipping VRM4U_5_7_YYYYMMDD.zip`
- Important: `build_ver2.bat` assumes Epic Games installs under `D:\Program Files\Epic Games`. Modify the script if your UE installs elsewhere.
- The build script contains engine-specific conditionals (e.g., it deletes retargeter or render module files for older UE versions). When changing features, update `build_ver2.bat` accordingly.
- Running locally for development: copy plugin to your project's `Plugins/VRM4U` folder (next to `.uproject`) and open the project in Unreal Editor. Use Visual Studio to build and attach to the UE Editor process for native debugging.

## Project-specific conventions & gotchas ⚠️
- Asset/name normalization: on import, spaces and `.` in bone/morph names are replaced with `_` by default (can be toggled in import options).
- Material prefixes: newer versions use `MI_` for material instances (previously `M_`). Be consistent with the existing assets under `Content/` when adding materials.
- Platform notes:
  - Windows: prebuilt assimp libs are included; builds should succeed without extra steps.
  - macOS / Mobile / runtime-load scenarios: custom assimp build is required. See README for details and `ThirdParty/assimp` structure.
- Build scripts use PowerShell helpers (`version.ps1`, `compress2.ps1`) and call `wsl` for minor math; ensure WSL and PowerShell execution policy permit running these scripts on CI/dev machines.

## Editor automation & useful files 🧰
- Editor Python helpers: `Content/Python/` contains scripts to automate common tasks (ControlRig generation, morph target controllers, node table creation). Use them for repetitive asset generation.
- Sample maps and debugging assets: `Content/Maps/VRM4U_sample.umap` and the `MaterialUtil/` assets are good references for expected runtime behavior.

## Where to make changes (practical guidance) 🔎
- For runtime fixes and new features, modify `Source/VRM4U*` modules; prefer to keep editor-only tools in `VRM4UEditor` or `VRM4UMisc`.
- If you add engine-version conditional code, document why and update `Source/ReleaseScript/build_ver2.bat` so release packaging doesn't accidentally strip needed files.

## Tests & CI (current state)
- There are no automated tests or GitHub Actions workflows in this repo. If you add CI, include steps to:
  - Install the required UE engine or use prebuilt CI images
  - Run `build_ver2.bat` for at least one UE version
  - Validate that sample maps open and import a small VRM file without errors

## Examples to reference inside the repo
- Packaging: `Source/ReleaseScript/build_5.bat`, `Source/ReleaseScript/build_ver2.bat`
- Plugin manifest & modules: `VRM4U.uplugin`
- Editor automation: `Content/Python/VRM4U_CreateHumanoidControllerUE5.py`
- ThirdParty: `ThirdParty/assimp/` (libs + include)

---
If anything above is unclear or you want more detail (CI recipe, tests, or sample PR description templates), tell me which section to expand. ✅
