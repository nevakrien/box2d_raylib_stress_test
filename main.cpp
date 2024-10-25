#include "raylib.h"
#include "box2d/box2d.h"
#include <cstdlib>
#include <ctime>

// Screen dimensions
const int screenWidth = 800;
const int screenHeight = 600;

// Number of balls (adjusted for stress test)
int numBalls = 1000;  

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
    // Define the static body
    b2BodyDef bodyDef;
    bodyDef.position.Set(x, y);  // Set wall position in meters
    b2Body* body = world->CreateBody(&bodyDef);

    // Define a box shape for the wall
    b2PolygonShape boxShape;
    boxShape.SetAsBox(width / 2.0f, height / 2.0f);  // Half-width and half-height in meters

    // Create the wall fixture
    body->CreateFixture(&boxShape, 0.0f);  // Static bodies have zero density
}

int main(int argc, char** argv) {
    // Allow user to modify number of balls via command line
    if (argc > 1) {
        numBalls = std::atoi(argv[1]);
    }

    // Initialize raylib
    InitWindow(screenWidth, screenHeight, "Raylib + Box2D Stress Test (1 Million Balls, Random Speed)");
    SetTargetFPS(60);

    // Create a Box2D world with no gravity (0,0)
    b2Vec2 gravity(0.0f, 0.0f);  // No gravity
    b2World world(gravity);

    // Create walls around the screen
    CreateWall(&world, screenWidth / 200.0f, 0.05f, screenWidth / 100.0f, 0.1f);       // Top wall
    CreateWall(&world, screenWidth / 200.0f, screenHeight / 100.0f - 0.05f, screenWidth / 100.0f, 0.1f); // Bottom wall
    CreateWall(&world, 0.05f, screenHeight / 200.0f, 0.1f, screenHeight / 100.0f);     // Left wall
    CreateWall(&world, screenWidth / 100.0f - 0.05f, screenHeight / 200.0f, 0.1f, screenHeight / 100.0f); // Right wall

    // Create balls
    Ball* balls = new Ball[numBalls];
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    for (int i = 0; i < numBalls; i++) {
        // Ball radius inversely proportional to the number of balls (1/x)
        float radius = 60.0f / (i + 1);  // Decreases as i increases
        Vector2 position = { static_cast<float>(std::rand() % screenWidth), static_cast<float>(std::rand() % screenHeight) };
        Color color = { (unsigned char)(std::rand() % 256),
                       (unsigned char)(std::rand() % 256),
                       (unsigned char)(std::rand() % 256),
                       255 };  // Random color

        balls[i] = CreateBall(&world, radius, position, color);
    }

    // Main game loop
    while (!WindowShouldClose()) {
        // Update physics world (60 steps per second)
        world.Step(1.0f / 60.0f, 6, 2);

        // Start drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw all balls
        for (int i = 0; i < numBalls; i++) {
            b2Vec2 position = balls[i].body->GetPosition();
            // Render the balls at their Box2D-calculated position
            DrawCircle(static_cast<int>(position.x * 100), static_cast<int>(position.y * 100), balls[i].radius, balls[i].color);
        }

        // Display the number of balls and FPS
        DrawText(TextFormat("Number of balls: %d", numBalls), 10, 10, 20, DARKGRAY);
        DrawFPS(10, 30);

        EndDrawing();
    }

    // Clean up
    delete[] balls;
    CloseWindow();  // Close window and OpenGL context

    return 0;
}
