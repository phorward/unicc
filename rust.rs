#[derive(Debug)]
struct Symbol {
	name: &'static str,
}

#[derive(Debug)]
struct Production {
	lhs: usize,
	emit: Option<&'static str>,
	rhs: Vec<usize>
}

#[derive(Debug)]
enum Action {
	Shift(usize),
	Reduce(usize),
	ShiftAndReduce(usize)
}

#[derive(Debug)]
struct State {
	default_reduce: Option<usize>,
	actions: Vec<Action>
}

#[derive(Debug)]
struct Parser {
	symbols: [Symbol; /**len(unicc.grammar.symbols)*/],
	productions: [Production; /**len(unicc.grammar.productions)*/],
	states: [State; /**len(unicc.states)*/]
}

fn main() {
	let parser = Parser{
		symbols: [
			/**-on unicc.grammar.symbols*/
			Symbol{name: "/**replace(name, "\\", "\\\\")*/"},
			/**-go*/
		],
		productions: [/**on unicc.grammar.productions*/
			Production{
				lhs: /**symbol*/,
				emit: /**on emit*/Some("/**emit*/")/**or*/None/**go*/,
				rhs: vec![/**on rhs*//**loop.item*/, /**go*/]
			},/**go*/
		],
		states: [/**on unicc.states*/
			State{
				default_reduce: 
					/**-on loop.item["reduce-default"] < 0 */ None
					/**-or*/ Some(/**loop.item["reduce-default"]*/)
					/**-go*/,
				actions: vec![
					/**-on transitions*/
					Action::
						/**-on action == "shift"*/Shift(/**state*/)
						/**-or action == "reduce"*/Reduce(/**production*/)
						/**-or*/ShiftAndReduce(/**production*/)
						/**-go*/,
					/**-go*/
				],
			},/**go*/
		]
	};

	println!("{:?}", parser);
}
