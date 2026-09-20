# FX8010 Accuracy Core 1.12.0 — native Creative SBLFX conformance

This pass adds native-program evidence from the July 1999 Sound Blaster Live! Value `SBLFX.DLL` without pretending that software-only comparison is equivalent to a silicon capture.

## New native corpus

The core now ships the complete 23-program RIFX corpus recovered from that SBLFX snapshot:

- 1,063 original FX8010 microinstructions
- 128 TRAM declarations
- 11 TRAM-using programs (9 ITRAM, 3 XTRAM; one program can use both categories in the corpus totals)
- 14 of the 16 FX8010 opcodes used by Creative's own programs
- direct native references to NOISE0/NOISE1, CCR, DBAC, IRQ and ACCU
- all 23 programs load and execute through the shared interpreter in an integration smoke test

The two opcodes not used by this particular Creative corpus are MACMV (7) and LIMITLT (11); both remain covered by the existing independent hardware-derived/michgz test work rather than being guessed from this corpus.

## Record Dither — NOISE evidence

Creative's 8-instruction `Record Dither` program computes `NOISE0 - NOISE1` separately for both output channels and scales the difference by GPR `0x8005`. The factory/default 16-bit value of that GPR is `0x00010000`, exactly `2^-15` in Q31, or one 16-bit PCM LSB.

This is strong native corroboration of the historical documentation that each noise source is uniform over approximately -0.5..+0.5: subtracting two independent sources gives the triangular distribution expected for TPDF dither, then the Creative host setter scales it to one target PCM LSB.

It does **not** reveal the silicon PRNG polynomial, seed, sequence, the exact meaning of NOISE4096, or conclusively prove whether the generators advance once per sample or between DSP instructions. The current software streams therefore remain deterministic stand-ins. In the present once-per-sample model the two dither channels are correlated because the RIFX repeats the same pair later in the 512-instruction frame; that is deliberately reported as a diagnostic rather than treated as proven hardware behavior.

## Peak Meter — LOG / INTERP / LIMITGE evidence

Creative's native 3-instruction `Peak Meter`:

1. averages its two input patches with `INTERP` and the 0.5 ROM constant;
2. applies `LOG` with exponent 31 / sign mode 1;
3. peak-latches the larger logarithmic code using `LIMITGE`.

The executable regression reproduces the monotonic latch behavior and ends at the expected `0x74000000` logarithmic peak for the test vector.

## AC3 pass-through — CCR / SKIP / DBAC / IRQ / ANDXOR evidence

The 24-instruction `ac3pass` program is a compact native special-register stress case. It contains:

- 3 SKIPs, all taking CCR in A;
- the `0x180` condition word with count 5 in the X/Y operand positions, independently corroborating the emulator's `A=CCR, X=test, Y=count` interpretation;
- 2 writes of the high-bit ROM constant to GPR_IRQ;
- one direct DBAC read;
- 4 ANDXOR and 2 LOG instructions;
- XTRAM accesses.

With the recovered default state the emulator follows the native control flow to the first IRQ request and captures `DBAC + 1 == 1` on the first sample.

## What changed in the accuracy estimate

Most of this pass increases **confidence and coverage** rather than changing arithmetic. The core was already implementing the behavior that Creative's programs expect.

Suggested engineering-confidence ranges after this pass:

- deterministic opcode/arithmetic/output path: **~98.5–99%**
- deterministic non-TRAM, non-NOISE Creative programs: **~99%**
- arbitrary sample-level FX8010 program including TRAM/specials: **~97–98%**, central estimate about **97.5%**
- the eleven recovered user effects at 48 kHz: roughly **~96.5–98%** depending on how TRAM-heavy the effect is
- full EMU10K1 card/effects subsystem: still lower, roughly **~91–94%**, because voice-engine/card scheduling and cycle-level tank behavior are broader than the FX8010 interpreter

These are engineering-confidence estimates, not measured percentages from a real-card sample-by-sample A/B test.

## Remaining blockers to true bit-exactness

1. exact NOISE0/NOISE1 generator, seed/state transition and refresh cadence; NOISE4096 meaning;
2. exact DSP instruction vs TRAM service phase, read visibility and write commit timing;
3. READ|WRITE/RSAW behavior if it has special timing beyond the raw control bits;
4. the historical 64-sample XTRAM/Freeverb boundary;
5. final real-silicon proof of the raw logarithmic TRAM codeword/LOG7 mapping;
6. a small set of guarded-ACCU/CCR boundary cases that still lack direct hardware vectors.
