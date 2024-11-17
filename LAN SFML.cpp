#include <iostream>
#include <vector>
#include <set>
#include <SFML/Network.hpp>
#include <random>
#include <stack>
#include <string>
using namespace std;

sf::Packet& operator <<(sf::Packet& packet, const vector<int>& d) {
    packet << static_cast<int>(d.size()); // Сначала записываем размер
    for (int value : d) {
        packet << value; // Затем сами элементы
    }
    return packet;
}

sf::Packet& operator >>(sf::Packet& packet, vector<int>& d) {
    int size;
    packet >> size; // Сначала считываем размер
    d.clear();
    for (int i = 0; i < size; ++i) {
        int value;
        packet >> value;
        d.push_back(value); // Затем считываем элементы
    }
    return packet;
}


// Содержит JACK, QUEEN, KING, ACE, JOCKER
enum suite { JACK, QUEEN, KING, ACE, JOCKER };
// Положил карту | Обвинения
enum Type { place = 12, accusation = 13, startDeck = 500 };
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
        else if (type == Type::place) {
            p << type << cards.size() << cards;
        }
        else if (type == Type::startDeck) {
            p << type << cards.size() << cards;
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
    enum Status { ready = 200, turn = 5, waiting = 302 };
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
    sf::Packet reciveData() {
        sf::Packet packet;
        if (socket.receive(packet) == sf::Socket::Done) {
            return packet;
        }
        else {
            cout << "Error: Failure recive data" << endl;
            return sf::Packet();
        }
    }

    int sendData(Data& data) {
        sf::Packet packet = data.get();
        if (socket.send(packet) == sf::Socket::Done) {
            return 0;
        }
        else {
            cout << "Error sending data to server" << endl;
            return -1;
        }
    }


    void startGame() {
        cout << "start Game" << endl;

        sf::Packet packet;
        int type;

        // Получение стартовых карт
        packet = reciveData();
        packet >> type;

        if (type == Type::startDeck) {
            packet >> cards;
        }
        else {
            cout << "Error: Don't get Cards" << endl;
        }

        while (true) {
            coutCards();
            packet = reciveData();
            if (!packet) {
                cout << "Error: Failed to receive data." << std::endl;
                continue;
            }
            packet >> status;
            cout << "Status: " << status << std::endl;
            if (status == Status::turn) {
                cout << "Your Action: 1.put cards 2. Say accusation";
                int action;
                cin >> action;

                if (action == 1) {
                    vector<int> selectedCards;
                    set<int> usedIndices;

                    while (true) {
                        cout << "Your Action: 1.put cards 2. Say accusation: ";
                        int index;
                        cin >> index;

                        if (index == 7) break;
                        if (usedIndices.count(index)) {
                            cout << "Error: You alread enter it!" << endl;
                        }
                        else if (index >= 0 && index < cards.size()) {
                            usedIndices.insert(index);
                            selectedCards.push_back(cards[index]);
                        }
                        else {
                            cout << "Error: out of Index" << endl;
                        }
                    }

                    // Отправляем данные на сервер
                    Data data(Type::place, selectedCards);
                    sendData(data);

                }
                else {
                    cout << "Error: Unknown Action" << endl;
                }
            }
            else if (status == Status::waiting) {
                std::cout << "Waiting for your turn..." << std::endl;
            }
            else {
                std::cout << "Error: Unknown status received: " << status << std::endl;
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
    Server(string name, int uid, int maxPlayer) : Player(name, uid), maxPlayer{ maxPlayer }, jCards{ 6 }, qCards{ 6 }, kCards{ 6 }, aCards{ 6 }, JCards{ 3 } {
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
                InitializationDeck();
                giveCards(Splayer);

            }
            else {
                cout << "Error: Socket is disconnected or unavailable for player." << endl;
            }
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

        if (player.send(packet) == sf::Socket::Done) {
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
        this->deck.erase(deck.begin());//;
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
        while (true) {
            for (auto Pclient : clients) {
                sf::TcpSocket& client = *Pclient;
                if (selector.isReady(client)) {
                    sf::Packet packet;
                    packet.clear();
                    packet << Status::turn;
                    if (client.send(packet) == sf::Socket::Done) {
                        cout << "Turn: " << client.getRemoteAddress() << endl;
                    }
                    else {
                        cout << "get out " << client.getRemoteAddress();
                    }
                    cout << "Wait card";
                    packet.clear();
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
    }

};

int main(int argc, char** argv) {
    int u;
    if (argc > 1) {
        std::string mode = argv[1];
        if (mode == "S") {
            std::string name = argv[2];
            int maxPlayers = std::stoi(argv[3]);
            Server s(name, rand(), maxPlayers);
            s.startServer();
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
            Player p("PlayerName", 123);
            p.connectServer(sf::IpAddress::getLocalAddress(), 53000);
            p.startGame();
        }
        else if (u == gameMode::SERVER) {
            Server s("Server", 1, 1);
            s.startServer();
        }
    }
}
