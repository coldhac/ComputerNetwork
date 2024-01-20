#include "switch.h"
#include "types.h"

#define __GET_CHECK_NUM_FUNC(x, y) ((x) >> y ^ (x))
#define GET_CHECK_NUM(x) (__GET_CHECK_NUM_FUNC(__GET_CHECK_NUM_FUNC(__GET_CHECK_NUM_FUNC(x, 4), 2), 1) & 1)

SwitchBase *CreateSwitchObject() {
	return new Switch();
}

int PackFrame(char *unpacked_frame, char *packed_frame, int frame_length) {
	int length = 0;
	uint8_t checksum = 0;

	packed_frame[length++] = FRAME_DELI;
	for (int i = 0; i < frame_length; i++) {
		if ((uint8_t)unpacked_frame[i] == FRAME_DELI)
			packed_frame[length++] = FRAME_DELI;
		packed_frame[length++] = unpacked_frame[i];
	}

	for (int i = 0; i < length; i++)
		checksum ^= packed_frame[i];
	packed_frame[length++] = GET_CHECK_NUM(checksum);

	return length;
}

int UnpackFrame(char *unpacked_frame, char *packed_frame, int frame_length) {
	int length = 0;
	uint8_t checksum = 0;

	for (int i = 0; i < frame_length; i++)
		checksum ^= packed_frame[i];
	if (GET_CHECK_NUM(checksum) || (uint8_t)packed_frame[0] != FRAME_DELI)
		return -1;

	for (int i = 1; i < frame_length - 1; i++) {
		if ((uint8_t)packed_frame[i] == FRAME_DELI)
			i++;
		unpacked_frame[length++] = packed_frame[i];
	}

	return length;
}

mac_t::mac_t() {}

mac_t::mac_t(const mac_addr_t &t) {
	uint8_t *src = (uint8_t *)&t;
	uint8_t *dst = (uint8_t *)&mac;
	for (int i = 0; i < ETH_ALEN; i++)
		dst[i] = src[i];
}

bool mac_t::operator==(const mac_t &t) const {
	uint8_t *l = (uint8_t *)&mac;
	uint8_t *r = (uint8_t *)&t;
	for (int i = 0; i < ETH_ALEN; i++)
		if (l[i] != r[i]) return false;
	return true;
}

uint64_t mac_hasher::operator()(const mac_t &t) const {
	uint8_t *p = (uint8_t *)&t;
	uint64_t ret = 0;
	for (int i = 0; i < ETH_ALEN; i++)
		ret = ret << 8 | p[i];
	return ret;
}

void Switch::InitSwitch(int numPorts) {
	counter = 0;
}

int Switch::ProcessFrame(int inPort, char *framePtr) {
	ether_header_t header = *(ether_header_t *)framePtr;
	mac_t src = header.ether_src;
	mac_t dst = header.ether_dest;

	switch (header.ether_type) {
	case ETHER_CONTROL_TYPE:
		++counter;
		break;

	case ETHER_DATA_TYPE:
		port[src] = inPort;
		stamp[src] = counter + thresh;
		if (port.find(dst) == port.end() || stamp[dst] <= counter)
			return 0;
		if (port[dst] == inPort)
			return -1;
		return port[dst];

	default:
		break;
	}
	return -1;
}
