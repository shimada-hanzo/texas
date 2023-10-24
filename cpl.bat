g++ game.cpp card.cpp compare.cpp server.cpp -lws2_32 -o server
g++ client.cpp -lws2_32 -o client