#include "game.hpp"
#include "timer.hpp"
#include <iostream>
#include <thread>
#include <atomic>

// set by signal handler, waited on by run()
volatile static sig_atomic_t stop_signal = false;
// set by run() after exiting loop, checked by other threads
volatile static std::atomic<bool> stop = false;

struct Constants {
    static constexpr std::chrono::milliseconds timestep{8};
    static_assert(timestep > std::chrono::milliseconds(2));
    static constexpr std::chrono::milliseconds send_delay{16};
    // distance traveled per second
    static constexpr uint16_t max_player_speed = 50;
    static constexpr uint16_t projectile_speed = 100;
    static constexpr uint16_t player_size = 10;
    static constexpr uint16_t serialize_projectile_size = 22;
    static constexpr uint16_t serialized_player_size = 31;
};

Game::Game() {
    static bool first_init = false;
    Server::run();
    if(first_init == false) {
        // CTRL+C
        struct sigaction action = {.sa_flags = SA_RESTART};
        action.sa_handler = [](int) { stop_signal = true; };
        sigfillset(&action.sa_mask);
        sigaction(SIGINT, &action, NULL);
        first_init = true;
    }
}

Game::~Game() {
    Server::stop();
}

void Game::run() {
    std::thread update(&Game::updateThread, this);
    std::thread send(&Game::sendThread, this);
    std::thread receive(&Game::receiveThread, this);
    while(stop_signal == false) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    stop.store(true);
    receive.join();
    send.join();
    update.join();
}

void Game::updateThread() {
    Timer update_timer;
    Timer<>::DurationType accumulator(0);

    while(stop.load() == false) {
        std::this_thread::sleep_for(Constants::timestep - std::chrono::milliseconds(2));
        accumulator += update_timer.restart();
        if(accumulator > Constants::timestep) {
            update_mutex.lock();
            while(accumulator > Constants::timestep) {
                updatePositions();
                checkCollisions();
                accumulator -= Constants::timestep;
            }
            update_mutex.unlock();
        }
    }
}

void Game::sendThread() {
    while(stop.load() == false) {
        std::this_thread::sleep_for(Constants::send_delay);
        update_mutex.lock();
        Message game_state = serializeGameState();
        update_mutex.unlock();
        Server::sendMessageToEveryone(game_state);
    }
}

void Game::receiveThread() {
    while(stop.load() == false) {
        handleMessage(Server::takeMessage(1));
    }
}

void Game::handleMessage(IncomingMessageWrapper&& message) {
    if(message.getType() != MessageType::EMPTY) {
        update_mutex.lock();
        switch(message.getType()) {
            case MessageType::NEW_CONNECTION:
                createNewPlayer(message.getClientId());
                sendWelcomeMessage(message.getClientId());
                {
                    Message map = serializeMap();
                    update_mutex.unlock();
                    sendCurrentMap(map, message.getClientId());
                }
                return;
            case MessageType::LOST_CONNECTION:
                deletePlayer(message.getClientId());
                break;
            case MessageType::MESSAGE:
                // deserialize
                break;
        }
        update_mutex.unlock();
    }
}

void Game::createNewPlayer(size_t player_id) {
    players[player_id] = Player();
}

void Game::deletePlayer(size_t player_id) {
    players.erase(player_id);
}

void Game::sendWelcomeMessage(size_t player_id) {
    const size_t size = 3;
    unsigned char* buf = new unsigned char[size];
    Message msg = {.size = size, .data = buf};
    *buf = DataType::CLIENT_ACCEPTANCE;
    buf += 1;
    *reinterpret_cast<uint16_t*>(buf) = static_cast<uint16_t>(player_id);
    buf += 2;
    Server::sendMessageTo(msg, player_id);
}

void Game::sendCurrentMap(Message msg, size_t player_id) {
    Server::sendMessageTo(msg, player_id);
}

void Game::updatePositions() {
    for(auto& [player_id, player] : players) {
        // calculate new position after Constants::timestep
    }
    for(auto& projectile : projectiles) {
        // calculate new position after Constants::timestep
    }
}

void Game::checkCollisions() {
    for(auto& [player_id, player] : players) {
        for(auto& wall : game_map.borders) {
            
        }

        for(auto& wall : game_map.walls) {

        }

        for(auto& obstacle : game_map.obstacles) {

        }
    }
}

void copyPointToBuf(unsigned char* buf, const Point& point) {
    *reinterpret_cast<double*>(buf) = point.x;
    buf += 8;
    *reinterpret_cast<double*>(buf) = point.y;
}

Message Game::serializeGameState() {
    uint16_t players_size = static_cast<uint16_t>(players.size());
    uint16_t projectiles_size = static_cast<uint16_t>(projectiles.size());
    const uint32_t size = 5 + Constants::serialized_player_size * players_size + Constants::serialize_projectile_size * projectiles_size;
    unsigned char* buf = new unsigned char[size];
    Message message = {.size = size, .data = buf};
    *buf = DataType::GAME_STATE;
    buf += 1;
    *reinterpret_cast<uint16_t*>(buf) = players_size;
    buf += 2;
    *reinterpret_cast<uint16_t*>(buf) = projectiles_size;
    buf += 2;
    for(const auto& [player_id, player] : players) {
        *reinterpret_cast<uint16_t*>(buf) = static_cast<uint16_t>(player.player_id);
        buf += 2;
        *buf = player.health;
        buf += 1;
        copyPointToBuf(buf, player.position);
        buf += 16;
        *reinterpret_cast<float*>(buf) = player.speed;
        buf += 4;
        *reinterpret_cast<float*>(buf) = player.movement_angle;
        buf += 4;
        *reinterpret_cast<float*>(buf) = player.view_angle;
        buf += 4;
    }
    for(const auto& projectile : projectiles) {
        *reinterpret_cast<uint16_t*>(buf) = static_cast<uint16_t>(projectile.owner_id);
        buf += 2;
        copyPointToBuf(buf, projectile.position);
        buf += 16;
        *reinterpret_cast<float*>(buf) = projectile.angle;
        buf += 4;
    }
    return message;
}

Message Game::serializeMap() {
    constexpr uint16_t wall_sizeof = 4 * 16;
    constexpr uint16_t obstacle_sizeof = 3 * 8;
    uint16_t walls_size = static_cast<uint16_t>(game_map.borders.size() + game_map.walls.size());
    uint16_t obstacles_size = static_cast<uint16_t>(game_map.obstacles.size());
    const uint32_t size = 5 + walls_size * wall_sizeof + obstacles_size * obstacle_sizeof;
    unsigned char* buf = new unsigned char[size];
    Message message = {.size = size, .data = buf};
    *buf = DataType::GAME_MAP;
    buf += 1;
    *reinterpret_cast<uint16_t*>(buf) = walls_size;
    buf += 2;
    *reinterpret_cast<uint16_t*>(buf) = obstacles_size;
    buf += 2;
    auto rectangle_serialization = [&](const std::vector<Rectangle>& walls) {
        for(const auto& wall : walls) {
            copyPointToBuf(buf, wall.points[0]);
            copyPointToBuf(buf + 16, wall.points[1]);
            copyPointToBuf(buf + 32, wall.points[2]);
            copyPointToBuf(buf + 48, wall.points[3]);
            buf += 64;
        }
    };
    rectangle_serialization(game_map.borders);
    rectangle_serialization(game_map.walls);
    for(const auto& obstacle : game_map.obstacles) {
        copyPointToBuf(buf, obstacle.centre);
        buf += 16;
        *reinterpret_cast<double*>(buf) = obstacle.r;
        buf += 8;
    }
    return message;
}
