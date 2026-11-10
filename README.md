# AutoBuild Layout (Geode Mod)

An advanced Geometry Dash 2.2081 mod built with the Geode SDK that procedurally generates full gameplay layouts directly inside the level editor based on customizable difficulty, BPM, and length parameters.

## Features
- **Modern UI Popup**: Purple square button with an "E" badge placed in the top editor toolbar.
- **Configurable Parameters**:
  - **Difficulty**: Easy, Normal, Hard, Harder, Insane, Easy Demon, Medium Demon, Hard Demon, Insane Demon, Extreme Demon.
  - **BPM**: Custom input to calculate obstacle spacing and rhythmic placement.
  - **Length**: Duration in seconds to control total layout span.
- **Procedural Layout Generation**: Spawns authentic gameplay structures using real Geometry Dash editor object APIs (blocks, spikes, portals, orbs, pads, and speed triggers) without illegal states or unfair placements.

## Building
Use the official Geode CLI and GitHub Actions workflow to build for Windows, Android, and macOS.
