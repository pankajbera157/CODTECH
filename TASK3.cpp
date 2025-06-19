#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <ctime>

using namespace sf;

struct SnakeSegment {
    int x, y;
    SnakeSegment(int x, int y) : x(x), y(y) {}
};

class SnakeGame {
private:
    const int width = 800, height = 600, size = 20;
    RenderWindow window;
    std::vector<SnakeSegment> snake;
    int dirX = 1, dirY = 0;
    Vector2i food;
    Clock clock;
    float delay = 0.2f;
    SoundBuffer eatBuffer, gameOverBuffer;
    Sound eatSound, gameOverSound;
    bool gameOver = false;

public:
    SnakeGame() : window(VideoMode(width, height), "Snake Game") {
        window.setFramerateLimit(60);
        reset();
        loadSounds();
    }

    void reset() {
        snake.clear();
        snake.emplace_back(10, 10);
        dirX = 1; dirY = 0;
        food = Vector2i(rand() % (width / size), rand() % (height / size));
        delay = 0.2f;
        gameOver = false;
    }

    void loadSounds() {
        eatBuffer.loadFromFile("eat.wav");
        gameOverBuffer.loadFromFile("gameover.wav");
        eatSound.setBuffer(eatBuffer);
        gameOverSound.setBuffer(gameOverBuffer);
    }

    void run() {
        while (window.isOpen()) {
            Event event;
            while (window.pollEvent(event))
                if (event.type == Event::Closed) window.close();

            if (!gameOver) handleInput();
            if (clock.getElapsedTime().asSeconds() > delay) {
                clock.restart();
                update();
            }

            draw();
        }
    }

    void handleInput() {
        if (Keyboard::isKeyPressed(Keyboard::Up) && dirY == 0) { dirX = 0; dirY = -1; }
        if (Keyboard::isKeyPressed(Keyboard::Down) && dirY == 0) { dirX = 0; dirY = 1; }
        if (Keyboard::isKeyPressed(Keyboard::Left) && dirX == 0) { dirX = -1; dirY = 0; }
        if (Keyboard::isKeyPressed(Keyboard::Right) && dirX == 0) { dirX = 1; dirY = 0; }
    }

    void update() {
        SnakeSegment head = snake.front();
        head.x += dirX;
        head.y += dirY;

        if (head.x < 0 || head.y < 0 || head.x >= width / size || head.y >= height / size ||
            std::find_if(snake.begin(), snake.end(), [&](SnakeSegment s){ return s.x == head.x && s.y == head.y; }) != snake.end()) {
            gameOverSound.play();
            gameOver = true;
            return;
        }

        snake.insert(snake.begin(), head);

        if (head.x == food.x && head.y == food.y) {
            eatSound.play();
            food = Vector2i(rand() % (width / size), rand() % (height / size));
            delay *= 0.95f; // Increase difficulty
        } else {
            snake.pop_back();
        }
    }

    void draw() {
        window.clear(Color::Black);

        RectangleShape rect(Vector2f(size - 2, size - 2));
        rect.setFillColor(Color::Green);
        for (auto& s : snake) {
            rect.setPosition(s.x * size, s.y * size);
            window.draw(rect);
        }

        rect.setFillColor(Color::Red);
        rect.setPosition(food.x * size, food.y * size);
        window.draw(rect);

        window.display();
    }
};

int main() {
    srand(time(0));
    SnakeGame game;
    game.run();
    return 0;
}
