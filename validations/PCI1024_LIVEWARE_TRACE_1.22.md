# PCI 1024 / LiveWare 2.5 trace — Core 1.22

The supplied 1024ENGLISH driver disc contains Win95 `SBLFX.DLL` SHA-256 `e64aaec380fcd04fe00ec1e69568ba895747b25e01ccd3dfe2d7ae2951f19d5f` and 23 embedded EMU8010_A4 RIFX/PTXT programs. Every extracted RIFX is byte-identical to `fixtures/native_sblfx_1999`; Core 1.22 locks that identity with independent FNV-1a fingerprints.

This also proves two LiveWare factory revision families must remain separate. The PCI1024 disc has Headphone/SB2/SB4/Surround/Encoder instruction counts 134/97/60/15/38 and FDN EAX2 80 instructions + 43 TRAM endpoints. The broader later factory corpus has 131/95/58/10/39 and FDN EAX2 84 + 47 respectively.

`DEVCON32.DLL` on the same disc exposes explicit `EFX8010_SETSENDLEVEL`, `GETSENDLEVEL`, `GETSENDLIMIT`, `GETSENDTYPE`, placement-preset APIs, and registry `SourceConfig` state. Surround Mixer contains a `MIXERCONTROLDETAILS` path for a 9-item value array, matching the nine native feed points used by SB2/SB4 spatializers. Its coordinate setter linearly rescales stored X/Y coordinates into the mixer's advertised maximum before committing control details. This strengthens the host architecture: source positioning is implemented in the host/mixer send layer feeding fixed spatializer RIFX networks.

The exact multidimensional nine-weight interpolation table/function is still not fully recovered, so Core 1.22 does not invent one.
