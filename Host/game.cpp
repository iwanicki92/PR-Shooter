#include "game.hpp"
#include "timer.hpp"
#include "collisions.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <random>
#include <cmath>
#include <fstream>
#include <sstream>
#include <limits>

// set by signal handler, waited on by run()
volatile static sig_atomic_t stop_signal = false;
// set by run() after exiting loop, checked by other threads
volatile static std::atomic<bool> stop = false;

struct Constants {
    static constexpr std::chrono::milliseconds timestep{3};
    static constexpr std::chrono::duration<double> double_timestep{timestep};
    static_assert(timestep > std::chrono::milliseconds(2));
    static constexpr std::chrono::milliseconds send_delay{16};
    static constexpr double epsilon_one = 0.00001;
    // distance traveled per second
    static constexpr uint16_t max_player_speed = 300;
    static constexpr uint16_t projectile_speed = 500;
    static constexpr uint16_t projectile_damage = 10;
    static constexpr double projectile_radius = 4;
    static constexpr double player_radius = 30;
    static constexpr double border_width = 4;
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

Projectile::Projectile(size_t owner_id, Point start_position, Vector velocity) :
            Circle(start_position, Constants::projectile_radius), owner_id(owner_id), velocity(velocity) {}

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

Game::Game(std::string map_name) {
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
    getMap(map_name);
}

std::pair<char, std::vector<double>> splitMapLine(const std::string& line) {
    std::vector<double> values;
    std::istringstream stream(line);
    std::string type;
    std::getline(stream, type, ',');
    if(type[0] == '#') {
        return std::make_pair(type[0], std::vector<double>());
    }
    std::string value;
    while(std::getline(stream, value, ',')) {
        values.push_back(std::stod(value));
    }
    return {type[0], values};
}

template<class T, class U>
std::ostream& operator<<(std::ostream& out, const std::vector<T, U>& vec) {
    out << "[";
    for(auto iter = vec.begin(); iter != vec.end() ;) {
        out << *iter;
        ++iter;
        if(iter != vec.end()) {
            out << ", ";
        }
    }
    out << "]";
    return out;
}

void Game::getMap(std::string map_name) {
    // add mutex locking before using outside of constructor(during map change)
    std::fstream map(std::string("../Maps/") + map_name, map.in);
    if(!map.is_open()) {
        map.open(std::string("Maps/") + map_name, map.in);
        if(!map.is_open()) {
            std::cout << "Couldn't open ../Maps/" << map_name << " or Maps/" << map_name << "\n";
            return;
        }
    }
    std::string line;
    while(std::getline(map, line)) {
        auto [type, values] = splitMapLine(line);
        switch(type) {
            case 'B':
                for(size_t i = 0; i < values.size(); i += 2) {
                    game_map.borders.push_back(Point(values[i], values[i+1]));
                }
                break;
            case 'W':
                game_map.walls.push_back(Rectangle(Point(values[0], values[1]), Point(values[2], values[3]),
                                                    Point(values[4], values[5]), Point(values[6], values[7])));
                break;
            case 'O':
            {
                Circle obstacle(Point(values[0], values[1]), values[2]);
                game_map.obstacles.push_back(obstacle);
            }
                break;
            case '#':
                break;
            default:
                std::cout << "Unknown type: " << type << ", values = " << values;
        }
    }
    double min_x = std::numeric_limits<double>::max();
    double min_y = min_x;
    double max_x = std::numeric_limits<double>::min();
    double max_y = max_x;
    for(const auto& point : game_map.borders) {
        min_x = std::min(min_x, point.x);
        min_y = std::min(min_y, point.y);
        max_x = std::max(max_x, point.x);
        max_y = std::max(max_y, point.y);
    }
    game_map.top_left = Point(min_x, min_y);
    game_map.bottom_right = Point(max_x, max_y);
}

Game::~Game() {
    Server::stop();
    for(const auto& [id, number] : packets) {
        std::cout << "Client(" << id << "): recv = " << number.first << ", send = " << number.second << "\n";
    }
    auto map_size = game_map.obstacles.size() + game_map.walls.size() + game_map.borders.size() / 2;
    std::cout << "Map size: " << map_size << "\n";
    for(const auto& [sizes, time_map] : calc_time) {
        const auto& [players_size, projectiles_size] = sizes;
        std::cout << "Total objects: " << players_size + projectiles_size + map_size << ", NO_Players: " << players_size << ", NO_Projectiles: " << projectiles_size <<", ";
        for(const auto& [timed_function, time] : time_map) {
            const auto& [no_time, total_time] = time;
            const auto elapsed_time = (total_time / no_time).count();
            std::cout << timed_function << ": " << elapsed_time << " us, ";
        }
        std::cout << "\b\b \n";
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
                Timer start;
                updatePositions();
                auto update_duration = start.restart();
                checkCollisions();
                auto collision_duration = start.duration();
                accumulator -= Constants::timestep;
                auto& update = calc_time[std::make_pair(players.size(), projectiles.size())];
                auto& [no_collision, collision_total_time] = update["collision"];
                auto& [no_update, update_total_time] = update["update"];
                no_collision += 1;
                collision_total_time += update_duration;
                no_update += 1;
                update_total_time += collision_duration;
            }
            update_mutex.unlock();
        }
    }
}

