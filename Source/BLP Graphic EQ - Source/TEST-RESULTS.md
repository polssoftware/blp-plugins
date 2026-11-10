# BLP Graphic EQ response validation

Updated on 2026-11-09. Only BLP Graphic EQ was modified and rebuilt for Windows x64 VST3.

The original DSP used seven RBJ peaking filters at Q 1.10. The archived baseline measured exactly +15 dB at each labelled frequency, at both 44.1 and 48 kHz.

**Implemented endpoints**

The first six +15 dB slider endpoints now use the six independently specified pole/zero frequency and Q pairs from the request. Their coefficient triplets follow C(t,Q) exactly, using the zero triplet as numerator, the pole triplet as denominator and denominator-a0 normalization. The Q values are pole/zero damping parameters.

The 6.4k slider now uses the specified first-order high shelf, with +13.29345 dB high-frequency gain and 1740.5777 Hz midpoint. Its second-order numerator and denominator terms are zero. DC gain is unity. The small approximately -0.01 dB offsets in the reported measurements were omitted as requested.

There is no added global boost, MP3-export roll-off, limiter, clipping or saturation stage. UI layout, slider limits, Output, bypass and parameter identifiers retain their current behavior.

**Response validation**

The comparison target is the supplied fitted transfer function, calculated independently in Python using extended-precision intermediate arithmetic. Original BandLab audio exports were not provided, so residual fit error against BandLab itself cannot be measured here.

Each isolated band was set to +15 dB with the other six bands at zero and Output at 0 dB. Identical stereo logarithmic sweeps ran from 10 Hz to 0.49 Fs over 12 seconds, followed by a 2-second tail. Input peak was -80 dBFS. The WAVs use stereo 32-bit float PCM. Input and output FFT ratios were compared over 20 Hz to 20 kHz. A separate 150,000-frequency logarithmic grid from 10 Hz to just below Nyquist tested coefficient response.

| Rate, Hz | Max numerical magnitude error, dB | Max rendered VST3 sweep error, dB |
| ---: | ---: | ---: |
| 44100 | 6.59e-13 | 0.00000450 |
| 48000 | 2.23e-11 | 0.00000466 |

The source-DSP sweeps and loaded-VST3 sweeps agreed to within 1.18e-38 in sample amplitude. The largest coefficient difference from the independent endpoint calculation was 6.67e-16.

At 44.1 kHz the responses are as follows. The shelf row reports its midpoint and high-frequency asymptote, not a bell peak.

| Band | Peak frequency, Hz | Peak / shelf gain, dB | Gain at label, dB | Max VST3 sweep error, dB |
| --- | ---: | ---: | ---: | ---: |
| 100 | 95.588 | 18.814360 | 18.579562 | 0.00000394 |
| 200 | 269.668 | 18.750734 | 14.798374 | 0.00000436 |
| 400 | 393.622 | 18.768051 | 18.731379 | 0.00000432 |
| 800 | 789.006 | 18.710908 | 18.678502 | 0.00000415 |
| 1600 | 1542.659 | 18.803755 | 18.668810 | 0.00000450 |
| 3200 | 3243.390 | 18.701637 | 18.673878 | 0.00000434 |
| 6400 | 1740.578 midpoint | 13.293450 | 12.219471 | 0.00000081 |

The 200 slider is deliberately tuned near 270 Hz at this endpoint. The 6.4k control is deliberately a broad shelf whose gain at 6.4 kHz is approximately 12.21947 dB at 44.1 kHz. These are consequences of the supplied fits.

48 kHz was validated separately against coefficients recalculated at 48 kHz. It was not compared with new BandLab 48 kHz exports, so cross-sample-rate BandLab matching remains unverified. Full-bandwidth formulas were used above 17 kHz with no artificial sharp low-pass.

**Neutral, stability, channels and transitions**

A newly prepared neutral processor passed sample-exact unity tests. Neutral numerator and denominator coefficients are identical. Existing filter state is retained during parameter changes, so tails decay naturally.

Tests covered 8, 22.05, 44.1, 48, 88.2, 96, 192 and 384 kHz, slider gains from -15 to +15 dB in 0.05 dB increments, and 33 intermediate coefficient positions per adjacent pair. All tested poles and zeros remained inside the unit circle. The largest radius over those rates was 0.999702726071, at the high sample-rate end. Maximum DC amplitude error was 1.85e-10.

Stereo outputs were bit-identical for identical inputs and the silent channel stayed exactly silent when only the other channel was excited. Prepared bypass was sample-exact. Random automation produced finite output without runaway growth. At -100 dBFS noise input, the largest stress-test output was 0.000476567, about -66.44 dBFS.

The existing 25 ms slider smoothing and 32-sample coefficient ramps were retained. Each ramp now assigns its exact destination at the final sample to avoid cumulative coefficient drift. Output and bypass retain 10 ms smoothing. The largest adjacent-sample step in the specified 0.001-amplitude DC transition test was 0.000013007 across all tested rates. This is a tested transition result, not a proof for every possible modulation sequence.

The compiled VST3 passed mono/stereo tests at 44.1, 48, 88.2 and 96 kHz, automated band/Output/bypass measurements, project-state restoration, a .vstpreset save/load round trip and editor rendering. Interactive DAW listening was not part of this validation.

**Headroom**

The individual fitted bell endpoints peak around +18.7 to +18.8 dB. The provisional all-maximum cascade peaks at +45.14364 dB at 44.1 kHz and +45.15186 dB at 48 kHz. Input attenuation must account for that gain. Output bottoms out at -15 dB, so Output attenuation alone cannot compensate for the worst all-max cascade.

A float test with all bands at maximum produced approximately 1.81 full-scale peak. Scaling the input by 100 scaled the output by 100, with maximum normalized sample discrepancy 1.64e-9. This confirms the tested path remains linear above full scale and applies no clipping or saturation. The downloadable low-level combined-band WAVs remain below full scale.

**Provisional assumptions and additional renders**

Only the isolated +15 dB endpoints have measurement support. For bands 100 through 3200, intermediate positions interpolate pole/zero frequencies and Q values logarithmically from a common geometric-mean pair at zero. Negative positions exchange the positive position's poles and zeros to give a reciprocal response. The shelf endpoint gain scales linearly in dB. These choices are stable in the tests and smoothly interpolated, but are implementation assumptions.

Combined bands are cascaded in the existing order. Their aggregate response has not been recovered from BandLab measurements. Reopening an existing preset preserves its slider values, but the new DSP changes the resulting response.

Additional BandLab renders are needed for isolated intermediate positions and negative gains, combinations of bands, actual control transitions, and repeated 48 kHz measurements. Matched bypass/Output renders would be needed to distinguish a real DC offset from export uncertainty.

**Included evidence**

The validation archive includes per-band response CSVs, endpoint coefficient CSVs, error summaries, baseline results, DSP/host logs, isolated loaded-VST3 sweep WAVs, input sweep WAVs and low-level provisional all-max cascade WAVs. Each isolated WAV corresponds to a single +15 dB slider and unchanged Output.

VST3 binary SHA-256
fb46ad4683aeabc165abc3aab52ece4acdeac1dcb1c9209af18a1da4e0404382
