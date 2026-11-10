# BLP TecLab BA-2A default-response calibration

Updated 2026-11-10. Only BLP TecLab BA-2A processing and its supporting source/tests/docs were changed and rebuilt. GUI, title, default control values, parameter IDs and plugin identity remain unchanged.

## Recordings and method

The supplied dry recording and "mine" render are stereo 48 kHz, 16-bit PCM, 647786 frames. The BandLab render is stereo 44.1 kHz, 16-bit PCM, 595154 frames. Both durations are approximately 13.49555 seconds. Cross-correlation found no whole-sample offset on a common 48 kHz timeline. A symmetric 97-tap windowed-sinc conversion was used for cross-rate comparisons; no test render was level-normalized. Native sample-rate peaks and resampled peaks can differ.

Before modifying the algorithm, an independent simulation of the inspected original source reproduced the user's "mine" render with relative error -72.84 dB (about the export's quantization floor). This establishes that the source and supplied dry signal describe the current starting point.

The original uses a stereo peak detector (3 ms attack / 55 ms release), a default -33.1 dB threshold and 3:1 ratio, 8 dB knee, an additional 12 ms attack/program-dependent release stage, and makeup linked to instantaneous gain reduction. Its default output is about 2.57 dB quieter in stereo RMS than BandLab while allowing a higher sample peak. A constant output boost alone does not correct the dynamics.

## Implementation

The revised compressor uses a stereo-linked mean-square detector (average power across channels), normalized to behave identically for mono and duplicate-mono stereo. Fitted default constants:

| Setting | Revised value |
| --- | ---: |
| Power-envelope attack | 0.0214972 ms |
| Power-envelope release | 17.0831 ms |
| Threshold at Squeeze 70% | -23.3873 dBFS |
| Compress ratio | 1.81148:1 |
| Soft knee width | 27.9204 dB |
| Gain-reduction attack | 1.82996 ms |
| Gain-reduction release | 0.103320 ms + 41.9369 ms x min(memory/18, 1) |
| Reduction memory time | 1.5 s |
| Makeup at Gain 60% | +0.510779 dB, independent of current reduction |

The power-envelope attack above is a coefficient time constant for this topology, not a claimed measurement of BandLab's published attack time. Detector, gain computer, and reduction smoothing operate together.

Defaults remain Squeeze 70%, Gain 60%, Compress, Mix 100%, Output 0 dB, bypass off. The fitted gain offset is part of the compressor's gain-control mapping, not a post-processing adjustment to the validation files. Gain changes by 0.36 dB per percentage point around 60%. Squeeze changes threshold by -0.43 dB per percentage point around 70%; its first 8% smoothly introduces compression/makeup. Squeeze 0 produces unity at default Output after parameter smoothing settles. Limit interpolates towards ratio 20:1 and a shorter attack. These other-setting mappings are provisional, not recovered BandLab behavior.

Actual processing sample rate is used for time constants; invalid rates fall back to 44.1 kHz. Detector and reduction state now use double precision. The existing 30 ms control smoothing, wet/dry and output parameters, bypass crossfade, preset/state recall, and high-level soft safety stage remain. That safety stage is inactive for these default comparison renders. No new saturation, EQ, latency, lookahead, per-recording envelope, or input-file detection was added.

## Actual loaded-VST3 results

These are output renders from the compiled Windows x64 VST3, not just the fitting model. Stereo RMS includes the entire recording and pauses; it is not LUFS. At 48 kHz the BandLab reference is resampled to the plugin render rate.

| Common 48 kHz comparison | Original BLP | Revised BLP | BandLab reference |
| --- | ---: | ---: | ---: |
| Stereo RMS, dBFS | -27.35884 | -24.82195 | -24.78790 |
| Sample peak, dBFS | -4.99143 | -5.37792 | -5.36172 |
| Relative waveform error, % RMS | 34.362 | 8.559 | 0 |
| Relative waveform error, dB | -9.278 | -21.351 | -infinity |
| Waveform correlation | 0.964751 | 0.996330 | 1 |
| Error after best constant level adjustment, dB | -11.595 | -21.351 | -infinity |

Relative waveform error = RMS(revised minus reference) / RMS(reference), across both channels. This decreased by **75.09%**. The revised full-clip RMS is 0.03406 dB from BandLab. The improvement after best constant gain adjustment shows the change addresses dynamics rather than only level. The best-adjustment calculation is diagnostic; neither plugin output nor supplied WAVs were adjusted by it.

At native 44.1 kHz, revised RMS is -24.82364 dBFS versus BandLab's original -24.78791 dBFS; revised peak is -5.48042 dBFS versus BandLab's -5.37853 dBFS. Relative waveform error is 8.548% (-21.363 dB), correlation 0.996340.

Alternating 0.5 s sections were used for fitting and checking, respectively. For the selected model, sampled fitting/checking errors were -21.454 / -21.136 dB. This is an internal split of the same recording, not validation on independent material. Several simple detector topologies were compared. Parameters are stored directly as an ordinary compressor model; no time-indexed correction curve is included.

Both 44.1 and 48 kHz passed actual-VST3 default renders, exact dry mix and bypass after smoothing settles, finite output, stereo symmetry with identical channels, Gain/Squeeze/Mode extremes, randomized simultaneous automation, saved-state recall, and unchanged 300 x 270 editor dimensions. Under the bounded synthetic extreme-setting test, output peaks stayed below full scale. This is not a guarantee against every possible input or automation sequence. The DC automation log includes initial startup from zero; its maximum is not a click-free transition specification.

## Limits and additional evidence

This is a materially closer default-response approximation, **not an exact BandLab match**. About 8.55% relative RMS waveform error remains. Residuals may include detector topology, program dependence, stereo linking details, phase/resampling behavior, export quantization, and unmodeled dynamics. There was no interactive DAW listening test.

The supplied BandLab recording establishes only this default setting at 44.1 kHz. The 48 kHz check verifies recalculated time constants and numerical behavior against a resampled reference; a native BandLab 48 kHz match remains unverified. Gain/Squeeze positions, Limit, other input levels and material remain uncalibrated. Useful follow-up renders: the same dry file at Squeeze 0/35/100 and Gain 40/80, both modes; stepped-level tones and bursts; a second dry clip plus BandLab default output; and native 48 kHz BandLab output if available. Keep track and export gains identical and avoid unrelated processing.

Existing projects retain parameter values but will sound different when using the revised plugin. The previous compiled all-plugin ZIP is preserved. Other 20 source archives and every file in that existing compiled suite were checked unchanged.

## Build and files

Windows x64 VST3, Visual Studio 2022, JUCE 8.0.12. Compiled binary SHA-256: `326b23eec20262f2dc2cff7047fa29122043cedbf7e71af00ef5ed59d8655ace`.

The validation archive includes two revised float32 stereo WAVs (44.1/48 kHz), the comparison plot, time-series CSV, metrics, fitting constants, and host-test log. Original user audio is not bundled into source. `Validation/HostTests.cpp` supplies a portable render/regression harness; source packages open directly to source files.
