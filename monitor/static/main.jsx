const lampTextList = {power:'インバータ電源', pedal:'ペダル', assist:'アシスト', regen:'回生', alleb:'全電気ブレーキ', battLB:'バッテリ低電圧', trouble:'三相異常', comfailed:'通信異常', demo:'デモ'};
const initialCommand = {reset:true, serial:true, demo:true};
const cars = {
    "東武100系":{
        "desc":"東武鉄道が1990年から営業運転を開始した特急型車両。うんたらかんたら……"
    },
    "ダミー":{
        "desc":"こんな風に車両の説明がある"
    },
    "ダミー copy":{
        "desc":"こんな風に車両の説明がある"
    },
    "ダミー copy 2":{
        "desc":"こんな風に車両の説明がある"
    }}
const carlist = Object.keys(cars);  // 車両の名前一覧
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
        result.assist = (info.Tw > 0.0);
        result.regen = (info.Tw < 0.0);
        result.alleb = (info.Tw < 0.0 && info.Vs < 0.0);
        result.battLB = (info.Vdc < 36.0);
        return result;
    }
}

class App extends React.Component {
    constructor(props) {
        super(props);
        this.handleButtonClick = this.handleButtonClick.bind(this);
        this.handleCarChange = this.handleCarChange.bind(this);
        this.state = {
            info: null,  // インバータの動作状態(マイコン→HTML)
            command: initialCommand,  // 指令(HTML→マイコン)
            comfailed: false,  // 通信に失敗した際trueになるフラグ
            demo: true,  // デモモード
            carname: carlist[0],    // 現在選択している車両の名前
            maincontents: '主回路動作状況'  // モニタに表示しているコンテンツ
        }
    }

