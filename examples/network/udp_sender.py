#!/usr/bin/env python3
"""Send finite HFS1 UDP requests; time replies on this one monotonic clock."""
import argparse
import ipaddress
import json
import socket
import struct
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('address', type=ipaddress.IPv4Address)
    parser.add_argument('--port', type=int, default=40420)
    parser.add_argument('--count', type=int, default=10)
    parser.add_argument('--timeout', type=float, default=2)
    parser.add_argument('--interval', type=float, default=0.05)
    parser.add_argument('--expect-timeout', action='store_true')
    parser.add_argument('--malformed', action='store_true', help='Send invalid version; expect rejection/timeouts.')
    args = parser.parse_args()
    if not 1 <= args.port <= 65535 or not 1 <= args.count <= 10000 or not 0 < args.timeout <= 30 or not 0 <= args.interval <= 10:
        parser.error('port/count/timeout/interval outside the finite lab bounds')
    received_sequences = []
    round_trips = []
    timeouts = 0
    unexpected = 0
    unreachable = 0
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as endpoint:
        endpoint.connect((str(args.address), args.port))
        for sequence in range(1, args.count + 1):
            payload = struct.pack('!3sBI', b'HFS', 2 if args.malformed else 1, sequence)
            started = time.monotonic()
            deadline = started + args.timeout
            endpoint.send(payload)
            matched = False
            while time.monotonic() < deadline:
                endpoint.settimeout(max(0.001, deadline - time.monotonic()))
                try:
                    reply = endpoint.recv(2048)
                except socket.timeout:
                    break
                except ConnectionRefusedError:
                    # An absent receiver can produce ICMP port-unreachable rather
                    # than silence. It is still an unsuccessful request, recorded
                    # distinctly so it cannot establish XDP drop by itself.
                    unreachable += 1
                    print(json.dumps({'event': 'unreachable', 'sequence': sequence}))
                    break
                if reply != payload or args.malformed:
                    unexpected += 1
                    continue
                elapsed = (time.monotonic() - started) * 1000
                received_sequences.append(sequence)
                round_trips.append(elapsed)
                print(json.dumps({'event': 'reply', 'sequence': sequence, 'rtt_ms': elapsed}))
                matched = True
                break
            if not matched:
                timeouts += 1
                print(json.dumps({'event': 'timeout', 'sequence': sequence}))
            time.sleep(args.interval)
    ordered = sorted(round_trips)
    summary = {'event': 'summary', 'sent': args.count, 'received': len(received_sequences),
               'timeouts': timeouts, 'unexpected': unexpected, 'unreachable': unreachable, 'sequences': received_sequences,
               'rtt_min_ms': min(ordered) if ordered else None,
               'rtt_median_ms': ordered[len(ordered) // 2] if ordered else None,
               'rtt_max_ms': max(ordered) if ordered else None}
    print(json.dumps(summary))
    if args.expect_timeout:
        return 0 if timeouts == args.count and not received_sequences and unexpected == 0 and unreachable == 0 else 1
    return 0 if len(received_sequences) == args.count and unexpected == 0 else 1


if __name__ == '__main__':
    raise SystemExit(main())
