#ifndef PARSER_H
#define PARSER_H
#include <QObject>
#include "token.h"
#include "abstractsyntaxtree.h"
class Parser : public QObject
{
		Q_OBJECT
	public:
		typedef QList<Token>::ConstIterator TokIterator;
		Parser();
		ast::Program *parse(const QList<Token> &tokens);
		bool success() {return mStatus == Ok;}
		ast::ConstDefinition *tryConstDefinition(TokIterator &i);
		ast::GlobalDefinition *tryGlobalDefinition(TokIterator &i);
		ast::Variable::VarType tryVarAsType(TokIterator &i);
		ast::Variable::VarType tryVarType(TokIterator &i);
		ast::Variable::VarType tryVarTypeSymbol(TokIterator &i);
		ast::TypeDefinition *tryTypeDefinition(TokIterator &i);
		ast::Variable *expectVariableOrTypePtrDefinition(TokIterator &i);
		ast::Variable *tryVariableOrTypePtrDefinition(TokIterator &i);
		void expectVariableOrTypePtrDefinition(ast::Variable *var, TokIterator &i);
		ast::Node *tryDim(TokIterator &i);
		ast::Node *tryIfStatement(TokIterator &i);
		ast::Node *expectElseIfStatement(TokIterator &i);
		ast::Node *tryWhileStatement(TokIterator &i);
		ast::Node *tryRepeatStatement(TokIterator &i);
		ast::Node *tryForStatement(TokIterator &i);
		ast::Node *tryFunctionOrCommandCallOrArraySubscriptAssignment(TokIterator &i);
		ast::Block expectBlock(TokIterator &i);
		ast::Block expectInlineBlock(TokIterator &i);

		ast::FunctionDefinition *tryFunctionDefinition(TokIterator &i);
		ast::FunctionParametreDefinition expectFunctionParametreDefinition(TokIterator &i);

		ast::Node *expectExpression(TokIterator &i);
		ast::Node *expectLogicalOrExpression(TokIterator &i);
		ast::Node *expectLogicalAndExpression(TokIterator &i);
		ast::Node *expectEqualityExpression(TokIterator &i);
		ast::Node *expectRelativeExpression(TokIterator &i);
		ast::Node *expectBitShiftExpression(TokIterator &i);
		ast::Node *expectAdditiveExpression(TokIterator &i);
		ast::Node *expectMultiplicativeExpression(TokIterator &i);
		ast::Node *expectPowExpression(TokIterator &i);
		ast::Node *expectUnaryExpession(TokIterator &i);
		ast::Node *expectPrimaryExpression(TokIterator &i);
		ast::Node *tryAssignmentExpression(TokIterator &i);
		QString expectIdentifier(TokIterator &i);
		void expectEndOfStatement(TokIterator &i);
	private:
		enum Status {
			Error,
			ErrorButContinue,
			Ok
		};
		Status mStatus;

	signals:
		void error(int code, QString msg, int line, QFile *file);
		void warning(int code, QString msg, int line, QFile *file);
};

#endif // PARSER_H
