﻿#include "parser.h"
#include "errorcodes.h"
#include "warningcodes.h"
#include <assert.h>
Parser::Parser():
	mStatus(Ok) {
}

typedef ast::Node *(Parser::*BlockParserFunction)(Parser::TokIterator &);
static BlockParserFunction blockParsers[] =  {
	&Parser::tryGotoGosubAndLabel,
	&Parser::tryDim,
	&Parser::tryRedim,
	&Parser::tryIfStatement,
	&Parser::tryWhileStatement,
	&Parser::tryRepeatStatement,
	&Parser::tryForStatement,
	&Parser::tryReturn,
	&Parser::trySelectStatement,
	&Parser::exceptExpression
};
static const int blockParserCount = 10;

ast::Block *Parser::expectBlock(Parser::TokIterator &i) {
	QList<ast::Node*> statements;
	CodePoint startCp = i->codePoint();
	while (true) {
		if (i->isEndOfStatement()) {
			i++;
			if (i->type() == Token::EndOfTokens) {
				i--;
				ast::Block *block = new ast::Block(startCp, i->codePoint());
				block->setChildNodes(statements);
				return block;
			}
			continue;
		}
		ast::Node * n = 0;
		for (int p = 0; p < blockParserCount; p++) {
			n =	(this->*(blockParsers[p]))(i);
			if (mStatus == Error) return 0;
			if (n) {
				expectEndOfStatement(i);
				if (mStatus == Error) return 0;
				break;
			}
		}
		if (n) {
			statements.append(n);
		}
		else {
			break;
		}
	}
	ast::Block *block = new ast::Block(startCp, i->codePoint());
	block->setChildNodes(statements);
	return block;
}

ast::Block Parser::expectInlineBlock(Parser::TokIterator &i) {
	ast::Block block;
	int line = i->mLine;
	while (i->mLine == line) {
		if (i->isEndOfStatement()) {
			i++;
			if (i->type() == Token::EndOfTokens) {
				i--;
				return block;
			}
			continue;
		}
		ast::Node * n;
		for (int p = 0; p < blockParserCount; p++) {
			n = (this->*(blockParsers[p]))(i);
			if (mStatus == Error) return ast::Block();
			if (n) {
				if (i->type() == Token::EOL || i->type() == Token::kElse || i->type() == Token::kElseIf) { // WTF CB?
					block.append(n);
					return block;
				}
				else {
					expectEndOfStatement(i);
					if (mStatus == Error) return ast::Block();
				}
				break;
			}
		}
		if (n) {
			block.append(n);
		}
		else {
			break;
		}
	}
	return block;
}

ast::Program *Parser::parse(const QList<Token> &tokens, const Settings &settings) {
	mSettings = settings;

	QList<ast::TypeDefinition*> typeDefs;
	QList<ast::Const*> constants;
	QList<ast::FunctionDefinition*> funcDefs;
	ast::Block *block = new ast::Block(tokens.first().codePoint(), tokens.last().codePoint());
	TokIterator i = tokens.begin();
	while (i->type() != Token::EndOfTokens) {
		if (i->isEndOfStatement()) {
			i++;
			continue;
		}
		ast::TypeDefinition *type = tryTypeDefinition(i);
		if (mStatus == Error) return 0;
		if (type) {
			program->mTypes.append(type);
			expectEndOfStatement(i);
			if (mStatus == Error) return 0;
			continue;
		}

		ast::FunctionDefinition *func = tryFunctionDefinition(i);
		if (mStatus == Error) return 0;
		if (func) {
			funcDefs.append(func);
			expectEndOfStatement(i);
			if (mStatus == Error) return 0;
			continue;
		}

		ast::Const *constant = tryConstDefinition(i);
		if (mStatus == Error) return 0;
		if (constant) {
			constants.append(constant);
			expectEndOfStatement(i);
			if (mStatus == Error) return 0;
			continue;
		}
		ast::Node * n;
		for (int p = 0; p < blockParserCount; p++) {
			n =	(this->*(blockParsers[p]))(i);
			if (mStatus == Error) return 0;
			if (n) {
				expectEndOfStatement(i);
				if (mStatus == Error) return 0;
				break;
			}
		}
		if (n) {
			block->appendNode(n);
		}
		else {
			emit error(ErrorCodes::ecUnexpectedToken, tr("Unexpected token \"%1\"").arg(i->toString()), i->codePoint());
			mStatus = Error;
			return 0;
		}
	}
	ast::Program *program = new ast::Program;
	program->setConstants(constants);
	program->setFunctionDefinitions(funcDefs);
	program->setTypeDefinitions(typeDefs);
	program->setMainBlock(block);

	return program;
}

ast::Const *Parser::tryConstDefinition(Parser::TokIterator &i) {
	if (i->type() == Token::kConst) {
		CodePoint cp = i->codePoint();
		i++;
		ast::Node *var = expectVariable(i);
		if (mStatus == Error) return 0;
		QString varType = tryVariableTypeDefinition(i);
		if (mStatus == Error) return 0;
		if (i->type() != Token::opEqual) {
			emit error(ErrorCodes::ecExpectingAssignment, tr("Expecting '=' after constant name, got \"%1\"").arg(i->toString()), i->codePoint());
			mStatus = Error;
			return 0;
		}
		i++;
		ast::Node *value = expectExpression(i);
		if (mStatus == Error) return 0;
		ast::Const *ret = new ast::Const(cp);
		ret->setVariable(var);
		ret->setValue(value);
		return ret;
	}
	return 0;
}

ast::Global *Parser::tryGlobalDefinition(Parser::TokIterator &i) {
	if (i->type() == Token::kGlobal) {
		int line = i->mLine;
		QString file = i->mFile;
		i++;
		ast::Variable *var = expectVariable(i);
		if (mStatus == Error) return 0;
		ast::GlobalDefinition *ret = new ast::GlobalDefinition;
		ret->mFile = file;
		ret->mLine = line;
		ret->mDefinitions.append(var);
		while (i->type() == Token::Comma) {
			i++;
			ast::Variable *var = expectVariable(i);
			if (mStatus == Error) return 0;
			ret->mDefinitions.append(var);
		}
		return ret;
	}

	//Not global
	return 0;
}

ast::Node *Parser::tryVariableTypeDefinition(Parser::TokIterator &i) {
	ast::Node *r;
	if (r = tryVariableTypeMark(i)) return r;
	if (i->type() == Token::kAs) {
		i++;
		return expectVariableTypeDefinition(i);
	}
}

ast::Node *Parser::tryVariableTypeMark(Parser::TokIterator &i) {
	switch (i->type()) {
		case Token::FloatTypeMark:
			return new ast::BasicType(ast::BasicType::Float, (i++)->codePoint());
		case Token::IntegerTypeMark:
			return new ast::BasicType(ast::BasicType::Integer, (i++)->codePoint());
		case Token::StringTypeMark:
			return new ast::BasicType(ast::BasicType::String, (i++)->codePoint());
		default:
			return 0;
	}
}

