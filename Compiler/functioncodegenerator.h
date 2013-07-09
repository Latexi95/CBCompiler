#ifndef FUNCTIONCODEGENERATOR_H
#define FUNCTIONCODEGENERATOR_H
#include <QObject>
#include <QVector>
#include <QStack>

#include "llvm.h"
#include "scope.h"
#include "abstractsyntaxtree.h"
#include "expressioncodegenerator.h"
#include "builder.h"
class QFile;
class CBFunction;
/**
 * @brief The FunctionCodeGenerator class generates llvm-ir for a function. It expects AST and Scope to valid.
 */
class FunctionCodeGenerator: public QObject{
		Q_OBJECT
	public:
		explicit FunctionCodeGenerator(QObject *parent = 0);
		/**
		 * @brief Sets the runtime used for the generation.
		 * @param r Runtime
		 */
		void setRuntime(Runtime *r);

		/**
		 * @brief setFunction
		 * @param func The function in which generateFunctionCode generates the llvm-ir.
		 */
		void setFunction(llvm::Function *func);
		void setScope(Scope *scope);
		void setSetupBasicBlock(llvm::BasicBlock *bb);

		/**
		 * @brief setStringPool sets the pointer to the global string pool.
		 * @param stringPool
		 */
		void setStringPool(StringPool *stringPool);

		bool generateCBFunction(ast::FunctionDefinition *func, CBFunction *cbFunc);

		/**
		 * @brief generateMainScope generates llvm-IR code for a AST tree.
		 *
		 * llvm::Function where llvm-IR is inserted have to be set using setFunction. The scope should be specificated with setScope.
		 * @param n The block that contains AST.
		 * @return True, if generation succeeded, otherwise false
		 */
		bool generateMainScope(ast::Block *n);

		void generateStringLiterals();
		Scope *scope() const {return mScope;}
		llvm::Function *function()const{return mFunction;}
		Runtime *runtime() const {return mRuntime;}
		void createBuilder();
	private:
		bool generate(ast::Node *n);
		bool generate(ast::IfStatement *n);
		bool generate(ast::AssignmentExpression *n);
		bool generate(ast::CommandCall *n);
		bool generate(ast::RepeatForeverStatement *n);
		bool generate(ast::FunctionCallOrArraySubscript *n);
		bool generate(ast::Exit *n);
		bool generate(ast::Return *n);
		bool generate(ast::Block *n);
		bool generate(ast::ArrayDefinition *n);
		bool generate(ast::ArraySubscriptAssignmentExpression *n);
		bool generate(ast::SelectStatement *n);
		bool generate(ast::ForEachStatement *n);
		bool generate(ast::ForToStatement *n);
		bool generate(ast::Goto *n);
		bool generate(ast::Gosub *n);
		bool generate(ast::RepeatUntilStatement *n);
		bool generate(ast::WhileStatement *n);
		bool generate(ast::VariableDefinition *n);
		bool generate(ast::Redim *n);
		bool generate(ast::Label *n);

		bool basicBlockGenerationPass(ast::Block *n);
		bool basicBlockGenerationPass(ast::IfStatement *n);
		bool basicBlockGenerationPass(ast::ForToStatement *n);
		bool basicBlockGenerationPass(ast::ForEachStatement *n);
		bool basicBlockGenerationPass(ast::RepeatUntilStatement *n);
		bool basicBlockGenerationPass(ast::RepeatForeverStatement *n);
		bool basicBlockGenerationPass(ast::Label *n);
		bool basicBlockGenerationPass(ast::WhileStatement *n);
		bool basicBlockGenerationPass(ast::SelectStatement *n);

		void addBasicBlock();

		bool generateLocalVariables();
		bool generateDestructors();

		bool isCurrentBBEmpty() const { return mCurrentBasicBlock->getInstList().empty(); }
		void nextBasicBlock();
		void pushExit(const QVector<llvm::BasicBlock*>::ConstIterator &i) { mExitStack.push(i);}
		void pushExit(ast::Node *n) { const QVector<llvm::BasicBlock*>::ConstIterator i = mExitLocations[n]; assert(i != mBasicBlocks.end()); pushExit(i); }
		void popExit() { mExitStack.pop(); }
		llvm::BasicBlock *currentExit() const { assert(!mExitStack.isEmpty()); return *mExitStack.top(); }

		Scope *mScope;
		Builder *mBuilder;
		llvm::Function *mFunction;
		llvm::BasicBlock *mSetupBasicBlock;
		QVector<llvm::BasicBlock*> mBasicBlocks;
		QVector<llvm::BasicBlock*>::ConstIterator mCurrentBasicBlockIt;
		llvm::BasicBlock *mCurrentBasicBlock;
		QMap<ast::Node*, QVector<llvm::BasicBlock*>::ConstIterator> mExitLocations;
		QStack<QVector<llvm::BasicBlock*>::ConstIterator> mExitStack;
		ExpressionCodeGenerator mExprGen;
		Runtime *mRuntime;
		StringPool *mStringPool;
		ValueType *mReturnType;
		llvm::BasicBlock *mStringLiteralInitializationBasicBlock;
		llvm::BasicBlock *mStringLiteralInitializationExitBasicBlock;
	public slots:

	signals:
		void error(int code, QString msg, int line, QFile *file);
		void warning(int code, QString msg, int line, QFile *file);
};

#endif // FUNCTIONCODEGENERATOR_H
