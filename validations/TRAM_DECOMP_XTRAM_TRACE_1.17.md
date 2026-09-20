# TRAM decompressor + XTRAM controller trace — Core 1.17

## Scope

This pass follows Core 1.16's exhaustive proof of the EMU10K1 physical XTRAM raw-word codebook. It attacks two narrower questions: which signed-20 representative is reconstructed from a truncated LOG7 word, and what controller geometry exists beneath the programmer-visible external delay abstraction.

## 1. Decompressor representative

### Source chain

The raw-word side was already established by the stock Linux EMU10K1 raw-IEC958 FX8010 program: after XTRAM has expanded an arbitrary physical 16-bit word, the microcode recovers the original raw word using a 20-bit mask, `LOG(...,7,0)`, upper-16 extraction, and the negative-word `^ 0x7000` correction.

Creative patent US 5,930,158 supplies the missing EXP sign rule. Its decompression procedure first one's-complements a negative LOG operand, reconstructs the positive mantissa with less-significant omitted bits equal to zero, de-normalizes, and then applies the requested sign transform to the linear result.

For a stored negative LOG prefix, zero-fill after the patent's initial one's complement is equivalent to restoring the missing bits of the original negative LOG word as ones. Core 1.16 restored zeros for both signs; Core 1.17 corrects the negative half.

### Result

For physical raw word `c`:

1. if bit 15 is set, undo the physical exponent-polarity transform with `c ^= 0x7000`;
2. place the resulting LOG16 prefix in bits 31..16;
3. fill bits 15..0 with `0x0000` for positive LOG words and `0xffff` for negative LOG words;
4. evaluate the measured `EXP(7,0)` implementation;
5. retain the 20-bit tank domain.

This produces the lower edge of every positive quantization bucket and the upper/toward-zero edge of every negative bucket. That is the sign-symmetric consequence of LOG mantissa truncation plus the patent's one's-complement signed representation.

### Exhaustive proof boundary

`tram_decompressor_patent` independently implements the patent sign path and checks every one of the 65,536 physical words against production code. `tram_raw_spdif_codebook` continues to sweep all 1,048,576 signed-20 values and proves every raw code remains populated and round-trippable.

The largest code bucket contains 64 signed-20 inputs, with maximum span 63 tank LSBs. There are 24,576 multi-value positive buckets and 24,576 multi-value negative buckets.

This is stronger than Core 1.16's symmetric zero-fill assumption. A direct raw-silicon capture would still be useful as an independent confirmation that the dedicated TRAM formatter instantiates the LOG/EXP patent rule exactly, but there is no longer an unexplained choice of fill polarity in the software model.

## 2. TRAM sequencer phase and ALIGN

The E-mu/Creative sound-memory-engine patent describes a separate TRAM sequencer with data/address buffers, an address generator, formatter, DBAC, and per-buffer controls. The TRAM and microinstruction sequencers are restarted together at the sample boundary, but each tank buffer is serviced sequentially at a particular point in the TRAM list.

That architecture explains the loader equations already recovered from the APS driver:

- ITRAM write: `PC >= 3*slot`;
- ITRAM read: `PC <= 3*slot + 1`;
- XTRAM write: `PC >= 128 + 4*slot`;
- XTRAM read: `PC <= 127 + 4*slot`.

ALIGN is therefore a physical +/-1 address correction around the service crossing, not an additional user delay. Core 1.17 preserves the existing sample-level abstraction: reads observe the previous committed cell state for the current sample and writes become visible after microcode.

## 3. External PCI delay-cache geometry

Creative US 6,275,899 gives the specific delay-cache implementation used to hide PCI/main-memory latency:

- PCI burst: 16 samples;
- delay cache: 18 samples;
- 32 caches total;
- 2 caches selected per sample;
- pair selection cycles through 0/1, 2/3, ... 30/31;
- each cache is revisited after 16 samples;
- request FIFO depth: 4;
- a request dropped because the FIFO is full waits for the cache's next 16-sample service opportunity;
- read transfer starts are parity-corrected by -18 (even) or -17 (odd);
- write transfer starts are parity-corrected by 0 (even) or +1 (odd).

Those constants match the otherwise obscure -18, modulo-16, parity and tap-phase operations recovered in the early A0-A3 APS VxD path. Core 1.17 exposes them as diagnostics and tests but does not fold cache refill artifacts into ordinary delay addressing.

### The 64-sample lead

A four-entry service FIFO and 16-sample cache revisit cadence naturally produce a 64-sample controller span. This is conspicuously consistent with the historical 64-sample XTRAM/Freeverb boundary previously observed in the project. It is **not yet proof of causation**. The missing discriminator is a trace showing that the glitch occurs when a cache service request is dropped/deferred or at the matching A0-A3 controller state.

## 4. READ|WRITE / RSAW

Creative US 6,032,235 explicitly describes internal TRAM read-sum-and-write (RSAW): read the addressed sample, add the corresponding data buffer, and write the sum back to the same address, at twice the normal memory bandwidth. This supports the existing `READ|WRITE -> ReadSumWrite` controller-mode decode and CLEAR/RSAW zero-initialization helper.

The patent evidence is strongest for high-bandwidth internal TRAM. Core 1.17 does not invent an external-XTRAM RSAW cache timing sequence beyond the raw control-state representation.

## 5. Frequency Shifter probe

Forcing the recovered Frequency Shifter off its `linearTramCompatibility` path and onto the corrected sign-aware LOG7 decompressor still leaves approximately:

- tail RMS: 0.00287648928201
- tail peak: 0.00406798115

So the known all-pass tail is not explained by Core 1.16's negative zero-fill mistake. The shipped effect retains its explicit compatibility mode while the reconstruction/timing issue remains separate.

## 6. Status after this pass

Solved/strongly constrained:

- physical XTRAM raw 16-bit LOG7 codebook;
- negative physical-word `^0x7000` transform;
- patent-consistent sign-aware EXP reconstruction;
- exact signed-20 preimage bucket for every raw word;
- TRAM sequencer/ALIGN conceptual phase;
- external PCI delay-cache geometry and parity corrections.

Still open:

- direct silicon confirmation of every decompressed representative;
- detailed cache arbitration and exact state transitions when service FIFO pressure occurs;
- whether the historical 64-sample Freeverb glitch is specifically a dropped/deferred service request;
- exact NOISE0 PRNG polynomial/seed/state;
- hardware waveform or register traces for the recovered factory corpus.
