#include "functioncodegenerator.h"
#include "functionsymbol.h"
#include "variablesymbol.h"
#include "labelsymbol.h"
#include "typesymbol.h"
#include "valuetypesymbol.h"
#include "constantsymbol.h"
#include "errorcodes.h"
#include "warningcodes.h"
#include "runtime.h"
#include "functionselectorvaluetype.h"
#include "functionvaluetype.h"
#include "typepointervaluetype.h"
#include "liststringjoin.h"
#include "castcostcalculator.h"
#include "cbfunction.h"
#define CHECK_UNREACHABLE(codePoint) if (checkUnreachable(codePoint)) return;

struct CodeGeneratorError {
		CodeGeneratorError(ErrorCodes::ErrorCode ec) : mErrorCodes(ec) { }
		ErrorCodes::ErrorCode errorCode() const { return mErrorCodes; }
	private:
		ErrorCodes::ErrorCode mErrorCodes;
};

FunctionCodeGenerator::FunctionCodeGenerator(Runtime *runtime, Settings *settings, QObject *parent) :
	QObject(parent),
	mSettings(settings),
	mRuntime(runtime),
	mTypeResolver(runtime)
{
	connect(this, &FunctionCodeGenerator::error, this, &FunctionCodeGenerator::errorOccured);
	connect(&mConstEval, &ConstantExpressionEvaluator::error, this, &FunctionCodeGenerator::error);
	connect(&mConstEval, &ConstantExpressionEvaluator::warning, this, &FunctionCodeGenerator::warning);
	connect(&mTypeResolver, &TypeResolver::error, this, &FunctionCodeGenerator::error);
	connect(&mTypeResolver, &TypeResolver::warning, this, &FunctionCodeGenerator::warning);
}

bool FunctionCodeGenerator::generate(Builder *builder, ast::Node *block, CBFunction *func, Scope *globalScope) {
	mMainFunction = false;
	mUnreachableBasicBlock = false;
	mLocalScope = func->scope();
	mGlobalScope = globalScope;
	mFunction = func->function();
	mValid = true;
	mBuilder = builder;
	mReturnType = func->returnValue();
	mUnresolvedGotos.clear();

	llvm::BasicBlock *firstBasicBlock = llvm::BasicBlock::Create(builder->context(), "firstBB", mFunction);
	mBuilder->setInsertPoint(firstBasicBlock);
	generateAllocas();
	generateFunctionParameterAssignments(func->parameters());

	try {
		block->accept(this);
	}
	catch (CodeGeneratorError ) {
		return false;
	}

	if (!mBuilder->currentBasicBlock()->getTerminator()) {
		generateDestructors();
		mBuilder->returnValue(func->returnValue(), Value(func->returnValue(), func->returnValue()->defaultValue()));
		emit warning(WarningCodes::wcReturnsDefaultValue, tr("Function might return default value of the return type because the function doesn't end to return statement"), func->codePoint());
	}

	resolveGotos();

	return mValid;
}

bool FunctionCodeGenerator::generateMainBlock(Builder *builder, ast::Node *block, llvm::Function *func, Scope *localScope, Scope *globalScope) {
	mLocalScope = localScope;
	mGlobalScope = globalScope;
	mFunction = func;
	mValid = true;
	mUnreachableBasicBlock = false;
	mBuilder = builder;
	mMainFunction = true;
	mReturnType = 0;
	mUnresolvedGotos.clear();

	llvm::BasicBlock *firstBasicBlock = llvm::BasicBlock::Create(builder->context(), "firstBB", func);
	mBuilder->setInsertPoint(firstBasicBlock);
	if (!generateAllocas()) return false;

	try {
		block->accept(this);
	}
	catch (CodeGeneratorError ) {
		return false;
	}
	generateDestructors();
	mBuilder->returnVoid();

	resolveGotos();

	return mValid;
}

void FunctionCodeGenerator::visit(ast::Expression *n) {
	CHECK_UNREACHABLE(n->codePoint());

	Value result = generate(n);
	mBuilder->destruct(result);
}


void FunctionCodeGenerator::visit(ast::Const *n) {
	CHECK_UNREACHABLE(n->codePoint());

	Value constVal = generate(n->value());
	if (!constVal.isConstant()) {
		emit error(ErrorCodes::ecNotConstant, tr("Expression isn't constant expression"), n->codePoint());
		throw CodeGeneratorError(ErrorCodes::ecNotConstant);
	}
	Symbol *sym = mLocalScope->find(n->variable()->identifier()->name());
	assert(sym->type() == Symbol::stConstant);
	ConstantSymbol *constant = static_cast<ConstantSymbol*>(sym);
	constant->setValue(constVal.constant());
}

void FunctionCodeGenerator::visit(ast::VariableDefinition *n) {
	CHECK_UNREACHABLE(n->codePoint());

	Value ref = generate(n->identifier());
	Value init;
	if (n->value()->type() == ast::Node::ntDefaultValue) {
		init = Value(ref.valueType(), ref.valueType()->defaultValue());
	}
	else {
		init = generate(n->value());
	}

	OperationFlags flags = ValueType::castCostOperationFlags(init.valueType()->castingCostToOtherValueType(ref.valueType()));
	if (operationFlagsContainFatalFlags(flags)) {
		emit error(ErrorCodes::ecCantCastValue, tr("Can't cast %1 to %2").arg(init.valueType()->name(), ref.valueType()->name()), n->identifier()->codePoint());
		return;
	}

	mBuilder->store(ref, ref.valueType()->cast(mBuilder, init));
}

