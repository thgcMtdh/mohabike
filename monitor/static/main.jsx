const lampTextList = {
    power:'インバータ電源',
    pedal:'ペダル',
    assist:'アシスト',
    regen:'回生',
    battLB:'バッテリ低電圧',
    comfailed:'通信異常',
    communicating:'通信中',
    demo:'デモ'
};
const interval = 100;  // 情報を取得する時間間隔[ms]
const PI = 3.141592653589;
const gear = 112.6533;       // ギア比
const r = 26*0.0254/2;  // 車輪半径

class App extends React.Component {
    constructor(props) {
        super(props);
        this.handleButtonClick = this.handleButtonClick.bind(this);
        this.handleCarChange = this.handleCarChange.bind(this);
        this.state = {
            info: null,  // インバータの動作状態(マイコン→HTML)
            comfailed: false,  // 通信に失敗した際trueになるフラグ
            communicating: false,   // commandを送り、返信待ちのときtrue
            cardata: null,  // 車両一覧のデータ
            carname: '車両を選択してください', // 現在選択している車両の名前
            maincontents: '主回路動作状況',  // モニタに表示しているコンテンツ
            kilo: 0.0,
            nextsta: '本郷三丁目'
        }
    }

     // 車両データをサーバから取得し、stateを更新する関数
     getCarData() {
        this.setState({communicating: true});
        fetch('/cardata')
        .then(res => res.json())
        .then(
            (result) => {
                this.setState({cardata: result, communicating: false});
            },
            (error) => {
                console.log(error);
                this.setState({comfailed: true, communicating: false});
            }
        );
    }

    // 情報をサーバから取得し、stateを更新する関数
    getInfo() {
        fetch('/info')
        .then(res => res.json())
        .then(
            (result) => {
                this.setState({info: result})
                this.setState({kilo: this.state.kilo + result.speed*interval*0.001/3600})
            },
            (error) => {
                console.log(error);
                this.setState({comfailed: true});
            }
        );
    }

