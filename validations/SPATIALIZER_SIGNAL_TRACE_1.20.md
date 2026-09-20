# Native Creative spatializer signal trace — Core 1.20

The July-1999 Creative `SBLFX.DLL` corpus contains five spatial/surround DSP
programs that are now tested as signal processors, not merely parsed as RIFX:

- `2HeadphoneSpatializer`
- `SB2SpeakerSpatializer`
- `SB4SpeakerSpatializer`
- `SurroundSpatializer`
- `Surround Encoder`

The conformance harness relocates each exposed RIFX input patch site to its own
synthetic GPR, injects an isolated impulse and renders 512 native 48-kHz frames.
This avoids assuming the still-host-side meaning/name of every input pin while
answering the important question: does the DSP network actually alter/spread the
audio, or is it a routing/no-op placeholder?

## Results

### 2HeadphoneSpatializer

An isolated left-side source produces an immediate ipsilateral response and a
contralateral response beginning at **12 samples** (0.250 ms at 48 kHz), with a
long filtered tail.  A second native path begins contralateral energy at
**31 samples** (0.646 ms).  For the tested primary source, about **35.06%** of
total output energy occurs after frame 0.

This is unequivocally not a bypass.  The delayed cross-ear path is the kind of
interaural/crossfeed cue used by binaural/headphone virtualizers.

### SB2SpeakerSpatializer

The RIFX contains direct front L/R routes, but its processed source path creates
strong same-side output plus delayed opposite-side crossfeed beginning at
**8 samples** (0.167 ms).  The tested processed path has about 31.9% delayed
energy.  Other paths use the recovered 12- and 31-sample tank taps.

### SB4SpeakerSpatializer

A processed single input fans into **all four outputs**.  Same-side/front+rear
components begin immediately, while the opposite-side pair begins at **8
samples**.  The network therefore does substantially more than duplicate or pan
a stereo signal.

### SurroundSpatializer

This small program has no TRAM, but one diffuse/common input fans into all four
outputs and leaves recursive/filter state for the entire 512-frame probe (the
measured delayed-energy fraction is about 81%).  Other pins are intentionally
simple direct routes.  It is therefore a hybrid mixer/spatial-field network, not
a global no-op.

### Surround Encoder

Its principal left input appears only after **5 samples**, feeds front-left and
rear-left corresponding outputs, and continues through sample 69 in the probe;
the right side mirrors it.  The principal path is 100% delayed relative to the
injected impulse.

## Interpretation

Creative/E-mu's contemporary 3-D-audio work describes synthesized spatial cues
including interaural time delay, interaural level differences, spectral/head
shadow filtering, reflective-environment cues, and binaural rendering over
headphones or loudspeakers.  The native RIFX impulse behavior is consistent with
that design family: short inter-channel delays, crossfeed, filtering/state and
multi-output distribution.

This test does **not** claim that every host-side source-position control or UI
preset has been reconstructed.  It proves that the on-card factory spatializer
microcode itself is active, nontrivial audio processing capable of creating a
virtual/spatial field; it is not a decorative or silent stub.