void FunctionCodeGenerator::visit(ast::ArrayInitialization *n) {
	CHECK_UNREACHABLE(n->codePoint());
}

void FunctionCodeGenerator::visit(ast::FunctionCall *n) {
	CHECK_UNREACHABLE(n->codePoint());
	mBuilder->destruct(generate(n));
}

void FunctionCodeGenerator::visit(ast::IfStatement *n) {
	CHECK_UNREACHABLE(n->codePoint());
	llvm::BasicBlock *condBB = mBuilder->currentBasicBlock();
	Value cond = generate(n->condition());
	llvm::BasicBlock *trueBlock = createBasicBlock("trueBB");
	llvm::BasicBlock *endBlock = createBasicBlock("endIfBB");

	mBuilder->setInsertPoint(trueBlock);
	n->block()->accept(this);
	if (!mUnreachableBasicBlock)
		mBuilder->branch(endBlock);
	mUnreachableBasicBlock = false;

	llvm::BasicBlock *elseBlock = 0;
	if (n->elseBlock()) {
		elseBlock = createBasicBlock("elseBB");
		mBuilder->setInsertPoint(elseBlock);
		n->elseBlock()->accept(this);
		if (!mUnreachableBasicBlock)
			mBuilder->branch(endBlock);
		mUnreachableBasicBlock = false;
	}


	mBuilder->setInsertPoint(condBB);
	if (n->elseBlock()) {
		mBuilder->branch(cond, trueBlock, elseBlock);
	}
	else {
		mBuilder->branch(cond, trueBlock, endBlock);
	}

	mBuilder->setInsertPoint(endBlock);
}

void FunctionCodeGenerator::visit(ast::WhileStatement *n) {
	CHECK_UNREACHABLE(n->codePoint());
	llvm::BasicBlock *condBB = createBasicBlock("whileCondBB");
	llvm::BasicBlock *blockBB = createBasicBlock("whileBodyBB");
	llvm::BasicBlock *wendBB = createBasicBlock("wendBB");

	mBuilder->branch(condBB);
	mBuilder->setInsertPoint(condBB);

	Value cond = generate(n->condition());
	mBuilder->branch(cond, blockBB, wendBB);

	mExitStack.push(wendBB);
	mBuilder->setInsertPoint(blockBB);
	n->block()->accept(this);
	if (!mUnreachableBasicBlock)
		mBuilder->branch(condBB);
	mUnreachableBasicBlock = false;
	mExitStack.pop();

	mBuilder->setInsertPoint(wendBB);
}

void FunctionCodeGenerator::visit(ast::RepeatForeverStatement *n) {
	CHECK_UNREACHABLE(n->codePoint());

	llvm::BasicBlock *block = createBasicBlock("repeatForeverBB");
	llvm::BasicBlock *endBlock = createBasicBlock("repeatForeverEndBB");

	mBuilder->branch(block);
	mBuilder->setInsertPoint(block);
	mExitStack.push(endBlock);
	n->block()->accept(this);
	mExitStack.pop();
	if (!mUnreachableBasicBlock)
		mBuilder->branch(block);
	mUnreachableBasicBlock = false;

	mBuilder->setInsertPoint(endBlock);


}

void FunctionCodeGenerator::visit(ast::RepeatUntilStatement *n) {
	CHECK_UNREACHABLE(n->codePoint());

	llvm::BasicBlock *block = createBasicBlock("repeatUntilBB");
	llvm::BasicBlock *endBlock = createBasicBlock("repeatUntilEndBB");

	mBuilder->branch(block);
	mBuilder->setInsertPoint(block);
	mExitStack.push(endBlock);
	n->block()->accept(this);
	mExitStack.pop();
	Value cond = generate(n->condition());
	if (!mUnreachableBasicBlock) {
		mBuilder->branch(cond, block, endBlock);
	}
	mUnreachableBasicBlock = false;

	mBuilder->setInsertPoint(endBlock);

}

void FunctionCodeGenerator::visit(ast::ForToStatement *n) {
	CHECK_UNREACHABLE(n->codePoint());

	llvm::BasicBlock *condBB = createBasicBlock("ForToCondBB");
	llvm::BasicBlock *blockBB = createBasicBlock("forBodyBB");
	llvm::BasicBlock *endBB = createBasicBlock("endForBB");

	Value value = generate(n->from());
	if (!value.isReference()) {
		emit error(ErrorCodes::ecReferenceRequired, tr("Expression should return a reference to a variable"), n->from()->codePoint());
		return;
	}
	ConstantValue step = 1;
	if (n->step()) {
		step = mConstEval.evaluate(n->step());
		if (!step.isValid()) {
			emit error(ErrorCodes::ecNotConstant, tr("\"Step\" has to be a constant value"), n->step()->codePoint());
			return;
		}
	}

	OperationFlags flags;
	bool positiveStep = ConstantValue::greaterEqual(step, ConstantValue(0), flags).toBool();

	mBuilder->branch(condBB);
	mBuilder->setInsertPoint(condBB);

	Value to = generate(n->to());
	Value cond;
	if (positiveStep) {
		cond = mBuilder->lessEqual(value, to);
	} else {
		cond = mBuilder->greaterEqual(value, to);
	}

	mBuilder->branch(cond, blockBB, endBB);

	mExitStack.push(endBB);
	mBuilder->setInsertPoint(blockBB);
	mExitStack.pop();
	n->block()->accept(this);
	if (!mUnreachableBasicBlock) {
		mBuilder->store(value, mBuilder->add(value, Value(step, mRuntime)));
		mBuilder->branch(condBB);
	}
	mUnreachableBasicBlock = false;
	mBuilder->setInsertPoint(endBB);
}