    // サーバに命令を送信する関数
    postCommand(command) {

        fetch('/command', {
            method: 'POST',
            headers: {'Content-Type':'application/json'},
            body: JSON.stringify(command)
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
            case 'demobutton':
                this.postCommand({mode: 0}); break;
            case 'actualbutton':
                this.postCommand({mode: 1}); break;
            case 'notchPbutton':
                this.postCommand({notch: 'P'}); break;
            case 'notchNbutton':
                this.postCommand({notch: 'N'}); break;
            case 'notchBbutton':
                this.postCommand({notch: 'B'}); break;
            case 'stopbutton':
                this.postCommand({invoff: 1}); break;
            default:
                break;
        }
    }

    // 車両を変更したときの処理
    handleCarChange(carno) {
        this.postCommand({carno: carno})
        this.state.maincontents = '主回路動作状況'
    }

    componentDidMount() { 
        // 車両データを取得       
        this.getCarData();
        // 一定時間おきに情報を取得するよう、タイマーを設定
        this.timerID = setInterval(() => {
            this.getInfo()
        }, interval);
    }

    componentWillUnmount() {
        clearInterval(this.timerID);
    }

    render() {
        return (
            <div id="app">
                <IndicatorArea info={this.state.info}
                    comfailed={this.state.comfailed}
                    communicating={this.state.communicating}/>
                <MonitorArea info={this.state.info}
                    comfailed={this.state.comfailed}
                    carname={this.state.carname}
                    cardata={this.state.cardata}
                    maincontents={this.state.maincontents}
                    kilo={this.state.kilo}
                    nextsta={this.state.nextsta}
                    handleButtonClick={this.handleButtonClick}
                    onCarChange={this.handleCarChange}
                    getCarData={this.getCarData}/>
            </div>
        );
    }
}

class IndicatorArea extends React.Component {
    renderLamp(keyname, color) {
        if (this.props.comfailed) {
            if (keyname == 'comfailed') {
                var is_on = true;
            } else {
                var is_on = false;
            }
        } else {
            if (this.props.info) {
                switch (keyname) {
                    case 'comfailed':
                        var is_on = false; break;
                    case 'communicating':
                        var is_on = this.props.communicating; break;
                    case 'demo': // demoランプは、デモモード時に点灯
                        var is_on = (this.props.info['mode'] == 0);  break;
                    case 'assist': // assistランプは、mode==EBIKE or ASSIST時に点灯
                        var is_on = (this.props.info['mode'] > 0 && this.props.info['Imm'] > 0);  break;
                    case 'regen':  // regenランプは、mode==EBIKE or ASSIST時に点灯
                        var is_on = (this.props.info['mode'] > 0 && this.props.info['Imm'] < 0);  break;
                    case 'power':
                        var is_on = Boolean(this.props.info['invstate']);  break;
                    case 'pedal':
                        var is_on = Boolean(this.props.info['pedal']);  break;
                    case 'battLB':
                        var is_on = this.props.info['Vdc'] < 36.0;  break;
                }
            } else {  // infoが来ていなかったら、全ランプを点灯(初期状態)
                var is_on = true;
            }
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
                {this.renderLamp('battLB', 'indicator_orange')}
                {this.renderLamp('comfailed', 'indicator_red')}
                {this.renderLamp('communicating', 'indicator_orange')}
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
            return (<SelectCars cardata={this.props.cardata} onCarChange={this.props.onCarChange}/>)
        } else {
            return null
        }
    }
    render() {
        if (this.props.info && this.props.info.carno >= 0) {
            var carname = this.props.cardata[this.props.info.carno].name;
        } else {
            var carname = '車両を選択してください';
        }
        return (
            <div id="monitorarea" className="bg_black">
                <Header carname={carname} kilo={this.props.kilo} nextsta={this.props.nextsta}/>
                <div id="title" className="valign bg_title">
                    <span>◆{this.props.maincontents}◆</span>
                </div>
                {this.renderMainContents()}
                <Footer info={this.props.info} maincontents={this.props.maincontents} onButtonClick={this.props.handleButtonClick}/>
            </div>
        )
    }
}

class Header extends React.Component {
    render() {
        return (
            <div id="header" className="bg_medium">
                <div id="carname" className="valign">
                    <span>{this.props.carname}</span>
                </div>
                <div id="headerinfos">
                    <InfoElement title={"次駅"} value={this.props.nextsta} r={3}/>
                    <InfoElement title={"キロ程"} value={Math.round(this.props.kilo*10)/10+' km'} r={3}/>
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
            var soc = '--'; //Math.round(this.props.info['soc']);
            var Vdc = (Math.round(this.props.info['Vdc']*10)/10).toFixed(1);
            var fs = (Math.round(this.props.info['fs']*10)/10).toFixed(1);
            var Vs = Math.round(this.props.info['Vs']*100);
            var fc = Math.round(this.props.info['fc']);
            var frand = Math.round(this.props.info['frand']);
            var rpm = Math.round(fs/gear*60);
            var Pw = '--'; //Math.round(Tw*2*PI*fw*10)/10;
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
                    <InfoElement title={'車輪回転数'} value={rpm+' rpm'}/>
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
    renderCarCard(carno, pos) {
        if (carno < this.props.cardata.length) {
            return (<CarCard carno={carno} cardata={this.props.cardata} pos={pos} onCarChange={this.props.onCarChange}/>)
        } else {
            return null
        }
    }
    render() {
        var pageIndex = this.state.pageIndex;
        var disable_prev = (pageIndex == 0);  // 最初のページに居る場合、戻るボタンを無効化
        var disable_next = (pageIndex == Math.floor((this.props.cardata.length-1)/3));  // 最後のページに居る場合、進むボタンを無効化
        var carno0 = pageIndex*3;  // 1枚目の車両id
        var carno1 = carno0 + 1;   // 2枚目
        var carno2 = carno0 + 2;   // 3枚目
        return (
            <div id="selectcars" className="bg_medium">
                <button id="car_prev" onClick={this.handlePrev} disabled={disable_prev}>◀</button>
                {this.renderCarCard(carno0, 0)}
                {this.renderCarCard(carno1, 1)}
                {this.renderCarCard(carno2, 2)}
                <button id="car_next" onClick={this.handleNext} disabled={disable_next}>▶</button>
            </div>
        )
    }
}
class CarCard extends React.Component {
    render() {
        const name = this.props.cardata[this.props.carno].name;
        const description = this.props.cardata[this.props.carno].desc;
        const imgurl = this.props.cardata[this.props.carno].imgurl;
        return (
            <div className={"carcard carcard_"+this.props.pos+" bg_light"}>
                <img className="carimg" src={imgurl}></img>
                <div className="carname valign"><span>{name}</span></div>
                <div className="cardescription"><span>{description}</span></div>
                <CarPicker carno={this.props.carno} onCarChange={this.props.onCarChange}/>
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
        this.props.onCarChange(this.props.carno);
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
        var status_class = (this.props.maincontents == '主回路動作状況')? "btn_on":"btn_off";
        var carselect_class = (this.props.maincontents == '車両選択')? "btn_on":"btn_off";
        var notch_disabled = "disabled";
        var mode_disabled = "disabled";
        if (this.props.info) {
            if (this.props.info.carno > -1) {  // 車両が選択されているときに限りノッチを有効化
                notch_disabled = "";
            }
            if (this.props.info.speed == 0) {  // 停止中に限りモード切替を有効化
                mode_disabled = "";
            }
            var demo_class = (this.props.info.mode == 0)? "btn_on":"btn_off";
            var actual_class = (this.props.info.mode > 0)? "btn_on":"btn_off"
            var notchP_class = (this.props.info.notch > 9)? "btn_on":"btn_off";
            var notchN_class = (this.props.info.notch == 9)? "btn_on":"btn_off";
            var notchB_class = (this.props.info.notch < 9)? "btn_on":"btn_off";
        } else {
            var demo_class = "";
            var actual_class = "";
            var notchP_class = "";
            var notchN_class = "btn_on";
            var notchB_class = "";
        }
        return (
            <div id="footer" className="bg_light">
               <button id="statusbutton" className={status_class} onClick={this.handleClick}>動作状況</button>
               <button id="carselectbutton" className={carselect_class} onClick={this.handleClick}>車両選択</button>
               <button disabled={mode_disabled} id="demobutton" className={demo_class} onClick={this.handleClick}>無負荷実演</button>
               <button disabled={mode_disabled} id="actualbutton" className={actual_class} onClick={this.handleClick}>実路面走行</button>
               <button disabled={notch_disabled} id="notchPbutton" className={notchP_class} onClick={this.handleClick}>P</button>
               <button disabled={notch_disabled} id="notchNbutton" className={notchN_class} onClick={this.handleClick}>N</button>
               <button disabled={notch_disabled} id="notchBbutton" className={notchB_class} onClick={this.handleClick}>B</button>
               <button id="stopbutton" onClick={this.handleClick}>停止</button>
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