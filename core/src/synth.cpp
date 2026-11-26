#include "filon/core/synth.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace filon {
namespace {

constexpr double TAU = 2 * std::numbers::pi;
constexpr double FLOOR = .0001;  // fin des rampes de volume (exponentialRampToValueAtTime(.0001))

double mid(int n) { return 440 * std::pow(2.0, (n - 69) / 12.0); }

double polyBlep(double t, double dt) {  // adoucit les sauts des ondes carrée et en dents de scie (moins de repliement)
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1;
    }
    if (t > 1 - dt) {
        t = (t - 1) / dt;
        return t * t + t + t + 1;
    }
    return 0;
}

}  // namespace

Synth::Synth(int rate, std::uint32_t seed) : rate_(rate), rng_(seed) {
    noiseBuf_.resize(static_cast<std::size_t>(rate * .3));
    for (float& s : noiseBuf_) s = static_cast<float>(rng_.next() * 2 - 1);
}

double Synth::time() const { return static_cast<double>(pos_.load()) / rate_; }

void Synth::setVolumes(double music, double sfx) {
    musicGain_ = std::clamp(music, 0.0, 1.0) * .5;
    sfxGain_ = std::clamp(sfx, 0.0, 1.0) * .6;
}

void Synth::tone(double f, double d, Wave w, double v, double slide, double when, bool music) {
    const double t = when < 0 ? time() : when;
    Voice vo{};
    vo.start = static_cast<std::int64_t>(t * rate_);
    vo.length = static_cast<std::int64_t>((d + .03) * rate_);  // o.stop(t + d + .03)
    vo.f0 = f;
    vo.f1 = slide ? std::max(30.0, f + slide) : f;
    vo.v = v;
    vo.d = d;
    vo.wave = w;
    vo.music = music;
    std::lock_guard<std::mutex> lock(m_);
    voices_.push_back(vo);
}

void Synth::noise(double d, double v, double cutoff) {
    Voice vo{};
    vo.start = pos_.load();
    vo.length = static_cast<std::int64_t>((d + .02) * rate_);
    vo.v = v;
    vo.d = d;
    vo.noise = true;
    // passe-bas biquadratique (Q de Web Audio : 1 dB de résonance)
    const double w0 = TAU * std::min(cutoff, rate_ * .45) / rate_, q = std::pow(10.0, 1 / 20.0), alpha = std::sin(w0) / (2 * q), c = std::cos(w0);
    const double a0 = 1 + alpha;
    vo.b0 = (1 - c) / 2 / a0;
    vo.b1 = (1 - c) / a0;
    vo.b2 = vo.b0;
    vo.a1 = -2 * c / a0;
    vo.a2 = (1 - alpha) / a0;
    std::lock_guard<std::mutex> lock(m_);
    voices_.push_back(vo);
}

void Synth::sfx(Sfx s) {
    if (sfxGain_.load() <= 0) return;
    using W = Wave;
    switch (s) {
        case Sfx::Dig: noise(.06, .25, 500 + rng_.next() * 600); break;
        case Sfx::Ore:
            tone(660, .08, W::Triangle, .25);
            tone(990, .12, W::Triangle, .2, 0, at(.06));
            break;
        case Sfx::Hit:
            noise(.08, .4, 1800);
            tone(140, .1, W::Square, .15, -60);
            break;
        case Sfx::Kill:
            tone(300, .25, W::Saw, .18, -200);
            noise(.2, .3, 900);
            break;
        case Sfx::Hurt: tone(180, .22, W::Saw, .25, -110); break;
        case Sfx::Land: noise(.18, .4, 300); break;
        case Sfx::Bump: tone(90, .08, W::Square, .12); break;
        case Sfx::Sell: {
            const double n[] = {523, 659, 784, 1047};
            for (int i = 0; i < 4; ++i) tone(n[i], .15, W::Triangle, .2, 0, at(i * .06));
            break;
        }
        case Sfx::Buy:
            tone(440, .08, W::Square, .12);
            tone(880, .12, W::Square, .12, 0, at(.07));
            break;
        case Sfx::Lvl: {
            const double n[] = {392, 523, 659, 784, 1047};
            for (int i = 0; i < 5; ++i) tone(n[i], .18, W::Triangle, .18, 0, at(i * .07));
            break;
        }
        case Sfx::Roar:
            tone(70, 1.1, W::Saw, .35, -30);
            noise(.9, .35, 400);
            break;
        case Sfx::Win: {
            const double n[] = {523, 659, 784, 1047, 1319};
            for (int i = 0; i < 5; ++i) tone(n[i], .3, W::Square, .14, 0, at(i * .1));
            break;
        }
        case Sfx::Fail: {
            const double n[] = {300, 220, 160};
            for (int i = 0; i < 3; ++i) tone(n[i], .25, W::Saw, .18, 0, at(i * .15));
            break;
        }
        case Sfx::Shoot: tone(520, .12, W::Square, .08, -300); break;
    }
}

