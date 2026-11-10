# BLP DeEsser V2 - two-threshold WAV comparison

Updated 2026-11-10. Only BLP DeEsser was modified and rebuilt. This revision supersedes the earlier single-setting MP3 fit. The GUI, title, animation, defaults, parameter IDs/ranges and plugin identity are unchanged.

## Inputs and alignment

The user confirmed the dry input is the earlier **mine - no fx raw.wav**, not ow acapella raw.wav. It is stereo 48 kHz/16-bit PCM, 647786 frames (13.49554 s). The two BandLab WAVs are stereo 44.1 kHz/16-bit PCM, 595154 frames (13.49556 s). The two emulation WAVs are stereo 48 kHz/16-bit PCM, 976929 frames (20.35269 s). Comparisons use the shared first 13.50-second section; extra emulation audio is excluded.

Correlation found zero whole-sample offset at a common 48 kHz. FFmpeg converted the BandLab WAVs to 48 kHz for common-rate comparisons. Native 44.1 kHz plugin renders were also compared directly to the original BandLab WAVs using a resampled dry input. No level normalization was applied.

Before modification, actual loaded VST3 renders reproduced the supplied emulation WAVs with residuals -81.81 dB at -19 dB threshold and -74.05 dB at -40 dB threshold. Those are near the 16-bit export noise floor, establishing that the selected dry file, control settings and source describe the supplied emulation. Hidden controls retain defaults: Amount 60%, Attack 2 ms, Release 80 ms, Mix 100%.

## What changed

The prior model was extrapolated from a single -40 dB MP3 render of a different vocal. On these new WAVs it attenuated too little at -19 dB, and its -40 dB timing and spectral balance differed from BandLab.

A high-pass sidechain driving **shared broadband attenuation** fits both thresholds better than the prior band-pass detector and mixed band subtraction. The audio path now multiplies both channels by the same time-varying gain. There is no static output gain adjustment or audio-path EQ. This is an independent approximation, not a claim to have recovered BandLab's internal code or unique filter topology.

| Internal calibrated value | Value |
|---|---:|
| High-pass detector Q | 0.486324092 |
| Effective attack at default 2 ms control | 1.993546761 ms |
| Effective release at default 80 ms control | 27.359766851 ms |
| Detector sensitivity offset at -40 dB | 27.234570180 dB |
| Offset change per threshold dB above -40 | 0.034567834 dB |
| Reduction cap before Amount scaling | 20.152526391 dB |
| Maximum reduction at Amount 60% | 12.091515835 dB |

The ordinary control values remain unchanged; these constants calibrate their internal mapping. For threshold t, sensitivity offset is A+B*(t+40). Target reduction uses max(0, detectorLevelDB-t+offset), slope 0.85, the fitted cap, and Amount. Activation fades continuously from full at -3 dB threshold to zero at 0 dB threshold, preserving neutral operation at the top of the slider. That unmeasured upper-threshold mapping is explicitly provisional.

The detector uses normalized second-order high-pass coefficients recalculated at the actual processing sample rate and selected frequency. Existing 20 ms control smoothing and bypass/wet crossfades remain. The gain is nonnegative attenuation only, with no added makeup, saturation, clipping, lookahead, artificial export delay, per-recording envelope or file detection. The stereo link uses the maximum magnitude of the two sidechain channels. Existing saved parameter values are retained but use the revised processing.

## Actual compiled VST3 results

Relative waveform error = RMS(output-reference)/RMS(reference), across both channels of the aligned shared section. It is not a listening score. RMS levels include pauses and are not LUFS.

Common 48 kHz comparison (BandLab converted from 44.1 kHz):

| Threshold | Supplied emulation error % | Revised VST3 error % | Error decrease % | BandLab RMS dBFS | Revised RMS dBFS |
|---|---:|---:|---:|---:|---:|
| -19 dB | 35.7892 | 9.7737 | 72.69 | -20.9181 | -21.0300 |
| -40 dB | 21.7465 | 13.3908 | 38.42 | -26.2380 | -26.3797 |

