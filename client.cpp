#include <WinSock2.h>
#include <iostream>
#include <string>

using namespace std;

SOCKET sock;
string s, message;
int port;

int mySend(string message) {
	int ret = send(sock, message.c_str(), strlen(message.c_str()) + 1, 0);
	if (ret == -1) {
		cout << "发送信息失败" << endl;
		WSACleanup();
		return -1;
	}
	return 0;
}

int myReceive(string& buf) {
	char recvBuf[0xFF];
	int ret = recv(sock, recvBuf, sizeof(recvBuf), 0);
	if (ret <= 0) {
		WSACleanup();
		cout << "接收服务器数据失败" << endl;
		return -1;
	}
    buf = recvBuf;
	// cout << "server > " << buf << endl;
	cout << buf << endl;
	Sleep(100);
	return 0;
}

int main() {
    // 确定网络协议版本
	WSADATA data;
	int ret = WSAStartup(MAKEWORD(2, 2), &data);
	if (ret) {
		cout << "初始化网络错误" << endl;
		WSACleanup();
		return -1;
	}

    // 创建socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		cout << "创建套接字失败" << endl;
		WSACleanup();
		return -1;
	}

    // 确定服务器协议地址簇
	sockaddr_in addr;
	addr.sin_family = AF_INET;	
	cout << "请输入主机端口号:";
	// cin >> port;
	getline(cin, s);
	port = stoi(s);
	addr.sin_port = htons(port);
	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

    // 连接服务器
	ret = connect(sock, (sockaddr*)&addr, sizeof(addr));
	if (ret == -1) {
		WSACleanup();
		cout << "连接服务器失败" << endl;
		return -1;
	}

	// 接收等待信息
	myReceive(message);
    // 发送用户名
	myReceive(message);
	cout << "you > ";
	getline(cin, s);
	mySend(s);

	while (message != "quit") {
		int ret = myReceive(message);
		if (ret == -1) {
			WSACleanup();
			cout << "接收服务器数据失败" << endl;
			return 0;
		}
		if (message.length() >= 6 && message.substr(0, 6) == "选择") {
			cout << "you > ";
			string input;
    		getline(cin, input);
			mySend(input);
		}
	}

	// 清理协议版本信息
	WSACleanup();
	return 0;
}