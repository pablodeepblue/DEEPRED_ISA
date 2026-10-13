# DEEPRED ISA

A 32-bit, ARM LEGv8-inspired instruction set architecture (ISA) simulator written in C++17. It was built as the final project for **CS535** by James Dolle and Pablo Ghezzi.

## Table of contents

- [Overview](#overview)
- [Project status](#project-status)
- [Repository layout](#repository-layout)
- [Building](#building)
- [Running the memory/cache simulator](#running-the-memorycache-simulator)
- [Architecture reference](#architecture-reference)
- [Memory and cache design](#memory-and-cache-design)
- [PPU (graphics)](#ppu-graphics)
- [Planned benchmarks](#planned-benchmarks)
- [Documentation](#documentation)
- [License](#license)

## Overview

DEEPRED is a general-purpose, signed-integer ISA heavily based on ARM LEGv8, simplified for simulation:

- 32-bit instructions and a 32-bit word size
- 16 registers (13 general purpose, stack pointer, program counter, zero register)
- Byte-addressed Princeton-style memory of 64 KB (16-bit addresses) with byte and word load/store
- A direct-mapped, write-back L1 cache in front of RAM
- A planned PPU (picture processing unit) inspired by the NES, outputting 256 x 224 pixels

## Project status

| Component | Status |
|---|---|
| RAM model with access latency and blocking (`src/memory.h`) | Implemented |
| Bus (address decoding to line/word/byte) | Implemented |
| L1 direct-mapped cache with miss handling and write-back (`src/memory.h`) | Implemented |
| Interactive memory/cache driver (`src/memdrive.cpp`) | Implemented |
| CPU shell with cycle counter (`src/cpu.h`) | Skeleton only (no fetch, decode or execute yet) |
| Main simulator entry point (`src/main.cpp`) | Stub (returns immediately) |
| PPU and tile renderer using SDL3 (`not_used/ppu.h`) | Prototype, not part of the build |
| Tetris demo using the PPU (`not_used/tetris_demo.h`) | Prototype, not part of the build |
| Assembler, pipeline, Qt GUI | Described in the report, not in this repo |

## Repository layout

```
DEEPRED_ISA/
├── src/
│   ├── main.cpp          # Simulator entry point (currently a stub)
│   ├── cpu.h             # CPU class: cycle counter + L1 cache (skeleton)
│   ├── memory.h          # Memory, Bus and Cache_lv1 models
│   ├── memdrive.cpp      # Interactive command-line driver for memory/cache
│   └── isadefines.h      # Type aliases, screen constants, NES color palette
├── not_used/
│   ├── ppu.h             # SDL3-based PPU prototype (sprites, nametables, palettes)
│   └── tetris_demo.h     # Demo that drives the PPU with NES-style tile data
├── include/SDL3/         # SDL3 headers
├── lib/                  # SDL3 CMake and pkg-config files
├── chr_rom.bin           # Tile (CHR) data used by the PPU demo
├── game_nametable_fixed.bin  # Background nametable used by the PPU demo
├── SDL3.dll              # SDL3 runtime for Windows
├── CS535_Final_Report_Draft.pdf  # Full ISA specification and design report
├── Makefile
└── LICENSE
```

## Building

### Requirements

- A C++17 compiler (the Makefile uses `g++`)
- `make`

The current build does not use SDL3. SDL3 is only needed by the PPU prototype in `not_used/`.

### Commands

```bash
# Build the (stub) simulator at build/main
mkdir -p build
make

# Build and run the stub simulator
make run

# Build the interactive memory/cache driver at ./memdrive
make memory_manager      # or: make memdrive
```

Compiler flags: `-Wall -Wextra -Wshadow -O3 -std=c++17`.

## Running the memory/cache simulator

`memdrive` lets you issue loads and stores one at a time and advance the clock manually, so you can watch latency, blocking and cache behavior.

```bash
./memdrive          # RAM only, accessed through the bus ("Mem mode")
./memdrive cache    # L1 cache in front of RAM ("Cache mode")
```

At the `>>` prompt, numbers can be decimal or hex (`0x...`). Commands are case-insensitive.

| Command | Arguments | Description |
|---|---|---|
| `lda` | `<address>` | Request the line containing `address` into the buffer. Reports `blocked` if the memory system is busy. |
| `buff` | none | Read the buffered line (once ready) and print the word selected by the last `lda`. Prints `Buffer not loaded yet` if the fetch has not completed. |
| `str` / `sta` | `<data> <address>` | Store a 32-bit word. |
| `stb` | `<data> <address>` | Store a single byte. |
| `cycle` | `<n>` | Advance the clock by `n` cycles. |
| `hlt` | none | Exit. |

Every command also advances the clock by one cycle, and the current cycle count is printed before each prompt.

Example session:

```
$ ./memdrive cache
Cache mode
Cycle: 0
>> sta 1234 0x40
1234 store to [64] buffered
Cycle: 1
>> cycle 300
Cycle: 301
>> lda 0x40
...
>> buff
final extracted val 1234
```

Exact output depends on the current cache state and timing.

## Architecture reference

### Registers

| Name | Number | Use |
|---|---|---|
| X0 to X12 | 0 to 12 | General purpose |
| X13 (SP) | 13 | Stack pointer |
| X14 (PC) | 14 | Program counter |
| XZR | 15 | Zero register |

Condition flags (NZVC): **N** negative, **Z** zero, **V** overflow, **C** carry.

### Instruction formats

All instructions are one 32-bit word. The top three bits select the format.

| Format | Bits [31:29] | Fields |
|---|---|---|
| H (halt) | `000` | Any invalid instruction halts |
| R | `001` | Opcode, Rm, shamt, Rn, Rd |
| I | `010` | Opcode, ALU immediate, Rn, Rd |
| D | `010` | Opcode, DT address, Rn, Rt |
| B | `011` | Opcode, branch address |
| CB | `100` | Opcode, conditional branch address |
| IW | `111` | Opcode, move immediate, SH, Rd |
| ME | `110` | Opcode, immediate / Rt |

> The report's format table and per-instruction listings differ slightly in a few places (for example, I and D share `010`, and B/CB/IW/ME encodings vary between sections). Treat `CS535_Final_Report_Draft.pdf` as the working source and verify encodings there before building an assembler or decoder.

### Instruction set

**Arithmetic and logic (R format)**

| Mnemonic | Operation | Flags |
|---|---|---|
| `ADD` | `Rd = Rn + Rm` | N Z V C |
| `SUB` | `Rd = Rn - Rm` | N Z V C |
| `MUL` | `Rd = Rn * Rm` | N Z V C |
| `DIV` | `Rd = Rn / Rm` | N Z V C |
| `MOD` | `Rd = Rn % Rm` | N Z V C |
| `AND` | `Rd = Rn & Rm` | N Z |
| `ORR` | `Rd = Rn \| Rm` | N Z |
| `EOR` | `Rd = Rn ^ Rm` | N Z |
| `LSL` | `Rd = Rn << shamt` | Z V C |
| `LSR` | `Rd = Rn >>> shamt` | Z V C |
| `BR` | `PC = R[Rt]` | none |

**Immediate arithmetic and logic (I format):** `ADDI`, `SUBI`, `ANDI`, `ORRI`, `EORI`

**Load/store (D format)**

| Mnemonic | Operation |
|---|---|
| `LDUR` | `Rt = M[Rn + DTAddr]` (word) |
| `LDURB` | `Rt = zero-extended byte at M[Rn + DTAddr]` |
| `STUR` | `M[Rn + DTAddr] = Rt` (word) |
| `STURB` | store low byte of `Rt` |

**Branches**

| Mnemonic | Condition |
|---|---|
| `B` | Unconditional, PC-relative |
| `BL` | Branch with link (return address saved) |
| `BEQ` | `Z == 1` |
| `BNE` | `Z == 0` |
| `BLT` | `N != V` |
| `BLE` | `Z == 1` or `N != V` |
| `BGT` | `Z == 0` and `N == V` |
| `BGE` | `N == V` |

For conditional branches, set the flags first (for example `CMP A, B`), then branch on the condition.

**Wide moves (IW format):** `MOVK` (keep other bits), `MOVZ` (zero other bits)

**Miscellaneous (ME format):** `PPU` (call the PPU), `FLU` (flush cache), `WAI` (wait)

**Aliases and macros (assembler text replacement)**

| Name | Expands to |
|---|---|
| `NOP` | no operation |
| `CMP Xn, Xm` | `SUB XZR, Xn, Xm` |
| `CMPI Xm, IMM` | `SUBI XZR, Xm, IMM` |
| `MOV Xd, Xn` | `ORR Xd, XZR, Xn` |
| `LDA Xd, IMM` | `ORRI Xd, XZR, IMM` |
| `NOT Xd, Xn` | `EORI Xd, Xn, -1` |
| `FOR` / `IF` | Macros that expand to compares and branches. Nesting is supported. Compare register to value or register to register (not value to value). |

### Assembler directives

| Directive | Meaning |
|---|---|
| `Label:` | Marks a location in `.text` or `.data` |
| `.text` | Instruction section |
| `.data` | Data section (placed after instructions in the binary) |
| `.padding` | Aligns sections to cache lines to avoid instruction/data conflicts |

### ISA modes

The design calls for four run modes, selectable from the GUI: (1) pipeline and cache disabled, (2) memory only, (3) pipeline only, (4) cache and pipeline enabled.

## Memory and cache design

### Implemented in code (`src/memory.h`)

| Parameter | Value |
|---|---|
| RAM size | 65,000 bytes (`MAXMEMORY`) |
| Line size | 8 words (32 bytes) |
| RAM access latency | 100 cycles (RAM is blocked while busy) |
| L1 cache | Direct-mapped, 16 lines of 8 words |
| Address split | tag = `addr >> 9`, index = bits [8:5], word offset = bits [4:2], byte offset = bits [1:0] |
| Cache states | `cache_hit`, `cache_miss`, `cache_fetch`, `cache_wait`, plus `blocked_error` (`0xFF`) |

Accesses are non-blocking requests that return a status immediately. If the unit is busy, the call returns `blocked_error` and the caller must retry on a later cycle. Dirty lines are written back to RAM through the bus.

### Specified in the report

The report targets a larger configuration than the current code:

- 64 KB byte-addressed RAM (16-bit addresses)
- Harvard-style split caches: 1 KB instruction cache and 1 KB data cache
- Each cache direct-mapped with 32 lines of 8 words, write-back
- Cache address: 6-bit tag, 5-bit index, 3-bit word offset, 2-bit byte offset, plus valid and dirty bits
- Optional second cache level as a stretch goal

## PPU (graphics)

The PPU prototype in `not_used/ppu.h` follows the NES model and renders to a 256 x 224 window (scaled 4x) using SDL3.

- 64 sprites in OAM (4 bytes each: Y, tile, attributes, X) with horizontal/vertical flip
- Two nametables (32 x 30 tile labels) with attribute tables
- Background and foreground palettes of 4 colors each, indexing the NES palette in `isadefines.h`
- Two 4 KB tile banks (8x8 tiles, 16 bytes each) loaded from `chr_rom.bin`
- X/Y scrolling

`not_used/tetris_demo.h` loads `game_nametable_fixed.bin` and `chr_rom.bin` (CHR data at offset 12288) to render a background. It uses Windows-style paths (`.\\chr_rom.bin`) and the bundled `SDL3.dll`. Building it requires SDL3 and adjusting the Makefile and include paths; it is not built by default.

## Planned benchmarks

- **Matrix multiplication** (starting at 2x2 and 3x3, scaling up to around 50x50) to fill the cache and exercise misses, with results checked against the known answer
- **Exchange sort** over a large set of numbers, verified by inspecting the final ordering
- **Graphical benchmark**: write an image's bytes into the PPU memory region and confirm it displays correctly
- Metrics: cache hit rate, CPI, final clock count and execution time

## Documentation

`CS535_Final_Report_Draft.pdf` contains the complete ISA specification: instruction encodings, register conventions, assembler directives, cache layout, benchmark plan, GUI design (Qt, with the simulation on a separate thread) and project management notes.

## License

Released under the [MIT License](LICENSE). Copyright (c) 2025 pghezzi.
