#[derive(Debug)]
struct Symbol {
	name: &'static str,
	emit: Option<&'static str>,
	terminal: bool,
	whitespace: bool,
	lexem: bool
}

#[derive(Debug)]
struct Production {
	lhs: usize,
	emit: Option<&'static str>,
	rhs: Vec<usize>
}

#[derive(Debug)]
enum Action {
	Accept,
	Error,
	Shift(usize),
	Reduce(usize),
	ShiftAndReduce(usize)
}

#[derive(Debug)]
struct Node {
	emit: &'static str,
	children: Option<Vec<Node>>
}

#[derive(Debug)]
struct Item {
	state: usize,
	nodes: Option<Vec<Node>>
}

#[derive(Debug)]
struct State {
	stack: Vec<Item>,
	action: Action
}

#[derive(Debug)]
struct Parser {
	symbols: Vec<Symbol>,
	productions: Vec<Production>,
}

impl Parser {
	pub fn new() -> Self {
		Parser{
			symbols: vec![
				/*(-on unicc.grammar.symbols)*/
				Symbol{
					name: "/*(replace(name, "\\", "\\\\"))*/",
					emit: /*(on emit)*/Some("/*(replace(emit, "\\", "\\\\"))*/")/*(or)*/None/*(go)*/,
					terminal: /*(on "terminal" in loop.item.flags)*/true/*(or)*/false/*(go)*/,
					whitespace: /*(on "whitespace" in loop.item.flags)*/true/*(or)*/false/*(go)*/,
					lexem: /*(on "lexem" in loop.item.flags)*/true/*(or)*/false/*(go)*/,
				},
				/*(-go)*/
			],
			productions: vec![
				/*(-on unicc.grammar.productions)*/
				Production{
					lhs: /*(symbol)*/,
					emit: /*(on emit)*/Some("/*(replace(emit, "\\", "\\\\"))*/")/*(or)*/None/*(go)*/,
					rhs: vec![/*(on rhs)*//*(loop.item)*/, /*(go)*/]
				},
				/*(-go)*/
			]
		}
	}

	fn next(&self, state: &mut State) -> Action {
		while let Action::Reduce(prod) | Action::ShiftAndReduce(prod) = state.action {
			let mut cnodes: Vec<Node> = Vec::new();
			
			for _ in 0..self.productions[prod].rhs.len() {
				let mut item = state.stack.pop().unwrap();

				if let Some(mut nodes) = item.nodes {
					nodes.extend(cnodes.drain(..));
					cnodes = nodes;
				}
			}

			if let Some(emit) = self.productions[prod].emit {
				cnodes = vec![Node{
					emit: emit,
					children: if cnodes.len() > 0 {
						Some(cnodes)
					} else {
						None
					}
				}]
			}

			match state.stack.last().unwrap().state {
				/*(-on unicc.states)*/
				/*(-on [goto for goto in transitions if "terminal" not in unicc.grammar.symbols[int(goto.symbol)].flags])*/
				/*(-on loop.first)*/
				/*(loop.parent.index0)*/ => {
					match self.productions[prod].lhs {
						/*(-go loop first)*/
						/*(symbol)*/ => Action::
							/*(-on action == "shift")*/Shift(/*(state)*/)
							/*(-or action == "reduce")*/Reduce(/*(production)*/)
							/*(-or)*/ShiftAndReduce(/*(production)*/)
							/*(-go-)*/,
						/*(-on loop.last)*/
						_ => Action::Error
					}
				},
				/*(-go loop last)*/
				/*(-go-)*/
				/*(go)*/
				_ => Action::Error
			};
		}


		Action::Accept
	}

	pub fn parse(&self) {
		
	}
}

fn main() {
	let parser = Parser::new();

	println!("{:?}", parser);
}
