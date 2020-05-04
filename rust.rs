
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
struct Token {
	symbol: usize,
	range: std::ops::Range
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
struct Parser {
	symbols: Vec<Symbol>,
	productions: Vec<Production>,
	stack: Vec<Item>,
	action: Option<Action>,
	result: Option<Vec<Node>>
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
			],
			stack: vec![Item{state: 0, nodes: None}],
			action: None,
			result: None
		}
	}

	fn get_action(state: usize, symbol: usize) -> Action { 
		match state {
			/*(-on unicc.states)*/
			/*(loop.index0)*/ => {
				match symbol {
					/*(on transitions)*/
					/*(symbol)*/ => Action::
						/*(-on action == "shift")*/Shift(/*(state)*/)
						/*(-or action == "reduce")*/Reduce(/*(production)*/)
						/*(-or)*/ShiftAndReduce(/*(production)*/)
						/*(-go-)*/,
					/*(go)*/
					_ => Action::Error
				}
			},
			/*(-go)*/
			_ => Action::Error
		}
	}

	fn next(&mut self, token: &Token) -> Action {
		// Reduce until shift occurs
		while let Some(Action::Reduce(prod) | Action::ShiftAndReduce(prod)) = self.action {
			let mut nodes: Vec<Node> = Vec::new();
			
			// Reduce number of production items from the stack, collect AST nodes
			for _ in 0..self.productions[prod].rhs.len() {
				if let Some(mut inodes) = self.stack.pop().unwrap().nodes {
					inodes.extend(nodes.drain(..));
					nodes = inodes;
				}
			}

			let nodes = if nodes.len() > 0 {
				Some(nodes)
			} else {
				None
			};

			// Make new AST node if necessary
			if let Some(emit) = self.productions[prod].emit {
				nodes = Some(vec![Node{
					emit: emit,
					children: nodes
				}])
			}

			// Check if goal-symbol was reduced and stack is empty
			let lhs = self.productions[prod].lhs;

			if self.stack.len() == 0 && lhs == /*(unicc.grammar.goal)*/ {
				println!("Goal symbol reduced");

				self.result = nodes;
				return Action::Accept;
			}

			// Get next action based on current state and reduced nonterminal
			self.action = Parser::get_action(self.stack.last().unwrap().state.unwrap(), symbol);

			// Push onto stack
			match self.action {
				Action::Shift(state) => self.stack.push(Item{state, nodes}),
				Action::ShiftAndReduce(_) => self.stack.push(Item{state, nodes}),
			}

			if let Some(state) = shift {
				self.stack.push(Item{state: shift, nodes: Some(nodes)});
			}
		}

		// Push 
		if let Some(state) = self.stack.pop().unwrap() {
			self.action = Parser::get_action(state, token.symbol);
		}
		
		// Return current action
		action
	}

	pub fn parse(&self) {
		
	}
}

fn main() {
	let parser = Parser::new();

	println!("{:?}", parser);
}
