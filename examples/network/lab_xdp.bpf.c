// Select only this book's untagged, unfragmented IPv4 UDP requests.
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, __u32);
} mode SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 3);
    __type(key, __u32);
    __type(value, __u64);
} counters SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_XSKMAP);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, __u32);
} sockets SEC(".maps");

static __always_inline void count_action(__u32 action) {
    __u64* counter = bpf_map_lookup_elem(&counters, &action);
    if (counter) {
        __sync_fetch_and_add(counter, 1);
    }
}

SEC("xdp")
int lab_xdp(struct xdp_md* context) {
    unsigned char* frame = (void*)(long)context->data;
    unsigned char* end = (void*)(long)context->data_end;
    if (frame + 14 + 20 > end || frame[12] != 0x08 || frame[13] != 0x00) {
        return XDP_PASS;
    }
    unsigned char* ip = frame + 14;
    unsigned int header_size = (ip[0] & 15) * 4;
    if ((ip[0] >> 4) != 4 || header_size != 20 || ip + header_size + 16 > end ||
        ip[9] != 17) {
        return XDP_PASS;
    }
    unsigned int total_size = ((__u32)ip[2] << 8) | ip[3];
    unsigned int fragment = ((__u32)ip[6] << 8) | ip[7];
    if ((fragment & 0x3fff) || total_size != header_size + 16) {
        return XDP_PASS;
    }
    unsigned char* udp = ip + header_size;
    if (udp[2] != (40420 >> 8) || udp[3] != (40420 & 255) || udp[4] != 0 ||
        udp[5] != 16 || udp[8] != 'H' || udp[9] != 'F' || udp[10] != 'S' ||
        udp[11] != 1) {
        return XDP_PASS;
    }
    __u32 key = 0;
    __u32* selected_mode = bpf_map_lookup_elem(&mode, &key);
    if (!selected_mode || *selected_mode == 0) {
        count_action(0);
        return XDP_PASS;
    }
    if (*selected_mode == 1) {
        count_action(1);
        return XDP_DROP;
    }
    key = context->rx_queue_index;
    // Map lookup distinguishes an absent socket: leave that packet on the stack.
    if (key == 0 && bpf_map_lookup_elem(&sockets, &key)) {
        count_action(2);
        return bpf_redirect_map(&sockets, key, XDP_PASS);
    }
    count_action(0);
    return XDP_PASS;
}

char LICENSE[] SEC("license") = "GPL";
