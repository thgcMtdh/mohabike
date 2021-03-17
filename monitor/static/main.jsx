const lampTextList = {power:'インバータ電源', pedal:'ペダル', assist:'アシスト', regen:'回生', alleb:'全電気ブレーキ', battLB:'バッテリ低電圧', trouble:'三相異常', comfailed:'通信異常'};
const initialCommand = {reset:true, serial:true};

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
        this.state = {
            comfailed: false,
            info: null,  // インバータの動作状態(マイコン→HTML)
            command: initialCommand  // 指令(HTML→マイコン)
        }
    }

    componentDidMount() {
        // 情報を取得し、stateを更新
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

        // 読み込みが完了したので、指令を送信
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

    render() {
        return (
            <div>
                <IndicatorArea info={this.state.info} comfailed={this.state.comfailed}/>
                <MonitorArea info={this.state.info}/>
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
            <div className="indicatorarea">
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
            <div className={"indicatorlamp lamp_"+this.props.is_on+" "+this.props.color}>
                {lampTextList[this.props.keyname]}
            </div>
        );
    }
}
class MonitorArea extends React.Component {
    render() {
        return null;
    }
}

ReactDOM.render(<App/>, document.querySelector('#route'));