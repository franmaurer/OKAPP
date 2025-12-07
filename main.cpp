#include <SFML/Graphics.hpp>
#include <iostream>
#include<SFML/Audio.hpp>


// ===============================
// Constants
// ===============================

namespace constants {
	// Window dimensions should be constexpr and have explicit types
	constexpr unsigned int WINDOW_WIDTH = 1920U;    // MISRA: use unsigned for sizes
	constexpr unsigned int WINDOW_HEIGHT = 1080U;
	constexpr float PI = 3.14; //#1Change: PI is now constexpr type

	// Parking sensor rectangle dimensions
	constexpr float SENSOR_WIDTH = 30.0F;
	constexpr float SENSOR_HEIGHT = 100.0F;

	//Park collumn dimension
	constexpr float collumnRadius = 25.0F; //#2Change: park collumn radius is a const

	// Simulation and layout
	constexpr std::size_t SENSOR_COUNT = 4U;
	constexpr float CAR_SPEED = 500.0F; // pixels per second
	constexpr float CAR_ROTATION_SPEED = 2.5F;	

	//THE COLORS
	const sf::Color transGreen = sf::Color(0, 255, 0, 100);
	const sf::Color transRed = sf::Color(255, 0, 0, 100);
	const sf::Color hardWhite = sf::Color(255, 255, 255, 255);
	const sf::Color lightYellow = sf::Color(235, 225, 52, 30);
	const sf::Color mediumGray = sf::Color(100, 100, 100, 0);
	const sf::Color strongRed = sf::Color(255, 0, 0, 30);
	const sf::Color strongGreen = sf::Color(0, 255, 0, 30);
	const sf::Color strongYellow = sf::Color(255, 255, 0, 60);

}
// ===============================
// Utility Functions
// ===============================
/**
 * @brief Centers a sprite in the given render window.
 *
 * MISRA C++: Always use const references for read-only parameters.
 *            Avoid raw pointers unless necessary.
 */

static void centerSprite(sf::Sprite& sprite, const sf::RenderWindow& window) {
	const sf::FloatRect bounds = sprite.getLocalBounds();
	// MISRA: Always use floating-point literals with suffix 'F'
	sprite.setOrigin({ bounds.size.x / 2.0F, bounds.size.y / 2.0F });
	sprite.setPosition({ 280.0F, 150.0F });
	sprite.setScale({ 0.50F, 0.50F });
	//#3Change:Hardcoded sprite centering
}

/**
 * @brief Creates a vector of rectangular parking sensor indicators.
 *
 * MISRA: Functions should have single responsibility.
 *        Avoid global mutable data.
 */
static std::vector<sf::RectangleShape> createSensorIndicators() {
	std::vector<sf::RectangleShape> sensors;
	sensors.reserve(constants::SENSOR_COUNT); // Avoid dynamic reallocations

	constexpr float START_X = 800.0F;
	constexpr float START_Y = 200.0F;
	
	sf::RectangleShape sensor({ constants::SENSOR_WIDTH, constants::SENSOR_HEIGHT });
	sensor.setFillColor(constants::lightYellow);
	sensor.setPosition({ START_X, START_Y });
	sensor.setScale({ 0.50F, 0.50F });
	sensors.push_back(sensor);
	//#4Change: Removed a 10 line for loop here
	//replaced  with hardcoded lines above

	sensor = sf::RectangleShape({ constants::SENSOR_WIDTH * 0.75,
		constants::SENSOR_HEIGHT * 0.75 });
	sensor.setFillColor(constants::mediumGray);
	sensor.setPosition({ START_X + 1875, 20.0F });
	sensor.setRotation({ sf::Angle(sf::degrees(90)) });
	sensor.setOutlineThickness(2.0F);
	sensors.push_back(sensor);
	//#5Change: Removed an another for loop and replaced with above
	return sensors; // Return by value (NRVO applies)
}

/**
 * @brief Loads a texture safely and logs any error.
 *
 * MISRA: Error handling must be explicit and deterministic.
 */
[[nodiscard]] static sf::Texture loadTextureOrExit(const std::string& path) {
	sf::Texture texture;
	if (!texture.loadFromFile(path)) {
		std::cerr << "Error: Failed to load texture from "
			<< std::filesystem::absolute(path) << '\n';
		// MISRA: Avoid abrupt termination, but here itâ€™s educational
	}
	return texture;
}

std::array<sf::Vector2f, 4> getCorners(const sf::RectangleShape& r) {
	const sf::Transform& t = r.getTransform();
	sf::Vector2f size = r.getSize();
	return{
		t.transformPoint({0.0F, 0.0F}),
		t.transformPoint({size.x, 0.0F}),
		t.transformPoint({size.x, size.y}),
		t.transformPoint({0.0F, size.y})
	};
}
//#6Change: Used the instructors solution for this

