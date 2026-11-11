#!/usr/bin/env python3
"""Bounded pseudo-terminal tests of the exact serial client. No Pico implied."""
import errno, os, pty, select, subprocess, termios, time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def case(name,reply,expected,command=('PING',),inherit_flow=False):
    master,slave=pty.openpty();path=os.ttyname(slave)
    flow=termios.IXON | termios.IXOFF | termios.IXANY
    if inherit_flow:
        attributes=termios.tcgetattr(slave);attributes[0] |= flow
        termios.tcsetattr(slave,termios.TCSANOW,attributes)
    proc=subprocess.Popen([str(ROOT/'.build/serial_client'),path,*command],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
    try:
        end=time.monotonic()+4;request=b''
        while b'\n' not in request:
            if time.monotonic()>=end:raise AssertionError('no request received')
            if select.select([master],[],[],.1)[0]:request+=os.read(master,128)
        assert request==b'1 '+(' '.join(command)).encode()+b'\n',(name,request)
        if inherit_flow:
            assert termios.tcgetattr(slave)[0] & flow == 0, (name,'software flow control remains enabled')
        if reply is not None:
            # Each byte is deliberately a distinct write: framing cannot rely on read boundaries.
            for byte in reply:os.write(master,bytes([byte]))
        out,err=proc.communicate(timeout=4)
        assert proc.returncode==expected,(name,proc.returncode,out,err)
        if expected==0:assert out=='1 OK 1\n',(name,out)
        print(name,'passed')
    finally:
        if proc.poll() is None:proc.kill();proc.wait()
        os.close(master);os.close(slave)
for spec in [
    ('fragmented success',b'1 OK 1\n',0),
    ('CRLF success',b'1 OK 1\r\n',0),
    ('inherited software flow control',b'1 OK 1\n',0,('PING',),True),
    ('wrong ID',b'2 OK 1\n',1),
    ('device error',b'1 ERR unsupported\n',1),
    ('oversized reply',b'x'*100+b'\n',1),
    ('embedded NUL',b'1 OK 1\x00ignored\n',1),
    ('bad numeric result',b'1 OK garbage\n',1),
    ('wrong PING value',b'1 OK 99\n',1),
    ('LED mismatch',b'1 OK 0\n',1,('LED','1')),
    ('deadline',None,1),
]:case(*spec)
print('Serial PTY tests passed; this verifies host stream handling, not electrical UART behaviour.')
