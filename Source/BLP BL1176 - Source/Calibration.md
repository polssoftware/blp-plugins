# BLP BL1176 - five-ratio WAV calibration

Updated 2026-11-10. Only BLP BL1176 was changed and rebuilt. **New instances and the Default factory program now use 12:1.** Existing saved ratios are retained. The GUI, title, knob layout, bypass styling, parameter IDs/ranges, plugin identity and other factory-program values are unchanged.

## Input validation

Reference files are the five supplied BandLab WAVs (named bl - bl1776) and five emulation WAVs, at 4:1, 8:1, 12:1, 20:1 and All. Ratios are compared independently at otherwise default settings. The matching dry input is the previously supplied mine - no fx raw.wav: stereo 48 kHz/16-bit PCM, 647786 frames (13.49554 s). The emulation files have the same format and duration. BandLab files are stereo 44.1 kHz/16-bit PCM, approximately 13.5142-13.5145 s; comparison uses the shared dry-file duration, excluding the small extra reference tail. Correlation found no whole-sample shift at a common 48 kHz.

Before modifying the DSP, an independent model of the existing source reproduced every emulation recording near its 16-bit export noise floor:

| Ratio | Original-source model versus supplied emulation residual, dB |
|---|---:|
| 4:1 | -74.665 |
| 8:1 | -73.173 |
| 12:1 | -72.721 |
| 20:1 | -72.356 |
| All | -70.264 |

This establishes the source, dry input and emulation settings. Defaults other than ratio remain Input +10 dB, Output 0 dB, Attack 1.2, Release 5.0, bypass off. FFmpeg resampled reference WAVs to 48 kHz for common-rate comparisons; native 44.1 kHz VST3 renders were also compared directly against the original BandLab WAVs using a resampled dry input. No output was level-normalized.

The prior implementation used one fixed -18 dB threshold for the first four ratios, -21 dB for All, a shared 4 dB knee, conventional ratio slopes and mostly shared timing. It became progressively quieter than BandLab at higher ratios. Merely adding gain left substantial waveform differences.

## Implementation

Each measured ratio now has its own fitted threshold, knee, gain-computer slope, attack/release anchor and small makeup correction. All has an independent calibration. Input still drives both audio and detection; Output remains a direct level control in addition to the calibrated correction. The following are coefficients of an independent approximation, not a unique identification of BandLab's internal thresholds, time constants or true static ratios.

| Ratio | Effective attack at default, ms | Effective release at default, ms | Model threshold, dB | Model slope | Knee, dB | Makeup correction, dB |
|---|---:|---:|---:|---:|---:|---:|
| 4:1 | 0.053304 | 205.506468 | -8.541375 | 0.855667 | 0.026045 | 0.225098 |
| 8:1 | 0.040310 | 210.042986 | -8.158611 | 0.891801 | 0.130074 | 0.304289 |
| 12:1 | 0.017202 | 219.969379 | -7.515945 | 0.949310 | 0.058150 | 0.535341 |
| 20:1 | 0.014087 | 218.293606 | -6.969759 | 0.954595 | 0.411515 | 0.455573 |
| All | 0.010000 | 230.261067 | -6.859030 | 0.976962 | 0.033081 | 0.844172 |

Attack and Release retain the original knob directions and exponential ranges relative to these measured default anchors. Their other positions are provisional. Ratio-dependent target values, including knee and makeup gain, smooth over 5 ms. The detector remains linked to the louder stereo channel, with one reduction value applied to both channels.

Across all five BandLab ratios, cross-spectral analysis found a consistent tonal/phase difference: a small positive low-frequency phase shift and a progressive high-frequency attenuation/phase lag. The pattern was not present in the earlier DeEsser WAV reference comparisons. A shared minimum-phase approximation improved all five modes substantially: a first-order high-pass at **6.971167 Hz** followed by a first-order low-pass at **13685.119311 Hz**, after compression. These are fitted cutoff values, not a claim about physical BandLab components. No brick-wall MP3 roll-off was copied; these measurements are from WAVs. A dedicated low-level sweep/bypass render would still be needed to identify the filter response independently of compression.

Both filters normalize their first-order coefficients, use double-precision state, recalculate from the actual sample rate, and reset in prepareToPlay. Cutoffs clamp below Nyquist for unusually low processing rates. The wet signal is filtered before the bypass crossfade; bypass remains untouched dry audio after its ramp. There is no lookahead, inserted audio delay, saturation, clipping, limiter, recording-specific envelope, or input-file detection. Reported latency remains zero; a 0.25-second tail allowance accounts for IIR settling. Floating-point headroom is retained.

## Actual compiled-VST3 comparison

Relative waveform error = RMS(output minus reference) / RMS(reference), across both channels in the aligned shared section. It is not a listening-quality score. RMS includes pauses and is not LUFS.

