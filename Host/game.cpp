#include "game.hpp"
#include "timer.hpp"
#include "collisions.h"
#include <iostream>
#include <thread>
#include <atomic>

// set by signal handler, waited on by run()
volatile static sig_atomic_t stop_signal = false;
// set by run() after exiting loop, checked by other threads
volatile static std::atomic<bool> stop = false;

struct Constants {
    static constexpr std::chrono::milliseconds timestep{8};
    static constexpr std::chrono::duration<float> float_timestep{timestep};
    static_assert(timestep > std::chrono::milliseconds(2));
    static constexpr std::chrono::milliseconds send_delay{16};
    // distance traveled per second
    static constexpr uint16_t max_player_speed = 50;
    static constexpr uint16_t projectile_speed = 100;
    static constexpr uint16_t projectile_damage = 10;
    static constexpr uint16_t projectile_radius = 10;
    static constexpr uint16_t player_radius = 10;
    static constexpr uint16_t serialized_projectile_size = 34;
    static constexpr uint16_t serialized_player_size = 39;
};

Projectile::Projectile() : Circle(Point(), Constants::projectile_radius) {}

Point& Projectile::getPosition() {
    return centre;
}

const Point& Projectile::getPosition() const {
    return centre;
}

Player::Player() : Circle(Point(), Constants::player_radius) {}

Point& Player::getPosition() {
    return centre;
}

const Point& Player::getPosition() const {
    return centre;
}

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
                switch(static_cast<DataType>(message.getBuffer()[0])) {
                    case SPAWN:
                        spawnPlayer(message.getClientId());
                        break;
                    case CHANGE_ORIENTATION:
                        changePlayerOrientation(message.getClientId(), *reinterpret_cast<float*>(message.getBuffer() + 1));
                        break;
                    case CHANGE_MOVEMENT_DIRECTION:
                        changePlayerMovement(message.getClientId(), *reinterpret_cast<double*>(message.getBuffer() + 1),
                                            *reinterpret_cast<double*>(message.getBuffer() + 9));
                        break;
                }
                break;
        }
        update_mutex.unlock();
    }
}

void Game::spawnPlayer(size_t player_id) {
    players[player_id].alive = true;
    // TODO set random position inside boundaries
}

void Game::changePlayerOrientation(size_t player_id, float angle) {
    players[player_id].view_angle = angle;
}

void Game::changePlayerMovement(size_t player_id, double velocity_x, double velocity_y) {
    if(square(velocity_x) + square(velocity_y) <= 1) {
        players[player_id].velocity = Vector(velocity_x, velocity_y);
    }
}

void Game::createNewPlayer(size_t player_id) {
    players[player_id] = Player();
}

void Game::deletePlayer(size_t player_id) {
    players.erase(player_id);
}

void Game::sendWelcomeMessage(size_t player_id) {
    // TODO send needed constants(max player speed, projectile speed)
    const size_t size = 7;
    unsigned char* buf = new unsigned char[size];
    Message msg = {.size = size, .data = buf};
    *buf = DataType::WELCOME_MESSAGE;
    buf += 1;
    *reinterpret_cast<uint16_t*>(buf) = static_cast<uint16_t>(player_id);
    buf += 2;
    *reinterpret_cast<uint16_t*>(buf) = static_cast<uint16_t>(Constants::player_radius);
    buf += 2;
    *reinterpret_cast<uint16_t*>(buf) = static_cast<uint16_t>(Constants::projectile_radius);
    buf += 2;    
    Server::sendMessageTo(msg, player_id);
}

void Game::sendCurrentMap(Message msg, size_t player_id) {
    Server::sendMessageTo(msg, player_id);
}

void Game::updatePositions() {
    for(auto& [player_id, player] : players) {
        player.getPosition() += player.velocity * Constants::max_player_speed * Constants::float_timestep.count();
    }
    for(auto& projectile : projectiles) {
        projectile.getPosition() += projectile.velocity * Constants::projectile_speed * Constants::float_timestep.count();
    }
}

