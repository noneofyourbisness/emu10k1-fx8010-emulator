# FX8010 Accuracy Core 1.27.0

## 1.27.0 — source-backed TRAM CLR controller

Core 1.27 promotes Creative's documented delay-memory initialization behavior into an executable 160-slot controller/reference model.  READ+CLR returns zero until the global zeroed-samples counter reaches the programmed delay length.  Internal RSAW+CLR writes zero rather than accumulating stale memory and forbids microcode from overwriting the corresponding data-buffer slot while clearing is active.  Equality ends CLR exactly; a new regression checks the READ and RSAW boundaries and counter saturation.

The controller is intentionally separate from normal recovered RIFX `TramDecl` playback because those public patch records do not include the initialization countdown metadata needed to drive it.  This gains host/controller fidelity without inventing an audible change to factory programs.

## 1.26.0 — Creative/kX driver tank-cache lock + mapping epoch

Core 1.26 adds the external-tank mapping lifetime behavior recovered from Creative-derived driver code. The EMU10K1 HCFG register describes `LOCKTANKCACHE` as cancelling tank-cache bus-master accesses. Both the old Creative-origin Linux driver path and ALSA assert that lock before clearing TCB/TCBS and freeing the old coherent DMA allocation, then program the replacement TCB/TCBS and unlock. `XtramServiceFifo` therefore now treats the lock edge as a controller request barrier: outstanding requests are cancelled, requests selected while locked are recorded as lock-suppressed rather than FIFO-overflow drops, and each admitted request carries a TCB/TCBS mapping generation. Rebinding the external tank advances that generation, preventing stale requests from being mistaken for work against the new mapping.

This is deliberately narrower than a cache flush claim. The sources prove cancellation of external bus-master accesses, but do not prove that the chip clears the payload already resident in each 18-sample on-chip cache. Cache contents after rebind remain unknown. `samplesForTcbsCode()` also exposes the 8K..1M-sample TCBS geometry used by the old driver allocation path.

The new `xtram_tankcache_lock` regression raises the standalone suite to **78/78 PASS**. Existing factory audio behavior is unchanged because normal RIFX execution does not manipulate HCFG/TCB/TCBS.

## 1.25.0 — RSAW topology + XTRAM service deadlines

Core 1.25 keeps the interleaved DSP/TRAM sequencer introduced in 1.24 and adds a source-backed internal-TRAM RSAW research path plus stronger external-XTRAM service diagnostics. Creative US 6,032,235 documents internal RSAW as: read the old TRAM value, add the corresponding TRAM data-buffer value, and write the sum back to the same address. `tramInternalRsaw20()` exposes that exact ordering while deliberately returning both signed-20-bit wrap and saturation overflow candidates because the source does not state the overflow law. RSAW CLEAR is modeled as the patent's forced-zero write path.

The XTRAM controller now records, per selected cache, the selection sample, admission/drop result, PCI burst start, the documented cache-headroom deadline (`selected + 2` samples from 18-sample cache minus 16-sample transfer), and next round-robin retry (`selected + 16`). Timestamped completions report whether service finished beyond the documented headroom. A dropped request therefore has a minimum 14-sample interval between the end of guaranteed headroom and its next possible selection; this is a service-risk bound, not an invented audible glitch duration. The one-free-FIFO-slot pair winner and the post-underflow/overflow sample value remain explicit unknowns.

The new `tram_rsaw_exact` and `xtram_service_deadlines` tests raise the standalone suite to **77/77 PASS**. The complete 41-program APS/LiveWare corpus remains unchanged relative to the actual Core 1.24 build: 512-sample smoke aggregate `5722ce973120c049`, and 4096-sample dynamic aggregate `ff418092c64a7213`. (Earlier 1.24 release notes contained stale aggregate strings from an older corpus build; the binaries/tests themselves were unaffected.)

Across the factory audit: 41 programs, 1,792 instructions, 236 TRAM declarations (140 read / 96 write), 19 TRAM-using programs, and 5 XTRAM-using programs. Normal audio execution still does not synthesize guessed PCI corruption or assume that external XTRAM implements the internal RSAW transaction.



Preservation/research interpreter for the original EMU10K1 / FX8010 used by Sound Blaster Live! and E-mu APS.

