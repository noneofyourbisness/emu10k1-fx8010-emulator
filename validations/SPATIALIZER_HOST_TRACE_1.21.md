# Creative EMU10K1 spatializer host-feed trace — Core 1.21

## Scope

Core 1.21 does not alter FX8010 arithmetic or the Core 1.20 PCI/TRAM signal path. It adds a host-feed topology proof for the July-1999 Creative `SBLFX.DLL` spatializer programs and separates facts present in the RIFX from the still-unrecovered AudioHQ coordinate interpolation law.

Programs covered:

- `2HeadphoneSpatializer` — 134 instructions, 8 patch inputs, 2 outputs, 12 ITRAM endpoints.
- `SB2SpeakerSpatializer` — 97 instructions, 9 patch inputs, 2 outputs, 12 ITRAM endpoints.
- `SB4SpeakerSpatializer` — 60 instructions, 9 patch inputs, 4 outputs, 8 ITRAM endpoints.
- `SurroundSpatializer` — 15 instructions, 11 patch inputs, 4 outputs, no TRAM.
- `Surround Encoder` — 38 instructions, 4 patch inputs, 4 outputs, 12 ITRAM endpoints.

## Host-side evidence

Creative's AudioHQ/Speaker documentation describes source position as a host operation. Headphone and two-speaker configurations allow the source icon to move continuously along the top edge of the preview. Four-speaker mode allows the source to be placed around the listener and states that moving it farther away reduces level.

The original EMU10K1 playback voice has independently routed FX sends with independently controlled send amounts. This gives the host a direct mechanism for crossfading a source among fixed FX8010 patch inputs while the spatializer microcode/filter coefficients remain unchanged.

The RIFX programs strongly match that architecture: their filter coefficient GPRs are fixed, while each exposes a bank of patch inputs whose impulse responses correspond to hard positions, processed left/right positions, and common/diffuse feeds.

## Exact feed-bank observations

### 2HeadphoneSpatializer

- three measured left/right counterpart feed pairs: 0/1, 2/3, 4/5;
- feed 6 is exactly equal direct L/R for an isolated impulse;
- feed 7 is exactly equal L/R but filtered/delayed, retaining output through frame 44;
- feed 2 is the most strongly lateral measured left-side feed: opposite-ear onset 31 samples and >500:1 left/right impulse energy ratio;
- feed 0 opposite-ear onset is 12 samples;
- feed 4 is another 12-sample cross-ear path.

For reconstructed horizontal positioning the preservation CLAP orders the seven directional feeds as `2,0,4,6,5,1,3`; feed 7 is exposed separately as a common/diffuse feed. The ordering is a reconstruction based on measured lateral energy and symmetry, not a recovered AudioHQ table.

### SB2SpeakerSpatializer

- feed 0 is exact direct left; feed 1 is exact direct right;
- processed counterpart pairs are 2/3, 4/5 and 6/7;
- feed 8 is a symmetric common/filter path;
- all processed opposite-side paths begin at the native 8-sample spatializing delay.

The reconstructed horizontal order is `0,4,2,6,8,7,3,5,1`. The direct endpoints and common center are strongly constrained; the exact ordering/curve of the intermediate host points remains inferred.

### SB4SpeakerSpatializer

The direct corners are unambiguous from isolated impulses:

- feed 0 -> front-left output only;
- feed 1 -> front-right output only;
- feed 4 -> rear-left output only;
- feed 5 -> rear-right output only.

Feeds 2/3 and 6/7 are processed left/right counterpart pairs which reach both front and rear, with opposite-side energy delayed by 8 samples. Feed 8 is common to all four outputs. This is a real spatial feed bank rather than ordinary four-channel volume routing.

The CLAP's continuous X/Y mode uses an explicitly reconstructed eight-feed ring plus common center. The assignment of 2/3 versus 6/7 to the two intermediate side sectors is inferred; Raw Factory Feed mode bypasses that inference completely.

### SurroundSpatializer

- feeds 0/1 are full-level direct front L/R;
- feeds 6/7 are full-level direct rear L/R;
- feeds 2/4/3/5 are half-level direct per-output paths;
- feed 8 and feed 10 are four-output filtered/stateful common paths;
- feed 9 is equal direct to all four outputs.

The CLAP reconstructed mode therefore exposes front/rear position plus diffuse feed rather than pretending an original 2-D AudioHQ coordinate map was recovered for this program.

### Surround Encoder

The principal evidenced stereo input pair is 0/1. An isolated impulse through feed 0 appears on front-left/rear-left after five samples; feed 1 mirrors it to the right. Factory feeds 2/3 remain accessible in Raw Factory Feed mode but do not produce isolated output in the current impulse probe.

## Core 1.21 regression

`native_spatializer_host_feeds` locks only source-backed/measured topology:

- headphone counterpart pairs + equal direct/common feeds;
- two-speaker direct endpoints + processed pairs + common feed;
- four-speaker direct FL/FR/RL/RR identities + processed counterpart pairs + all-output common feed.

It deliberately does **not** assert Creative's exact pixel-to-send-volume interpolation curve, because that host-side table/formula has not been recovered.