void FunctionCodeGenerator::visit(ast::ForEachStatement *n) {
	CHECK_UNREACHABLE(n->codePoint());
	Value container = generate(n->container());
	Value var = generate(n->variable());
	assert(var.isReference());

	llvm::BasicBlock *condBB = createBasicBlock("forEachCondBB");
	llvm::BasicBlock *blockBB = createBasicBlock("forEachBlockBB");
	llvm::BasicBlock *endBB = createBasicBlock("endForEachBB");

	ValueType *valueType = container.valueType();
	if (container.isValueType() && valueType->isTypePointer()) { //Type
		if (var.valueType() != valueType) {
			emit error(ErrorCodes::ecInvalidAssignment, tr("The type of the variable \"%1\" doesn't match the container type \"%2\"").arg(var.valueType()->name(), valueType->name()), n->codePoint());
			return;
		}
		TypePointerValueType *typePointerValueType = static_cast<TypePointerValueType*>(valueType);
		TypeSymbol *typeSymbol = typePointerValueType->typeSymbol();

		mBuilder->store(var, mBuilder->firstTypeMember(typeSymbol));
		mBuilder->branch(condBB);

		mBuilder->setInsertPoint(condBB);
		Value cond = mBuilder->typePointerNotNull(var);
		mBuilder->branch(cond, blockBB, endBB);

		mBuilder->setInsertPoint(blockBB);
		n->block()->accept(this);
		if (!mUnreachableBasicBlock) {
			mBuilder->store(var, mBuilder->afterTypeMember(var));
			mBuilder->branch(condBB);
		}
		mUnreachableBasicBlock = false;

		mBuilder->setInsertPoint(endBB);
	}
	else if (container.isNormalValue() && valueType->isArray()) {
		assert("Array For-Each isn't implemented yet" && 0);
	}
	else {
		emit error(ErrorCodes::ecNotContainer, tr("For-Each-statement requires a container (an array or a type). Can't iterate \"%1\"").arg(valueType->name()), n->container()->codePoint());
		return;
	}
}

void FunctionCodeGenerator::visit(ast::SelectStatement *n) {
	CHECK_UNREACHABLE(n->codePoint());
	assert("UNIMPLEMENTED FUNCTION" && 0);
}

void FunctionCodeGenerator::visit(ast::SelectCase *n) {
	CHECK_UNREACHABLE(n->codePoint());
	assert("UNIMPLEMENTED FUNCTION" && 0);
}

void FunctionCodeGenerator::visit(ast::Return *n) {
	CHECK_UNREACHABLE(n->codePoint());
	if (mMainFunction) {
		assert("Implement gosub" && 0);
	}
	else {
		if (n->value()) {
			mBuilder->returnValue(mReturnType, generate(n->value()));
		}
		else {
			mBuilder->returnValue(mReturnType, mBuilder->defaultValue(mReturnType));
		}
	}
	mUnreachableBasicBlock = true;
}

void FunctionCodeGenerator::visit(ast::Goto *n) {
	CHECK_UNREACHABLE(n->codePoint());

	Symbol *sym = mLocalScope->find(n->label()->name());
	if (!sym) {
		emit error(ErrorCodes::ecCantFindSymbol, tr("Can't find label \"%1\"").arg(n->label()->name()), n->label()->codePoint());
		return;
	}
	if (sym->type() != Symbol::stLabel) {
		emit error(ErrorCodes::ecNotLabel, tr("Symbol \"%1\" isn't a label").arg(n->label()->name()), n->label()->codePoint());
		return;
	}

	LabelSymbol *label = static_cast<LabelSymbol*>(sym);
	if (label->basicBlock()) {
		mBuilder->branch(label->basicBlock());
	}
	else {
		mUnresolvedGotos.append(QPair<LabelSymbol*, llvm::BasicBlock*>(label, mBuilder->currentBasicBlock()));
	}
	mUnreachableBasicBlock = true;
}

void FunctionCodeGenerator::visit(ast::Gosub *n) {
	assert("UNIMPLEMENTED FUNCTION" && 0);
}

void FunctionCodeGenerator::visit(ast::Label *n) {
	llvm::BasicBlock *bb = createBasicBlock("label_" + n->name().toStdString());
	if (!mUnreachableBasicBlock) {
		mBuilder->branch(bb);
	}
	mBuilder->setInsertPoint(bb);
	mUnreachableBasicBlock = false;

	Symbol *sym = mLocalScope->find(n->name());
	assert(sym && sym->type() == Symbol::stLabel);

	LabelSymbol *label = static_cast<LabelSymbol*>(sym);
	label->setBasicBlock(bb);
}

void FunctionCodeGenerator::visit(ast::Exit *n) {
	CHECK_UNREACHABLE(n->codePoint());

	if (mExitStack.empty()) {
		emit error(ErrorCodes::ecInvalidExit, tr("Nothing to exit from"), n->codePoint());
		return;
	}

	llvm::BasicBlock *bb = mExitStack.pop();
	mBuilder->branch(bb);
	mUnreachableBasicBlock = true;
}


