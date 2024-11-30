#pragma once
#include <vector>
#include <set>
#include <iostream>
#include <stack>
#include <random>
#include "Player.h"
#include <SFML/Network.hpp>
using namespace std;

class Server : public Player {
private:

    vector<sf::TcpSocket* > clients{};
    vector<string> clientsNames{};
    int maxPlayer;
    unsigned short int port = 53000;

    sf::SocketSelector selector;
    sf::TcpListener listener;


    vector<int> deck;
    vector<int> getDeck() {
        return deck;
    }
    void appendDeck(int card) {
        deck.push_back(card);
    }



    int jCards; // Jack
    int qCards; // Queen
    int kCards; // King
    int aCards; // Ace
    int JCards; // Joker
    int currentCard; // Верная карта

    int lastQuanityCards;
    stack<int> currentDeck;


public:
    unsigned short int getPort() {
        return port;
    }
    //Name UID maxPlayer
    Server(string name, int uid, int maxPlayer) : Player(name, uid), maxPlayer{ maxPlayer }, jCards{ 6 }, qCards{ 6 }, kCards{ 6 }, aCards{ 6 }, JCards{ 3 }, lastQuanityCards{ 0 }, currentCard{ -1 } {

        listener.listen(port);
        selector.add(listener);
        listener.setBlocking(true);

    }
    bool sendPacket(sf::Packet& packet, sf::TcpSocket& Rx, bool clear = true) {
        cout << "Send Packet " << packet.getData() << " to " << Rx.getRemoteAddress() << ":" << Rx.getRemotePort() << endl;
        if (Rx.send(packet) == sf::Socket::Done) {
            cout << "Success!" << endl;
            if (clear) packet.clear();
            return true;
        }
        else {
            cout << "Error: Cannot send packet!" << endl;
            return false;
        }
        cout << "Error: it has not send anything" << endl;
        return false;
    }
    void Ready() {

        InitializationDeck();
        int tmp = 0;
        for (auto player = clients.begin(); player != clients.end(); ++player) {
            sf::TcpSocket& Splayer = **player; // Получаем указатель на игрока
            sf::Packet packet;
            int status = Player::Status::ready;
            int prevIndex = (tmp - 1 + clients.size()) % clients.size();
            sf::TcpSocket& prevPlayer = *clients[prevIndex];
            packet << status << prevIndex;

            if (sendPacket(packet, Splayer)) {
                cout << "Player " << Splayer.getRemoteAddress() << " is ready!" << endl;
            }
            else {
                cout << "Error: Failed to send ready " << Splayer.getRemoteAddress() << endl;
            }
            giveCards(Splayer);
            tmp++;
        }


    }

    void giveCards(sf::TcpSocket& player) {
        sf::Packet packet;
        vector<int> cards;

        // Генерация карт для игрока
        for (int i = 0; i < 6; ++i) {
            cards.push_back(getCardfromDeck());
        }

        packet << Type::startDeck << cards;

        if (sendPacket(packet, player)) {
            cout << "Player " << player.getRemoteAddress() << "Get: ";
            for (int card : cards) {
                cout << cardName(card) << " ";
            }
            cout << endl;
        }
        else {
            cout << "Error: Cannot give cards to " << player.getRemoteAddress() << endl;
        }
    }


    int getCardfromDeck() {
        int copy = deck[0];
        cout << "Get " << cardName(copy) << endl;
        this->deck.erase(deck.begin());
        return copy;
    }

