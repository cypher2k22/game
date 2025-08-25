#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <memory>
#include <cstdlib>
#include <ctime>
#include <sstream>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

const int BRICK_ROWS = 5;
const int BRICK_COLS = 5;
const float BRICK_WIDTH = 70.f;
const float BRICK_HEIGHT = 50.f;
const sf::Color BRICK_COLOR = sf::Color(144, 238, 144); // Light green
const sf::Color BRICK_BORDER_COLOR = sf::Color::Black;

const float PADDLE_WIDTH = 120.f;
const float PADDLE_HEIGHT = 20.f;
const float PADDLE_SPEED = 500.f;

const float BALL_RADIUS = 10.f;
float BALL_SPEED = 250.f; // initial speed

class GameObject {
public:
    virtual void draw(sf::RenderWindow& window) = 0;
    virtual void update(float dt) = 0;
    virtual ~GameObject() {}
};

class Brick : public GameObject {
public:
    sf::RectangleShape shape;
    bool destroyed = false;
    bool hasHeart = false;

    Brick(float x, float y) {
        shape.setSize({BRICK_WIDTH, BRICK_HEIGHT});
        shape.setFillColor(BRICK_COLOR);
        shape.setOutlineThickness(2.f);
        shape.setOutlineColor(BRICK_BORDER_COLOR);
        shape.setPosition(x, y);
    }

    void draw(sf::RenderWindow& window) override {
        if (!destroyed) window.draw(shape);
    }

    void update(float dt) override {}
};

class Paddle : public GameObject {
public:
    sf::RectangleShape shape;
    Paddle() {
        shape.setSize({PADDLE_WIDTH, PADDLE_HEIGHT});
        shape.setFillColor(sf::Color::Red);
        shape.setPosition(WINDOW_WIDTH / 2 - PADDLE_WIDTH / 2, WINDOW_HEIGHT - 50);
    }

    void draw(sf::RenderWindow& window) override { window.draw(shape); }

    void update(float dt) override {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            shape.move(-PADDLE_SPEED * dt, 0);
            if (shape.getPosition().x < 0) shape.setPosition(0, shape.getPosition().y);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            shape.move(PADDLE_SPEED * dt, 0);
            if (shape.getPosition().x + PADDLE_WIDTH > WINDOW_WIDTH)
                shape.setPosition(WINDOW_WIDTH - PADDLE_WIDTH, shape.getPosition().y);
        }
    }
};

class Ball : public GameObject {
public:
    sf::CircleShape shape;
    sf::Vector2f velocity;
    bool launched = false;
    Paddle* paddlePtr = nullptr;

    Ball() {
        shape.setRadius(BALL_RADIUS);
        shape.setFillColor(sf::Color::Yellow);
        velocity = {BALL_SPEED, -BALL_SPEED};
    }

    void draw(sf::RenderWindow& window) override { window.draw(shape); }

    void update(float dt) override {
        if (!launched && paddlePtr) {
            shape.setPosition(paddlePtr->shape.getPosition().x + PADDLE_WIDTH / 2 - BALL_RADIUS,
                              paddlePtr->shape.getPosition().y - 2 * BALL_RADIUS);
        } else {
            shape.move(velocity * dt);
            if (shape.getPosition().x <= 0 || shape.getPosition().x + BALL_RADIUS * 2 >= WINDOW_WIDTH)
                velocity.x = -velocity.x;
            if (shape.getPosition().y <= 0)
                velocity.y = -velocity.y;
        }
    }

    void reset(float speed) {
        velocity = {speed, -speed};
        launched = false;
    }
};

class Game {
private:
    sf::RenderWindow window;
    Paddle paddle;
    Ball ball;
    std::vector<std::unique_ptr<Brick>> bricks;
    int score = 0;
    int level = 1;
    int lives = 3;
    sf::Font font;
    sf::Text scoreText;
    sf::Text livesText;
    sf::SoundBuffer landBuffer, lineBuffer;
    sf::Sound landSound, lineSound;
    bool gameOver = false;
    int heartIndex = -1;

public:
    Game() : window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Brick Breaker") {
        srand(time(0));
        ball.paddlePtr = &paddle; // link ball to paddle

        font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf");

        scoreText.setFont(font);
        scoreText.setCharacterSize(24);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setPosition(10, 10);

        livesText.setFont(font);
        livesText.setCharacterSize(24);
        livesText.setFillColor(sf::Color::White);
        livesText.setPosition(WINDOW_WIDTH - 120, 10);

        landBuffer.loadFromFile("land.wav");
        lineBuffer.loadFromFile("line.wav");
        landSound.setBuffer(landBuffer);
        lineSound.setBuffer(lineBuffer);

