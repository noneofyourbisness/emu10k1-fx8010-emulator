# FX8010 Core 1.15 — APS/LiveWare factory-corpus trace

## Sources recovered

This pass uses original EMU10K1-era binaries recovered during the project:

- E-mu APS 1.5 `EAPSFX.DLL` — 15 embedded `RIFX/PTXT` factory programs targeting `EMU8010_A0`.
- Creative LiveWare `SBLFX.DLL` from the recovered digital-output package — 26 embedded factory RIFX programs targeting `EMU8010_A4`.
- E-mu APS 1.5 `EMUAPS.VXD` — original hardware loader/controller path.
- Contemporary EMU10K1 `emu-tools`/`dsp.txt` documentation and Creative patent material.

No Audigy/EMU10K2 behavior is used as a bit-exact authority in this pass.

## Factory corpus totals

The combined corpus contains 41 programs, 1,792 instructions and 236 active TRAM declarations.

Opcode histogram (`0x0..0xF`):

```
1051, 259, 2, 4, 133, 13, 10, 0,
4, 7, 8, 1, 11, 3, 279, 7
```

Thus 15/16 FX8010 opcodes are exercised by the two factory DLLs. `MACMV` is the only absent factory-DLL opcode; the earlier Creative Linux tone-control microprogram uses it and the core has dedicated MACMV/67-bit-ACCU regressions.

Special-register exposure after RIFX relocation:

- NOISE0/NOISE1: two programs (the APS and LiveWare copies of Record Dither);
- DBAC: one program (`ac3pass`);
- IRQ: one program (`ac3pass`);
- CCR: five programs;
- ACCU: eleven programs.

Nineteen programs use TRAM and five use XTRAM.

## Factory undefined-case audit

Old EMU10K1 documentation warns that consuming more than one direct physical input register in one instruction can give undefined behavior.

Across all 1,792 recovered factory instructions, **zero** instructions use more than one register in the `0x00..0x20` direct-input range across A/X/Y. Creative/E-mu's production compiler therefore avoids this case entirely.

## RIFX TRAM resource correction

The 26-byte `rsrc` record reports allocated access-pair resources. Allocated internal pairs may exceed active `tram` declarations because OFF-mode internal tank buffers can be used as persistent pseudo-GPR resources.

The active `tram` records are ordered internal first, external last. Across the full recovered corpus, the number of active external records equals `rsrc[5]` exactly.

The smallest unambiguous failure of the old interpretation is APS `Everb`:

- allocated ITRAM pairs: 25;
- allocated XTRAM pairs: 28;
- active `tram` records: 50;
- actual active domains: 22 ITRAM + 28 XTRAM.

Using `rsrc[4]` as the active ITRAM-record count incorrectly classified three XTRAM taps as internal. Core 1.15 fixes this.

## ALIGN validation from EMUAPS.VXD

E-mu's own APS loader resolves the RIFX local instruction reference to the final relocated physical PC, resolves the virtual tank resource to a physical tank slot, and only then computes ALIGN.

Recovered equations:

```
ITRAM write/result: ALIGN = PC >= 3*tap
ITRAM read/input:   ALIGN = PC <= 3*tap + 1

XTRAM write/result: ALIGN = PC >= 128 + 4*tap
XTRAM read/input:   ALIGN = PC <= 127 + 4*tap
```

This supersedes the unfinished open-source XTRAM `set_tram_align()` implementation. The old code's second `if/else` overwrote the first write result and did not implement the read case, causing large disagreement with the APS loader.

ALIGN is therefore a **relocation-dependent controller property**, not a permanent flag intrinsic to the source effect.

## Early-revision external TRAM correction

`EMUAPS.VXD` contains an additional block enabled for `EMU8010` revisions A0-A3 and physical XTRAM slots `0x380..0x39f`.

The block contains the characteristic constants and operations:

- read-side term around `-18` samples;
- write-side near-zero/+one term;
- tap/2 phase contribution;
- signed modulo 16;
- odd/even alignment correction.

Creative patent US 6,275,899 by Thomas Savell and Stephen Hoge describes the corresponding external-delay architecture:

- host/main-memory delay lines;
- 32 read/write delay caches in the specific embodiment;
- 16-sample PCI burst transfers;
- 18-sample cache length;
- approximately 16-sample servicing interval;
- two caches considered for service per sample;
- read transfer starts adjusted by -18/-17 samples;
- write transfer starts adjusted by 0/+1 samples to align to the 32-bit PCI bus.

The patent constants line up with the VxD code closely enough to identify the block as an external-delay-cache/burst compensation path, rather than part of the abstract DSP-visible delay length.

For that reason Core 1.15 documents this behavior but does not blindly alter the high-level sample delay by those correction values.

## NOISE correction

The mature contemporary EMU10K1 `dsp.txt` describes:

```
0x58 = uniform white noise, approximately -0.5..+0.5
0x59 = 4097-sample delayed copy of 0x58
```

The factory Record Dither program consumes both sources and forms their difference, consistent with using a delayed correlated pair to obtain dither with a different distribution.

Core 1.15 therefore replaces the two-independent-placeholder topology with:

```
NOISE0[n] = placeholder_source[n]
NOISE1[n] = placeholder_source[n-4097]
```

The exact placeholder source is still deliberately non-authoritative: the real silicon polynomial/state/seed has not been recovered.

## Complete execution validation

All 41 factory programs are parsed, relocated and executed.

Smoke run:

- 512 samples/program;
- repeated twice;
- deterministic aggregate `78c006743aae0b95`.

Dynamic run:

- every exposed input is relocated onto synthetic GPRs;
- deterministic changing bipolar input is supplied;
- 4,096 samples/program;
- repeated twice;
- aggregate `b66f345ebbef618a`;
- nine programs legitimately hit the saturation latch during deliberately hot input.

These are software determinism/conformance tests. They are not a substitute for matching rendered audio against an actual SB Live!/APS card.

## What this pass resolves versus what remains

### Strongly resolved / substantially strengthened

- complete factory RIFX parsing for the recovered corpus;
- active ITRAM/XTRAM classification bug;
- instruction/resource relocation across 41 production effects;
- EMUAPS-loader ALIGN equations and boundary inclusivity;
- relocation-dependent nature of ALIGN;
- external XTRAM cache/burst architecture underlying the early-revision correction;
- NOISE0/NOISE1 single-stream delayed-pair topology;
- production coverage of 15/16 opcodes, with the sixteenth covered by older Creative code.

### Still prevents a literal silicon-bit-exact 99% claim

1. Raw physical TRAM formatter codewords. The current signed-20/log16 LOG7 model remains a strong candidate but still lacks a raw-cell capture from EMU10K1.
2. Exact NOISE source sequence. The topology and amplitude are constrained; the polynomial/seed/state transition are not.
3. Pathological external-cache service misses/underflow/overflow and exact early-revision controller state, if full card/controller emulation rather than effect-level behavior is required.
4. End-to-end real-card waveform captures for the factory corpus.

## Current engineering-confidence estimate

After this pass it is reasonable to describe:

- ordinary deterministic FX8010 instruction/resource behavior as **about 99%**;
- the recovered factory software execution model as **about 99%**;
- arbitrary sample-level EMU10K1 FX8010 including unresolved physical TRAM/noise details as **about 98.8–99.0%**.

This is an engineering-confidence estimate based on converging primary binaries, contemporary source, patents and regression coverage—not a measured percentage of identical output samples against silicon.