void Game::sendThread() {
    while(stop.load() == false) {
        std::this_thread::sleep_for(Constants::send_delay);
        update_mutex.lock();
        Timer start = Timer();
        Message game_state = serializeGameState();
        auto serialization_duration = start.duration();
        auto& [no_serialize, serialize_total_time] = calc_time[std::make_pair(players.size(), projectiles.size())]["serialize"];
        no_serialize += 1;
        serialize_total_time += serialization_duration;
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
        auto start = Timer();
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
                    case PING:
                    {
                        unsigned char* buf = new unsigned char[3];
                        Message msg = {.size = 3, .data = buf};
                        copyToBuf<uint8_t>(buf, PING);
                        copyToBuf<uint16_t>(buf, *reinterpret_cast<uint16_t*>(message.getBuffer() + 1));
                        Server::sendMessageTo(msg, message.getClientId());
                    }   
                        break;
                    default:
                        std::cout << "UNKNOWN\n";
                        break;
                }
                break;
        }
        auto duration = start.duration();
        auto& [no_receive, receive_total_time] = calc_time[std::make_pair(players.size(), projectiles.size())]["receive"];
        no_receive += 1;
        receive_total_time += duration;
        ++packets[message.getClientId()].first;
        update_mutex.unlock();
    }
}

void Game::shootProjectile(size_t player_id) {
    const auto& player = players[player_id];
    if(player.alive == false) {
        return;
    }
    Vector normalized_direction(cos(player.orientation_angle), sin(player.orientation_angle));
    Projectile projectile(player_id, player.centre + normalized_direction * player.r, normalized_direction);
    projectiles.push_back(projectile);
}

void Game::spawnPlayer(size_t player_id) {
    Player& player = players[player_id];
    if(player.alive == false) {
        auto r_engine = std::default_random_engine(std::random_device()());
        auto dist_x = std::uniform_int_distribution(static_cast<int>(game_map.top_left.x), static_cast<int>(game_map.bottom_right.x));
        auto dist_y = std::uniform_int_distribution(static_cast<int>(game_map.top_left.y), static_cast<int>(game_map.bottom_right.y));
        player.alive = true;
        player.health = 100;
        do {
        player.centre = Point(dist_x(r_engine), dist_y(r_engine));
        } while(!isInsideBorder(player.centre));
        // FIXME add check to make sure player isn't inside another rectangle
        // if players centre is outside then collision check will push him out
        // but if it's inside player will be stuck inside wall
    }
}

