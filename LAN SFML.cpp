#include <iostream>
#include <vector>
#include <SFML/Network.hpp>
using namespace std;

// Содержит JACK, QUEEN, KING, ACE, JOCKER
enum suite { JACK, QUEEN, KING, ACE, JOCKER };

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
    int sendData(string data, sf::TcpSocket& socket) {
        if (data.empty()) { return -1; }
        if (socket.send(data.c_str(), data.length()) == sf::Socket::Done) {
            cout << "Data sent: " << data << endl;
            return 0;
        }
        return -2;
    }
};

// Пример класса Server
class Server : public Player {
public:
    Server(string name, int uid) : Player(name, uid) {}

    void startServer() {
        sf::TcpListener listener;
        if (listener.listen(53000) != sf::Socket::Done) {
            cout << "Error: Server failed to start." << endl;
            return;
        }
        cout << "Server is listening on port 53000" << endl;

        // Принятие входящего соединения
        sf::TcpSocket client;
        if (listener.accept(client) != sf::Socket::Done) {
            cout << "Error: Failed to accept client." << endl;
            return;
        }

        cout << "Client connected." << endl;

        // Чтение данных от клиента
        char buffer[128];
        std::size_t received;
        if (client.receive(buffer, sizeof(buffer), received) == sf::Socket::Done) {
            cout << "Received data: " << string(buffer, received) << endl;
        }
    }
};

int main() {
    int u;
    cin >> u;
    if (u == 1) {
        sf::TcpSocket socket;
        Player p("PlayerName", 123);
        p.connectServer(sf::IpAddress::getLocalAddress(), 53000, socket);
        // Пример отправки данных
        string data = "|123|1|0|0|0|0|0|";
        int result = p.sendData(data, socket);
        if (result == 0) {
            cout << "Data sent successfully." << endl;
        }
        else {
            cout << "Error sending data." << endl;
        }

        // Пример подключения к серверу
        
    }
    else {
        // Пример создания сервера
        Server s("Server", 1);
        s.startServer();
    }
    cin >> u;
    return 0;
}
