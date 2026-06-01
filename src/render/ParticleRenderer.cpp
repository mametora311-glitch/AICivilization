#include "render/ParticleRenderer.hpp"

#include <cmath>

namespace aic {

void ParticleRenderer::drawRadiation(Vector2 center, float radius, Color color,
                                     float phase, int rays) {
    if (rays <= 0) return;
    for (int i = 0; i < rays; ++i) {
        const float t = phase + i * (2.0f * PI / rays);
        const float r0 = radius * 0.25f;
        const float r1 = radius * (0.75f + 0.15f * std::sin(phase + i));
        Vector2 a{ center.x + std::cos(t) * r0, center.y + std::sin(t) * r0 };
        Vector2 b{ center.x + std::cos(t) * r1, center.y + std::sin(t) * r1 };
        DrawLineV(a, b, color);
    }
}

} // namespace aic