## 1.23.0 — PCI1024 Surround Mixer host law + surround feed taxonomy

This pass changes evidence/tests, not FX8010 arithmetic. Reverse engineering of the supplied PCI1024 `CTSURMIX.EXE` recovered the exact integer helper logged as `GetBalance Left/Right`:

```text
L > R:  balance = R * max / (2*L)
L < R:  balance = max - L * max / (2*R)
L = R:  balance = max/2
```

`DEVCON32.DLL` stores per-source `Balance` and `Fade` as 32-bit floats under `SourceConfig`; `0x41B85FD9` (about 23.0468) is used as an out-of-normal-range unconfigured sentinel. The shipped `.sea` presets provide real normalized examples. These facts are host-side placement evidence; they do not by themselves prove the full coordinate-to-nine-spatial-send weight function.

Core 1.23 also exhaustively classifies the native PCI1024 `SurroundSpatializer` inputs by isolated impulse response: four unity direct corner channels, four -3 dB single-speaker channels, a long recursive/diffuse common path, a direct 0.5-to-all common path, and a short filtered common path. In `Surround Encoder`, isolated feeds 0/1 are the active left/right encoded paths and feeds 2/3 are silent when driven alone.

Clean validation is **73/73 PASS**. See `SURROUND_HOST_TRACE_1.23.md`.



## 1.22.0 — PCI1024 LiveWare revision/provenance pass

Core 1.22 adds exact identity coverage for the 23 RIFX programs embedded in the supplied PCI 1024 LiveWare 2.5 SBLFX.DLL, keeps that spatializer/reverb generation distinct from the later factory corpus, and documents the recovered DEVCON/Surround Mixer host-send architecture. DSP arithmetic is unchanged from 1.21. See `PCI1024_LIVEWARE_TRACE_1.22.md`.

## 1.21.0 — spatializer host-feed topology

Core 1.21 adds an executable host-feed topology regression for Creative's native headphone/2-speaker/4-speaker spatializer RIFXs. It proves the fixed feed-bank structure used by the host panner (direct endpoints/corners, processed mirrored feed pairs, and common/diffuse paths) while deliberately leaving the exact AudioHQ coordinate-to-send interpolation curve marked as unrecovered. The DSP engine itself is unchanged from Core 1.20. Clean validation is **70/70 PASS**. See `SPATIALIZER_HOST_TRACE_1.21.md`.

## 1.20.0 — PCI tank-state budget + native spatializer signal proof

Core 1.20 leaves the normal audible TRAM path unchanged and tightens two evidence areas. `XtramServiceFifo::timingState()` now exposes the Creative US 6,275,899 `M-B = 18-16 = 2` sample latency budget directly: queued requests at age 0..2 remain inside the patented cache headroom, while older requests are reported as controller-risk states without inventing the exact bad sample a late PCI transaction would produce. `nextSelectionAfter()` locks the patented +16-sample reselection of a dropped cache. Public EMU10K1 register traces expose tank base/size, a tank-cache bus-master lock, and a generic PCI-error interrupt, but no four-entry FIFO identity/age register; Core therefore does not manufacture one or reinterpret the generic PCI-error bit as a tank underflow flag.

The recovered July-1999 Creative spatializers are also exercised as actual signal processors. Isolated-input impulse tests prove that `2HeadphoneSpatializer`, `SB2SpeakerSpatializer`, `SB4SpeakerSpatializer`, `SurroundSpatializer`, and `Surround Encoder` are **not no-op/routing placeholders**. The headphone path generates delayed contralateral energy beginning at 12 samples (0.250 ms at 48 kHz), with another path at 31 samples (0.646 ms); SB2/SB4 processed paths begin opposite-side crossfeed at 8 samples (0.167 ms); the four-speaker network fans one source into all four outputs; SurroundSpatializer contains a long four-output recursive/filter path; and Surround Encoder's principal path begins at 5 samples. These structures are consistent with Creative/E-mu's contemporary virtual-3D/binaural work.

Clean standalone validation is **69/69 PASS**. See `XTRAM_PCI_STATE_TRACE_1.20.md`, `SPATIALIZER_SIGNAL_TRACE_1.20.md`, and `VALIDATION_1.20.txt`. The global arbitrary-sample engineering-confidence range remains approximately **99.1–99.3%**: this pass proves the factory spatializers are functional and narrows controller state, but does not add a silicon PCI-contention capture.

