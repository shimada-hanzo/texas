#include "server.h"

std::vector <SOCKET> clientSocket;

std::map <int, std::string> username;
int players;

WSADATA data;
SOCKET sock;
sockaddr_in addr;
int port;

int mySend(int idx, std::string message) {
	int ret = send(clientSocket[idx], message.c_str(), strlen(message.c_str()) + 1, 0);
	if (ret == -1) {
		std::cout << "发送信息失败" << std::endl;
		WSACleanup();
		return -1;
	}
    Sleep(100);
	return 0;
}

int broadcast(std::string message) {
    // std::cout << "broadcast:" << std::endl;
    // std::cout << message << std::endl;
    for (int i = 0; i < players; i ++) {
        // std:: cout << i << std::endl;
        int ret = send(clientSocket[i], message.c_str(), strlen(message.c_str()) + 1, 0);
        if (ret == -1) {
            std::cout << "发送信息失败" << std::endl;
            WSACleanup();
            return -1;
        }
    }
    // std::cout << "broadcast over." << std::endl;
    Sleep(100);
    return 0;
}

int broadcast_except(std::string message, int idx) {
    for (int i = 0; i < players; i ++) {
		if (i == idx) {
			continue;
		}
        int ret = send(clientSocket[i], message.c_str(), strlen(message.c_str()) + 1, 0);
        if (ret == -1) {
            std::cout << "发送信息失败" << std::endl;
            WSACleanup();
            return -1;
        }
    }
    Sleep(100);
    return 0;
}

int myReceive(int idx, std::string& buf) {
	char recvBuf[0xFF];
	int ret = recv(clientSocket[idx], recvBuf, 0xFF, 0);
	buf = recvBuf;
    if (ret <= 0) {
		WSACleanup();
		std::cout << "接收客户端数据失败" << std::endl;
		return -1;
	}
    std::cout << "客户端" << idx << ": " << buf << std::endl;
	// if (ret > 0) {
	// 	std::cout << "客户端" << idx << ": " << buf << std::endl;
	// 	return -1;
	// }
	return 0;
}

int myReceiveWithName(int idx, std::string& buf) {
	char recvBuf[0xFF];
	int ret = recv(clientSocket[idx], recvBuf, 0xFF, 0);
	buf = recvBuf;
	std::cout << username[idx] << ": " << buf << std::endl;
	if (ret <= 0) {
		std::cout << "接受客户端数据失败" << std::endl;
		WSACleanup();
		return -1;
	}
	return 0;
}

int networkInit() {
    std::cout << "请输入玩家人数: ";
	std::cin >> players;
	for (int i = 0; i < players; i ++) {
		SOCKET s;
		clientSocket.push_back(s);
	}

	// 确定网络协议版本
	int ret = WSAStartup(MAKEWORD(2,2), &data);
	if (ret) {
		std::cout << "初始化网络错误！" << std::endl;
		WSACleanup();
		return -1;
	}

	// 创建socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		std::cout << "创建套接字失败" << std::endl;
		WSACleanup();
		return -1;
	}

	// 确定服务器协议地址簇
	addr.sin_family = AF_INET;
	std::cout << "请输入主机端口号：";
	std::cin >> port;
	addr.sin_port = htons(port);
	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

	// 绑定
	ret = bind(sock, (sockaddr*)& addr, sizeof(addr));
	if (ret == -1) {
		std::cout << "绑定地址端口失败" << std::endl;
		WSACleanup();
		return -1;
	}

	// 监听
	ret = listen(sock, 5);
	if (ret == -1) {
		std::cout << "监听套接字失败" << std::endl;
		WSACleanup();
		return -1;
	}

	// 接受客户端连接
	for (int i = 0; i < players; i ++) {
		sockaddr_in addrCli; 
		int len = sizeof(addrCli);
		clientSocket[i] = accept(sock, (sockaddr*)& addrCli, &len);
		std::string ip = inet_ntoa(addrCli.sin_addr);
		std::cout << "client ip: " << ip << std::endl;
		unsigned short cport = ntohs(addrCli.sin_port);//把网络上转换成本地的，linux其实不需要
		std::cout << "client port:" << cport << std::endl;

		if (clientSocket[i] == -1) {
			std::cout << "接收客户端" << i << "连接失败" << std::endl;
			WSACleanup();
			return -1;
		}

        std::string m = "等待其他玩家连接中...";
        mySend(i, m);
	}
    return 0;
}

void setUsername(int idx) {
	mySend(idx, "请输入用户名");
	std::string name;
	// while (1) {
	// 	int ret = myReceive(idx, name);
	// 	if (ret != 0) {
	// 		username[idx] = name;
	// 		break;
	// 	}
	// }
	myReceive(idx, name);
	username[idx] = name;
	mySend(idx, "等待其他玩家输入用户名...");
}

void getUsername() {
	// 并发获取用户名
	std::vector<std::thread> threads;
	for (int i = 0; i < players; i ++) {
        threads.push_back(std::thread(setUsername, i));
    }
	for (int i = 0; i < players; i ++) {
        threads[i].join();
    }
	
	for (int i = 0; i < players; i ++) {
		std::string s = "您的昵称为" + username[i];
		mySend(i, s);
	}
}

// int main() {
// 	// 并发获取用户名
// 	std::vector<std::thread> threads;
// 	for (int i = 0; i < players; i ++) {
//         threads.push_back(std::thread(setUsername, i));
//     }
// 	for (int i = 0; i < players; i ++) {
//         threads[i].join();
//     }
	
// 	for (int i = 0; i < players; i ++) {
// 		std::string s = "你的昵称为" + username[i];
// 		mySend(i, s);
// 	}

// 	// // 通信
// 	// string buf = "我是服务器";
// 	// ret = send(sockCli, buf.c_str(), strlen(buf.c_str()) + 1, 0);
// 	// if (ret == -1) {
// 	// 	cout << "发送信息失败" << endl;
// 	// 	WSACleanup();
// 	// 	return -1;
// 	// }
// 	// char recvBuf[0xFF];
// 	// ret = recv(sockCli, recvBuf, 0xFF, 0);
// 	// buf = recvBuf;
// 	// cout << "客户端：" << buf << endl;
// 	// if (ret <= 0) {
// 	// 	cout << "接受客户端数据失败" << endl;
// 	// 	WSACleanup();
// 	// 	return -1;
// 	// }

// 	// 清理协议版本信息
// 	WSACleanup();
// }