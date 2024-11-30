#pragma once
#include <SFML/Network.hpp>
#include <set>
#include <vector>
#include <iostream>
using namespace std;
// Содержит JACK, QUEEN, KING, ACE, JOCKER
enum suite { JACK, QUEEN, KING, ACE, JOCKER };
// Положил карту | Обвинения
enum Type { place = 12, accusation = 13, startDeck = 500, notification = 250, Shoot = 62 };
enum gameMode { SERVER = 2, CLIENT = 1 };

sf::Packet& operator <<(sf::Packet& packet, const vector<int>& d) {
    cout << "PACKET sen: " << (int)d.size() << endl;
    packet << (int)d.size(); // Почему то size не даёт int по умолчанию
    for (int value : d) {
#ifdef _DEBUG
        cout << "PACKET sen: " << value << endl;
#endif //_DEBUG
        packet << value;
    }
    return packet;
}

sf::Packet& operator >>(sf::Packet& packet, vector<int>& d) {
    int size;
    packet >> size;
    cout << "PACKET rec: " << size << endl;
    d.clear();
    for (int i = 0; i < size; ++i) {
        int value;
        packet >> value;
#ifdef _DEBUG
        cout << "PACKET rec: " << value << endl;
#endif //_DEBUG
        d.push_back(value);
    }
    return packet;
}

struct Data {
    int type;
    bool accusation;
    vector<int> cards;
    set<int> cardsUses;
    int indexPlayer;
    string str;
    Data() : type{ -1 } {}
    Data(int type, bool lie, int index) : type{ type }, accusation{ lie }, indexPlayer{ index } {}
    Data(int type, vector<int> cards) : type{ type }, cards{ cards } {}
    Data(int type, string message) : type{ type }, str{ message } {}
    sf::Packet get() {
        sf::Packet p;
        if (type == Type::accusation) {
            p << type << accusation << indexPlayer;
        }
        else if (type == Type::place) {
            p << type << cards;
        }
        else if (type == Type::startDeck) {
            p << type << cards;
        }
        else if (type == Type::notification) {
            p << type << str;
        }
        else {
            cout << "Error: Undefined Type Data" << endl;
            return sf::Packet();
        }
        return p;
    }

};


class Client
{
protected:
    sf::TcpSocket socket;
    string name;
    int uid;
    sf::IpAddress ip;
public:
    Client(int uid, string name) : name{ name }, uid{ uid }, ip{ sf::IpAddress::getLocalAddress()} {}
    void connectServer(sf::IpAddress ip, unsigned short port);

    std::string cardName(int cardIndex);

    sf::Packet reciveData();

    int sendData(Data data);

};

