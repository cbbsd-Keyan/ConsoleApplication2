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

// 优化后的 mapSpectrumToBands 函数
std::vector<float> mapSpectrumToBands(const std::vector<float>& spectrum, int bands) {
    std::vector<float> bandEnergies(bands, 0.0f);

    int specSize = spectrum.size();
    if (specSize == 0) return bandEnergies;

    // 使用更合理的频带划分
    int binsPerBand = specSize / bands;

    // 动态调整频带划分，确保低频部分有足够分辨率
    for (int band = 0; band < bands; band++) {
        int startBin = 0;
        int endBin = 0;

        // 改进：让低频和高频的bin分布更均匀
        if (band == 0) {
            // 第一个频带：0-2个bin
            startBin = 0;
            endBin = 1;
        }
        else if (band < bands / 4) {
            // 低频部分：逐步增加bin数量
            startBin = (band - 1) * 2;
            endBin = startBin + 2;
        }
        else if (band < bands / 2) {
            // 中低频部分
            startBin = (bands / 4 - 1) * 2 + (band - bands / 4) * 4;
            endBin = startBin + 4;
        }
        else {
            // 中高频和高频部分
            int baseBin = (bands / 4 - 1) * 2 + (bands / 4) * 4;
            startBin = baseBin + (band - bands / 2) * (specSize - baseBin) / (bands - bands / 2);
            endBin = (band == bands - 1) ? specSize - 1 : startBin + (specSize - baseBin) / (bands - bands / 2);
        }

        // 确保索引在范围内
        startBin = std::clamp(startBin, 0, specSize - 1);
        endBin = std::clamp(endBin, startBin, specSize - 1);

        // 使用平均值而不是最大值，使曲线更平滑
        float sum = 0.0f;
        int count = 0;
        for (int i = startBin; i <= endBin && i < specSize; i++) {
            sum += spectrum[i];
            count++;
        }
        float avgVal = (count > 0) ? sum / count : 0.0f;

        // 调整增益：减小低频增益，使整体更平衡
        float gain = 1.0f;
        if (band < bands / 8) {
            gain = 1.5f;  // 减小低频增益
        }
        else if (band < bands / 4) {
            gain = 1.3f;
        }
        else if (band < bands / 2) {
            gain = 1.1f;
        }

        bandEnergies[band] = avgVal * gain;
    }

    return bandEnergies;
}

// ========== 修改的频率条类 ==========
class FrequencyBars {
private:
    std::vector<sf::RectangleShape> bars;
    std::vector<SmoothValue<float>> barHeights;
    int barCount;
    float barWidth;
    float maxHeight;
    float spacing;  // 添加间距

public:
    // 修改 FrequencyBars 构造函数
    FrequencyBars(int count, float maxH = 400.0f)
        : barCount(count), maxHeight(maxH) {

        // 修改：统一计算宽度和间距
        float totalSpacing = (count - 1) * 1.0f;  // 1像素间距
        float availableWidth = WINDOW_WIDTH - totalSpacing;
        barWidth = availableWidth / count;  // 现在所有条宽度相同

        std::cout << "Creating " << count << " frequency bars" << std::endl;
        std::cout << "Bar width: " << barWidth << " pixels" << std::endl;
        std::cout << "Total spacing: " << totalSpacing << " pixels" << std::endl;

        // 创建所有条
        for (int i = 0; i < count; i++) {
            // 所有条使用相同宽度
            sf::RectangleShape bar(sf::Vector2f(barWidth, 10));

            // 计算x位置，考虑间距
            float xPos = i * (barWidth + 1.0f);  // 1像素间距
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
        std::cout << "Expected total width: " << WINDOW_WIDTH << std::endl;
        std::cout << "Actual total width: "
            << (bars[count - 1].getPosition().x + bars[count - 1].getSize().x)
            << std::endl;
    }

    void update(const std::vector<float>& bandEnergies, float dt, int frameCount) {
        // 静态变量用于阻尼效果
        static std::vector<float> prevEnergies(barCount, 0.0f);

        for (int i = 0; i < barCount; i++) {
            // 获取能量
            float energy = (i < (int)bandEnergies.size()) ? bandEnergies[i] : 0.0f;

            // 关键修改：增强阻尼效果，使变化更平滑
            float smoothedEnergy = prevEnergies[i] * 0.95f + energy * 0.05f;
            prevEnergies[i] = smoothedEnergy;

            // 应用对数压缩 - 增强动态范围
            float logEnergy = std::log10(1.0f + smoothedEnergy * 99.0f);

            // 计算目标高度
            float targetHeight = logEnergy * maxHeight * 1.3f;
            targetHeight = std::min(targetHeight, maxHeight);
            targetHeight = std::max(targetHeight, 7.0f);

            // 添加额外的平滑：限制最大变化速率
            float currentHeight = barHeights[i].getCurrent();
            float maxChange = maxHeight * 0.1f * dt * 60.0f;  // 每秒最大变化为高度的10%
            float limitedTargetHeight = currentHeight + std::clamp(targetHeight - currentHeight,
                -maxChange, maxChange);

            // 平滑高度变化
            barHeights[i].setTarget(targetHeight);
            barHeights[i].update(dt);

            currentHeight = barHeights[i].getCurrent();

            // 获取条的当前位置和宽度
            float currentX = bars[i].getPosition().x;
            float currentWidth = bars[i].getSize().x;

            // 更新条的高度和位置
            bars[i].setSize(sf::Vector2f(currentWidth, currentHeight));
            bars[i].setPosition(currentX, WINDOW_HEIGHT - currentHeight);

            // 更新颜色 - 保持原来的彩虹色效果
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

    // 5. 创建频率条
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

    // 8. 中央圆环 - 保持白色
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
            // 音乐停止时，让能量逐渐衰减
            currentVolume = 0.0f;
            smoothedVolume.setTarget(0.0f);
            // 让bandEnergies逐渐衰减而不是直接归零
            for (auto& energy : bandEnergies) {
                energy *= 0.9f;
            }
        }

        // 更新平滑音量
        smoothedVolume.update(dt);

        // 更新频率条
        frequencyBars.update(bandEnergies, dt, frameCount);

        // 更新中央圆环
        float targetCircleSize = smoothedVolume.getCurrent() * 2200.0f;
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