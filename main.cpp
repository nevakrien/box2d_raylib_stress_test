#include "raylib.h"
#include "box2d/box2d.h"
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <math.h>
#include <vector>

const int screenWidth = 800;
const int screenHeight = 600;
int numBalls = 30000;

struct Ball {
    b2Body* body;
    float radius;
    Color color;
};

Ball CreateBall(b2World* world, float radius, Vector2 position, Color color) {
    Ball* ball = new Ball;  // Dynamically allocate to ensure pointer stability
    ball->radius = radius;
    ball->color = color;

    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position.Set(position.x / 100.0f, position.y / 100.0f);
    bodyDef.fixedRotation = true;
    ball->body = world->CreateBody(&bodyDef);

    b2CircleShape circleShape;
    circleShape.m_radius = radius / 100.0f;

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &circleShape;
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 0.0f;
    fixtureDef.restitution = 0.0f;

    ball->body->CreateFixture(&fixtureDef);

    float velocityX = static_cast<float>((std::rand() % 100) - 50) / 50.0f;
    float velocityY = static_cast<float>((std::rand() % 100) - 50) / 50.0f;
    ball->body->SetLinearVelocity(b2Vec2(velocityX, velocityY));

    // Safely store the pointer in user data
    ball->body->GetUserData().pointer = reinterpret_cast<uintptr_t>(ball);

    return *ball;
}

void CreateWall(b2World* world, float x, float y, float width, float height) {
    b2BodyDef bodyDef;
    bodyDef.position.Set(x, y);
    b2Body* body = world->CreateBody(&bodyDef);

    b2PolygonShape boxShape;
    boxShape.SetAsBox(width / 2.0f, height / 2.0f);

    body->CreateFixture(&boxShape, 0.0f);
}

float CalculateMovingAverage(float newValue, float currentAverage, int frameCount) {
    return ((currentAverage * (frameCount - 1)) + newValue) / frameCount;
}

// Query callback class for gathering balls within the visible area
class QueryCallback : public b2QueryCallback {
public:
    std::vector<Ball*> visibleBalls;

    bool ReportFixture(b2Fixture* fixture) override {
        b2Body* body = fixture->GetBody();
        Ball* ball = reinterpret_cast<Ball*>(body->GetUserData().pointer);  // Retrieve Ball data safely
        if (ball) {
            visibleBalls.push_back(ball);
        }
        return true;  // Continue querying
    }
};

int main(int argc, char** argv) {
    if (argc > 1) {
        numBalls = std::atoi(argv[1]);
    }

    InitWindow(screenWidth, screenHeight, "Raylib + Box2D Moving Average Benchmark");
    SetTargetFPS(60);

    b2Vec2 gravity(0.0f, 0.0f);
    b2World world(gravity);

    float areaPerBall = 1.0f;
    float boxArea = numBalls * areaPerBall;
    float aspectRatio = static_cast<float>(screenWidth) / screenHeight;
    float boxWidth = sqrt(boxArea * aspectRatio);
    float boxHeight = boxWidth / aspectRatio;

    CreateWall(&world, boxWidth / 2.0f, 0.05f, boxWidth, 0.1f);
    CreateWall(&world, boxWidth / 2.0f, boxHeight - 0.05f, boxWidth, 0.1f);
    CreateWall(&world, 0.05f, boxHeight / 2.0f, 0.1f, boxHeight);
    CreateWall(&world, boxWidth - 0.05f, boxHeight / 2.0f, 0.1f, boxHeight);

    std::vector<Ball*> balls(numBalls);  // Store pointers to Ball instances
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    float fixedRadius = 10.0f;
    for (int i = 0; i < numBalls; i++) {
        Vector2 position = { static_cast<float>(std::rand() % static_cast<int>(boxWidth * 100)),
                             static_cast<float>(std::rand() % static_cast<int>(boxHeight * 100)) };
        Color color = { (unsigned char)(std::rand() % 256),
                       (unsigned char)(std::rand() % 256),
                       (unsigned char)(std::rand() % 256),
                       255 };

        balls[i] = new Ball(CreateBall(&world, fixedRadius, position, color));  // Dynamically allocate Ball objects
    }

    Camera2D camera = { 0 };
    camera.target = { boxWidth * 50.0f, boxHeight * 50.0f };
    camera.offset = { screenWidth / 2.0f, screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f / std::max(screenWidth / boxWidth, screenHeight / boxHeight);

    float physicsMovingAverage = 0;
    float renderMovingAverage = 0;
    int frameCount = 1;

    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_RIGHT)) camera.target.x += 10;
        if (IsKeyDown(KEY_LEFT)) camera.target.x -= 10;
        if (IsKeyDown(KEY_DOWN)) camera.target.y += 10;
        if (IsKeyDown(KEY_UP)) camera.target.y -= 10;

        if (IsKeyDown(KEY_Z)) camera.zoom += 0.02f * camera.zoom;
        if (IsKeyDown(KEY_X)) camera.zoom -= 0.02f * camera.zoom;

        auto physicsStart = std::chrono::high_resolution_clock::now();
        world.Step(1.0f / 60.0f, 1, 1);
        auto physicsEnd = std::chrono::high_resolution_clock::now();

        float physicsTime = std::chrono::duration<float, std::milli>(physicsEnd - physicsStart).count();
        physicsMovingAverage = CalculateMovingAverage(physicsTime, physicsMovingAverage, frameCount);

        auto renderStart = std::chrono::high_resolution_clock::now();
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(camera);

        // Define visible area based on the camera's current settings
        Vector2 topLeft = GetScreenToWorld2D({ 0, 0 }, camera);
        Vector2 bottomRight = GetScreenToWorld2D({ static_cast<float>(screenWidth), static_cast<float>(screenHeight) }, camera);

        // Convert screen-space coordinates to Box2D world-space bounds
        b2AABB visibleArea;
        visibleArea.lowerBound = b2Vec2(topLeft.x / 100.0f, topLeft.y / 100.0f);
        visibleArea.upperBound = b2Vec2(bottomRight.x / 100.0f, bottomRight.y / 100.0f);

        // Query the world for balls within the visible area
        QueryCallback queryCallback;
        world.QueryAABB(&queryCallback, visibleArea);

        // Render only the visible balls
        for (Ball* ball : queryCallback.visibleBalls) {
            b2Vec2 position = ball->body->GetPosition();
            DrawCircle(static_cast<int>(position.x * 100), static_cast<int>(position.y * 100), ball->radius, ball->color);
        }

        EndMode2D();

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

    // Clean up dynamically allocated balls
    for (Ball* ball : balls) {
        delete ball;
    }
    CloseWindow();

    return 0;
}
