const lampTextList = {power:'アシスト電源', pedal:'ペダル', assist:'アシスト', regen:'回生', alleb:'全電気ブレーキ', battLB:'バッテリ低電圧', trouble:'三相異常'};

class App extends React.Component {
    constructor(props) {
        super(props);
        this.state = {
            error: null,
            info: null,  // インバータの動作状態(マイコン→HTML)
            command: null  // 指令(HTML→マイコン)
        }
    }

    componentDidMount() {
        fetch('/info')  // 情報を取得し、stateを更新
            .then(res => res.json())
            .then((result) => {
                this.setState({error: false, info: result});
            });
    }

    render() {
        return (
            <div>
                <IndicatorArea info={this.state.info}/>
                <MonitorArea info={this.state.info}/>
            </div>
        );
    }
}

class IndicatorArea extends React.Component {
    renderLamp(keyname) {
        if (!this.props.info) {
            return <IndicatorLamp keyname={keyname} is_on={true}/>;
        } else {
            return <IndicatorLamp keyname={keyname} is_on={this.props.info[keyname]}/>;
        }
    }
    render() {
        return (
            <div class="indicatorarea">
                {this.renderLamp('power')}
                {this.renderLamp('pedal')}
            </div>
        )
    }
}
class IndicatorLamp extends React.Component {
    render() {
        return (
            <div class={"indicatorlamp indicator_"+this.props.is_on}>
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