    // 情報をサーバから取得し、stateを更新する関数
    getInfo() {
        fetch('/info')
        .then(res => res.json())
        .then(
            (result) => {
                if (result.serialfailed) {
                    this.setState({comfailed: true});
                } else {
                    result = judgeCurrentState(result);
                    this.setState({comfailed: false, info: result});
                }
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

    // フッターでボタンが押されたときの処理
    handleButtonClick(id) {
        switch (id) {
            case 'carselectbutton':
                this.setState({maincontents: '車両選択'}); break;
            case 'statusbutton':
                this.setState({maincontents: '主回路動作状況'}); break;
            default:
                break;
        }
    }

    // 車両を変更したときの処理
    handleCarChange(name) {
        this.state.carname = name;
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
        this.setState({command: initialCommand});
        this.postCommand();
    }

    render() {
        return (
            <div id="app">
                <IndicatorArea info={this.state.info}
                    comfailed={this.state.comfailed}
                    demo={this.state.demo}/>
                <MonitorArea info={this.state.info}
                    comfailed={this.state.comfailed}
                    carname={this.state.carname}
                    maincontents={this.state.maincontents}
                    handleButtonClick={this.handleButtonClick}
                    onCarChange={this.handleCarChange}/>
            </div>
        );
    }
}

class IndicatorArea extends React.Component {
    renderLamp(keyname, color) {
        switch (keyname) {
            case 'comfailed':
                var is_on = this.props.comfailed;  break;
            case 'demo':
                var is_on = this.props.demo;  break;
            default:  // その他のランプは、comfailed時は消灯
                if (this.props.comfailed) {
                    var is_on = false;
                } else {
                    if (this.props.info) {
                        var is_on = this.props.info[keyname];
                    } else {
                        var is_on = true;
                    }
                }
                break;
        }
        return <IndicatorLamp keyname={keyname} is_on={is_on} color={color}/>;
    }
    render() {
        return (
            <div id="indicatorarea" className="bg_black">
                {this.renderLamp('power', 'indicator_green')}
                {this.renderLamp('pedal', 'indicator_orange')}
                {this.renderLamp('assist', 'indicator_green')}
                {this.renderLamp('regen', 'indicator_orange')}
                {this.renderLamp('alleb', 'indicator_orange')}
                {this.renderLamp('battLB', 'indicator_orange')}
                {this.renderLamp('trouble', 'indicator_red')}
                {this.renderLamp('comfailed', 'indicator_red')}
                {this.renderLamp('demo', 'indicator_orange')}
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
            return (<SelectCars onCarChange={this.props.onCarChange}/>)
        } else {
            return null
        }
    }
    render() {
        return (
            <div id="monitorarea" className="bg_black">
                <Header carname={this.props.carname}/>
                <div id="title" className="valign bg_title">
                    <span>◆{this.props.maincontents}◆</span>
                </div>
                {this.renderMainContents()}
                <Footer maincontents={this.props.maincontents} onButtonClick={this.props.handleButtonClick}/>
            </div>
        )
    }
}

class Header extends React.Component {
    render() {
        const next_sta = "本郷三丁目";
        const kilotei = "4.3km";
        return (
            <div id="header" className="bg_medium">
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
            var Pw = '--';
            var pulsemode = '--';
        } else {
            var soc = Math.round(this.props.info['soc']);
            var Vdc = Math.round(this.props.info['Vdc']*10)/10;
            var fs = Math.round(this.props.info['fs']*10)/10;
            var Vs = Math.round(this.props.info['Vs']*100);
            var fc = Math.round(this.props.info['fc']);
            var frand = Math.round(this.props.info['frand']);
            var Tw = this.props.info['Tw'];
            var fw = fs/pp/gear;    // 歯車を通った後の車輪の回転周波数
            var rpm = Math.round(fw*60*10)/10;
            var Pw = Math.round(Tw*2*PI*fw*10)/10;
            var pulsemode = this.props.info['pulsemode'];
            if (pulsemode == 0) {
                pulsemode = '非同期';
            } else {
                pulsemode = '同期 '+pulsemode+' パルス';
            }
        }
        return (
            <div id="operationstatus" className="bg_medium">
                <div id="status_dcac">
                    <span className="content_title">DC</span>
                    <InfoElement title={'バッテリ残量'} value={soc+' %'}/>
                    <InfoElement title={'入力電圧'} value={Vdc+' V'}/>
                    <span className="content_title">AC</span>
                    <InfoElement title={'主電動機回転数'} value={rpm+' rpm'}/>
                    <InfoElement title={'主電動機出力'} value={Pw+' W'}/>
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
    constructor(props) {
        super(props);
        this.handlePrev = this.handlePrev.bind(this);
        this.handleNext = this.handleNext.bind(this);
        this.state = {pageIndex: 0};  // 何ページ目を表示しているか
    }
    handlePrev() {  // ひとつ前のページを表示
        this.setState({pageIndex: this.state.pageIndex-1});
    }
    handleNext() {  // ひとつ後のページを表示
        this.setState({pageIndex: this.state.pageIndex+1});
    }
    renderCarCard(carname, pos) {
        if (carname) {
            return (<CarCard name={carname} pos={pos} onCarChange={this.props.onCarChange}/>)
        } else {
            return null
        }
    }
    render() {
        var pageIndex = this.state.pageIndex;
        var disable_prev = (pageIndex == 0);  // 最初のページに居る場合、戻るボタンを無効化
        var disable_next = (pageIndex == Math.floor((carlist.length-1)/3));  // 最後のページに居る場合、進むボタンを無効化
        var car0 = (carlist[pageIndex*3])? carlist[pageIndex*3] : null;      // 1枚目の車両名
        var car1 = (carlist[pageIndex*3+1])? carlist[pageIndex*3+1] : null;  // 2枚目〃
        var car2 = (carlist[pageIndex*3+2])? carlist[pageIndex*3+2] : null;  // 3枚目〃
        return (
            <div id="selectcars" className="bg_medium">
                <button id="car_prev" onClick={this.handlePrev} disabled={disable_prev}>◀</button>
                {this.renderCarCard(car0, 0)}
                {this.renderCarCard(car1, 1)}
                {this.renderCarCard(car2, 2)}
                <button id="car_next" onClick={this.handleNext} disabled={disable_next}>▶</button>
            </div>
        )
    }
}
class CarCard extends React.Component {
    render() {
        const imgurl = '/static/cars/' + this.props.name + '.jpg';
        const description = cars[this.props.name].desc;
        return (
            <div className={"carcard carcard_"+this.props.pos+" bg_light"}>
                <img className="carimg" src={imgurl}></img>
                <div className="carname valign"><span>{this.props.name}</span></div>
                <div className="cardescription"><span>{description}</span></div>
                <CarPicker name={this.props.name} onCarChange={this.props.onCarChange}/>
            </div>
        )
    }
}
class CarPicker extends React.Component {
    constructor(props) {
        super(props);
        this.handleClick = this.handleClick.bind(this);
    }
    handleClick() {
        this.props.onCarChange(this.props.name);
    }
    render() {
        return (
            <button className="carpicker" onClick={this.handleClick}>この車両を選択</button>
        )
    }
}

class Footer extends React.Component {
    constructor(props) {
        super(props);
        this.handleClick = this.handleClick.bind(this);
    }
    // フッターのボタンがクリックされたとき、どのボタンがクリックされたかをonButtonClickに渡す
    handleClick(e) {
        this.props.onButtonClick(e.target.id);
    }
    render() {
        const carselect_active = (this.props.maincontents == '車両選択')? "btn_on":"";
        const status_active = (this.props.maincontents == '主回路動作状況')? "btn_on":"";
        return (
            <div id="footer" className="bg_light">
               <button id="statusbutton" className={status_active} onClick={this.handleClick}>動作状況</button>
               <button id="carselectbutton" className={carselect_active} onClick={this.handleClick}>車両選択</button>
            </div>
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