## 1.19.0 — XTRAM FIFO ambiguity + congestion boundary correction

Core 1.19 tightens the four-entry external-TRAM service controller from 1.18 without changing the normal audible delay path. Creative US 6,275,899 is explicit about the source-backed pieces: 32 caches, two selected per sample by a 4-bit round-robin counter, 16-sample service period, 16-sample transfers, 18-sample caches, a four-entry FIFO, FIFO-order retirement, and drop-on-full with the dropped cache waiting until its next 16-sample selection.

Two conclusions are corrected/clarified in this pass:

1. **There is no source-backed 64-sample retry law.** A cache skipped because the FIFO is full is eligible again 16 samples later. The same cache pair will of course also appear at +32, +48, +64, etc., because the service period is 16; FIFO depth 4 does not create a distinct 64-sample cadence. The historical Freeverb/XTRAM 64-sample glitch therefore remains unresolved rather than being attributed to `4 * 16`.
2. **Intra-pair admission with one free FIFO entry is not specified.** The patent names two simultaneously selected caches, but does not say which one wins when both are active and exactly one FIFO slot remains. `admissionEnvelope()` now reports this as `identityAmbiguous`; `select()` accepts an explicit research tie-break (`LowerThenUpper` or `UpperThenLower`) rather than hiding the assumption. The accepted/dropped *counts* are still source-determined.

The controller also exposes the patent geometry as constants: transfer size 16, cache size 18, service period 16, FIFO depth 4, and therefore `M-B = 2` sample periods of stated latency tolerance. A new regression shows that from an empty FIFO with all cache pairs active and no completions, two pair selections fill the four entries and the third pair is the first forced to drop. If two requests are retired each sample, 1,024/1,024 requests are accepted across 512 samples with zero drops. This proves queue overflow is a congestion condition, not an inherent periodic artifact.

Clean standalone validation is **67/67 PASS**. The sample-level TRAM signal path, LOG7 codebook, sign-aware decompressor, instruction arithmetic, and factory corpus are unchanged from 1.17/1.18.

Engineering confidence for arbitrary sample-level original EMU10K1 FX8010 behavior remains approximately **99.1–99.3%**. XTRAM controller semantics are more narrowly bounded, but exact PCI completion/arbitration timing and silicon behavior in the one-free/two-active corner still require hardware evidence.

See `XTRAM_FIFO_TRACE_1.19.md` and `VALIDATION.txt`.

## 1.18.0 — four-entry XTRAM service FIFO controller

Core 1.18 turns the external-delay cache notes from 1.17 into an executable controller model based on Creative US 6,275,899.  The patent's specific EMU10K1-era embodiment is unusually concrete: 32 delay caches, a 4-bit sample counter, cache pairs 0/1 through 30/31 selected in order, one revisit every 16 samples, 16-sample PCI transfers, 18-sample caches, and a four-entry FIFO for active service requests.  If the FIFO is already full when an active cache is selected, that request is ignored and the cache is not selected again for another 16 samples.

`XtramServiceFifo` implements exactly that control law without guessing PCI completion latency.  The caller configures which of the 32 caches are active and whether each is a read or write cache, calls `select(sample)` at the hardware selection point, and explicitly retires completed requests with `completeOne()` / `complete()`.  FIFO ordering, drop-on-full, the 16-sample retry interval, and parity-corrected burst starts are therefore testable independently of the sample-level delay abstraction.

The controller preserves the patent's PCI start-address corrections:

- read current address even: start at current - 18 samples;
- read current address odd: start at current - 17 samples;
- write current address even: start at current;
- write current address odd: start at current + 1 sample.

The new `xtram_fifo_controller` regression proves the selection and queue rules, strict FIFO retirement, full-queue request loss, 16-sample reselection after a dropped request, inactive-cache behavior, and both read/write parity adjustments.  It also runs all 32 caches active for 256 samples while retiring the two selected requests promptly each sample: **512 requests are accepted and completed with zero drops**.

