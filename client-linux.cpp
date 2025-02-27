#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <map>

typedef websocketpp::client<websocketpp::config::asio_client> client;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

// WebSocket Client
client c;

typedef std::vector<uint8_t> Buffer;

// Utils
std::string hslToANSI(short hue) {
    const short S = 1;
    const short L = 0.5;
    const short C = (1 - abs(2 * L - 1)) * S;
    const short X = C * (1 - abs(((hue / 60) % 2) - 1));
    const short m = L - C / 2;

    short rPrime, gPrime, bPrime;

    if (hue >= 0 && hue < 60) {
        rPrime = C;
        gPrime = X;
        bPrime = 0;
    } else if (hue >= 60 && hue < 120) {
        rPrime = X;
        gPrime = C;
        bPrime = 0;
    } else if (hue >= 120 && hue < 180) {
        rPrime = 0;
        gPrime = C;
        bPrime = X;
    } else if (hue >= 180 && hue < 240) {
        rPrime = 0;
        gPrime = X;
        bPrime = C;
    } else if (hue >= 240 && hue < 300) {
        rPrime = X;
        gPrime = 0;
        bPrime = C;
    } else {
        rPrime = C;
        gPrime = 0;
        bPrime = X;
    }

    const short r = round((rPrime + m) * 255);
    const short g = round((gPrime + m) * 255);
    const short b = round((bPrime + m) * 255);

    return "\\[38;2;"+r+";"+g+";"+b+"m";
}

void printColouredText(std::string text, uint16_t hue) {
    const std::string ansi_colour_text = hslToANSI((short)hue);

    std::cout << ansi_colour_text << text << "\033[0m"; // hope this works
}

void printNormalText(std::string text) {
    std::cout << text << std::endl;
    handleInput();
}

void handleInput() {
    std::string text;

    text << cin;

    websocketpp::connection_hdl hdl;
    c->send(hdl, text, websocketpp::frame::opcode::text);
}

int decodeSingleMessage(Buffer &buffer, int offset = 1) {
    uint16_t hue;
    uint16_t id;
    
    int nick_length = 0;

    int message_size;

    char[50] message_cstr;

    char[20] nick_cstr;

    // decode the id
    std::memcpy(&id, &buffer[offset], sizeof(uint16_t));
    offset += sizeof(uint16_t);

    // decode the hue
    std::memcpy(&hue, &buffer[offset], sizeof(uint16_t));
    offset += sizeof(uint16_t);

    // decode nick length
    std::memcpy(&nick_length, &buffer[offset], sizeof(int));
    offset += sizeof(nick_length);

    // decode nick
    std::memcpy(&nick_cstr, &buffer[offset], nick_length);
    offset += nick_length;

    // decode message size
    std::memcpy(&message_size, &buffer[offset], sizeof(int));
    offset += sizeof(int);

    // decode message
    std::memcpy(&message_cstr, &buffer[offset], message_size);
    offset += message_size;

    std::string nick(nick_cstr);
    std::string message(message_cstr);

    printColouredText("<" + nick + id + ">", hue);
    printNormalText(message);

    return offset;
}

void decodeEntireInitialMessage(Buffer &buffer) {
    int offset = 1; // Skip opcode

    int sizeOfBuffer = buffer.size();

    while(offset < sizeOfBuffer) {
        offset = decodeSingleMessage(buffer, offset);
    }
}

// Callbacks
void on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
    Buffer buffer(msg->get_payload.begin(), msg->get_payload.end());
    processMessage(buffer);
}

// Network
void processMessage(Buffer &buffer) {
    uint8_t op = buffer[0];

    switch(op) {
        case 3:
            decodeEntireInitialMessage(buffer);
        break;

        case 4:
            decodeSingleMessage(buffer);
        break;

        default:
            std::cout << "Unknown Opcode" << std::endl;
        break;
    }
}


int main(int argc, char* argv[]) {
    std::string uri = "ws://localhost:8081";

    if (argc == 2) {
        uri = argv[1];
    }

    try {
        c.set_access_channels(websocketpp::log::alevel::all);
        c.clear_access_channels(websocketpp::log::alevel::frame_payload);

        c.init_asio();

        c.set_message_handler(bind(&on_message,&c,::_1,::_2));

        websocketpp::lib::error_code ec;
        client::connection_ptr con = c.get_connection(uri, ec);
        if (ec) {
            std::cout << "could not create connection because: " << ec.message() << std::endl;
            return 0;
        }

        c.connect(con);

        c.run();
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    }
}
