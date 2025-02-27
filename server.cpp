#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <map>

#include <websocketpp/server.hpp>

#include <websocketpp/config/asio_no_tls.hpp>

typedef websocketpp::connection_hdl connection_hdl;
typedef websocketpp::server<websocketpp::config::asio> server;
typedef server::message_ptr message_ptr;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// WebSocket Server
server ws_server;

typedef std::vector<uint8_t> Buffer;

struct Client_Handle {
    uint16_t id;
    uint16_t hue;
    std::string nick;
    std::vector<std::string> msgs;
    connection_hdl hdl;
}

struct Message {
    Client_Handle sender;
    std::string content;
}

std::map<connection_hdl, Client_Handle> client_handles;

std::vector<Message> messages;

// some pointers
uint16_t id_ptr0;

// Callbacks
void on_open(connection_hdl hdl) {
    std::cout << "New client connected" << std::endl;

    Client_Handle client_handle;

    client_handle.hdl = hdl;

    client_handle.id = id_ptr0++;

    client_handle.nick = "Anonymous";

    client_handle.hue = 120; // green

    client_handles[hdl] = client_handle;

    sendInitialUpdate(client_handle);
}

void on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg_ptr) {
    std::cout << "on_message called with hdl: " << hdl.lock().get()
              << " and message: " << msg_ptr->get_payload()
              << std::endl;

    try {
        Client_Handle client_handle = client_handles[hdl];

        Message msg;

        msg.sender = client_handle;

        msg.content = msg_ptr->get_payload();

        messages.push_back(msg);

        sendUpdate();
    } catch (websocketpp::exception const & e) {
        std::cout << "Send failed because: "
                  << "(" << e.what() << ")" << std::endl;
    }
}

//  Network
void sendUpdate(Message &msg) {
    Buffer buffer(100);

    createNewMessage(buffer, msg);

    for (Client_Handle client_handle : client_handles) {
        try {
            server->send(client_handle.hdl, &buffer[0], buffer.size(), websocketpp::frame::opcode::binary);
        } catch(websocketpp::exception const & e) {
            std::cout << "Send failed" << std::endl;
        }
    }
}

void sendInitialUpdate(Client_Handle &client_handle) {
    Buffer buffer(1000);

    createInitialMessage(buffer);

    try {
        server->send(client_handle.hdl, &buffer[0], buffer.size(), websocketpp::frame::opcode::binary);
    } catch(websocketpp::exception const & e) {
        std::cout << "Send failed" << std::endl;
    }
}

void createNewMessage(Buffer &buffer, Message &msg) {
    buffer[0] = 4;

    int offset = 1;

    Client_Handle client_handle = msg.sender;

    std::memcpy(&buffer[offset], &client_handle.id, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    std::memcpy(&buffer[offset], &client_handle.hue, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    const int nick_length = std::strlen(client_handle.nick);
    std::memcpy(&buffer[offset], &nick_length, sizeof(nick_length));
    offset += sizeof(nick_length);

    std::memcpy(&buffer[offset], client_handle.nick, nick_length);
    offset += nick_length;

    const int message_size = std::strlen(msg.content);
    std::memcpy(&buffer[offset], &message_size, sizeof(message_size));
    offset += sizeof(message_size);

    std::memcpy(&buffer[offset], msg.content, message_size);
}

void createInitialMessage(Buffer &buffer) {
    buffer[0] = 3;

    int offset = 1;

    for (const Message& msg : messages) {
        Client_Handle client_handle = msg.sender;

        // encode the uint16 id
        std::memcpy(&buffer[offset], &client_handle.id, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        // encode the uint16 hue
        std::memcpy(&buffer[offset], &client_handle.hue, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        // encode nick length
        const int nick_length = std::strlen(client_handle.nick);
        std::memcpy(&buffer[offset], &nick_length, sizeof(nick_length));
        offset += sizeof(nick_length);

        // encode nick
        std::memcpy(&buffer[offset], client_handle.nick, nick_length);
        offset += nick_length;

        // encode message size
        const int message_size = std::strlen(msg.content);
        std::memcpy(&buffer[offset], &message_size, sizeof(message_size));
        offset += sizeof(message_size);

        // encode the message content
        std::memcpy(&buffer[offset], msg.content, message_size);
        offset += message_size;
    }
}

int main(int argc, char** argv) {
    try {
        ws.set_access_channels(websocketpp::log::alevel::all);
        ws_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

        ws_server.init_asio();

        ws_server.set_message_handler(bind(&on_message,&ws_server,::_1,::_2));

        ws_server.set_open_handler(&on_open);

        ws_server.listen(8081);

        ws_server.start_accept();

        ws_server.run();
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    } catch (...) {
        std::cout << "other exception" << std::endl;
    }
}
