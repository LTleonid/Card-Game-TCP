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


// Содержит JACK, QUEEN, KING, ACE, JOCKER
enum suite { JACK, QUEEN, KING, ACE, JOCKER };
// Положил карту | Обвинения
enum Type { place = 12, accusation = 13, startDeck = 500, notification = 250, Shoot = 62 };
enum gameMode { SERVER = 2, CLIENT = 1 };
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




// Разыменование карт по индексу
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
    bool live;
protected:
    string name;
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
        packet << name;
        if (socket.send(packet) != sf::Socket::Done) {
            cout << "Error: Failed to send name to server" << endl;
        }
        packet.clear();
        if (socket.receive(packet) == sf::Socket::Done) {
            packet >> status >> playerPrev;
        }
    }

    // Отправка данных на сервер
    sf::Packet reciveData() {
        sf::Packet packet;
        while(socket.receive(packet) != sf::Socket::Done) {
            
        }
        return packet;
        /*else {
            cout << "Error: Failure recive data" << endl;
            return sf::Packet();
        }*/
    }

    int sendData(Data data) {
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
            // Получение стартовых карт
            packet = reciveData();
            packet >> type;

            if (type == Type::startDeck) {
                packet >> cards;
            }
            else {
                cout << "Error: Don't get Cards" << endl;
            }
            socket.setBlocking(false);

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
                else {
                    cout << "Error: unknown status!" << endl;
                }
            }
        }
    }
};


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
            if(clear) packet.clear();
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
            packet << Type::notification << clientsNames[index+1] + "Is make mistakes!";
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
            packet << Status::waiting;
            for (auto Pclient = 0; Pclient < clients.size(); Pclient++) {
                packet.clear();
                sf::TcpSocket& client = *clients[Pclient];
                for (int i = 0; i < clients.size(); i++) {
                    if (clients[i] == clients[Pclient]) {
                        continue;
                    }
                    else {
                        sendPacket(packet, *clients[i], false);
                    }
                }
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
                        packet <<Type::notification << clientsNames[Pclient] + "is place " + to_string(lastQuanityCards) + cardName(currentCard);
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

int main(int argc, char** argv) {
    srand(time(NULL));
    int u;
    if (argc > 1) {
        string mode = argv[1];

        if (mode == "S") {
            string name;
            int maxPlayers;
            try {
                name = argv[2];
                throw "Error: Not a string!(How)";
            }
            catch (string error) {
                cout << "Error: ";
                return -1;
            }
            try {
                throw "Not a Number";
                maxPlayers = stoi(argv[3]); //Нет блин падай, Чё за названия для string->int
                
            }
            catch (string error) {
                cout << "Error: ";
                return -1;
            }
            try {
                Server s(name, rand(), maxPlayers);
                s.startServer();
            }
            catch (...) {
                cout << "Error: Cannot start server!";
                return -1;
            }
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
            Player p(to_string(rand()), 123);
            p.connectServer(sf::IpAddress::getLocalAddress(), 53000);
            p.startGame();
        }
        else if (u == gameMode::SERVER) {
            Server s("Server", 1, 2); //Name UID maxPlayer
            s.startServer();
        }
    }
    return 0;
}
