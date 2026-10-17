import struct, hashlib
from Crypto.Cipher import AES
KEYS={'dsr':bytes.fromhex('0123456789abcdeffedcba9876543210'),'ds3':bytes.fromhex('fd464d695e69a39a10e319a7ace8b7fa'),'ds2':bytes.fromhex('599f9b699640a55236ee2d70835ec744')}
def load(path, game):
    d=open(path,'rb').read(); key=KEYS[game]
    n=struct.unpack_from('<I',d,12)[0]; out=[]
    for i in range(n):
        _,size,off,noff,_=struct.unpack_from('<QQIIQ',d,64+i*32)
        name=d[noff:noff+26].decode('utf-16le').split('\0')[0]
        raw=d[off:off+size]; md5,iv,enc=raw[:16],raw[16:32],raw[32:]
        ok=hashlib.md5(raw[16:]).digest()==md5
        dec=AES.new(key,AES.MODE_CBC,iv).decrypt(enc)
        ln=struct.unpack_from('<I',dec,0)[0]
        out.append(dict(name=name,size=size,ok=ok,len=ln,data=dec[4:4+ln],pad=dec[4+ln:]))
    return out
