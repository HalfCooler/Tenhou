#include "game.hpp"
#include <iostream>
#include <memory>

int main() {
    try {
        std::unique_ptr<authority::Game> game;
        std::string line;
        while (std::getline(std::cin, line)) {
            const auto message = authority::parse(line);
            const auto &a = message.attrs;
            if (message.tag == "CONFIG") game = std::make_unique<authority::Game>(authority::Config::from(a));
            else if (!game) throw std::runtime_error("Missing config");
            else if (message.tag == "START") game->start(authority::integers(a.at("wall")));
            else if (message.tag == "ACT") game->submit(authority::number(a.at("seat")), {a.at("kind"), authority::integers(a.at("tiles"))});
            else throw std::runtime_error("Unknown command");
            if (game) game->invariants();
        }
        if (!game) return 1;
        for (const auto &event: game->events) {
            auto attrs = event.attrs;
            for (const auto &hint: event.hints) if (hint.second) attrs["_h" + std::to_string(hint.first)] = std::to_string(hint.second);
            std::cout << authority::xml(event.tag, attrs) << '\n';
        }
        std::cout << authority::xml("RESULT", {{"scores", authority::list(game->scores)}, {"phase", game->phase}}) << '\n';
        return 0;
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