ast::Node *Parser::tryVariableAsType(Parser::TokIterator &i) {
	if (i->type() == Token::kAs) {
		i++;
		return expectIdentifierAfter(i, (i - 1)->toString()); //expecting an identifier after "As"
	}
	return QString();
}


ast::Node *Parser::tryReturn(Parser::TokIterator &i) {
	if (i->type() == Token::kReturn) {
		CodePoint cp = i->codePoint();
		i++;
		ast::Node *r = 0;
		if (!i->isEndOfStatement()) {
			r = expectExpression(i);
		}
		if (mStatus == Error) return 0;
		ast::Return *ret = new ast::Return(cp);
		ret->setValue(r);
		return ret;
	}
	return 0;
}

ast::TypeDefinition *Parser::tryTypeDefinition(Parser::TokIterator &i) {
	if (i->type() == Token::kType) {
		QString file = i->mFile;
		int line = i->mLine;
		i++;
		QString name = expectIdentifier(i);
		if (mStatus == Error) return 0;
		expectEndOfStatement(i);
		if (mStatus == Error) return 0;
		QList<QPair<int, ast::Variable*> > fields;
		while (i->isEndOfStatement()) i++;
		while (i->type() == Token::kField) {
			int line = i->mLine;
			i++;
			ast::Variable *field = expectVariable(i);
			if (mStatus == Error) return 0;
			fields.append(QPair<int, ast::Variable*> (line, field));
			expectEndOfStatement(i);
			if (mStatus == Error) return 0;

			while (i->isEndOfStatement()) i++;
		}
		if (i->type() != Token::kEndType) {
			emit error(ErrorCodes::ecExpectingEndType, tr("Expecting \"EndType\", got \"%1\"").arg(i->toString()), i->codePoint());
			mStatus = Error;
			return 0;
		}
		i++;

		ast::TypeDefinition *ret = new ast::TypeDefinition;
		ret->mFields = fields;
		ret->mFile = file;
		ret->mLine = line;
		ret->mName = name;
		return ret;
	}
	return 0;
}

ast::Identifier *Parser::expectIdentifier(Parser::TokIterator &i) {
	if (i->type() == Token::Identifier) {
		ast::Identifier *id = new ast::Identifier(i->toString(), i->codePoint());
		i++;
		return id;
	}

	emit error(ErrorCodes::ecExpectingIdentifier, tr("Expecting an identifier, got \"%1\"").arg(i->toString()), i->codePoint());
	mStatus = Error;
	return 0;
}

ast::Node *Parser::expectIdentifierAfter(Parser::TokIterator &i, const QString &after) {
	if (i->type() == Token::Identifier) {
		ast::Identifier *id = new ast::Identifier(i->toString(), i->codePoint());
		i++;
		return id;
	}

	emit error(ErrorCodes::ecExpectingIdentifier, tr("Expecting an identifier after \"%2\", got \"%1\"").arg(i->toString(), after), i->codePoint());
	mStatus = Error;
	return 0;
}

bool Parser::expectLeftParenthese(Parser::TokIterator &i) {
	if (i->type() != Token::LeftParenthese) {
		emit error(ErrorCodes::ecExpectingLeftParenthese, tr("Expecting a left parenthese after \"%1\"").arg((i - 1)->toString()), i->codePoint());
		mStatus = Error;
		return false;
	}
	i++;
	return true;
}

bool Parser::expectRightParenthese(Parser::TokIterator &i) {
	if (i->type() != Token::RightParenthese) {
		emit error(ErrorCodes::ecExpectingRightParenthese, tr("Expecting a right parenthese after \"%1\"").arg((i - 1)->toString()), i->codePoint());
		mStatus = Error;
		return false;
	}
	i++;
	return true;
}

ast::Node *Parser::expectDefinitionOfVariableOrArray(Parser::TokIterator &i) {
	CodePoint cp = i->codePoint();
	ast::Node *id = expectIdentifierAfter(i, (i - 1).toString());
	if (mStatus == Error) return 0;
	i++;
	ast::Node *varType = tryVariableTypeDefinition(i);
	if (mStatus == Error) return 0;
	if (i->type() == Token::LeftSquareBracket) { //Sized array
		i++;
		ast::Node *dims = expectExpressionList(i);

		if (i->type() != Token::RightSquareBracket) {
			emit error(ErrorCodes::ecExpectingRightParenthese, tr("Expecting right parenthese, got \"%1\"").arg(i->toString()), i->codePoint());
			mStatus = Error;
			return 0;
		}
		i++;
		ast::Node *varType2 = tryVariableAsType(i);
		if (varType == 0) {
			if (varType2 == 0) {
				if (variableTypesAreEqual(varType, varType2)) {
					emit error(ErrorCodes::ecVariableTypeDefinedTwice, tr("Variable \"%1\" type defined twice"), i->codePoint());
					mStatus = ErrorButContinue;
				}
				else {
					emit warning(WarningCodes::wcVariableTypeDefinedTwice, tr("Variable \"%1\" type defined twice"), i->codePoint());
				}
			}
		}
		else {
			varType = varType2;
		}
		ast::ArrayInitialization *arr = new ast::ArrayInitialization(cp);
		arr->setIdentifier(id);
		arr->setDimensions(dims);
		arr->setType(varType);
		return arr;
	}
	else {
		ast::Node *value = 0;
		if (i->type() == Token::opEqual) {
			i++;
			value = expectExpression(i);
			if (mStatus == Error) return 0;
		} else {
			value = new ast::DefaultValue(cp);
		}
		ast::VariableDefinition *def = new ast::VariableDefinition(cp);
		def->setIdentifier(id);
		def->setValueType(varType);
		def->setValue(value);
	}
}

ast::Node *Parser::expectPrimaryTypeDefinition(Parser::TokIterator &i) {
	switch (i->type()) {
		case Token::LeftParenthese: {
			i++;
			ast::Node *ty = expectVariableTypeDefinition(i);
			if (mStatus == Error) return 0;
			if (i->type() != Token::RightParenthese) {
				emit error(ErrorCodes::ecExpectingRightSquareBracket, tr("Expecting a right parenthese"), i->codePoint());
				mStatus = Error;
				delete ty;
				return 0;
			}
			++i;
		}
		case Token::Identifier: {
			ast::Identifier *id = new ast::Identifier(i->toString(), i->codePoint());
			ast::NamedType *namedType = new ast::NamedType(i->codePoint());
			namedType->setIdentifier(id);
			++i;
			return namedType;
		}
		default:
			emit error(ErrorCodes::ecExpectingPrimaryExpression, tr("Expecting a primary type expression after \"%1\"").arg((i - 1)->toString()), i->codePoint());
			mStatus = Error;
			return 0;
	}
}

