#include <iostream>
#include <vector>
#include <set>
#include <SFML/Network.hpp>
#include <random>
#include <stack>
#include <string>
using namespace std;

sf::Packet& operator <<(sf::Packet& packet, const vector<int>& d) {
    cout << "PACKET sen: " << (int)d.size() << endl;
    packet << (int)d.size(); // Почему то size не даёт int по умолчанию
    for (int value : d) {
        cout << "PACKET sen: " << value << endl; // Где то тут пробелма
        packet << value; // Затем сами элементы
    }
    return packet;
}

sf::Packet& operator >>(sf::Packet& packet, vector<int>& d) {
    int size;
    packet >> size; // Сначала считываем размер
    cout << "PACKET rec: " << size << endl;
    d.clear();
    for (int i = 0; i < size; ++i) {
        int value;
        packet >> value;
        cout << "PACKET rec: " << value << endl;
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
            p << type << cards;
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
    enum Status { ready = 200, turn = 5, waiting = 302 };
    int status = Status::waiting;
    sf::IpAddress getIP() const { return this->ip; }
    int getUID() const { return this->uid; }
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
        if (status == Player::Status::ready) {
            cout << "start Game" << endl;

            sf::Packet packet;
            int type;
            int action;
            set<int> cardUses;
            vector<int> temp;
            Data data;
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
                cout << status << endl;
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
                                cout << (cardUses.count(i) ? "\033[38;5;11m"+to_string(i) + ". " : to_string(i)+". ") << cardName(getCards()[i]) << "\033[0m | ";
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
                        
                        data.type = Type::place;
                        data.cards = putCard(cardUses);
                        sendData(data);
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
    bool sendPacket(sf::Packet& packet, sf::TcpSocket& Rx) {
        cout << "Send Packet " << packet.getData() << " to " << Rx.getRemoteAddress() << ":" << Rx.getRemotePort() << endl;
        if (Rx.send(packet) == sf::Socket::Done) {
            cout << "Success!" << endl;
            packet.clear();
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
        for (auto player : clients) {
            sf::TcpSocket& Splayer = *player;
            // Проверяем, что сокет активен
            sf::Packet packet;
            int status = Player::Status::ready;
            packet << status;

            if (sendPacket(packet, Splayer)) { // Отправляем пакет
                cout << "Player " << Splayer.getRemoteAddress() << " is ready!" << endl;
            }
            else {
                cout << "Error: Failed to send ready" << Splayer.getRemoteAddress() << endl;
            }
            
            giveCards(Splayer);

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
                cout << "Checking: " << client.getRemoteAddress() << ":" << client.getRemotePort() << endl;
                sf::Packet packet;
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
                        packet >> lie;
                        cout << "Accusation received: " << (lie ? "True" : "False") << endl;
                    }
                    else if (type == Type::place) {
                        int quantity;
                        packet >> quantity;
                        cout << quantity;
                        vector<int> receivedDeck;
                        packet >> receivedDeck;
                        cout << "Received cards: ";
                        for (int card : receivedDeck) {
                            cout << cardName(card) << " | ";
                            appendDeck(card);
                            
                        }
                        cout << endl;
                    }
                    else {
                        cout << "Error: Failed to receive data." << endl;
                    }
                }
                else {
                    cout << "Error: Selector unready" << endl;
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