void Synth::music(bool bossAwake) {
    static constexpr int PROG[4][3] = {{57, 60, 64}, {53, 57, 60}, {48, 52, 55}, {55, 59, 62}};  // La m, Fa, Do, Sol
    static constexpr int ARP[8] = {0, 1, 2, 1, 2, 0, 1, 2};
    const double now = time();
    if (nextT_ < now) nextT_ = now + .05;  // rattrapage (démarrage, pause)
    if (musicGain_.load() <= 0) {
        nextT_ = now + .1;
        return;
    }
    const double spb = bossAwake ? .15 : .22;  // durée d'un pas
    while (nextT_ < now + .2) {
        const int* ch = PROG[(step_ / 16) % 4];
        const int s = step_ % 16;
        if (s % 4 == 0) tone(mid(ch[0] - 24), spb * 3.2, Wave::Sine, .35, 0, nextT_, true);  // basse
        if (bossAwake && s % 2 == 0) tone(mid(ch[0] - 12), spb * .9, Wave::Square, .07, 0, nextT_, true);
        if (s % 2 == 0 || bossAwake) tone(mid(ch[ARP[s % 8]] + (s >= 8 ? 12 : 0)), spb * .9, Wave::Triangle, .07, 0, nextT_, true);  // arpège
        nextT_ += spb;
        ++step_;
    }
}

void Synth::render(float* out, int frames) {
    std::fill(out, out + frames, 0.f);
    const std::int64_t p0 = pos_.load();
    const double mg = musicGain_.load(), sg = sfxGain_.load();
    {
        std::lock_guard<std::mutex> lock(m_);
        for (Voice& vo : voices_) {
            const double bus = vo.music ? mg : sg;
            for (int i = 0; i < frames; ++i) {
                const std::int64_t k = p0 + i - vo.start;  // échantillons depuis le départ de la voix
                if (k < 0) continue;
                if (k >= vo.length) break;
                const double t = static_cast<double>(k) / rate_, u = std::min(t / vo.d, 1.0);
                const double gain = vo.v * std::pow(FLOOR / vo.v, u);
                double x = 0;
                if (vo.noise) {
                    const double in = k < static_cast<std::int64_t>(noiseBuf_.size()) ? noiseBuf_[static_cast<std::size_t>(k)] : 0.0;
                    x = vo.b0 * in + vo.b1 * vo.x1 + vo.b2 * vo.x2 - vo.a1 * vo.y1 - vo.a2 * vo.y2;
                    vo.x2 = vo.x1;
                    vo.x1 = in;
                    vo.y2 = vo.y1;
                    vo.y1 = x;
                } else {
                    const double f = vo.f0 * std::pow(vo.f1 / vo.f0, u), dt = f / rate_, ph = vo.phase;
                    switch (vo.wave) {
                        case Wave::Sine: x = std::sin(TAU * ph); break;
                        case Wave::Square: x = (ph < .5 ? 1.0 : -1.0) + polyBlep(ph, dt) - polyBlep(std::fmod(ph + .5, 1.0), dt); break;
                        case Wave::Triangle: x = ph < .25 ? 4 * ph : ph < .75 ? 2 - 4 * ph : 4 * ph - 4; break;
                        case Wave::Saw: x = 2 * ph - 1 - polyBlep(ph, dt); break;
                    }
                    vo.phase = ph + dt - std::floor(ph + dt);
                }
                out[i] += static_cast<float>(x * gain * bus);
            }
        }
        std::erase_if(voices_, [&](const Voice& vo) { return p0 + frames - vo.start >= vo.length; });
    }
    for (int i = 0; i < frames; ++i) out[i] = std::clamp(out[i], -1.f, 1.f);  // comme la sortie Web Audio
    pos_ += frames;
}

}  // namespace filon
