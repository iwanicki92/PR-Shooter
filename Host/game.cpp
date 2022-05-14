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
};

Game::Game() {
    static bool first_init = false;
    if(first_init == false) {
        // CTRL+C
        struct sigaction action = {.sa_flags = SA_RESTART};
        action.sa_handler = [](int) { stop_signal = true; };
        sigfillset(&action.sa_mask);
        sigaction(SIGINT, &action, NULL);
        first_init = true;
    }
}

void Game::run() {
    Server::run();
    std::thread update(&Game::updateThread, this);
    std::thread send(&Game::sendThread, this);
    std::thread receive(&Game::receiveThread, this);
    while(stop_signal == false) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    stop.store(true);
    update.join();
    send.join();
    receive.join();
    Server::stop();
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
        Message game_state = serialize();
        update_mutex.unlock();
        Server::sendMessageToEveryone(game_state);
        //Server::sendMessageToEveryone(game_state);
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
                break;
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

    }
}

Message Game::serialize() {
    return Message{.size = 0, .data = nullptr};
}