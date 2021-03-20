# This Python file uses the following encoding: utf-8
import sys, time, struct, subprocess, glob, json
import sysv_ipc
from flask import Flask, render_template, request
from flask_socketio import SocketIO

# 車両一覧を取得
carnamelist = []
currentcarindex = 0
for p in glob.glob("pwm_*.o"):
    carnamelist.append(p[4:len(p)-2])

# 共有メモリの確保
OFFSET_isdynamo, OFFSET_isvvvf, OFFSET_notch, OFFSET_run = 0, 1, 2, 7
OFFSET_speed, OFFSET_trainspeed = 8, 16
OFFSET_fs, OFFSET_fc, OFFSET_frand, OFFSET_Vs = 24, 32, 40, 48
OFFSET_pulsemode = 64
shm = sysv_ipc.SharedMemory(None, flags=sysv_ipc.IPC_CREX)
print("[main.py] Shared memory ready:","key =",shm.key,"id =",shm.id)

# Flask ウェブサーバーの初期化
app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
socketio = SocketIO(app)

# indexにアクセスされたときに走る
@app.route('/')
def index():
    # ブラウザへデータを送信するタスクの開始
    socketio.start_background_task(target=send_data)
    # ブラウザにwebページのデータ(index.html)と車両一覧(carnamelist)を返す
    return render_template("index.html", carnamelist=carnamelist)


# 以下は、ブラウザ上で設定が変更されたときに走る処理
@socketio.on('send_control')  # 各種スイッチが変更されたときに実行
def handle_control(control):
    shm.write(control['isdynamo'].encode(), offset=OFFSET_isdynamo)
    shm.write(control['isvvvf'].encode(), offset=OFFSET_isvvvf)
    shm.write(control['notch'].encode(), offset=OFFSET_notch)
    print("[main.py] send_control:", control)

proc_pwm = None
@socketio.on('send_run')  # PWMソフトの起動/停止が指示されたときに実行
def handle_run(run):
    global proc_pwm
    if run == '1':
        shm.write(b'1', offset=OFFSET_run)
        proc_pwm = subprocess.Popen(['./pwm_' + carnamelist[currentcarindex] + '.o', str(shm.id)])
    else:
        shm.write(b'0', offset=OFFSET_run)
    print("[main.py] send_run: run =", run)

@socketio.on('send_car')  # 選択されている車両が変更されたときに実行
def handle_car(carindex):
    global currentcarindex
    if currentcarindex != carindex:  # 現在の車両と異なる車両が指示されたら
        currentcarindex = carindex  # currentcarindexを変更
        if shm.read(1, offset=OFFSET_run).decode() == '1':  # 現在PWMを実行中の場合は一旦停止する
            handle_run('0')
            time.sleep(0.2)
            handle_run('1')
        print("[main.py] handle_car: currentcarindex =", currentcarindex)


# 速度等をブラウザへ送信
def send_data():
    while True:
        if shm.read(1, offset=OFFSET_run).decode() == '1':
            socketio.sleep(0.1)  # 0.1秒おきに実行
            speed = struct.unpack("<d", shm.read(8,offset=OFFSET_speed))[0]
            trainspeed = struct.unpack("<d", shm.read(8,offset=OFFSET_trainspeed))[0]
            fs = struct.unpack("<d", shm.read(8,offset=OFFSET_fs))[0]
            fc = struct.unpack("<d", shm.read(8,offset=OFFSET_fc))[0]
            frand = struct.unpack("<d", shm.read(8,offset=OFFSET_frand))[0]
            Vs = struct.unpack("<d", shm.read(8,offset=OFFSET_Vs))[0]
            pulsemode = struct.unpack("<i", shm.read(4,offset=OFFSET_pulsemode))[0]
            socketio.emit('send_data', {'speed':speed, 'trainspeed':trainspeed, 'fs':fs, 'fc':fc, 'frand':frand, 'Vs':Vs, 'pulsemode':pulsemode})  # send_speedというイベント名で、速度を送る



if __name__ == "__main__":
    # Flask ソケットを起動
    socketio.run(app, host="0.0.0.0", port=50050)

# 共有メモリの解放
shm.detach()
shm.remove()
print("[main.py] Sheared memory removed")