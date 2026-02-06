// waveform_visualizer_english.cpp
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include "SmoothValue.h"

const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;
const int WAVEFORM_POINTS = 500;  // Number of waveform points
const float WAVEFORM_HEIGHT = 300.0f;  // Waveform display height

// Audio energy calculation function
float calculateVolume(const sf::Int16* samples, size_t count) {
    if (count == 0) return 0.0f;

    float sumSquares = 0.0f;
    for (size_t i = 0; i < count; i++) {
        float sample = samples[i] / 32768.0f;
        sumSquares += sample * sample;
    }

    return std::sqrt(sumSquares / count);
}

// Waveform visualizer class
class WaveformVisualizer {
private:
    sf::VertexArray waveform;  // Waveform vertex array
    std::vector<float> audioBuffer;  // Audio buffer
    float scaleFactor;  // Waveform scaling factor

public:
    WaveformVisualizer()
        : waveform(sf::LineStrip, WAVEFORM_POINTS), scaleFactor(100.0f) {

        // Initialize waveform vertices
        for (int i = 0; i < WAVEFORM_POINTS; i++) {
            float x = static_cast<float>(i) / (WAVEFORM_POINTS - 1) * WINDOW_WIDTH;
            waveform[i].position = sf::Vector2f(x, WINDOW_HEIGHT / 2);
            waveform[i].color = sf::Color::White;
        }
    }

    void update(const std::vector<float>& samples, float volume, float time) {
        if (samples.empty()) return;

        // Adjust waveform amplitude based on volume
        float amplitude = scaleFactor * (5.0f + volume * 15.0f);

        // Add some dynamic changes based on time
        float timeOffset = sin(time * 2.0f) * 10.0f;

        // Update waveform
        for (int i = 0; i < WAVEFORM_POINTS; i++) {
            // Calculate sample index (uniform sampling)
            int sampleIndex = (i * samples.size()) / WAVEFORM_POINTS;
            sampleIndex = std::min(sampleIndex, (int)samples.size() - 1);

            // Get sample value
            float sampleValue = samples[sampleIndex];

            // Apply smoothing filter
            static std::vector<float> prevValues(WAVEFORM_POINTS, 0.0f);
            float smoothedValue = prevValues[i] * 0.7f + sampleValue * 0.3f;
            prevValues[i] = smoothedValue;

            // Calculate y coordinate (centered)
            float x = static_cast<float>(i) / (WAVEFORM_POINTS - 1) * WINDOW_WIDTH;
            float y = WINDOW_HEIGHT / 2 + smoothedValue * amplitude;

            // Add time offset to make waveform dynamic
            y += sin(i * 0.1f + time * 3.0f) * (5.0f + volume * 15.0f);

            waveform[i].position = sf::Vector2f(x, y);

            // Set color based on position (rainbow gradient)
            float hue = fmod(i * 0.5f + time * 50.0f, 360.0f);
            float ratio = hue / 60.0f;
            int sector = static_cast<int>(ratio) % 6;
            float fraction = ratio - sector;

            sf::Uint8 r, g, b;
            switch (sector) {
            case 0: r = 255; g = static_cast<sf::Uint8>(fraction * 255); b = 0; break;
            case 1: r = static_cast<sf::Uint8>((1 - fraction) * 255); g = 255; b = 0; break;
            case 2: r = 0; g = 255; b = static_cast<sf::Uint8>(fraction * 255); break;
            case 3: r = 0; g = static_cast<sf::Uint8>((1 - fraction) * 255); b = 255; break;
            case 4: r = static_cast<sf::Uint8>(fraction * 255); g = 0; b = 255; break;
            default: r = 255; g = 0; b = static_cast<sf::Uint8>((1 - fraction) * 255); break;
            }

            // Adjust transparency based on volume
            sf::Uint8 alpha = static_cast<sf::Uint8>(150 + volume * 105);
            waveform[i].color = sf::Color(r, g, b, alpha);
        }
    }

