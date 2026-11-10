# BLP EQ3-M response validation

Updated on 2026-11-10. The request's name EQM-3 was interpreted as the existing BLP EQ3-M. Only this plugin was modified and rebuilt as a Windows x64 VST3.

**Implementation**

The inspected source defaulted to a 120 Hz low shelf, an 8000 Hz bell with Q=1.0 and an 8000 Hz high shelf. Defaults now match the supplied targets.

| Control | Filter | Frequency | Q | Positive gain endpoint |
| --- | --- | ---: | ---: | ---: |
| Low | Second-order RBJ low shelf | 220 Hz midpoint | 1/sqrt(2), approximately 0.70710678 | +30 dB |
| Mid | RBJ peaking/bell | 5000 Hz center | 1.20 | +30 dB |
| High | Second-order RBJ high shelf | 7000 Hz midpoint | 1/sqrt(2), approximately 0.70710678 | +30 dB |

The JUCE ArrayCoefficients shelf API used here expects Q. The supplied 1/sqrt(2) value produces beta=sin(omega)*sqrt(2*A), equivalent to the standard RBJ shelf with slope S=1. It is not passed to an S parameter.

Coefficient calculation uses the actual processing sample rate, and every numerator and denominator coefficient is divided by a0. The existing 30 ms parameter smoothing remains active. The previous coefficient replacement every 8 samples now ramps coefficients per sample over the same 8-sample interval and assigns the exact target at each ramp's end.

The gain control limits remain -30 to +30 dB. There is no added clipping, saturation or gain compensation. Output remains a separate unchanged parameter. UI layout, plugin identifiers and parameter IDs are retained.

The calibrated frequencies and Q are defaults. Mid Sweep still operates over 100 to 8000 Hz and resets to 5000 Hz. Existing saved frequency/Q values are preserved on state restoration. A fresh instance or reset parameters use the new calibration. Loading an older preset that stores 8000 Hz will continue to use that stored value.

**Loaded-VST3 sweep validation**

Six isolated +30 dB sweeps were rendered through the rebuilt, loaded VST3, one per band at 44.1 and 48 kHz. For each render the other two gains were zero, frequency/Q controls were at their calibrated defaults and Output was 0 dB.

Each input is a 12-second logarithmic sweep from 10 Hz to 0.49 Fs with -80 dBFS peak, 20 ms endpoint fades and a 2-second tail. WAVs are stereo 32-bit float. Both channels were identical and their rendered outputs were checked for exact equality.

The response was calculated from the complex output/input FFT ratio over 20 Hz to 20 kHz and compared with an independent Python implementation of standard RBJ formulas, explicitly using S=1 for the shelves. Shape error subtracts the mean dB residual only during analysis. No level correction was applied to the plugin or WAVs.

| Rate, Hz | Low shape error, dB | Mid shape error, dB | High shape error, dB |
| ---: | ---: | ---: | ---: |
| 44100 | 0.00002813 | 0.00000912 | 0.00000434 |
| 48000 | 0.00002615 | 0.00000722 | 0.00000464 |

The worst shape error was 0.00002814 dB at 44.1 kHz and 0.00002615 dB at 48 kHz. The worst absolute magnitude errors were 0.00002819 dB and 0.00002610 dB respectively. The largest mean residual offset was less than 0.00000025 dB. The largest shape RMS residual was below 0.00000483 dB.

A separate 150,000-point logarithmic frequency grid from 10 Hz to just below Nyquist compared normalized coefficient responses with the independent formulas. Maximum numerical magnitude discrepancy was below 3e-14 dB.

At +30 dB the shelf gain is +15 dB at its midpoint, rising toward the +30 dB plateau. The Mid bell is +30 dB at 5000 Hz. These half-gain shelf midpoints are the intended standard RBJ behavior.

These error figures measure agreement with the requested filter targets. No original BandLab audio exports were supplied, so residual error relative to those recordings cannot be quantified.

**Unity, stability and parameter changes**

A freshly prepared neutral processor passed sample-exact unity tests. Stereo output was exact for identical input channels. Settled bypass passed direct sample comparisons. Pole and zero roots stayed inside the unit circle for frozen gain settings from -30 to +30 dB in 0.1 dB steps at both sample rates, including 9 points along each adjacent coefficient interpolation. The largest pole/zero radius in that grid was 0.991449184109.

Random automation covered all three band gains, frequency controls, Mid Q, Output and bypass. Output remained finite, both channels agreed exactly and no runaway growth was observed. This is implementation stability testing of the provisional gain mapping, not measurement recovery of those settings.

On a 0.0001-amplitude DC input, the tested all-gain and bypass transitions had a maximum adjacent-sample step of 0.000077598. This specific result does not establish matching BandLab transition behavior or every possible automation sequence.

The loaded plugin reported the new normalized defaults. Existing Mid frequency and Q choices survived a state save/restore. The editor opened at its existing 300 x 270 dimensions.

All low-level sweep renders remained below -49.95 dBFS. A +30 dB boost requires corresponding signal headroom, and the processing path applies no limiter or saturation.

**Unverified assumptions**

Only individual +30 dB endpoints at the specified frequency/Q settings are calibrated. Cuts and intermediate settings continue to use the standard RBJ gain law provisionally. The three filters remain cascaded in their existing order, and combined-band behavior has not been established as a BandLab match.

Additional BandLab renders would be needed at intermediate gains, negative gains, alternate Mid Sweep settings, combined bands and parameter transitions. Interactive DAW listening was not included in these tests. Validation used the existing local JUCE 8.0.12 framework; the source's dependency configuration was retained.

**Evidence**

The validation ZIP contains the isolated loaded-VST3 output sweeps, corresponding inputs, per-band response CSVs, endpoint coefficients, error summary and host-test log. Source tests and the independent response analysis are included in the updated source ZIP.

VST3 binary SHA-256
126b0dee98f4e8e68ca71362f4db9b4260f5e9197fe7d7b8378178c5e9785640
