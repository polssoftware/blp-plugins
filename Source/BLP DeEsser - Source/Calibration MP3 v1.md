# BLP DeEsser calibration - 2026-11-10

Only BLP DeEsser was changed and rebuilt. This revision targets the supplied 8000 Hz, -40.0 dB recordings. The user confirmed equal track/master levels, 320 kbps MP3 export and no other plugins. Hidden host parameters are assumed to retain source defaults: Amount 60%, Attack 2 ms, Release 80 ms, Mix 100%.

## Inspection and comparison

The inspected original implementation uses a Q=0.8 constant-peak band-pass detector, stereo peak linking, capped gain reduction and subtraction of only the band-pass region from the original input. The source model at the supplied setting reproduced the aligned emulation MP3 with relative waveform error -56.89 dB (roughly 0.143%). This confirms the starting source/DSP, allowing for MP3 loss.

The dry vocal is stereo 48 kHz, 16-bit PCM. BandLab's MP3 is 44.1 kHz and the emulation MP3 is 48 kHz. Both MP3s are 320 kbps, but equal bitrate does not imply sample-identical encoding. FFmpeg decoded/resampled them for comparison. Correlation alignment found 0 samples of delay for BandLab and 1285 samples for the emulation on the 48 kHz timeline. This export alignment was corrected for measurement; no delay was added to the plugin.

BandLab reduces energy below the sibilance band as well as within it. This is better approximated by a focused sidechain detector driving predominantly broadband gain reduction, with a small additional band reduction. Matching only the old band's depth or increasing a fixed output attenuation does not explain the time-varying spectral balance.

## Implementation

The band-pass detector now uses Q=4.103537; effective attack/release at the existing default host controls are 0.435410 / 51.808816 ms. A fitted detector sensitivity offset of 23.014530 dB at -40 dB threshold drives the existing soft reduction calculation. Its cap before Amount scaling is 17.775671 dB (about 10.665403 dB at default Amount). The broadband gain uses exponent 0.956743 of the full reduction gain; the remaining difference acts on the band-pass signal. A small fitted gain offset of 0.223861 dB applies at the measured point. These are model coefficients, not unique measurements of BandLab's internal controls.

Sensitivity offset, reduction activation and the small gain offset fade with threshold strength toward 0 dB threshold. Amount 0, Mix 0 and bypass remain dry. Frequency and all control IDs/ranges/defaults, GUI, title, animation, and bypass styling are preserved. Existing 20 ms parameter smoothing remains. Time constants and detector coefficients use the actual processing sample rate. Invalid processing rates fall back to 44.1 kHz. No audio-specific timeline, waveform, learned envelope, file detection, saturation, limiter, MP3 roll-off or artificial delay was embedded.

Mappings away from the measured setting are provisional. Old sessions retain their control values but use the revised DSP. Only the 8 kHz / -40 dB point has been calibrated; other thresholds, frequencies, hidden controls and different material are not established matches.

## Actual compiled VST3 results

Relative error is RMS(output minus reference) divided by RMS(reference), across both channels over the aligned clip. No output level normalization was used. RMS includes pauses and is not LUFS.

| Measurement | Original emulation MP3 | Revised VST3 at 48 kHz | BandLab reference at 48 kHz |
|---|---:|---:|---:|
| Relative waveform error, % | 80.0806 | 10.4174 | 0 |
| Stereo RMS, dBFS | -31.3870 | -36.1877 | -36.0732 |
| Sample peak, dBFS | -13.2898 | -21.0728 | -18.7006 |
| Waveform correlation | 0.962158 | 0.994596 | 1 |

Relative error decreased by 86.99%. Overall RMS differs by 0.1145 dB. Peaks remain approximately 2.37 dB lower than the reference: transient recovery is not an exact match. At native 44.1 kHz the actual VST3's relative error is 10.4025%, correlation 0.994582. Native BandLab 48 kHz behavior is unverified; the 48 kHz reference is resampled from its 44.1 kHz MP3.

Broad-band energy changes relative to the dry input, dB:

| Range Hz | BandLab | Original | Revised |
|---|---:|---:|---:|
| 100-1000 | -4.5128 | -0.0062 | -4.6398 |
| 1000-4000 | -5.9544 | -0.1080 | -6.0121 |
| 4000-7000 | -6.4589 | -0.5391 | -5.9301 |
| 7000-10000 | -9.6167 | -1.8598 | -9.1490 |
| 10000-15000 | -8.4732 | -0.7972 | -7.8947 |

Parameters were fitted on alternating 0.5-second sections and checked on the other sections of the same recording. Final model fitting/checking error was -20.051 / -19.230 dB. This internal split is not independent validation on a new vocal. MP3 encoding and sample-rate conversion contribute to the residual, and the remaining roughly 10.4% waveform error cannot be attributed entirely to export noise.

The actual loaded Windows x64 VST3 rendered finite outputs at 44.1/48 kHz. Settled bypass and 0% wet matched dry samples within 1e-7. Existing source tests passed: 200 Hz body ratio 0.976391, 6000 Hz sibilance ratio 0.174084, dry=1, bypass=1, restored frequency=6000. These test ratios are checks of operation, not additional BandLab measurements. No interactive DAW listening, exhaustive automation or all-input stability certification was performed.

## Remaining evidence

A closer match is established for this supplied render pair, not a complete or exact recreation. Please use lossless WAV renders for further calibration. Useful additional comparisons are 0/-20/-30/-40 dB threshold at 8 kHz, and 4/6/10 kHz at -40 dB, plus a second vocal at the measured setting. Those would separate sensitivity, detector shape, broadband/split-band behavior, transients and level offset without MP3 ambiguity.

## Deliverables and provenance

Source ZIP opens directly to source files. Validation ZIP contains revised float32 stereo WAVs at both sample rates, this report, spectral CSV, numerical metrics, fit coefficients and test logs. Original vocal files are not bundled in source. Calibration/RenderHost.cpp supplies the loaded-VST3 render/transparency harness (inputs are interleaved stereo float32 dry-44100.f32 and dry-48000.f32). Build it with CALIBRATION_HOST=ON and pass the plugin bundle and input/output directory.

Windows x64 build used Visual Studio 2022 and JUCE 8.0.12. Binary SHA-256: `817240e51c9e6ccaa313207c84d467c61957d90d68de5a132169f8af8c0f3343`. Other 20 source ZIPs and all entries of the prior compiled suite were verified unchanged. The prior all-plugin binary ZIP remains intact; install the separately updated plugin bundle to use this revision.
