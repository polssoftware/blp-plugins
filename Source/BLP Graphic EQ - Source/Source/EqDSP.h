#pragma once
#include <array>
#include <algorithm>
#include <cmath>

// Endpoint fits supplied for one band at +15 dB. Other gains use a provisional
// reciprocal, log-parameter interpolation, not a recovered BandLab gain law.
class EqDSP {
public:
    static constexpr std::array<double, 7> frequencies {100,200,400,800,1600,3200,6400};
    using Gains = std::array<float, 7>;
    using Coeff = std::array<double, 5>; // b0,b1,b2,a1,a2, with a0 = 1

    static Coeff coefficients(double rate, double frequency, double db) noexcept {
        size_t band = 0;
        for (size_t i = 1; i < frequencies.size(); ++i)
            if (std::abs(frequencies[i] - frequency) < std::abs(frequencies[band] - frequency))
                band = i;
        return bandCoefficients(rate, band, db);
    }

    static Coeff bandCoefficients(double rate, size_t band, double db) noexcept {
        constexpr double pi = 3.14159265358979323846;
        rate = validRate(rate);
        db = std::isfinite(db) ? std::clamp(db, -15.0, 15.0) : 0.0;
        const double position = db / 15.0;
        if (band >= 6) {
            constexpr double midpoint = 1740.5777;
            constexpr double endpointGain = 13.29345;
            const double a = std::pow(10.0, endpointGain * position / 40.0);
            const double t = std::tan(pi * midpoint / rate);
            const double zero = t / a;
            const double pole = t * a;
            const double a0 = 1.0 + 1.0 / pole;
            const double a1 = (1.0 - 1.0 / pole) / a0;
            if (db == 0.0)
                return {1.0, a1, 0.0, a1, 0.0};
            return {(1.0 + 1.0 / zero) / a0, (1.0 - 1.0 / zero) / a0, 0.0, a1, 0.0};
        }

        const auto& fit = endpoints[band];
        const auto interpolate = [position](double pole, double zero) {
            // At +15 dB return the supplied parameters exactly.
            if (position == 1.0) return pole;
            if (position == -1.0) return zero;
            return std::exp(0.5 * (std::log(pole) + std::log(zero))
                          + 0.5 * position * (std::log(pole) - std::log(zero)));
        };
        const double poleFrequency = interpolate(fit.poleFrequency, fit.zeroFrequency);
        const double zeroFrequency = interpolate(fit.zeroFrequency, fit.poleFrequency);
        const double poleQ = interpolate(fit.poleQ, fit.zeroQ);
        const double zeroQ = interpolate(fit.zeroQ, fit.poleQ);
        const auto triplet = [rate, pi](double frequencyHz, double q) {
            const double t = std::tan(pi * frequencyHz / rate);
            const double inverseT = 1.0 / t;
            const double damping = inverseT / q;
            const double squared = inverseT * inverseT;
            return std::array<double, 3> {1.0 + damping + squared,
                                         2.0 - 2.0 * squared,
                                         1.0 - damping + squared};
        };
        const auto denominator = triplet(poleFrequency, poleQ);
        const double a1 = denominator[1] / denominator[0];
        const double a2 = denominator[2] / denominator[0];
        if (db == 0.0)
            return {1.0, a1, a2, a1, a2};
        const auto numerator = triplet(zeroFrequency, zeroQ);
        return {numerator[0] / denominator[0], numerator[1] / denominator[0],
                numerator[2] / denominator[0], a1, a2};
    }

    void prepare(double rate, const Gains& initial, float output, bool bypass) noexcept {
        sr = validRate(rate);
        smoothing = 1.0 - std::exp(-32.0 / (sr * 0.025));
        for (size_t band = 0; band < gain.size(); ++band) {
            gain[band] = validGain(initial[band]);
            coeff[band] = destination[band] = bandCoefficients(sr, band, gain[band]);
        }
        delta = {};
        memory = {};
        countdown = 0;
        out = std::pow(10.0, validGain(output) / 20.0);
        wet = bypass ? 0.0 : 1.0;
        sampleSmoothing = 1.0 - std::exp(-1.0 / (sr * 0.010));
    }

    void process(float* const* data, int channels, int samples, const Gains& target,
                 float output, bool bypass) noexcept {
        const double outTarget = std::pow(10.0, validGain(output) / 20.0);
        const double wetTarget = bypass ? 0.0 : 1.0;
        for (int sample = 0; sample < samples; ++sample) {
            if (countdown == 0) {
                for (size_t band = 0; band < gain.size(); ++band) {
                    const float nextGain = validGain(target[band]);
                    gain[band] += float(smoothing * (nextGain - gain[band]));
                    if (std::abs(nextGain - gain[band]) < 0.00001f)
                        gain[band] = nextGain;
                    destination[band] = bandCoefficients(sr, band, gain[band]);
                    for (size_t k = 0; k < 5; ++k)
                        delta[band][k] = (destination[band][k] - coeff[band][k]) / 32.0;
                }
                countdown = 32;
            }
            --countdown;
            for (size_t band = 0; band < gain.size(); ++band)
                for (size_t k = 0; k < 5; ++k)
                    coeff[band][k] = countdown == 0 ? destination[band][k] : coeff[band][k] + delta[band][k];

            out += sampleSmoothing * (outTarget - out);
            wet += sampleSmoothing * (wetTarget - wet);
            if (std::abs(wetTarget - wet) < 1e-9)
                wet = wetTarget;
            for (int channel = 0; channel < std::min(channels, 2); ++channel) {
                const double dry = std::isfinite(data[channel][sample]) ? data[channel][sample] : 0.0;
                double x = dry;
                for (size_t band = 0; band < gain.size(); ++band) {
                    auto& state = memory[size_t(channel)][band];
                    const auto& c = coeff[band];
                    const double y = c[0] * x + state[0];
                    state[0] = c[1] * x - c[3] * y + state[1];
                    state[1] = c[2] * x - c[4] * y;
                    x = y;
                }
                data[channel][sample] = float(dry + wet * (out * x - dry));
            }
        }
    }

private:
    struct Endpoint {
        double poleFrequency, poleQ, zeroFrequency, zeroQ;
    };
    static constexpr std::array<Endpoint, 6> endpoints {{
        {95.5658, 2.62966, 94.0484, 0.306305},
        {269.5379, 2.03929, 260.6956, 0.243495},
        {393.5380, 2.88446, 387.3492, 0.337734},
        {788.8211, 3.13986, 776.4935, 0.370041},
        {1542.2678, 2.43143, 1513.3170, 0.284443},
        {3242.5899, 2.88646, 3183.5981, 0.341649}
    }};
    static double validRate(double rate) noexcept {
        return std::isfinite(rate) && rate >= 8000.0 ? rate : 44100.0;
    }
    static float validGain(float db) noexcept {
        return std::isfinite(db) ? std::clamp(db, -15.0f, 15.0f) : 0.0f;
    }

    double sr = 44100, smoothing = 0.02, sampleSmoothing = 0.002, out = 1, wet = 1;
    int countdown = 0;
    Gains gain {};
    std::array<Coeff, 7> coeff {}, delta {}, destination {};
    std::array<std::array<std::array<double, 2>, 7>, 2> memory {};
};
