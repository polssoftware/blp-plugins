# BLP BL1176

Windows x64 VST3 dynamics effect. Calibrated against the user's five BandLab default-setting WAV renders on 2026-11-10; see Calibration.md for results and limits.

New instances and the Default factory program use **12:1**. Saved projects retain their stored ratios. Other defaults remain Input +10 dB, Output 0 dB, Attack 1.2 and Release 5.0.

Input drives audio and stereo-linked detection. Output adjusts level after compression in addition to a small ratio-specific calibration correction. Turning Attack or Release up makes it faster; their effective default times now depend on the fitted ratio model. Other knob positions remain provisional. Ratio selects 4:1, 8:1, 12:1, 20:1 or All, each with its own calibration. Both stereo channels receive the same reduction. A shared first-order high-pass/low-pass coloration model follows compression. Controls and bypass smooth over 5 ms. Bypass returns untouched dry input after its ramp.

The GUI, BLP BL1176 title and bypass styling are unchanged. No lookahead, inserted audio delay, saturation or output limiter was added. Reported latency is zero, and a 0.25-second tail allowance accounts for filter settling. Floating-point headroom is preserved. This is a closer measured approximation, not a complete recovery of BandLab's algorithm; other input levels and settings remain unverified.

Install by extracting the compiled ZIP and copying the entire BLP BL1176.vst3 bundle into your VST3 folder, then rescan the DAW. Existing sessions retain parameter values but use the revised DSP.

## Build

Use Visual Studio 2022 Desktop development with C++, CMake 3.22+ and Git. The original FetchContent configuration obtains JUCE 8.0.8. The local calibration build used JUCE 8.0.12.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target BLP1176_VST3
```

Enable BLP1176_BUILD_TESTS to build BL1176UnitTests and BL1176RenderHost. The latter loads an actual VST3 and takes a bundle path plus a directory with stereo interleaved float32 dry-44100.f32 and dry-48000.f32 inputs. Original vocal files are not included in source. See Calibration.md and Calibration/ for numerical results and test logs.
