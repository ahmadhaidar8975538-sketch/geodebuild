#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>

#include <algorithm>
#include <cstdlib>
#include <string>

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
                "Generation finished.\nObjects created: {}",
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
        const float groundY = 105.f;
        const float beatInterval = 60.f / bpm;

        float spacing = beatInterval * 300.f;
        spacing *= 1.f - (m_difficulty * .035f);
        spacing = std::clamp(spacing, 65.f, 180.f);

        int beats = static_cast<int>(
            length / beatInterval
        );

        beats = std::min(beats, 900);

        float x = 300.f;
        int placed = 0;

        auto addObject = [&](int id, float px, float py) {
            if (placed >= 1800)
                return;

            auto object = editor->createObject(
                id, ccp(px, py), false
            );

            if (object)
                placed++;
        };

        for (int i = 0; i < beats && placed < 1800; i++) {
            x += spacing;

            int pattern = i % 8;

            switch (pattern) {
                case 0:
                    addObject(8, x, groundY);
                    break;

                case 1:
                    if (m_difficulty >= 3) {
                        addObject(8, x, groundY);
                        addObject(8, x + 30.f, groundY);
                    }
                    break;

                case 2:
                    addObject(1, x, groundY);
                    addObject(1, x + 30.f, groundY);
                    break;

                case 3:
                    if (m_difficulty >= 2) {
                        addObject(36, x, groundY + 60.f);
                        addObject(1, x + 75.f, groundY);
                    }
                    break;

                case 4:
                    addObject(39, x, groundY);

                    if (m_difficulty >= 6)
                        addObject(8, x + 45.f, groundY);
                    break;

                case 5:
                    if (m_difficulty >= 4) {
                        addObject(1, x, groundY);
                        addObject(36, x + 35.f, groundY + 60.f);
                    }
                    break;

                case 6:
                    addObject(8, x, groundY);

                    if (m_difficulty >= 7)
                        addObject(8, x + 35.f, groundY);
                    break;

                case 7:
                    addObject(1, x, groundY);
                    break;
            }
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
