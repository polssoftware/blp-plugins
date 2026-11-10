# BLP DIGI Comp calibration - 2026-11-10

Only DIGI Comp changed. The user confirmed unchanged defaults for Attack, Release, Threshold and Knee, with ratio set to 1:1 or 20:1. Source defaults are 18 ms, 222 ms, -17.5 dB and 6 dB. UI, defaults, IDs and bypass remain unchanged.

BandLab exports are 44.1 kHz; the dry and BLP files are 48 kHz. BandLab exports have approximately 35 samples of delay on a 48 kHz timeline. Comparisons correct this delay using a windowed-sinc common-rate conversion. A fractional export delay and conversion differences remain: even BandLab's 1:1 recording differs slightly from the dry waveform. BLP 1:1 is effectively exact dry audio.

At 20:1 BandLab adds substantial level and reaches the 16-bit export ceiling, while the old BLP attenuates the signal. Thus both dynamics and makeup differ. The fitted model uses the same feed-forward linked peak topology with revised attack/release scaling, wider effective knee, small threshold offset and ratio-dependent makeup. No clipping or limiter was added to imitate the overloaded export. Floating output can exceed full scale; use normal downstream headroom.

Fitted effective defaults at 20:1: attack 1.74295 ms; release 164.42870 ms; threshold -18.19329 dB; knee 29.99937 dB; makeup 11.87703 dB. These constants are not a unique identification of BandLab's internal topology. The displayed controls keep their original values; internal calibrated scaling applies. Makeup scales with 1 - 1/ratio, preserving exact unity at 1:1. Other control positions use provisional continuous scaling. Existing parameter and bypass smoothing remain.

Fit used alternate 0.5-second sections excluding samples near the reference export ceiling; remaining sections checked the fit. Model training error: -19.185 dB; checking error: -18.160 dB. This is the same clip split into sections, not independent material.

## Actual loaded-VST3 measurements

Relative waveform error is RMS(output-reference)/RMS(reference), not a listening score. No validation output is level normalized.

| Ratio | Original error % | Revised error % | Original RMS dBFS | Revised RMS dBFS |
|---|---:|---:|---:|---:|
| 1:1 | 4.903 | 4.903 | -18.438 | -18.438 |
| 20:1 | 60.599 | 12.442 | -23.345 | -15.618 |

The compiled Windows x64 VST3 rendered at 44.1 and 48 kHz, with finite output in both cases and sample-exact unity at 1:1. A native BandLab 48 kHz render was not supplied; its cross-rate equivalence remains unverified. No interactive DAW listening test was performed. Source regression host and numerical results are included.

This is a closer approximation for the supplied 20:1 recording, not an exact match or full recovery of the compressor. Export clipping limits identification of peaks, and the fit's knee reached its allowed boundary. More useful evidence: an unclipped BandLab 20:1 render with the dry input lowered 12 dB, matching BLP render, plus 2:1/4:1/10:1 and attack/release variations. Those would help distinguish makeup, knee, detector behavior and timing without export overload.

Prior compiled suite is preserved; other 20 source ZIPs are verified unchanged. Replacing this plugin changes existing sessions' sound while retaining parameter values. The source ZIP opens directly to source files.
