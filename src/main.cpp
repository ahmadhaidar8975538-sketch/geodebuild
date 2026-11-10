#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/EditorUI.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

// Global settings stored from popup inputs
class AutoBuildSettings {
public:
    int difficultyIndex = 0; // 0: Easy to 9: Extreme Demon
    float bpm = 120.0f;
    float lengthSeconds = 60.0f;

    static AutoBuildSettings& get() {
        static AutoBuildSettings instance;
        return instance;
    }
};

// Popup menu for configuring generator parameters
class AutoBuildPopup : public Popup<> {
protected:
    TextInput* m_bpmInput = nullptr;
    TextInput* m_lengthInput = nullptr;
    CCMenuItemToggler* m_diffTogglers[10];
    int m_selectedDifficulty = 0;

    bool setup() override {
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        
        // Background / Title
        this->setTitle("AutoBuild Layout", "goldFont.fnt", 0.8f);

        // Menu root
        auto menu = CCMenu::create();
        menu->setPosition({0, 0});
        this->m_mainLayer->addChild(menu);

        float startY = winSize.height / 2 + 60;
        float startX = winSize.width / 2 - 140;

        // Difficulty selection labels & buttons
        auto diffLabel = CCLabelBMFont::create("Difficulty:", "bigFont.fnt");
        diffLabel->setScale(0.4f);
        diffLabel->setPosition({startX + 40, startY});
        diffLabel->setAnchorPoint({0.0f, 0.5f});
        this->m_mainLayer->addChild(diffLabel);

        const char* diffNames[10] = {
            "Easy", "Normal", "Hard", "Harder", "Insane",
            "Easy Dem", "Med Dem", "Hard Dem", "Insane Dem", "Extreme"
        };

        // Scrollable or compact difficulty selector buttons
        auto diffMenu = CCMenu::create();
        diffMenu->setPosition({startX + 130, startY});
        this->m_mainLayer->addChild(diffMenu);

        // Simple cycling difficulty button
        auto diffButtonLabel = CCLabelBMFont::create(diffNames[m_selectedDifficulty], "goldFont.fnt");
        diffButtonLabel->setScale(0.4f);
        
        auto diffBtn = CCMenuItemSpriteExtra::create(
            diffButtonLabel,
            this,
            menu_selector(AutoBuildPopup::onToggleDifficulty)
        );
        diffBtn->setTag(100);
        diffBtn->setPosition({0, 0});
        diffMenu->addChild(diffBtn);

        // BPM Input
        startY -= 45;
        auto bpmLbl = CCLabelBMFont::create("BPM:", "bigFont.fnt");
        bpmLbl->setScale(0.4f);
        bpmLbl->setPosition({startX + 40, startY});
        bpmLbl->setAnchorPoint({0.0f, 0.5f});
        this->m_mainLayer->addChild(bpmLbl);

        m_bpmInput = TextInput::create(100, "120", "chatFont.fnt");
        m_bpmInput->setString("120");
        m_bpmInput->setPosition({startX + 180, startY});
        m_mainLayer->addChild(m_bpmInput);

        // Length Input (Seconds)
        startY -= 45;
        auto lenLbl = CCLabelBMFont::create("Length (s):", "bigFont.fnt");
        lenLbl->setScale(0.4f);
        lenLbl->setPosition({startX + 40, startY});
        lenLbl->setAnchorPoint({0.0f, 0.5f});
        this->m_mainLayer->addChild(lenLbl);

        m_lengthInput = TextInput::create(100, "60", "chatFont.fnt");
        m_lengthInput->setString("60");
        m_lengthInput->setPosition({startX + 180, startY});
        m_mainLayer->addChild(m_lengthInput);

        // CREATE Button
        auto createBtnSprite = ButtonSprite::create("CREATE", "goldFont.fnt", "GJ_button_01.png", 0.9f);
        auto createBtn = CCMenuItemSpriteExtra::create(
            createBtnSprite,
            this,
            menu_selector(AutoBuildPopup::onCreateClicked)
        );
        createBtn->setPosition({winSize.width / 2, winSize.height / 2 - 95});
        menu->addChild(createBtn);

        return true;
    }

    void onToggleDifficulty(CCObject* sender) {
        const char* diffNames[10] = {
            "Easy", "Normal", "Hard", "Harder", "Insane",
            "Easy Dem", "Med Dem", "Hard Dem", "Insane Dem", "Extreme"
        };
        m_selectedDifficulty = (m_selectedDifficulty + 1) % 10;
        
        auto btn = typeinfo_cast<CCMenuItemSpriteExtra*>(sender);
        if (btn) {
            auto label = typeinfo_cast<CCLabelBMFont*>(btn->getNormalImage());
            if (label) {
                label->setString(diffNames[m_selectedDifficulty]);
            }
        }
    }

    void onCreateClicked(CCObject* sender) {
        // Parse inputs
        float bpm = 120.0f;
        float length = 60.0f;

        if (m_bpmInput) {
            std::string s = m_bpmInput->getString();
            try { bpm = std::stof(s); } catch(...) { bpm = 120.0f; }
        }
        if (m_lengthInput) {
            std::string s = m_lengthInput->getString();
            try { length = std::stof(s); } catch(...) { length = 60.0f; }
        }

        AutoBuildSettings::get().difficultyIndex = m_selectedDifficulty;
        AutoBuildSettings::get().bpm = bpm > 20.0f ? bpm : 120.0f;
        AutoBuildSettings::get().lengthSeconds = length > 5.0f ? length : 30.0f;

        // Run generator
        generateLayout();

        this->onClose(sender);
        FLAlertLayer::create("Success!", "AutoBuild layout generated successfully!", "OK")->show();
    }