Value FunctionCodeGenerator::generate(ast::Integer *n) {
	return Value(ConstantValue(n->value()), mRuntime);
}


Value FunctionCodeGenerator::generate(ast::String *n) {
	return Value(ConstantValue(n->value()), mRuntime);
}

Value FunctionCodeGenerator::generate(ast::Float *n) {
	return Value(ConstantValue(n->value()), mRuntime);
}

Value FunctionCodeGenerator::generate(ast::Variable *n) {
	return generate(n->identifier());
}

Value FunctionCodeGenerator::generate(ast::Identifier *n) {
	Symbol *symbol = mLocalScope->find(n->name());
	assert(symbol);
	switch (symbol->type()) {
		case Symbol::stVariable: {
			VariableSymbol *varSym = static_cast<VariableSymbol*>(symbol);
			return Value(varSym->valueType(), varSym->alloca_(), true);
		}
		case Symbol::stConstant: {
			ConstantSymbol *constant = static_cast<ConstantSymbol*>(symbol);
			return Value(constant->value(), mRuntime);
		}
		case Symbol::stFunctionOrCommand: {
			FunctionSymbol *funcSym = static_cast<FunctionSymbol*>(symbol);
			if (funcSym->functions().size() == 1) {
				Function *func = funcSym->functions().first();
				return Value(func->functionValueType(), func->function(), false);
			}

			return Value(funcSym->functionSelector());
		}
		case Symbol::stLabel:
			emit error(ErrorCodes::ecNotVariable, tr("Symbol \"%1\" is a label defined [%2]").arg(symbol->name(), symbol->codePoint().toString()), n->codePoint());
			throw CodeGeneratorError(ErrorCodes::ecNotVariable);
			return Value();
		case Symbol::stValueType: {
			ValueTypeSymbol *valTySymbol = static_cast<ValueTypeSymbol*>(symbol);
			return Value(valTySymbol->valueType());
		}
		case Symbol::stType: {
			TypeSymbol *type = static_cast<TypeSymbol*>(symbol);
			return Value(type->valueType());
		}
		default:
			assert("Invalid Symbol::Type");
			return Value();
	}
}