ast::Node *Parser::expectArrayTypeDefinition(Parser::TokIterator &i) {
	ast::Node *base = expectPrimaryTypeDefinition(i);
	if (mStatus == Error) return 0;

	while(i->type() == Token::LeftSquareBracket) {
		CodePoint cp = i->codePoint();
		i++;
		int dims = 1;
		while ((i++)->type() == Token::Comma) ++dims;
		if (i->type() != Token::RightSquareBracket) {
			emit error(ErrorCodes::ecExpectingRightSquareBracket, tr("Expecting a right square bracket after ','"), i->codePoint());
			mStatus = Error;
			delete base;
			return 0;
		}
		++i;

		ast::ArrayType *arrTy = new ast::ArrayType(cp);
		arrTy->setDimensions(dims);
		arrTy->setParentType(base);
		base = arrTy;
	}
	return base;
}

ast::Node *Parser::expectVariableTypeDefinition(Parser::TokIterator &i) {
	return expectArrayTypeDefinition(i);
}

void Parser::expectEndOfStatement(Parser::TokIterator &i) {
	if (i->isEndOfStatement()) {
		i++;
		return;
	}
	mStatus = Error;
	emit error(ErrorCodes::ecExpectingEndOfStatement, tr("Expecting end of line or ':', got \"%1\"").arg(i->toString()), i->codePoint());
	i++;
}

bool Parser::variableTypesAreEqual(ast::Node *a, ast::Node *b) {
	assert(a->type() == ast::Node::ntNamedType || a->type() == ast::Node::ntArrayType);
	assert(b->type() == ast::Node::ntNamedType || b->type() == ast::Node::ntArrayType);

	if (a->type() != b->type()) return false;
	if (a->type() == ast::Node::ntNamedType) {
		return static_cast<ast::NamedType*>(a)->identifier()->name() == static_cast<ast::NamedType*>(b)->identifier()->name();
	}
	else { // ast::Node::ntArrayType
		ast::ArrayType *arrTyA = static_cast<ast::ArrayType*>(a);
		ast::ArrayType *arrTyB = static_cast<ast::ArrayType*>(b);
		if (arrTyA->dimensions() != arrTyB->dimensions()) return false;
		return variableTypesAreEqual(arrTyA->parentType(), arrTyB->parentType());
	}
}


ast::Variable *Parser::expectVariable(Parser::TokIterator &i) {
	ast::Variable * var = tryVariable(i);
	if (!var) {
		emit error(ErrorCodes::ecExpectingIdentifier, tr("Expecting variable, got \"%1\"").arg(i->toString()), i->codePoint());
	}
	return var;
}


ast::Variable *Parser::tryVariable(Parser::TokIterator &i) {
	if (i->type() != Token::Identifier) {
		return 0;
	}
	ast::Variable *var = new ast::Variable(i->codePoint());
	var->setIdentifier(expectIdentifier(i));
	i++;
	ast::Node *ty = tryVariableTypeDefinition(i);
	if (mStatus == Error) { delete var; return 0; }

	if (ty) var->setType(ty);
	return var;
}

ast::Node *Parser::trySelectStatement(Parser::TokIterator &i) {
	if (i->type() != Token::kSelect) return 0;
	//Select
	CodePoint startCp = i->codePoint();
	i++;
	ast::Node *var = expectExpression(i);
	if (mStatus == Error) return 0;
	expectEndOfStatement(i);
	if (mStatus == Error) return 0;
	QList<ast::Case> cases;
	ast::Node *defaultCase = 0;
	while (i->type() != Token::kEndSelect) {
		if (i->isEndOfStatement()) {
			i++;
			continue;
		}
		if (i->type() == Token::kCase) {
			CodePoint caseStartCp = i->codePoint();
			i++;
			ast::Node *val = expectExpression(i);
			if (mStatus == Error) return 0;
			expectEndOfStatement(i);
			if (mStatus == Error) return 0;
			ast::Node *block = expectBlock(i);
			if (mStatus == Error) return 0;
			ast::Case *c = new ast::Case(caseStartCp, i->codePoint());
			c->setValue(val);
			c->setBlock(block);
			cases.append(c);
			continue;
		}
		else if (i->type() == Token::kDefault) {
			if (defaultCase != 0) {
				emit error(ErrorCodes::ecMultipleSelectDefaultCases, tr("Multiple default cases"), i->codePoint());
				mStatus = ErrorButContinue;
			}
			i++;
			expectEndOfStatement(i);
			if (mStatus == Error) return 0;
			defaultCase = expectBlock(i);
			if (mStatus == Error) return 0;
		}
		else {
			emit error(ErrorCodes::ecExpectingEndSelect, tr("Expecting \"EndSelect\", \"Case\" or \"Default\", got \"%1\"").arg(i->toString()), i->codePoint());
			mStatus = Error;
			return 0;
		}
	}
	ast::SelectStatement *ret = new ast::SelectStatement(startCp, i->codePoint());
	ret->setDefaultCase(defaultCase);
	ret->setCases(cases);
	ret->setVariable(var);
	i++;
	return ret;
}


ast::Node *Parser::tryGotoGosubAndLabel(Parser::TokIterator &i) {
	switch (i->type()) {
		case Token::kGoto: {
			ast::Goto *ret = new ast::Goto(i->codePoint());
			i++;
			ret->setLabel(expectIdentifier(i));
			if (mStatus == Error) { delete ret; return 0;}
			return ret;
		}
		case Token::kGosub: {
			ast::Gosub *ret = new ast::Gosub(i->codePoint());
			i++;
			ret->setLabel(expectIdentifier(i));
			if (mStatus == Error) { delete ret; return 0;}
			return ret;
		}
		case Token::Label: {
			ast::Label *label = new ast::Label(i->toString(), i->codePoint());
			i++;
			return label;
		}
		default:
			return 0;
	}
}

ast::Node *Parser::tryRedim(Parser::TokIterator &i) {
	if (i->type() != Token::kRedim) return 0;
	int line = i->mLine;
	QString file = i->mFile;
	i++;
	QString name = expectIdentifier(i);
	QString varType = tryVariableTypeDefinition(i);
	if (mStatus == Error) return 0;
	if (i->type() != Token::LeftParenthese) {
		emit error(ErrorCodes::ecExpectingLeftParenthese, tr("Expecting left parenthese, got \"%1\"").arg(i->toString()), i->codePoint());
		mStatus = Error;
		return 0;
	}
	i++;

	QList<ast::Node*> dimensions;
	dimensions.append(expectExpression(i));
	if (mStatus == Error) return 0;
	while (i->type() == Token::Comma) {
		i++;
		dimensions.append(expectExpression(i));
		if (mStatus == Error) return 0;
	}

	if (i->type() != Token::RightParenthese) {
		emit error(ErrorCodes::ecExpectingRightParenthese, tr("Expecting right parenthese, got \"%1\"").arg(i->toString()), i->codePoint());
		mStatus = Error;
		return 0;
	}
	i++;

	if (varType.isEmpty()) {
		varType = tryVariableAsType(i);
		if (mStatus == Error) return 0;
	}
	ast::Redim *arr = new ast::Redim;
	arr->mLine = line;
	arr->mFile = file;
	arr->mName = name;
	arr->mType = varType;
	arr->mDimensions = dimensions;
	return arr;
}

