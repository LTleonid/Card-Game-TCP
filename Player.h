#pragma once
#include <vector>
#include <set>
#include <iostream>
#include <SFML/Network.hpp>
#include "Client.h"

using namespace std;

class Player : public Client {
private:

    sf::IpAddress ip;
    int uid;
    vector<int> cards;
    bool live;
protected:
    enum Status { ready = 200, turn = 5, waiting = 302 };
    int status = Status::waiting;
    sf::IpAddress getIP() const { return this->ip; }
    int getUID() const { return this->uid; }
    int playerPrev;
    vector<int> getCards() { return cards; }

    int getCard(int index) {
        if (index >= 0 && index < cards.size()) {
            int copy = cards[index];
            cards.erase(cards.begin() + index);
            return copy;
        }
        else {
            cout << "Error: Index out of range in putCard()" << endl;
            return -1;
        }
    }
    vector<int> putCard(set<int> cardIndex) {
        vector<int> cards;
        if (cardIndex.size() > getCards().size()) { cout << "SIZE ERROR" << endl; return vector<int>{}; } // Проверка размера
        int tmp = 0;
        for (int i : cardIndex) {
            cout << i;
            if (i > 5) { cout << "Index Error" << endl; return vector<int>{}; } //Проверка карты
            if (tmp > 0) {
                cards.push_back(getCard(i - 1 * tmp));
            }
            else {
                cards.push_back(getCard(i));
            }
            ++tmp;
        }
        cout << "CARDS: ";
        for (int card : cards) {
            cout << cardName(card) << endl;
        }
        return cards;
    }
    void setCards(vector<int> newCards) {
        this->cards = newCards;
    }


public:
    Player(string name, int uid) : Client(name, uid) {
    }

    int getQuanityCards() { return cards.size(); }


    void coutCards() {
        int i = 0;
        for (int card : cards) {
            cout << i << ". " << cardName(card) << " | ";
            i++;
        }
        cout << endl;
    }

    


    void startGame() {
        if (status == Player::Status::ready) {
            cout << "start Game" << endl;

            sf::Packet packet;
            int type;
            int action;
            set<int> cardUses;
            vector<int> temp;
            // Получение стартовых карт
            packet = reciveData();
            packet >> type;

            if (type == Type::startDeck) {
                packet >> cards;
            }
            else {
                cout << "Error: Don't get Cards" << endl;
            }

            while (true)
            {
                coutCards();
                packet = reciveData();
                packet >> status;
                if (status == Status::turn) {
                    cout << "Your Action: 1.put cards 2. Say accusation: ";
                    cin >> action;
                    switch (action)
                    {
                    case 1:
                        cardUses.clear();
                        temp.clear();
                        while (cardUses.size() != 6) {
                            cout << "\033[2J";
                            cout << "\033[0;0f";
                            for (int i = 0; i < getCards().size(); i++) {
                                cout << (cardUses.count(i) ? "\033[38;5;11m" + to_string(i) + ". " : to_string(i) + ". ") << cardName(getCards()[i]) << "\033[0m | ";
                            }
                            cout << endl;
                            cout << "Enter index cards(7 for exit, re-enter for undo): ";
                            cin >> action;
                            if (action == 7) break;
                            if (cardUses.count(action)) cardUses.erase(action);
                            else {
                                cardUses.insert(action);

                            }
                        }
                        sendData(Data(Type::place, putCard(cardUses)));
                        break;
                    case 2:
                        while (action != 0 && action != 1) {
                            cout << "Enter your Choice: 0 - Lie 1 - True : ";
                            cin >> action;
                        }

                        sendData(Data(Type::accusation, action, playerPrev));
                        packet = reciveData();
                        packet >> type;
                        if (type == Type::notification) {
                            string notif;
                            packet >> notif;
                            cout << "Server: " << notif << endl;
                        }
                        else if (type == Type::Shoot) {
                            packet >> live;
                        }
                        break;

                    default:
                        break;
                    }
                }
                else if (status == Status::waiting) {
                    cout << "Waiting turn" << endl;
                }
                else if (status == Type::notification) {
                    string notif;
                    packet >> notif;
                    cout << "Server: " << notif << endl; //TODO: Доработать
                }
            }
        }
    }
};