Value FunctionCodeGenerator::generate(ast::Expression *n) {
	if (n->associativity() == ast::Expression::LeftToRight) {
		Value op1 = generate(n->firstOperand());
		for (QList<ast::ExpressionNode*>::ConstIterator i = n->operations().begin(); i != n->operations().end(); ++i) {
			ast::ExpressionNode *exprNode = *i;
			if (exprNode->op() == ast::ExpressionNode::opMember) {
				ValueType *valueType = op1.valueType();
				QString memberName;
				ast::Node *memberId = exprNode->operand();
				switch (memberId->type()) {
					case ast::Node::ntIdentifier:
						memberName = memberId->cast<ast::Identifier>()->name();
						break;
					case ast::Node::ntVariable: {
						ast::Variable *var = memberId->cast<ast::Variable>();
						memberName = var->identifier()->name();
						if (var->valueType()->type() != ast::Node::ntDefaultType) {
							ValueType *memberType = valueType->memberType(memberName);
							ValueType *resolvedType = mTypeResolver.resolve(var->valueType());
							if (resolvedType && memberType) {
								if (memberType != resolvedType) {
									emit error(ErrorCodes::ecVariableAlreadyDefinedWithAnotherType, tr("Member (\"%1\") is already defined with type \"%2\"").arg(resolvedType->name(), memberType->name()), memberId->codePoint());
								}
							}
						}
						break;

					}
					default:
						assert("Invalid ast::Node::Type" && 0);
				}

				if (!valueType->hasMember(memberName)) {
					emit error(ErrorCodes::ecCantFindTypeField, tr("Can't find type field \"%1\"").arg(memberName), memberId->codePoint());
					throw CodeGeneratorError(ErrorCodes::ecCantFindTypeField);
				}

				op1 = valueType->member(mBuilder, op1, memberName);
			}
			else {
				Value op2 = generate(exprNode->operand());
				assert(op1.isValid());
				assert(op2.isValid());

				ValueType *valueType = op1.valueType();
				OperationFlags opFlags;

				if (op1.isConstant() ^ op2.isConstant()) {
					op1.toLLVMValue(mBuilder);
					op2.toLLVMValue(mBuilder);
				}

				Value result = valueType->generateOperation(mBuilder, exprNode->op(), op1, op2, opFlags);

				if (opFlags.testFlag(OperationFlag::MayLosePrecision)) {
					emit warning(WarningCodes::wcMayLosePrecision, tr("Operation \"%1\" may lose precision with operands of types \"%2\" and \"%3\"").arg(ast::ExpressionNode::opToString(exprNode->op()), valueType->name(), op2.valueType()->name()), exprNode->codePoint());
				}
				if (opFlags.testFlag(OperationFlag::IntegerDividedByZero)) {
					emit error(ErrorCodes::ecIntegerDividedByZero, tr("Integer divided by zero"), exprNode->codePoint());
					throw CodeGeneratorError(ErrorCodes::ecIntegerDividedByZero);
					return Value();
				}

				if (opFlags.testFlag(OperationFlag::ReferenceRequired)) {
					emit error(ErrorCodes::ecInvalidAssignment, tr("You can only assign a value to a reference"), exprNode->codePoint());
					throw CodeGeneratorError(ErrorCodes::ecInvalidAssignment);
					return Value();
				}
				if (opFlags.testFlag(OperationFlag::CastFromString)) {
					emit warning(WarningCodes::wcMayLosePrecision, tr("Automatic cast from a string to a number."), exprNode->codePoint());
				}

				if (opFlags.testFlag(OperationFlag::NoSuchOperation)) {
					emit error(ErrorCodes::ecMathematicalOperationOperandTypeMismatch, tr("No operation \"%1\" between operands of types \"%2\" and \"%3\"").arg(ast::ExpressionNode::opToString(exprNode->op()), valueType->name(), op2.valueType()->name()), exprNode->codePoint());
					throw CodeGeneratorError(ErrorCodes::ecMathematicalOperationOperandTypeMismatch);
					return Value();
				}

				assert(result.isValid());

				mBuilder->destruct(op1);
				mBuilder->destruct(op2);

				op1 = result;
			}
		}
		return op1;
	}
	else {
		QList<ast::ExpressionNode*>::ConstIterator i = --n->operations().end();
		Value op2 = generate((*i)->operand());
		ast::ExpressionNode::Op op = (*i)->op();
		CodePoint cp = (*i)->codePoint();
		OperationFlags opFlags;
		if (i != n->operations().begin()) {
			do {
				--i;
				Value op1 = generate((*i)->operand());
				ValueType *valueType = op1.valueType();

				if (op1.isConstant() ^ op2.isConstant()) {
					op1.toLLVMValue(mBuilder);
					op2.toLLVMValue(mBuilder);
				}
				assert(op1.isValid());
				assert(op2.isValid());

				Value result = valueType->generateOperation(mBuilder, op, op1, op2, opFlags);

				if (opFlags.testFlag(OperationFlag::MayLosePrecision)) {
					emit warning(WarningCodes::wcMayLosePrecision, tr("Operation \"%1\" may lose precision with operands of types \"%2\" and \"%3\"").arg(ast::ExpressionNode::opToString(op), valueType->name(), op2.valueType()->name()), cp);
				}
				if (opFlags.testFlag(OperationFlag::IntegerDividedByZero)) {
					emit error(ErrorCodes::ecIntegerDividedByZero, tr("Integer divided by zero"), cp);
					throw CodeGeneratorError(ErrorCodes::ecIntegerDividedByZero);
				}

				if (opFlags.testFlag(OperationFlag::ReferenceRequired)) {
					emit error(ErrorCodes::ecInvalidAssignment, tr("You can only assign a value to a reference"), cp);
					throw CodeGeneratorError(ErrorCodes::ecInvalidAssignment);
				}
				if (opFlags.testFlag(OperationFlag::CastFromString)) {
					emit warning(WarningCodes::wcMayLosePrecision, tr("Automatic cast from a string to a number."), cp);
				}

				if (opFlags.testFlag(OperationFlag::NoSuchOperation)) {
					emit error(ErrorCodes::ecMathematicalOperationOperandTypeMismatch, tr("No operation \"%1\" between operands of types \"%2\" and \"%3\"").arg(ast::ExpressionNode::opToString(op), valueType->name(), op2.valueType()->name()), cp);
					throw CodeGeneratorError(ErrorCodes::ecMathematicalOperationOperandTypeMismatch);
				}

				mBuilder->destruct(op1);
				mBuilder->destruct(op2);

				op = (*i)->op();
				cp = (*i)->codePoint();

				assert(result.isValid());
				op2 = result;
			} while (i != n->operations().begin());
		}
		Value op1 = generate(n->firstOperand());
		ValueType *valueType = op1.valueType();
		Value result = valueType->generateOperation(mBuilder, op, op1, op2, opFlags);

		if (opFlags.testFlag(OperationFlag::MayLosePrecision)) {
			emit warning(WarningCodes::wcMayLosePrecision, tr("Operation \"%1\" may lose precision with operands of types \"%2\" and \"%3\"").arg(ast::ExpressionNode::opToString(op), valueType->name(), op2.valueType()->name()), cp);
		}
		if (opFlags.testFlag(OperationFlag::IntegerDividedByZero)) {
			emit error(ErrorCodes::ecIntegerDividedByZero, tr("Integer divided by zero"), cp);
			throw CodeGeneratorError(ErrorCodes::ecIntegerDividedByZero);
		}

		if (opFlags.testFlag(OperationFlag::ReferenceRequired)) {
			emit error(ErrorCodes::ecInvalidAssignment, tr("You can only assign a value to a reference"), cp);
			throw CodeGeneratorError(ErrorCodes::ecInvalidAssignment);
		}
		if (opFlags.testFlag(OperationFlag::CastFromString)) {
			emit warning(WarningCodes::wcMayLosePrecision, tr("Automatic cast from a string to a number."), cp);
		}

		if (opFlags.testFlag(OperationFlag::NoSuchOperation)) {
			emit error(ErrorCodes::ecMathematicalOperationOperandTypeMismatch, tr("No operation \"%1\" between operands of types \"%2\" and \"%3\"").arg(ast::ExpressionNode::opToString(op), valueType->name(), op2.valueType()->name()), cp);
			throw CodeGeneratorError(ErrorCodes::ecMathematicalOperationOperandTypeMismatch);
		}
		assert(result.isValid());

		return result;
	}
}