    void generateLayout() {
        auto editor = LevelEditorLayer::get();
        if (!editor) return;

        auto settings = AutoBuildSettings::get();
        float bpm = settings.bpm;
        float duration = settings.lengthSeconds;
        int diff = settings.difficultyIndex;

        // Calculate spacing based on BPM and speed (assume standard speed ~ 10.5 blocks per second)
        // Beat interval in seconds: 60 / bpm
        float beatInterval = 60.0f / bpm;
        float currentX = 150.0f; // Starting X in editor
        float startY = 105.0f;  // Ground level

        // Object IDs in GD 2.2
        // Block: 1, Spike: 8, Small Spike: 39, Yellow Orb: 36, Pink Orb: 141, Gravity Portal: 10, Speed Portal (2x): 203
        
        float timeElapsed = 0.0f;
        int objectCount = 0;

        // Add initial cube and start trigger / speed if needed
        // Generate procedural obstacles
        while (timeElapsed < duration && objectCount < 2500) {
            // Determine structure pattern based on beat interval & difficulty
            float stepDistance = beatInterval * 10.0f * (1.0f + (diff * 0.05f));
            if (stepDistance < 15.0f) stepDistance = 15.0f;

            currentX += stepDistance;
            timeElapsed += beatInterval;

            // Pattern variation based on diff & iteration
            int patternType = objectCount % 6;

            if (patternType == 0 || patternType == 1) {
                // Standard jump obstacle (Spike or double spike for higher diffs)
                int spikeId = (diff >= 5 && (objectCount % 2 == 0)) ? 39 : 8;
                auto spike = editor->createObject(spikeId, {currentX, startY});
                if (spike) {
                    spike->setZOrder(10);
                }
                
                // Add a platform or jump orb above or before for timing
                if (diff >= 3 && (objectCount % 3 == 0)) {
                    auto orb = editor->createObject(36, {currentX - 15.0f, startY + 45.0f});
                    if (orb) orb->setZOrder(12);
                }
            } 
            else if (patternType == 2) {
                // Floating platform with spikes underneath or jump challenge
                for (int p = 0; p < 3; p++) {
                    auto plat = editor->createObject(1, {currentX + (p * 30.0f), startY + 60.0f});
                    if (plat) plat->setZOrder(10);
                }
                auto spikeUnder = editor->createObject(8, {currentX + 30.0f, startY});
                if (spikeUnder) spikeUnder->setZOrder(10);
            }
            else if (patternType == 3 && diff >= 2) {
                // Orb chain / timing click
                int orbId = (objectCount % 2 == 0) ? 36 : 141; // Yellow or Pink orb
                auto orb = editor->createObject(orbId, {currentX, startY + 50.0f});
                if (orb) orb->setZOrder(12);

                // Landing block
                auto block = editor->createObject(1, {currentX + 60.0f, startY});
                if (block) block->setZOrder(10);
            }
            else if (patternType == 4 && diff >= 6) {
                // Extreme Demon tight timing pillars
                auto topSpike = editor->createObject(8, {currentX, startY + 90.0f});
                if (topSpike) {
                    topSpike->setRotation(180.0f);
                    topSpike->setZOrder(10);
                }
                auto botSpike = editor->createObject(8, {currentX, startY});
                if (botSpike) botSpike->setZOrder(10);
            }
            else {
                // Standard structure block column
                auto block = editor->createObject(1, {currentX, startY + 30.0f});
                if (block) block->setZOrder(10);
            }

            objectCount++;
        }
    }

public:
    static void showPopup() {
        auto popup = new AutoBuildPopup();
        if (popup && popup->initBorders(360.0f, 260.0f)) {
            popup->autorelease();
            popup->show();
        }
    }
};

// Hook into EditorUI to add the purple "E" AutoBuild button
#include <Geode/modify/EditorUI.hpp>
class $modify(MyEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) {
            return false;
        }

        // Create the purple square button with letter E
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        
        // Custom purple background sprite or tinted menu button
        auto sprBg = CCLayerColor::create({128, 0, 128, 255}, 36.0f, 36.0f);
        sprBg->ignoreAnchorPointForPosition(false);

        auto labelE = CCLabelBMFont::create("E", "goldFont.fnt");
        labelE->setScale(0.7f);
        labelE->setPosition({18.0f, 18.0f});
        sprBg->addChild(labelE);

        auto btn = CCMenuItemSpriteExtra::create(
            sprBg,
            this,
            menu_selector(MyEditorUI::onAutoBuildClicked)
        );

        // Find bottom or top menu container in EditorUI
        // Typically added to the top-left or top-right toolbar menu
        auto topMenu = this->getChildByID("top-right-menu");
        if (!topMenu) {
            // Fallback: create a new menu if layout node missing
            topMenu = CCMenu::create();
            topMenu->setPosition({winSize.width - 40, winSize.height - 40});
            this->addChild(topMenu, 100);
        }

        topMenu->addChild(btn);
        topMenu->updateLayout();

        return true;
    }

    void onAutoBuildClicked(CCObject* sender) {
        AutoBuildPopup::showPopup();
    }
};
