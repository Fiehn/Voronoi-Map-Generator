#pragma once
#include <functional>

class MenuButton {
public:
    MenuButton(sf::RenderWindow* window, const sf::Font& font, const std::string& text, const sf::Vector2f& size, const sf::Vector2f& position)
        : bwindow(window), bfont(font), btext(text, font, 30), bbox(size), bshadow(size), bsize(size), bposition(position) {
        create_button();
    }

    void create_button() {
        // Adjust text size to fit within the button
        unsigned int charSize = static_cast<unsigned int>(bsize.y * 0.6);
        btext.setCharacterSize(charSize);
        sf::FloatRect textBounds = btext.getLocalBounds();

        // Ensure text fits within the button width
        while (textBounds.width > bsize.x * 0.9f && charSize > 10) {
            charSize -= 2;
            btext.setCharacterSize(charSize);
            textBounds = btext.getLocalBounds();
        }

        // Center text
        btext.setOrigin(textBounds.left + textBounds.width / 2.0f, textBounds.top + textBounds.height / 2.0f);
        btext.setPosition(bposition.x + bsize.x / 2.0f, bposition.y + bsize.y / 2.0f);

        // Set up button appearance
        bbox.setPosition(bposition);
        bbox.setFillColor(sf::Color(200, 200, 200));
        bbox.setOutlineThickness(2);
        bbox.setOutlineColor(sf::Color::Black);

        // Set up shadow
        bshadow.setPosition(bposition.x + 3, bposition.y + 3);
        bshadow.setFillColor(sf::Color(0, 0, 0, 50));
    }

    void draw_button() const {
        bwindow->draw(bshadow);
        bwindow->draw(bbox);
        bwindow->draw(btext);
    }

    bool handle_event(const sf::Event& event) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(*bwindow);
            if (bbox.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
                if (on_click) on_click();
                return true;
            }
        }
        return false;
    }

    // Callback for click events
    std::function<void()> on_click;

private:
    sf::RenderWindow* bwindow;
    const sf::Font& bfont;
    sf::Text btext;
    sf::RectangleShape bbox;
    sf::RectangleShape bshadow;
    sf::Vector2f bsize;
    sf::Vector2f bposition;
};

bool mainMenu(sf::RenderWindow& window, MapConfig& config, unsigned int MAXWIDTH, unsigned int MAXHEIGHT, bool& showMainMenu)
{
    sf::Font font;
    if (!font.loadFromFile("Fonts/Roboto-Medium.ttf")) { std::cout << "Could not load font" << std::endl; }

    // Screen elements
    sf::Texture menuTexture;
    sf::Sprite menuSprite;
    sf::RectangleShape loadingBar(sf::Vector2f(0, 50));
    sf::Text progressText;

    // Initialize loading elements
    loadingBar.setFillColor(sf::Color::Green);
    loadingBar.setPosition(100, MAXHEIGHT - 100);
    progressText.setFont(font);
    progressText.setFillColor(sf::Color::Black);
    progressText.setCharacterSize(40);
    progressText.setPosition(MAXWIDTH / 2 - progressText.getGlobalBounds().width / 2, MAXHEIGHT / 2 - progressText.getGlobalBounds().height / 2);

    // Create credit text
	sf::Text creditText("Created by: Fiehn", font, 20);
	creditText.setPosition(MAXWIDTH - creditText.getGlobalBounds().width - 10, MAXHEIGHT - creditText.getGlobalBounds().height - 10);

    // Load a random image from assets/MenuScreens
    std::vector<std::string> menuImages;
    for (const auto& entry : std::filesystem::directory_iterator("assets/MenuScreens")) {
        if (entry.path().extension() == ".png" || entry.path().extension() == ".jpg") {
            menuImages.push_back(entry.path().string());
        }
    }
    if (!menuImages.empty()) {
        std::string randomImagePath = menuImages[std::rand() % menuImages.size()];
        if (menuTexture.loadFromFile(randomImagePath)) {
            menuSprite.setTexture(menuTexture);
            menuSprite.setPosition(MAXWIDTH / 2 - menuSprite.getGlobalBounds().width / 2,
                MAXHEIGHT / 2 - menuSprite.getGlobalBounds().height / 2);
        }
    }
    else {
        std::cout << "No loading images found in assets/LoadingScreens\n";
        progressText.setFillColor(sf::Color::Black);
    }

    MenuButton randomButton(&window, font, "Generate Random World", sf::Vector2f(300, 80), sf::Vector2f(MAXWIDTH/2 - 150, 250));
    randomButton.on_click = [&]() { std::cout << "Random world starting.." << std::endl; showMainMenu = false; };

	bool showLoadConfig = false;
	bool showSaveConfig = false;
	MenuButton loadConfigFile(&window, font, "Load Config File", sf::Vector2f(300, 80), sf::Vector2f(MAXWIDTH / 2 - 150, 400));
    loadConfigFile.on_click = [&]() { showLoadConfig = true; std::cout << "Load config" << std::endl; };
    
	bool exit = false;
	MenuButton exitButton(&window, font, "Exit", sf::Vector2f(300, 80), sf::Vector2f(MAXWIDTH / 2 - 150, 550));
    exitButton.on_click = [&]() { std::cout << "Exiting" << std::endl; exit = true; };

    sf::Clock deltaClock;

    while (showMainMenu)
    {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            if (event.type == sf::Event::Closed)
            {
                return false;
            }
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
            {
                return false;
            }
            randomButton.handle_event(event);
			loadConfigFile.handle_event(event);
            exitButton.handle_event(event);
        }
        ImGui::SFML::Update(window, deltaClock.restart());

        configLoadSave(config, showLoadConfig, showSaveConfig);

        window.clear();

        window.draw(menuSprite);
        window.draw(creditText);

        randomButton.draw_button();
		loadConfigFile.draw_button();
		exitButton.draw_button();

		if (exit == true) {
            return false;
		}
        
        ImGui::SFML::Render(window);

        window.display();
    }
	return true;
}