Value FunctionCodeGenerator::generate(ast::Unary *n) {
	Value val = generate(n->operand());
	OperationFlags flags;
	ValueType *valueType = val.valueType();
	Value result = val.valueType()->generateOperation(mBuilder, n->op(), val, flags);
	if (flags.testFlag(OperationFlag::MayLosePrecision)) {
		emit warning(WarningCodes::wcMayLosePrecision, tr("Operation \"%1\" may lose precision with operand of type \"%2\"").arg(ast::Unary::opToString(n->op()), valueType->name()), n->codePoint());
	}

	if (flags.testFlag(OperationFlag::CastFromString)) {
		emit warning(WarningCodes::wcMayLosePrecision, tr("Automatic cast from a string to a number."), n->codePoint());
	}

	if (flags.testFlag(OperationFlag::NoSuchOperation)) {
		emit error(ErrorCodes::ecMathematicalOperationOperandTypeMismatch, tr("No operation \"%1\" for operand of type \"%2\"").arg(ast::Unary::opToString(n->op()), valueType->name()), n->codePoint());
		throw CodeGeneratorError(ErrorCodes::ecMathematicalOperationOperandTypeMismatch);
	}

	return result;
}

Value FunctionCodeGenerator::generate(ast::FunctionCall *n) {
	Value functionValue = generate(n->function());
	ValueType *valueType = functionValue.valueType();
	QList<Value> paramValues = generateParameterList(n->parameters());

	if (functionValue.isFunctionSelectorValueType()) {
		FunctionSelectorValueType *funcSelector = static_cast<FunctionSelectorValueType*>(valueType);
		QList<Function*> functions = funcSelector->overloads();

		Function *func = findBestOverload(functions, paramValues, n->isCommand(), n->codePoint());
		assert(func);

		return mBuilder->call(func, paramValues);
	}
	else if (functionValue.isValueType()) {
		//Cast function
		if (paramValues.size() != 1) {
			emit error(ErrorCodes::ecCastFunctionRequiresOneParameter, tr("Cast functions take one parameter"), n->codePoint());
			throw CodeGeneratorError(ErrorCodes::ecCastFunctionRequiresOneParameter);
		}

		Value val = paramValues.first();
		ValueType::CastCost cc = val.valueType()->castingCostToOtherValueType(valueType);
		if (cc == ValueType::ccNoCast) {
			emit error(ErrorCodes::ecCantCastValue, tr("Can't cast \"%1\" to \"%2\"").arg(val.valueType()->name(), valueType->name()), n->codePoint());
			throw CodeGeneratorError(ErrorCodes::ecCantCastValue);
		}

		return valueType->cast(mBuilder, val);
	}
	else {
		if (!valueType->isCallable()) {
			emit error(ErrorCodes::ecCantFindFunction, tr("%1 is not a function and can't be called").arg(valueType->name()), n->codePoint());
			throw CodeGeneratorError(ErrorCodes::ecCantFindFunction);
		}
		FunctionValueType *functionValueType = static_cast<FunctionValueType*>(valueType);
		llvm::Value *llvmFunction = functionValue.value();
		return mBuilder->call(functionValueType, llvmFunction, paramValues);
	}
}

Value FunctionCodeGenerator::generate(ast::KeywordFunctionCall *n) {
	QList<Value> paramValues = generateParameterList(n->parameters());
	if (paramValues.size() != 1) {
		emit error(ErrorCodes::ecWrongNumberOfParameters, tr("This function takes one parameter"), n->codePoint());
		throw CodeGeneratorError(ErrorCodes::ecWrongNumberOfParameters);
	}
	const Value &param = paramValues.first();
	switch (n->keyword()) {
		case ast::KeywordFunctionCall::New: {
			if (!param.isValueType() || !param.valueType()->isTypePointer()) {
				emit error(ErrorCodes::ecNotTypeName, tr("\"New\" is expecting a type as a parameter. Invalid parameter type \"%1\"").arg(param.valueType()->name()), n->codePoint());
				throw CodeGeneratorError(ErrorCodes::ecNotTypeName);
			}
			TypePointerValueType *typePointerValueType = static_cast<TypePointerValueType*>(param.valueType());
			return mBuilder->newTypeMember(typePointerValueType->typeSymbol());
		}

		case ast::KeywordFunctionCall::First: {
			if (!param.isValueType() || !param.valueType()->isTypePointer()) {
				emit error(ErrorCodes::ecNotTypeName, tr("\"First\" is expecting a type as a parameter. Invalid parameter type \"%1\"").arg(param.valueType()->name()), n->codePoint());
				throw CodeGeneratorError(ErrorCodes::ecNotTypeName);
			}
			TypePointerValueType *typePointerValueType = static_cast<TypePointerValueType*>(param.valueType());
			return mBuilder->firstTypeMember(typePointerValueType->typeSymbol());
		}
		case ast::KeywordFunctionCall::Last: {
			if (!param.isValueType() || !param.valueType()->isTypePointer()) {
				emit error(ErrorCodes::ecNotTypeName, tr("\"First\" is expecting a type as a parameter. Invalid parameter type \"%1\"").arg(param.valueType()->name()), n->codePoint());
				throw CodeGeneratorError(ErrorCodes::ecNotTypeName);
			}
			TypePointerValueType *typePointerValueType = static_cast<TypePointerValueType*>(param.valueType());
			return mBuilder->lastTypeMember(typePointerValueType->typeSymbol());
		}

		case ast::KeywordFunctionCall::Before: {
			ValueType *valueType = param.valueType();
			if (!valueType->isTypePointer()) {
				emit error(ErrorCodes::ecNotTypePointer, tr("\"Before\" takes a type pointer as a parameter. Invalid parameter type \"%1\"").arg(valueType->name()), n->codePoint());
				throw CodeGeneratorError(ErrorCodes::ecNotTypePointer);
			}
			return mBuilder->beforeTypeMember(param);
		}
		case ast::KeywordFunctionCall::After: {
			ValueType *valueType = param.valueType();
			if (!valueType->isTypePointer()) {
				emit error(ErrorCodes::ecNotTypePointer, tr("\"After\" takes a type pointer as a parameter. Invalid parameter type \"%1\"").arg(valueType->name()), n->codePoint());
				throw CodeGeneratorError(ErrorCodes::ecNotTypePointer);
			}
			return mBuilder->afterTypeMember(param);
		}
		default:
			assert("Invalid ast::KeywordFunctionCall::KeywordFunction" && 0);
	}

	assert("UNIMPLEMENTED FUNCTION " && 0);
	return Value();
}

