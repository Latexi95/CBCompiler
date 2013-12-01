#include "abstractsyntaxtree.h"
#include "astvisitor.h"

#include <QDebug>
#include <QByteArray>
#include <QFile>
#include <QTextStream>
#include <QObject>

namespace ast {


static printTabs(QTextStream &s, int tabs) {
	for (int i = 0; i < tabs; i++) {
		s << '\t';
	}
}

void Node::write(QTextStream &s, int tabs) {
	printTabs(s, tabs);
	s << "Node:" << typeAsString() << " {\n";
	for (int i = 0; i < childNodeCount(); i++) {
		childNode(i)->write(s, tabs + 1);
	}
	printTabs(s, tabs);
	s << "}\n";
}

const char *Node::typeAsString() const {
	static const char * const types[] = {
		"ntBlock",
		"ntInteger",
		"ntFloat",
		"ntString",
		"ntIdentifier",
		"ntType",
		"ntLabel",
		"ntList",
		"ntGoto",
		"ntGosub",
		"ntReturn",
		"ntExit",

		"ntDefaultType",
		"ntBasicType",
		"ntNamedType",
		"ntArrayType",

		"ntExpression",
		"ntExpressionNode",
		"ntUnary",
		"ntArraySubscript",
		"ntFunctionCall",
		"ntDefaultValue",
		"ntVariable",

		"ntIfStatement",
		"ntWhileStatement",
		"ntRepeatForeverStatement",
		"ntRepeatUntilStatement",
		"ntForToStatement",
		"ntForEachStatement",
		"ntSelectStatement",
		"ntCase",
		"ntDefault",

		"ntConst",
		"ntDim",
		"ntGlobal",
		"ntVariableDefinition",
		"ntArrayInitialization",
		"ntFunctionDefinition",
		"ntTypeDefinition",
		"ntProgram"

		"ntInvalid" };
	Type ty = type();
	if (ty < 0 || ty >= ntInvalid) return types[ntInvalid];
	return types[ty];

}

Node *Variable::childNode(int n) {
	switch (n) {
		case 0:
			return mIdentifier;
		case 1:
			return mType;
		default:
			assert("Invalid child node id" && 0);
			return 0;
	}
}

NODE_ACCEPT_VISITOR_DEF(Integer)
NODE_ACCEPT_VISITOR_DEF(Float)
NODE_ACCEPT_VISITOR_DEF(String)

NODE_ACCEPT_VISITOR_DEF(Identifier)
NODE_ACCEPT_VISITOR_DEF(Label)
NODE_ACCEPT_VISITOR_DEF(Return)
NODE_ACCEPT_VISITOR_DEF(Exit)
NODE_ACCEPT_VISITOR_DEF(DefaultType)
NODE_ACCEPT_VISITOR_DEF(BasicType)
NODE_ACCEPT_VISITOR_DEF(NamedType)
NODE_ACCEPT_VISITOR_DEF(ArrayType)
NODE_ACCEPT_VISITOR_DEF(Variable)
NODE_ACCEPT_VISITOR_DEF(ExpressionNode)
NODE_ACCEPT_VISITOR_DEF(Expression)
NODE_ACCEPT_VISITOR_DEF(List)
NODE_ACCEPT_VISITOR_DEF(FunctionCall)
NODE_ACCEPT_VISITOR_DEF(KeywordFunctionCall)
NODE_ACCEPT_VISITOR_DEF(ArraySubscript)
NODE_ACCEPT_VISITOR_DEF(DefaultValue)
NODE_ACCEPT_VISITOR_DEF(Unary)
NODE_ACCEPT_VISITOR_DEF(VariableDefinition)
NODE_ACCEPT_VISITOR_DEF(ArrayInitialization)
NODE_ACCEPT_VISITOR_DEF(Dim)
NODE_ACCEPT_VISITOR_DEF(Global)
NODE_ACCEPT_VISITOR_DEF(Redim)
NODE_ACCEPT_VISITOR_DEF(TypeDefinition)
NODE_ACCEPT_VISITOR_DEF(Block)
NODE_ACCEPT_VISITOR_DEF(IfStatement)
NODE_ACCEPT_VISITOR_DEF(WhileStatement)
NODE_ACCEPT_VISITOR_DEF(RepeatUntilStatement)
NODE_ACCEPT_VISITOR_DEF(RepeatForeverStatement)
NODE_ACCEPT_VISITOR_DEF(ForToStatement)
NODE_ACCEPT_VISITOR_DEF(ForEachStatement)
NODE_ACCEPT_VISITOR_DEF(SelectStatement)
NODE_ACCEPT_VISITOR_DEF(SelectCase)
NODE_ACCEPT_VISITOR_DEF(Const)
NODE_ACCEPT_VISITOR_DEF(FunctionDefinition)
NODE_ACCEPT_VISITOR_DEF(Goto)
NODE_ACCEPT_VISITOR_DEF(Gosub)
NODE_ACCEPT_VISITOR_DEF(Program)





QString ExpressionNode::opToString(ExpressionNode::Op op) {
	const static QString ops[] = {
		QT_TRANSLATE_NOOP("ExpressionNode", "assign"),
		QT_TRANSLATE_NOOP("ExpressionNode", "equal"),
		QT_TRANSLATE_NOOP("ExpressionNode", "not equal"),
		QT_TRANSLATE_NOOP("ExpressionNode", "greater"),
		QT_TRANSLATE_NOOP("ExpressionNode", "less"),
		QT_TRANSLATE_NOOP("ExpressionNode", "greater or equal"),
		QT_TRANSLATE_NOOP("ExpressionNode", "less or equal"),
		QT_TRANSLATE_NOOP("ExpressionNode", "add"),
		QT_TRANSLATE_NOOP("ExpressionNode", "subtract"),
		QT_TRANSLATE_NOOP("ExpressionNode", "multiply"),
		QT_TRANSLATE_NOOP("ExpressionNode", "power"),
		QT_TRANSLATE_NOOP("ExpressionNode", "modulo"),
		QT_TRANSLATE_NOOP("ExpressionNode", "shl"),
		QT_TRANSLATE_NOOP("ExpressionNode", "shr"),
		QT_TRANSLATE_NOOP("ExpressionNode", "sar"),
		QT_TRANSLATE_NOOP("ExpressionNode", "divide"),
		QT_TRANSLATE_NOOP("ExpressionNode", "logical and"),
		QT_TRANSLATE_NOOP("ExpressionNode", "logical or"),
		QT_TRANSLATE_NOOP("ExpressionNode", "logical xor"),
		QT_TRANSLATE_NOOP("ExpressionNode", "comma"),, // ,
		QT_TRANSLATE_NOOP("ExpressionNode", "member"),, // .
		QT_TRANSLATE_NOOP("ExpressionNode", "invalid"),
	};
	if (op >= ExpressionNode::opAssign && op < ExpressionNode::opInvalid) {
		return ops[op];
	}
	return ops[ExpressionNode::opInvalid];
}


QString Unary::opToString(Unary::Op op) {
	static QString ops[] = {
		QT_TRANSLATE_NOOP("Unary", "not"),
		QT_TRANSLATE_NOOP("Unary", "positive"),
		QT_TRANSLATE_NOOP("Unary", "negative"),
		QT_TRANSLATE_NOOP("Unary", "invalid")
	};
	if (op >= Unary::opNot && op < Unary::opInvalid) {
		return ops[op];
	}
	return ops[Unary::opInvalid];
}

IfStatement::~IfStatement() {
	if (mCondition) delete mCondition;
	if (mBlock) delete mBlock;
	if (mElse) delete mElse;
}

Node *IfStatement::childNode(int n) const {
	switch (n) {
		case 0: return mCondition;
		case 1: return mBlock;
		case 2: return mElse;
		default: assert("Invalid child node id" && 0);
	}
	return 0;
}

ForToStatement::~ForToStatement() {
	if (mFrom) delete mFrom;
	if (mTo) delete mTo;
	if (mStep) delete mStep;
	if (mBlock) delete mBlock;
}

Node *ForToStatement::childNode(int n) const {
	switch(n) {
		case 0:
			return mFrom;
		case 1:
			return mTo;
		case 2:
			if (mStep != 0) return mStep;
			return mBlock;
		case 3:
			if (mStep != 0) return mBlock;
		default:
			assert("Invalid child node id" && 0);
			return 0;
	}
}

ForEachStatement::~ForEachStatement() {
	if (mVariable) delete mVariable;
	if (mContainer) delete mContainer;
	if (mBlock) delete mBlock;
}

Node *ForEachStatement::childNode(int n) const {
	switch (n) {
		case 0:
			return mVariable;
		case 1:
			return mContainer;
		case 2:
			return mBlock;
		default:
			assert("Invalid child node id" && 0);
			return 0;
	}
}

FunctionDefinition::~FunctionDefinition() {
	if (mIdentifier) delete mIdentifier;
	if (mParameterList) delete mParameterList;
	if (mReturnType) delete mReturnType;
	if (mBlock) delete mBlock;
}

Node *FunctionDefinition::childNode(int n) const {
	switch(n) {
		case 0:
			return mIdentifier;
		case 1:
			return mParameterList;
		case 2:
			return mReturnType;
		case 3:
			return mBlock;
		default:
			assert("Invalid child node id" && 0);
			return 0;
	}
}

ArrayInitialization::~ArrayInitialization() {
	if (mIdentifier) delete mIdentifier;
	if (mType) delete mType;
	if (mDimensions) delete mDimensions;
}

Node *ArrayInitialization::childNode(int n) const {
	switch (n) {
		case 0:
			return mIdentifier;
		case 1:
			return mType;
		case 2:
			return mDimensions;
		default:
			assert("Invalid child node id" && 0);
			return 0;
	}
}

Node *Const::childNode(int n) const {
	switch (n) {
		case 0:
			return mIdentifier;
		case 1:
			return mValue;
		default:
			assert("Invalid child node id" && 0);
			return 0;
	}
}

SelectStatement::~SelectStatement() {
	if (mVariable) delete mVariable;
	qDeleteAll(mCases);
	if (mDefault) delete mDefault;
}

Node *SelectStatement::childNode(int n) const {
	if (n == 0) return mVariable;
	if (n > 0 && n <= mCases.size()) return mCases.at(n - 1);
	if (n == mCases.size() + 1 && mDefault) return mDefault;
	assert("Invalid child node id" && 0);
	return 0;
}

Program::~Program() {
	qDeleteAll(mFunctionDefinitions);
	qDeleteAll(mTypeDefinitions);
}

Node *Program::childNode(int n) const {
	assert("Invalid child node id" && (n >= 0 && n < childNodeCount()));
	if (n < mTypeDefinitions.size()) return mTypeDefinitions.at(n);
	n -= mTypeDefinitions.size();
	if (n < mFunctionDefinitions.size()) return mFunctionDefinitions.at(n);
	return mMainBlock;
}



ast::Node *VariableDefinition::getMValueType() const
{
	return mValueType;
}

void VariableDefinition::setMValueType(ast::Node *value)
{
	mValueType = value;
}

ast::Identifier *VariableDefinition::getMIdentifier() const
{
	return mIdentifier;
}

void VariableDefinition::setMIdentifier(ast::Identifier *value)
{
	mIdentifier = value;
}

VariableDefinition::~VariableDefinition() {
	if (mIdentifier) delete mIdentifier;
	if (mValueType) delete mValueType;
	if (mValue) delete mValue;
}

Node *VariableDefinition::childNode(int n) const {
	assert(n >= 0 && n < childNodeCount() && "Invalid child node id");
	switch (n) {
		case 0: return mIdentifier;
		case 1: return mValueType;
		case 2: return mValue;
	}
}

}
