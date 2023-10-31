#include <regex>
#include <string>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <limit.hpp>
#include <header.hpp>
#include <sockall.hpp>

using namespace std;

const header open_request_header = OPEN_CONN_REQUEST;

void p_help_page(){
	cout << '\n';
	cout << "FTP Client Usage:" << '\n';
	cout << '\n';
	cout << "open <IP> <port>  : Connect to <IP>:<port>" << '\n';
	cout << "get <filename>\t  : Download <filename> file in server directory" << '\n';
	cout << "put <filename>\t  : Upload <filename> file in client directory" << '\n';
	cout << "sha256 <filename> : Check SHA256 value of <filename> file in server directory" << '\n';
	cout << "ls\t\t  : Get the file list in server directory" << '\n';
	cout << "quit\t\t  : Disconnect when connecting, otherwise close the client" << '\n';
	cout << '\n';
}

int input_handler(char *IP, int &port, char *filename){
	string op;
	cin >> op;
	if(op == "open"){
		cin >> IP >> port;
		if(regex_match(IP, regex("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}")) == false)
			return -1;
		return 1;
	}
	if(op == "ls") return 2;
	if(op == "get"){ cin >> filename; return 3; }
	if(op == "put"){ cin >> filename; return 4; }
	if(op == "sha256"){ cin >> filename; return 5; }
	if(op == "quit") return 6;
	return -1;
}

bool init(int &connfd, const char *IP, const int port){
	if((connfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return cerr << "Socket Error." << endl, false;
	struct sockaddr_in addr;
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	inet_pton(AF_INET, IP, &addr.sin_addr);

	if(connect(connfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		return cerr << "Connecting Error." << endl, false;
	return true;
}

bool open(const int &connfd){
	header rec_header;
	send_all(connfd, &open_request_header, HEADER_LENGTH);
	recv_all(connfd, &rec_header, HEADER_LENGTH);

	if(rec_header == OPEN_CONN_REPLY)
		return cout << "Connection Accepted." << endl, true;
	return cout << "Connection Denied." << endl, false;
}

void list(const int &connfd){
	header req_header = LIST_REQUEST, rec_header;
	static char message[MESSAGE_SIZE_LIMIT];
	send_all(connfd, &req_header, HEADER_LENGTH);
	recv_all(connfd, &rec_header, HEADER_LENGTH);

	if(headereq(rec_header, LIST_REPLY, IGNORE_LENGTH)){
		int payload_length = rec_header.get_length() - HEADER_LENGTH;
		recv_all(connfd, message, payload_length);
		cout << message << flush;
	}
	else cerr << "Invalid Reply Header." << endl;
}

void getfile(const int &connfd, const char *filename){
	header req_header = GET_REQUEST, rec_header, dat_header;
	static char payload[FILE_SIZE_LIMIT];
	req_header.set_length(HEADER_LENGTH + strlen(filename) + 1);
	send_all(connfd, &req_header, HEADER_LENGTH);
	send_all(connfd, filename, strlen(filename) + 1);
	recv_all(connfd, &rec_header, HEADER_LENGTH);

	if(headereq(rec_header, GET_REPLY(0), IGNORE_STATUS) == false)
		cerr << "Invalid Reply Header." << endl;
	else if(rec_header.get_status() == false)
		cout << "No such file in server directory." << endl;
	else {
		recv_all(connfd, &dat_header, HEADER_LENGTH);
		if(headereq(dat_header, FILE_DATA, IGNORE_LENGTH)){
			int file_length = dat_header.get_length() - HEADER_LENGTH;
			recv_all(connfd, payload, file_length);

			FILE *fptr = fopen(filename, "w");
			if(fptr == NULL)
				cerr << "File Opening Error." << endl;
			else fwrite(payload, sizeof(uint8_t), file_length, fptr);
			fclose(fptr);
		}
		else cerr << "File Data Error." << endl;
	}
}

void putfile(const int &connfd, const char *filename){
	header req_header = PUT_REQUEST, rec_header, dat_header;
	FILE *fptr = fopen(filename, "r");
	static string payload;

	if(fptr == NULL)
		cerr << "No Such File in client directory." << endl;
	else{
		req_header.set_length(HEADER_LENGTH + strlen(filename) + 1);
		send_all(connfd, &req_header, HEADER_LENGTH);
		send_all(connfd, filename, strlen(filename) + 1);
		recv_all(connfd, &rec_header, HEADER_LENGTH);

		if(rec_header == PUT_REPLY){
			for(payload = "";
				feof(fptr) == false;
				payload += fgetc(fptr));
			fclose(fptr);

			dat_header = FILE_DATA;
			dat_header.set_length(HEADER_LENGTH + payload.length());
			send_all(connfd, &dat_header, HEADER_LENGTH);
			send_all(connfd, payload.c_str(), payload.length());
		}
		else cerr << "Invalid Reply Header." << endl;
	}
}

void sha256(const int &connfd, const char *filename){
	header req_header = SHA_REQUEST, rec_header, dat_header;
	static char message[MESSAGE_SIZE_LIMIT];
	
	req_header.set_length(HEADER_LENGTH + strlen(filename) + 1);
	send_all(connfd, &req_header, HEADER_LENGTH);
	send_all(connfd, filename, strlen(filename) + 1);
	recv_all(connfd, &rec_header, HEADER_LENGTH);

	if(headereq(rec_header, SHA_REPLY(0), IGNORE_STATUS) == false)
		cerr << "Invalid Reply Header." << endl;
	else if(rec_header.get_status() == false)
		cout << "No such file in server directory." << endl;
	else {
		recv_all(connfd, &dat_header, HEADER_LENGTH);
		if(headereq(dat_header, FILE_DATA, IGNORE_LENGTH)){
			int message_length = dat_header.get_length() - HEADER_LENGTH;
			recv_all(connfd, message, message_length);
			cout << message << flush;
		}
		else cerr << "File Data Error." << endl;
	}
}

bool quit(const int &connfd){
	header req_header = QUIT_REQUEST, rec_header;
	send_all(connfd, &req_header, HEADER_LENGTH);
	recv_all(connfd, &rec_header, HEADER_LENGTH);

	if(rec_header == QUIT_REPLY)
		return close(connfd), cout << "Disconnected." << endl, true;
	else return cerr << "Disconnecting Failed." << endl, false;
}

int main() {
	while(true){
		static char IP[16], filename[FILENAME_SIZE_LIMIT];
		static int port, connfd;
		static bool connected = false;

		switch(input_handler(IP, port, filename)){
		case 1: //open
			if(!connected && init(connfd, IP, port))
				connected = open(connfd);
			else cout << "Please check if there are other connections." << endl;
			break;
		case 2: //ls
			if(connected)
				list(connfd);
			else cout << "Please connect to the server before executing other commands." << endl;
			break;
		case 3: //get
			if(connected)
				getfile(connfd, filename);
			else cout << "Please connect to the server before executing other commands." << endl;
			break;
		case 4: //put
			if(connected)
				putfile(connfd, filename);
			else cout << "Please connect to the server before executing other commands." << endl;
			break;
		case 5: //sha256
			if(connected)
				sha256(connfd, filename);
			else cout << "Please connect to the server before executing other commands." << endl;
			break;
		case 6: //quit
			if(connected)
				connected = !quit(connfd);
			else return 0;
			break;
		default:
			p_help_page();
			break;
		}
	}

	return 0;
}
