#include "raylib.h"
#include "box2d/box2d.h"
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>

// Screen dimensions
const int screenWidth = 800;
const int screenHeight = 600;

// Number of balls (adjusted for stress test)
int numBalls = 1000;  // Example size, adjust as needed

// Ball data structure
struct Ball {
    b2Body* body;
    float radius;
    Color color;
};

// Function to create a ball in Box2D world with random velocity
Ball CreateBall(b2World* world, float radius, Vector2 position, Color color) {
    Ball ball;
    ball.radius = radius;
    ball.color = color;

    // Define the dynamic body
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position.Set(position.x / 100.0f, position.y / 100.0f);  // Convert pixels to meters
    ball.body = world->CreateBody(&bodyDef);

    // Define a circle shape for the ball
    b2CircleShape circleShape;
    circleShape.m_radius = radius / 100.0f;  // Convert pixels to meters

    // Define the fixture with properties (density, friction, restitution)
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &circleShape;
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 0.3f;
    fixtureDef.restitution = 0.7f;  // Bounciness

    ball.body->CreateFixture(&fixtureDef);

    // Assign a random velocity to the ball (between -5 and 5 m/s)
    float velocityX = static_cast<float>((std::rand() % 100) - 50) / 10.0f;
    float velocityY = static_cast<float>((std::rand() % 100) - 50) / 10.0f;
    ball.body->SetLinearVelocity(b2Vec2(velocityX, velocityY));

    return ball;
}

// Function to create a wall in Box2D world
void CreateWall(b2World* world, float x, float y, float width, float height) {
    b2BodyDef bodyDef;
    bodyDef.position.Set(x, y);  // Set wall position in meters
    b2Body* body = world->CreateBody(&bodyDef);

    b2PolygonShape boxShape;
    boxShape.SetAsBox(width / 2.0f, height / 2.0f);

    body->CreateFixture(&boxShape, 0.0f);
}

// Helper function to calculate moving average
float CalculateMovingAverage(float newValue, float currentAverage, int frameCount) {
    return ((currentAverage * (frameCount - 1)) + newValue) / frameCount;
}

int main(int argc, char** argv) {
    if (argc > 1) {
        numBalls = std::atoi(argv[1]);
    }

    InitWindow(screenWidth, screenHeight, "Raylib + Box2D Moving Average Benchmark");
    SetTargetFPS(60);

    b2Vec2 gravity(0.0f, 0.0f);
    b2World world(gravity);

    CreateWall(&world, screenWidth / 200.0f, 0.05f, screenWidth / 100.0f, 0.1f);
    CreateWall(&world, screenWidth / 200.0f, screenHeight / 100.0f - 0.05f, screenWidth / 100.0f, 0.1f);
    CreateWall(&world, 0.05f, screenHeight / 200.0f, 0.1f, screenHeight / 100.0f);
    CreateWall(&world, screenWidth / 100.0f - 0.05f, screenHeight / 200.0f, 0.1f, screenHeight / 100.0f);

    Ball* balls = new Ball[numBalls];
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    for (int i = 0; i < numBalls; i++) {
        float radius = 60.0f / (i + 1);
        Vector2 position = { static_cast<float>(std::rand() % screenWidth), static_cast<float>(std::rand() % screenHeight) };
        Color color = { (unsigned char)(std::rand() % 256),
                       (unsigned char)(std::rand() % 256),
                       (unsigned char)(std::rand() % 256),
                       255 };

        balls[i] = CreateBall(&world, radius, position, color);
    }

    // Variables for calculating moving averages
    float physicsMovingAverage = 0;
    float renderMovingAverage = 0;
    int frameCount = 1;

    while (!WindowShouldClose()) {
        // Start timing physics calculation
        auto physicsStart = std::chrono::high_resolution_clock::now();
        world.Step(1.0f / 60.0f, 6, 2);
        auto physicsEnd = std::chrono::high_resolution_clock::now();

        // Calculate physics time in milliseconds
        float physicsTime = std::chrono::duration<float, std::milli>(physicsEnd - physicsStart).count();
        physicsMovingAverage = CalculateMovingAverage(physicsTime, physicsMovingAverage, frameCount);

        // Start timing rendering
        auto renderStart = std::chrono::high_resolution_clock::now();
        BeginDrawing();
        ClearBackground(RAYWHITE);

        for (int i = 0; i < numBalls; i++) {
            b2Vec2 position = balls[i].body->GetPosition();
            DrawCircle(static_cast<int>(position.x * 100), static_cast<int>(position.y * 100), balls[i].radius, balls[i].color);
        }

        DrawText(TextFormat("Number of balls: %d", numBalls), 10, 10, 20, DARKGRAY);
        DrawText(TextFormat("Physics Time: %.2f ms", physicsTime), 10, 40, 20, DARKGRAY);
        DrawText(TextFormat("Render Time: %.2f ms", renderMovingAverage), 10, 70, 20, DARKGRAY);
        DrawText(TextFormat("Average Physics Time: %.2f ms", physicsMovingAverage), 10, 100, 20, DARKGRAY);
        DrawFPS(10, 130);

        EndDrawing();

        // Calculate render time after EndDrawing
        float renderTime = std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - renderStart).count();
        renderMovingAverage = CalculateMovingAverage(renderTime, renderMovingAverage, frameCount);
        frameCount++;
    }

    delete[] balls;
    CloseWindow();

    return 0;
}
