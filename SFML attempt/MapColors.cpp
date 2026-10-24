#include "MapColors.hpp"
#include <algorithm>
#include <cmath>
#include "imgui.h"

namespace {

    int channel(const sf::Color& color, int i)
    {
        switch (i)
        {
        case 0: return color.r;
        case 1: return color.g;
        case 2: return color.b;
        default: return color.a;
        }
    }

    sf::Color toColor(const float channels[4])
    {
        sf::Uint8 c[4];
        for (int i = 0; i < 4; i++) { c[i] = static_cast<sf::Uint8>(std::clamp(std::lround(channels[i]), 0L, 255L)); }
        return sf::Color(c[0], c[1], c[2], c[3]);
    }

    // Length of the normal range, the channel that changes the most moves one step per step
    float normalRange(const sf::Color& low, const sf::Color& high)
    {
        int range = 0;
        for (int i = 0; i < 4; i++) { range = std::max(range, std::abs(channel(high, i) - channel(low, i))); }
        return static_cast<float>(range);
    }

    // Steps the zigzag takes to get from one colour to the other
    float overflowLength(const sf::Color& from, const sf::Color& to)
    {
        int steps = 0;
        for (int i = 0; i < 4; i++) { steps += std::abs(channel(to, i) - channel(from, i)); }
        return static_cast<float>(steps);
    }

    // Walks from 'from' toward 'to' one channel step at a time, rotating between the channels that still
    // differ so they move evenly, and stops at 'to'
    sf::Color zigzag(const sf::Color& from, const sf::Color& to, float steps)
    {
        float distance[4];
        for (int i = 0; i < 4; i++) { distance[i] = static_cast<float>(std::abs(channel(to, i) - channel(from, i))); }

        // Every unfinished channel has moved the same amount (level), find the level the steps reach
        float sorted[4] = { distance[0], distance[1], distance[2], distance[3] };
        std::sort(sorted, sorted + 4);
        float level = 0.f;
        float remaining = std::max(steps, 0.f);
        for (int i = 0; i < 4; i++)
        {
            int active = 4 - i;
            float span = sorted[i] - level;
            if (span * active >= remaining)
            {
                level += remaining / active;
                break;
            }
            remaining -= span * active;
            level = sorted[i];
        }

        float result[4];
        for (int i = 0; i < 4; i++)
        {
            float moved = std::min(distance[i], level);
            result[i] = channel(from, i) + (channel(to, i) >= channel(from, i) ? moved : -moved);
        }
        return toColor(result);
    }

    // Steps is the position from the low end of the normal range, past either end it zigzags toward the targets
    sf::Color scaleColor(float steps, const sf::Color& low, const sf::Color& high, const sf::Color& lowTarget, const sf::Color& highTarget)
    {
        if (std::isnan(steps)) { steps = 0.f; }
        float range = normalRange(low, high);
        if (steps <= 0.f) { return zigzag(low, lowTarget, -steps); }
        if (steps >= range) { return zigzag(high, highTarget, steps - range); }

        float t = steps / range;
        float result[4];
        for (int i = 0; i < 4; i++) { result[i] = channel(low, i) + (channel(high, i) - channel(low, i)) * t; }
        return toColor(result);
    }

    sf::Color withAlpha(sf::Color color, sf::Uint8 alpha)
    {
        color.a = alpha;
        return color;
    }

    bool colorEdit(const char* label, sf::Color& color)
    {
        float c[3] = { color.r / 255.f, color.g / 255.f, color.b / 255.f };
        if (!ImGui::ColorEdit3(label, c)) { return false; }
        color = sf::Color(
            static_cast<sf::Uint8>(std::lround(c[0] * 255.f)), static_cast<sf::Uint8>(std::lround(c[1] * 255.f)),
            static_cast<sf::Uint8>(std::lround(c[2] * 255.f)), 255);
        return true;
    }

    bool stepsEdit(const char* label, float& steps, float speed, float min, const char* format)
    {
        bool changed = ImGui::DragFloat(label, &steps, speed, min, 10000.f, format, ImGuiSliderFlags_AlwaysClamp);
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Colour steps per unit, changing it makes maps look different from before"); ImGui::EndTooltip(); }
        return changed;
    }
}

sf::Color heightColor(const HeightColors& colors, float height, bool ocean)
{
    const sf::Color& base = ocean ? colors.ocean : colors.land;
    sf::Color low = withAlpha(base, static_cast<sf::Uint8>(std::clamp(colors.alphaFloor, 0, 255)));
    return scaleColor(height * colors.stepsPerUnit, low, withAlpha(base, 255), low, colors.overflow);
}

sf::Color temperatureColor(const TemperatureColors& colors, float temperature)
{
    float steps = normalRange(colors.cold, colors.hot) / 2.f + temperature * colors.stepsPerDegree;
    return scaleColor(steps, colors.cold, colors.hot, colors.coldOverflow, colors.hotOverflow);
}

sf::Color precipitationColor(const PrecipitationColors& colors, float precipitation)
{
    return scaleColor(precipitation * colors.stepsPerUnit, colors.dry, colors.wet, colors.dry, colors.overflow);
}

