#include <iostream>
#include <vector>
#include <set>
#include <SFML/Network.hpp>
using namespace std;
//Содержит JACK, QUEEN, KING, ACE, JOCKER
enum suite { JACK, QUEEN, KING, ACE, JOCKER };

// Разименновывание карт по индексу
string cardName(int cardIndex) {
    switch (cardIndex)
    {
    case suite::JACK:
        return "Jack";
        break;
    case suite::QUEEN:
        return "Queen";
        break;
    case suite::KING:
        return "King";
        break;
    case suite::ACE:
        return "Ace";
        break;
    case suite::JOCKER:
        return "Jocker";
        break;
    default:
        return "F";
        break;
    }
}
class Player{
private:
    sf::IpAddress ip;
    

protected:
    string name;
    vector<int> cards;
    vector<int> getCards() { return cards; }

    int putCard(int index) {
        if (index >= 0 && index < cards.size()) {
            int copy = cards[index];
            cards.erase(cards.begin() + index);
            return copy;
        }
        else {
            cout << "Error: Index out of range in getCard()" << endl;
            return -1;

        }
    }

    void setCards(vector<int> cards) {
        this->cards = cards;
    }

public:
    Player(string name) : name{ name } {
        ip = sf::IpAddress::getLocalAddress();
    }

    int getQuanityCards() { return cards.size(); }
    void coutCards() {
        for (int card : cards) {
            cout << cardName(card) << " | ";
        }
        cout << endl;
    }

    void connectServer(sf::IpAddress ip, unsigned short port, sf::TcpSocket& socket) {
        if (socket.connect(ip, port) != sf::Socket::Done)
            return;
        std::cout << "Connected to server " << ip << std::endl;

    }
    int sendData(string data, sf::TcpSocket& socket){ //  | 0:Обвинение 1:Выдача карты | 0:Правда 1:Ложь | карта | карта | карта | карта | карта | конец |
        if (data.length() != 8) { return -1; }
        if (socket.send(data.c_str(), 8) == sf::Socket::Done) { return 0; }
        return -2;
    }
   
};
class Server : private Player {



};

int main()
{
    int test = 12345;
    cout << to_string(test).length();
    return 0;
}