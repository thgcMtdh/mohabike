# This Python file uses the following encoding: utf-8
import sys, time, struct, subprocess, glob, json
import sysv_ipc


# 共有メモリの確保
OFFSET_isdynamo, OFFSET_isvvvf, OFFSET_notch, OFFSET_run = 0, 1, 2, 7
OFFSET_speed, OFFSET_trainspeed = 8, 16
OFFSET_fs, OFFSET_fc, OFFSET_frand, OFFSET_Vs = 24, 32, 40, 48
OFFSET_pulsemode = 64
shm = sysv_ipc.SharedMemory(None, flags=sysv_ipc.IPC_CREX)
shm.write(b'0', offset=OFFSET_isdynamo)  # 1:ダイナモ入力で速度を計算, 0:デモモード
shm.write(b'1', offset=OFFSET_isvvvf)    # 1:VVVF出力ON, 0:VVVF出力OFF
shm.write(b'P', offset=OFFSET_notch)     # P,N,B:ノッチ
shm.write(b'1', offset=OFFSET_run)       # 1:実行中, 0:プロセス終了
print("[main.py] Shared memory ready:","key =",shm.key,"id =",shm.id)

# PWMをバックグラウンドで起動
proc_pwm = subprocess.Popen(['./pwm_tobu100.o', str(shm.id)])
print("[main.py] PWM process started")

# 数秒間動かす
time.sleep(3)
shm.write(b'0', offset=OFFSET_run)  # PWM停止

# 共有メモリの解放
shm.detach()
shm.remove()
print("[main.py] Sheared memory removed")




