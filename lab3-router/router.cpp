#include "router.h"
#include <cassert>
#include <iostream>

#define REV32(x) (((x) & 0xff) << 24 | ((x) & 0xff00) << 8 | ((x) & 0xff0000) >> 8 | ((x) & 0xff000000) >> 24)

RouterBase *create_router_object() {
    static uint32_t obj_cnt = 0;
    return new Router(++obj_cnt);
}

uint32_t ip_str2uint(char *addr) {
    uint32_t ret = 0, res = 0;

    for (int i = 0, pnt = 0; i < 4; i++, pnt++, res = 0) {
        for (;; pnt++) {
            if (isdigit(addr[pnt]))
                res = res * 10 + addr[pnt] - '0';
            else
                break;
        }
        ret = (ret << 8) + res;
    }
    return ret;
}

uint32_t ip_str2pair(char *addr, int &block_siz) {
    int pnt = 0;
    uint32_t ret = ip_str2uint(addr), res = 0;

    for (; addr[pnt] != '/'; pnt++)
        ;
    pnt++;
    for (; addr[pnt] != '\0'; pnt++)
        res = res * 10 + addr[pnt] - '0';
    ret = ret & (0xffffffff << 32 - res);
    block_siz = 1 << 32 - res;
    return ret;
}

void Router::router_init(int port_num, int external_port, char *external_addr, char *available_addr) {
    assert(port_num <= PORT_SIZE_LIM);
    port_siz = port_num;
    ext_port = external_port;
    if (ext_port) {
        ext_addr = ip_str2pair(external_addr, ext_block_siz);
        nat_addr = ip_str2pair(available_addr, nat_block_siz);
        for (uint32_t i = nat_addr; i < nat_addr + nat_block_siz; i++)
            idle_addr.push(i);
    }
    return;
}

void depack(header_t &header, char *packet) {
    header = *(header_t *)packet;
    header.src = REV32(header.src);
    header.dst = REV32(header.dst);
}

void inpack(header_t header, char *packet) {
    header.src = REV32(header.src);
    header.dst = REV32(header.dst);
    *(header_t *)packet = header;
}

int Router::router(int in_port, char *packet) {
    int sig;
    dvNode_t info;
    header_t header;
    map<uint32_t, dvNode_t>::iterator it;
    depack(header, packet);

    switch (header.type) {
    case TYPE_DV:
        packet2dv(in_port, header.length, packet + sizeof(header_t));
        if (dv_update()) {
            header.length = dv2packet(packet + sizeof(header_t));
            inpack(header, packet);
            return 0;
        }
        return -1;

    case TYPE_DATA:
        if (in_port == ext_port) {
            auto nat_it = rcv_nat.find(header.dst);
            if (nat_it == rcv_nat.end())
                return -1;
            header.dst = nat_it->second;
        }
        if (ban_list.find(header.src) != ban_list.end())
            return -1;

        it = dv.upper_bound(header.dst);
        if (it == dv.begin())
            return inpack(header, packet), 1;
        info = (--it)->second;
        if (header.dst >= it->first + info.block)
            return inpack(header, packet), 1;

        if (info.port == ext_port) {
            auto nat_it = snd_nat.find(header.src);
            if (nat_it == snd_nat.end()) {
                if (idle_addr.empty())
                    return -1;

                int32_t new_ip = idle_addr.front();
                idle_addr.pop();
                snd_nat[header.src] = new_ip;
                rcv_nat[new_ip] = header.src;
                header.src = new_ip;
            } else
                header.src = nat_it->second;
        }

        inpack(header, packet);
        return info.port;

    case TYPE_CONTROL:
        sig = control_handler(packet + sizeof(header_t));
        if (sig == SIG_RESEND || (sig == SIG_RECALC && dv_update())) {
            header = header_t(0, 0, TYPE_DV, 0);
            header.length = dv2packet(packet + sizeof(header_t));
            inpack(header, packet);
            return 0;
        }
        return -1;

    default:
        break;
    }

    return -1;
}

