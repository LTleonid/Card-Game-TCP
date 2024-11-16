#include <iostream>
#include <vector>
#include <set>
#include <SFML/Network.hpp>
using namespace std;


// Содержит JACK, QUEEN, KING, ACE, JOCKER
enum suite { JACK, QUEEN, KING, ACE, JOCKER };
// Положил карту | Обвинения
enum Type { place = 0, accusation = 1 };
enum gameMode { SERVER = 2, CLIENT = 1 };
struct Data {
    int type;
    bool accusation;
    vector<int> cards;
   
    Data(int type, bool lie) : type{ type }, accusation{ lie } {}
    Data(int type, vector<int> cards) : type{ type }, cards{ cards } {}
    sf::Packet get() {
        sf::Packet p;
        if (type == Type::accusation) {
            p << type << accusation;
        }
        else if (type == Type::place) {
            p << type << cards.size();
            for (int card : cards) {
                cout << card << " ";
                p << card;
            }
            cout << endl;
        }
        else {
            cout << "Error: Undefined Type Data" << endl;
            return sf::Packet();
        }
        return p;
    }

};

sf::Packet& operator <<(sf::Packet& packet, const vector<int>& d) {
    int size = d.size();
    packet << size;
    for (int value : d) {
        packet << value;
    }
    return packet;
}

sf::Packet& operator >>(sf::Packet& packet, vector<int>& d) {
    int size;
    packet >> size;
    for (int i = 0; i < size; ++i) {
        int value;
        packet >> value;
        d.push_back(value);
    }
    return packet;
}


// Разменование карт по индексу
string cardName(int cardIndex) {
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

class Player {
private:
    sf::IpAddress ip;
    int uid;
    vector<int> cards;

protected:
    string name;

    sf::IpAddress getIP() const { return this->ip; }
    int getUID() const { return this->uid; }
    vector<int> getCards() { return cards; }

    int putCard(int index) {
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

    void setCards(vector<int> newCards) {
        this->cards = newCards;
    }

public:
    Player(string name, int uid) : name{ name }, uid{ uid } {
        ip = sf::IpAddress::getLocalAddress(); // Получаем локальный IP
    }

    int getQuanityCards() { return cards.size(); }

    void coutCards() {
        for (int card : cards) {
            cout << cardName(card) << " | ";
        }
        cout << endl;
    }

    // Подключение к серверу
    void connectServer(sf::IpAddress ip, unsigned short port, sf::TcpSocket& socket) {
        if (socket.connect(ip, port) != sf::Socket::Done)
            return;
        cout << "Connected to server " << ip << endl;
    }
    
    // Отправка данных на сервер
    int sendData(Data data, sf::TcpSocket& socket) {
        sf::Packet p = data.get();
        if (socket.send(p) == sf::Socket::Done) {
            cout << "Data sent" << endl;
            return 0;
        }
        return -2;
    }



};


// Пример класса Server
class Server : public Player {
private:
    unsigned short int port = 53000;
    vector<int> deck;
    vector<Player*> players;
    vector<sf::TcpSocket> clients;
    vector<int> getDeck() const {
        return deck;
    }
    void appendDeck(int card) {
        deck.push_back(card);
    }
public:
    unsigned short int getPort() {
        return port;
    }
    Server(string name, int uid) : Player(name, uid) {}
    void startServer() {
        sf::TcpListener listener;
        listener.setBlocking(false);
        if (listener.listen(port) != sf::Socket::Done) {
            cout << "Error: Server failed to start." << endl;
            return;
        }
        cout << "Server is listening on port " << port << endl;

        sf::TcpSocket client;
        
        if (listener.accept(client) != sf::Socket::Done) {
            cout << "Error: Failed to accept client." << endl;
            return;
        }

        cout << "Client connected." << endl;

    
        while (true) {
            if (listener.NotReady) {
                for (int card : getDeck()) {
                    cout << card << endl;
                }
            }
            else{
                sf::Packet packet;
                if (client.receive(packet) == sf::Socket::Done) {
                    int type;
                    packet >> type;
                    if (type == Type::accusation) {
                        bool lie;
                        packet >> lie;
                        cout << "Accusation received: " << (lie ? "True" : "False") << endl;
                    }
                    if (type == Type::place) {
                        int quanity;
                        packet >> quanity;
                        packet >> deck;
                        cout << "Received cards: ";
                        for (int card : deck) {
                            appendDeck(card);
                        }
                        cout << endl;
                    }
                    else {
                        cout << "Error: Failed to receive data." << endl;
                    }
                }
            }
            
        
        }
        
    }
    
};



int main() {
    
    int u;
    cout << "Enter mode (1 - Client 2 - Server): ";
    cin >> u;
    if (u == gameMode::CLIENT) {
        sf::TcpSocket socket;
        Player p("PlayerName", 123);
        p.connectServer(sf::IpAddress::getLocalAddress(), 53000, socket);
        vector<int> cards{ 1,2,3 };
        Data data(Type::place, cards);
        p.sendData(data, socket);
    }
    else if (u == gameMode::SERVER) {
        Server s("Server", 1);
        s.startServer();
    }
    cin >> u;
    return 0;
}