Native 44.1 kHz comparison against the supplied BandLab WAVs:

| Threshold | Revised error % | Correlation | Revised RMS dBFS | BandLab RMS dBFS | Revised peak dBFS | BandLab peak dBFS |
|---|---:|---:|---:|---:|---:|---:|
| -19 dB | 9.6969 | 0.995306 | -21.0123 | -20.9180 | -4.4708 | -3.9194 |
| -40 dB | 13.5070 | 0.990850 | -26.3609 | -26.2380 | -7.8851 | -7.5051 |

The improvement is not explained only by overall level. After the best diagnostic constant gain adjustment, common-rate waveform errors are:

| Threshold | Original error % | Revised error % |
|---|---:|---:|
| -19 dB | 22.6044 | 9.7404 |
| -40 dB | 21.1889 | 13.3714 |

That gain adjustment is an analysis diagnostic and was not applied to plugin output or the delivered WAVs. Spectral comparison CSVs report energy changes in frequency regions of this vocal; they are not fixed EQ frequency responses.

Parameters were fitted jointly using equal weighting for normalized errors at both thresholds, on alternating 0.5-second sections. Remaining sections checked the fits and helped compare detector models, so they are not a blind independent test. Selected model joint fitting residual: -17.7076 dB. Checking residuals: -20.9934 dB at -19 and -18.1577 dB at -40. A high-pass model with no fixed output offset and purely broadband reduction retained essentially the same error as a model with extra gain/blend controls, so those extra fitted controls were removed.

## Checks and limits

Actual loaded VST3 tests passed at 44.1 and 48 kHz for both measured thresholds, finite output, and unity dry behavior within 1e-7 at bypass, Mix 0, Amount 0 and threshold 0. Existing source tests passed: 200 Hz body ratio=1, 6000 Hz sibilance ratio=0.103847, dry=1, bypass=1, recalled frequency=6000. These tone results verify operation; they are not BandLab target measurements. GUI source following GraphicLook was verified unchanged.

This is a closer match at both supplied settings, not an exact recreation. At native 44.1 kHz, peaks remain approximately 0.55 dB lower at -19 and 0.38 dB lower at -40. Program-dependent timing, detector shape, stereo linking and gain mapping still contribute to residual error. Other frequencies and thresholds, hidden controls, and native BandLab 48 kHz operation remain unverified. The 48 kHz test uses a resampled BandLab reference. No interactive DAW listening or exhaustive automation certification was performed.

For further identification, use lossless matching dry/BandLab WAVs at intermediate thresholds (-10/-25/-30 dB), additional frequencies (4/6/10 kHz), a second vocal, and stepped sibilant/noise bursts. Keep all track, master and export levels identical.

## Files and build

Validation ZIP contains four revised float32 stereo WAVs (both thresholds at 44.1/48 kHz), numerical metrics, spectral energy CSVs, fit constants and test logs. Source ZIP opens directly to source files and includes the portable Calibration/RenderHost.cpp used for the actual-VST3 tests. Build with CALIBRATION_HOST=ON; pass the bundle path and a directory containing interleaved stereo float32 dry-44100.f32 / dry-48000.f32. Host mode 0=-40 dB, 3=-19 dB, 1=bypass, 2=Mix 0, 4=threshold 0, 5=Amount 0.

Windows x64 VST3 built with Visual Studio 2022 and JUCE 8.0.12. Compiled binary SHA-256: `c6d7f4baf3301b69fa5d549d80aee7edd81937132925eb9c1f39bf6c9b9c79da`.

Only DeEsser is rebuilt. Other 20 nested source ZIPs and every entry in the existing all-plugin binary suite are verified unchanged. The existing full compiled-suite ZIP remains intact; the separate DeEsser VST3 ZIP contains this revision. The updated source suite contains the revised DeEsser source. Previous MP3 calibration notes are kept as historical source documentation and superseded by this report.