Value FunctionCodeGenerator::generate(ast::ArraySubscript *n) {
	assert("UNIMPLEMENTED FUNCTION " && 0);
	return Value();
}

Value FunctionCodeGenerator::generate(ast::Node *n) {
	Value result;
	switch (n->type()) {
		case ast::Node::ntBlock:
			result = generate(n->cast<ast::Block>()); break;
		case ast::Node::ntInteger:
			result = generate(n->cast<ast::Integer>()); break;
		case ast::Node::ntFloat:
			result = generate(n->cast<ast::Float>()); break;
		case ast::Node::ntString:
			result = generate(n->cast<ast::String>()); break;
		case ast::Node::ntIdentifier:
			result = generate(n->cast<ast::Identifier>()); break;
		case ast::Node::ntLabel:
			result = generate(n->cast<ast::Label>()); break;
		case ast::Node::ntList:
			result = generate(n->cast<ast::List>()); break;
		case ast::Node::ntGoto:
			result = generate(n->cast<ast::Goto>()); break;
		case ast::Node::ntGosub:
			result = generate(n->cast<ast::Gosub>()); break;
		case ast::Node::ntReturn:
			result = generate(n->cast<ast::Return>()); break;
		case ast::Node::ntExit:
			result = generate(n->cast<ast::Exit>()); break;


		case ast::Node::ntExpression:
			result = generate(n->cast<ast::Expression>()); break;
		case ast::Node::ntExpressionNode:
			result = generate(n->cast<ast::ExpressionNode>()); break;
		case ast::Node::ntUnary:
			result = generate(n->cast<ast::Unary>()); break;
		case ast::Node::ntArraySubscript:
			result = generate(n->cast<ast::ArraySubscript>()); break;
		case ast::Node::ntFunctionCall:
			result = generate(n->cast<ast::FunctionCall>()); break;
		case ast::Node::ntKeywordFunctionCall:
			result = generate(n->cast<ast::KeywordFunctionCall>()); break;
		case ast::Node::ntDefaultValue:
			result = generate(n->cast<ast::DefaultValue>()); break;
		case ast::Node::ntVariable:
			result = generate(n->cast<ast::Variable>()); break;

		default:
			assert("Invalid ast::Node for generate(ast::Node*)" && 0);
			return Value();
	}
	assert(result.isValid());
	return result;
}

