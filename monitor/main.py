import time
import responder

# instead of serial com read text file
rxbuf = open("rxdata.json", "r").read().replace("\n","")
print(rxbuf)

# background task control flag
flagRun = True

# web server start
api = responder.API()

@api.background.task
def recievedata():
    while (flagRun):
        time.sleep(2)
        print("serial...")
            
@api.route("/")
def hello_html(req,resp):
    resp.html = api.template('index.html')
    #recievedata()

@api.route("/info")
def send_speed(req,resp):
    resp.text = rxbuf

@api.route("/stopserial")
def stop_serial(req,resp):
    global flagRun
    flagRun = False
    resp.text = f"stop serial"

if __name__ == '__main__':
    api.run(address='0.0.0.0', port=5042)  # 0.0.0.0にすると外部からアクセスできる
