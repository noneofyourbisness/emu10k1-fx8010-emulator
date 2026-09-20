# EMU10K1 XTRAM four-entry FIFO trace — Core 1.18

## Source-backed controller

Creative Technology US 6,275,899 (filed 1998) describes a specific delay-cache implementation matching the EMU10K1 external TRAM era:

- 32 delay caches;
- 16-sample PCI block transfers;
- 18-sample caches;
- a 4-bit sample counter selecting two caches per sample;
- pair order 0/1, 2/3, ... 30/31;
- every cache selected once per 16 sample periods;
- a four-entry FIFO holding active service requests;
- FIFO head completion advances later requests;
- if the FIFO is full when an active cache is selected, the request is ignored and the cache waits until its next selection 16 samples later.

The same embodiment aligns 16-bit samples to the 32-bit PCI bus with start corrections of -18/-17 for read bursts and 0/+1 for write bursts depending on current-address parity.

## Core 1.18 model

`XtramServiceFifo` models only these evidenced controller semantics.  It does not assign an invented fixed PCI latency.  Bus completion is explicit, which lets tests model prompt service, stalls, and drop/retry conditions without changing the normal DSP/TRAM signal path.

A service request records cache index, read/write direction, current 20-bit address, parity-corrected burst start, and the sample at which the cache was selected.

## 64-sample result

The old observation `4 FIFO entries * 16 sample revisit period = 64 samples` is structurally true, but Core 1.18 shows why it is not by itself a periodic glitch mechanism.  With all 32 caches active and both selected requests retired each sample, a 256-sample run accepts and completes 512 requests with zero drops.

A 64-sample-related artifact can arise only after service congestion: once a request is dropped because the FIFO is full, that cache is not eligible again for 16 samples.  The four-deep queue controls how much outstanding work can accumulate; it does not spontaneously corrupt audio every 64 samples.

Therefore the historical Freeverb/XTRAM 64-sample report remains a useful clue for PCI-stall/cache-service investigation, but normal emulation must not synthesize the glitch without evidence of the corresponding service backlog.

## Remaining hardware question

The exact PCI bus-controller completion/arbitration timeline is not specified tightly enough here to assign a universal number of sample periods to each request.  That should be recovered from a real-card trace, a lower-level Creative document/driver, or bus-analyzer evidence.  The Core 1.18 model deliberately exposes that variable instead of guessing it.
