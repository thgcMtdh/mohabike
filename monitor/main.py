import time, json, glob, os, enum
import serial
import threading
import responder

# 環境によって書き換える変数
isMCUConnected = True          # マイコンがUSBポートに接続されているか
SERIALPATH_RASPI = '/dev/ttyACM0'  # ラズパイのシリアルポート
SERIALPATH_WIN = 'COM16'           # Windowsのシリアルポート

# 各種定数
PIN_SERVO1 = 12  # GPIO12 PWM0 Pin
PIN_SERVO2 = 13  # GPIO13 PWM1 Pin
PIN_LED = 16     # GPIO25 LED Pin
SERVO_MIN = 26000   # サーボの最小duty
SERVO_MAX = 115000  # サーボの最大duty
SPEED_MAX = 30  # 速度の最大値 [km/h]
IMM_MAX = 7.5   # 電流の最大値(プラスとマイナス両方に振れる) [A]
RXBUF0 = open("rxdata.json", "r").read().replace("\n","")  # シリアル通信しないときにダミーで読み込む受信結果

class Meters():
    def __init__(self):
        self.pi = None  # pigpioオブジェクト
        # pigpioのセットアップ
        if os.name == 'posix':  # ラズパイで動かした場合にはpigpioをインポート
            import pigpio
            self.pi = pigpio.pi()
            self.pi.set_mode(PIN_SERVO1, pigpio.OUTPUT)
            self.pi.set_mode(PIN_SERVO2, pigpio.OUTPUT)
            self.pi.set_mode(PIN_LED, pigpio.OUTPUT)

    def indicate(self, kmh=None, amp=None, led=None):
        if self.pi:
            if kmh != None:
                kmh = SPEED_MAX if (kmh > SPEED_MAX) else kmh  # constrain upbound and lowbound
                kmh = 0         if (kmh < 0)         else kmh
                self.pi.hardware_PWM(PIN_SERVO1, 50, int(SERVO_MIN + kmh/SPEED_MAX * (SERVO_MAX - SERVO_MIN)))  # 速度計
            if amp != None:
                amp =  IMM_MAX if (amp > IMM_MAX)  else amp
                amp = -IMM_MAX if (amp < -IMM_MAX) else amp
                self.pi.hardware_PWM(PIN_SERVO2, 50, int(SERVO_MIN + 0.5*(1 + amp/IMM_MAX) * (SERVO_MAX - SERVO_MIN)))  # 電流計
            if led != None:
                self.pi.write(PIN_LED, led)

class SerialCom():
    def __init__(self, meterObj=None):
        self.ser = None   # シリアル通信オブジェクト
        self.rxdata = {}  # 受信したデータを入れておく辞書型変数。外部からこれにアクセスすることでデータを取り出す
        self.flagrx = True  # Trueの間シリアル通信を実行
        self.t1 = None  # シリアルの受信を行うThreadingオブジェクト
        self.METERS = meterObj  # 速度を表示するMetersオブジェクトへの参照をセット  # Metersオブジェクトへの参照

        # MCUが接続されていればシリアルポートをオープン
        print("[serialcom.__init__] open serial port")
        if isMCUConnected:
            try:
                # OSによってポートを切り替え
                if os.name == 'posix':
                    portpath = SERIALPATH_RASPI
                elif os.name == 'nt':
                    portpath = SERIALPATH_WIN

                # ポートを開く
                self.ser = serial.Serial(portpath, 115200, timeout=None)

            # ポートオープン失敗時
            except serial.serialutil.SerialException:
                print("[serialcom.__init__] failed to open port")
                self.rxdata = {"serialfailed":1}
        
        else:
            print("[serialcom.__init__] port wasn't opened because isMCUConnected==False.")
    
    def recieve_loop(self):
        # シリアルポートから受信を行う無限ループ
        if self.ser:
            print("[serialcom.recieve_loop] start recieving")
            self.ser.readline() # 1回目は不完全なデータなので空読み
            while self.flagrx:
                rxbuf = self.ser.readline().decode('ascii','ignore')
                print(rxbuf)
                try:
                    self.rxdata = json.loads(rxbuf)  # JSON形式へデコード
                    self.rxdata['serialfailed'] = 0
                    if self.METERS:  # メーターに表示
                        self.METERS.indicate(self.rxdata['speed'], self.rxdata['Imm'], self.rxdata['invstate'])
                except json.decoder.JSONDecodeError:
                    print("[serialcom.recieve_loop] when decoding, error has occured")
                    self.rxdata['serialfailed'] = 1
            self.ser.close()

        # シリアルポートが開いていないときは、 rxdataとしてRXBUF0を代入する
        else:
            print("[serialcom.recieve_loop] Because MCU is not connected, RXBUF0 is set to rxdata.")
            self.rxdata = json.loads(RXBUF0)
            self.rxdata['serialfailed'] = 0
            while self.flagrx:
                time.sleep(0.5)
        
        print("[serialcom.recieve_loop] end recieving")

    def recieve_start(self):
        if not(self.t1):
            self.flagrx = True
            self.t1 = threading.Thread(target=self.recieve_loop, daemon=True)
            self.t1.start()

    def recieve_end(self):
        if self.t1:
            self.flagrx = False
            self.t1.join()
            del self.t1
    
    def send(self, txbuf):
        if self.ser:
            print(bytes(txbuf,"ascii"))
            return self.ser.write(bytes(txbuf,"ascii"))
            
def main():
    class Mode(enum.IntEnum):
        DEMO = 0
        EBIKE = 1
        ASSIST = 2
    
    mode = Mode.DEMO  # 動作モード
    
    # メーターとシリアル通信のインスタンスを生成、初期化
    meters = Meters()
    meters.indicate(0, 0, 0)
    serialcom = SerialCom(meters)
    serialcom.recieve_start()

    # サーバを立てる
    api = responder.API()

    @api.route("/reset")
    def reset(req,resp):
        serialcom.send("invoff\n")

    @api.route("/info")
    def get_info(req,resp):
        resp.headers = {"Content-Type": "application/json; charset=utf-8"}
        resp.media = serialcom.rxdata

    @api.route("/cardata")
    def get_cardata(req,resp):
        text = open("static/cars/cardata.json", "r", encoding='utf-8').read()
        resp.headers = {"Content-Type": "application/json; charset=utf-8"}
        resp.text = text

    @api.route("/command")
    async def post_command(req,resp):
        data = await req.media()
        print(data)
        if 'carno' in data:
            serialcom.send("invoff\n")
            time.sleep(0.5)
            while serialcom.rxdata['invstate'] == 1:
                time.sleep(0.1)
            serialcom.send(f"carno={data['carno']}\n")
        if 'mode' in data:
            serialcom.send("invoff\n")
            time.sleep(0.5)
            while serialcom.rxdata['invstate'] == 1:
                time.sleep(0.1)
            serialcom.send(f"mode={data['mode']}\n")
        if 'notch' in data:
            if data['notch'] == 'P':
                serialcom.send("P\n")
            elif data['notch'] == 'N':
                serialcom.send("N\n")
            elif data['notch'] == 'B':
                serialcom.send("B\n")
            else:
                serialcom.send(f"notch={data['notch']}\n")

    @api.route("/")
    def hello_html(req,resp):
        resp.html = api.template('index.html')

    # web server start
    api.run(address='0.0.0.0', port=5042)  # 0.0.0.0にすると外部からアクセスできる
    
    
if __name__ == '__main__':
    main()