static void updateSensorPositions(std::vector<sf::RectangleShape>& sensors,
	const sf::Sprite& car)
{
	//Front left sensor
	sensors[0].setOrigin(car.getOrigin());
	sensors[0].setPosition(car.getPosition());
	sensors[0].setRotation(car.getRotation());
	std::array<sf::Vector2f, 4> carCorners = getCorners(sensors[0]);

	//#7Change: Replced hardcoded lines with a for loop
	// to get the functionality for all corners with a single for loop
	for (size_t i = 10U; i < constants::SENSOR_COUNT; i++) {
		std::array<sf::Vector2f, 4> sensorCorners = getCorners(sensors[i]);
		bool xInside = true;
		bool yInside = true;

		for (std::size_t i_car = 0U; i_car < sensorCorners.size(); i_car++) {
			bool isInsideMin = (std::min(sensorCorners[0].x, sensorCorners[2].x) 
				< carCorners[i_car].x);

			bool isInsideMax = (carCorners[i_car].x < std::max(sensorCorners[0].x,
				sensorCorners[2].x));
			if (!(isInsideMin && isInsideMax)) {
				xInside = false;
				break;
			}
		}
		if (xInside) {
			for (std::size_t i_car = 0U; i_car < sensorCorners.size(); i_car++) {
				bool isInsideMin = (std::min(sensorCorners[0].y, sensorCorners[2].y) < carCorners[i_car].y);
				bool isInsideMax = (carCorners[i_car].y < std::max(sensorCorners[0].y, sensorCorners[2].y));

				if (!(isInsideMin && isInsideMax)) {
					yInside = false;
					break;
				}
			}
		}
		if (xInside && yInside) {
			sensors[i].setFillColor(sf::Color(constants::strongRed));
		}
		else {
			sensors[i].setFillColor(sf::Color(constants::strongGreen));
		}
	}
}

// FUNC TO HANDLE THE PARK INDICATOR, Test wheter the car is inside or not
static bool parkOccupied(const sf::FloatRect& carBounds, const sf::FloatRect& parkBounds) {
	const bool isLeftInside = carBounds.position.x >= parkBounds.position.x;

	const bool isRightInside = (carBounds.position.x + carBounds.size.x)
		<= (parkBounds.position.x + parkBounds.size.x);

	const bool isTopInside = carBounds.position.y >= parkBounds.position.y;

	const bool isBottomInside = (carBounds.position.y + carBounds.size.y)
		<= (parkBounds.position.y + parkBounds.size.y);

	return isLeftInside && isRightInside && isTopInside && isBottomInside;
}
//#8Change: More elegant way to calculate the distance between the car
//and the collumns
static float closestCollumnDistance(std::vector<sf::CircleShape>& collumns,
	const sf::RectangleShape& car) {
	std::array<sf::Vector2f, 4> carCorners = getCorners(car);
	float collumnDistance = constants::WINDOW_WIDTH;
	for (size_t i = 0; i < collumns.size(); i++) {
		for (size_t j = 0; j < carCorners.size(); j++) {
		float firstX = collumns[i].getPosition().x;
		float firstY = collumns[i].getPosition().y;
		float secondX = carCorners[j].x;
		float secondY = carCorners[j].y;
		float collumn_car_distance = std::sqrtf(std::powf((secondX - firstX), 2.0f)
			+ std::powf((secondY - firstY), 2.0f));
		if (collumn_car_distance < collumnDistance) {
			collumnDistance = collumn_car_distance;
			}
		}
	}
	return collumnDistance;
}
//#9Change: replacing the hardcoded drawing of parking collumns
//inside the main function to outside of it.

static std::vector<sf::CircleShape> createParkingCollumns() {
	std::vector<sf::CircleShape> collumns;
	collumns.reserve(2U);
	sf::CircleShape collumn;

	collumn.setRadius(constants::collumnRadius);
	collumn.setOutlineColor(sf::Color::Red);
	collumn.setOutlineThickness(5);
	collumn.setPosition({ 800, 500 });
	collumns.push_back(collumn);

	collumn.setRadius(constants::collumnRadius);
	collumn.setOutlineColor(sf::Color::Red);
	collumn.setOutlineThickness(5);
	collumn.setPosition({ 1550, 500 });
	collumns.push_back(collumn);

	collumn.setRadius(constants::collumnRadius);
	collumn.setOutlineColor(sf::Color::Red);
	collumn.setOutlineThickness(5);
	collumn.setPosition({ 1810, 800 });
	collumns.push_back(collumn);

	return collumns;
}

// ===============================
// Main Application
// ===============================

