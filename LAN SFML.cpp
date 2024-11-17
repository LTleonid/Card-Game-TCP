#include <iostream>
#include <vector>
#include <set>
#include <SFML/Network.hpp>
#include <random>
#include <stack>
using namespace std;

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

// Содержит JACK, QUEEN, KING, ACE, JOCKER
enum suite { JACK, QUEEN, KING, ACE, JOCKER };
// Положил карту | Обвинения
enum Type { place = 0, accusation = 1, startDeck = 3 };
enum gameMode { SERVER = 2, CLIENT = 1 };
struct Data {
    int type;
    bool accusation;
    vector<int> cards;
    set<int> cardsUses;
    Data() : type{ -1 } {}
    Data(int type, bool lie) : type{ type }, accusation{ lie } {}
    Data(int type, vector<int> cards) : type{ type }, cards{ cards } {}
    sf::Packet get() {
        sf::Packet p;
        if (type == Type::accusation) {
            p << type << accusation;
        }
        else if (type == Type::place ) {
            p << type << cards.size() << cards;
        }
        else if (type == Type::startDeck) {
            p << type << cards;
        }
        else {
            cout << "Error: Undefined Type Data" << endl;
            return sf::Packet();
        }
        return p;
    }

};




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
    enum Status { ready = 0, turn, waiting};
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
    Player(string name, int uid) : name{ name }, uid{ uid } {
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
            packet >> status;
        }
    }

    // Отправка данных на сервер
    int sendData(Data data) {
        sf::Packet p = data.get();
        if (socket.send(p) == sf::Socket::Done) {
            cout << "Data sent to server" << endl;
            return 0;
        }
        else {
            cout << "Error sending data to server" << endl;
            return -2;
        }
        
        
    }

    sf::Packet reciveData() {
        sf::Packet p;
        if (socket.receive(p) == sf::Socket::Done) {
            return p;
        }
        else {
            cout << "Error: Failure recive data" << endl;
            return sf::Packet();
        }
    }

    void startGame() {
        cout << "start Game" << endl;
        
        int action;
        int type;
        set<int> cardUses;
        vector<int> temp;

        Data data;
        sf::Packet packet;

        packet = reciveData();
        packet >> type;

        if (type == Type::startDeck) {
            packet >> cards;
            cout << "Get cards: ";
            coutCards();
        }
        else {
            cout << "Error: Don't get Cards" << endl;
        }
        while (true)
        {     
            coutCards();
            packet = reciveData();
            packet >> status;
            cout << status << endl;
            if (status == Status::turn) {
                cout << "Your Action: 1.put cards 2. Say accusation";
                cin >> action;
                switch (action)
                {
                case 1:

                    while (cardUses.size() != 6) {
                        cout << "\033[2J";
                        for (int i = 0; i < getCards().size(); i++){
                            cout << (cardUses.count(i)? "\033[48;0;255;255m" : "") << cardName(getCards()[i]) << " | ";
                        }
                        cout << endl;
                        cout << "Enter index cards(7 for exit): ";
                        cin >> action;
                        if (action == 7) break;
                        if (cardUses.count(action)) cout << "Error: You alread enter it!" << endl;
                        else {
                            cardUses.insert(action);
                        }
                    }
                    
                    for (int card : cardUses) {
                        temp.push_back(putCard(card));
                    }
                    sendData(Data(Type::place, temp));
                    break;
                default:
                    break;
                }
            }
            else if (status == Status::waiting) {
                cout << "Waiting turn" << endl;
            }
        }
    }


};


class Server : public Player {
private:

    vector<sf::TcpSocket* > clients{};
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
    int quantityCards;

public:
    unsigned short int getPort() {
        return port;
    }
    Server(string name, int uid, int maxPlayer) : Player(name, uid), maxPlayer{ maxPlayer } , jCards { 6 }, qCards{ 6 }, kCards{ 6 }, aCards{ 6 }, JCards{ 3 } {
        listener.listen(port);
        selector.add(listener);
        listener.setBlocking(true);
        
    }

    void Ready() {
        for (auto player : clients) {
            sf::TcpSocket& Splayer = *player;
            if (Splayer.getRemoteAddress() != sf::IpAddress::None) { // Проверяем, что сокет активен
                sf::Packet packet;
                int status = Player::Status::ready;
                packet << status;

                if (Splayer.send(packet) == sf::Socket::Done) { // Отправляем пакет
                    cout << "Player " << Splayer.getRemoteAddress() << " is ready!" << endl;
                }
                else {
                    cout << "Error: Failed to send ready" << Splayer.getRemoteAddress() << endl;
                }
                giveCards(Splayer);
                
            }
            else {
                cout << "Error: Socket is disconnected or unavailable for player." << endl;
            }
        }
    }
    
    void giveCards(sf::TcpSocket& player) {
        if (InitializationDeck()) {
            sf::Packet packet;
            packet << Type::startDeck;
            if (player.send(packet) == sf::Socket::Done) {
                packet.clear();
                vector<int> cards;
                for (int i = 0; i < 6; i++ ) cards.push_back( getCardfromDeck());
                packet << cards;
                if (player.send(packet) == sf::Socket::Done) {
                    cout << "Player " << player.getRemoteAddress() << "Get: ";
                    for (int card : cards) {
                        cout << cardName(card) << endl;
                    }
                }
                
            }
        }
    }

    int getCardfromDeck() {
        int copy = deck[0];
        cout << "Get " << cardName(copy) << endl;
        this->deck.erase(deck.begin());
        cout << "now on zero " << cardName(deck[0]) << endl;
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
            for (int card : deck) {
                cout << cardName(card) << endl;
            }
        }
        return 1;
    }

    void startServer() {
        cout << "Server is listening on port " << port << endl;
        while (true) {
            if (clients.size() != maxPlayer) {
                cout << clients.size();
                if (selector.wait(sf::milliseconds(100))) {

                    if (selector.isReady(listener)) {
                        cout << "New connection detected" << endl;
                        sf::TcpSocket* client = new sf::TcpSocket;
                        if (listener.accept(*client) == sf::Socket::Done) {
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
        Ready();
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
                            cout << cardName(card) << " | ";
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
        Player p("PlayerName", 123);
        p.connectServer(sf::IpAddress::getLocalAddress(), 53000);
        p.startGame();

    }
    else if (u == gameMode::SERVER) {
        Server s("Server", 1, 1);
        s.startServer();
    }
    cin >> u;
    return 0;
}