ast::Node *Parser::tryDim(Parser::TokIterator &i) {
	if (i->type() == Token::kDim) {
		CodePoint cp = i->codePoint();
		i++;
		QList<ast::Node*> definitions;
		while (true) {
			ast::Node *def = expectDefinitionOfVariableOrArray(i);
			if (mStatus == Error) return 0;

			definitions.append(def);

			if (i->type() != Token::Comma) break;
			i++;
		}

		ast::Dim *dim = new ast::Dim(cp);
		dim->setDefinitions(definitions);
		return dim;
	}
	return 0;
}

ast::Node *Parser::tryIfStatement(Parser::TokIterator &i) { //FINISH
	if (i->type() == Token::kIf) {
		CodePoint startCp = i->codePoint();
		i++;
		return expectIfStatementNoKeyword(startCp, i);
	}

	return 0;
}

ast::Node *Parser::expectIfStatementNoKeyword(const CodePoint &startCp, Parser::TokIterator &i) {
	ast::Node *condition = expectExpression(i);
	if (mStatus == Error) return 0;
	if (i->type() == Token::kThen) {
		i++;
		if (!i->isEndOfStatement()) { //Inline if
			ast::Node *block = expectInlineBlock(i);
			if (mStatus == Error) return 0;
			if (i->type() == Token::kElseIf) {
				ast::Node *elseIf = expectElseIfStatement(i);
				if (mStatus == Error) return 0;
				ast::IfStatement *ret = new ast::IfStatement(startCp, i->codePoint());
				ret->setCondition(condition);
				ret->setBlock(block);
				ret->setElseBlock(elseIf);
				return ret;
			}
			if (i->type() == Token::kElse) {
				i++;
				ast::Node *elseBlock = expectInlineBlock(i);
				if (mStatus == Error) return 0;
				ast::IfStatement *ret = new ast::IfStatement(startCp, i->codePoint());
				ret->setCondition(condition);
				ret->setBlock(block);
				ret->setElseBlock(elseBlock);
				return ret;
			}

			ast::IfStatement *ret = new ast::IfStatement(startCp, i->codePoint());
			ret->setCondition(condition);
			ret->setBlock(block);
			return ret;
		}
		i++;
	}
	else {
		if (!i->isEndOfStatement()) {
			emit error(ErrorCodes::ecExpectingEndOfStatement, tr("Expecting the end of the line, ':', or \"then\", got \"%1\"").arg(i->toString()), i->codePoint());
			mStatus = Error;
			return 0;
		}
		i++;
	}

	ast::Node *block = *expectBlock(i);
	ast::Node *elseBlock = 0;
	if (mStatus == Error) return 0;
	if (i->type() == Token::kElseIf) {
		elseBlock = expectElseIfStatement(i);
		if (mStatus == Error) return 0;
	}
	else if (i->type() == Token::kElse) {
		i++;
		elseBlock = expectBlock(i);
		if (mStatus == Error) return 0;
	}
	if (i->type() != Token::kEndIf) {
		emit error(ErrorCodes::ecExpectingEndIf, tr("Expecting \"EndIf\" for if-statement, which begin at line %1,").arg(QString::number(line)), i->codePoint());
		mStatus = Error;
		return 0;
	}

	ast::IfStatement *ret = new ast::IfStatement(startCp, i->codePoint());
	ret->setElseBlock(elseBlock);
	ret->setCondition(condition);
	ret->setBlock(block);
	i++;
	return ret;
}

ast::Node *Parser::expectElseIfStatement(Parser::TokIterator &i) {
	assert(i->type() == Token::kElseIf);
	CodePoint startCp = i->codePoint();
	i++;
	return expectIfStatementNoKeyword(startCp, i);
}

ast::Node *Parser::tryWhileStatement(Parser::TokIterator &i) {
	if (i->type() == Token::kWhile) {
		CodePoint startCp = i->codePoint();
		i++;

		ast::Node *cond = expectExpression(i);
		if (mStatus == Error) return 0;

		expectEndOfStatement(i);
		if (mStatus == Error) return 0;

		ast::Block *block = expectBlock(i);
		if (mStatus == Error) return 0;

		if (i->type() != Token::kWend) {
			emit error(ErrorCodes::ecExpectingWend, tr("Expecting \"Wend\" to end \"While\" on line %1, got %2").arg(QString::number(startCp.line()), i->toString()),i->codePoint());
			mStatus = Error;
			return 0;
		}
		ast::WhileStatement *ret = new ast::WhileStatement(startCp, i->codePoint());
		ret->setBlock(block);
		ret->setCondition(cond);
		i++;
		return ret;

	}
	return 0;
}


ast::FunctionDefinition *Parser::tryFunctionDefinition(Parser::TokIterator &i) {
	if (i->type() == Token::kFunction) {
		CodePoint startCp = i->codePoint();
		i++;
		ast::Identifier *functionId = expectIdentifier(i);
		if (mStatus == Error) return 0;

		ast::Node *retType = tryVariableTypeDefinition(i);
		if (mStatus == Error) return 0;

		if (i->type() != Token::LeftParenthese) {
			emit error(ErrorCodes::ecExpectingLeftParenthese, tr("Expecting '(' after a function name, got \"%1\"").arg(i->toString()), i->codePoint());
			mStatus = Error;
			return 0;
		}
		i++;

		QList<ast::Node*> args;
		if (i->type() != Token::RightParenthese) {
			args = expectVariableDefinitionList(i);
			if (mStatus == Error) return 0;
			if (i->type() != Token::RightParenthese) {
				emit error(ErrorCodes::ecExpectingRightParenthese, tr("Expecting ')', got \"%1\"").arg(i->toString()), i->codePoint());
				mStatus = Error;
				return 0;
			}
		}
		else {
			args = new ast::List(i->codePoint());
		}
		i++;
		ast::Node *retType2 = tryVariableAsType(i);
		if (mStatus == Error) return 0;
		if (retType != 0 && retType2 != 0) {
			if (variableTypesAreEqual(retType, retType2)) {
				emit warning(WarningCodes::wcFunctionReturnTypeDefinedTwice, tr("Function return type defined twice"), i->codePoint());
			}
			else {
				emit error(ErrorCodes::ecFunctionReturnTypeDefinedTwice, tr("Function return type defined twice"), i->codePoint());
				mStatus = ErrorButContinue;
			}
		}
		if (retType2) retType = retType2;
		if (retType) {
			emit error(ErrorCodes::ecFunctionReturnTypeRequired, tr("Function return type required"), startCp);
			mStatus = ErrorButContinue;
		}

		expectEndOfStatement(i);
		ast::Node *block = expectBlock(i);
		if (i->type() != Token::kEndFunction) {
			emit error(ErrorCodes::ecExpectingEndFunction, tr("Expecting \"EndFunction\", got %1").arg(i->toString()), i->codePoint());
			mStatus = Error;
			return 0;
		}
		ast::FunctionDefinition *func = new ast::FunctionDefinition(startCp, i->codePoint());
		func->setBlock(block);
		func->setIdentifier(functionId);
		func->setParameterList(args);
		func->setReturnType(retType);
		i++;

		return func;
	}
	return 0;
}

