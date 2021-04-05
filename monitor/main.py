import time, json, glob, os
import serial
import responder

# instead of serial com, read text file
rxbuf0 = open("rxdata.json", "r").read().replace("\n","")

# flags and variables
flagSerial = False  # シリアル通信の実行有無 (Trueのときマイコンからinfo受信)
flagReset = False   # Falseにするとマイコンをリセット
flagDemo = True     # デモモード(モータ単体を回す)
rxbuf = None   # マイコンから受け取った文字列
speed = 0.0  # 走行速度[km/h](メーターに表示)
Iac = 0.0    # 主電動機電流[A](メーターに表示)

# 車両一覧を取得
carlist = []
for p in glob.glob("static/cars/*.txt"):
    carlist.append(os.path.split(p)[1].replace(".txt", ""))
print(carlist)

# web server start
api = responder.API()

@api.background.task
def recieveinfo():
    global rxbuf, speed, Iac
    print("[recieveinfo] start serial communication")
    # シリアルポートをオープン
    # try:
    #     ser = serial.Serial('COM16', 9600)
    # except serial.serialutil.SerialException:
    #     rxbuf = "{\"serialfailed\":true}"
    #     flagSerial = False

    while flagSerial:
        # シリアル通信
        # rxbuf = ser.readline()
        # rxbuf = rxbuf.decode('ascii','ignore')
        rxbuf = rxbuf0
        # デコード
        try:
            rxdict = json.loads(rxbuf)
            speed = rxdict['speed']
            # Iac = rxdict['Iac']
        except json.decoder.JSONDecodeError:
           print("[recieveinfo] json.decoder.JSONDecodeError")
    print("[recieveinfo] end serial communication")

@api.route("/reset")
def reset(req,resp):
    global flagSerial, flagReset, flagDemo
    flagSerial = False
    flagReset = False
    flagDemo = True
    time.sleep(0.5)  # 確実にリセットされるまで時間をおく

@api.route("/info")
def get_info(req,resp):
    resp.text = rxbuf

@api.route("/carlist")
def get_carlist(req,resp):
    resp.media = carlist

@api.route("/command")
async def post_command(req,resp):
    global flagSerial, flagReset, flagDemo
    data = await req.media()
    # シリアル通信を行うバックグラウンドプロセスの立ち上げ
    if (flagSerial == False) and (data['serial'] == True):
        flagSerial = True
        recieveinfo()
    # 状態の同期
    flagSerial = data['serial']
    flagReset = data['reset']
    flagDemo = data['demo']
    # マイコンのGPIOポートへ転送

@api.route("/")
def hello_html(req,resp):
    global flagSerial, flagReset, flagDemo
    reset(None,None)
    resp.html = api.template('index.html')

if __name__ == '__main__':
    api.run(address='0.0.0.0', port=5042)  # 0.0.0.0にすると外部からアクセスできる
