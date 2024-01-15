#include "router_prototype.h"
#include <cstdint>
#include <map>
#include <queue>
#include <set>

#define PORT_SIZE_LIM 4096
#define IP_LENGTH_LIM 40

using std::map;
using std::queue;
using std::set;

typedef std::pair<int, int> pii;

const uint8_t TYPE_DV = 0x00;
const uint8_t TYPE_DATA = 0x01;
const uint8_t TYPE_CONTROL = 0x02;

const int PORT_IDLE = 0;
const int PORT_PEER = 1;
const int PORT_HOST = 2;
const int PORT_EXTERN = 3;
const int PORT_UNKNOWN_KEY = -1;

const int CTRL_TRIGGER_DV_SEND = 0;
const int CTRL_RELEASE_NAT_ITEM = 1;
const int CTRL_PORT_VALUE_CHANGE = 2;
const int CTRL_ADD_HOST = 3;
const int CTRL_BLOCK_ADDR = 5;
const int CTRL_UNBLOCK_ADDR = 6;

const int SIG_IGNORE = 0;
const int SIG_RESEND = 1;
const int SIG_RECALC = 2;

struct header_t {
    uint32_t src;
    uint32_t dst;
    uint8_t type;
    uint16_t length;
    header_t();
    header_t(uint32_t, uint32_t, uint8_t, uint16_t);
};

struct dvMsg_t {
    uint32_t ip;
    int block;
    int dis;
    int nex_id;
};

struct dvNode_t {
    int block;
    int port;
    int dis;
    uint32_t nex_id;
    dvNode_t();
    dvNode_t(int, int, int, int);
    friend bool operator==(dvNode_t, dvNode_t);
};

struct portInfo_t {
    uint32_t key; // key: id if type == PORT_PEER; ip if type == PORT_HOST || PORT_EXTERN.
    int dis;
    int type;
    portInfo_t();
    portInfo_t(int, int, int);
};

class Router : public RouterBase {
    uint32_t local_id;
    uint32_t port_siz;
    uint32_t ext_port;
    uint32_t ext_addr;
    int ext_block_siz;
    uint32_t nat_addr;
    int nat_block_siz;
    portInfo_t port_info[PORT_SIZE_LIM];
    map<uint32_t, dvNode_t> dv;
    map<uint32_t, dvNode_t> port_dv[PORT_SIZE_LIM];
    set<uint32_t> ban_list;
    queue<uint32_t> idle_addr;
    map<uint32_t, uint32_t> snd_nat, rcv_nat;

public:
    Router(uint32_t);
    void router_init(int port_num, int external_port, char *external_addr, char *available_addr);
    int router(int in_port, char *packet);

    uint16_t dv2packet(char *payload);
    void packet2dv(int in_port, short length, char *payload);
    bool dv_update();

    int control_handler(char *payload);
};

header_t::header_t() {}
header_t::header_t(uint32_t _src, uint32_t _dst, uint8_t _type, uint16_t _length)
    : src(_src), dst(_dst), type(_type), length(_length) {}

dvNode_t::dvNode_t() {}
dvNode_t::dvNode_t(int _block, int _port, int _dis, int _nex_id)
    : block(_block), port(_port), dis(_dis), nex_id(_nex_id) {}

portInfo_t::portInfo_t() {}
portInfo_t::portInfo_t(int _key, int _dis, int _type)
    : key(_key), dis(_dis), type(_type) {}

bool operator==(dvNode_t x, dvNode_t y) {
    return x.block == y.block && x.dis == y.dis && x.nex_id == y.nex_id && x.port == y.port;
}

Router::Router(uint32_t id = 0) : local_id(id) {}