Common 48 kHz timeline (reference resampled from BandLab's 44.1 kHz WAV):

| Ratio | Original error % | Revised error % | Error decrease % | BandLab RMS dBFS | Revised RMS dBFS |
|---|---:|---:|---:|---:|---:|
| 4:1 | 46.5488 | 5.4730 | 88.24 | -17.9369 | -17.9509 |
| 8:1 | 58.2312 | 5.4788 | 90.59 | -17.9729 | -17.9866 |
| 12:1 | 62.0099 | 5.6840 | 90.83 | -17.8761 | -17.8974 |
| 20:1 | 65.7716 | 5.7272 | 91.29 | -17.5575 | -17.5694 |
| All | 76.0175 | 5.9323 | 92.20 | -17.3543 | -17.3416 |

Native 44.1 kHz comparison directly against the BandLab WAVs:

| Ratio | Revised error % | Correlation | BandLab RMS dBFS | Revised RMS dBFS | BandLab peak dBFS | Revised peak dBFS |
|---|---:|---:|---:|---:|---:|---:|
| 4:1 | 5.5666 | 0.998450 | -17.9369 | -17.9454 | -2.5066 | -4.0864 |
| 8:1 | 5.5773 | 0.998444 | -17.9729 | -17.9795 | -2.7193 | -4.5559 |
| 12:1 | 5.7658 | 0.998336 | -17.8761 | -17.8878 | -2.8349 | -5.6212 |
| 20:1 | 5.8071 | 0.998314 | -17.5575 | -17.5592 | -2.6539 | -5.3347 |
| All | 6.0361 | 0.998187 | -17.3542 | -17.3308 | -2.5211 | -5.1753 |

At native 44.1 kHz, overall RMS differs by less than 0.024 dB across all five modes. Short transient peaks remain about 1.6-2.8 dB lower than BandLab. Thus overall level and most waveform behavior are closer, but peak recovery is not an exact match.

The improvement is not just a level increase. After applying the best diagnostic constant gain to each output, errors remain:

| Ratio | Original level-adjusted error % | Revised level-adjusted error % |
|---|---:|---:|
| 4:1 | 20.0725 | 5.4730 |
| 8:1 | 22.9209 | 5.4788 |
| 12:1 | 24.3523 | 5.6834 |
| 20:1 | 25.9770 | 5.7272 |
| All | 32.6230 | 5.9236 |

Those gain adjustments are diagnostic calculations only; the delivered WAVs and plugin outputs were not normalized. Spectral comparison CSVs show energy changes of this vocal, not a fixed EQ transfer function.

## Fitting and validation limits

The fitting objective was normalized waveform error on alternating 0.5-second sections, with the other sections used for checking and model selection. This is an internal split of the same clip, not blind validation on independent material. Peak-envelope and RMS-envelope alternatives were evaluated; the existing gain-reduction-envelope family plus the shared filter model produced the better fit. The 20:1 calibration was also checked from a second starting point to avoid a poorer wide-knee fit. A delayed gain-control experiment worsened overall matching and was not added.

Selected fitting/checking residuals:

| Ratio | Fitting residual, dB | Checking residual, dB |
|---|---:|---:|
| 4:1 | -25.3936 | -25.0181 |
| 8:1 | -25.3886 | -25.0119 |
| 12:1 | -25.1370 | -24.6172 |
| 20:1 | -24.9911 | -24.6924 |
| All | -24.7015 | -24.3686 |

These are endpoint calibrations for default Input/Output/Attack/Release and the five supplied ratios. Static compression curves, other control positions, All-mode harmonic behavior, other input levels and other material have not been recovered. Native BandLab 48 kHz response is unverified; the 48 kHz reference here is resampled. More useful evidence would be identical dry and BandLab WAVs of stepped-level tones, short bursts and a second vocal, plus low-level sweeps and bypass at both sample rates. Those would separate transient recovery, true ratios/knees, program dependence and coloration.

## Regression checks

Actual loaded VST3 tests passed all five ratios at 44.1/48 kHz, finite default-render output, sample-exact settled bypass, and a 12:1 default parameter value in new instances. Direct-source tests passed fresh and Default-factory-program 12:1 behavior, preservation of an existing saved 20:1 state, unchanged 430 x 270 editor dimensions, equal output for identical stereo channels, bit-exact mono-versus-duplicate-stereo consistency, all ratio/control extremes, finite randomized automation and a decaying filter tail at both rates. GUI source following KnobStyle was verified unchanged.

The extreme-control test's maximum absolute sample was 6.45185: the processor preserves floating-point headroom rather than imposing an output limiter. Default comparison renders remain below full scale. These finite tests are not certification for every possible input or automation pattern. No interactive DAW listening test was performed.

## Deliverables and build

The validation ZIP includes ten revised float32 stereo WAVs (five ratios at 44.1/48 kHz), five spectral CSVs, selected constants, metrics and test logs. Original user audio is not embedded in source. The source ZIP opens directly to source files and includes Calibration/RenderHost.cpp and UnitTests.cpp. Optional BLP1176_BUILD_TESTS builds the repeatable render and regression tools. Render host arguments are the VST3 bundle and a directory containing interleaved stereo float32 dry-44100.f32 / dry-48000.f32. Mode indices 0..4 correspond to 4/8/12/20/All, and 5 tests bypass.

The calibration build used Visual Studio 2022 and JUCE 8.0.12. The source's original JUCE 8.0.8 FetchContent configuration is retained. Compiled Windows x64 binary SHA-256: `6855d66a0b62efc74a6705ee8a21a4152006b663c382bbdb866a2ee12aefa022`.

Only BL1176 was rebuilt. Other 20 source archives and every file in the existing full compiled suite were verified unchanged. The full compiled-suite ZIP remains intact; use the separate updated BL1176 VST3 ZIP for this revision. The complete updated source suite includes the new BL1176 source.
