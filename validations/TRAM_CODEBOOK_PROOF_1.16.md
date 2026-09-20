# EMU10K1 physical XTRAM codebook proof — Core 1.16

## Result

Core 1.16 separates two questions that older revisions called one "TRAM codec candidate":

1. **Which 16-bit physical word corresponds to a DSP-facing signed-20 value?**
   This mapping/recovery law is now strongly source-proven for EMU10K1 external TRAM by the stock Linux raw-IEC958 DSP program and exhaustively mirrored over the complete signed-20 input domain.
2. **Which signed-20 value does silicon present for each 16-bit physical word?**
   Core uses the natural FX8010 `EXP(7,0)` lower-edge representative. It is mathematically self-consistent and exactly round-trips all raw words, but a direct hardware capture is still needed to prove that silicon chooses that exact point rather than another member of the same quantisation bucket.

This distinction prevents a strong raw-codeword result from being overstated as an unmeasured analog/sample-level hardware result.

## Primary source trace: Linux raw IEC958

Historical `sound/pci/emu10k1/emufx.c` builds an FX8010 program for raw S/PDIF/IEC958. The raw stream is documented as 48 kHz stereo 16-bit little-endian data passed to the digital output without modification, while the data is staged through external TRAM.

The relevant microcode constants are:

```
0xfffff000
0xffff0000
0x70000000
0x00000007
```

For each ETRAM channel the program performs the equivalent of:

```
tmp = ETRAM_DATA & 0xfffff000
log = LOG(tmp, 7, 0)
out = (log & 0xffff0000) ^ 0x70000000
if (out is not negative)
    out = (out & 0xffff0000) ^ 0x70000000
```

The second XOR cancels the first for the positive half. Therefore the net physical-word recovery law is:

```
raw16 = high16(LOG(tank20 << 12, 7, 0))
if (raw16 & 0x8000)
    raw16 ^= 0x7000
```

This exactly matches `Engine::tramRawWordFromTank20()`.

Why this is unusually strong evidence: the raw device must preserve arbitrary 16-bit PCM words. The microcode is not merely shaping normal audio; it is compensating for the hardware TRAM expansion so the original physical 16-bit word can be forwarded unchanged.

## Architectural cross-checks

Contemporary EMU10K1 headers describe the DSP-facing tank-data field as 20 bits and explicitly state that tank audio is logarithmically compressed to 16 bits on TRAM writes and decompressed to 20 bits on reads.

The FX8010 architecture paper likewise describes all EMU10K1 TRAM as physically 16 bits wide with transparent encoding intended to improve effective dynamic range and recursive-effect behavior.

Creative's LOG/EXP patent supplies the matching sign/exponent/mantissa normalization model, implied leading one and low-bit truncation behavior. The raw-IEC958 program is the source that pins the TRAM relation specifically to `LOG(..., 7, 0)` and the negative `0x7000` transform.

## Exhaustive Core 1.16 proof

`tests/tram_raw_spdif_codebook.cpp` does not call the production formatter to derive its expectation. It independently transcribes the Linux `ANDXOR -> LOG7 -> ANDXOR -> MINUS/SKIP -> ANDXOR` sequence.

The test then checks:

- all **1,048,576** signed-20 inputs (`-524288..524287`) against the independently transcribed Linux recovery sequence;
- all **65,536** possible physical 16-bit words are populated by at least one signed-20 input;
- every physical word round-trips through `tramTank20RepresentativeFromRawWord()` and back to exactly the same raw word;
- the EXP7 zero-fill representative equals the lower boundary of every discovered LOG7 bucket;
- maximum bucket population is **64** values;
- maximum bucket span is **63 signed-20 LSBs**;
- explicit positive/negative exponent-transition and full-scale boundary vectors.

Observed result:

```
tram_raw_spdif_codebook PASS: 1048576 signed20 inputs, 65536 raw words,
max bucket=64, max quantisation span=63 LSB
```

Full Core 1.16 result: **63/63 CTest PASS**.

## What changed in the engine

The hardware-oriented path now uses explicit names:

```
tramRawWordFromTank20()
tramTank20RepresentativeFromRawWord()
```

The older names remain wrappers for source compatibility:

```
tramEncode20Candidate()
tramDecode20Candidate()
```

Normal TRAM writes use the source-traced raw-word law. Normal reads use the canonical EXP7 lower-edge representative.

## What is proven vs. still inferred

**Promoted to high confidence / source-traced:**

- DSP-side tank boundary is 20 bits;
- physical external TRAM storage is 16 bits per sample;
- low 12 Q31 bits are absent from the tank value used by the raw path;
- physical codeword recovery uses FX8010 `LOG(7,0)`;
- stored/recovered value is the upper 16 LOG bits;
- negative physical words use the `^ 0x7000` transform;
- the complete raw-word law covers all 65,536 physical codewords.

**Still requires a real-card capture for a literal silicon-bit-exact claim:**

- whether the hardware decompressor returns the exact lower-edge EXP7 representative used by Core 1.16 for every multi-value bucket;
- whether ITRAM's formatter is absolutely identical at every edge case (architecture strongly indicates the same transparent 16-bit encoding, but the raw-IEC958 oracle directly exercises ETRAM/XTRAM);
- pathological low-level XTRAM cache/service timing beneath the programmer-visible delay abstraction.

## Frequency Shifter preservation exception

The recovered Frequency Shifter retains `linearTramCompatibility=true`. Forcing its reconstructed all-pass path through the current hardware-oriented LOG7 decode representative produces a persistent post-burst tail (`~0.00287649 RMS`, `~0.00406798 peak` in the probe used during this pass), failing its established silence-tail regression. That makes it a useful detector for the remaining decode-representative/timing uncertainty rather than a reason to discard the now-proven raw codeword law.

## Source identifiers

- Linux kernel historical source: `sound/pci/emu10k1/emufx.c`, raw IEC958 FX8010 program around the `0xfffff000`, `0xffff0000`, `0x70000000`, `7` GPR setup and ETRAM `LOG` sequence.
- Linux EMU10K1 register documentation: tank data is 20-bit DSP-facing, logarithmically compressed to 16-bit TRAM.
- FX8010 architecture paper (DAFX-98): 16-bit physical TRAM with transparent dynamic-range encoding.
- Creative LOG/EXP patent US 5,930,158: programmable sign/exponent/mantissa normalization and truncation rules.
