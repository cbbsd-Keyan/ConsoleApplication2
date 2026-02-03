#include<SFML/Graphics.hpp>
#include<SFML/Audio.hpp>
#include<iostream>
#include<cmath>
#include "SmoothValue.h";

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
float calculateEnergy(const sf::Int16* samples, int count) {
	if (count == 0) return 0.0f;
	float sumSquares = 0.0f;
	for (int i = 0; i < count; i++) {
		float normalizedSample = samples[i] / 32768.0f;
		sumSquares += normalizedSample * normalizedSample;
	}
	return std::sqrt(sumSquares / count);
}

int main() {
	std::cout << "===音乐可视化程序启动===" << std::endl;
	//创建窗口
	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT),"Music Visualizer");
	window.setFramerateLimit(60);
	//加载音频文件
	sf::SoundBuffer soundBuffer;
	std::string musicPath = "C:\\Users\\zhaok\\Desktop\\dvorak_new_world.mp3";
	std::cout << "正在加载音频文件" << musicPath << std::endl;
	if (!soundBuffer.loadFromFile(musicPath)){
		std::cerr << "错误！无法加载音频文件" << std::endl;
		std::cout << "请检查文件路径是否正确，或尝试:" << std::endl;
		std::cout << "1. 使用绝对路径，如 C:/Users/你的名字/Music/song.mp3" << std::endl;
		std::cout << "2. 确保文件格式支持（MP3、WAV、OGG）" << std::endl;
		return 1;
	}
	std::cout << "音频加载成功！" << std::endl;
	std::cout << "采样率：" << soundBuffer.getSampleRate() << "Hz" << std::endl;
	std::cout << "声道数: " << soundBuffer.getChannelCount() << std::endl;
	std::cout << "时长: " << soundBuffer.getSampleCount() / soundBuffer.getSampleRate() << " 秒" << std::endl;
	//创建音频播放器
	sf::Sound sound;
	sound.setBuffer(soundBuffer);
	//创建可视化元素：一个圆
	sf::CircleShape energyCircle(50.0f);
	energyCircle.setOrigin(50.0f, 50.0f);
	energyCircle.setPosition(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f);
	energyCircle.setFillColor(sf::Color(100, 150, 220));
	energyCircle.setOutlineThickness(3.0f);
	energyCircle.setOutlineColor(sf::Color::White);
	//准备音频分析
	const sf::Int16* allSamples = soundBuffer.getSamples();
	const size_t totalSamples = soundBuffer.getSampleCount();
	const unsigned int sampleRate = soundBuffer.getSampleRate();
	const unsigned int channels = soundBuffer.getChannelCount();
	//创建平滑器
	SmoothValue<float> smoothedRadius(5.0f, 12.0f);
	SmoothValue<sf::Color>smoothedColor(sf::Color(100, 150, 220), 8.0f);
	//时间管理
	sf::Clock frameClock;
	sf::Clock audioClock;
	bool isPlaying = false;

	std::cout << "\n=== 准备就绪 ===" << std::endl;
	std::cout << "按空格键开始/暂停播放" << std::endl;
	std::cout << "按ESC键退出程序" << std::endl;
	std::cout << "按R键重新开始" << std::endl;
	//主循环
	while (window.isOpen()) {
		float dt = frameClock.restart().asSeconds();
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				window.close();
			}
			//键盘按键处理
			if (event.type == sf::Event::KeyPressed) {
				//空格播放/暂停
				if (event.key.code == sf::Keyboard::Space) {
					if (sound.getStatus() == sf::Sound::Playing) {
						sound.pause();
						isPlaying = false;
						std::cout << "音乐暂停" << std::endl;
					}
					else {
						sound.play();
						if (!isPlaying) {
							audioClock.restart();
							isPlaying = true;
						}
					}
				}
				//Esc退出
				else if (event.key.code == sf::Keyboard::Escape) {
					window.close();
				}
				//R重新开始
				else if (event.key.code == sf::Keyboard::R) {
					sound.stop();
					audioClock.restart();
					sound.play();
					isPlaying = true;
					std::cout << "重新开始播放" << std::endl;
				}
			}
		}
		//音频分析与可视化处理
		if (sound.getStatus() == sf::Sound::Playing) {
			float currentTime = audioClock.getElapsedTime().asSeconds();
			size_t sampleIndex = static_cast<size_t>(currentTime * sampleRate * channels);
			const size_t analysisSamples = 1024;
			if (sampleIndex + analysisSamples < totalSamples) {
				float energy = calculateEnergy(&allSamples[sampleIndex], analysisSamples);
				//将能量转化为圆的半径
				float baseRadius = 50.0f;
				float maxRadius = 200.0f;
				float energyRadius = energy * 400.0f;
				float targetRadius = baseRadius + energyRadius;
				if (targetRadius > maxRadius) targetRadius = maxRadius;
				energyCircle.setRadius(targetRadius);
				energyCircle.setOrigin(targetRadius, targetRadius);
				//根据能量调整颜色
				sf::Uint8 blueValue = static_cast<sf::Uint8>(150 + energy * 105);
				sf::Color targetColor(100, 150, blueValue);
				//设置平滑器目标值
				smoothedRadius.setTarget(targetRadius);
				smoothedColor.setTarget(targetColor);
			}
		}
		//更新所有平滑器
		smoothedRadius.update(dt);
		smoothedColor.update(dt);
		//获取平滑后的值并更新图形
		float currentRadius = smoothedRadius.getCurrent();
		sf::Color currentColor = smoothedColor.getCurrent();
		energyCircle.setRadius(currentRadius);
		energyCircle.setOrigin(currentRadius, currentRadius);
		energyCircle.setFillColor(currentColor);

		//渲染（绘制）
		window.clear(sf::Color(20, 20, 40));
		window.draw(energyCircle);
		//显示文字说明
		sf::Font font;
		if (font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
			sf::Text text;
			text.setFont(font);
			text.setCharacterSize(20);
			text.setFillColor(sf::Color::White);
			text.setPosition(10, 10);
			std::string status = isPlaying ? "Status: Playing" : "Status: Paused";
			status += "\nControls: SPACE=Play/Pause, R=Restart, ESC=Quit";
			text.setString(status);
			window.draw(text);
		}
		//显示最终画面
		window.display();
	}
	std::cout << "=== 程序结束 ===" << std::endl;
	return 0;
 }