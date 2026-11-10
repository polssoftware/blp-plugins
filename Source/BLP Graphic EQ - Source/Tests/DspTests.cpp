#include "../Source/EqDSP.h"
#include <complex>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
double magnitude(const EqDSP::Coeff& c, double rate, double frequency) {
    const auto z = std::polar(1.0, -6.2831853071795864769 * frequency / rate);
    return std::abs((c[0] + c[1] * z + c[2] * z * z) / (1.0 + c[3] * z + c[4] * z * z));
}
double rootRadius(double a, double b) {
    const auto discriminant = std::sqrt(std::complex<double>(a * a - 4.0 * b));
    return std::max(std::abs((-a + discriminant) / 2.0), std::abs((-a - discriminant) / 2.0));
}
int main() {
    try {
        std::mt19937 random(4811);
        std::uniform_real_distribution<float> noise(-1.0f, 1.0f);
        double maximumPole = 0, maximumZero = 0, maximumDcError = 0;
        double maximumStep = 0, maximumAutomationPeak = 0;
        for (double rate : {8000., 22050., 44100., 48000., 88200., 96000., 192000., 384000.}) {
            for (size_t band = 0; band < 7; ++band) {
                EqDSP::Coeff previous {};
                for (int step = 0; step <= 600; ++step) {
                    const auto c = EqDSP::bandCoefficients(rate, band, -15.0 + step * 0.05);
                    for (auto value : c) check(std::isfinite(value), "non-finite coefficient");
                    const auto pole = rootRadius(c[3], c[4]);
                    const auto zero = rootRadius(c[1] / c[0], c[2] / c[0]);
                    check(pole < 1.0 && zero < 1.0, "unstable or non-minimum-phase filter");
                    maximumPole = std::max(maximumPole, pole);
                    maximumZero = std::max(maximumZero, zero);
                    maximumDcError = std::max(maximumDcError, std::abs(magnitude(c, rate, 0) - 1.0));
                    check(std::abs(magnitude(c, rate, 0) - 1.0) < 2e-8, "DC normalization");
                    const auto inverse = EqDSP::bandCoefficients(rate, band, 15.0 - step * 0.05);
                    for (double frequency : {30., 100., 997., 1740.5777, rate * .45})
                        check(std::abs(magnitude(c, rate, frequency) * magnitude(inverse, rate, frequency) - 1.0)
                              < 2e-8, "provisional reciprocal mapping");
                    if (step > 0) {
                        // The normalized denominator's Schur region is convex.
                        for (int subdivision = 0; subdivision <= 32; ++subdivision) {
                            const auto mix = subdivision / 32.0;
                            const auto a1 = previous[3] + mix * (c[3] - previous[3]);
                            const auto a2 = previous[4] + mix * (c[4] - previous[4]);
                            check(rootRadius(a1, a2) < 1.0, "unstable coefficient interpolation");
                            const auto b0 = previous[0] + mix * (c[0] - previous[0]);
                            const auto b1 = previous[1] + mix * (c[1] - previous[1]);
                            const auto b2 = previous[2] + mix * (c[2] - previous[2]);
                            check(rootRadius(b1 / b0, b2 / b0) < 1.0, "non-minimum-phase coefficient interpolation");
                        }
                    }
                    previous = c;
                }
                const auto c = EqDSP::bandCoefficients(rate, band, 0.0);
                check(c[0] == 1.0 && c[1] == c[3] && c[2] == c[4], "zero transfer must be exact unity");
            }
            EqDSP flat;
            flat.prepare(rate, {}, 0, false);
            std::array<float, 257> left {}, right {};
            float* data[] {left.data(), right.data()};
            for (int block = 0; block < 20; ++block) {
                for (size_t i = 0; i < left.size(); ++i)
                    left[i] = right[i] = .01f * noise(random);
                const auto original = left;
                flat.process(data, 2, 257, {}, 0, false);
                check(left == original && right == original, "zero slider path must be sample-exact unity");
            }

            EqDSP stress;
            stress.prepare(rate, {}, 0, false);
            for (int block = 0; block < 1600; ++block) {
                EqDSP::Gains gains;
                for (auto& gain : gains) gain = 15.0f * noise(random);
                for (size_t i = 0; i < left.size(); ++i) left[i] = right[i] = .00001f * noise(random);
                stress.process(data, 2, 257, gains, 15 * noise(random), block % 11 == 0);
                for (size_t i = 0; i < left.size(); ++i) {
                    check(std::isfinite(left[i]) && std::abs(left[i]) < 1.0f, "automation runaway");
                    check(left[i] == right[i], "stereo mismatch");
                    maximumAutomationPeak = std::max(maximumAutomationPeak, double(std::abs(left[i])));
                }
            }

            // No cross-channel state, including after nonzero filters have run.
            EqDSP::Gains gains;
            gains.fill(15);
            EqDSP isolated;
            isolated.prepare(rate, gains, 0, false);
            left.fill(.00001f);
            right.fill(0);
            isolated.process(data, 2, 257, gains, 0, false);
            for (auto value : right) check(value == 0, "stereo crosstalk");
            EqDSP bypassed;
            bypassed.prepare(rate, gains, 15, true);
            const auto original = left;
            bypassed.process(data, 2, 257, gains, 15, true);
            check(left == original, "bypass is not exact");

            EqDSP smooth;
            smooth.prepare(rate, {}, 0, false);
            float value, previous = 0;
            float* one[] {&value};
            for (int i = 0; i < int(rate * 1.5); ++i) {
                value = .001f;
                smooth.process(one, 1, 1, i < int(rate * .1) ? EqDSP::Gains{} : gains,
                               i < int(rate * .1) ? 0 : -6, i > int(rate));
                if (i > 10) maximumStep = std::max(maximumStep, std::abs(double(value - previous)));
                previous = value;
            }
            std::cout << "PASS " << rate << " Hz: normalization, minimum phase, exact unity, stereo, bypass, automation\n";
        }
        check(maximumStep < .0002, "parameter/output/bypass transition discontinuity");
        EqDSP invalid;
        EqDSP::Gains bad;
        bad.fill(std::numeric_limits<float>::quiet_NaN());
        invalid.prepare(std::numeric_limits<double>::quiet_NaN(), bad,
                        std::numeric_limits<float>::quiet_NaN(), false);
        float sample = .01f;
        float* pointer[] {&sample};
        invalid.process(pointer, 1, 1, bad, std::numeric_limits<float>::infinity(), false);
        check(std::isfinite(sample), "invalid parameter handling");
        std::cout.precision(12);
        std::cout << "max_pole_radius=" << maximumPole << "\nmax_zero_radius=" << maximumZero
                  << "\nmax_dc_amplitude_error=" << maximumDcError << "\nmax_dc_transition_step=" << maximumStep
                  << "\nmax_automation_output=" << maximumAutomationPeak << "\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << "\n";
        return 1;
    }
}