static ast::ExpressionNode::Op tokenTypeToOperator(Token::Type t) {
	switch (t) {
		case Token::opAssign:
			return ast::ExpressionNode::opAssign;
		case Token::opEqual:
			return ast::ExpressionNode::opEqual;
		case Token::opNotEqual:
			return ast::ExpressionNode::opNotEqual;
		case Token::opGreater:
			return ast::ExpressionNode::opGreater;
		case Token::opLess:
			return ast::ExpressionNode::opLess;
		case Token::opGreaterEqual:
			return ast::ExpressionNode::opGreaterEqual;
		case Token::opLessEqual:
			return ast::ExpressionNode::opLessEqual;
		case Token::opMultiply:
			return ast::ExpressionNode::opMultiply;
		case Token::opPower:
			return ast::ExpressionNode::opPower;
		case Token::opMod:
			return ast::ExpressionNode::opMod;
		case Token::opShl:
			return ast::ExpressionNode::opShl;
		case Token::opShr:
			return ast::ExpressionNode::opShr;
		case Token::opSar:
			return ast::ExpressionNode::opSar;
		case Token::opDivide:
			return ast::ExpressionNode::opDivide;
		case Token::opAnd:
			return ast::ExpressionNode::opAnd;
		case Token::opOr:
			return ast::ExpressionNode::opOr;
		case Token::opXor:
			return ast::ExpressionNode::opXor;
		default:
			return ast::ExpressionNode::opInvalid;
	}
}

ast::Node *Parser::expectExpression(Parser::TokIterator &i) {
	return expectAssignementExpression(i);

}

ast::Node *Parser::expectAssignementExpression(Parser::TokIterator &i) {
	CodePoint cp1 = i->codePoint();
	ast::Node *first = expectLogicalOrExpression(i);
	if (mStatus == Error) return 0;
	if (i->type() == Token::opAssign) {
		CodePoint cp2 = i->codePoint();
		++i;
		ast::Node *second = expectLogicalOrExpression(i);
		if (mStatus == Error) { delete first; return 0; }
		ast::Expression *expr = ast::Expression(ast::Expression::RightToLeft, cp1);
		expr->setFirstOperand(first);
		ast::ExpressionNode *exprNode = new ast::ExpressionNode(ast::ExpressionNode::opAssign, cp2);
		exprNode->setOperand(second);
		expr->appendOperation(exprNode);
		while (i->type() == Token::opAssign) {
			exprNode = new ast::ExpressionNode(ast::ExpressionNode::opAssign, i->codePoint());
			expr->appendOperation(exprNode);
			i++;

			ast::Node *operand = expectLogicalOrExpression(i);
			if (mStatus == Error) { delete expr; return 0; }

			exprNode->setOperand(operand);
		}
		return expr;
	}

	return first;
}

ast::Node *Parser::expectLogicalOrExpression(Parser::TokIterator &i) {
	CodePoint cp1 = i->codePoint();
	ast::Node *first = expectLogicalAndExpression(i);
	if (mStatus == Error) return 0;
	if (i->type() == Token::opOr || i->type() == Token::opXor) {
		CodePoint cp2 = i->codePoint();
		ast::ExpressionNode::Op op = tokenTypeToOperator(i->type());
		i++;
		ast::Node *second = expectLogicalAndExpression(i);
		if (mStatus == Error) return 0;
		ast::Expression *expr = new ast::Expression(ast::Expression::LeftToRight, cp1);
		expr->setFirstOperand(first);
		ast::ExpressionNode *exprNode = new ast::ExpressionNode(op, cp2);
		exprNode->setOperand(second);
		expr->appendOperation(exprNode);

		while (i->type() == Token::opOr || i->type() == Token::opXor) {
			ast::ExpressionNode::Op op = tokenTypeToOperator(i->type());
			exprNode = new ast::ExpressionNode(op, i->codePoint());
			expr->appendOperation(exprNode);
			i++;
			ast::Node *ep = expectLogicalAndExpression(i);
			if (mStatus == Error) return 0;
			exprNode->setOperand(ep);
		}
		return expr;
	}
	else {
		return first;
	}
}

ast::Node *Parser::expectLogicalAndExpression(Parser::TokIterator &i) {
	CodePoint cp1 = i->codePoint();
	ast::Node *first = expectEqualityExpression(i);
	if (mStatus == Error) return 0;
	if (i->type() == Token::opAnd) {
		CodePoint cp2 = i->codePoint();
		i++;
		ast::Node *second = expectEqualityExpression(i);
		if (mStatus == Error) return 0;

		ast::Expression *expr = new ast::Expression(ast::Expression::LeftToRight, cp1);
		expr->setFirstOperand(first);
		ast::ExpressionNode *exprNode = new ast::ExpressionNode(ast::ExpressionNode::opAnd, cp2);
		exprNode->setOperand(second);
		expr->appendOperation(exprNode);
		while (i->type() == Token::opAnd) {
			exprNode = new ast::ExpressionNode(ast::ExpressionNode::opAnd, i->codePoint());
			expr->appendOperation(exprNode);
			i++;
			ast::Node *ep = expectEqualityExpression(i);
			if (mStatus == Error) { delete expr; return 0; }
			exprNode->setOperand(ep);
		}
		return expr;
	}
	else {
		return first;
	}
}


ast::Node *Parser::expectEqualityExpression(Parser::TokIterator &i) {
	CodePoint cp1 = i->codePoint();
	ast::Node *first = expectRelativeExpression(i);
	if (mStatus == Error) return 0;
	if (i->type() == Token::opEqual || i->type() == Token::opNotEqual) {
		CodePoint cp2 = i->codePoint();
		ast::ExpressionNode::Op op = tokenTypeToOperator(i->type());
		i++;
		ast::Node *second = expectRelativeExpression(i);
		if (mStatus == Error) return 0;
		ast::Expression *expr = new ast::Expression(ast::Expression::LeftToRight, cp1);
		expr->setFirstOperand(first);
		ast::ExpressionNode *exprNode = new ast::ExpressionNode(op, cp2);
		exprNode->setOperand(second);
		expr->appendOperation(exprNode);
		while (i->type() == Token::opEqual || i->type() == Token::opNotEqual) {
			op = tokenTypeToOperator(i->type());
			exprNode = new ast::ExpressionNode(op, i->codePoint());
			expr->appendOperation(exprNode);
			i++;
			ast::Node *ep = expectRelativeExpression(i);
			if (mStatus == Error) { delete expr; return 0; }
			exprNode->setOperand(ep);
		}
		return expr;
	}
	else {
		return first;
	}
}

