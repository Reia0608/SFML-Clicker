# My Familiars

An idle clicker built solo in C++ with SFML (no engine) for Windows. Instead of the usual click-to-earn-currency loop, you collect and evolve familiars that fight for you, unlock passive and active abilities, and push through a randomly generated stage.

This is a work-in-progress portfolio project — the code, architecture, and feature set are still evolving.

## Download and play

Grab the latest build from the [Releases page](https://github.com/Reia0608/SFML-Clicker/releases/latest):

1. Download the `.zip` from the latest release
2. Extract it anywhere
3. Run `SFML_Clicker.exe`

No installation or extra setup needed — everything required to run is included in the zip.

## Current features

- Purchasable upgrades
- 3 purchasable familiars, each with a first evolution
- Active attacks and passive abilities for familiars
- Familiar slots that must be unlocked with amber emblems before a purchased familiar can be put into them
- Achievements
- Emblems that drop from defeating bosses
- One randomly generated, repeatable stage
- Options menu
- A debug tab (type `debug` in-game to unlock) for faster testing with emblems and gold

## About this project

Built from scratch without a game engine to have direct control over performance, game feel, and loading times. The codebase applies object-oriented design and SOLID principles pragmatically — favoring simple, readable solutions over pattern-for-pattern's-sake abstraction.
