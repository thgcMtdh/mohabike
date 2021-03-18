const lampTextList = {power:'インバータ電源', pedal:'ペダル', assist:'アシスト', regen:'回生', alleb:'全電気ブレーキ', battLB:'バッテリ低電圧', trouble:'三相異常', comfailed:'通信異常'};
const initialCommand = {reset:true, serial:true};
const carlist = ['東武100系', 'ダミー'];
const interval = 1000;  // 情報を取得する時間間隔[ms]
const PI = 3.141592653589;
const pp = 16;          // 極対数
const gear = 8.0;       // ギア比
const r = 27*0.0254/2;  // 車輪半径

function judgeCurrentState(info) {
    if (!info) {
        return null;
    } else {
        // マイコンから受け取ったinfoには、数値情報とpower,pedal,troubleが含まれる
        var result = info;
        // 数値情報をもとに各種stateを判断
        result.assist = (info.Tm > 0.0);
        result.regen = (info.Tm < 0.0);
        result.alleb = (info.Tm < 0.0 && info.Vs < 0.0);
        result.battLB = (info.Vdc < 36.0);
        return result;
    }
}

class App extends React.Component {
    constructor(props) {
        super(props);
        this.handleCarChange = this.handleCarChange.bind(this);
        this.state = {
            info: null,  // インバータの動作状態(マイコン→HTML)
            command: initialCommand,  // 指令(HTML→マイコン)
            comfailed: false,  // 通信に失敗した際trueになるフラグ
            carindex: 0,    // 現在選択している車両のid
            maincontents: '主回路動作状況'  // モニタに表示しているコンテンツ
        }
    }

    // 情報をサーバから取得し、stateを更新する関数
    getInfo() {
        fetch('/info')
        .then(res => res.json())
        .then(
            (result) => {
                result = judgeCurrentState(result);
                this.setState({comfailed: false, info: result});
            },
            (error) => {
                console.log(error);
                this.setState({comfailed: true});
            }
        );
    }

    // サーバに命令を送信する関数
    postCommand() {
        fetch('/command', {
            method: 'POST',
            headers: {'Content-Type':'application/json'},
            body: JSON.stringify(this.state.command)
        })
        .then(res => res.json())
        .then(
            (result) => {
                this.setState({comfailed: false})
            },
            (error) => {
                console.log(error);
                this.setState({comfailed: true})
            }
        )
    }

    // 車両を選択ボタンが押されたときの処理
    handleCarChange() {
       if (this.state.maincontents == '主回路動作状況') {
           this.setState({maincontents: '車両選択'});
       } else {
           this.setState({maincontents: '主回路動作状況'});
       }
    }

    componentDidMount() {
        // 指令を送信し、マイコンを立ち上げ
        this.postCommand();
        
        // 1秒おきに情報を取得するよう、タイマーを設定
        this.timerID = setInterval(() => {
            this.getInfo()
        }, interval);
    }

    componentWillUnmount() {
        clearInterval(this.timerID);
        this.setState({command: {reset:false, serial:false}});
        this.postCommand();
    }

    render() {
        const carname = carlist[this.state.carindex];
        return (
            <div id="app">
                <IndicatorArea info={this.state.info} comfailed={this.state.comfailed}/>
                <MonitorArea info={this.state.info} comfailed={this.state.comfailed} carname={carname} maincontents={this.state.maincontents} handleCarChange={this.handleCarChange}/>
            </div>
        );
    }
}

class IndicatorArea extends React.Component {
    renderLamp(keyname, color) {
        if (!this.props.info) {  // infoを受け取っていない場合、すべてのランプを点灯
            var is_on = true;
        } else {
            if (keyname == 'comfailed') {
                var is_on = this.props.comfailed;
            } else {
                var is_on = this.props.info[keyname];
            }
        }
        return <IndicatorLamp keyname={keyname} is_on={is_on} color={color}/>;
    }
    render() {
        return (
            <div id="indicatorarea" className="bg_black">
                {this.renderLamp('power', 'indicator_green')}
                {this.renderLamp('pedal', 'indicator_orange')}
                {this.renderLamp('assist', 'indicator_orange')}
                {this.renderLamp('regen', 'indicator_green')}
                {this.renderLamp('alleb', 'indicator_orange')}
                {this.renderLamp('battLB', 'indicator_orange')}
                {this.renderLamp('trouble', 'indicator_red')}
                {this.renderLamp('comfailed', 'indicator_red')}
            </div>
        )
    }
}
class IndicatorLamp extends React.Component {
    render() {
        return (
            <div className={"valign "+"indicatorlamp lamp_"+this.props.is_on+" "+this.props.color}>
                <span>{lampTextList[this.props.keyname]}</span>
            </div>
        )
    }
}

class MonitorArea extends React.Component {
    renderMainContents() {
        if (this.props.maincontents == '主回路動作状況') {
            return (<OperationStatus info={this.props.info} comfailed={this.props.comfailed}/>)
        } else if (this.props.maincontents == '車両選択') {
            return (<SelectCars/>)
        } else {
            return null
        }
    }
    render() {
        return (
            <div id="monitorarea" className="bg_black">
                <Header carname={this.props.carname} maincontents={this.props.maincontents} onCarChange={this.props.handleCarChange}/>
                <div id="title" className=" valign bg_title">
                    <span>◆{this.props.maincontents}◆</span>
                </div>
                {this.renderMainContents()}
                <Footer/>
            </div>
        )
    }
}

