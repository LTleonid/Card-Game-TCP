#include "Client.h"
#include <SFML/Network.hpp>
#include <iostream>
#include <set>
using namespace std;



std::string Client::cardName(int cardIndex) {
    switch (cardIndex)
    {
    case suite::JACK: return "Jack";
    case suite::QUEEN: return "Queen";
    case suite::KING: return "King";
    case suite::ACE: return "Ace";
    case suite::JOCKER: return "Joker";
    default: return "Unknown";
    }
}



void Client::connectServer(sf::IpAddress ip, unsigned short port) {
    if (socket.connect(ip, port) != sf::Socket::Done)
        return;
    cout << "Connected to server " << ip << endl;
    sf::Packet packet;
    packet << name;
}

sf::Packet Client::reciveData() {
    sf::Packet packet;
    if (socket.receive(packet) == sf::Socket::Done) {
        return packet;
    }
    else {
        cout << "Error: Failure recive data" << endl;
        return sf::Packet();
    }
}

int Client::sendData(Data data) {
    sf::Packet packet = data.get();
    if (socket.send(packet) == sf::Socket::Done) {
        return 0;
    }
    else {
        cout << "Error sending data to server" << endl;
        return -1;
    }
}

