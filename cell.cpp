#include <cmath>

#include "expression_parser.hpp"

using std::cout;
using std::endl;

const double EPS = 1.0E-5;

Cell::Cell() :
	type(Type::NONE)
{
}

Cell::Cell(const Cell &c)
{
	*this = c;
}

Cell::~Cell()
{
	if(type == Type::FUNCTION) {
		for(auto i : func.args) {
			delete i;
		}
	}
}

Cell& Cell::operator=(const Cell &c)
{
	type = c.type;
	switch(type) {
	case Type::FUNCTION:
	{
		func.iter = c.func.iter;
		for(auto i : c.func.args) {
			func.args.push_back(new Cell(*i));
		}
		break;
	}
	case Type::VARIABLE:
	{
		var.name = c.var.name;
		break;
	}
	case Type::NUMBER:
	{
		val = c.val;
		break;
	}
	default:
	{
		break;
	}
	}
	return *this;
}

bool Cell::operator<(const Cell &c) const
{
	if((c.type == Type::FUNCTION) && (type == Type::FUNCTION)) {
		auto i1 = func.iter, i2 = c.func.iter;
		return (i1->name < i2->name) || ((i1->name == i2->name) && (i1->args_num < i2->args_num));
	} else if((c.type == Type::VARIABLE) && (type == Type::VARIABLE)) {
		return var.name < c.var.name;
	} else if((c.type == Type::NUMBER) && (type == Type::NUMBER)) {
		return val < c.val;
	} else if(((type == Type::VARIABLE) && (c.type == Type::NUMBER))
	          || (type == Type::FUNCTION)) {
		return true;
	} else {
		return false;
	}
}

bool Cell::operator==(const Cell &c) const
{
	if((type == Type::FUNCTION) && (c.type == Type::FUNCTION) && (func.iter == c.func.iter)) {
		bool ok = true;
		for(size_t i = 0; i < func.args.size(); ++i) {
			if(!(*func.args[i] == *c.func.args[i])) {
				ok = false;
			}
		}
		return ok;
	} else if((type == Type::VARIABLE) && (c.type == Type::VARIABLE)) {
		return var.name == c.var.name;
	} else if((type == Type::NUMBER) && (c.type == Type::NUMBER)) {
		return fabs(val - c.val) < EPS;
	} else {
		return false;
	}
}

void Cell::print() const
{
	if(type == Type::FUNCTION) {
		cout << "(";
		cout << func.iter->name;
		for(const auto &i : func.args) {
			cout << " ";
			i->print();
		}
		cout << ")";
	} else if(type == Type::VARIABLE) {
		cout << var.name;
	} else if(type == Type::NUMBER) {
		cout << val;
	}
}

double Cell::eval(const std::map <std::string, double> &vars)
{
	switch(type) {
	case Type::FUNCTION:
	{
		Args args;
		for(auto i : func.args) {
			args.push_back(i->eval(vars));
		}
		return func.iter->func(args);
		break;
	}
	case Type::VARIABLE:
	{
		auto it = vars.find(var.name);
		return it->second;
		break;
	}
	case Type::NUMBER:
	{
		return val;
		break;
	}
	default:
		throw ExpressionParserException("Attempt to evaluate cell of type \"NONE\"");
	}
}

bool Cell::isSubExpression(std::vector <Cell*> &curcell, bool &subtree_match) const
{
	// Invariant relation:
	// curcell - path from root to one of the leaves in a assumed subtree that
	// hasn't been matched yet
	if(type == Type::FUNCTION) {
		bool tsm;
		subtree_match = true;
		for(size_t i = 0; i < func.iter->args_num; ++i) {
			if(func.args[i]->isSubExpression(curcell, tsm)) {
				return true;
			}
			subtree_match &= tsm;
			// It's possible that recursive call has changed value of curcell, so we have
			// to restore it (i.e. push necessary arguments)
			auto cell = curcell[curcell.size() - 1];
			if((cell->type == Type::FUNCTION) && (i + 1 < func.iter->args_num)) {
				if(func.iter != cell->func.iter) {
					subtree_match = false;
				} else {
					curcell.push_back(cell->func.args[i + 1]);
					while(curcell[curcell.size() - 1]->type == Cell::Type::FUNCTION) {
						curcell.push_back(curcell[curcell.size() - 1]->func.args[0]);
					}

				}
			}
		}
		auto cell = curcell[curcell.size() - 1];
		if(subtree_match && (func.iter == cell->func.iter)) {
			curcell.pop_back();
			return (curcell.size() == 0);
		} else {
			subtree_match = false;
			while(cell->type == Cell::Type::FUNCTION) {
				curcell.push_back(cell = cell->func.args[0]);
			}
			return false;
		}
	} else if(curcell.size() > 1) {
		subtree_match = (*this == *curcell[curcell.size() - 1]);
		if(subtree_match) {
			curcell.pop_back();
		}
		return false;
	} else {
		subtree_match = (*this == *curcell[0]);
		if(subtree_match) {
			curcell.pop_back();
		}
		return subtree_match;
	}
}

void Cell::sort()
{
	if(type == Type::FUNCTION) {
		auto f = func.iter;
		if((f->args_num == 2) && f->is_commutative && (*func.args[1] < *func.args[0])) {
			std::swap(func.args[0], func.args[1]);
		}
		for(auto i : func.args) {
			i->sort();
		}
	}
}