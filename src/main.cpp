
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>

#include <algorithm>
#include <cstdlib>
#include <string>
#include <random>
#include <chrono>
#include <cstdint>
#include <cmath>

using namespace geode::prelude;

class AutoBuildPopup : public Popup {
protected:
    TextInput* m_bpmInput = nullptr;
    TextInput* m_lengthInput = nullptr;
    CCLabelBMFont* m_difficultyLabel = nullptr;

    int m_difficulty = 0;

    const char* m_difficulties[10] = {
        "Easy", "Normal", "Hard", "Harder", "Insane",
        "Easy Demon", "Medium Demon", "Hard Demon",
        "Insane Demon", "Extreme Demon"
    };

    bool init() {
        if (!Popup::init(360.f, 260.f))
            return false;

        this->setTitle("AutoBuild Layout");

        auto bpmLabel = CCLabelBMFont::create(
            "BPM:", "bigFont.fnt"
        );
        bpmLabel->setScale(.55f);
        bpmLabel->setPosition({75.f, 175.f});
        m_mainLayer->addChild(bpmLabel);

        m_bpmInput = TextInput::create(110.f, "120");
        m_bpmInput->setCommonFilter(CommonFilter::Uint);
        m_bpmInput->setString("120");
        m_bpmInput->setPosition({205.f, 175.f});
        m_mainLayer->addChild(m_bpmInput);

        auto lengthLabel = CCLabelBMFont::create(
            "Length (seconds):", "bigFont.fnt"
        );
        lengthLabel->setScale(.48f);
        lengthLabel->setPosition({90.f, 125.f});
        m_mainLayer->addChild(lengthLabel);

        m_lengthInput = TextInput::create(110.f, "60");
        m_lengthInput->setCommonFilter(CommonFilter::Uint);
        m_lengthInput->setString("60");
        m_lengthInput->setPosition({230.f, 125.f});
        m_mainLayer->addChild(m_lengthInput);

        m_difficultyLabel = CCLabelBMFont::create(
            m_difficulties[m_difficulty], "goldFont.fnt"
        );
        m_difficultyLabel->setScale(.55f);

        auto diffButton = CCMenuItemSpriteExtra::create(
            m_difficultyLabel,
            this,
            menu_selector(AutoBuildPopup::onDifficulty)
        );

        auto diffMenu = CCMenu::create();
        diffMenu->setPosition({180.f, 75.f});
        diffMenu->addChild(diffButton);
        m_mainLayer->addChild(diffMenu);

        auto createButton = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("CREATE"),
            this,
            menu_selector(AutoBuildPopup::onCreate)
        );

        m_buttonMenu->addChildAtPosition(
            createButton, Anchor::Bottom, ccp(0.f, 22.f)
        );

