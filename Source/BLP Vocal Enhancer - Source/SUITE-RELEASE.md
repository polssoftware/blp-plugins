# BLP Vocal Enhancer - suite release 2026-11-10

GUI subtitle: **v1.0.0**. CMake and compiled plugin version: **1.0.0**.

This release applies the approved small white regular-weight version label beside the bold title. Header size, controls, power-button behavior and plugin identity are preserved. The latest DSP and existing validation material are included. No additional DSP calibration was performed for this label/metadata rebuild; all non-theme C/C++ source files were verified byte-for-byte against the latest input source archive.

The six v1.0.1 plugins are BL1176, DeEsser, DIGI Comp, EQ3-M, Graphic EQ and TecLab BA-2A. The other fifteen use v1.0.0, as requested. BL1176 retains its new 12:1 fresh-instance and Default-program ratio; saved ratios remain compatible.

Built for Windows x64 using the existing JUCE 8.0.12 build runtime. Each project's dependency configuration and build instructions remain included. Plugin class IDs are unchanged. The compiled plugin passed loading/stereo processing checks, and native GUI checks matched the approved header preview.

Earlier calibration reports describe the DSP calibration builds and their limitations; their old binary hashes are historical. The binary hash for this version-label build is:

`16c80be803453fe2e42305920b5f6304a6ebfa3c892d5d58db44dbd30fe2bec5`
