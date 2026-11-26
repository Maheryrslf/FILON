#pragma once
// Synthétiseur du jeu : bruitages et musique fabriqués à la volée, sans fichier audio (fiche technique §9).
// Reproduit tone(), noise(), SFX et music() de la version JS (Web Audio) : oscillateurs à rampes exponentielles,
// bruit blanc filtré, deux bus (musique, effets). Le jeu planifie les sons ; render() est appelé par le fil audio.
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

#include "filon/core/random.hpp"

namespace filon {

enum class Wave { Sine, Square, Triangle, Saw };
enum class Sfx { Dig, Ore, Hit, Kill, Hurt, Land, Bump, Sell, Buy, Lvl, Roar, Win, Fail, Shoot };

class Synth {
public:
    explicit Synth(int rate = 44100, std::uint32_t seed = 1);

    double time() const;                   // horloge audio en secondes (comme AudioContext.currentTime)
    void setVolumes(double music, double sfx);  // réglages 0..1 ; gains de bus 0.5 et 0.6 comme en JS

    // Note : fréquence f (Hz), durée d (s), volume v, glissement de fréquence slide (Hz), départ when (s, horloge audio ; < 0 : maintenant)
    void tone(double f, double d, Wave w, double v, double slide = 0, double when = -1, bool music = false);
    void noise(double d, double v, double cutoff);  // bruit blanc passe-bas
    void sfx(Sfx s);                       // un bruitage du jeu (rien si le volume des effets est nul)
    void music(bool bossAwake);            // planifie la musique ~0,2 s à l'avance ; à appeler à chaque image

    void render(float* out, int frames);   // mono, valeurs dans [-1, 1] ; appelé par le fil audio

private:
    struct Voice {
        std::int64_t start, length;        // en échantillons
        double f0, f1, v;                  // fréquence de départ et d'arrivée, volume de départ
        double d;                          // durée des rampes, en s
        Wave wave;
        bool noise, music;
        double phase = 0;
        double b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;  // filtre passe-bas
    };
    double at(double dt) const { return time() + dt; }

    const int rate_;
    std::atomic<std::int64_t> pos_{0};     // échantillons déjà rendus
    std::mutex m_;
    std::vector<Voice> voices_;
    std::vector<float> noiseBuf_;          // 0,3 s de bruit blanc, joué sans boucle comme en JS
    Rng rng_;
    std::atomic<double> musicGain_{.45 * .5}, sfxGain_{.8 * .6};
    double nextT_ = -1;                    // prochaine note de musique
    int step_ = 0;
};

}  // namespace filon
