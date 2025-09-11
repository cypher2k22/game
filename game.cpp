#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>

const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 800;

// Paddle
const float PADDLE_WIDTH = 120.f;
const float PADDLE_HEIGHT = 20.f;
const float PADDLE_SPEED = 500.f;

// Ball
const float BALL_RADIUS = 10.f;
const int MAX_BALLS = 3;

// Brick
const int BRICK_ROWS = 5;
const int BRICK_COLUMNS = 10;
const float BRICK_WIDTH = 80.f;
const float BRICK_HEIGHT = 30.f;

class Ball {
public:
    sf::CircleShape shape;
    sf::Vector2f velocity;

    Ball(float x, float y, float vx, float vy) {
        shape.setRadius(BALL_RADIUS);
        shape.setFillColor(sf::Color::Red);
        shape.setPosition(x - BALL_RADIUS, y - BALL_RADIUS);
        velocity = {vx, vy};
    }

    void update(float dt) {
        shape.move(velocity * dt);
        sf::Vector2f pos = shape.getPosition();

        if (pos.x < 0) { velocity.x = std::abs(velocity.x); shape.setPosition(0, pos.y); }
        if (pos.x + 2 * BALL_RADIUS > WINDOW_WIDTH) { velocity.x = -std::abs(velocity.x); shape.setPosition(WINDOW_WIDTH - 2 * BALL_RADIUS, pos.y); }
        if (pos.y < 0) { velocity.y = std::abs(velocity.y); shape.setPosition(pos.x, 0); }
    }
};

class Paddle {
public:
    sf::RectangleShape shape;

    Paddle(float x, float y) {
        shape.setSize({PADDLE_WIDTH, PADDLE_HEIGHT});
        shape.setFillColor(sf::Color::Blue);
        shape.setPosition(x - PADDLE_WIDTH / 2.f, y);
    }

    void update(float dt) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && shape.getPosition().x > 0)
            shape.move(-PADDLE_SPEED * dt, 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && shape.getPosition().x + PADDLE_WIDTH < WINDOW_WIDTH)
            shape.move(PADDLE_SPEED * dt, 0);
    }
};

class Brick {
public:
    sf::RectangleShape shape;
    bool destroyed = false;

    Brick(float x, float y) {
        shape.setSize({BRICK_WIDTH - 2, BRICK_HEIGHT - 2});
        shape.setFillColor(sf::Color::Green);
        shape.setPosition(x, y);
    }
};

class Game {
private:
    sf::RenderWindow &window;
    sf::Font font;
    sf::Text scoreText, livesText, levelText, messageText;

    Paddle paddle;
    std::vector<Ball> balls;
    std::vector<Brick> bricks;

    int score = 0;
    int lives = 3;
    int level = 1;

    bool waitingForServe = true;
    bool gameOverFlag = false;
    sf::Clock speedClock;

    // Sounds
    sf::SoundBuffer bounceBuffer, brickBuffer;
    sf::Sound bounceSound, brickSound;

public:
    Game(sf::RenderWindow &win) : window(win), paddle(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT - 50.f) {
        srand(static_cast<unsigned>(time(nullptr)));

        if (!font.loadFromFile("ARIALI.TTF")) {
            printf("Font file missing!\n");
            exit(1);
        }

        // Load sounds
        if (!bounceBuffer.loadFromFile("line.wav") || !brickBuffer.loadFromFile("land.wav")) {
            printf("Sound files missing!\n");
        }
        bounceSound.setBuffer(bounceBuffer);
        brickSound.setBuffer(brickBuffer);

        // Setup texts
        scoreText.setFont(font); scoreText.setCharacterSize(24); scoreText.setFillColor(sf::Color::White); scoreText.setPosition(20, 10);
        livesText.setFont(font); livesText.setCharacterSize(24); livesText.setFillColor(sf::Color::White); livesText.setPosition(250, 10);
        levelText.setFont(font); levelText.setCharacterSize(24); levelText.setFillColor(sf::Color::White); levelText.setPosition(500, 10);
        messageText.setFont(font); messageText.setCharacterSize(36); messageText.setFillColor(sf::Color::Yellow); messageText.setPosition(WINDOW_WIDTH / 2 - 250, WINDOW_HEIGHT / 2 - 50);

        reset();
    }

