#include "game.hpp"
#include "timer.hpp"

volatile static sig_atomic_t stop = false;

struct Constants {
    static constexpr std::chrono::milliseconds timestep{8};
    static constexpr std::chrono::milliseconds send_update{16};
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
        action.sa_handler = [](int) { stop = true; };
        sigfillset(&action.sa_mask);
        sigaction(SIGINT, &action, NULL);
        first_init = true;
    }
}

void Game::run() {
    stop = !Server::run();

    Timer update_timer;
    Timer<>::DurationType accumulator(0);
    while(stop == false) {
        accumulator += update_timer.restart();
        while(accumulator > Constants::timestep) {
            updatePositions();
            checkCollisions();
            accumulator -= Constants::timestep;
        }
        if(Server::isMessageWaiting()) {
            handleMessage(Server::takeMessage());
        }
    }

    Server::stop();
}

void Game::handleMessage(IncomingMessageWrapper&& message) {
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