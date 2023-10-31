#include <regex>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <limit.hpp>
#include <header.hpp>
#include <sockall.hpp>

using namespace std;

const header open_reply_header = OPEN_CONN_REPLY;

void init(const char *IP, const int port, int &listenfd){
	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		cerr << "Socket Error." << endl,
		exit(EXIT_FAILURE);
	struct sockaddr_in addr;
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	inet_pton(AF_INET, IP, &addr.sin_addr);
	
	if(bind(listenfd, (struct sockaddr*) &addr, sizeof(addr)) < 0)
		cerr << "Binding Error." << endl,
		cout << "Usage: ftp_server <IP> <port>" << endl,
		exit(EXIT_FAILURE);
	if(listen(listenfd, 128) < 0)
		cerr << "Listening Error." << endl,
		cout << "Usage: ftp_server <IP> <port>" << endl,
		exit(EXIT_FAILURE);
	cout << "Listening on port " << port << " of server " << IP << " ..." << endl;
}

void list(const int &connfd){
	header rep_header = LIST_REPLY;
	static string payload;
	static char buffer[MESSAGE_SIZE_LIMIT];

	FILE *fptr = popen("ls", "r");
	if(fptr == NULL) cerr << "Pipe Opening Error." << endl;
	else{
		for(payload = "";
			feof(fptr) == false && fgets(buffer, MESSAGE_SIZE_LIMIT, fptr) != NULL;
			payload += buffer);
		pclose(fptr);
			
		rep_header.set_length(HEADER_LENGTH + payload.length() + 1);
		send_all(connfd, &rep_header, HEADER_LENGTH);
		send_all(connfd, payload.c_str(), payload.length() + 1);
	}
}

void getfile(const int &connfd, const int filename_length){
	header rep_header = GET_REPLY(0), dat_header;
	static char filename[FILENAME_SIZE_LIMIT];
	static string payload;
	recv_all(connfd, filename, filename_length);

	FILE *fptr = fopen(filename, "r");
	if(fptr == NULL){
		cerr << "No Such File." << endl;
		send_all(connfd, &rep_header, HEADER_LENGTH);
	}
	else{
		rep_header.set_status(1);
		send_all(connfd, &rep_header, HEADER_LENGTH);

		for(payload = "";
			feof(fptr) == false;
			payload += getc(fptr));
		fclose(fptr);

		dat_header = FILE_DATA;
		dat_header.set_length(HEADER_LENGTH + payload.length());
		send_all(connfd, &dat_header, HEADER_LENGTH);
		send_all(connfd, payload.c_str(), payload.length());
	}
}

void putfile(const int &connfd, const int filename_length){
	header dat_header;
	header rep_header = PUT_REPLY;
	static char filename[FILENAME_SIZE_LIMIT];
	static char payload[FILE_SIZE_LIMIT];
	recv_all(connfd, filename, filename_length);
	send_all(connfd, &rep_header, HEADER_LENGTH);
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
	else cerr << "Data Error." << endl;
}

void sha256(const int &connfd, const int filename_length){
	header rep_header = SHA_REPLY(0), dat_header;
	static char filename[FILENAME_SIZE_LIMIT];
	static char buffer[MESSAGE_SIZE_LIMIT];
	static string payload;
	recv_all(connfd, filename, filename_length);

	if(access(filename, F_OK)) //return ZERO means file exists.
		send_all(connfd, &rep_header, HEADER_LENGTH);
	else{
		rep_header.set_status(1);
		send_all(connfd, &rep_header, HEADER_LENGTH);

		FILE *fptr = popen((string("sha256sum ") + filename).c_str(), "r");
		for(payload = "";
			!feof(fptr) && fgets(buffer, MESSAGE_SIZE_LIMIT, fptr) != NULL;
			payload += buffer);
		pclose(fptr);

		dat_header = FILE_DATA;
		dat_header.set_length(HEADER_LENGTH + payload.length() + 1);
		send_all(connfd, &dat_header, HEADER_LENGTH);
		send_all(connfd, payload.c_str(), payload.length() + 1);
	}
}

void request_handler(const int &connfd){
	while(true){
		header req_header;
		recv_all(connfd, &req_header, HEADER_LENGTH);

		if(headereq(req_header, QUIT_REQUEST)) break;
		else if(headereq(req_header, LIST_REQUEST))
			cout << "\'ls\' Request Recieved." << endl,
			list(connfd);
		else if(headereq(req_header, GET_REQUEST, IGNORE_LENGTH))
			cout << "\'get\' Request Recieved." << endl,
			getfile(connfd, req_header.get_length() - HEADER_LENGTH);
		else if(headereq(req_header, PUT_REQUEST, IGNORE_LENGTH))
			cout << "\'put\' Request Recieved." << endl,
			putfile(connfd, req_header.get_length() - HEADER_LENGTH);
		else if(headereq(req_header, SHA_REQUEST, IGNORE_LENGTH))
			cout << "\'sha256\' Request Recieved." << endl,
			sha256(connfd, req_header.get_length() - HEADER_LENGTH);
		else cerr << "Invalid Request Header." << endl;
	}
	cout << "\'quit\' Request Recieved." << endl;
	
	header rep_header = QUIT_REPLY;
	send_all(connfd, &rep_header, HEADER_LENGTH);
}

int main(int argc, char **argv) {
	if (argc != 3 ||
		regex_match(argv[1], regex("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}")) == false ||
		regex_match(argv[2], regex("\\d{1,5}")) == false)
			cerr << "Usage: ftp_server <IP> <port>" << endl,
			exit(EXIT_FAILURE);

	int listenfd;
	init(argv[1], stoi(argv[2]), listenfd);

	while(true){
		static int connfd;
		header recv_header;
		if((connfd = accept(listenfd, nullptr, nullptr)) < 0)
			cerr << "Accepting Error." << endl;
		else {
			cout << "Connecting ..." << endl;
			recv_all(connfd, &recv_header, HEADER_LENGTH);

			if(recv_header == OPEN_CONN_REQUEST)
				send_all(connfd, &open_reply_header, HEADER_LENGTH),
				request_handler(connfd);
			else cerr << "Invalid Connection." << endl;

			cout << "Disconnected." << endl;
			close(connfd);
		}
	}

	return 0;
}
