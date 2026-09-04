# Project Gracemeria

Project Gracemeria is a native recompilation project for **Ace Combat 6: Fires of Liberation**, targeting modern operating systems and hardware.

The project is intended to replace as much Xbox 360-specific runtime behaviour as practical with native host implementations, including native graphics rendering rather than relying on a full Xbox 360 GPU emulator.

Project Gracemeria is built on top of a separate modular Xbox 360 recompilation runtime so that shared functionality can eventually be reused by other Xbox 360 recompilation projects.

## Current Status

Project Gracemeria is currently in very early development.

At this stage, the project is focused on:

- Defining the overall architecture.
- Supporting both the NTSC-U and PAL releases of Ace Combat 6.
- Designing a modular recompilation/runtime interface.
- Establishing native rendering infrastructure.
- Supporting original game content, title updates, and DLC.
- Providing an explicit offline mode while online functionality is not yet implemented.

The project is not currently playable.

## Planned Platforms

The long-term target platforms are:

- Windows x86-64
- Windows ARM64
- Linux x86-64
- Linux ARM64
- Android ARM64

Android support is planned and should be considered experimental until the core desktop versions are functional.

## Graphics

The project is designed around native GPU rendering.

The primary planned graphics backend is:

- Vulkan

Additional backends may be added where useful, including:

- Direct3D 12 on Windows

The renderer will be designed specifically around the behaviour required by Ace Combat 6 rather than implementing a complete Xbox 360 GPU emulator.

## Supported Game Versions

Planned support includes:

- Ace Combat 6 NTSC-U / North American release
- Ace Combat 6 PAL / European release

Version-specific executable information, symbols, patches, and signatures will be kept separate from shared game logic.

Region-specific addresses should not be hardcoded throughout the main codebase.

## DLC and Game Content

Project Gracemeria is intended to support:

- Base game content
- Title updates
- Downloadable content
- Additional aircraft
- Aircraft skins
- DLC missions
- Other supported original game content

The project will not distribute copyrighted game assets.

Users will be required to provide their own legally obtained game files and downloadable content.

## Networking

Xbox Live functionality will initially operate in an explicit offline mode.

The initial implementation is expected to provide local functionality where required by the game while disabling unsupported online services such as:

- Matchmaking
- Multiplayer sessions
- Friends
- Presence
- Online authentication

The project is being designed so that these services can later be connected to a native replacement networking system without requiring major changes to the game-specific code.

## Architecture

Project Gracemeria is intended to contain only Ace Combat 6-specific functionality.

Shared Xbox 360 recompilation functionality belongs in the generic runtime project.

The intended dependency direction is:

Project Gracemeria
        |
        v
Generic Xbox 360 Recompilation Runtime
        |
        v
Platform / CPU / Graphics Backends

The generic runtime must not depend on Project Gracemeria.

## Development Philosophy

The project aims to remain highly modular.

Major systems should be isolated behind clear interfaces, including:

- CPU recompilation
- Graphics
- Filesystem
- Audio
- Input
- Xbox services
- Networking
- Game version support
- DLC handling
- Platform-specific functionality

Platform-specific behaviour should remain isolated from shared game code wherever possible.

## Legal

Project Gracemeria is an unofficial fan-made project.

It is not affiliated with, endorsed by, or sponsored by Bandai Namco Entertainment, Microsoft, Project Aces, or any related rights holders.

Ace Combat, Ace Combat 6: Fires of Liberation, Xbox, Xbox 360, and related names and assets are trademarks and copyrights of their respective owners.

No copyrighted game data is included in this repository.
