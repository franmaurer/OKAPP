/*
==============================================================================
Car Parking Sensor Simulation
==============================================================================
Now includes:
 - Car movement using keyboard
 - Four diagonal sensors around the car
 - Sensors change color on pillar collision
 - Delta-time–based smooth motion
 - Mathematical algorithms
 - Playing sound
 - MISRA guidelines
 - ESC key to close program
 - Car centered in window
==============================================================================
*/
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <cmath>

// ===============================
// Constants
// ===============================
namespace constants {
	constexpr unsigned int WINDOW_WIDTH = 1920U;
	constexpr unsigned int WINDOW_HEIGHT = 1080U;
	constexpr float PI = 3.14159265358979323846f;

	// Parking sensor rectangle dimensions
	constexpr float SENSOR_WIDTH = 950.0F;
	constexpr float SENSOR_HEIGHT = 500.0F;
	constexpr float PILLAR_RADIUS = 25.0F;

	// Simulation and layout
	constexpr std::size_t SENSOR_COUNT = 5U; // 1 for car bounds + 4 diagonal sensors
	constexpr float CAR_SPEED = 500.0F;
	constexpr float CAR_ROTATION = 2.5F;

	// Car and sensor scale down
	constexpr float SCALE_DOWN_FACTOR{ 0.35F };

	// Diagonal sensor properties
	constexpr float SENSOR_CIRCLE_RADIUS = 15.0F;
	constexpr float SENSOR_DETECTION_RANGE = 150.0F;
}

// ===============================
// Utility Functions
// ===============================

static void centerSprite(sf::Sprite& sprite, const sf::RenderWindow& window) {
	const sf::FloatRect bounds = sprite.getLocalBounds();
	sprite.setOrigin({ bounds.size.x / 2.0F, bounds.size.y / 2.0F });

	// Center the car in the window
	sprite.setPosition({
		constants::WINDOW_WIDTH / 2.0F,
		constants::WINDOW_HEIGHT / 2.0F
		});
	sprite.setScale({ constants::SCALE_DOWN_FACTOR, constants::SCALE_DOWN_FACTOR });
}

static std::vector<sf::RectangleShape> createSensorIndicators() {
	std::vector<sf::RectangleShape> sensors;
	sensors.reserve(4U);

	constexpr float START_X = 20.0F;
	constexpr float START_Y = 20.0F;

	sf::RectangleShape sensor({ constants::SENSOR_WIDTH, constants::SENSOR_HEIGHT });
	sensor.setFillColor(sf::Color(235, 225, 52, 0));
	sensor.setPosition({ START_X, START_Y });
	sensor.setScale({ constants::SCALE_DOWN_FACTOR, constants::SCALE_DOWN_FACTOR });
	sensors.push_back(sensor);

	return sensors;
}

// Create four diagonal sensor circles
static std::vector<sf::CircleShape> createDiagonalSensors() {
	std::vector<sf::CircleShape> sensors;
	sensors.reserve(4U);

	for (std::size_t i = 0U; i < 4U; ++i) {
		sf::CircleShape sensor(constants::SENSOR_CIRCLE_RADIUS);
		sensor.setOrigin({ constants::SENSOR_CIRCLE_RADIUS, constants::SENSOR_CIRCLE_RADIUS });
		sensor.setFillColor(sf::Color(0, 255, 0, 200)); // Green by default
		sensor.setOutlineColor(sf::Color::White);
		sensor.setOutlineThickness(2.0f);
		sensors.push_back(sensor);
	}

	return sensors;
}

[[nodiscard]] static sf::Texture loadTextureOrExit(const std::string& path) {
	sf::Texture texture;
	if (!texture.loadFromFile(path)) {
		std::cerr << "Error: Failed to load texture from "
			<< std::filesystem::absolute(path) << '\n';
	}
	return texture;
}

std::array<sf::Vector2f, 4> getCorners(const sf::RectangleShape& r) {
	const sf::Transform& t = r.getTransform();
	sf::Vector2f size = r.getSize();

	return {
		t.transformPoint({0.f,       0.f}),
		t.transformPoint({size.x,    0.f}),
		t.transformPoint({size.x, size.y}),
		t.transformPoint({0.f,    size.y})
	};
}

// Calculate distance between two points
static float calculateDistance(const sf::Vector2f& p1, const sf::Vector2f& p2) {
	float dx = p2.x - p1.x;
	float dy = p2.y - p1.y;
	return std::sqrtf(dx * dx + dy * dy);
}

