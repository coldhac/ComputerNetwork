#include <iostream>
#include <sys/socket.h>

void send_all(const int &fd, const void *buffer, size_t len){
	for(size_t ret = 0, b; ret < len; ret += b){
		b = send(fd, (uint8_t*)buffer + ret, len - ret, 0);
		if (b == 0){ std::cerr << "Socket Closed" << std::endl; break; }
		if (b < 0){ std::cerr << "Sending Error." << std::endl; break; }
	}
}

void recv_all(const int &fd, void *buffer, size_t len){
	for(size_t ret = 0, b; ret < len; ret += b){
		b = recv(fd, (uint8_t*)buffer + ret, len - ret, 0);
		if (b == 0){ std::cerr << "Socket Closed" << std::endl; break; }
		if (b < 0){ std::cerr << "Recieving Error." << std::endl; break; }
	}
}
