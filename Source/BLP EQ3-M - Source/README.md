# BLP EQ3-M

Extract the Windows x64 ZIP and copy the entire BLP EQ3-M.vst3 folder into C:\Program Files\Common Files\VST3. Replace the previous bundle and rescan your DAW's plug-ins.

Mid Sweep ranges from 100 to 8000 in whole Hz and defaults to 5000. Low, Mid, and High display their gain on hover.

To build from source, install Visual Studio 2022 with Desktop development with C++, Git and CMake 3.22 or newer. Run:

    cmake -S . -B build -A x64
    cmake --build build --config Release --target EQ3M_VST3

To use a local JUCE 9.0.5 source tree, pass -DJUCE_DIR=C:/path/to/JUCE when configuring. The bundle is under build/EQ3M_artefacts/Release/VST3.

Project source is provided under AGPL-3.0-or-later. See JUCE-LICENSE.md for JUCE terms.


## Calibrated +30 dB endpoints

The default Low filter is an RBJ second-order low shelf at 220 Hz with slope S=1
(Q=1/sqrt(2)). The default Mid filter is an RBJ peaking filter at 5000 Hz with Q=1.20.
The default High filter is an RBJ second-order high shelf at 7000 Hz with slope S=1
(Q=1/sqrt(2)). All three gain controls retain their +30 dB endpoint and -30 dB limit.
The shelf API receives Q, not S.

Only individual +30 dB endpoints are calibrated to the supplied targets.
Other gains use the standard RBJ continuation and combined bands use the existing
cascade. Neither behavior has been established as a BandLab match.
Saved parameter values, including existing frequency and Q selections, are restored
unchanged. Create a fresh instance or reset the relevant parameters to obtain the
new calibrated defaults.

Coefficients use the processing sample rate and are normalized by a0.
The existing 30 ms parameter smoothing is retained, and each 8-sample coefficient
update now interpolates sample by sample with exact destination assignment.
There is no saturation, clipping or loudness compensation. The neutral path is
sample-exact unity with zero initial filter state.

## Repeating the validation

Configure with -DEQ3M_BUILD_TESTS=ON and a local JUCE path if needed, then build
EQ3M_VST3 and EQ3MHostTests. Run EQ3MHostTests.exe with the VST3 bundle path and
a render directory. Run python Tests/AnalyzeResponse.py followed by that directory
with NumPy installed. The host renders isolated +30 dB stereo sweeps at 44.1 and
48 kHz and tests defaults, unity, bypass, stereo, stability, automation and state restore.
The analysis compares frequency-response shape to independent RBJ formulas.
See TEST-RESULTS.md for measured errors and the limits of the comparison.