sf::Color tintColor(sf::Color base, sf::Color target, float strength)
{
    strength = std::clamp(strength, 0.f, 1.f);
    const auto mix = [strength](sf::Uint8 a, sf::Uint8 b) { return static_cast<sf::Uint8>(a + (b - a) * strength + 0.5f); };
    return sf::Color(mix(base.r, target.r), mix(base.g, target.g), mix(base.b, target.b), base.a);
}

sf::Color waterColor(const WaterColors& colors, float discharge)
{
    const float t = std::clamp(std::log10(1.f + std::max(discharge, 0.f)) / std::log10(1.f + colors.fullDischarge), 0.f, 1.f);
    const auto mix = [t](sf::Uint8 a, sf::Uint8 b) { return static_cast<sf::Uint8>(a + (b - a) * t + 0.5f); };
    return sf::Color(mix(colors.dry.r, colors.wet.r), mix(colors.dry.g, colors.wet.g), mix(colors.dry.b, colors.wet.b), 255);
}

bool mapColorsUI(MapColors& colors)
{
    if (!ImGui::CollapsingHeader("Map Colours")) { return false; }

    bool changed = false;

    // Own ID scope, the map type radio buttons use the same labels
    ImGui::PushID("MapColors");

    if (ImGui::TreeNode("Height"))
    {
        HeightColors& h = colors.height;
        changed |= colorEdit("Land", h.land);
        changed |= colorEdit("Ocean", h.ocean);
        changed |= colorEdit("Overflow", h.overflow);
        changed |= ImGui::SliderInt("Alpha floor", &h.alphaFloor, 0, 255, "%d", ImGuiSliderFlags_AlwaysClamp);
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Alpha at height 0"); ImGui::EndTooltip(); }
        changed |= stepsEdit("Steps per unit", h.stepsPerUnit, 1.f, 1.f, "%.1f");

        float start = (255 - h.alphaFloor) / h.stepsPerUnit;
        float end = start + overflowLength(withAlpha(h.land, 255), h.overflow) / h.stepsPerUnit;
        ImGui::TextDisabled("Land overflows from h = %.2f, reaches the target at h = %.2f", start, end);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Temperature"))
    {
        TemperatureColors& t = colors.temperature;
        changed |= colorEdit("Cold", t.cold);
        changed |= colorEdit("Hot", t.hot);
        changed |= colorEdit("Cold overflow", t.coldOverflow);
        changed |= colorEdit("Hot overflow", t.hotOverflow);
        changed |= stepsEdit("Steps per degree", t.stepsPerDegree, 0.05f, 0.1f, "%.2f");

        float half = normalRange(t.cold, t.hot) / 2.f / t.stepsPerDegree;
        ImGui::TextDisabled("Hot overflows from %.1f C, reaches the target at %.1f C",
            half, half + overflowLength(t.hot, t.hotOverflow) / t.stepsPerDegree);
        ImGui::TextDisabled("Cold overflows from %.1f C, reaches the target at %.1f C",
            -half, -half - overflowLength(t.cold, t.coldOverflow) / t.stepsPerDegree);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Percepitation"))
    {
        PrecipitationColors& p = colors.precipitation;
        changed |= colorEdit("Dry", p.dry);
        changed |= colorEdit("Wet", p.wet);
        changed |= colorEdit("Overflow", p.overflow);
        changed |= stepsEdit("Steps per unit", p.stepsPerUnit, 0.01f, 0.01f, "%.2f");

        float start = normalRange(p.dry, p.wet) / p.stepsPerUnit;
        ImGui::TextDisabled("Overflows from %.0f, reaches the target at %.0f",
            start, start + overflowLength(p.wet, p.overflow) / p.stepsPerUnit);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Snow and ice"))
    {
        SnowIceColors& s = colors.snowIce;
        changed |= ImGui::Checkbox("Show in Height and Biome modes", &s.enabled);
        changed |= colorEdit("Snow", s.snow);
        changed |= ImGui::SliderFloat("Snow strength", &s.snowStrength, 0.f, 1.f, "%.2f");
        changed |= colorEdit("Sea ice", s.ice);
        changed |= ImGui::SliderFloat("Ice strength", &s.iceStrength, 0.f, 1.f, "%.2f");
        ImGui::TextDisabled("Land that stays at or below 0 C in its warmest month, ocean at or below -2 C");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Water"))
    {
        WaterColors& w = colors.water;
        changed |= colorEdit("Lake", w.lake);
        changed |= colorEdit("Dry land", w.dry);
        changed |= colorEdit("Wet land", w.wet);
        changed |= colorEdit("Ocean", w.ocean);
        changed |= ImGui::SliderFloat("Wet from (m3/s)", &w.fullDischarge, 100.f, 100000.f, "%.0f", ImGuiSliderFlags_Logarithmic);
        ImGui::TextDisabled("Lakes use the lake colour in the Height and Biome modes too");
        ImGui::TreePop();
    }

    ImGui::PopID();
    return changed;
}
