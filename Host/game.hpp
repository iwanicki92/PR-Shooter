#pragma once
#include "basic_structs.hpp"
#include "server_wrapper.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>
#include <array>
#include <utility>
#include <mutex>
#include <cstdint>

struct Projectile : Circle {
    size_t owner_id;
    Vector velocity;

    Projectile();
    Projectile(size_t owner_id, Point start_position, Vector velocity);
    Point& getPosition();
    const Point& getPosition() const;
};

struct Player : Circle {
    size_t player_id;
    bool alive = false;
    uint8_t health = 100;
    Vector velocity = {0, 0};
    float orientation_angle = 0;

    Player(size_t player_id = 0);
    Point& getPosition();
    const Point& getPosition() const;
};

struct Map {
    std::vector<Point> borders;
    // border is inside this rectangle(top left, bottom right points)
    Point top_left, bottom_right;
    std::vector<Rectangle> walls;
    std::vector<Circle> obstacles;
};

struct PairHash {
    size_t operator()(std::pair<uint16_t, uint16_t> p) const noexcept {
        return size_t(p.first) << 16 | p.second;
    }
};

class Game {
public:
    Game(std::string map_name = "map1");
    ~Game();
    void run();

private:
    void getMap(std::string map_name);
    void handleMessage(IncomingMessageWrapper&& message);
    void createNewPlayer(size_t player_id);
    void deletePlayer(size_t player_id);
    void updatePositions();
    void checkCollisions();
    bool checkProjectileCollisions(const Projectile& projectile);
    Message serializeGameState();
    Message serializeMap();
    void updateThread();
    void sendThread();
    void receiveThread();
    void sendWelcomeMessage(size_t player_id);
    void moveAlongNormal(Player& player, const Point& P1, const Point& P2);
    void moveAlongNormal(Player& player, const Circle& object);
    void moveAlongNormal(Player& player, const Rectangle& object);
    void moveAlongNormal(Player& player, Player& object);
    void shootProjectile(size_t player_id);
    void spawnPlayer(size_t player_id);
    bool isInsideBorder(const Point& point) const;
    void changePlayerOrientation(size_t player_id, float angle);
    void changePlayerMovement(size_t player_id, double velocity_x, double velocity_y);

    Map game_map;
    std::unordered_map<size_t, Player> players;
    std::vector<Projectile> projectiles;
    std::mutex update_mutex;

    // debug
    std::unordered_map<size_t, std::pair<size_t, size_t>> packets;
    // calc_time[(no_players, no_projectiles)]["collision"] = (no_collisions, total_time)
    std::unordered_map<std::pair<uint16_t, uint16_t>, // <key
                        std::unordered_map<std::string, // value: map<key,...
                                            std::pair<size_t, std::chrono::microseconds>>, // ...value>
                        PairHash> calc_time; // hash>: map<pair, map<string, pair>, hash>
};