bool Game::isInsideBorder(const Point& point) const {
    // TODO check if inside polygon(ray casting?)
    return true;
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
        for(auto& [player_id, second_player] : players) {
            if(&second_player == &player) {
                continue;
            }
            else if(checkCollision(player, second_player)) {
                moveAlongNormal(player, second_player);
            }
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
        for(size_t i = 0; i < game_map.borders.size() - 1; ++i) {
            if(checkCollision(player, game_map.borders[i], game_map.borders[i+1])) {
                moveAlongNormal(player, game_map.borders[i], game_map.borders[i+1]);
            }
        }
        if(!game_map.borders.empty() && checkCollision(player, game_map.borders.back(), game_map.borders.front())) {
            moveAlongNormal(player, game_map.borders.back(), game_map.borders.front());
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
        if(player.alive == true && player_id != projectile.owner_id && checkCollision(projectile, player)) {
            if(player.health <= Constants::projectile_damage) {
                player.alive = false;
                ++player.deaths;
                ++players[projectile.owner_id].kills;
            } else {
                player.health -= Constants::projectile_damage;
            }
            return true;
        }
    }
    for(size_t i = 0; i < game_map.borders.size() - 1; ++i) {
            if(checkCollision(projectile, game_map.borders[i], game_map.borders[i+1])) {
                return true;
            }
        }
    if(!game_map.borders.empty() && checkCollision(projectile, game_map.borders.back(), game_map.borders.front())) {
        return true;
    }
    return false;
}

std::pair<Vector, double> calculateDisplacement(const Circle& circle1, const Circle& circle2) {
    double dist = distance(circle1.centre, circle2.centre);
    double displacement = circle1.r + circle2.r - dist; // displacement length
    Vector displacement_vector = (circle1.centre - circle2.centre);
    displacement_vector /= displacement_vector.length(); // normalized displacement vector
    return std::make_pair(displacement_vector, displacement);
}

std::pair<Vector, double> calculateDisplacement(const Circle& circle, const Point& P1, const Point& P2) {
    auto dx = P2.x - P1.x;
    auto dy = P2.y - P1.y;
    Vector normal = Point(-dy, dx);
    normal /= normal.length(); // normalize vector
    double displacement = circle.r - triangleHeight(circle.centre, P1, P2);
    return std::make_pair(normal, displacement);
}

void Game::moveAlongNormal(Player& player1, Player& player2) {
    const auto& [displacement_vector, displacement] = calculateDisplacement(player1, player2);
    double half_displacement = displacement / 2;
    player1.centre += displacement_vector * half_displacement;
    player2.centre += displacement_vector * half_displacement * -1;
}

void Game::moveAlongNormal(Player& player, const Point& P1, const Point& P2) {
    const auto& [displacement_vector, displacement] = calculateDisplacement(player, P1, P2);
    player.centre += displacement_vector * displacement;
}

void Game::moveAlongNormal(Player& player, const Circle& object) {
    const auto& [displacement_vector, displacement] = calculateDisplacement(player, object);
    player.centre += displacement_vector * displacement;
}

void Game::moveAlongNormal(Player& player, const Rectangle& object) {
    // FIXME wrong displacement when colliding with rectangle edges
    // maybe change to movement along tangent(styczna) or displacement along tangent's normal
    // detecting edge: check if rectangle's points are inside circle? if yes then edge
    if(checkCollision(player, object.points[0], object.points[1])) {
        moveAlongNormal(player, object.points[1], object.points[0]);
    }
    if(checkCollision(player, object.points[1], object.points[2])) {
        moveAlongNormal(player, object.points[2], object.points[1]);
    }
    if(checkCollision(player, object.points[2], object.points[3])) {
        moveAlongNormal(player, object.points[3], object.points[2]);
    }
    if(checkCollision(player, object.points[3], object.points[0])) {
        moveAlongNormal(player, object.points[0], object.points[3]);
    }
}

Message Game::serializeGameState() {
    uint16_t players_size = static_cast<uint16_t>(players.size());
    uint16_t projectiles_size = static_cast<uint16_t>(projectiles.size());
    unsigned char* buf = new unsigned char[5 + players_size * sizeof(Player) + projectiles_size * sizeof(Projectile)];
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
        size += copyToBuf<uint16_t>(buf, player.kills);
        size += copyToBuf<uint16_t>(buf, player.deaths);
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
    uint16_t borders_size = static_cast<uint16_t>(game_map.borders.size());
    unsigned char* buf = new unsigned char[7 + walls_size * sizeof(Rectangle) + obstacles_size * sizeof(Circle) + borders_size * sizeof(Point)];
    uint32_t size = 0;
    Message message = {.data = buf};
    size += copyToBuf<uint8_t>(buf, DataType::GAME_MAP);
    size += copyToBuf<uint16_t>(buf, walls_size);
    size += copyToBuf<uint16_t>(buf, obstacles_size);
    size += copyToBuf<uint16_t>(buf, borders_size);
    for(const auto& wall : game_map.walls) {
        for(int i = 0; i < 4; ++i)
            size += copyToBuf<double>(buf, wall.points[i]);
    }
    for(const auto& obstacle : game_map.obstacles) {
        size += copyToBuf<double>(buf, obstacle.centre);
        size += copyToBuf<double>(buf, obstacle.r);
    }
    for(const auto& point : game_map.borders) {
        size += copyToBuf<double>(buf, point);
    }
    message.size = size;
    return message;
}