ast::Node *Parser::expectRelativeExpression(Parser::TokIterator &i) {
	CodePoint cp1 = i->codePoint();
	ast::Node *first = expectBitShiftExpression(i);
	if (mStatus == Error) return 0;
	if (i->type() == Token::opGreater || i->type() == Token::opLess || i->type() == Token::opGreaterEqual || i->type() == Token::opLessEqual) {
		CodePoint cp2 = i->codePoint();
		ast::ExpressionNode::Op op = tokenTypeToOperator(i->type());
		i++;
		ast::Node *second = expectBitShiftExpression(i);
		if (mStatus == Error) return 0;
		ast::Expression *expr = new ast::Expression(ast::Expression::LeftToRight, cp1);
		expr->setFirstOperand(first);
		ast::ExpressionNode *exprNode = new ast::ExpressionNode(op, cp2);
		exprNode->setOperand(second);
		expr->appendOperation(exprNode);
		while (i->type() == Token::opGreater || i->type() == Token::opLess || i->type() == Token::opGreaterEqual || i->type() == Token::opLessEqual) {
			op = tokenTypeToOperator(i->type());
			exprNode = new ast::ExpressionNode(op, i->codePoint());
			expr->appendOperation(exprNode);
			i++;
			ast::Node *ep = expectBitShiftExpression(i);
			if (mStatus == Error) { delete expr; return 0; }
			exprNode->setOperand(ep);
		}
		return expr;
	}
	else {
		return first;
	}
}

ast::Node *Parser::expectBitShiftExpression(Parser::TokIterator &i) {
	CodePoint cp1 = i->codePoint();
	ast::Node *first = expectAdditiveExpression(i);
	if (mStatus == Error) return 0;
	if (i->type() == Token::opShl || i->type() == Token::opShr || i->type() == Token::opSar) {
		CodePoint cp2 = i->codePoint();
		ast::ExpressionNode::Op op = tokenTypeToOperator(i->type());
		i++;
		ast::Node *second = expectAdditiveExpression(i);
		if (mStatus == Error) return 0;
		ast::Expression *expr = new ast::Expression(ast::Expression::LeftToRight, cp1);
		expr->setFirstOperand(first);
		ast::ExpressionNode *exprNode = new ast::ExpressionNode(op, cp2);
		exprNode->setOperand(second);
		expr->appendOperation(exprNode);
		while (i->type() == Token::opShl || i->type() == Token::opShr || i->type() == Token::opSar) {
			op = tokenTypeToOperator(i->type());
			exprNode = new ast::ExpressionNode(op, i->codePoint());
			expr->appendOperation(exprNode);
			i++;
			ast::Node *ep = expectAdditiveExpression(i);
			if (mStatus == Error) { delete expr; return 0; }
			exprNode->setOperand(ep);
		}
		return expr;
	}
	else {
		return first;
	}
}

ast::Node *Parser::expectAdditiveExpression(Parser::TokIterator &i) {
	CodePoint cp1 = i->codePoint();
	ast::Node *first = expectMultiplicativeExpression(i);
	if (mStatus == Error) return 0;
	if (i->type() == Token::opPlus || i->type() == Token::opMinus) {
		CodePoint cp2 = i->codePoint();
		ast::ExpressionNode::Op op = tokenTypeToOperator(i->type());
		i++;
		ast::Node *second = expectMultiplicativeExpression(i);
		if (mStatus == Error) return 0;

		ast::Expression *expr = new ast::Expression(ast::Expression::LeftToRight, cp1);
		expr->setFirstOperand(first);
		ast::ExpressionNode *exprNode = new ast::ExpressionNode(op, cp2);
		exprNode->setOperand(second);
		expr->appendOperation(exprNode);

		while (i->type() == Token::opPlus || i->type() == Token::opMinus) {
			op = tokenTypeToOperator(i->type());
			exprNode = new ast::ExpressionNode(op, i->codePoint());
			expr->appendOperation(exprNode);
			i++;
			ast::Node *ep = expectMultiplicativeExpression(i);
			if (mStatus == Error) { delete expr; return 0; }
			exprNode->setOperand(ep);
		}
		return expr;
	}
	else {
		return first;
	}
}

ast::Node *Parser::expectMultiplicativeExpression(Parser::TokIterator &i) {
	CodePoint cp1 = i->codePoint();
	ast::Node *first = expectPowExpression(i);
	if (mStatus == Error) return 0;
	if (i->type() == Token::opMultiply || i->type() == Token::opDivide || i->type() == Token::opMod) {
		CodePoint cp2 = i->codePoint();
		ast::ExpressionNode::Op op = tokenTypeToOperator(i->type());
		i++;
		ast::Node *second = expectPowExpression(i);
		if (mStatus == Error) return 0;
		ast::Expression *expr = new ast::Expression(ast::Expression::LeftToRight, cp1);
		expr->setFirstOperand(first);
		ast::ExpressionNode *exprNode = new ast::ExpressionNode(op, cp2);
		exprNode->setOperand(second);
		expr->appendOperation(exprNode);
		while (i->type() == Token::opMultiply || i->type() == Token::opDivide || i->type() == Token::opMod) {
			op = tokenTypeToOperator(i->type());
			exprNode = new ast::ExpressionNode(op, i->codePoint());
			expr->appendOperation(exprNode);
			i++;
			ast::Node *ep = expectPowExpression(i);
			if (mStatus == Error) { delete expr; return 0; }
			exprNode->setOperand(ep);
		}
		return expr;
	}
	else {
		return first;
	}
}

ast::Node *Parser::expectPowExpression(Parser::TokIterator &i) {
	CodePoint cp1 = i->codePoint();
	ast::Node *first = expectUnaryExpession(i);
	if (mStatus == Error) return 0;
	if (i->type() == Token::opPower) {
		CodePoint cp2 = i->codePoint();
		i++;
		ast::Node *second = expectUnaryExpession(i);
		if (mStatus == Error) return 0;
		ast::Expression *expr = new ast::Expression(ast::Expression::LeftToRight, cp1);
		expr->setFirstOperand(first);
		ast::ExpressionNode *exprNode = new ast::ExpressionNode(ast::ExpressionNode::opPower, cp2);
		exprNode->setOperand(second);
		expr->appendOperation(exprNode);
		while (i->type() == Token::opPower) {
			exprNode = new ast::ExpressionNode(ast::ExpressionNode::opPower, i->codePoint());
			expr->appendOperation(exprNode);
			i++;
			ast::Node *ep = expectUnaryExpession(i);
			if (mStatus == Error) { delete expr; return 0; }
			exprNode->setOperand(ep);
		}
		return expr;
	}
	else {
		return first;
	}
}

