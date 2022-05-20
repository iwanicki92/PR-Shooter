#include "game.hpp"
#include "timer.hpp"
#include "collisions.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <random>

// set by signal handler, waited on by run()
volatile static sig_atomic_t stop_signal = false;
// set by run() after exiting loop, checked by other threads
volatile static std::atomic<bool> stop = false;

struct Constants {
    static constexpr std::chrono::milliseconds timestep{8};
    static constexpr std::chrono::duration<double> double_timestep{timestep};
    static_assert(timestep > std::chrono::milliseconds(2));
    static constexpr std::chrono::milliseconds send_delay{16};
    static constexpr double epsilon_one = 0.00001;
    // distance traveled per second
    static constexpr uint16_t max_player_speed = 150;
    static constexpr uint16_t projectile_speed = 100;
    static constexpr uint16_t projectile_damage = 10;
    static constexpr double projectile_radius = 2;
    static constexpr double player_radius = 10;
};

template <class CopyAs, class ArgType>
inline typename std::enable_if_t<not std::is_same_v<std::decay_t<ArgType>, Point>, size_t>
copyToBuf(unsigned char*& buf, ArgType val) {
    *reinterpret_cast<CopyAs*>(buf) = static_cast<CopyAs>(val);
    buf += sizeof(CopyAs);
    return sizeof(CopyAs);
}

template<class CopyAs, class ArgType>
inline typename std::enable_if_t<std::is_same_v<std::decay_t<ArgType>, Point>, size_t>
copyToBuf(unsigned char*& buf, ArgType point) {
    size_t size = copyToBuf<double>(buf, point.x);
    return size + copyToBuf<double>(buf, point.y);
}

Projectile::Projectile() : Circle(Point(), Constants::projectile_radius) {}

Point& Projectile::getPosition() {
    return centre;
}

const Point& Projectile::getPosition() const {
    return centre;
}

Player::Player(size_t player_id) : Circle(Point(), Constants::player_radius), player_id(player_id) {}

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
    for(const auto& [id, number] : packets) {
        std::cout << "Client(" << id << "): recv = " << number.first << ", send = " << number.second << "\n";
    }
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
        for(auto& [player_id, player] : players) {
            packets[player_id].second += 1;
        }
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
                    Server::sendMessageTo(map, message.getClientId());
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
                    case SHOOT:
                        shootProjectile(message.getClientId());
                        break;
                    case CHANGE_ORIENTATION:
                        changePlayerOrientation(message.getClientId(), *reinterpret_cast<float*>(message.getBuffer() + 1));
                        break;
                    case CHANGE_MOVEMENT_DIRECTION:
                        changePlayerMovement(message.getClientId(), *reinterpret_cast<double*>(message.getBuffer() + 1),
                                            *reinterpret_cast<double*>(message.getBuffer() + 9));
                        break;
                    default:
                        std::cout << "UNKNOWN\n";
                        break;
                }
                break;
        }
        ++packets[message.getClientId()].first;
        update_mutex.unlock();
    }
}

void Game::shootProjectile(size_t player_id) {
    // TODO create projectile
}

void Game::spawnPlayer(size_t player_id) {
    Player& player = players[player_id];
    if(player.alive == false) {
        auto r_engine = std::default_random_engine(std::random_device()());
        auto dist = std::uniform_int_distribution(static_cast<int>(Constants::player_radius), 500);
        player.alive = true;
        player.centre = Point(dist(r_engine), dist(r_engine));
    }
}

void Game::changePlayerOrientation(size_t player_id, float angle) {
    players[player_id].orientation_angle = angle;
}

void Game::changePlayerMovement(size_t player_id, double velocity_x, double velocity_y) {
    if(square(velocity_x) + square(velocity_y) <= 1 + Constants::epsilon_one) {
        players[player_id].velocity = Vector(velocity_x, velocity_y);
    }
    else {
        std::cout << "|v| = " << square(velocity_x) + square(velocity_y) << "\n";
    }
}