        createBricks();
        showCountdown();
    }

    void showCountdown() {
        sf::Text countdownText;
        countdownText.setFont(font);
        countdownText.setCharacterSize(120);
        countdownText.setFillColor(sf::Color::Yellow);
        countdownText.setOutlineColor(sf::Color::Red);
        countdownText.setOutlineThickness(5);

        for (int i = 3; i >= 1; --i) {
            countdownText.setString(std::to_string(i));
            countdownText.setPosition(WINDOW_WIDTH / 2 - countdownText.getLocalBounds().width / 2,
                                      WINDOW_HEIGHT / 2 - countdownText.getLocalBounds().height / 2);
            window.clear(sf::Color::Black);
            paddle.draw(window);
            for (auto& brick : bricks) brick->draw(window);
            window.draw(countdownText);
            window.display();
            sf::sleep(sf::seconds(1));
        }

        countdownText.setString("Go!");
        countdownText.setPosition(WINDOW_WIDTH / 2 - countdownText.getLocalBounds().width / 2,
                                  WINDOW_HEIGHT / 2 - countdownText.getLocalBounds().height / 2);
        window.clear(sf::Color::Black);
        paddle.draw(window);
        for (auto& brick : bricks) brick->draw(window);
        window.draw(countdownText);
        window.display();
        sf::sleep(sf::seconds(1));
    }

    void createBricks() {
        bricks.clear();
        float startX = (WINDOW_WIDTH - (BRICK_COLS * BRICK_WIDTH)) / 2.f;
        heartIndex = rand() % (BRICK_ROWS * BRICK_COLS);
        int idx = 0;
        for (int i = 0; i < BRICK_ROWS; ++i) {
            for (int j = 0; j < BRICK_COLS; ++j) {
                bricks.push_back(std::make_unique<Brick>(startX + j * BRICK_WIDTH, 50 + i * BRICK_HEIGHT));
                if (idx == heartIndex) bricks[idx]->hasHeart = true;
                idx++;
            }
        }
    }

    void run() {
        sf::Clock clock;
        while (window.isOpen()) {
            float dt = clock.restart().asSeconds();
            handleEvents();
            if (!gameOver) update(dt);
            render();
        }
    }

    void handleEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        if (!ball.launched && sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
            ball.launched = true;
    }

    void update(float dt) {
        paddle.update(dt);
        ball.update(dt);

        if (ball.launched && ball.shape.getGlobalBounds().intersects(paddle.shape.getGlobalBounds())) {
            ball.velocity.y = -ball.velocity.y;
            landSound.play();
        }

        for (auto& brick : bricks) {
            if (!brick->destroyed && ball.shape.getGlobalBounds().intersects(brick->shape.getGlobalBounds())) {
                brick->destroyed = true;
                ball.velocity.y = -ball.velocity.y;
                score += 10;
                lineSound.play();
                if (brick->hasHeart) lives++;
            }
        }

        if (ball.shape.getPosition().y > WINDOW_HEIGHT) {
            lives--;
            ball.reset(BALL_SPEED + (level - 1) * 50);
            if (lives <= 0) gameOver = true;
        }

        bool allDestroyed = true;
        for (auto& brick : bricks) if (!brick->destroyed) allDestroyed = false;
        if (allDestroyed) {
            level++;
            BALL_SPEED += 50;
            ball.reset(BALL_SPEED);
            ball.launched = false;
            createBricks();
            showCountdown();
        }

        std::ostringstream ss;
        ss << "Score: " << score;
        scoreText.setString(ss.str());

        std::ostringstream ls;
        ls << "Lives: " << lives;
        livesText.setString(ls.str());
    }

    void render() {
        window.clear(sf::Color::Black);
        paddle.draw(window);
        ball.draw(window);
        for (auto& brick : bricks) brick->draw(window);
        window.draw(scoreText);
        window.draw(livesText);

        if (gameOver) {
            sf::Text goText;
            goText.setFont(font);
            goText.setString("GAME OVER! Press R to Restart");
            goText.setCharacterSize(36);
            goText.setFillColor(sf::Color::Red);
            goText.setPosition(WINDOW_WIDTH / 2 - 200, WINDOW_HEIGHT / 2 - 20);
            window.draw(goText);

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
                score = 0;
                level = 1;
                lives = 3;
                BALL_SPEED = 250.f;
                ball.reset(BALL_SPEED);
                ball.launched = false;
                createBricks();
                gameOver = false;
                showCountdown();
            }
        }

        window.display();
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}
