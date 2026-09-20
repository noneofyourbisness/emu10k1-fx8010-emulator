# EMU10K1 XTRAM PCI/cache state trace — Core 1.20

## What the public register interface exposes

The EMU10K1 host interface exposes the host-based TRAM base (`TCB`), tank-cache
buffer size (`TCBS`) and `HCFG_LOCKTANKCACHE`, whose contemporary Linux comment
says it cancels bus-master accesses to the tank cache.  The interrupt block has
a generic PCI bus-error pending/enable bit.  No public register definition found
in the Linux/OSS/APS material exposes the identities or ages of the four patented
XTRAM service-FIFO entries.

This matters because a PCI-error interrupt is not evidence of a tank-cache
underflow/overflow by itself; it is a generic PCI bus error.  Core 1.20 therefore
does not turn it into a synthetic XTRAM fault/status bit.

## Source-backed request timing state

Creative US 6,275,899 states that the specific delay-cache embodiment uses:

- 16-sample block transfers (`B=16`);
- 18-sample caches (`M=18`);
- service requests made `M-B = 2` sample periods before a read cache would empty
  or a write cache would fill;
- tolerance of memory/bus latency of up to two full sample periods;
- 32 read/write caches, two selected each sample by a 4-bit round-robin counter;
- a four-entry service FIFO;
- a full FIFO causes a new active-cache request to be ignored;
- that cache is selected again 16 samples later.

Core 1.20 now exposes `XtramServiceFifo::timingState(now)`.  It reports queued
request ages and separates requests at/inside the patented two-sample budget
from requests older than that budget.  An age greater than two is deliberately
called a **controller-risk state**, not a guaranteed particular bad output
sample: the source establishes cache headroom but not the exact EMU10K1 PCI
completion edge, fallback data value, or audible corruption waveform.

`nextSelectionAfter(cache, sample)` also encodes the strictly-later round-robin
opportunity.  A request dropped for cache 0/1 at sample 0 is selected again at
sample 16; cache 4/5 dropped at sample 2 returns at sample 18.

## What remains silicon-only

- exact bus-controller arbitration/completion latency under real PCI contention;
- which simultaneously selected cache wins when exactly one FIFO slot is free;
- whether a late read repeats stale cache data, produces another deterministic
  value, or raises some internal condition before audible failure;
- whether the old reported 64-sample Freeverb/XTRAM artifact is caused by this
  controller at all.

The normal effect engine remains at the programmer-visible TRAM abstraction and
does not inject guessed PCI stalls.