// Update diagonal sensor positions around the car
static void updateDiagonalSensorPositions(
	std::vector<sf::CircleShape>& diagonalSensors,
	const sf::RectangleShape& carBounds,
	const std::vector<sf::CircleShape>& pillars
) {
	// Get car corners (these are our sensor positions)
	std::array<sf::Vector2f, 4> corners = getCorners(carBounds);

	// Position each diagonal sensor at a corner
	for (std::size_t i = 0U; i < 4U && i < diagonalSensors.size(); ++i) {
		diagonalSensors[i].setPosition(corners[i]);

		// Check distance to all pillars
		float minDistance = constants::SENSOR_DETECTION_RANGE + 100.0f;

		for (const auto& pillar : pillars) {
			sf::Vector2f pillarCenter = pillar.getPosition() +
				sf::Vector2f(constants::PILLAR_RADIUS, constants::PILLAR_RADIUS);
			float distance = calculateDistance(corners[i], pillarCenter);

			if (distance < minDistance) {
				minDistance = distance;
			}
		}

		// Change color based on distance
		if (minDistance < 125.0F) {
			diagonalSensors[i].setFillColor(sf::Color(255, 0, 0, 200)); // Red - very close
		}
		else if (minDistance < 140.0F) {
			diagonalSensors[i].setFillColor(sf::Color(255, 165, 0, 200)); // Orange - close
		}
		else if (minDistance < 180.0F) {
			diagonalSensors[i].setFillColor(sf::Color(255, 255, 0, 200)); // Yellow - medium
		}
		else {
			diagonalSensors[i].setFillColor(sf::Color(0, 255, 0, 200)); // Green - safe
		}
	}
}

static void updateSensorPositions(std::vector<sf::RectangleShape>& sensors, const sf::Sprite& car) {
	sensors[0].setOrigin(car.getOrigin());
	sensors[0].setPosition(car.getPosition());
	sensors[0].setRotation(car.getRotation());
}

static float calcClosestPillarDistance(std::vector<sf::CircleShape>& pillars, const sf::RectangleShape& car) {
	std::array<sf::Vector2f, 4> carCorners = getCorners(car);
	float pillarDistance = constants::WINDOW_WIDTH;

	for (size_t i = 0; i < pillars.size(); i++) {
		sf::Vector2f pillarCenter = pillars[i].getPosition() +
			sf::Vector2f(constants::PILLAR_RADIUS, constants::PILLAR_RADIUS);

		for (size_t j = 0; j < carCorners.size(); j++) {
			float intr_distance = calculateDistance(carCorners[j], pillarCenter);

			if (intr_distance < pillarDistance) {
				pillarDistance = intr_distance;
			}
		}
	}

	return pillarDistance;
}

// The three hard code, parking pillars, they too can be removed with a right click
static std::vector<sf::CircleShape> createParkingPillars() {
	std::vector<sf::CircleShape> pillars;
	pillars.reserve(3u);
	sf::CircleShape pillar;

	pillar.setRadius(constants::PILLAR_RADIUS);
	pillar.setFillColor(sf::Color(100, 100, 100));
	pillar.setOutlineColor(sf::Color::Red);
	pillar.setOutlineThickness(5);
	//pillar.setPosition({ 800, 500 });
	//pillars.push_back(pillar); Old pillar, car spawns on top it and starts to beep immediately, its annoying and correction was needed

	pillar.setPosition({ 1550, 800 });
	pillars.push_back(pillar);

	pillar.setPosition({ 1810, 800 });
	pillars.push_back(pillar);

	return pillars;
}

// ===============================
// Main Application
// ===============================