ast::Node *Parser::expectUnaryExpession(Parser::TokIterator &i) {
	CodePoint cp = i->codePoint();
	switch (i->type()) {
		case Token::opPlus:
		case Token::opMinus:
		case Token::opNot: {
			ast::Unary::Op op = tokenTypeToOperator(i->type());
			i++;
			ast::Node *expr = expectCallOrArraySubscriptExpression(i);
			if (mStatus == Error) return 0;
			ast::Unary *unary = new ast::Unary(op, cp);
			unary->setOperand(expr);
			return unary;
		}
		default:
			return expectCallOrArraySubscriptExpression(i);
	}
}

ast::Node *Parser::expectCallOrArraySubscriptExpression(Parser::TokIterator &i) {
	ast::Node *base = expectPrimaryExpression(i);
	if (mStatus == Error) return 0;
	while (true) {
		switch (i->type()) {
			case Token::LeftParenthese: {
				CodePoint cp = i->codePoint();
				i++;
				ast::Node *params = 0;
				if (i != Token::RightParenthese) {
					params = expectExpressionList(i);
					if (mStatus == Error) { delete p; return 0; }
				}
				else {
					params = new ast::List(i->codePoint());
				}

				if (i != Token::RightParenthese) {
					emit error(ErrorCodes::ecExpectingRightParenthese, tr("Expecting a right parenthese, got \"%1\"").arg(i->toString()), i->codePoint());
					mStatus = Error;
					delete params;
					delete p;
					return 0;
				}
				++i;

				ast::FunctionCall *call = new ast::FunctionCall(cp);
				call->setFunction(p);
				call->setParameters(params);
				return call;
			}
			case Token::LeftSquareBracket: {
				CodePoint cp = i->codePoint();
				i++;
				ast::Node *params = 0;
				if (i != Token::RightSquareBracket) {
					params = expectExpressionList(i);
					if (mStatus == Error) { delete p; return 0; }
				}
				else {
					params = new ast::List(i->codePoint());
				}

				if (i != Token::RightSquareBracket) {
					emit error(ErrorCodes::ecExpectingRightSquareBracket, tr("Expecting a right square bracket, got \"%1\"").arg(i->toString()), i->codePoint());
					mStatus = Error;
					delete params;
					delete p;
					return 0;
				}
				++i;

				ast::ArraySubscript *s = new ast::ArraySubscript(cp);
				s->setArray(base);
				s->setSubscript(params);
				base = s;
			}
			default:
				return base;
		}
	}
}


ast::Node *Parser::expectPrimaryExpression(TokIterator &i) {
	switch (i->type()) {
		case Token::LeftParenthese: {
			i++;
			ast::Node *expr = expectExpression(i);
			if (i->type() != Token::RightParenthese) {
				emit error(ErrorCodes::ecExpectingRightParenthese, tr("Expecting a right parenthese, got \"%1\"").arg(i->toString()), i->codePoint());
				mStatus = Error;
				return 0;
			}
			i++;
			return expr;
		}
		case Token::Integer: {
			bool success;
			int val = i->toString().toInt(&success);
			if (!success) {
				emit error(ErrorCodes::ecCantParseInteger, tr("Cannot parse an integer \"%1\"").arg(i->toString()), i->codePoint());
				mStatus = Error;
				return 0;
			}
			ast::Integer *intN = new ast::Integer;
			intN->mValue = val;
			i++;
			return intN;
		}
		case Token::IntegerHex: {
			bool success;
			int val = i->toString().toInt(&success,16);
			if (!success) {
				emit error(ErrorCodes::ecCantParseInteger, tr("Cannot parse integer \"%1\"").arg(i->toString()), i->codePoint());
				mStatus = Error;
				return 0;
			}
			ast::Integer *intN = new ast::Integer(val, i->codePoint());
			i++;
			return intN;
		}

		case Token::Float: {
			bool success;
			float val;
			if (*i->mBegin == '.') { //leading dot .13123
				val = ('0' + i->toString()).toFloat(&success);
			}
			else if (*(i->mEnd - 1) == '.') { //Ending dot 1231.
				val = (i->toString() + '0').toFloat(&success);
			}
			else {
				val = i->toString().toFloat(&success);
			}
			if (!success) {
				emit error(ErrorCodes::ecCantParseFloat, tr("Cannot parse float \"%1\"").arg(i->toString()), i->codePoint());
				mStatus = Error;
				return 0;
			}
			ast::Float *f = new ast::Float(val, i->codePoint());
			i++;
			return f;
		}
		case Token::String: {
			ast::String *str = new ast::String(i->toString(), i->codePoint());
			i++;
			return str;
		}
		case Token::Identifier: {
			CodePoint cp = i->codePoint();
			ast::Identifier *identifier = ast::Identifier(i->toString(), i->codePoint());
			i++;
			ast::Node *varType = tryVariableTypeDefinition(i);
			if (varType) {
				ast::Variable *var = new ast::Variable(cp);
				var->setIdentifier(identifier);
				var->setType(varType);
				return var;
			}
			else {
				return identifier;
			}
		}
		case Token::kNew:
		case Token::kFirst:
		case Token::kLast:
		case Token::kBefore:
		case Token::kAfter: {
			ast::KeywordFunctionCall *ret;
			switch (i->type()) {
				case Token::kNew:
					ret = new ast::KeywordFunctionCall(ast::KeywordFunctionCall::New, i->codePoint()); break;
				case Token::kFirst:
					ret = new ast::KeywordFunctionCall(ast::KeywordFunctionCall::First, i->codePoint()); break;
				case Token::kLast:
					ret = new ast::KeywordFunctionCall(ast::KeywordFunctionCall::Last, i->codePoint()); break;
				case Token::kBefore:
					ret = new ast::KeywordFunctionCall(ast::KeywordFunctionCall::Before, i->codePoint()); break;
				case Token::kAfter:
					ret = new ast::KeywordFunctionCall(ast::KeywordFunctionCall::After, i->codePoint()); break;
				default:
					assert("WTF assertion");
					ret = 0;
			}
			i++;

			if (!expectLeftParenthese(i)) {
				delete ret;
				return 0;
			}
			i++;
			ret->setParameters(expectExpressionList(i));
			if (mStatus == Error) {
				delete ret;
				return 0;
			}
			if (!expectRightParenthese(i)) {
				delete ret;
				return 0;
			}

			return ret;
		}
		default:
			emit error(ErrorCodes::ecExpectingPrimaryExpression, tr("Expecting a primary expression, got \"%1\"").arg(i->toString()), i->codePoint());
			mStatus = Error;
			return 0;
	}
}

ast::FunctionParametreDefinition Parser::expectFunctionParametreDefinition(Parser::TokIterator &i) {
	ast::FunctionParametreDefinition ret;
	expectVariable(&ret.mVariable, i);
	if (mStatus == Error) return ret;

	ret.mDefaultValue = 0;
	if (i->type() == Token::opEqual) {
		i++;
		ret.mDefaultValue = expectExpression(i);
		if (mStatus == Error) return ret;
	}
	return ret;
}

