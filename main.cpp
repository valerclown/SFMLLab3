#include <SFML/Graphics.hpp>
#include <functional>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <filesystem> 

#include "imgui.h"
#include "imgui-SFML.h"
enum class ImageState {
    RedCircle,
    FirstGradient,
    SecondGradient
};    ImageState currentState = ImageState::RedCircle;

class RFuncSprite : public sf::Sprite {
public:
    void Create(const sf::Vector2u& size) {
        _image.create(size.x, size.y, sf::Color::Blue);
        _texture.loadFromImage(_image);
        setTexture(_texture);
    }
    bool useGradient = false;
    bool useSecondGradient = false;




    void DrawRFunc(const std::function<float(const sf::Vector2f&)>& rfunc, const sf::FloatRect& subSpace) {
        sf::Vector2f spaceStep = {
            subSpace.width / static_cast<float>(_image.getSize().x),
            subSpace.height / static_cast<float>(_image.getSize().y)
        };

        for (unsigned x = 0; x < _image.getSize().x; ++x) {
            for (unsigned y = 0; y < _image.getSize().y; ++y) {
                sf::Vector2f spacePoint = {
                    subSpace.left + x * spaceStep.x,
                    subSpace.top + y * spaceStep.y
                };

                float z = rfunc(spacePoint);
                if (z > 0) {
                    if (useGradient) {

                        float gradient = static_cast<float>(x) / _image.getSize().x;
                        sf::Uint8 colorIntensity = static_cast<sf::Uint8>(255 * gradient);
                        _image.setPixel(x, y, sf::Color(colorIntensity, colorIntensity, colorIntensity));
                    }
                    else if (useSecondGradient) {

                        float gradient = static_cast<float>(y) / _image.getSize().y;
                        sf::Uint8 colorIntensity = static_cast<sf::Uint8>(255 * gradient);
                        _image.setPixel(x, y, sf::Color(colorIntensity, colorIntensity, colorIntensity));
                    }
                    else {
                        _image.setPixel(x, y, sf::Color::Red);
                    }
                }
            }
        }
        _texture.loadFromImage(_image);
    }

    bool SaveImageToFile(const std::string& filename) {

        std::filesystem::path dir("images");
        if (!std::filesystem::exists(dir)) {
            std::cout << "Directory 'images' does not exist. Creating..." << std::endl;
            if (!std::filesystem::create_directory(dir)) {
                std::cerr << "Failed to create directory 'images'." << std::endl;
                return false;
            }
        }

        std::filesystem::path filePath = dir / filename;
        std::cout << "Saving image to: " << filePath.string() << std::endl;

        if (!_image.saveToFile(filePath.string())) {
            std::cerr << "Failed to save image to " << filePath.string() << std::endl;
            return false;
        }
        return true;
    }


private:
    sf::Texture _texture;
    sf::Image _image;
};
void GradientDescent(sf::Image& pathImage, int startX, int startY, bool isDescent) {
    const int maxIterations = 1000;
    const float stepSize = 1.0f;
    float x = static_cast<float>(startX);
    float y = static_cast<float>(startY);

    const sf::Vector2f center = { 400.0f, 300.0f };

    for (int i = 0; i < maxIterations; ++i) {
        float gradientX = center.x - x;
        float gradientY = center.y - y;
        float length = std::sqrt(gradientX * gradientX + gradientY * gradientY);
        gradientX /= length;
        gradientY /= length;

        x += isDescent ? -stepSize * gradientX : stepSize * gradientX;
        y += isDescent ? -stepSize * gradientY : stepSize * gradientY;

        if (x < 0 || x >= pathImage.getSize().x || y < 0 || y >= pathImage.getSize().y) {
            break;
        }

        pathImage.setPixel(static_cast<unsigned int>(x), static_cast<unsigned int>(y), sf::Color::Red);
    }
}


int main() {
    bool isDescent = true;
    sf::RenderWindow window(sf::VideoMode(800, 600), "ImGui");
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);
    RFuncSprite rFuncSprite;
    rFuncSprite.Create(window.getSize());

    sf::Clock deltaClock;
    sf::Texture pathTexture;
    pathTexture.create(window.getSize().x, window.getSize().y);
    sf::Sprite pathSprite;
    pathSprite.setTexture(pathTexture);
    sf::Image pathImage;
    pathImage.create(window.getSize().x, window.getSize().y, sf::Color::Transparent);


    auto rfunc = [](const sf::Vector2f& point) -> float {
        float radius = 200.0f;
        sf::Vector2f center = { 400.0f, 300.0f };
        sf::Vector2f p = point - center;
        return radius * radius - (p.x * p.x + p.y * p.y);
        };

    rFuncSprite.DrawRFunc(rfunc, sf::FloatRect(0, 0, 800, 600));

    if (rFuncSprite.SaveImageToFile("saved_image.png")) {
        std::cout << "Image successfully saved to 'saved_image.png'" << std::endl;
    }
    else {
        std::cerr << "Failed to save image." << std::endl;
    }

    while (window.isOpen()) {
        sf::Time deltaTime = deltaClock.restart();
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    int mouseX = event.mouseButton.x;
                    int mouseY = event.mouseButton.y;
                    GradientDescent(pathImage, mouseX, mouseY, isDescent);
                    pathTexture.loadFromImage(pathImage);
                }
            }

        }

        ImGui::SFML::Update(window, deltaTime);

        window.clear(sf::Color::Blue);


        window.draw(rFuncSprite);


        ImGui::Begin("Control Panel");

        if (ImGui::Button("Red Circle")) {
            currentState = ImageState::RedCircle;
            rFuncSprite.useGradient = false;
            rFuncSprite.useSecondGradient = false;
            rFuncSprite.DrawRFunc(rfunc, sf::FloatRect(0, 0, 800, 600));
        }
        if (ImGui::Button("Apply Gradient")) {
            currentState = ImageState::FirstGradient;
            rFuncSprite.useGradient = true;
            rFuncSprite.useSecondGradient = false;
            rFuncSprite.DrawRFunc(rfunc, sf::FloatRect(0, 0, 800, 600));
        }

        if (ImGui::Button("Apply Second Gradient")) {
            currentState = ImageState::SecondGradient;
            rFuncSprite.useGradient = false;
            rFuncSprite.useSecondGradient = true;
            rFuncSprite.DrawRFunc(rfunc, sf::FloatRect(0, 0, 800, 600));
        }


        if (ImGui::Button("Save Image")) {
            std::string filename = "saved_image_";
            switch (currentState) {
            case ImageState::RedCircle:
                filename += "red_circle.png";
                break;
            case ImageState::FirstGradient:
                filename += "first_gradient.png";
                break;
            case ImageState::SecondGradient:
                filename += "second_gradient.png";
                break;
            }
            if (rFuncSprite.SaveImageToFile(filename)) {
                std::cout << "Image successfully saved to '" << filename << "'" << std::endl;
            }
            else {
                std::cerr << "Failed to save image." << std::endl;
            }
        }
        if (ImGui::Button("Clear Path")) {
            pathImage.create(window.getSize().x, window.getSize().y, sf::Color::Transparent);
            pathTexture.loadFromImage(pathImage);
        }
        ImGui::Checkbox("Gradient Descent", &isDescent);

        ImGui::End();
        window.draw(pathSprite);

        ImGui::SFML::Render(window);

        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