Function *FunctionCodeGenerator::findBestOverload(const QList<Function *> &functions, const QList<Value> &parameters, bool command, const CodePoint &cp) {
	//No overloads
	if (functions.size() == 1) {
		Function *f = functions.first();

		if (!(f->isCommand() == command || (command == false && parameters.size() == 1))) {
			if (command) {
				emit error(ErrorCodes::ecNotCommand, tr("There is no command \"%1\" (but there is a function with same name)").arg(f->name()), cp);
				throw CodeGeneratorError(ErrorCodes::ecNotCommand);
			} else {
				emit error(ErrorCodes::ecCantFindFunction,  tr("There is no function \"%1\" (but there is a command with same name)").arg(f->name()), cp);
				throw CodeGeneratorError(ErrorCodes::ecCantFindFunction);
			}
		}

		if (f->requiredParams() > parameters.size()) {
			emit error(ErrorCodes::ecCantFindFunction, tr("Function doesn't match given parameters. \"%1\" was tried to call with parameters of types (%2)").arg(
						   f->functionValueType()->name(),
						   listStringJoin(parameters, [](const Value &val) {
				return val.valueType()->name();
			})), cp);
			throw CodeGeneratorError(ErrorCodes::ecCantFindFunction);
		}

		Function::ParamList::ConstIterator p1i = f->paramTypes().begin();
		CastCostCalculator totalCost;
		for (QList<Value>::ConstIterator p2i = parameters.begin(); p2i != parameters.end(); p2i++) {
			totalCost += p2i->valueType()->castingCostToOtherValueType(*p1i);
			p1i++;
		}
		if (!totalCost.isCastPossible()) {
			if (command) {
				emit error(ErrorCodes::ecCantFindFunction, tr("Command doesn't match given parameters. \"%1\" was tried to call with parameters of types (%2)").arg(
							   f->functionValueType()->name(),
							   listStringJoin(parameters, [](const Value &val) {
					return val.valueType()->name();
				})), cp);
				throw CodeGeneratorError(ErrorCodes::ecCantFindCommand);
			}
			else {
				emit error(ErrorCodes::ecCantFindFunction, tr("Function doesn't match given parameters. \"%1\" was tried to call with parameters of types (%2)").arg(
							   f->functionValueType()->name(),
							   listStringJoin(parameters, [](const Value &val) {
					return val.valueType()->name();
				})), cp);
				throw CodeGeneratorError(ErrorCodes::ecCantFindFunction);
			}
		}
		return f;
	}
	bool multiples = false;
	Function *bestFunc = 0;
	CastCostCalculator bestCost = CastCostCalculator::maxCastCost();

	for (QList<Function*>::ConstIterator fi = functions.begin(); fi != functions.end(); fi++) {
		Function *f = *fi;
		if (f->paramTypes().size() >= parameters.size() && f->requiredParams() <= parameters.size() && (f->isCommand() == command || (command == false && parameters.size() == 1))) {
			QList<ValueType*>::ConstIterator p1i = f->paramTypes().begin();
			CastCostCalculator totalCost;
			for (QList<Value>::ConstIterator p2i = parameters.begin(); p2i != parameters.end(); p2i++) {
				totalCost += p2i->valueType()->castingCostToOtherValueType(*p1i);
				p1i++;
			}
			if (totalCost == bestCost) {
				multiples = true;
				continue;
			}
			if (totalCost < bestCost) {
				bestFunc = f;
				bestCost = totalCost;
				multiples = false;
				continue;
			}
		}
	}
	if (bestFunc == 0) {
		if (command) {
			emit error(ErrorCodes::ecCantFindCommand, tr("Can't find a command overload which would accept given parameters (%1)").arg(
						   listStringJoin(parameters, [](const Value &val) {
				return val.valueType()->name();
			})), cp);
			throw CodeGeneratorError(ErrorCodes::ecCantFindCommand);
		}
		else {
			emit error(ErrorCodes::ecCantFindFunction, tr("Can't find a function overload which would accept given parameters (%1)").arg(
						   listStringJoin(parameters, [](const Value &val) {
				return val.valueType()->name();
			})), cp);
			throw CodeGeneratorError(ErrorCodes::ecCantFindFunction);
		}

	}
	if (multiples) {
		QString msg = command ? tr("Found multiple possible command overloads with parameters (%1) and can't choose between them.") : tr("Found multiple possible function overloads with parameters (%1) and can't choose between them.");
		emit error(ErrorCodes::ecMultiplePossibleOverloads,  msg.arg(
					   listStringJoin(parameters, [](const Value &val) {
			return val.valueType()->name();
		})), cp);
		throw CodeGeneratorError(ErrorCodes::ecMultiplePossibleOverloads);
	}
	return bestFunc;
}

QList<Value> FunctionCodeGenerator::generateParameterList(ast::Node *n) {
	QList<Value> result;
	if (n->type() == ast::Node::ntList) {
		for (ast::ChildNodeIterator i = n->childNodesBegin(); i != n->childNodesEnd(); i++) {
			result.append(generate(*i));
		}
	}
	else {
		result.append(generate(n));
	}
	return result;
}

void FunctionCodeGenerator::resolveGotos() {
	for (const QPair<LabelSymbol*, llvm::BasicBlock*> &g : mUnresolvedGotos) {
		assert(g.first->basicBlock());
		mBuilder->setInsertPoint(g.second);
		mBuilder->branch(g.first->basicBlock());
	}
}

bool FunctionCodeGenerator::generateAllocas() {
	for (Scope::Iterator i = mLocalScope->begin(); i != mLocalScope->end(); ++i) {
		Symbol *symbol = *i;
		if (symbol->type() == Symbol::stVariable) {
			VariableSymbol *varSymbol = static_cast<VariableSymbol*>(symbol);
			mBuilder->construct(varSymbol);
		}
	}
	return true;
}

void FunctionCodeGenerator::generateDestructors() {
	for (Scope::Iterator i = mLocalScope->begin(); i != mLocalScope->end(); ++i) {
		Symbol *symbol = *i;
		if (symbol->type() == Symbol::stVariable) {
			VariableSymbol *varSymbol = static_cast<VariableSymbol*>(symbol);
			mBuilder->destruct(varSymbol);
		}
	}
}

llvm::BasicBlock *FunctionCodeGenerator::createBasicBlock(const llvm::Twine &name, llvm::BasicBlock *insertBefore) {
	return llvm::BasicBlock::Create(mBuilder->context(), name, mFunction, insertBefore);
}

bool FunctionCodeGenerator::checkUnreachable(CodePoint cp) {
	if (mUnreachableBasicBlock) {
		emit warning(WarningCodes::wcUnreachableCode, tr("Unreachable code"), cp);
		return true;
	}
	return false;
}

void FunctionCodeGenerator::generateFunctionParameterAssignments(const QList<CBFunction::Parameter> &parameters) {
	QList<CBFunction::Parameter>::ConstIterator pi = parameters.begin();
	for (llvm::Function::arg_iterator i = mFunction->arg_begin(); i != mFunction->arg_end(); ++i) {
		mBuilder->store(pi->mVariableSymbol, i);
		pi++;
	}
}


void FunctionCodeGenerator::errorOccured(int, QString, CodePoint) {
	mValid = false;
}
