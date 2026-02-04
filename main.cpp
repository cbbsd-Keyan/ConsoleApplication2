/*// music_visualizer_final.cpp
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <complex>
#include "SmoothValue.h"

const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;
const int FFT_SIZE = 1024;
const int VISUAL_BANDS = 256;

// 能量计算函数
float calculateEnergy(const sf::Int16* samples, size_t count) {
    if (count == 0) return 0.0f;
    float sumSquares = 0.0f;
    for (size_t i = 0; i < count; i++) {
        float sample = samples[i] / 32768.0f;
        sumSquares += sample * sample;
    }
    return std::sqrt(sumSquares / count);
}

// HSB到RGB转换函数
sf::Color hsbToRgb(float hue, float saturation, float brightness) {
    hue = fmod(hue, 360.0f);
    if (hue < 0) hue += 360.0f;
    saturation = std::clamp(saturation, 0.0f, 100.0f) / 100.0f;
    brightness = std::clamp(brightness, 0.0f, 100.0f) / 100.0f;

    float c = brightness * saturation;
    float x = c * (1.0f - fabs(fmod(hue / 60.0f, 2.0f) - 1.0f));
    float m = brightness - c;

    float r = 0.0f, g = 0.0f, b = 0.0f;

    if (hue >= 0 && hue < 60) {
        r = c; g = x; b = 0;
    }
    else if (hue >= 60 && hue < 120) {
        r = x; g = c; b = 0;
    }
    else if (hue >= 120 && hue < 180) {
        r = 0; g = c; b = x;
    }
    else if (hue >= 180 && hue < 240) {
        r = 0; g = x; b = c;
    }
    else if (hue >= 240 && hue < 300) {
        r = x; g = 0; b = c;
    }
    else {
        r = c; g = 0; b = x;
    }

    return sf::Color(
        static_cast<sf::Uint8>((r + m) * 255),
        static_cast<sf::Uint8>((g + m) * 255),
        static_cast<sf::Uint8>((b + m) * 255)
    );
}

// FFT实现
void computeFFT(const std::vector<float>& input, std::vector<float>& spectrum) {
    int N = input.size();
    if (N == 0) return;

    std::vector<std::complex<float>> data(N);
    for (int i = 0; i < N; i++) {
        data[i] = std::complex<float>(input[i], 0.0f);
    }

    int j = 0;
    for (int i = 0; i < N; i++) {
        if (j > i) {
            std::swap(data[i], data[j]);
        }

        int m = N >> 1;
        while (m >= 1 && j >= m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }

    for (int s = 1; s <= (int)std::log2(N); s++) {
        int m = 1 << s;
        float theta = -2.0f * 3.14159265358979323846f / m;
        std::complex<float> wm(cos(theta), sin(theta));

        for (int k = 0; k < N; k += m) {
            std::complex<float> w(1.0f, 0.0f);
            for (int j = 0; j < m / 2; j++) {
                std::complex<float> t = w * data[k + j + m / 2];
                std::complex<float> u = data[k + j];
                data[k + j] = u + t;
                data[k + j + m / 2] = u - t;
                w *= wm;
            }
        }
    }

    spectrum.resize(N / 2);
    for (int i = 0; i < N / 2; i++) {
        spectrum[i] = std::abs(data[i]) / (N / 2);
    }
}

// 将FFT结果映射到可视化频段
std::vector<float> mapSpectrumToBands(const std::vector<float>& spectrum, int bands) {
    std::vector<float> bandEnergies(bands, 0.0f);

    int specSize = spectrum.size();
    if (specSize == 0) return bandEnergies;

    float logMinFreq = std::log10(20.0f);
    float logMaxFreq = std::log10(20000.0f);

    for (int band = 0; band < bands; band++) {
        float logFreqStart = logMinFreq + (logMaxFreq - logMinFreq) * band / bands;
        float logFreqEnd = logMinFreq + (logMaxFreq - logMinFreq) * (band + 1) / bands;

        int idxStart = std::pow(10.0f, logFreqStart) * specSize / 20000.0f;
        int idxEnd = std::pow(10.0f, logFreqEnd) * specSize / 20000.0f;

        idxStart = std::clamp(idxStart, 0, specSize - 1);
        idxEnd = std::clamp(idxEnd, 0, specSize - 1);

        float sum = 0.0f;
        int count = 0;
        for (int i = idxStart; i <= idxEnd; i++) {
            sum += spectrum[i];
            count++;
        }

        bandEnergies[band] = (count > 0) ? sum / count : 0.0f;
    }

    return bandEnergies;
}

// ========== 修复的频率条类 ==========
// 使用原来的类名FrequencyBars，但完全重新实现
class FrequencyBars {
private:
    std::vector<sf::RectangleShape> bars;
    std::vector<SmoothValue<float>> barHeights;
    int barCount;
    float barWidth;
    float maxHeight;

public:
    FrequencyBars(int count, float maxH = 400.0f)
        : barCount(count), maxHeight(maxH) {

        // 计算每个条的宽度
        barWidth = static_cast<float>(WINDOW_WIDTH) / count;

        std::cout << "Creating " << count << " frequency bars" << std::endl;
        std::cout << "Bar width: " << barWidth << " pixels" << std::endl;

        // 创建所有条
        for (int i = 0; i < count; i++) {
            // 创建矩形条 - 宽度增加0.5像素确保没有间隙
            sf::RectangleShape bar(sf::Vector2f(barWidth + 0.5f, 10));

            // 计算x位置
            float xPos = static_cast<float>(i) * barWidth;
            bar.setPosition(xPos, WINDOW_HEIGHT);

            // 设置初始颜色
            bar.setFillColor(sf::Color::White);

            // 添加细边框以便看清边界
            bar.setOutlineThickness(0.5f);
            bar.setOutlineColor(sf::Color(0, 0, 0, 50));

            bars.push_back(bar);

            // 创建平滑器
            barHeights.push_back(SmoothValue<float>(10.0f, 20.0f));
        }

        // 输出调试信息
        std::cout << "First bar position: " << bars[0].getPosition().x << std::endl;
        std::cout << "Last bar position: " << bars[count - 1].getPosition().x << std::endl;
        std::cout << "Last bar right edge: " << (bars[count - 1].getPosition().x + bars[count - 1].getSize().x) << std::endl;
    }

    void update(const std::vector<float>& bandEnergies, float dt, int frameCount) {
        // 静态变量用于阻尼效果
        static std::vector<float> prevEnergies(barCount, 0.0f);

        for (int i = 0; i < barCount; i++) {
            // 获取能量
            float energy = (i < (int)bandEnergies.size()) ? bandEnergies[i] : 0.0f;

            // 阻尼处理
            float smoothedEnergy = prevEnergies[i] * 0.8f + energy * 0.2f;
            prevEnergies[i] = smoothedEnergy;

            // 应用对数压缩
            float logEnergy = std::log10(1.0f + smoothedEnergy * 9.0f);

            // 计算目标高度
            float targetHeight = logEnergy * maxHeight * 1.5f;
            targetHeight = std::min(targetHeight, maxHeight);
            targetHeight = std::max(targetHeight, 2.0f); // 最小高度2像素

            // 平滑高度变化
            barHeights[i].setTarget(targetHeight);
            barHeights[i].update(dt);

            float currentHeight = barHeights[i].getCurrent();

            // 获取条的当前位置和宽度
            float currentX = bars[i].getPosition().x;
            float currentWidth = bars[i].getSize().x;

            // 更新条的高度和位置
            bars[i].setSize(sf::Vector2f(currentWidth, currentHeight));
            bars[i].setPosition(currentX, WINDOW_HEIGHT - currentHeight);

            // 更新颜色
            float hue = fmod(i * 1.5f + frameCount * 0.5f, 360.0f);
            sf::Color color = hsbToRgb(hue, 80.0f, 100.0f);
            bars[i].setFillColor(color);
        }
    }

    void draw(sf::RenderTarget& target) {
        for (auto& bar : bars) {
            target.draw(bar);
        }
    }
};

int main() {
    std::cout << "=== Music Visualizer with Real FFT ===" << std::endl;

    // 1. 创建窗口
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT),
        "Music Visualizer - Fixed Width Bars");
    window.setFramerateLimit(60);

    // 2. 加载音频
    sf::SoundBuffer soundBuffer;
    std::string musicPath = "C:\\Users\\zhaok\\Desktop\\dvorak_new_world.mp3";

    if (!soundBuffer.loadFromFile(musicPath)) {
        std::cerr << "Error: Cannot load audio file!" << std::endl;
        std::cout << "Please make sure the audio file exists: " << musicPath << std::endl;
        return -1;
    }

    std::cout << "Audio loaded successfully!" << std::endl;
    std::cout << "Sample rate: " << soundBuffer.getSampleRate() << " Hz" << std::endl;
    std::cout << "Channels: " << soundBuffer.getChannelCount() << std::endl;

    // 3. 创建音频播放器
    sf::Sound sound;
    sound.setBuffer(soundBuffer);

    // 4. 获取音频数据
    const sf::Int16* allSamples = soundBuffer.getSamples();
    size_t totalSamples = soundBuffer.getSampleCount();
    unsigned int sampleRate = soundBuffer.getSampleRate();
    unsigned int channels = soundBuffer.getChannelCount();

    // 5. 创建频率条 - 使用原来的类名
    FrequencyBars frequencyBars(VISUAL_BANDS, 600.0f);

    // 6. 时间管理
    sf::Clock frameClock;
    bool isPlaying = false;
    int frameCount = 0;

    // 7. 创建拖尾效果纹理
    sf::RenderTexture trailTexture;
    if (!trailTexture.create(WINDOW_WIDTH, WINDOW_HEIGHT)) {
        std::cerr << "Error: Cannot create render texture!" << std::endl;
        return -1;
    }
    trailTexture.clear(sf::Color::Black);

    // 8. 中央圆环
    sf::CircleShape centerCircle(0.0f);
    centerCircle.setOrigin(centerCircle.getRadius(), centerCircle.getRadius());
    centerCircle.setPosition(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f);
    centerCircle.setFillColor(sf::Color::Transparent);
    centerCircle.setOutlineThickness(3.0f);
    centerCircle.setOutlineColor(sf::Color::White);

    SmoothValue<float> circleSize(0.0f, 10.0f);

    // 9. FFT缓冲区
    std::vector<float> audioBuffer(FFT_SIZE, 0.0f);
    std::vector<float> spectrum;
    std::vector<float> bandEnergies;

    // 10. 平滑音量
    SmoothValue<float> smoothedVolume(0.0f, 5.0f);

    std::cout << "\n=== Ready ===" << std::endl;
    std::cout << "FFT Size: " << FFT_SIZE << std::endl;
    std::cout << "Visual Bands: " << VISUAL_BANDS << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  SPACE - Play/Pause" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "  R - Restart" << std::endl;

    // 主循环
    while (window.isOpen()) {
        float dt = frameClock.restart().asSeconds();
        frameCount++;

        // 事件处理
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space) {
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

                if (event.key.code == sf::Keyboard::Escape)
                    window.close();

                if (event.key.code == sf::Keyboard::R) {
                    sound.stop();
                    sound.play();
                    isPlaying = true;
                    frameCount = 0;
                    std::cout << "Music restarted" << std::endl;
                }
            }
        }

        // 拖尾效果：半透明黑色矩形
        sf::RectangleShape trailRect(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
        trailRect.setFillColor(sf::Color(0, 0, 0, 30));
        trailTexture.draw(trailRect, sf::BlendAlpha);

        // 音频分析和FFT计算
        float currentVolume = 0.0f;

        if (sound.getStatus() == sf::Sound::Playing) {
            // 使用getPlayingOffset()获取准确时间
            sf::Time currentTime = sound.getPlayingOffset();

            // 计算样本索引
            size_t sampleIndex = static_cast<size_t>(currentTime.asSeconds() * sampleRate * channels);

            if (sampleIndex + FFT_SIZE < totalSamples) {
                // 准备FFT输入数据
                audioBuffer.clear();
                audioBuffer.resize(FFT_SIZE, 0.0f);

                // 提取当前帧的音频数据
                for (size_t i = 0; i < FFT_SIZE; i++) {
                    if (sampleIndex + i < totalSamples) {
                        if (channels == 2) {
                            float left = allSamples[sampleIndex + i * 2] / 32768.0f;
                            float right = allSamples[sampleIndex + i * 2 + 1] / 32768.0f;
                            audioBuffer[i] = (left + right) / 2.0f;
                        }
                        else {
                            audioBuffer[i] = allSamples[sampleIndex + i] / 32768.0f;
                        }
                    }
                }

                // 应用汉宁窗
                for (size_t i = 0; i < FFT_SIZE; i++) {
                    float window = 0.5f * (1.0f - cos(2.0f * 3.14159265358979323846f * i / (FFT_SIZE - 1)));
                    audioBuffer[i] *= window;
                }

                // 计算FFT
                computeFFT(audioBuffer, spectrum);

                // 计算当前音量
                currentVolume = calculateEnergy(&allSamples[sampleIndex], std::min((size_t)1024, totalSamples - sampleIndex));
                smoothedVolume.setTarget(currentVolume);

                // 映射到可视化频段
                bandEnergies = mapSpectrumToBands(spectrum, VISUAL_BANDS);
            }
        }
        else {
            // 音乐停止时
            currentVolume = 0.0f;
            smoothedVolume.setTarget(0.0f);
            bandEnergies = std::vector<float>(VISUAL_BANDS, 0.0f);
        }

        // 更新平滑音量
        smoothedVolume.update(dt);

        // 更新频率条
        frequencyBars.update(bandEnergies, dt, frameCount);

        // 更新中央圆环
        float targetCircleSize = smoothedVolume.getCurrent() * 300.0f;
        targetCircleSize = std::min(targetCircleSize, 400.0f);

        circleSize.setTarget(targetCircleSize);
        circleSize.update(dt);

        float currentCircleSize = circleSize.getCurrent();
        centerCircle.setRadius(currentCircleSize);
        centerCircle.setOrigin(currentCircleSize, currentCircleSize);

        // 绘制频率条到拖尾纹理
        frequencyBars.draw(trailTexture);

        // 绘制中央圆环到拖尾纹理
        if (currentCircleSize > 5.0f) {
            trailTexture.draw(centerCircle);
        }

        // 完成拖尾纹理绘制
        trailTexture.display();

        // 渲染到窗口
        window.clear(sf::Color::Black);

        // 绘制拖尾纹理
        sf::Sprite trailSprite(trailTexture.getTexture());
        window.draw(trailSprite);

        // 绘制UI信息
        sf::Font font;
        if (font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
            sf::Text infoText;
            infoText.setFont(font);
            infoText.setCharacterSize(20);
            infoText.setFillColor(sf::Color::White);
            infoText.setPosition(20, 20);

            std::string info = "Music Visualizer - Fixed Frequency Bars\n";
            info += isPlaying ? "Status: Playing" : "Status: Paused";
            info += "\nVolume: " + std::to_string(smoothedVolume.getCurrent());
            info += "\nFrame: " + std::to_string(frameCount);
            info += "\nTime: " + std::to_string(sound.getPlayingOffset().asSeconds()) + "s";
            info += "\nCircle size: " + std::to_string(currentCircleSize);

            infoText.setString(info);
            window.draw(infoText);
        }

        window.display();
    }

    std::cout << "\nProgram finished" << std::endl;
    return 0;
}
*/
// music_visualizer_simple.cpp - 简化版，保证能运行
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include "SmoothValue.h"