class Header extends React.Component {
    constructor(props) {
        super(props);
        this.handleClick = this.handleClick.bind(this);
    }
    handleClick(e) {
        this.props.onCarChange(e.target.value);
    }
    render() {
        const next_sta = "本郷三丁目";
        const kilotei = "4.3km";
        var buttontext = (this.props.maincontents == '主回路動作状況')? '車両\r\n選択' : '動作\r\n状況';
        return (
            <div id="header" className="bg_medium">
                <button id="carselectbutton" onClick={this.handleClick}>{buttontext}</button>
                <div id="carname" className="valign">
                    <span>{this.props.carname}</span>
                </div>
                <div id="headerinfos">
                    <InfoElement title={"次駅"} value={next_sta} r={3}/>
                    <InfoElement title={"キロ程"} value={kilotei} r={3}/>
                </div>
                <Clock/>
            </div>
        )
    }
}
class Clock extends React.Component {
    constructor(props) {
        super(props);
        this.state = {date: new Date()};
    }
    tick() {
        this.setState({date: new Date()});
    }
    componentDidMount() {
        this.timerID = setInterval(() => {  // 1秒おきに時刻を更新
            this.tick()
        }, 1000);
    }
    componentWillUnmount() {
        clearInterval(this.timerID);
    }

    render() {
        const dayOfWeekStr = ["(日)", "(月)", "(火)", "(水)", "(木)", "(金)", "(土)"];
        return (
            <div id="clock">
                <div className="valign clocktxt">
                    <span>{this.state.date.toLocaleDateString()}</span>
                </div>
                <div className="valign clocktxt">
                    <span>{dayOfWeekStr[this.state.date.getDay()]}</span>
                </div>
                <div className="valign clocktxt">
                    <span>{this.state.date.toLocaleTimeString()}</span>
                </div>
            </div>
        )
    }
}

class OperationStatus extends React.Component {
    render() {
        if (!this.props.info || this.props.comfailed) {
            var soc = '--';
            var Vdc = '--';
            var fs = '--';
            var Vs = '--';
            var fc = '--';
            var frand = '--';
            var rpm = '--';
            var Pm = '--';
            var pulsemode = '--';
        } else {
            var soc = Math.round(this.props.info['soc']);
            var Vdc = Math.round(this.props.info['Vdc']*10)/10;
            var fs = Math.round(this.props.info['fs']*10)/10;
            var Vs = Math.round(this.props.info['Vs']*100);
            var fc = Math.round(this.props.info['fc']);
            var frand = Math.round(this.props.info['frand']);
            var Tm = this.props.info['Tm'];
            var fm = fs/pp/gear;    // 歯車を通った後の車輪の回転周波数
            var rpm = Math.round(fm*60*10)/10;
            var Pm = Math.round(Tm*2*PI*fm*10)/10;
            var pulsemode = this.props.info['pulsemode'];
            if (pulsemode == 0) {
                pulsemode = '非同期';
            } else {
                pulsemode = '同期'+pulsemode+'パルス';
            }
        }
        return (
            <div id="operationstatus" className="bg_medium">
                <div id="status_dc">
                    <span className="content_title">DC</span>
                    <InfoElement title={'バッテリ残量'} value={soc+' %'}/>
                    <InfoElement title={'入力電圧'} value={Vdc+' V'}/>
                </div>
                <div id="status_ac">
                    <span className="content_title">AC</span>
                    <InfoElement title={'主電動機回転数'} value={rpm+' rpm'}/>
                    <InfoElement title={'主電動機出力'} value={Pm+' W'}/>
                </div>
                <div id="status_vvvf">
                    <span className="content_title">VVVF</span>
                    <InfoElement title={'信号波周波数'} value={fs+' Hz'}/>
                    <InfoElement title={'信号波変調率'} value={Vs+' %'}/>
                    <InfoElement title={'パルスモード'} value={pulsemode}/>
                    <InfoElement title={'搬送波周波数'} value={fc+' Hz'}/>
                    <InfoElement title={'ランダム変調'} value={frand+' Hz'}/>
                </div>
            </div>
        )
    }
}

class SelectCars extends React.Component {
    render() {
        return (
            <div id="selectcars" className="bg_medium"></div>
        )
    }
}

class Footer extends React.Component {
    render() {
        return (
            <div id="footer" className="bg_light"></div>
        )
    }
}

class InfoElement extends React.Component {
    render() {
        if (!this.props.r) {  // r(infotitleが全体の横幅に占める割合) を取得
            var r = 5;
        } else {
            var r = this.props.r;
        }
        return (
            <div className="infoelement">
                <div className="infotitle valign" style={{width: r*10 + '%'}}>
                    <span>{this.props.title}</span>
                </div>
                <div className="infovalue valign bg_dark" style={{width: 100-r*10 + '%'}}>
                    <span>{this.props.value}</span>
                </div>
            </div>
        )
    }
}

ReactDOM.render(<App/>, document.getElementById('route'));