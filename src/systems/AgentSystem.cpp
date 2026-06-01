#include "systems/AgentSystem.hpp"

#include "simulation/World.hpp"
#include "util/Localization.hpp"
#include "util/Random.hpp"

#include <algorithm>
#include <cmath>

namespace aic {

namespace {
constexpr int   kMaxPop = 60;
constexpr float kAgentMaxSpeed = 14.0f;
constexpr float kMargin = 60.0f;

float clampf(float v, float lo, float hi) { return std::min(std::max(v, lo), hi); }
} // namespace

Agent AgentSystem::makeChild(World& world, Random& rng, const Agent& p) {
    Agent c;
    c.id = world.newAgentId();
    c.name = Agent::makeName(rng);
    c.life = rng.nextFloat(70.0f, 100.0f);
    c.intelligence = clampf(p.intelligence + rng.gaussian(0.0f, 6.0f), 0.0f, 100.0f);
    c.emotion = clampf(p.emotion + rng.gaussian(0.0f, 6.0f), 0.0f, 100.0f);
    c.adaptability = clampf(p.adaptability + rng.gaussian(0.0f, 6.0f), 0.0f, 100.0f);
    c.creativity = clampf(p.creativity + rng.gaussian(0.0f, 8.0f), 0.0f, 100.0f);
    c.sociality = clampf(p.sociality + rng.gaussian(0.0f, 6.0f), 0.0f, 100.0f);
    c.faith_bias = clampf(p.faith_bias + rng.gaussian(0.0f, 6.0f), 0.0f, 100.0f);
    c.memory = rng.nextFloat(40.0f, 80.0f);
    c.resource = rng.nextFloat(30.0f, 50.0f);
    c.position = { p.position.x + rng.nextFloat(-20.0f, 20.0f),
                   p.position.y + rng.nextFloat(-20.0f, 20.0f) };
    c.velocity = { rng.nextFloat(-4.0f, 4.0f), rng.nextFloat(-4.0f, 4.0f) };
    c.state = AgentState::Alive;
    return c;
}

void AgentSystem::update(World& world, Random& rng) {
    std::vector<Agent> newborns;
    const size_t count = world.agents.size();

    for (size_t i = 0; i < count; ++i) {
        Agent& a = world.agents[i];
        if (!a.alive()) continue;

        // --- move: wander + sociality pull toward (or escape away from) center
        a.velocity.x += rng.nextFloat(-6.0f, 6.0f);
        a.velocity.y += rng.nextFloat(-6.0f, 6.0f);
        Vector2 toCenter{ world.center.x - a.position.x, world.center.y - a.position.y };
        const float dc = std::sqrt(toCenter.x * toCenter.x + toCenter.y * toCenter.y) + 0.001f;
        const bool fleeing = (world.collapseRisk > 85.0f && a.adaptability > 70.0f);
        const float pull = fleeing ? -0.08f : (a.sociality * 0.0006f);
        a.velocity.x += (toCenter.x / dc) * pull * dc;
        a.velocity.y += (toCenter.y / dc) * pull * dc;
        const float sp = std::sqrt(a.velocity.x * a.velocity.x + a.velocity.y * a.velocity.y);
        if (sp > kAgentMaxSpeed) {
            a.velocity.x = a.velocity.x / sp * kAgentMaxSpeed;
            a.velocity.y = a.velocity.y / sp * kAgentMaxSpeed;
        }
        a.position.x += a.velocity.x;
        a.position.y += a.velocity.y;
        if (a.position.x < kMargin) { a.position.x = kMargin; a.velocity.x = -a.velocity.x; }
        if (a.position.x > world.bounds.x - kMargin) { a.position.x = world.bounds.x - kMargin; a.velocity.x = -a.velocity.x; }
        if (a.position.y < kMargin) { a.position.y = kMargin; a.velocity.y = -a.velocity.y; }
        if (a.position.y > world.bounds.y - kMargin) { a.position.y = world.bounds.y - kMargin; a.velocity.y = -a.velocity.y; }

        // --- consume from the shared resource pool (only what can be stored,
        // so full agents don't waste the pool)
        const float demand = 1.5f + a.intelligence * 0.01f;
        const float room = std::max(0.0f, 100.0f - a.resource);
        const float take = std::min(std::min(demand, room),
                                    std::max(0.0f, (float)world.resourceTotal));
        world.resourceTotal -= take;
        a.resource += take;

        // --- metabolism
        const float upkeep = 1.0f + world.environmentPressure * 2.0f +
                             world.anomalyPressure * 1.5f;
        a.resource -= upkeep;
        if (a.resource < 0.0f) {
            a.life += a.resource;
            a.resource = 0.0f;
            a.state = AgentState::Hungry;
        } else {
            a.state = AgentState::Alive;
            a.life = std::min(100.0f, a.life + 0.2f);
        }

        // --- observe: slow passive growth of intelligence/memory
        a.intelligence = std::min(100.0f, a.intelligence + 0.01f);
        a.memory = std::min(100.0f, a.memory + 0.006f);

        // --- mutate: rare, amplified by anomaly pressure
        if (rng.chance(0.003f + world.anomalyPressure * 0.05f)) {
            a.creativity = clampf(a.creativity + rng.gaussian(0.0f, 10.0f), 0.0f, 100.0f);
            a.adaptability = clampf(a.adaptability + rng.gaussian(0.0f, 10.0f), 0.0f, 100.0f);
            a.state = AgentState::Mutating;
            if (world.anomalyPressure > 0.2f && rng.chance(0.25f)) {
                Event e;
                e.cycle = world.cycle; e.type = EventType::Agent; e.name = a.name;
                e.source = a.name; e.target = "World"; e.value = a.creativity;
                e.message = a.name + " がストレス下で変異しました"; e.tag = "warning";
                world.pushEvent(e);
            }
        }

        // --- share_resource: surplus flows to a nearby hungry agent
        if (a.resource > 80.0f) {
            for (size_t k = 0; k < world.agents.size(); ++k) {
                if (k == i) continue;
                Agent& b = world.agents[k];
                if (!b.alive() || b.resource > 40.0f) continue;
                const float dx = b.position.x - a.position.x;
                const float dy = b.position.y - a.position.y;
                if (dx * dx + dy * dy < 120.0f * 120.0f) {
                    a.resource -= 15.0f;
                    b.resource = std::min(100.0f, b.resource + 15.0f);
                    break;
                }
            }
        }

        // --- collapse: depleted agents die
        if (a.life <= 0.0f) {
            a.life = 0.0f;
            a.state = AgentState::Dead;
            Event e;
            e.cycle = world.cycle; e.type = EventType::Agent; e.name = a.name;
            e.source = a.name; e.target = "World"; e.value = 0.0f;
            e.message = a.name + " が死亡しました"; e.tag = "agent_dead";
            world.pushEvent(e);
            continue;
        }

        // --- reproduce
        const int pop = (int)world.agents.size() + (int)newborns.size();
        if (a.resource > 70.0f && pop < kMaxPop &&
            rng.chance(0.01f + a.sociality * 0.0005f)) {
            a.resource -= 30.0f;
            a.state = AgentState::Reproducing;
            Agent child = makeChild(world, rng, a);
            Event e;
            e.cycle = world.cycle; e.type = EventType::Agent; e.name = child.name;
            e.source = a.name; e.target = child.name; e.value = 0.0f;
            e.message = child.name + " が誕生しました"; e.tag = "agent_born";
            world.pushEvent(e);
            newborns.push_back(std::move(child));
        }
    }

    for (auto& c : newborns) world.agents.push_back(std::move(c));

    world.agents.erase(
        std::remove_if(world.agents.begin(), world.agents.end(),
                       [](const Agent& a) { return a.state == AgentState::Dead; }),
        world.agents.end());
}

} // namespace aic
