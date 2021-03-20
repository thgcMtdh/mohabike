# This Python file uses the following encoding: utf-8
import struct, subprocess, sys
import sysv_ipc
shm = sysv_ipc.SharedMemory(None, flags=sysv_ipc.IPC_CREX)  # key
#shm = sysv_ipc.SharedMemory(29963)
print("key=",shm.key,"id=",shm.id)
# subprocess.run(["./mem.o", str(shm.id)], stdout=sys.stdout)
# print(shm.read(1,offset=0).decode())  # 1バイト読み(bytesクラス)、文字列(str)として出力:H
# print(shm.read(4,offset=6))
# res = struct.unpack("<i", shm.read(4,offset=6))  # 4バイト読み(bytesクラス)、intに変換
# print(res[0])
# print(shm.read(8,offset=10))
# res = struct.unpack("<d", shm.read(8,offset=10))  # 8バイト読み、doubleに変換
# print(res[0])
# shm.detach()
# shm.remove()