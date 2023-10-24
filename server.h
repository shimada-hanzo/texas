#include <WinSock2.h>
#include <map>
#include <iostream>
#include <vector>
#include <thread>

extern std::vector <SOCKET> clientSocket;

extern std::map <int, std::string> username;
extern int players;

extern WSADATA data;
extern SOCKET sock;
extern sockaddr_in addr;

int mySend(int idx, std::string message);
int broadcast(std::string message);
int broadcast_except(std::string message, int idx);
int myReceive(int idx, std::string& buf);
// int myReceiveWithName(int idx, std::string& buf);
void getUsername();
int networkInit();