ast::Node *Parser::expectExpressionList(Parser::TokIterator &i) {
	ast::List *list = ast::List(i->codePoint());
	do {
		ast::Node *expr = expectExpression(i);
		if (mStatus == Error) {
			delete list;
			return 0;
		}
		list->appendItem(expr);
	} while ((i++)->type() == Token::Comma);

	if (list->childNodeCount() == 1) {
		ast::Node *onlyItem = list->childNode(0);
		list->takeAll();
		delete list;
		return onlyItem;
	}
	return list;
}

ast::Node *Parser::expectVariableDefinitionList(Parser::TokIterator &i) {
	CodePoint cp = i->codePoint();
	i++;
	QList<ast::Node*> definitions;
	while (true) {
		ast::Node *def = expectDefinitionOfVariableOrArray(i);
		if (mStatus == Error) return 0;

		definitions.append(def);

		if (i->type() != Token::Comma) break;
		i++;
	}
	if (definitions.size() == 1) {
		return definitions.first();
	}
	ast::List *ret = new ast::List(cp);
	ret->setItems(definitions);
	return ret;
}


ast::Node *Parser::tryRepeatStatement(Parser::TokIterator &i) {
	if (i->type() == Token::kRepeat) {
		CodePoint startCp = i->codePoint();
		i++;
		expectEndOfStatement(i);
		if (mStatus == Error) return 0;
		ast::Node *block = expectBlock(i);
		if (mStatus == Error) return 0;
		if (i->type() == Token::kUntil) {
			CodePoint endCp = i->codePoint();
			i++;
			ast::Node *condition = expectExpression(i);
			if (mStatus == Error) return 0;
			ast::RepeatUntilStatement *ret = new ast::RepeatUntilStatement(startCp, endCp);
			ret->setCondition(condition);
			ret->setBlock(block);
			return ret;
		}
		if (i->type() == Token::kForever) {
			ast::RepeatForeverStatement *ret = new ast::RepeatForeverStatement(startCp, i->codePoint());
			ret->setBlock(block);
			i++;
			return ret;
		}
		emit error(ErrorCodes::ecExpectingEndOfRepeat, tr("Expecting \"Until\" or \"Forever\", got \"%1\""), i->codePoint());
		mStatus = Error;
		return 0;
	}

	return 0;
}

ast::Node *Parser::tryForStatement(Parser::TokIterator &i) {
	//TODO: Better error reporting
	if (i->type() == Token::kFor) {
		QString file = i->mFile;
		int startLine = i->mLine;
		i++;
		QString varName = expectIdentifier(i);
		if (mStatus == Error) return 0;

		QString varType = tryVariableTypeDefinition(i);
		if (mStatus == Error) return 0;
		ast::Node *var = expectVariable(i);

		if (i->type() != Token::opEqual) {
			emit error(ErrorCodes::ecExpectingAssignment, tr("Expecting \"=\" after the variable name in For-statement, got \"%1\"").arg(i->toString()), i->codePoint());
			mStatus = Error;
			return 0;
		}
		i++;

		if (i->type() == Token::kEach) { //For-Each
			i++;
			ast::ForEachStatement *ret = new ast::ForEachStatement;
			ret->mContainer = expectIdentifierAfter(i, (i - 1)->toString());
			if (mStatus == Error) return 0;

			expectEndOfStatement(i);
			if (mStatus == Error) return 0;

			ret->mBlock = expectBlock(i);
			if (mStatus == Error) return 0;

			if (i->type() != Token::kNext) {
				emit error(ErrorCodes::ecExpectingNext, tr("Expecting \"Next\" for For-statement starting at line %1").arg(QString::number(startLine)), i->codePoint());
				mStatus = Error;
				return 0;
			}
			int endLine = i->mLine;
			i++;

			QString varName2 = expectIdentifierAfter(i, (i - 1)->toString());
			if (mStatus == Error) return 0;
			QString varType2 = tryVariableTypeDefinition(i);
			if (mStatus == Error) return 0;
			if (varName != varName2) {
				emit error(ErrorCodes::ecForAndNextDontMatch, tr("The variable name \"%1\" in For-statement starting at line %2 doesn't match the variable name \"%3\"in \"Next\"").arg(
							   varName, QString::number(startLine), varName2), endLine, file);
				mStatus = ErrorButContinue;
			}
			if (!varType.isEmpty() && !varType2.isEmpty() && varType != varType2) {
				emit error(ErrorCodes::ecForAndNextDontMatch, tr("The variable \"%1\" type in For-statement starting at line %2 doesn't match the variable type in \"Next\"").arg(
							   varName, QString::number(startLine), varName2), endLine, file);
				mStatus = ErrorButContinue;
			}

			ret->mStartLine = startLine;
			ret->mEndLine = endLine;
			ret->mFile =file;
			ret->mVarName = varName;
			ret->mVarType = varType;
			return ret;

		}
		else { //For-To
			ast::ForToStatement *ret = new ast::ForToStatement;
			ret->mFrom = expectExpression(i);
			if (mStatus == Error) return 0;

			if (i->type() != Token::kTo) {
				emit error(ErrorCodes::ecExpectingTo, tr("Expecting \"To\", got \"%1\"").arg(i->toString()), i->codePoint());
				mStatus = Error;
				return 0;
			}
			i++;

			ret->mTo = expectExpression(i);
			if (mStatus == Error) return 0;

			if (i->type() == Token::kStep) {
				i++;
				ret->mStep = expectExpression(i);
				if (mStatus == Error) return 0;
			}
			else ret->mStep = 0;

			expectEndOfStatement(i);
			if (mStatus == Error) return 0;

			ret->mBlock = expectBlock(i);
			if (mStatus == Error) return 0;

			int endLine = i->mLine;
			if (i->type() != Token::kNext) {
				emit error(ErrorCodes::ecExpectingNext, tr("Expecting \"Next\" for For-statement starting at line %1").arg(QString::number(startLine)), i->codePoint());
				mStatus = Error;
				return 0;
			}
			i++;
			QString varName2 = expectIdentifierAfter(i, (i - 1)->toString());
			if (mStatus == Error) return 0;
			QString varType2 = tryVariableTypeDefinition(i);
			if (mStatus == Error) return 0;
			if (varName != varName2) {
				emit error(ErrorCodes::ecForAndNextDontMatch, tr("The variable name \"%1\" in For-statement starting at line %2 doesn't match the variable name \"%3\"in \"Next\"").arg(
							   varName, QString::number(startLine), varName2), endLine, file);
				mStatus = ErrorButContinue;
			}
			if (!varType.isEmpty() && !varType2.isEmpty() && varType != varType2) {
				emit error(ErrorCodes::ecForAndNextDontMatch, tr("The variable \"%1\" type in For-statement starting at line %2 doesn't match the variable type in \"Next\"").arg(
							   varName, QString::number(startLine), varName2), endLine, file);
				mStatus = ErrorButContinue;
			}


			ret->mStartLine = startLine;
			ret->mFile =file;
			ret->mVarName = varName;
			ret->mVarType = varType;
			ret->mEndLine = endLine;
			return ret;
		}
	}

	return 0;
}