void Game::createNewPlayer(size_t player_id) {
    players[player_id] = Player(player_id);
    packets[player_id] = {0,0};
    std::cout << "New player connected: " << player_id << "\n";
}

void Game::deletePlayer(size_t player_id) {
    players.erase(player_id);
}

void Game::sendWelcomeMessage(size_t player_id) {
    // TODO send needed constants(max player speed, projectile speed)
    const size_t size = 1 + 2 + sizeof(double) * 2;
    unsigned char* buf = new unsigned char[size];
    Message msg = {.size = size, .data = buf};
    copyToBuf<uint8_t>(buf, DataType::WELCOME_MESSAGE);
    copyToBuf<uint16_t>(buf, player_id);
    copyToBuf<double>(buf, Constants::player_radius);
    copyToBuf<double>(buf, Constants::projectile_radius);
    Server::sendMessageTo(msg, player_id);
}

void Game::updatePositions() {
    for(auto& [player_id, player] : players) {
        if(player.alive == false) {
            continue;
        }
        player.getPosition() += player.velocity * Constants::max_player_speed * Constants::double_timestep.count();
    }
    for(auto& projectile : projectiles) {
        projectile.getPosition() += projectile.velocity * Constants::projectile_speed * Constants::double_timestep.count();
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
            else if(checkCollision(player, second_player)) {
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
    player.getPosition() += (player.velocity * Constants::max_player_speed * Constants::double_timestep.count()) * -1;
}

void Game::moveAlongNormal(Player& player, const Rectangle& object) {
    // TODO move player to collision point along normal(sliding), currently it pushes back
    player.getPosition() += (player.velocity * Constants::max_player_speed * Constants::double_timestep.count()) * -1;
}

Message Game::serializeGameState() {
    uint16_t players_size = static_cast<uint16_t>(players.size());
    uint16_t projectiles_size = static_cast<uint16_t>(projectiles.size());
    unsigned char* buf = new unsigned char[5 + players_size * sizeof(Player) + projectiles_size * sizeof(projectiles_size)];
    uint32_t size = 0;
    Message message = {.data = buf};
    size += copyToBuf<uint8_t>(buf, DataType::GAME_STATE);
    size += copyToBuf<uint16_t>(buf, players_size);
    size += copyToBuf<uint16_t>(buf, projectiles_size);
    for(const auto& [player_id, player] : players) {
        size += copyToBuf<uint16_t>(buf, player.player_id);
        size += copyToBuf<uint8_t>(buf, player.alive);
        size += copyToBuf<uint8_t>(buf, player.health);
        size += copyToBuf<double>(buf, player.getPosition());
        size += copyToBuf<double>(buf, player.velocity);
        size += copyToBuf<float>(buf, player.orientation_angle);
    }
    for(const auto& projectile : projectiles) {
        size += copyToBuf<uint16_t>(buf, projectile.owner_id);
        size += copyToBuf<double>(buf, projectile.getPosition());
        size += copyToBuf<double>(buf, projectile.velocity);
    }
    message.size = size;
    return message;
}

Message Game::serializeMap() {
    uint16_t walls_size = static_cast<uint16_t>(game_map.walls.size());
    uint16_t obstacles_size = static_cast<uint16_t>(game_map.obstacles.size());
    unsigned char* buf = new unsigned char[5 + walls_size * sizeof(Rectangle) + obstacles_size * sizeof(Circle)];
    uint32_t size = 0;
    Message message = {.data = buf};
    size += copyToBuf<uint8_t>(buf, DataType::GAME_MAP);
    size += copyToBuf<uint16_t>(buf, walls_size);
    size += copyToBuf<uint16_t>(buf, obstacles_size);
    for(const auto& wall : game_map.walls) {
        for(int i = 0; i < 4; ++i)
            size += copyToBuf<double>(buf, wall.points[i]);
    }
    for(const auto& obstacle : game_map.obstacles) {
        size += copyToBuf<double>(buf, obstacle.centre);
        size += copyToBuf<double>(buf, obstacle.r);
    }
    message.size = size;
    return message;
}