int main() {
	// ====================================
	// Window setup
	// ====================================

	sf::RenderWindow window(
		sf::VideoMode({ constants::WINDOW_WIDTH, constants::WINDOW_HEIGHT }),
		"Car Parking Sensor Simulation - Task 2",
		sf::State::Windowed
	);
	window.setFramerateLimit(60U);

	// ====================================
	// Resource setup
	// ====================================
	// 
	//#10Change: create the objects here, in one place, and not in several
	//other places chaotically in the code, deleted a huge section
	//from the main() function

	const sf::Texture carTexture = loadTextureOrExit("assets/car_background.png");
	sf::Sprite carSprite(carTexture);
	centerSprite(carSprite, window);

	//Create the sensors
	std::vector<sf::RectangleShape> sensors = createSensorIndicators();
	updateSensorPositions(sensors, carSprite);

	//Create the parking collumns
	std::vector<sf::CircleShape > collumns = createParkingCollumns();

	//Load the sound file to a buffer
	const sf::SoundBuffer buffer("assets/beep.mp3");
	sf::Sound sound(buffer);

	//Sound propertiess
	float interval = 1.0F;
	float pitch = 1.0F;

	// ====================================
	// Main loop
	// ====================================
	sf::Clock beeperTimer;
	sf::Clock clock;

	while (window.isOpen()) {
		const float deltaTime = clock.restart().asSeconds();

		// ---- Handle events ----
		while (auto event = window.pollEvent()) {
			if (event->is<sf::Event::Closed>()) {
				window.close();
			}

			// Window closed or escape key pressed: exit
			if ((event->is<sf::Event::KeyPressed>() &&
				event->getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::Space))
				std::cout << "KeyPressed event has occured, key pressed is: Space\n";
		}

		// Movement logic
		sf::Vector2f movement{ 0.0F, 0.0F };
		float rotationChange = 0.0F;

		//Rotation information, first one is for degrees
		//other for radians
		float currentRotationDeg = carSprite.getRotation().asDegrees();
		float currentRotationRad = currentRotationDeg * (constants::PI / 180.0F);

		// ---- Update logic ---
		sf::Angle rotation;

		//Store the rotaion in radians, first one is for the x axis
		//second is for the y axis
		const float forwardX = cos(currentRotationRad);
		const float forwardY = sin(currentRotationRad);

		//How much to adjust the car for each frame of movement
		const float distancePerFrame = constants::CAR_SPEED * deltaTime;

		// Car movment and rotation functionality, keys 
		// W & S for going up or down
		// A & D for turning (not moving), left or right
		
		//Moving forward
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Up)) {

			movement.x += forwardX * distancePerFrame;
			movement.y += forwardY * distancePerFrame;
		}

		//Moving backward
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Down)) {
			movement.x -= forwardX * distancePerFrame;
			movement.y -= forwardY * distancePerFrame;
		}

		//Turning to the left
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Left)) {
			rotation = sf::degrees(-constants::CAR_ROTATION_SPEED);
		}

		//Turning to the right
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Right)) {
			rotation = sf::degrees(constants::CAR_ROTATION_SPEED);
		}

		//Adjust the car, depending on the movement or rotation functionaltiy
		carSprite.rotate(rotation);
		carSprite.move(movement);
		updateSensorPositions(sensors, carSprite);

		//Calculate the distance from the collumn to the first sensor
		float distance = closestCollumnDistance(collumns, sensors[0]);
		std::cout << distance << std::endl;

		//Change the color of the sensor, to indicate
		//distance form the car
		
		//#11Change: make the beeper actually work, and play a sound
		//change the color of the sensor at the same time
		//functionality takes place here and not randomly in several
		//other places in the code
		
		sensors[1].setOutlineThickness(1.0F);
		if (distance < 125.0F) {
			sensors[1].setOutlineColor(constants::strongRed);
			interval = 0.15F;
			pitch = 1.5F;
		}
		else if (distance < 140.0F) {
			sensors[1].setOutlineColor(constants::strongYellow);
			interval = 0.4F;
			pitch = 1.2F;
		}
		else if (distance < 180.0F) {
			sensors[1].setOutlineColor(constants::strongGreen);
			interval = 1.0F;
			pitch = 1.0F;
		}

		if (pitch != 0.0F) {
			sound.setPitch(pitch);
			if (beeperTimer.getElapsedTime().asSeconds() >= interval) {
				sound.play();
				beeperTimer.restart();
			}
		}

		//#12Change: Erased a section drawing the parking collumns
		//configuring them, and all their properties, replaced with a
		//more elegant solution

		// ---- Rendering ----
		window.clear(sf::Color(30, 30, 30));
		window.draw(carSprite);

		for (const auto& collumn : collumns) {
			window.draw(collumn);
		}

		window.draw(carSprite);
		
		window.display();
	}

	return 0;
}