int main() {
	sf::RenderWindow window(
		sf::VideoMode({ constants::WINDOW_WIDTH, constants::WINDOW_HEIGHT }),
		"Car Parking Sensor Simulation - Four Diagonal Sensors",
		sf::State::Windowed
	);
	window.setFramerateLimit(60U);

	const sf::Texture carTexture = loadTextureOrExit("assets/car_background.png");
	sf::Sprite carSprite(carTexture);
	centerSprite(carSprite, window);

	std::vector<sf::RectangleShape> sensors = createSensorIndicators();
	std::vector<sf::CircleShape> diagonalSensors = createDiagonalSensors();
	updateSensorPositions(sensors, carSprite);

	std::vector<sf::CircleShape> pillars = createParkingPillars();

	const sf::SoundBuffer buffer("assets/beep.mp3");
	sf::Sound sound(buffer);
	float interval = 1.0f;
	float pitch = 1.0f;

	sf::Clock beepTimer;
	sf::Clock clock;

	while (window.isOpen()) {
		const float deltaTime = clock.restart().asSeconds();

		while (auto event = window.pollEvent()) {
			if (event->is<sf::Event::Closed>()) {
				window.close();
			}

			// ESC key functionality to close the program instantly
			if (event->is<sf::Event::KeyPressed>()) {
				auto keyEvent = event->getIf<sf::Event::KeyPressed>();
				if (keyEvent->code == sf::Keyboard::Key::Escape) {
					std::cout << "ESC key pressed, closing program\n";
					window.close();
				}
			}

			// Left click to add pillar
			if (event->is<sf::Event::MouseButtonPressed>() &&
				event->getIf<sf::Event::MouseButtonPressed>()->button == sf::Mouse::Button::Left) {
				sf::Vector2i mousePos = sf::Mouse::getPosition(window);

				sf::CircleShape newPillar;
				newPillar.setRadius(constants::PILLAR_RADIUS);
				newPillar.setFillColor(sf::Color(100, 100, 100));
				newPillar.setOutlineColor(sf::Color::Red);
				newPillar.setOutlineThickness(5);
				newPillar.setPosition({ static_cast<float>(mousePos.x), static_cast<float>(mousePos.y) });

				pillars.push_back(newPillar);
				std::cout << "Pillar added at position: (" << mousePos.x << ", " << mousePos.y << ")\n";
			}

			// Right click to remove pillar
			if (event->is<sf::Event::MouseButtonPressed>() &&
				event->getIf<sf::Event::MouseButtonPressed>()->button == sf::Mouse::Button::Right) {
				sf::Vector2i mousePos = sf::Mouse::getPosition(window);
				sf::Vector2f mousePosF(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));

				// Check if mouse is hovering over any pillar
				for (auto it = pillars.begin(); it != pillars.end(); ++it) {
					sf::Vector2f pillarCenter = it->getPosition() +
						sf::Vector2f(constants::PILLAR_RADIUS, constants::PILLAR_RADIUS);
					float distance = calculateDistance(mousePosF, pillarCenter);

					// If mouse is within pillar radius, remove it
					if (distance <= constants::PILLAR_RADIUS) {
						std::cout << "Pillar removed at position: ("
							<< it->getPosition().x << ", " << it->getPosition().y << ")\n";
						pillars.erase(it);
						break; // Only remove one pillar per click
					}
				}
			}
		}

		// Movement logic
		sf::Vector2f movement{ 0.0F, 0.0F };
		sf::Angle angle = sf::degrees(0);

		float rotDeg = carSprite.getRotation().asDegrees();
		float rotRad = rotDeg * (constants::PI / 180.f);

		sf::Vector2f forward{
			std::cos(rotRad),
			std::sin(rotRad)
		};

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Up)) {
			movement += forward * (constants::CAR_SPEED * deltaTime);
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Down)) {
			movement -= forward * (constants::CAR_SPEED * deltaTime);
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Left)) {
			angle = sf::degrees(-constants::CAR_ROTATION);
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Right)) {
			angle = sf::degrees(constants::CAR_ROTATION);
		}

		// Car logic, movement, rotation and adjust the position of the sensors
		carSprite.move(movement);
		carSprite.rotate(angle);
		updateSensorPositions(sensors, carSprite);
		updateDiagonalSensorPositions(diagonalSensors, sensors[0], pillars);

		// Distance measurement
		float distance = calcClosestPillarDistance(pillars, sensors[0]);

		// Sound feedback based on distance
		if (distance < 125.0f) {
			interval = 0.15f;
			pitch = 1.5f;
		}
		else if (distance < 140.f) {
			interval = 0.4f;
			pitch = 1.2f;
		}
		else if (distance < 180.f) {
			interval = 1.0f;
			pitch = 1.0f;
		}
		else {
			pitch = 0.0f;
		}

		if (pitch != 0.0f) {
			sound.setPitch(pitch);
			if (beepTimer.getElapsedTime().asSeconds() >= interval) {
				sound.play();
				beepTimer.restart();
			}
		}

		// Rendering
		window.clear(sf::Color(30, 30, 30));
		for (const auto& pillar : pillars) {
			window.draw(pillar);
		}
		window.draw(carSprite);

		// Draw diagonal sensors on top
		for (const auto& sensor : diagonalSensors) {
			window.draw(sensor);
		}
		window.display();
	}
	return 0;
}