void Game::checkCollisions() {
    for(auto& [player_id, player] : players) {
        if(player.alive == false) {
            continue;
        }
        for(const auto& wall : game_map.walls) {
            if(checkCollision(player, wall)) {
                moveAlongNormal(player, wall);
            }
        }
        for(const auto& obstacle : game_map.obstacles) {
            if(checkCollision(player, obstacle)) {
                moveAlongNormal(player, obstacle);
            }
        }
        for(const auto& [player_id, second_player] : players) {
            if(&second_player == &player) {
                continue;
            }
            if(checkCollision(player, second_player)) {
                moveAlongNormal(player, second_player);
            }
        }
    }
    for(auto projectile_iter = projectiles.begin(); projectile_iter != projectiles.end();) {
        if(checkProjectileCollisions(*projectile_iter)) {
            projectile_iter = projectiles.erase(projectile_iter);
        } else {
            ++projectile_iter;
        }
    }
}

bool Game::checkProjectileCollisions(const Projectile& projectile) {
    for(const auto& wall : game_map.walls) {
        if(checkCollision(projectile, wall)) {
            return true;
        }
    }
    for(const auto& obstacle : game_map.obstacles) {
        if(checkCollision(projectile, obstacle)) {
            return true;
        }
    }
    for(auto& [player_id, player] : players) {
        if(player.alive == true && checkCollision(projectile, player)) {
            if(player.health <= Constants::projectile_damage) {
                player.alive = false;
            } else {
                player.health -= Constants::projectile_damage;
            }
            return true;
        }
    }
    return false;
}

void Game::moveAlongNormal(Player& player, const Circle& object) {
    // TODO move player to collision point along normal(sliding), currently it pushes back
    player.getPosition() += (player.velocity * Constants::max_player_speed * Constants::float_timestep.count()) * -1;
}

void Game::moveAlongNormal(Player& player, const Rectangle& object) {
    // TODO move player to collision point along normal(sliding), currently it pushes back
    player.getPosition() += (player.velocity * Constants::max_player_speed * Constants::float_timestep.count()) * -1;
}

void copyPointToBuf(unsigned char* buf, const Point& point) {
    *reinterpret_cast<double*>(buf) = point.x;
    buf += 8;
    *reinterpret_cast<double*>(buf) = point.y;
}

Message Game::serializeGameState() {
    uint16_t players_size = static_cast<uint16_t>(players.size());
    uint16_t projectiles_size = static_cast<uint16_t>(projectiles.size());
    const uint32_t size = 5 + Constants::serialized_player_size * players_size + Constants::serialized_projectile_size * projectiles_size;
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
        copyPointToBuf(buf, player.getPosition());
        buf += 16;
        copyPointToBuf(buf, player.velocity);
        *reinterpret_cast<float*>(buf) = player.view_angle;
        buf += 4;
    }
    for(const auto& projectile : projectiles) {
        *reinterpret_cast<uint16_t*>(buf) = static_cast<uint16_t>(projectile.owner_id);
        buf += 2;
        copyPointToBuf(buf, projectile.getPosition());
        buf += 16;
        copyPointToBuf(buf, projectile.velocity);
        buf += 16;
    }
    return message;
}

Message Game::serializeMap() {
    constexpr uint16_t wall_sizeof = 4 * 16;
    constexpr uint16_t obstacle_sizeof = 3 * 8;
    uint16_t walls_size = static_cast<uint16_t>(game_map.walls.size());
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
    for(const auto& wall : game_map.walls) {
        copyPointToBuf(buf, wall.points[0]);
        copyPointToBuf(buf + 16, wall.points[1]);
        copyPointToBuf(buf + 32, wall.points[2]);
        copyPointToBuf(buf + 48, wall.points[3]);
        buf += 64;
    }
    for(const auto& obstacle : game_map.obstacles) {
        copyPointToBuf(buf, obstacle.centre);
        buf += 16;
        *reinterpret_cast<double*>(buf) = obstacle.r;
        buf += 8;
    }
    return message;
}