That result narrowed the old 64-sample XTRAM/Freeverb lead, but **Core 1.19 corrects the earlier “64-sample retry span” wording**: a dropped cache is eligible again after 16 samples, not 64. FIFO depth controls outstanding request capacity; it does not multiply the retry period. Core 1.18/1.19 do not inject synthetic cache corruption into normal effects.

Clean standalone validation is **66/66 PASS**.  The normal audible TRAM path is unchanged from Core 1.17; this revision replaces a vague cache hypothesis with a source-backed controller/fault model and removes the temptation to emulate a deterministic glitch that the patent does not support.

Engineering confidence for arbitrary sample-level original EMU10K1 FX8010 behavior remains approximately **99.1–99.3%**, with somewhat higher confidence specifically in XTRAM controller topology.  This is a source-convergence engineering estimate, not a measured silicon waveform score.  Remaining uncertainty is concentrated in exact NOISE0 state generation, PCI completion/arbitration timing under contention, and end-to-end silicon reference renders.

See `XTRAM_FIFO_TRACE_1.18.md` and `VALIDATION.txt`.

## 1.17.0 — decompressor sign reconstruction + XTRAM cache geometry

Core 1.17 narrows two of the largest remaining TRAM uncertainties without pretending to have a new silicon capture.

### Patent-consistent LOG7 decompressor representative

Core 1.16 proved the physical 16-bit XTRAM raw-word law by reproducing the stock Linux raw-IEC958 recovery sequence over every signed-20 input and every 16-bit physical codeword. The remaining question was how the hardware EXP side reconstructs the discarded low LOG bits.

Creative US 5,930,158 specifies the EXP sign path: for a negative LOG operand the whole LOG word is one's-complemented first, the positive mantissa is reconstructed with omitted low bits filled with zero, the value is de-normalized, and the final linear result is complemented back. Therefore a truncated negative LOG word must restore its missing encoded low bits as **ones**, not zeros.

`tramTank20RepresentativeFromRawWord()` now follows that sign path. Positive raw words decode to the lower edge of their exact signed-20 preimage bucket; negative raw words decode to the upper/toward-zero edge. `tramTank20BucketFromRawWord()` exposes the complete preimage interval separately so the mathematical codebook and the chosen decompressor representative are not conflated.

The new `tram_decompressor_patent` test independently implements the patent procedure and verifies all **65,536 raw physical words**. The raw-code test still sweeps all **1,048,576 signed-20 inputs**. There are 24,576 multi-value buckets on each sign; every raw code round-trips.

### External-XTRAM cache/service geometry

Creative US 6,275,899 and the recovered early-revision `EMUAPS.VXD` correction agree on the external-delay cache geometry used by the EMU10K1-era PCI implementation:

- 16 samples per PCI burst;
- 18 samples per delay cache (16 payload + latency allowance);
- 32 delay caches;
- two caches selected per sample in round-robin pairs;
- each cache pair revisited every 16 samples;
- four queued service requests;
- aligned read-burst starts at current-18 for even addresses and current-17 for odd addresses;
- aligned write-burst starts at current+0 for even addresses and current+1 for odd addresses.

The `xtram_cache_geometry` test locks those controller facts into explicit diagnostics. **Historical note:** 1.17 initially called `4 FIFO entries × 16 samples` a 64-sample structural lead. Core 1.19 supersedes that interpretation: the patent specifies a 16-sample reselection after a dropped request, so the old Freeverb/XTRAM 64-sample observation remains unresolved.

The sound-memory-engine patents also strengthen the existing ALIGN interpretation: the TRAM sequencer and 512-instruction DSP program restart in lockstep each sample, while each tank buffer is serviced at its own point in the independent TRAM list. ALIGN corrects the physical address by one sample when the DSP accesses the buffer on the opposite side of that service point. The sample-level interpreter therefore keeps its existing programmer-visible read-before-write abstraction rather than double-applying ALIGN.

### Validation and confidence

Clean standalone validation is **65/65 PASS**. Core 1.17 does not change ordinary instruction arithmetic or the proven raw XTRAM codeword law.

Engineering confidence for arbitrary sample-level EMU10K1 FX8010 behavior is now approximately **99.1–99.3%**. This is a source-convergence engineering estimate, not a measured percentage of bit-identical samples against a physical card. The remaining uncertainty is concentrated in exact NOISE0 sequence/seed, complete external-cache arbitration/missed-service behavior, and end-to-end hardware reference renders.

