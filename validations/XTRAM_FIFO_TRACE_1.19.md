# EMU10K1 four-entry XTRAM FIFO trace — Core 1.19

## Source-backed controller law

Creative US 6,275,899 gives a concrete embodiment matching the EMU10K1-era external tank design:

- 16 delay lines / 32 delay caches (read + write cache per line);
- 16-sample PCI transfer blocks;
- 18-sample caches, giving two samples of latency headroom;
- two caches selected per audio sample;
- a 4-bit sample counter selects pairs 0/1, 2/3, ... 30/31;
- every cache pair is revisited every 16 audio samples;
- up to four service requests are stored in a FIFO;
- completion retires the FIFO head and advances later entries;
- if the FIFO is full when an active cache is selected, that request is ignored and the cache waits until its next 16-sample selection;
- read PCI burst start is current-18 for even current addresses or current-17 for odd;
- write burst start is current+0 for even current addresses or current+1 for odd.

The Linux EMU10K1 headers independently expose `HCFG_LOCKTANKCACHE`, described as cancelling bus-master accesses to the tank cache, plus the external TRAM base/size registers. This independently supports a real asynchronous PCI tank-cache layer beneath the DSP-visible TRAM abstraction.

## What 1.19 corrects

### No special 64-sample retry

A dropped cache is selected again one service period later: 16 samples. Because the selection period is 16, the same cache is naturally also selected at +32, +48, +64, etc. Multiplying FIFO depth (4) by service period (16) does **not** establish a distinct 64-sample glitch mechanism. The old Freeverb 64-sample observation remains an open hardware/bus-timing question.

### One-free-entry pair ambiguity

The patent describes two caches selected in the same sample and a four-entry FIFO, but does not define arbitration if both selected caches are active while exactly one FIFO entry is free. Source-backed facts are only: one request can fit and one cannot. Core 1.19 therefore separates:

- `admissionEnvelope()` — source-backed count and ambiguity report;
- `select(..., LowerThenUpper)` / `select(..., UpperThenLower)` — explicit research tie-breaks for experiments.

No audible behavior depends on that choice.

## Congestion boundary

With every selected cache active and zero request completions:

- sample 0: +2 requests, depth 2;
- sample 1: +2 requests, depth 4;
- sample 2: FIFO already full, both newly selected requests drop.

This is consistent with the patent's 18-vs-16 cache sizing: the design has two sample periods of latency headroom, but persistent lack of PCI service necessarily overruns the request queue. Conversely, if two requests complete every sample, all 32 caches can remain active indefinitely with no FIFO overflow.

## Emulator policy

The controller is retained as a diagnostic/fault-injection model only. Normal FX8010 effects continue to use the programmer-visible TRAM abstraction (reads available for the frame, writes committed after DSP execution), because inserting guessed PCI completion latency would be less accurate than abstracting it.
