#ifndef COMPNET_LAB4_SRC_SWITCH_H
#define COMPNET_LAB4_SRC_SWITCH_H

#include "types.h"
#include <unordered_map>

using std::unordered_map;

class SwitchBase {
public:
	SwitchBase() = default;
	~SwitchBase() = default;

	virtual void InitSwitch(int numPorts) = 0;
	virtual int ProcessFrame(int inPort, char *framePtr) = 0;
};

extern SwitchBase *CreateSwitchObject();
extern int PackFrame(char *unpacked_frame, char *packed_frame, int frame_length);
extern int UnpackFrame(char *unpacked_frame, char *packed_frame, int frame_length);

struct mac_t {
	mac_t();
	mac_t(const mac_addr_t &t);
	mac_addr_t mac;
	bool operator==(const mac_t &t) const;
};

struct mac_hasher {
	uint64_t operator()(const mac_t &t) const;
};

class Switch : public SwitchBase {
public:
	Switch() = default;
	~Switch() = default;

	void InitSwitch(int numPorts);
	int ProcessFrame(int inPort, char *framePtr);

private:
	static const int thresh = ETHER_MAC_AGING_THRESHOLD;
	int counter;
	unordered_map<mac_t, int, mac_hasher> port;
	unordered_map<mac_t, int, mac_hasher> stamp;
};

#endif // ! COMPNET_LAB4_SRC_SWITCH_H
