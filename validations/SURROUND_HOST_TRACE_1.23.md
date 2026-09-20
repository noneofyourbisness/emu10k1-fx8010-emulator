# PCI1024 Surround Mixer / spatial host trace — Core 1.23

## Exact CTSURMIX balance law

`CTSURMIX.EXE` function `0x40A0A1` contains debug strings `GetBalance Left = %d` and `GetBalance Right = %d` and implements the unsigned 16-bit magnitude-to-mixer-balance mapping:

```text
if L > R: B = R * MAX / (2*L)
if L < R: B = MAX - L * MAX / (2*R)
if L = R: B = MAX/2
```

The arithmetic is integer multiply/divide. Core test `liveware_balance_law` locks representative exact results. This is an exact recovered Surround Mixer host utility, but it is not automatically equated with the still-unresolved full nine-feed spatializer interpolation.

## Placement persistence

The PCI1024 `DEVCON32.DLL` contains placement APIs and `SourceConfig` registry handling. It stores `Balance` and `Fade` as 32-bit floats. `0x41B85FD9` = approximately 23.0468006 is written as the missing/unconfigured sentinel, clearly outside the normal 0..1 placement range. The shipped `Forsaken.sea` preset contains non-sentinel examples including approximately 0.48648649, 0.46551725 and 0.74137932.

`DEVCON32.DLL` also exposes the EFX8010 send service operations `SETSENDLEVEL`, `GETSENDLEVEL`, `GETSENDLIMIT` and `GETSENDTYPE`, independently supporting the architecture in which host placement changes send controls feeding fixed FX8010 spatial programs.

## Nine-item mixer control

`CTSURMIX.EXE` has a mixer-control path that gets/sets nine DWORD detail values in a single `MIXERCONTROLDETAILS` transaction. SB2/SB4 spatializer RIFXs each expose nine native inputs. This is strong architectural/numerical correlation, but the current trace has not symbolically tied that exact mixer control ID to the RIFX input array, so Core 1.23 deliberately does not promote the correlation to an exact mapping.

## SurroundSpatializer native feed taxonomy

Isolated native-feed impulses establish:

```text
0  front-left unity only
1  front-right unity only
2  front-left ~-3 dB only
3  rear-left ~-3 dB only
4  front-right ~-3 dB only
5  rear-right ~-3 dB only
6  rear-left unity only
7  rear-right unity only
8  equal four-channel long recursive/diffuse response
9  equal four-channel direct one-frame 0.5 response
10 equal four-channel short filtered response
```

The -3 dB group has approximately half the energy of its corresponding unity feed.

## Surround Encoder

For the PCI1024 program, feed 0 produces the left front/rear encoded pair and feed 1 the right front/rear pair, both beginning five samples after the impulse. Feeds 2 and 3 are silent under isolated-feed excitation. That does not prove they are globally unused; they may be reserved or meaningful only under a coupled host condition.

## Boundary

Recovered exactly: balance helper arithmetic, Balance/Fade storage type and sentinel, the isolated native feed behavior above. Strong but not yet symbolically complete: relation of the nine-item mixer-control transaction to all nine spatializer RIFX feeds. Still reconstructed: exact AudioHQ X/Y/distance -> all send weights.
