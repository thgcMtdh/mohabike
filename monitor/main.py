import time, json, glob, os
import serial
import responder

# setup pigpio
import pigpio
pi = pigpio.pi()
pin_servo1 = 12  # PWM0
pin_servo2 = 13  # PWM1
pin_led = 25
pi.set_mode(pin_servo1, pigpio.OUTPUT)
pi.set_mode(pin_servo2, pigpio.OUTPUT)
pi.set_mode(pin_led, pigpio.OUTPUT)
SERVO_MIN = 26000
SERVO_MAX = 115000
pi.hardware_PWM(pin_servo1, 50, SERVO_MIN)  # (PIN, [Hz], duty cycle[%] * 10000)
pi.write(pin_led, 0)

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
    global rxbuf, speed, Iac, flagSerial
    print("[recieveinfo] start serial communication")
    # シリアルポートをオープン
    try:
        # ser = serial.Serial('/dev/ttyACM0', 9600)  # USB接続後、ttyACM0
        ser = None
    except serial.serialutil.SerialException:
        rxbuf = "{\"serialfailed\":true}"
        flagSerial = False
    
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
            pi.hardware_PWM(pin_servo1, 50, 26000)  # servoに書き込み(30km/hで180deg)
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
    # マイコンのpiポートへ転送

@api.route("/")
def hello_html(req,resp):
    global flagSerial, flagReset, flagDemo
    reset(None,None)
    resp.html = api.template('index.html')

if __name__ == '__main__':
    api.run(address='0.0.0.0', port=5042)  # 0.0.0.0にすると外部からアクセスできる
