BLP TecLab BA-2A - calibrated default response (2026-11-10)

Windows x64 VST3. Copy the entire BLP TecLab BA-2A.vst3 bundle to your VST3 folder and rescan in the DAW.

This revision fits the user's supplied dry recording and BandLab default-setting render. See Response Validation.md for numerical results, measurement limits, and source/build checks. It is an independent approximation, not proprietary BandLab code.

Defaults and parameter IDs are unchanged: Squeeze 70%, Gain 60%, Compress, Mix 100%, Output 0 dB. The GUI and title are unchanged. Existing sessions retain their parameter values but will use the revised processing after replacing the plugin.

Processing changes: stereo-linked mean-square detector; fitted fast attack/release and wide soft knee; revised compression ratio and threshold; shorter, program-dependent reduction recovery; Gain controls a fixed makeup level instead of varying with instantaneous reduction. At default Gain the calibrated makeup is about +0.511 dB. Gain changes by 0.36 dB per percent around 60%. Squeeze changes threshold continuously around the measured 70% setting. Limit mode and other control settings are provisional and have not been matched to BandLab. Squeeze 0 disables compression and makeup after smoothing settles. The existing -1 dBFS soft safety stage is retained; it is inactive for the supplied default-setting comparison and is not evidence of measured BandLab saturation.

30 ms parameter smoothing remains. Bypass crossfades to the unaltered input. Mix/Output are still host-automatable. No lookahead, latency, input file detection, or per-recording gain envelope is embedded.

Build with Visual Studio 2022, CMake and separately licensed JUCE source:
cmake -S . -B build -A x64 -DJUCE_DIR="C:/path/to/JUCE"
cmake --build build --config Release --parallel
The calibration build used JUCE 8.0.12. JUCE licensing requirements still apply.

Tests.cpp provides the original DSP smoke checks. Optional BA2A_BUILD_HOST_TESTS=ON builds Validation/HostTests.cpp, which loads the compiled VST3, renders supplied interleaved stereo float32 dry-44100.f32 / dry-48000.f32 inputs, and tests bypass, dry mix, state, stereo and parameter extremes. Invoke BA2AHostTests with the VST3 bundle path and a directory containing those inputs. Test inputs are not embedded in the source package.
