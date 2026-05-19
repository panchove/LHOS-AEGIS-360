#include <cstdio>
#include <cstring>
#include "crc16.hpp"
#include "parser.hpp"
#include "pool.hpp"

extern "C" void app_main()
{
    // Simple smoke tests executed on startup
    // CRC test vector "123456789" -> 0x29B1
    const char* v = "123456789";
    uint16_t c = aegis::crc16::ccitt(reinterpret_cast<const uint8_t*>(v), strlen(v));
    printf("CRC test: 0x%04X (expect 0x29B1)\n", c);

    // Packet parser example
    aegis::PacketParser parser;
    parser.init([](const uint8_t* data, size_t len){
        printf("Parsed packet (%zu): %.*s\n", len, (int)len, (const char*)data);
    }, 1000);

    const char* msg = "<Pb>Hello</Pb>";
    parser.feed_bytes((const uint8_t*)msg, strlen(msg));

    // PacketPool example
    aegis::PacketPool pool;
    pool.init(nullptr, 16, 512);
    auto b = pool.alloc();
    if (b) {
        printf("Allocated buffer\n");
        pool.release(b);
    }
}