    void draw(sf::RenderTarget& target) {
        for (int offset = -10; offset <= 10; offset++) {
            sf::VertexArray thickLine = waveform;
            for (unsigned int i = 0; i < thickLine.getVertexCount(); i++) {
                thickLine[i].position.y += static_cast<float>(offset) * 0.3f;
            }
            target.draw(thickLine);
        }

        // Add some visual effects: draw gradient background below waveform
        sf::VertexArray background(sf::Quads, 4);
        background[0].position = sf::Vector2f(0, WINDOW_HEIGHT / 2 - WAVEFORM_HEIGHT / 2);
        background[1].position = sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT / 2 - WAVEFORM_HEIGHT / 2);
        background[2].position = sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT / 2 + WAVEFORM_HEIGHT / 2);
        background[3].position = sf::Vector2f(0, WINDOW_HEIGHT / 2 + WAVEFORM_HEIGHT / 2);

        background[0].color = sf::Color(10, 10, 40, 50);
        background[1].color = sf::Color(10, 10, 40, 50);
        background[2].color = sf::Color(10, 10, 40, 0);
        background[3].color = sf::Color(10, 10, 40, 0);

        target.draw(background);
    }

    void setScaleFactor(float scale) {
        scaleFactor = scale;
    }
};

int main() {
    std::cout << "=== Waveform Visualizer Demo ===" << std::endl;
    std::cout << "Visualizing audio waveform over time" << std::endl;

    // 1. Create window
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT),
        "Waveform Visualizer - Audio Waveform");
    window.setFramerateLimit(60);

    // 2. Load audio
    sf::SoundBuffer soundBuffer;
    std::string musicPath = "C:\\Users\\zhaok\\Desktop\\dvorak_new_world.mp3";

    if (!soundBuffer.loadFromFile(musicPath)) {
        std::cerr << "Error: Unable to load audio file!" << std::endl;
        std::cout << "Trying to load test.mp3..." << std::endl;
        musicPath = "test.mp3";
        if (!soundBuffer.loadFromFile(musicPath)) {
            std::cerr << "Error: Unable to load any audio file!" << std::endl;
            std::cout << "Will use simulated audio data..." << std::endl;
        }
        else {
            std::cout << "Audio loaded successfully!" << std::endl;
        }
    }
    else {
        std::cout << "Audio loaded successfully!" << std::endl;
    }

    // 3. Create audio player
    sf::Sound sound;
    bool hasAudio = false;

    if (soundBuffer.getSampleCount() > 0) {
        sound.setBuffer(soundBuffer);
        hasAudio = true;

        std::cout << "Sample rate: " << soundBuffer.getSampleRate() << " Hz" << std::endl;
        std::cout << "Channels: " << soundBuffer.getChannelCount() << std::endl;
        std::cout << "Duration: " << soundBuffer.getDuration().asSeconds() << " seconds" << std::endl;
    }

    // 4. Get audio data
    const sf::Int16* allSamples = nullptr;
    size_t totalSamples = 0;
    unsigned int sampleRate = 44100;
    unsigned int channels = 2;

    if (hasAudio) {
        allSamples = soundBuffer.getSamples();
        totalSamples = soundBuffer.getSampleCount();
        sampleRate = soundBuffer.getSampleRate();
        channels = soundBuffer.getChannelCount();
    }

    // 5. Create waveform visualizer
    WaveformVisualizer waveform;

    // 6. Time management
    sf::Clock frameClock;
    sf::Clock audioClock;
    bool isPlaying = false;
    int frameCount = 0;

    // 7. Smooth volume
    SmoothValue<float> smoothedVolume(0.0f, 10.0f);

    // 8. Add particle system as background (optional)
    std::vector<sf::CircleShape> backgroundParticles;
    for (int i = 0; i < 100; i++) {
        sf::CircleShape particle(1.0f + (rand() % 100) / 100.0f * 3.0f);
        particle.setPosition(
            rand() % WINDOW_WIDTH,
            rand() % WINDOW_HEIGHT
        );
        particle.setFillColor(sf::Color(50, 100, 200, 30));
        backgroundParticles.push_back(particle);
    }

    std::cout << "\n=== Ready ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  SPACE - Play/Pause" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "  R - Restart" << std::endl;
    std::cout << "  + - Increase waveform amplitude" << std::endl;
    std::cout << "  - - Decrease waveform amplitude" << std::endl;
    std::cout << "  C - Toggle color mode" << std::endl;

    float currentScale = 100.0f;
    bool colorMode = true;  // true: Colorful, false: Monochromatic

    // Main loop
    while (window.isOpen()) {
        float dt = frameClock.restart().asSeconds();
        frameCount++;

        // Event handling
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space) {
                    if (hasAudio) {
                        if (sound.getStatus() == sf::Sound::Playing) {
                            sound.pause();
                            isPlaying = false;
                            std::cout << "Music paused" << std::endl;
                        }
                        else {
                            sound.play();
                            isPlaying = true;
                            std::cout << "Music playing" << std::endl;
                        }
                    }
                    else {
                        isPlaying = !isPlaying;
                        std::cout << (isPlaying ? "Simulation playing" : "Simulation paused") << std::endl;
                    }
                }

                if (event.key.code == sf::Keyboard::Escape)
                    window.close();

                if (event.key.code == sf::Keyboard::R) {
                    if (hasAudio) {
                        sound.stop();
                        sound.play();
                        isPlaying = true;
                        audioClock.restart();
                        std::cout << "Restarted playback" << std::endl;
                    }
                    else {
                        audioClock.restart();
                        std::cout << "Reset simulation time" << std::endl;
                    }
                }

                if (event.key.code == sf::Keyboard::Add || event.key.code == sf::Keyboard::Equal) {
                    currentScale += 20.0f;
                    waveform.setScaleFactor(currentScale);
                    std::cout << "Waveform amplitude: " << currentScale << std::endl;
                }

                if (event.key.code == sf::Keyboard::Subtract || event.key.code == sf::Keyboard::Dash) {
                    currentScale = std::max(20.0f, currentScale - 20.0f);
                    waveform.setScaleFactor(currentScale);
                    std::cout << "Waveform amplitude: " << currentScale << std::endl;
                }

                if (event.key.code == sf::Keyboard::C) {
                    colorMode = !colorMode;
                    std::cout << "Color mode: " << (colorMode ? "Colorful" : "Monochromatic") << std::endl;
                }
            }
        }

        // Clear screen
        window.clear(sf::Color(10, 10, 30));

        // Draw background particles
        for (auto& particle : backgroundParticles) {
            // Move particles slowly
            particle.move(0.1f, 0.05f);

            // Reset position if particle moves off screen
            if (particle.getPosition().y > WINDOW_HEIGHT) {
                particle.setPosition(
                    rand() % WINDOW_WIDTH,
                    -10.0f
                );
            }

            window.draw(particle);
        }

        // Get audio data and time
        float currentTime = 0.0f;
        float currentVolume = 0.0f;
        std::vector<float> currentSamples;

        if (hasAudio && isPlaying) {
            // Get current playback time
            currentTime = sound.getPlayingOffset().asSeconds();

            // Calculate sample index
            size_t sampleIndex = static_cast<size_t>(
                currentTime * sampleRate * channels
                );

            // Analysis window size
            const size_t ANALYSIS_SIZE = 4096;

            if (sampleIndex + ANALYSIS_SIZE < totalSamples) {
                // Calculate current volume
                currentVolume = calculateVolume(
                    &allSamples[sampleIndex],
                    std::min(ANALYSIS_SIZE, totalSamples - sampleIndex)
                );

                // Extract sample data for waveform display
                currentSamples.resize(ANALYSIS_SIZE);
                for (size_t i = 0; i < ANALYSIS_SIZE; i++) {
                    if (sampleIndex + i < totalSamples) {
                        // If stereo, take average
                        if (channels == 2) {
                            float left = allSamples[sampleIndex + i * 2] / 32768.0f;
                            float right = allSamples[sampleIndex + i * 2 + 1] / 32768.0f;
                            currentSamples[i] = (left + right) / 2.0f;
                        }
                        else {
                            currentSamples[i] = allSamples[sampleIndex + i] / 32768.0f;
                        }
                    }
                    else {
                        currentSamples[i] = 0.0f;
                    }
                }
            }
        }
        else if (isPlaying) {
            // Simulate audio data (for demonstration)
            currentTime = audioClock.getElapsedTime().asSeconds();

            // Generate simulated audio data
            currentSamples.resize(4096);
            float baseFrequency = 220.0f;  // A3 note
            float melodyFrequency = 440.0f;  // A4 note

            for (size_t i = 0; i < currentSamples.size(); i++) {
                // Base waveform (sine wave)
                float t = currentTime + i / 44100.0f;

                // Melody (high frequency)
                float melody = sin(t * melodyFrequency * 2.0f * 3.14159f);

                // Harmony (low frequency)
                float harmony = sin(t * baseFrequency * 2.0f * 3.14159f) * 0.3f;

                // Rhythm (envelope)
                float envelope = 0.5f + 0.5f * sin(t * 2.0f);

                // Combine waveforms
                currentSamples[i] = (melody + harmony) * envelope * 0.5f;

                // Add some noise to make the waveform more realistic
                currentSamples[i] += ((rand() % 100) / 100.0f - 0.5f) * 0.05f;
            }

            // Simulated volume (changes over time)
            currentVolume = 0.5f + 0.3f * sin(currentTime * 0.5f);
        }

        // Smooth volume
        smoothedVolume.setTarget(currentVolume);
        smoothedVolume.update(dt);

        // Update waveform visualizer
        if (!currentSamples.empty()) {
            waveform.update(currentSamples, smoothedVolume.getCurrent(), currentTime);
        }

        // Draw waveform
        waveform.draw(window);

        // Draw center reference line
        sf::RectangleShape centerLine(sf::Vector2f(WINDOW_WIDTH, 1));
        centerLine.setPosition(0, WINDOW_HEIGHT / 2);
        centerLine.setFillColor(sf::Color(255, 255, 255, 50));
        window.draw(centerLine);

        // Draw UI information
        sf::Font font;
        if (font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
            sf::Text infoText;
            infoText.setFont(font);
            infoText.setCharacterSize(20);
            infoText.setFillColor(sf::Color::White);
            infoText.setPosition(20, 20);

            std::string info = "Waveform Visualizer Demo\n";
            info += hasAudio ? "Audio file: Loaded" : "Audio file: Simulated data";
            info += "\nStatus: " + std::string(isPlaying ? "Playing" : "Paused");
            info += "\nVolume: " + std::to_string(smoothedVolume.getCurrent());
            info += "\nTime: " + std::to_string(currentTime) + "s";
            info += "\nWaveform points: " + std::to_string(WAVEFORM_POINTS);
            info += "\nWaveform amplitude: " + std::to_string(currentScale);
            info += "\nColor mode: " + std::string(colorMode ? "Colorful" : "Monochromatic");

            infoText.setString(info);
            window.draw(infoText);

            // Draw control instructions
            sf::Text controlsText;
            controlsText.setFont(font);
            controlsText.setCharacterSize(18);
            controlsText.setFillColor(sf::Color(200, 200, 200));
            controlsText.setPosition(WINDOW_WIDTH - 300, 20);

            std::string controls = "Controls:\n";
            controls += "SPACE: Play/Pause\n";
            controls += "R: Restart\n";
            controls += "+/-: Adjust waveform amplitude\n";
            controls += "C: Toggle color mode\n";
            controls += "ESC: Exit";

            controlsText.setString(controls);
            window.draw(controlsText);
        }

        // Draw waveform description
        sf::Text waveText;
        waveText.setFont(font);
        waveText.setCharacterSize(24);
        waveText.setFillColor(sf::Color(150, 200, 255));
        waveText.setPosition(WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT - 100);
        window.draw(waveText);

        // Display final frame
        window.display();
    }

    std::cout << "\nProgram finished" << std::endl;
    return 0;
}