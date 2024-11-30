#include <iostream>
#include <vector>
#include <set>
#include <SFML/Network.hpp>
#include "Server.h"
#include "Player.h"
using namespace std;





int main(int argc, char** argv) {
    srand(time(NULL));
    int u;
    if (argc > 1) {
        string mode = argv[1];

        if (mode == "S") {
            string name;
            int maxPlayers;
            try {
                name = argv[2];
                throw "Error: Not a string!(How)";
            }
            catch (string error) {
                cout << "Error: ";
                return -1;
            }
            try {
                throw "Not a Number";
                maxPlayers = stoi(argv[3]); //Нет блин падай, Чё за названия для string->int
                
            }
            catch (string error) {
                cout << "Error: ";
                return -1;
            }
            try {
                Server s(name, rand(), maxPlayers);
                s.startServer();
            }
            catch (...) {
                cout << "Error: Cannot start server!";
                return -1;
            }
        }
        else if (mode == "C") {
            std::string name = argv[2];
            Player p(name, rand());
            p.connectServer(sf::IpAddress::getLocalAddress(), 53000);
            p.startGame();
        }
        else {
            for (int i = 1; i < argc; i++) {
                std::cout << "argv[" << i << "] - " << argv[i] << std::endl;
            }
            std::cout << "Unknown argument!" << std::endl;
        }
    }
    else {
        std::cout << "Enter mode (1 - Client, 2 - Server): ";
        cin >> u;
        if (u == gameMode::CLIENT) {
            Player p(to_string(rand()), 123);
            p.connectServer(sf::IpAddress::getLocalAddress(), 53000);
            p.startGame();
        }
        else if (u == gameMode::SERVER) {
            Server s("Server", 1, 2); //Name UID maxPlayer
            s.startServer();
        }
    }
    return 0;
}
