EMU10K1 FX8010 Emulator

An experimental software implementation of the FX8010 DSP found in Creative Labs / E-mu EMU10K1 audio processors, most famously used by the Sound Blaster Live! family.

The goal of this project is to reproduce the behavior of the original programmable effects processor closely enough to execute authentic Creative DSP microcode and recreate classic Sound Blaster Live! effects on modern systems.

Status

Experimental / partially complete.

The FX8010 portion is substantially implemented and capable of executing recovered Creative and E-mu DSP programs.

This is not currently a complete EMU10K1 hardware emulator. In particular, the EMU10K1 wavetable/SoundFont voice engine, PCI device interface, DMA engine, AC'97 interface, and other surrounding hardware are outside the current implementation or remain works in progress.

No claim of cycle-perfect or bit-perfect emulation is currently made.

Currently implemented

FX8010 instruction execution

67-bit accumulator behavior

CCR/status handling

Conditional SKIP execution

DSP register handling

Operand/result forwarding behavior

ITRAM support

XTRAM support

TRAM delay-line behavior

TRAM compression/decompression behavior

LOG / EXP

INTERP

MAC family operations

ACC family operations

LIMIT

SKIP

CLEAR

ALIGN

RSAW support

NOISE support, with some unresolved hardware details

Creative-style DSP routing

Execution of recovered Creative/APS RIFX effects

Regression and conformance tests

Experimental CLAP/VST3 ports of selected Creative-style effects

Accuracy

The emulator is intended to reproduce normal FX8010 behavior closely enough for real-world DSP programs and audio effects.

A number of obscure hardware details are still being investigated, including:

Exact NOISE0 / NOISE1 generator sequence and timing

Some RSAW overflow and edge-case behavior

External XTRAM arbitration/timing details

PCI/XTRAM completion timing

A small number of numerical edge cases

Exact behavior of undocumented or poorly documented DSP features

Because of these remaining unknowns, the project should currently be considered functionally accurate rather than bit-exact.

Project scope

This project currently focuses on:

FX8010 DSP microcode → audio processing

rather than complete emulation of:

Sound Blaster Live! PCI card → Windows driver → full EMU10K1 hardware

Future work may include the EMU10K1 hardware wavetable/SoundFont synthesizer, voice engine, sample interpolation, envelopes, hardware filter, MIDI playback, PCI-facing registers, DMA, and additional Sound Blaster Live! subsystems.

Why?

The FX8010 was an unusually powerful programmable audio DSP for consumer hardware of its era.

Much of its behavior was either sparsely documented, undocumented, or hidden behind Creative's proprietary drivers and tools.

This project attempts to preserve and reproduce that hardware behavior using surviving documentation, open-source drivers, SDK material, historical software, recovered DSP programs, reverse engineering, and automated testing.

AI-assisted development

Development and reverse-engineering work on this project has been substantially assisted by OpenAI GPT models.

AI assistance has been used for tasks including:

Analysis of historical source code and drivers

Reverse engineering

Comparing independent implementations

DSP arithmetic analysis

Code generation and refactoring

Test generation

Regression analysis

Documentation

Searching for relationships between historical EMU10K1 resources

AI-generated conclusions should not automatically be considered authoritative.

Where possible, behavior is validated against surviving documentation, open-source drivers, Creative/E-mu software, recovered DSP programs, test vectors, and observable hardware behavior.

Contributions

Additional documentation, hardware captures, register traces, DSP programs, driver source code, patents, SDK material, and tests from real EMU10K1 hardware are very welcome.

In particular, information about obscure FX8010 behavior, XTRAM timing, RSAW, and the hardware noise generators would be extremely useful.

Disclaimer

This is an independent preservation and emulation project and is not affiliated with or endorsed by Creative Technology Ltd. or E-mu Systems.

Creative, Sound Blaster, Sound Blaster Live!, E-mu, and related names and trademarks belong to their respective owners.