See `TRAM_DECOMP_XTRAM_TRACE_1.17.md` and `VALIDATION.txt`.

## 1.16.0 — raw XTRAM codebook proof pass

This pass promotes the **physical 16-bit external-TRAM codeword law** from a research candidate to a source-traced, exhaustively checked reconstruction. The decisive evidence is the stock Linux EMU10K1 raw-IEC958 FX8010 program: raw 48 kHz stereo 16-bit words are buffered through ETRAM even though the hardware automatically expands logarithmic TRAM on read, so the microcode explicitly recovers the original physical word with `ANDXOR`, `LOG(..., 7, 0)`, the upper 16 bits, and a negative-word `^ 0x7000` correction.

Core 1.16 names that operation `tramRawWordFromTank20()` and uses it on normal TRAM writes. The old `tramEncode20Candidate()` API remains as a source-compatibility alias. A new independent test transcribes the Linux instruction sequence literally and proves equality over **all 1,048,576 signed 20-bit input values**.

The reverse direction is now named `tramTank20RepresentativeFromRawWord()`. It undoes `^ 0x7000`, applies the measured `EXP(7,0)` law with omitted LOG bits restored as zero, and therefore chooses the **lower boundary** of each LOG7 quantisation bucket. Exhaustive tests prove that all **65,536 physical raw words** are populated and round-trip exactly through this representative; the largest bucket contains 64 signed-20 values, so the maximum within-bucket span is 63 tank LSBs. This is a mathematical/codebook proof, not a claim that a direct silicon capture has yet shown which member of every multi-value bucket the hardware decompressor presents.

The standalone suite is now **63/63 PASS**. No previous factory-corpus, opcode, patent, TRAM-control or NOISE-topology regressions were lost.

### Accuracy assessment after the codebook proof

The physical XTRAM **codeword format/recovery law** is no longer counted as a broad unknown. Engineering confidence for arbitrary sample-level EMU10K1 FX8010 behavior is raised modestly from about **98.8–99.0% to about 99.0–99.2%**. This remains an engineering confidence range rather than a measured silicon waveform-match percentage. The remaining uncertainty is now concentrated in the exact hardware decompressor representative inside each LOG7 bucket, NOISE0's generator state transition/seed, and low-level XTRAM cache/service phase.

See `TRAM_CODEBOOK_PROOF_1.16.md` for the proof boundary and exhaustive results.

## 1.15.0 — APS/LiveWare factory-corpus conformance pass

This pass folds the recovered E-mu APS 1.5 and Creative LiveWare factory-effect evidence into the standalone core. It deliberately targets EMU10K1 only; Audigy/EMU10K2 behavior is not used as a bit-exact authority.

The standalone suite is now **62/62 PASS**.

### Complete recovered factory corpus

The test fixtures now include:

- 15 E-mu APS 1.5 `EMU8010_A0` RIFX programs recovered from `EAPSFX.DLL`;
- 26 Creative LiveWare `EMU8010_A4` RIFX programs recovered from `SBLFX.DLL`;
- 41 factory programs total;
- 1,792 FX8010 microinstructions;
- 236 active TRAM declarations: 140 reads and 96 writes;
- 19 TRAM-using programs, of which 5 use XTRAM;
- 15 of the 16 opcodes exercised by the factory corpus. The absent opcode is MACMV, already covered by the earlier Creative tone-control program and standalone regression.

No factory instruction in this corpus uses more than one direct physical input register (0x00..0x20) in A/X/Y, matching the old warning that this is an undefined silicon case.

### RIFX resource fix

`rsrc[4]` and `rsrc[5]` are allocated ITRAM/XTRAM pair-resource counts, not both counts of active `tram` records. Factory programs may reserve unused/OFF ITRAM pairs as pseudo-GPR storage.

The loader now classifies active declarations as:

- external active declarations = the final `min(rsrc[5], tram_count)` records;
- all preceding active declarations = internal.

This fixes a real parsing error in APS Everb. Everb allocates 25 ITRAM pairs and 28 XTRAM pairs, but its 50 active declarations are **22 ITRAM + 28 XTRAM**; the three extra ITRAM pairs are not active delay taps.

