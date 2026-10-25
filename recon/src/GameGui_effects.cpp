#include "recon_all.h"
#include <algorithm>
#include <cmath>

void GameGui::drawControllerUpdates(sf::RenderTarget &target, std::vector<float> &gamepads,
                                   std::vector<float> &keyboards) {
    const auto size = target.getView().getSize();
    for (std::size_t i = 0; i < gamepads.size(); ++i) {
        Sprite *sprite;
        float time = gamepads[i];
        if (time <= 0.f || time < keyboards[i]) {
            time = keyboards[i];
            if (time <= 0.f) continue;
            sprite = &keyboardSprite;
        } else sprite = &gamepadSprite;
        const float alpha = std::max(std::sin(time * 3.1415927410125732f), 0.f);
        sprite->setColor({205, 205, 205, sf::Uint8(alpha * 255.f)});
        sprite->setPosition(size.x * .25f * float(i) + size.x * .125f,
                            size.y - 42.f - std::sqrt(alpha) * 25.f);
        sprite->draw(target, {});
    }
}

void GameGui::drawExperienceBar(sf::RenderTarget &target, float delta, int experience, bool) {
    const float value = float(experience);
    if (displayExperience > value) {
        experienceEffectCounter = 0;
        displayExperience = value;
    } else if (displayExperience < value) {
        displayExperience += delta * .5f;
        experienceEffectCounter += delta * .5f;
        if (displayExperience > value) {
            if (displayExperience >= 100.f) experienceShineEffectCounter = 3.f;
            displayExperience = value;
        }
    }
    const auto size = target.getView().getSize();
    const float width = size.x - 2.f;
    float height = 1.f;
    if (experienceShineEffectCounter > 0.f) {
        experienceShineEffectCounter -= experienceShineEffectCounter * .1f * delta;
        if (double(experienceShineEffectCounter) < .1) experienceShineEffectCounter = 0;
        height += experienceShineEffectCounter;
    }
    sf::RectangleShape bar({float(experience) * .01f * width, height});
    bar.setPosition(1, size.y - height);
    bar.setFillColor({50, 50, 50, 100});
    target.draw(bar);
    bar.setSize({displayExperience * .01f * width, height});
    bar.setFillColor(displayExperience >= 100.f ? sf::Color(255, 157, 50, 150) :
                                                   sf::Color(255, 255, 0, 150));
    target.draw(bar);
    if (experienceEffectCounter > 0.f) {
        experienceEffectCounter = (experienceEffectCounter - delta * .03f) * .89f;
        // MSVC generate_canonical<float>: exactly one MT draw, rounded before division.
        const float random = float(_randEngine()) / 4294967296.f;
        bar.setSize({experienceEffectCounter * .01f * width, random + 1.f});
        bar.setPosition(displayExperience * .01f * width + 1.f - bar.getSize().x,
                        size.y - bar.getSize().y);
        bar.setFillColor({255, 255, 255, 50});
        target.draw(bar, sf::RenderStates(sf::BlendAdd));
    }
}

void GameGui::drawGoldDisplay(sf::RenderTarget &target, int gold, int newlyPickedUp) {
    const float width = target.getView().getSize().x;
    goldBannerSprite.setPosition(width - 66.f, 10.f);
    goldBannerSprite.draw(target, {});
    // lambda_ae0ede928685dab13a0350b39c951f07, 005cfbe0.
    const auto drawNumber = [&](const std::string &number, float x, float y, bool highlighted) {
        for (std::size_t i = 0; i < number.size(); ++i) {
            int frame = int(number[i]) - int('-');
            if (number[i] == '+') frame = 0;
            else if (number[i] == 'c') frame = 2;
            goldFontSprite.setFrame(frame + (highlighted ? 13 : 0));
            goldFontSprite.setPosition(float(i * 10) + x - float(number.size() * 10), y);
            goldFontSprite.draw(target, {});
        }
    };
    drawNumber(std::to_string(std::max(gold - newlyPickedUp, 0)), width - 14.f, 6.f, false);
    goldCoinSprite.setPosition(width - 61.f, 7.f);
    goldCoinSprite.draw(target, {});
    if (newlyPickedUp > 0) drawNumber("+" + std::to_string(newlyPickedUp), width - 14.f, 22.f, true);
}

void GameGui::drawTransitionCircle(sf::RenderTarget &target, float radius,
                                   std::shared_ptr<Player>) {
    const sf::View view = target.getView();
    const auto size = view.getSize();
    const float outer = std::sqrt(std::pow(size.x * .5f, 2.f) + std::pow(size.y * .5f, 2.f));
    const float inner = radius * .00390625f * outer;
    sf::VertexArray strip(sf::TriangleStrip, 62);
    for (unsigned i = 0; i < 62; ++i) {
        const float angle = float(i / 2) * .20943953096866608f;
        const float r = i % 2 ? inner : outer;
        strip[i].position = {std::cos(angle) * r, std::sin(angle) * r};
        strip[i].color = sf::Color::Black;
    }
    sf::Transform transform;
    transform.translate(size.x * .5f, size.y * .5f);
    target.draw(strip, sf::RenderStates(transform));
}
