#include "raylib.h"
#include "box2d/box2d.h"
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <math.h>

// Screen dimensions
const int screenWidth = 800;
const int screenHeight = 600;

// Number of balls (adjusted for stress test)
int numBalls = 30000;  // Example size, adjust as needed

// Ball data structure
struct Ball {
    b2Body* body;
    float radius;
    Color color;
};

// Function to create a ball in Box2D world with fixed radius and no rotation
Ball CreateBall(b2World* world, float radius, Vector2 position, Color color) {
    Ball ball;
    ball.radius = radius;
    ball.color = color;

    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position.Set(position.x / 100.0f, position.y / 100.0f);  // Convert pixels to meters
    bodyDef.fixedRotation = true;  // Prevent rotation
    ball.body = world->CreateBody(&bodyDef);

    b2CircleShape circleShape;
    circleShape.m_radius = radius / 100.0f;  // Convert pixels to meters

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &circleShape;
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 0.0f;
    fixtureDef.restitution = 0.0f;

    ball.body->CreateFixture(&fixtureDef);

    float velocityX = static_cast<float>((std::rand() % 100) - 50) / 50.0f;
    float velocityY = static_cast<float>((std::rand() % 100) - 50) / 50.0f;
    ball.body->SetLinearVelocity(b2Vec2(velocityX, velocityY));

    return ball;
}

// Function to create a wall in Box2D world
void CreateWall(b2World* world, float x, float y, float width, float height) {
    b2BodyDef bodyDef;
    bodyDef.position.Set(x, y);
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

    // Calculate box dimensions based on number of balls
    float areaPerBall = 1.0f;  // Arbitrary value to maintain density
    float boxArea = numBalls * areaPerBall;
    float aspectRatio = static_cast<float>(screenWidth) / screenHeight;
    float boxWidth = sqrt(boxArea * aspectRatio);
    float boxHeight = boxWidth / aspectRatio;

    // Create walls based on scaled box dimensions
    CreateWall(&world, boxWidth / 2.0f, 0.05f, boxWidth, 0.1f);                  // Top wall
    CreateWall(&world, boxWidth / 2.0f, boxHeight - 0.05f, boxWidth, 0.1f);      // Bottom wall
    CreateWall(&world, 0.05f, boxHeight / 2.0f, 0.1f, boxHeight);                // Left wall
    CreateWall(&world, boxWidth - 0.05f, boxHeight / 2.0f, 0.1f, boxHeight);     // Right wall

    Ball* balls = new Ball[numBalls];
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // Set fixed radius for all balls
    float fixedRadius = 10.0f;
    for (int i = 0; i < numBalls; i++) {
        Vector2 position = { static_cast<float>(std::rand() % static_cast<int>(boxWidth * 100)),
                             static_cast<float>(std::rand() % static_cast<int>(boxHeight * 100)) };
        Color color = { (unsigned char)(std::rand() % 256),
                       (unsigned char)(std::rand() % 256),
                       (unsigned char)(std::rand() % 256),
                       255 };

        balls[i] = CreateBall(&world, fixedRadius, position, color);
    }

    // Initialize camera and set zoom to fit the box area
    Camera2D camera = { 0 };
    camera.target = { boxWidth * 50.0f, boxHeight * 50.0f };  // Set target to center of the box
    camera.offset = { screenWidth / 2.0f, screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f/std::max(screenWidth / boxWidth, screenHeight / boxHeight);  // Adjust zoom to fit box

    float physicsMovingAverage = 0;
    float renderMovingAverage = 0;
    int frameCount = 1;

    while (!WindowShouldClose()) {
        // Camera movement
        if (IsKeyDown(KEY_RIGHT)) camera.target.x += 10;
        if (IsKeyDown(KEY_LEFT)) camera.target.x -= 10;
        if (IsKeyDown(KEY_DOWN)) camera.target.y += 10;
        if (IsKeyDown(KEY_UP)) camera.target.y -= 10;

        // Camera zoom control
        if (IsKeyDown(KEY_Z)) camera.zoom += 0.02f * camera.zoom;
        if (IsKeyDown(KEY_X)) camera.zoom -= 0.02f * camera.zoom;
        //if (camera.zoom < 0.1f) camera.zoom = 0.1f; // Prevent zooming out too far

        auto physicsStart = std::chrono::high_resolution_clock::now();
        world.Step(1.0f / 60.0f, 1, 1);
        auto physicsEnd = std::chrono::high_resolution_clock::now();

        float physicsTime = std::chrono::duration<float, std::milli>(physicsEnd - physicsStart).count();
        physicsMovingAverage = CalculateMovingAverage(physicsTime, physicsMovingAverage, frameCount);

        auto renderStart = std::chrono::high_resolution_clock::now();
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(camera);  // Begin camera view

        for (int i = 0; i < numBalls; i++) {
            b2Vec2 position = balls[i].body->GetPosition();
            DrawCircle(static_cast<int>(position.x * 100), static_cast<int>(position.y * 100), balls[i].radius, balls[i].color);
        }

        EndMode2D();  // End camera view

        DrawText(TextFormat("Number of balls: %d", numBalls), 10, 10, 20, DARKGRAY);
        DrawText(TextFormat("Physics Time: %.2f ms", physicsTime), 10, 40, 20, DARKGRAY);
        DrawText(TextFormat("Render Time: %.2f ms", renderMovingAverage), 10, 70, 20, DARKGRAY);
        DrawText(TextFormat("Average Physics Time: %.2f ms", physicsMovingAverage), 10, 100, 20, DARKGRAY);
        DrawText("Use arrow keys to move, Z/X to zoom in/out", 10, 160, 20, DARKGRAY);
        DrawFPS(10, 130);

        EndDrawing();

        float renderTime = std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - renderStart).count();
        renderMovingAverage = CalculateMovingAverage(renderTime, renderMovingAverage, frameCount);
        frameCount++;
    }

    delete[] balls;
    CloseWindow();

    return 0;
}
