#include "render/Renderer.hpp"

#include "anomaly/Anomaly.hpp"
#include "gui/GuiText.hpp"
#include "simulation/World.hpp"
#include "util/Localization.hpp"

#include <algorithm>
#include <cmath>

#define DrawText aic::drawText

namespace aic {

namespace {
Color stageColor(Stage s) {
    switch (s) {
        case Stage::Primitive:       return Color{ 120, 110, 90, 255 };
        case Stage::Tribal:          return Color{ 150, 130, 80, 255 };
        case Stage::Agricultural:    return Color{ 110, 160, 90, 255 };
        case Stage::Industrial:      return Color{ 150, 150, 160, 255 };
        case Stage::Digital:         return Color{ 90, 170, 220, 255 };
        case Stage::PostSingularity: return Color{ 210, 200, 255, 255 };
        case Stage::Collapsed:       return Color{ 120, 60, 60, 255 };
        case Stage::Unknown:         return Color{ 200, 70, 160, 255 };
    }
    return GRAY;
}

float markerScale(const Camera2D& cam) {
    return std::clamp(1.0f / std::max(0.18f, cam.zoom), 1.0f, 4.2f);
}

void drawAgentFigure(Vector2 p, Vector2 velocity, Color col, float scale) {
    const float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    Vector2 dir{ 1.0f, 0.0f };
    if (speed > 0.01f) {
        dir = { velocity.x / speed, velocity.y / speed };
    }
    Vector2 side{ -dir.y, dir.x };
    const Vector2 trail{ p.x - dir.x * 9.0f * scale, p.y - dir.y * 9.0f * scale };
    DrawLineEx(trail, p, 1.0f * scale, Color{ col.r, col.g, col.b, 55 });

    const Vector2 head{ p.x, p.y - 7.0f * scale };
    const Vector2 neck{ p.x, p.y - 3.8f * scale };
    const Vector2 hip{ p.x, p.y + 6.0f * scale };
    const Vector2 legL{ p.x - side.x * 4.2f * scale - dir.x * 2.0f * scale,
                        p.y + 12.0f * scale - side.y * 4.2f * scale };
    const Vector2 legR{ p.x + side.x * 4.2f * scale + dir.x * 2.0f * scale,
                        p.y + 12.0f * scale + side.y * 4.2f * scale };
    const float thick = 1.35f * scale;
    DrawCircleV(head, 2.7f * scale, col);
    DrawCircleLinesV(head, 2.7f * scale, Color{ 245, 248, 255, 185 });
    DrawLineEx(neck, hip, thick, Color{ col.r, col.g, col.b, 235 });
    DrawLineEx({ p.x - 5.2f * scale, p.y - 0.5f * scale },
               { p.x + 5.2f * scale, p.y - 0.5f * scale }, thick,
               Color{ col.r, col.g, col.b, 225 });
    DrawLineEx(hip, legL, thick, Color{ col.r, col.g, col.b, 220 });
    DrawLineEx(hip, legR, thick, Color{ col.r, col.g, col.b, 220 });
}
} // namespace

void Renderer::drawWorld(const World& world, const Camera2D& cam, Rectangle viewport) {
    BeginScissorMode((int)viewport.x, (int)viewport.y,
                     (int)viewport.width, (int)viewport.height);
    DrawRectangleRec(viewport, Color{ 8, 9, 14, 255 });

    BeginMode2D(cam);

    // World boundary
    DrawRectangleLinesEx({ 0, 0, world.bounds.x, world.bounds.y }, 2.0f,
                         Color{ 40, 46, 64, 255 });

    // Civilization core
    const float coreR = std::min(220.0f, 30.0f + world.civ.population * 2.0f);
    Color cc = stageColor(world.civ.stage);
    DrawCircleV(world.center, coreR, Color{ cc.r, cc.g, cc.b, 40 });
    DrawCircleLinesV(world.center, coreR, Color{ cc.r, cc.g, cc.b, 200 });
    DrawCircleV(world.center, 14.0f, cc);

    // Law rings (blue lines) around the core, one per law.
    for (int li = 0; li < (int)world.laws.size(); ++li) {
        DrawCircleLinesV(world.center, coreR * 0.5f + 8.0f + li * 6.0f,
                         Color{ 90, 150, 220, 70 });
    }

    // Religion aura (gold / violet) when faiths exist.
    if (!world.religions.empty()) {
        const float aura = coreR + 16.0f + (float)world.religions.size() * 4.0f;
        DrawCircleLinesV(world.center, aura, Color{ 225, 185, 95, 130 });
        DrawCircleLinesV(world.center, aura + 6.0f, Color{ 180, 120, 220, 90 });
    }

    const bool showWorldLabels = cam.zoom >= 0.58f;
    const float viewScale = markerScale(cam);

    // Structures (placed buildings)
    for (const auto& s : world.structures) {
        Rectangle r = { s.position.x - s.size * 0.5f, s.position.y - s.size * 0.5f,
                        s.size, s.size };
        DrawRectangleRec(r, Color{ 170, 150, 110, 220 });
        DrawRectangleLinesEx(r, 1.0f, Color{ 215, 200, 160, 200 });
        if (showWorldLabels && world.structures.size() <= 80) {
            const std::string sname = jpDomainName(s.name);
            DrawText(sname.c_str(), (int)(s.position.x + s.size), (int)s.position.y - 6,
                     10, Color{ 205, 195, 165, 200 });
        }
    }

    // Active anomalies
    for (const auto& a : world.activeAnomalies) {
        const Color base = anomalyColor(a.type);
        const float pulse = 0.85f + 0.15f * std::sin(a.phase * 3.0f);
        if (a.type == AnomalyType::BlackHole) {
            DrawCircleV(a.position, a.radius * 0.28f * pulse, Color{ 0, 0, 0, 245 });
            for (int i = 0; i < 8; ++i) {
                const float r = a.radius * (0.25f + i * 0.09f);
                DrawCircleLinesV(a.position, r, Color{ 40, 38, 55, (unsigned char)(180 - i * 16) });
            }
        } else if (a.type == AnomalyType::Gravity) {
            for (int i = 0; i < 5; ++i) {
                DrawCircleLinesV(a.position, a.radius * (0.35f + i * 0.18f) * pulse,
                                 Color{ base.r, base.g, base.b, (unsigned char)(150 - i * 18) });
            }
            for (int i = 0; i < 12; ++i) {
                const float ang = a.phase + i * (2.0f * PI / 12.0f);
                Vector2 p2{ a.position.x + std::cos(ang) * a.radius * 0.9f,
                            a.position.y + std::sin(ang) * a.radius * 0.9f };
                DrawLineV(a.position, p2, Color{ base.r, base.g, base.b, 70 });
            }
        } else if (a.type == AnomalyType::WhiteHole) {
            DrawCircleV(a.position, a.radius * 0.18f * pulse, Color{ 245, 245, 255, 230 });
            for (int i = 0; i < 18; ++i) {
                const float ang = a.phase + i * (2.0f * PI / 18.0f);
                Vector2 p2{ a.position.x + std::cos(ang) * a.radius * 0.95f,
                            a.position.y + std::sin(ang) * a.radius * 0.95f };
                DrawLineV(a.position, p2, Color{ 240, 240, 255, 100 });
            }
        } else {
            DrawCircleLinesV(a.position, a.radius * pulse, Color{ base.r, base.g, base.b, 180 });
            for (int i = 0; i < 5; ++i) {
                const float x = a.position.x + std::sin(a.phase + i) * a.radius * 0.7f;
                DrawLine((int)x, (int)(a.position.y - a.radius * 0.5f),
                         (int)(x + 20.0f * std::sin(a.phase * 2.0f)),
                         (int)(a.position.y + a.radius * 0.5f),
                         Color{ base.r, base.g, base.b, 90 });
            }
        }
        DrawText(jpAnomalyTypeName(a.type), (int)a.position.x + 12, (int)a.position.y + 12,
                 12, Color{ base.r, base.g, base.b, 220 });
    }

    // Concepts: main view shows a sampled field; detailed names live in the
    // Concepts screen and on click/hover labels.
    const int conceptCount = (int)world.concepts.size();
    const int conceptStride = std::max(1, conceptCount / 700);
    int labelsDrawn = 0;
    for (int i = conceptCount - 1; i >= 0; i -= conceptStride) {
        const auto& c = world.concepts[(size_t)i];
        const float r = std::clamp(1.6f + std::sqrt(std::max(0.0f, c.strength)) * 0.45f,
                                   1.8f, 7.0f);
        const unsigned char alpha = (unsigned char)(world.concepts.size() > 500 ? 105 : 145);
        DrawCircleV(c.position, r, Color{ 172, 150, 225, alpha });
        if (showWorldLabels && labelsDrawn < 28 && c.strength >= 3.0f) {
            const std::string cname = jpDomainName(c.name);
            DrawText(cname.c_str(), (int)c.position.x + 8, (int)c.position.y - 6,
                     10, Color{ 205, 198, 235, 190 });
            ++labelsDrawn;
        }
    }

    // Agents
    for (const auto& a : world.agents) {
        if (!a.alive()) continue;
        Color col = agentStateColor(a.state);
        drawAgentFigure(a.position, a.velocity, col, viewScale);
    }

    EndMode2D();
    EndScissorMode();
}

} // namespace aic