const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;

// 能量计算函数
float calculateVolume(const sf::Int16* samples, size_t count) {
    if (count == 0) return 0.0f;

    float sumSquares = 0.0f;
    for (size_t i = 0; i < count; i++) {
        float sample = samples[i] / 32768.0f;
        sumSquares += sample * sample;
    }

    return std::sqrt(sumSquares / count);
}

// 简化版：使用圆形粒子系统
class MusicParticle {
public:
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;
    float radius;
    float life;

    MusicParticle(float x, float y)
        : position(x, y), velocity(0, 0), color(sf::Color::White),
        radius(10.0f), life(1.0f) {
    }

    void update(float dt, float volume) {
        life -= dt * 0.5f;
        position += velocity * dt * 60.0f;
        velocity.y += 0.1f; // 重力

        // 根据音量调整大小
        radius = 5.0f + volume * 40.0f;
    }

    void draw(sf::RenderTarget& target) {
        if (life <= 0) return;

        sf::CircleShape circle(radius);
        circle.setPosition(position);
        circle.setFillColor(sf::Color(color.r, color.g, color.b,
            static_cast<sf::Uint8>(life * 255)));
        target.draw(circle);
    }
};

int main() {
    std::cout << "=== Simple Music Visualizer ===" << std::endl;

    // 创建窗口
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT),
        "Music Visualizer - Simple Particle System");
    window.setFramerateLimit(60);

    // 加载音频
    sf::SoundBuffer soundBuffer;
    std::string musicPath = "C:\\Users\\zhaok\\Desktop\\dvorak_new_world.mp3";

    if (!soundBuffer.loadFromFile(musicPath)) {
        std::cerr << "Error: Cannot load audio file!" << std::endl;
        musicPath = "test.mp3";
        if (!soundBuffer.loadFromFile(musicPath)) {
            std::cout << "Using simulated audio..." << std::endl;
        }
    }

    sf::Sound sound;
    if (soundBuffer.getSampleCount() > 0) {
        sound.setBuffer(soundBuffer);

        // 输出音频信息
        std::cout << "Audio loaded successfully!" << std::endl;
        std::cout << "Sample rate: " << soundBuffer.getSampleRate() << " Hz" << std::endl;
        std::cout << "Channels: " << soundBuffer.getChannelCount() << std::endl;
    }

    // 获取音频样本数据（用于真实音量计算）
    const sf::Int16* allSamples = nullptr;
    size_t totalSamples = 0;
    unsigned int sampleRate = 0;
    unsigned int channels = 0;

    if (soundBuffer.getSampleCount() > 0) {
        allSamples = soundBuffer.getSamples();
        totalSamples = soundBuffer.getSampleCount();
        sampleRate = soundBuffer.getSampleRate();
        channels = soundBuffer.getChannelCount();
    }

    // 粒子系统
    std::vector<MusicParticle> particles;

    // 中央能量圆环
    sf::CircleShape energyCircle(50.0f);
    energyCircle.setOrigin(50.0f, 50.0f);
    energyCircle.setPosition(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f);
    energyCircle.setFillColor(sf::Color::Transparent);
    energyCircle.setOutlineThickness(3.0f);
    energyCircle.setOutlineColor(sf::Color::White);

    SmoothValue<float> energyRadius(50.0f, 10.0f);
    SmoothValue<sf::Color> energyColor(sf::Color::White, 5.0f);

    // 平滑音量
    SmoothValue<float> smoothedVolume(0.0f, 8.0f);

    // 时间管理
    sf::Clock frameClock;
    sf::Clock particleClock;
    bool isPlaying = false;

    std::cout << "\n=== Ready ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  SPACE - Play/Pause" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "  R - Restart" << std::endl;
    std::cout << "  C - Clear particles" << std::endl;

    // 主循环
    while (window.isOpen()) {
        float dt = frameClock.restart().asSeconds();

        // 事件处理
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space) {
                    if (sound.getStatus() == sf::Sound::Playing) {
                        sound.pause();
                        isPlaying = false;
                    }
                    else {
                        sound.play();
                        isPlaying = true;
                    }
                }

                if (event.key.code == sf::Keyboard::Escape)
                    window.close();

                if (event.key.code == sf::Keyboard::R) {
                    sound.stop();
                    sound.play();
                    isPlaying = true;
                }

                if (event.key.code == sf::Keyboard::C) {
                    particles.clear();
                }
            }
        }

        // 清除屏幕（使用渐变色背景）
        window.clear(sf::Color(10, 10, 30));

        // ====== 关键修改：使用真实音量计算 ======
        float currentVolume = 0.0f;

        if (sound.getStatus() == sf::Sound::Playing && allSamples != nullptr) {
            // 获取当前播放时间
            sf::Time currentTime = sound.getPlayingOffset();

            // 计算当前时间对应的样本索引
            size_t sampleIndex = static_cast<size_t>(
                currentTime.asSeconds() * sampleRate * channels
                );

            // 定义分析窗口大小（1024个样本）
            const size_t ANALYSIS_SIZE = 1024;

            // 确保索引在有效范围内
            if (sampleIndex + ANALYSIS_SIZE < totalSamples) {
                // 计算当前音频块的能量（音量）
                currentVolume = calculateVolume(
                    &allSamples[sampleIndex],
                    ANALYSIS_SIZE
                );

                // 平滑音量值
                smoothedVolume.setTarget(currentVolume);
            }
        }
        else {
            // 音乐暂停时，音量逐渐衰减到0
            smoothedVolume.setTarget(0.0f);
        }

        // 更新平滑音量
        smoothedVolume.update(dt);

        // 使用平滑后的音量
        float vol = smoothedVolume.getCurrent();

        // 根据音量生成粒子
        if (isPlaying) {
            if (particleClock.getElapsedTime().asSeconds() > 0.05f) {
                for (int i = 0; i < 3; i++) {
                    float angle = static_cast<float>(rand() % 360) * 3.14159f / 180.0f;
                    float distance = 100.0f + vol * 200.0f;

                    MusicParticle particle(
                        WINDOW_WIDTH / 2.0f + cos(angle) * distance,
                        WINDOW_HEIGHT / 2.0f + sin(angle) * distance
                    );

                    // 设置粒子速度和颜色
                    particle.velocity = sf::Vector2f(
                        cos(angle) * 5.0f,
                        sin(angle) * 5.0f
                    );

                    // 根据角度设置颜色
                    float hue = fmod(angle * 180.0f / 3.14159f, 360.0f);
                    particle.color = sf::Color(
                        static_cast<sf::Uint8>(127 + 127 * sin(hue * 3.14159f / 180.0f)),
                        static_cast<sf::Uint8>(127 + 127 * sin((hue + 120.0f) * 3.14159f / 180.0f)),
                        static_cast<sf::Uint8>(127 + 127 * sin((hue + 240.0f) * 3.14159f / 180.0f))
                    );

                    particles.push_back(particle);
                }
                particleClock.restart();
            }
        }

        // 更新能量圆环（使用真实音量）
        float targetRadius = 50.0f + vol * 200.0f;
        energyRadius.setTarget(targetRadius);
        energyRadius.update(dt);

        sf::Color targetColor = sf::Color(
            100 + static_cast<sf::Uint8>(vol * 155),
            150 + static_cast<sf::Uint8>(vol * 105),
            220
        );
        energyColor.setTarget(targetColor);
        energyColor.update(dt);

        energyCircle.setRadius(energyRadius.getCurrent());
        energyCircle.setOrigin(energyCircle.getRadius(), energyCircle.getRadius());
        energyCircle.setOutlineColor(energyColor.getCurrent());

        // 更新和绘制粒子
        for (auto it = particles.begin(); it != particles.end();) {
            it->update(dt, vol);
            it->draw(window);

            if (it->life <= 0) {
                it = particles.erase(it);
            }
            else {
                ++it;
            }
        }

        // 绘制能量圆环
        window.draw(energyCircle);

        // 绘制UI信息
        sf::Font font;
        if (font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
            sf::Text infoText;
            infoText.setFont(font);
            infoText.setCharacterSize(20);
            infoText.setFillColor(sf::Color::White);
            infoText.setPosition(20, 20);

            std::string info = "Music Visualizer - Particle System\n";
            info += isPlaying ? "Status: Playing" : "Status: Paused";
            info += "\nVolume: " + std::to_string(vol);
            info += "\nParticles: " + std::to_string(particles.size());
            info += "\n\nControls:";
            info += "\n  SPACE - Play/Pause";
            info += "\n  ESC - Exit";
            info += "\n  R - Restart";
            info += "\n  C - Clear particles";

            infoText.setString(info);
            window.draw(infoText);
        }

        window.display();
    }

    std::cout << "\nProgram finished" << std::endl;
    return 0;
}