    void reset() {
        score = 0; lives = 3; level = 1;
        waitingForServe = true; gameOverFlag = false;
        balls.clear(); setupBricks();
        paddle.shape.setPosition(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT - 50.f);
        speedClock.restart();
    }

    void serveBall() {
        balls.clear();
        balls.push_back(Ball(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f, 200.f, -200.f));
    }

    void setupBricks() {
        bricks.clear();
        float startX = (WINDOW_WIDTH - (BRICK_COLUMNS * BRICK_WIDTH)) / 2.f;
        for (int i = 0; i < BRICK_ROWS; i++)
            for (int j = 0; j < BRICK_COLUMNS; j++)
                bricks.push_back(Brick(startX + j * BRICK_WIDTH, 50 + i * BRICK_HEIGHT));
    }

    bool gameOver() { return gameOverFlag; }
    bool waiting() { return waitingForServe; }

    void update(float dt) {
        paddle.update(dt);

        // Increase speed gradually
        if (speedClock.getElapsedTime().asSeconds() >= 3.f) {
            for (auto &ball : balls) ball.velocity *= 1.05f;
            speedClock.restart();
        }

        for (size_t i = 0; i < balls.size(); ++i) {
            Ball &ball = balls[i];
            ball.update(dt);

            // Paddle collision
            if (ball.shape.getGlobalBounds().intersects(paddle.shape.getGlobalBounds())) {
                ball.velocity.y = -std::abs(ball.velocity.y);
                bounceSound.play();
                float paddleCenter = paddle.shape.getPosition().x + PADDLE_WIDTH / 2.f;
                float ballCenter = ball.shape.getPosition().x + BALL_RADIUS;
                float diff = ballCenter - paddleCenter;
                ball.velocity.x += diff * 5.f;
            }

            // Brick collision
            for (auto &brick : bricks) {
                if (!brick.destroyed && ball.shape.getGlobalBounds().intersects(brick.shape.getGlobalBounds())) {
                    brick.destroyed = true;
                    score += 10;
                    brickSound.play();
                    ball.velocity.y = -ball.velocity.y;

                    // Random duplicate
                    if (balls.size() < MAX_BALLS && rand() % 2 == 0) {
                        balls.push_back(Ball(ball.shape.getPosition().x + BALL_RADIUS, ball.shape.getPosition().y + BALL_RADIUS, -ball.velocity.x, ball.velocity.y));
                    }
                    break;
                }
            }

            // Ball fell out
            if (ball.shape.getPosition().y + 2 * BALL_RADIUS > WINDOW_HEIGHT) {
                balls.erase(balls.begin() + i);
                i--; // adjust index
            }
        }

        // If no balls left
        if (balls.empty()) {
            lives--;
            if (lives <= 0) gameOverFlag = true;
            else waitingForServe = true;
        }

        // Check level up
        bool allDestroyed = std::all_of(bricks.begin(), bricks.end(), [](Brick &b){ return b.destroyed; });
        if (allDestroyed) {
            level++;
            setupBricks();
            balls.clear();
            waitingForServe = true;
        }

        // Update texts
        scoreText.setString("Score: " + std::to_string(score));
        livesText.setString("Lives: " + std::to_string(lives));
        levelText.setString("Level: " + std::to_string(level));
    }

    void draw() {
        window.clear();
        window.draw(paddle.shape);
        for (auto &ball : balls) window.draw(ball.shape);
        for (auto &brick : bricks) if (!brick.destroyed) window.draw(brick.shape);
        window.draw(scoreText); window.draw(livesText); window.draw(levelText);

        if (gameOverFlag) {
            messageText.setString("GAME OVER!\nPress SPACE to restart");
            window.draw(messageText);
        } else if (waitingForServe) {
            messageText.setString("Press SPACE to serve!");
            window.draw(messageText);
        }

        window.display();
    }

    void handleSpace() {
        if (gameOverFlag) reset();
        else if (waitingForServe) { serveBall(); waitingForServe = false; }
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Brick Breaker");
    window.setFramerateLimit(60);

    Game game(window);
    sf::Clock clock;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space)
                game.handleSpace();
        }

        float dt = clock.restart().asSeconds();
        if (!game.gameOver() && !game.waiting()) game.update(dt);
        game.draw();
    }

    return 0;
}