uint16_t Router::dv2packet(char *payload) {
    int dv_siz = dv.size();
    dvMsg_t *snd_msg = (dvMsg_t *)(payload + sizeof(int));
    auto it = dv.begin();

    *(int *)payload = local_id;
    for (int i = 0; i < dv_siz; i++, it++) {
        snd_msg[i].ip = it->first;
        snd_msg[i].dis = it->second.dis;
        snd_msg[i].block = it->second.block;
        snd_msg[i].nex_id = it->second.nex_id;
    }

    return dv_siz * sizeof(dvMsg_t) + sizeof(int);
}

void Router::packet2dv(int in_port, short length, char *payload) {
    int dv_siz = (length - sizeof(int)) / sizeof(dvMsg_t);
    dvMsg_t *rcv_msg = (dvMsg_t *)(payload + sizeof(int));

    if (port_info[in_port].key == PORT_UNKNOWN_KEY)
        port_info[in_port].key = *(int *)payload;
    port_dv[in_port].clear();
    for (int i = 0; i < dv_siz; i++) {
        port_dv[in_port][rcv_msg[i].ip] =
            dvNode_t(rcv_msg[i].block, -1, rcv_msg[i].dis, rcv_msg[i].nex_id);
    }
}

bool Router::dv_update() {
    auto las_dv = dv;
    dv.clear();
    if (ext_port)
        dv[ext_addr] = dvNode_t(ext_block_siz, ext_port, 0, ext_addr);

    for (int p = 2; p <= port_siz; p++)
        if (port_info[p].type == PORT_HOST)
            dv[port_info[p].key] = dvNode_t(1, p, 0, port_info[p].key);

    for (int p = 2; p <= port_siz; p++) {
        if (port_info[p].type == PORT_PEER) {
            for (auto it = port_dv[p].begin(); it != port_dv[p].end(); it++) {
                uint32_t ip = it->first;
                int tot_dis = it->second.dis + port_info[p].dis;

                if (it->second.nex_id == local_id)
                    continue;
                if (dv.find(ip) == dv.end() || tot_dis < dv[ip].dis)
                    dv[ip] = dvNode_t(it->second.block, p, tot_dis, port_info[p].key);
            }
        }
    }

    return las_dv != dv;
}

int Router::control_handler(char *payload) {
    static char ip_str[IP_LENGTH_LIM];
    int type = payload[0] - '0';
    int port = 0, val = 0;
    uint32_t ip = 0;

    switch (type) {
    case CTRL_TRIGGER_DV_SEND:
        return SIG_RESEND;

    case CTRL_RELEASE_NAT_ITEM:
        ip = ip_str2uint(payload + 2);
        rcv_nat.erase(snd_nat[ip]);
        idle_addr.push(snd_nat[ip]);
        snd_nat.erase(ip);
        return SIG_IGNORE;

    case CTRL_PORT_VALUE_CHANGE:
        sscanf(payload + 2, "%d%d", &port, &val);
        if (val == -1)
            port_info[port] = portInfo_t(0, 0, 0);
        else if (port_info[port].type == PORT_IDLE) {
            port_info[port] = portInfo_t(PORT_UNKNOWN_KEY, val, PORT_PEER);
            return SIG_RESEND;
        } else
            port_info[port].dis = val;
        return SIG_RECALC;

    case CTRL_ADD_HOST:
        sscanf(payload + 2, "%d%s", &port, ip_str);
        ip = ip_str2uint(ip_str);
        port_info[port] = portInfo_t(ip, 0, PORT_HOST);
        return SIG_RECALC;

    case CTRL_BLOCK_ADDR:
        ip = ip_str2uint(payload + 2);
        ban_list.insert(ip);
        return SIG_IGNORE;

    case CTRL_UNBLOCK_ADDR:
        ip = ip_str2uint(payload + 2);
        ban_list.erase(ip);
        return SIG_IGNORE;

    default:
        return SIG_IGNORE;
    }
}
