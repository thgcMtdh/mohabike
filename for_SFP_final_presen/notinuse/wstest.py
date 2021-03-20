# This Python file uses the following encoding: utf-8
import time, glob, json
from flask import Flask, render_template, request
from flask_socketio import SocketIO

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
socketio = SocketIO(app)

# 車両一覧を取得
carnamelist = []
for p in glob.glob("cardata/*.json"):
    with open(p) as f:
        jsondata = json.load(f)
        carnamelist.append(jsondata.get("name"))

# HTMLのレンダリングと、速度を送るバックグラウンドタスクの開始
@app.route('/')
def index():
    socketio.start_background_task(target=send_speed)
    return render_template("index.html", carnameList=carnamelist)

# 設定が変更されたとき
@socketio.on('my_sendconfig')
def handle_config(json):
    print(str(json))

# バックグラウンドで1秒おきに速度を送信する関数 (サーバ -> ブラウザ)
def send_speed():
    while True:
        socketio.sleep(1)
        content = time.time()
        socketio.emit('my_sendspeed', {'time':content})  # my_sendspeedというイベント名で、速度を送る

if __name__ == "__main__":
    socketio.run(app, host="0.0.0.0", port=50050)