### E-mu APS loader ALIGN evidence

The controller-level `tramAlignFlag()` equations are now attributed to the recovered E-mu APS 1.5 `EMUAPS.VXD`, where they were traced directly from the loader:

- ITRAM result/write: `PC >= 3*tap`
- ITRAM input/read: `PC <= 3*tap + 1`
- XTRAM result/write: `PC >= 128 + 4*tap`
- XTRAM input/read: `PC <= 127 + 4*tap`

The calculation is relocation-dependent: E-mu computes it from the final physical instruction PC and final physical tank slot.

The sample-level interpreter still performs programmer-visible current-frame tank reads before microcode and commits writes after microcode; it does not apply ALIGN a second time.

### NOISE topology status (superseded evidence note)

Earlier Core 1.15 notes treated a 4097-sample `NOISE0 -> NOISE1` delay as documented. The surviving public material checked in the later driver/patent passes does **not** independently corroborate that topology, and the old manual's independence wording is itself presented as a guess. Core retains the 4097 relation only as a compatibility hypothesis so existing effect behavior is reproducible; it is not claimed as proved silicon behavior.

The exact EMU10K1 NOISE generator polynomial, seed/reset state, bit sequence, and the precise NOISE0/NOISE1 relationship remain hardware-capture targets. The current deterministic source/delay model is explicitly not presented as silicon-exact.

### Factory execution tests

Two whole-corpus execution tests are included:

- zero/default smoke: every factory program executes 512 samples twice with identical result state;
- dynamic stress: every factory program executes 4,096 samples of deterministic changing nonzero input twice with identical result state.

Current reference aggregates:

- zero/default aggregate: `78c006743aae0b95`
- dynamic aggregate: `b66f345ebbef618a`

Nine programs hit legitimate arithmetic saturation during the deliberately hot first 64 samples of the dynamic stress run; saturation is diagnostic and not treated as failure.

## New external-XTRAM architectural trace

Reverse engineering of the early-revision `EMUAPS.VXD` XTRAM correction now lines up with Creative patent US 6,275,899 (Savell/Hoge): the external delay engine uses 16-sample PCI bursts and 18-sample read/write caches, with PCI transfer-start corrections around -18/-17 samples for reads and 0/+1 for writes depending on alignment.

This explains the otherwise mysterious -18, modulo-16, parity and tap-phase arithmetic in the A0-A3-only VxD correction block. It is controller/cache behavior beneath the abstract delay-line address, so it is documented rather than forced into the sample-level tank memory where doing so could double-compensate the hardware workaround.

## Accuracy assessment

The recovered factory corpus substantially raises confidence in the deterministic instruction/resource path:

- instruction decoding/arithmetic/control/resource relocation: approximately **99% engineering confidence**;
- recovered factory-program structural/functional execution: approximately **99% engineering confidence**;
- arbitrary sample-level EMU10K1 FX8010 including tank/noise behavior: approximately **99.0–99.2% engineering confidence**, not a measured silicon A/B percentage.

A literal bit-exact 99% claim is still withheld because the remaining uncertainty is concentrated in a few silicon-only details rather than broad DSP behavior.

## Still not claimed solved

1. Exact silicon decompressor representative within each proven LOG7 code bucket. The physical 16-bit XTRAM codeword/recovery law is now source-traced and exhaustive; direct raw-silicon capture is still required to prove which signed-20 member of every multi-value bucket the hardware presents on read.
2. Exact NOISE0 generator polynomial/state/seed and the exact NOISE0/NOISE1 relationship. The 4097-sample relation is retained only as a compatibility hypothesis, not independently proved silicon behavior.
3. Full low-level A0-A3 external-TRAM cache/burst state, including pathological underflow/overflow/missed-service behavior. The VxD/patent geometry is now understood, but the normal sample-level effect interpreter intentionally abstracts it.
4. Rare hardware-only pathological cases such as undefined multi-physical-input instructions and exact debug single-step timing.
5. End-to-end real-card reference renders for the factory programs. Deterministic execution is not a substitute for a hardware waveform oracle.

For preservation work, **48 kHz remains the reference rate**.