        return true;
    }

    void onDifficulty(CCObject*) {
        m_difficulty = (m_difficulty + 1) % 10;
        m_difficultyLabel->setString(
            m_difficulties[m_difficulty]
        );
    }

    void onCreate(CCObject*) {
        auto editor = LevelEditorLayer::get();

        if (!editor) {
            FLAlertLayer::create(
                "Error", "Open the level editor first.", "OK"
            )->show();
            return;
        }

        std::string bpmText = m_bpmInput->getString();
        std::string lengthText = m_lengthInput->getString();

        char* endBpm = nullptr;
        char* endLength = nullptr;

        float bpm = std::strtof(bpmText.c_str(), &endBpm);
        float length = std::strtof(lengthText.c_str(), &endLength);

        if (endBpm == bpmText.c_str() ||
            endLength == lengthText.c_str() ||
            bpm < 20.f || bpm > 300.f ||
            length < 5.f || length > 180.f) {
            FLAlertLayer::create(
                "Invalid Input",
                "BPM: 20-300\nLength: 5-180 seconds.",
                "OK"
            )->show();
            return;
        }

        int placed = generateLayout(editor, bpm, length);

        this->onClose(nullptr);

        FLAlertLayer::create(
            "AutoBuild",
            fmt::format(
                "Generation finished!\nObjects created: {}",
                placed
            ),
            "OK"
        )->show();
    }

    int generateLayout(
        LevelEditorLayer* editor,
        float bpm,
        float length
    ) {
        constexpr float groundY = 105.f;
        constexpr int maxObjects = 1800;

        const float beatTime = 60.f / bpm;

        const uint32_t seed =
            static_cast<uint32_t>(
                std::chrono::steady_clock::now()
                    .time_since_epoch().count()
            );

        std::mt19937 rng(seed);

        auto ri = [&](int min, int max) {
            return std::uniform_int_distribution<int>(
                min, max
            )(rng);
        };

        auto rf = [&](float min, float max) {
            return std::uniform_real_distribution<float>(
                min, max
            )(rng);
        };

        const float diff =
            static_cast<float>(m_difficulty) / 9.f;

        float spacing = beatTime * 300.f;
        spacing *= 1.f - diff * .32f;
        spacing = std::clamp(spacing, 70.f, 180.f);

        int beats = std::min(
            static_cast<int>(length / beatTime),
            900
        );

        float x = 300.f;
        int placed = 0;

        // 0 Cube, 1 Ship, 2 Ball, 3 UFO,
        // 4 Wave, 5 Robot, 6 Spider
        int currentMode = 0;

        auto add = [&](int id, float px, float py) {
            if (placed >= maxObjects)
                return;

            auto obj = editor->createObject(
                id, ccp(px, py), false
            );

            if (obj)
                placed++;
        };

        const int modePortals[] = {
            12, 13, 47, 111, 660, 745, 1331
        };

        const int speedPortals[] = {
            201, 202, 203, 200
        };

        auto addOrb = [&](float px, float py) {
            int orb = ri(0, 3);

            switch (orb) {
                case 0: add(36, px, py); break;
                case 1: add(141, px, py); break;
                case 2: add(84, px, py); break;
                case 3: add(1333, px, py); break;
            }
        };

        auto cubePattern = [&](float px) {
            int p = ri(0, 11);
            float gap = rf(28.f, 43.f);
            float high = groundY + rf(55.f, 85.f);

            switch (p) {
                case 0:
                    add(8, px, groundY);
                    break;

                case 1:
                    add(8, px, groundY);
                    add(8, px + gap, groundY);
                    break;

                case 2:
                    add(1, px, groundY);
                    break;

                case 3:
                    add(1, px, groundY);
                    add(8, px + 55.f, groundY);
                    break;

                case 4:
                    addOrb(px + 25.f, high);
                    add(8, px + 85.f, groundY);
                    break;

                case 5:
                    add(8, px, groundY);
                    addOrb(px + 55.f, high);
                    break;

                case 6:
                    add(1, px, groundY);
                    add(1, px + gap, groundY);
                    add(8, px + 90.f, groundY);
                    break;

                case 7:
                    add(35, px, groundY);
                    add(8, px + 90.f, groundY);
                    break;

                case 8:
                    add(140, px, groundY);
                    add(8, px + 80.f, groundY);
                    break;

                case 9:
                    add(8, px, groundY);
                    add(8, px + 35.f, groundY);
                    add(8, px + 70.f, groundY);
                    break;

                case 10:
                    add(1, px, groundY);
                    addOrb(px + 45.f, high);
                    add(1, px + 100.f, groundY);
                    break;

                case 11:
                    add(36, px, high);
                    add(141, px + 55.f, high + 10.f);
                    break;
            }
        };

        auto shipPattern = [&](float px) {
            int p = ri(0, 4);
            float top = groundY + 180.f;
            float bottom = groundY + 35.f;

            switch (p) {
                case 0:
                    add(1, px, top);
                    add(1, px + 30.f, top);
                    break;

                case 1:
                    add(1, px, bottom);
                    add(1, px + 30.f, bottom);
                    break;

                case 2:
                    add(8, px, top);
                    add(8, px + 70.f, bottom);
                    break;

                case 3:
                    add(1, px, top);
                    add(1, px + 70.f, bottom);
                    add(1, px + 140.f, top);
                    break;

                case 4:
                    add(8, px, bottom);
                    add(1, px + 60.f, top);
                    break;
            }
        };

        auto ballPattern = [&](float px) {
            int p = ri(0, 5);

            switch (p) {
                case 0:
                    add(8, px, groundY);
                    break;

                case 1:
                    add(8, px, groundY + 120.f);
                    break;

                case 2:
                    add(1, px, groundY);
                    add(8, px + 65.f, groundY + 120.f);
                    break;

                case 3:
                    add(8, px, groundY);
                    add(1, px + 65.f, groundY + 120.f);
                    break;

                case 4:
                    add(1, px, groundY);
                    add(1, px + 35.f, groundY);
                    break;

                case 5:
                    add(8, px, groundY + 120.f);
                    add(8, px + 45.f, groundY);
                    break;
            }
        };

        auto ufoPattern = [&](float px) {
            int p = ri(0, 4);

            if (p == 0) {
                add(8, px, groundY);
            } else if (p == 1) {
                add(8, px, groundY);
                add(8, px + 38.f, groundY);
            } else if (p == 2) {
                addOrb(px + 20.f, groundY + 75.f);
                add(8, px + 85.f, groundY);
            } else if (p == 3) {
                add(1, px, groundY);
                add(8, px + 75.f, groundY);
            } else {
                add(35, px, groundY);
                add(8, px + 100.f, groundY);
            }
        };

        auto wavePattern = [&](float px) {
            int p = ri(0, 3);
            float upper = groundY + 175.f;
            float lower = groundY + 35.f;

            switch (p) {
                case 0:
                    add(1, px, upper);
                    add(1, px + 35.f, lower);
                    break;

                case 1:
                    add(1, px, lower);
                    add(1, px + 35.f, upper);
                    break;

                case 2:
                    add(1, px, upper);
                    add(1, px + 35.f, upper);
                    add(1, px + 70.f, lower);
                    break;

                case 3:
                    add(1, px, lower);
                    add(1, px + 35.f, lower);
                    add(1, px + 70.f, upper);
                    break;
            }
        };

        auto robotPattern = [&](float px) {
            int p = ri(0, 4);

            switch (p) {
                case 0:
                    add(8, px, groundY);
                    break;

                case 1:
                    add(35, px, groundY);
                    add(8, px + 95.f, groundY);
                    break;

                case 2:
                    add(1, px, groundY);
                    add(8, px + 65.f, groundY);
                    break;

                case 3:
                    add(8, px, groundY);
                    add(8, px + 45.f, groundY);
                    break;

                case 4:
                    addOrb(px + 25.f, groundY + 75.f);
                    add(8, px + 90.f, groundY);
                    break;
            }
        };

        auto spiderPattern = [&](float px) {
            int p = ri(0, 4);

            switch (p) {
                case 0:
                    add(1, px, groundY);
                    add(8, px + 65.f, groundY + 120.f);
                    break;

                case 1:
                    add(8, px, groundY);
                    add(1, px + 65.f, groundY + 120.f);
                    break;

                case 2:
                    add(1, px, groundY);
                    add(1, px + 40.f, groundY + 120.f);
                    break;

                case 3:
                    add(8, px, groundY + 120.f);
                    add(8, px + 45.f, groundY);
                    break;

                case 4:
                    addOrb(px + 25.f, groundY + 75.f);
                    add(1, px + 90.f, groundY);
                    break;
            }
        };

        for (int i = 0; i < beats && placed < maxObjects; i++) {
            x += spacing + rf(-7.f, 7.f);

            // Change gamemode every 32 beats.
            if (i > 0 && i % 32 == 0) {
                currentMode = (currentMode + 1) % 7;
                add(modePortals[currentMode], x, groundY);
                x += 55.f;
            }

            // Change speed at section boundaries.
            if (i > 0 && i % 48 == 0) {
                int speedIndex = (i / 48) % 4;
                add(speedPortals[speedIndex], x, groundY);
                x += 45.f;
            }

            // Higher difficulty adds more pattern density.
            int repeats = 1;

            if (m_difficulty >= 5 && ri(0, 3) == 0)
                repeats = 2;

            if (m_difficulty >= 8 && ri(0, 4) == 0)
                repeats = 3;

            for (int r = 0; r < repeats; r++) {
                float px = x + r * 65.f;

                switch (currentMode) {
                    case 0: cubePattern(px); break;
                    case 1: shipPattern(px); break;
                    case 2: ballPattern(px); break;
                    case 3: ufoPattern(px); break;
                    case 4: wavePattern(px); break;
                    case 5: robotPattern(px); break;
                    case 6: spiderPattern(px); break;
                }

                if (placed >= maxObjects)
                    break;
            }

            // Occasional empty beat for rhythm variation.
            int restChance = 0;

            if (m_difficulty <= 2) restChance = 3;
            else if (m_difficulty <= 5) restChance = 2;
            else restChance = 1;

            if (ri(0, 9) < restChance)
                x += spacing * rf(.2f, .5f);
        }

        return placed;
    }

public:
    static AutoBuildPopup* create() {
        auto ret = new AutoBuildPopup();

        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }

        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class $modify(AutoBuildEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer))
            return false;

        auto menu = CCMenu::create();
        menu->setID("autobuild-button-menu");

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        menu->setPosition({
            winSize.width - 38.f,
            winSize.height - 85.f
        });

        auto sprite = CCSprite::createWithSpriteFrameName(
            "GJ_button_01.png"
        );

        if (!sprite)
            return true;

        sprite->setColor({150, 40, 220});

        auto label = CCLabelBMFont::create(
            "E", "bigFont.fnt"
        );

        label->setPosition(
            sprite->getContentSize() / 2
        );

        sprite->addChild(label);

        auto button = CCMenuItemSpriteExtra::create(
            sprite,
            this,
            menu_selector(AutoBuildEditorUI::onAutoBuild)
        );

        menu->addChild(button);
        this->addChild(menu, 100);

        return true;
    }

    void onAutoBuild(CCObject*) {
        auto popup = AutoBuildPopup::create();

        if (popup)
            popup->show();
    }
};
