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
    Data() : type{ -1 } {}
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
    d.clear();
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
    sf::TcpSocket socket = sf::TcpSocket();

protected:
    string name;
    enum Status { play = 0, turn, waiting };
    int status = Status::waiting;
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
    Player(string name, int uid ) : name{ name }, uid{ uid } {
        ip = sf::IpAddress::getLocalAddress(); // Получаем локальный IP

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
    
    // Подключение к серверу
    void connectServer(sf::IpAddress ip, unsigned short port) {
        if (socket.connect(ip, port) != sf::Socket::Done)
            return;
        cout << "Connected to server " << ip << endl;
        sf::Packet packet;

        if (socket.receive(packet) == sf::Socket::Done) {
            packet >> this->status;
        }
        if (this->status == Status::play) {
            if (socket.receive(packet) == sf::Socket::Done) {
                packet >> cards;
            }
            startGame();
        }
    }
    
    // Отправка данных на сервер
    int sendData(Data data) {
        sf::Packet p = data.get();
        if (socket.send(p) == sf::Socket::Done) {
            cout << "Data sent to server" << endl;
        }
        else {
            cout << "Error sending data to server" << endl;
        }

        return -2;
    }
    void startGame() {
        int action;
        set<int> cardUses;
        Data data;
        coutCards();
        if (status == Status::turn) {
            cout << "Your Action: 1.put cards 2. Say accusation";
            cin >> action;
            switch (action)
            {
            case 1:
                while (cardUses.size() != 6) {
                    cout << "Enter index cards(7 for exit): ";
                    cin >> action;
                    if (action == 7) break;
                    if (cardUses.count(action)) cout << "Error: You alread enter it!" << endl;
                    else {
                        cardUses.insert(action);
                    }
                }
                data.type = Type::place;
                
                for (int card : cardUses) {
                    data.cards.push_back(this->putCard(card));
                }
                sendData(data);
               
            default:
                break;
            }
        }
    };


};


// Пример класса Server
class Server : public Player {
private:

    vector<Player*> players;
    vector<sf::TcpSocket* > clients;

    unsigned short int port = 53000;

    sf::SocketSelector selector;
    sf::TcpListener listener;
    

    vector<int> deck;
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
    Server(string name, int uid) : Player(name, uid) {
        listener.listen(port);
        selector.add(listener);
        listener.setBlocking(true);
        
    }
    void startServer() {

        
        cout << "Server is listening on port " << port << endl;
        while (true) {
            if (selector.wait(sf::milliseconds(100))) {
                cout << "Selector is ready" << endl;
                if (selector.isReady(listener)) {
                    cout << "New connection detected" << endl;
                    sf::TcpSocket* client = new sf::TcpSocket;
                    if (listener.accept(*client) == sf::Socket::Done) {
                        clients.push_back(client);
                        selector.add(*client);
                        cout << "Received new connection: " << client->getRemoteAddress() << endl;
                    }
                    else {
                        delete client;
                    }
                }
                else {
                    
                    for (auto Pclient : clients) {
                        sf::TcpSocket& client = *Pclient;
                        if (selector.isReady(client)) {
                            sf::Packet packet;
                            if (client.receive(packet) == sf::Socket::Done) {
                                int type;
                                packet >> type;
                                if (type == Type::accusation) {
                                    bool lie;
                                    packet >> lie;
                                    cout << "Accusation received: " << (lie ? "True" : "False") << endl;
                                }
                                else if (type == Type::place) {
                                    int quantity;
                                    packet >> quantity;

                                    vector<int> receivedDeck;
                                    packet >> receivedDeck;

                                    cout << "Received cards: ";
                                    for (int card : receivedDeck) {
                                        appendDeck(card);
                                        cout << cardName(card) << " | ";  // Выводим карту
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
            }
            
        }
    }
    
};



int main() {
    
    int u;
    cout << "Enter mode (1 - Client 2 - Server): ";
    cin >> u;
    if (u == gameMode::CLIENT) {
        Player p("PlayerName", 123);
        p.connectServer(sf::IpAddress::getLocalAddress(), 53000);
        vector<int> cards{ 1,2,3 };
        Data data(Type::place, cards);
        while (true) {
            sf::sleep(sf::seconds(1));
            p.sendData(data);
        }
    }
    else if (u == gameMode::SERVER) {
        Server s("Server", 1);
        s.startServer();
    }
    cin >> u;
    return 0;
}