    int InitializationDeck() {

        if (deck.empty()) {

            for (int i = 0; i < jCards; i++) appendDeck(JACK);
            for (int i = 0; i < qCards; i++) appendDeck(QUEEN);
            for (int i = 0; i < kCards; i++) appendDeck(KING);
            for (int i = 0; i < aCards; i++) appendDeck(ACE);
            for (int i = 0; i < JCards; i++) appendDeck(JOCKER);

            mt19937 rng = default_random_engine(time(NULL));
            shuffle(deck.begin(), deck.end(), rng);
            currentCard = rng() % 5;

        }
        return 1;
    }
    void playerList() {

        cout << "Players: " << clients.size() << endl;

        for (int i = 0; i < clients.size(); i++) {
            cout << clientsNames[i] << " | " << clients[i]->getRemoteAddress() << ":" << clients[i]->getRemotePort() << endl;
        }
    }
    void startServer() {
        cout << "Server is listening on port " << port << endl;
        while (true) {
            system("cls");
            playerList();
            if (clients.size() != maxPlayer) {
                if (selector.wait(sf::milliseconds(100))) {
                    if (selector.isReady(listener)) {
                        cout << "New connection detected" << endl;
                        sf::TcpSocket* client = new sf::TcpSocket;
                        if (listener.accept(*client) == sf::Socket::Done) {

                            sf::Packet packet;
                            if (client->receive(packet) == sf::Socket::Done) {
                                string playerName;
                                packet >> playerName;
                                clientsNames.push_back(playerName);
                            }
                            clients.push_back(client);
                            selector.add(*client);
                            cout << "Received new connection: " << client->getRemoteAddress() << ":" << client->getRemotePort() << endl;
                        }
                        else {
                            delete client;
                        }
                    }
                }
            }
            else {
                break;
            }
        }
        newGame();
    }

    void sendAll(sf::Packet& packet) {
        for (auto player : clients) {
            sf::TcpSocket& Pplayer = *player;
            sendPacket(packet, Pplayer, false);
        }
        packet.clear();
    }

    void sendAccusation(bool lie, sf::TcpSocket& Rx, sf::TcpSocket& Tx, int index) {
        sf::Packet packet;


        if (lie) {

            packet << Type::notification << clientsNames[index] + "Is lied!";
            sendAll(packet);
            packet << Type::Shoot;
            sendPacket(packet, Tx);

        }
        else {
            packet << Type::notification << clientsNames[index + 1] + "Is make mistakes!";
            sendAll(packet);
            packet << Type::Shoot;
            sendPacket(packet, Rx);

        }
    }


    void newGame() {
        Ready();
        cout << "Current Card is " + cardName(this->currentCard) << endl;
        sf::Packet packet;
        packet << Type::notification << "Deck Card is " + cardName(this->currentCard);
        sendAll(packet);
        while (true) {
            for (auto Pclient = 0; Pclient < clients.size(); Pclient++) {
                packet.clear();
                sf::TcpSocket& client = *clients[Pclient];
                cout << "Checking: " << client.getRemoteAddress() << ":" << client.getRemotePort() << endl;
                packet << Type::notification << "Now turn is " + clientsNames[Pclient];
                sendAll(packet);
                packet << Status::turn;

                if (sendPacket(packet, client)) {
                    cout << "Turn: " << client.getRemoteAddress() << endl;
                }
                else {
                    cout << "get out " << client.getRemoteAddress();
                }

                cout << "Wait card" << endl;

                if (client.receive(packet) == sf::Socket::Done) {
                    int type;
                    packet >> type;
                    if (type == Type::accusation) {
                        bool lie;
                        int index;
                        packet >> lie >> index;
                        cout << "Accusation received: " << (lie ? "True" : "False") << endl;
                        sf::Packet accusation;
                        string wasindeck;

                        for (int i = 0; i < lastQuanityCards; i++) {
                            cout << cardName(currentDeck.top()) << " | " << endl;
                            if (currentDeck.top() != currentCard and !lie) {
                                lie = true;
                                cout << clients[index]->getRemoteAddress() << ":" << clients[index]->getRemotePort() << " is Lied!";
                            }
                            wasindeck += " | " + currentDeck.top();
                            currentDeck.pop();
                        }

                        packet << Type::notification << "In deck was: " + wasindeck;
                        sendAll(packet);
                        sendAccusation(lie, client, *clients[index], index);

                    }
                    else if (type == Type::place) {
                        packet >> lastQuanityCards;
                        cout << lastQuanityCards;
                        cout << "Received cards: ";
                        int card;
                        for (int i = 0; i < lastQuanityCards; i++) {
                            packet >> card;
                            cout << cardName(card) << " | ";
                            currentDeck.push(card);
                        }
                        packet << Type::notification << clientsNames[Pclient] + "is place " + to_string(lastQuanityCards) + cardName(currentCard);
                        sendAll(packet);
                        cout << endl;
                    }
                    else {
                        cout << "Error: Failed to receive data." << endl;
                    }
                }
                else {
                    cout << "Error: player unreacheble" << endl;
                }
            }

        }
    }


};