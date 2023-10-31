#include <sys/socket.h>
#include <bits/stdint-uintn.h>

#define HEADER_LENGTH		12
#define MAGIC_NUMBER_LENGTH 6
#define UNUSED				0

#define OPEN_CONN_REQUEST 	header(0xA1, UNUSED, 12)
#define OPEN_CONN_REPLY		header(0xA2, 1, 12)
#define LIST_REQUEST		header(0xA3, UNUSED, 12)
#define LIST_REPLY			header(0xA4, UNUSED, 12)
#define GET_REQUEST			header(0xA5, UNUSED, 12)
#define GET_REPLY(x)		header(0xA6, x, 12)
#define PUT_REQUEST			header(0xA7, UNUSED, 12)
#define PUT_REPLY			header(0xA8, UNUSED, 12)
#define SHA_REQUEST			header(0xA9, UNUSED, 12)
#define SHA_REPLY(x)		header(0xAA, x, 12)
#define QUIT_REQUEST		header(0xAB, UNUSED, 12)
#define QUIT_REPLY			header(0xAC, UNUSED, 12)
#define FILE_DATA			header(0xFF, UNUSED, 12)

#define IGNORE_TYPE			1
#define IGNORE_STATUS		2
#define IGNORE_LENGTH		4

const char key_protocol[] = {'\xc1', '\xa1', '\x10', 'f', 't', 'p'};

struct header{
	uint8_t m_protocol[MAGIC_NUMBER_LENGTH];
	uint8_t m_type;
	uint8_t m_status;
	uint32_t m_length;

	header(uint8_t type = UNUSED, uint8_t status = UNUSED, uint32_t length = UNUSED):
		m_type(type), m_status(status), m_length(htonl(length)){
			memcpy(m_protocol, key_protocol, MAGIC_NUMBER_LENGTH);
	}

	void set_type(uint8_t type){ m_type = type; }
	void set_status(uint8_t status){ m_status = status; }
	void set_length(uint32_t length){ m_length = htonl(length); }
	uint8_t get_type(){ return m_type; }
	uint8_t get_status(){ return m_status; }
	uint32_t get_length(){ return ntohl(m_length); }

	bool operator == (const header &ano){
		return memcmp(m_protocol, ano.m_protocol, MAGIC_NUMBER_LENGTH) == 0
			&& m_type == ano.m_type
			&& m_status == ano.m_status
			&& m_length == ano.m_length;
	}

} __attribute__ ((packed));

bool headereq(const header &le, const header &ri, const int flag = 0){
	return memcmp(le.m_protocol, ri.m_protocol, MAGIC_NUMBER_LENGTH) == 0
		&& ((flag & IGNORE_TYPE) || le.m_type == ri.m_type)
		&& ((flag & IGNORE_STATUS) || le.m_status == ri.m_status)
		&& ((flag & IGNORE_LENGTH) || le.m_length == ri.m_length);
}
