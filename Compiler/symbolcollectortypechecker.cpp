#include "symbolcollectortypechecker.h"
#include "scope.h"
#include "runtime.h"
#include "errorcodes.h"
#include "variablesymbol.h"
#include "constantsymbol.h"
#include "typesymbol.h"
#include "functionsymbol.h"
#include "conversionhelper.h"
#include "typepointervaluetype.h"
#include "intvaluetype.h"
#include "floatvaluetype.h"
#include "stringvaluetype.h"
#include "shortvaluetype.h"
#include "bytevaluetype.h"
#include "booleanvaluetype.h"
#include "arraysymbol.h"
#include "labelsymbol.h"
#include <QString>
SymbolCollectorTypeChecker::SymbolCollectorTypeChecker():
	mForceVariableDeclaration(false),
	mGlobalScope(0),
	mReturnValueType(0)
{
}

ValueType *SymbolCollectorTypeChecker::typeCheck(ast::Node *s) {
	switch(s->type()) {
		case ast::Node::ntInteger:
			return mRuntime->intValueType();
		case ast::Node::ntString:
			return mRuntime->stringValueType();
		case ast::Node::ntFloat:
			return mRuntime->floatValueType();
		case ast::Node::ntExpression:
			return typeCheck((ast::Expression*)s);
		case ast::Node::ntFunctionCallOrArraySubscript:
			return typeCheck((ast::FunctionCallOrArraySubscript*)s);
		case ast::Node::ntNew:
			return typeCheck((ast::New*)s);
		case ast::Node::ntVariable:
			return typeCheck((ast::Variable*)s);
		case ast::Node::ntUnary:
			return typeCheck((ast::Unary*)s);
		case ast::Node::ntTypePtrField:
			return typeCheck((ast::TypePtrField*)s);
		default:
			assert(0);
			return 0;
	}
}

ValueType *SymbolCollectorTypeChecker::typeCheck(ast::Expression *s) {
	ValueType *first = typeCheck(s->mFirst);
	if (!first) return 0;

	for (QList<ast::Operation>::ConstIterator i = s->mRest.begin(); i != s->mRest.end(); i++) {
		ValueType *second = typeCheck(i->mOperand);
		if (!second) return 0;
		ValueType *result = 0;
		switch (i->mOperator) {
			case ast::opEqual:
			case ast::opNotEqual:
				switch (first->type()) {
					case ValueType::Float:
					case ValueType::Integer:
					case ValueType::Byte:;
					case ValueType::Short:
					case ValueType::Boolean:
					case ValueType::String:
						switch (second->type()) {
							case ValueType::Float:
							case ValueType::Integer:
							case ValueType::Byte:
							case ValueType::Short:
							case ValueType::Boolean:
							case ValueType::String:
								result = mRuntime->booleanValueType(); break;
						}
						break;
					case ValueType::NULLTypePointer:
						switch (second->type()) {
							case ValueType::TypePointer:
							case ValueType::NULLTypePointer:
								result = mRuntime->booleanValueType();
								break;
						}
					case ValueType::TypePointer:
						switch (second->type()) {
							case ValueType::TypePointer:
								if (first == second) result = mRuntime->booleanValueType();
								break;
							case ValueType::NULLTypePointer:
								result = mRuntime->booleanValueType();
								break;
						}
				}
			case ast::opGreater:
			case ast::opLess:
			case ast::opGreaterEqual:
			case ast::opLessEqual:
				switch (first->type()) {
					case ValueType::Float:
					case ValueType::Integer:
					case ValueType::Byte:;
					case ValueType::Short:
					case ValueType::Boolean:
					case ValueType::String:
						switch (second->type()) {
							case ValueType::Float:
							case ValueType::Integer:
							case ValueType::Byte:
							case ValueType::Short:
							case ValueType::Boolean:
							case ValueType::String:
								result = mRuntime->booleanValueType(); break;
						}
						break;
				}
			case ast::opPlus:
				switch (first->type()) {
					case ValueType::Float:
						switch (second->type()) {
							case ValueType::Float:
							case ValueType::Integer:
							case ValueType::Byte:
							case ValueType::Short:
							case ValueType::Boolean:
								result = mRuntime->floatValueType(); break;
							case ValueType::String:
								result = mRuntime->stringValueType(); break;
						}
						break;
					case ValueType::Integer:
					case ValueType::Byte:;
					case ValueType::Short:
					case ValueType::Boolean:
						switch (second->type()) {
							case ValueType::Float:
								result = mRuntime->floatValueType(); break;
							case ValueType::Integer:
							case ValueType::Byte:
							case ValueType::Short:
							case ValueType::Boolean:
								result = mRuntime->intValueType(); break;
							case ValueType::String:
								result = mRuntime->stringValueType(); break;
						}
						break;
					case ValueType::String:
						switch (second->type()) {
							case ValueType::Float:
							case ValueType::Integer:
							case ValueType::Byte:
							case ValueType::Short:
							case ValueType::String:
								result = mRuntime->stringValueType(); break;
						}
						break;
				}
				break;
			case ast::opMinus:
			case ast::opMultiply:
			case ast::opDivide:
			case ast::opPower:
			case ast::opMod:
				switch (first->type()) {
					case ValueType::Float:
						switch (second->type()) {
							case ValueType::Float:
							case ValueType::Integer:
							case ValueType::Byte:
							case ValueType::Short:
								result = mRuntime->floatValueType(); break;
						}
						break;
					case ValueType::Integer:
					case ValueType::Byte:
					case ValueType::Short:
					case ValueType::Boolean:
						switch (second->type()) {
							case ValueType::Float:
								result = mRuntime->floatValueType(); break;
							case ValueType::Integer:
							case ValueType::Byte:
							case ValueType::Short:
								result = mRuntime->intValueType(); break;
						}
						break;
				}
				break;
			case ast::opShl:
			case ast::opShr:
			case ast::opSar:
				switch (first->type()) {
					case ValueType::Integer:
					case ValueType::Byte:
					case ValueType::Short:
					case ValueType::Boolean:
						switch (second->type()) {
							case ValueType::Integer:
							case ValueType::Byte:
							case ValueType::Short:
							case ValueType::Boolean:
								result = mRuntime->intValueType(); break;
						}
						break;
				}
				break;
			case ast::opAnd:
			case ast::opOr:
			case ast::opXor:
				if (first->castCost(mRuntime->booleanValueType()) < ValueType::maxCastCost && second->castCost(mRuntime->booleanValueType()) < ValueType::maxCastCost ) {
					return mRuntime->booleanValueType();
				}
			default:
				assert(0);
				return 0;
		}
		if (!result) {
			emit error(ErrorCodes::ecMathematicalOperationOperandTypeMismatch,
					   tr("Mathematical operation operand type mismatch: No operation \"%1\" between %2 and %3").arg(ast::operatorToString(i->mOperator), first->name(), second->name()), mLine, mFile);
			return 0;
		}
		first = result;
	}
	return first;
}

ValueType *SymbolCollectorTypeChecker::typeCheck(ast::Unary *s) {
	ValueType *r = typeCheck(s->mOperand);
	if (!r) return 0;
	if (s->mOperator == ast::opNot) {
		return mRuntime->booleanValueType();
	}
	if (r == mRuntime->booleanValueType()) {
		return mRuntime->intValueType();
	}
	return r;
}

ValueType *SymbolCollectorTypeChecker::typeCheck(ast::New *s) {
	Symbol *sym = mScope->find(s->mTypeName);
	if (!sym) {
		emit error(ErrorCodes::ecCantFindType, tr("Can't find type with name \"%1\"").arg(s->mTypeName), mLine, mFile);
		return 0;
	}
	if (sym->type() != Symbol::stType) {
		emit error(ErrorCodes::ecCantFindType, tr("\"%1\" is not a type").arg(s->mTypeName), mLine, mFile);
		return 0;
	}
	return static_cast<TypeSymbol*>(sym)->typePointerValueType();
}

ValueType *SymbolCollectorTypeChecker::typeCheck(ast::FunctionCallOrArraySubscript *s) {
	mLine = s->mLine;
	mFile = s->mFile;
	Symbol *sym = mScope->find(s->mName);
	if (!sym) {
		emit error(ErrorCodes::ecCantFindSymbol, tr("Can't find function or array with name \"%1\"").arg(s->mName), mLine, mFile);
	}
	if (sym->type() == Symbol::stArray) {
		ArraySymbol *array = static_cast<ArraySymbol*>(sym);
		bool err = false;
		for (QList<ast::Node*>::ConstIterator i = s->mParams.begin(); i != s->mParams.end(); i++) {
			ValueType *p = typeCheck(*i);
			if (p) {
				if (!(p->type() == ValueType::Integer || p->type() == ValueType::Short || p->type() == ValueType::Byte)) {
					emit error(ErrorCodes::ecArraySubscriptNotInteger, tr("Array subscript should be integer value"), mLine, mFile);
					err = true;
				}
			}
			else {
				err = true;
			}
		}
		if (array->dimensions() != s->mParams.size()) {
			emit error(ErrorCodes::ecInvalidArraySubscript, tr("Array dimensions don't match, expecing %1 array subscripts").arg(QString::number(array->dimensions())), mLine, mFile);
			err = true;
		}
		if (err) return 0;
		return array->valueType();
	}
	else if (sym->type() == Symbol::stFunctionOrCommand) {
		FunctionSymbol *funcSym = static_cast<FunctionSymbol*>(sym);
		bool err;
		QList<ValueType*> params;
		for (QList<ast::Node*>::ConstIterator i = s->mParams.begin(); i != s->mParams.end(); i++) {
			ValueType *p = typeCheck(*i);
			if (p) {
				params.append(p);
			}
			else {
				err = true;
			}
		}
		if (err) return 0;
		Symbol::OverloadSearchError searchErr;
		Function *func = funcSym->findBestOverload(params, false, &searchErr);
		if (!func) {
			switch (searchErr) {
				case Symbol::oseCannotFindAny:
					emit error(ErrorCodes::ecCantFindFunction, tr("Can't find function \"%1\" with given parametres").arg(s->mName), mLine, mFile);
					return 0;
				case Symbol::oseFoundMultipleOverloads:
					emit error(ErrorCodes::ecMultiplePossibleOverloads, tr("Can't choose between function \"%1\" overloads").arg(s->mName), mLine, mFile);
					return 0;
				default:
					assert(0); //WTF?
					return 0;
			}
		}
		return func->returnValue();
	}
	else {
		emit error(ErrorCodes::ecNotArrayOrFunction, tr("Symbol \"%1\" is not a function or an array").arg(s->mName), mLine, mFile);
		return 0;

	}
}

ValueType *SymbolCollectorTypeChecker::typeCheck(ast::TypePtrField *s) {
	Symbol *sym = mScope->find(s->mTypePtrVar);
	if (!sym || sym->type() != Symbol::stVariable) {
		emit error(ErrorCodes::ecNotVariable, tr("\"%1\" is not a variable").arg(s->mTypePtrVar), mLine, mFile);
		return 0;
	}

	VariableSymbol *varSym = static_cast<VariableSymbol*>(sym);
	if (varSym->valueType()->type() != ValueType::TypePointer) {
		emit error(ErrorCodes::ecNotTypePointer, tr("\"%1\" is not a type pointer").arg(s->mTypePtrVar), mLine, mFile);
		return 0;
	}
	TypePointerValueType *typePtrVt = static_cast<TypePointerValueType*>(varSym->valueType());
	if (!typePtrVt->typeSymbol()->hasField(s->mFieldName)) {
		emit error(ErrorCodes::ecCantFindTypeField, tr("Type \"%1\" doesn't have field \"%2\"").arg(typePtrVt->name(), s->mFieldName), mLine, mFile);
		return 0;
	}

	const TypeField &field = typePtrVt->typeSymbol()->field(s->mFieldName);
	if (s->mFieldType != ast::Variable::Default && field.valueType()->type() != valueTypeFromVarType(s->mFieldType)) {
		emit error(ErrorCodes::ecVarAlreadyDefinedWithAnotherType, tr("Type field \"%1\" is defined with another type").arg(s->mFieldName), mLine, mFile);
		return 0;
	}

	return field.valueType();
}

ValueType *SymbolCollectorTypeChecker::typeCheck(ast::Variable *s) {
	return checkVariable(s->mName, s->mVarType, s->mTypeName);
}

bool SymbolCollectorTypeChecker::run(const ast::Block *block, Scope *scope) {
	mScope = scope;
	return checkBlock(block);
}

bool SymbolCollectorTypeChecker::checkBlock(const ast::Block *block) {
	bool valid = true;
	for (ast::Block::ConstIterator i = block->begin(); i != block->end(); i++) {
		if (!checkStatement(*i)) valid = false;
	}
	return valid;
}

bool SymbolCollectorTypeChecker::checkStatement(ast::Node *s) {
	switch(s->type()) {
		case ast::Node::ntArrayDefinition:
			return checkStatement((ast::ArrayDefinition*)s);
		case ast::Node::ntArraySubscriptAssignmentExpression:
			return checkStatement((ast::ArraySubscriptAssignmentExpression*)s);
		case ast::Node::ntIfStatement:
			return checkStatement((ast::IfStatement*)s);
		case ast::Node::ntWhileStatement:
			return checkStatement((ast::WhileStatement*)s);
		case ast::Node::ntAssignmentExpression:
			return checkStatement((ast::AssignmentExpression*)s);
		case ast::Node::ntForToStatement:
			return checkStatement((ast::ForToStatement*)s);
		case ast::Node::ntForEachStatement:
			return checkStatement((ast::ForEachStatement*)s);
		case ast::Node::ntVariableDefinition:
			return checkStatement((ast::VariableDefinition*)s);
		case ast::Node::ntSelectStatement:
			return checkStatement((ast::SelectStatement*)s);
		case ast::Node::ntRepeatForeverStatement:
			return checkStatement((ast::RepeatForeverStatement*)s);
		case ast::Node::ntRepeatUntilStatement:
			return checkStatement((ast::RepeatUntilStatement*)s);
		case ast::Node::ntCommandCall:
			return checkStatement((ast::CommandCall*)s);
		case ast::Node::ntFunctionCallOrArraySubscript:
			return checkStatement((ast::FunctionCallOrArraySubscript*)s);
		case ast::Node::ntReturn:
			return checkStatement((ast::Return*)s);
		default:
			assert(0);
	}
	return 0;
}

bool SymbolCollectorTypeChecker::checkStatement(ast::IfStatement *s) {
	mFile = s->mFile;
	mLine = s->mLine;
	ValueType *cond = typeCheck(s->mCondition);
	bool valid = true;
	if (!cond || !tryCastToBoolean(cond)) valid = false;
	if (!checkBlock(&s->mIfTrue)) valid = false;
	if (!checkBlock(&s->mElse)) valid = false;
	return valid;
}

bool SymbolCollectorTypeChecker::checkStatement(ast::WhileStatement *s) {
	mFile = s->mFile;
	mLine = s->mStartLine;
	ValueType *cond = typeCheck(s->mCondition);
	bool valid = true;
	if (!cond || !tryCastToBoolean(cond)) valid = false;
	if (!checkBlock(&s->mBlock)) valid = false;
	return valid;
}

bool SymbolCollectorTypeChecker::checkStatement(ast::AssignmentExpression *s) {
	mFile = s->mFile;
	mLine = s->mLine;
	ValueType *var = typeCheck(s->mVariable);
	ValueType *value = typeCheck(s->mExpression);
	bool valid = var && value;
	if (var && value) {
		if (var->isTypePointer() && !value->isTypePointer()) {
			emit error(ErrorCodes::ecInvalidAssignment, tr("Invalid assignment. Can't %1 value to type pointer").arg(value->name()), mLine, mFile);
			valid = false;
		}
		else if(!var->isTypePointer() && value->isTypePointer()) {
			emit error(ErrorCodes::ecInvalidAssignment, tr("Invalid assignment. Can't assign type pointer to %1 variable").arg(var->name()), mLine, mFile);
			valid = false;
		}
		else if (var->isTypePointer() && value->isTypePointer() && var != value) {
			emit error(ErrorCodes::ecInvalidAssignment, tr("Type pointer of type \"%1\" can not be assigned to a type pointer variable of type \"%2\"").arg(value->name(), var->name()), mLine, mFile);
			valid = false;
		}
	}

	return valid;
}

bool SymbolCollectorTypeChecker::checkStatement(ast::ArraySubscriptAssignmentExpression *s) {
	assert(0 && "STUB");
	return false;
}

bool SymbolCollectorTypeChecker::checkStatement(ast::ForToStatement *s) {
	mFile = s->mFile;
	mLine = s->mStartLine;
	ValueType *var = checkVariable(s->mVarName, s->mVarType);
	ValueType *from = typeCheck(s->mFrom);
	ValueType *to = typeCheck(s->mTo);
	bool valid = from != 0;
	if (var) {
		if (!var->isNumber()) {
			emit error(ErrorCodes::ecNotNumber, tr("Variable \"%1\" should be number").arg(s->mVarName), mLine, mFile);
			valid = false;
		}
	}
	else {
		valid = false;
	}
	if (to){
		if (!to->isNumber()) {
			emit error(ErrorCodes::ecNotNumber, tr("\"To\" value should be a number"), mLine, mFile);
			valid = false;
		}
	}
	else {
		valid = false;
	}
	if (s->mStep) {
		ValueType *step = typeCheck(s->mStep);
		if (step) {
			if (!step->isNumber()) {
				emit error(ErrorCodes::ecNotNumber, tr("\"Step\" value should be a number"), mLine, mFile);
				valid = false;
			}
		}
		else {
			valid = false;
		}
	}

	if (!checkBlock(&s->mBlock)) valid = false;
	return valid;
}

bool SymbolCollectorTypeChecker::checkStatement(ast::ForEachStatement *s) {
	ValueType *var = checkVariable(s->mVarName, ast::Variable::TypePtr, s->mTypeName);
	return var != 0 && checkBlock(&s->mBlock);
}

bool SymbolCollectorTypeChecker::checkStatement(ast::ArrayDefinition *s) {
	Symbol *sym = mScope->find(s->mName);
	if (sym) {
		emit error(ErrorCodes::ecVariableAlreadyDefined, tr("Symbol \"%1\" already defined").arg(s->mName), mLine, mFile);
		return false;
	}
	else {
		bool valid = true;
		for (QList<ast::Node*>::ConstIterator i = s->mDimensions.begin(); i != s->mDimensions.end(); i++) {
			if (!typeCheck(*i)) valid = false;
		}
		ArraySymbol *sym = new ArraySymbol(s->mName, mRuntime->findValueType(valueTypeFromVarType(s->mType)), s->mDimensions.size(), mFile, mLine);
		mGlobalScope->addSymbol(sym);
		return valid;
	}
}

bool SymbolCollectorTypeChecker::checkStatement(ast::VariableDefinition *s) {
	bool valid = true;
	mFile = s->mFile;
	mLine = s->mLine;
	for (QList<ast::Variable*>::ConstIterator i = s->mDefinitions.begin(); i != s->mDefinitions.end(); i++) {
		ast::Variable *var = *i;

		Symbol *sym = mScope->find(var->mName);
		if (sym) {
			//TODO: better error messages
			if (sym->type() == Symbol::stVariable) {
				emit error(ErrorCodes::ecVariableAlreadyDefined,
						   tr("Variable \"%1\" already defined at line %2 in file \"%3\"").arg(
							   var->mName, QString::number(sym->line()), sym->file()->fileName())
						   , mLine, mFile);
				valid = false;
				continue;
			}
			else {
				emit error(ErrorCodes::ecVariableAlreadyDefined, tr("Symbol \"%1\" already defined").arg(var->mName), mLine, mFile);
				valid = false;
				continue;
			}
		}
		else {
			if (var->mVarType != ast::Variable::TypePtr) {
				VariableSymbol *varSym = new VariableSymbol(var->mName, mRuntime->findValueType(valueTypeFromVarType(var->mVarType)), mFile, mLine);
				mScope->addSymbol(varSym);
			}
			else {
				ValueType *valType = checkTypePointerType(var->mTypeName);
				if (!valType) {
					valid = false;
					continue;
				}
				VariableSymbol *varSym = new VariableSymbol(var->mName, valType, mFile, mLine);
				mScope->addSymbol(varSym);
			}
		}
	}
	return valid;
}

bool SymbolCollectorTypeChecker::checkStatement(ast::SelectStatement *s) {
	assert(0 && "STUB");
	return false;
}

bool SymbolCollectorTypeChecker::checkStatement(ast::RepeatForeverStatement *s){
	return run(&s->mBlock, mScope);
}

bool SymbolCollectorTypeChecker::checkStatement(ast::RepeatUntilStatement *s) {
	bool valid = true;
	mLine = s->mEndLine;
	mFile = s->mFile;
	ValueType *condVt = typeCheck(s->mCondition);
	if (condVt) {
		valid = tryCastToBoolean(condVt);
	}
	else {
		valid = false;
	}
	if (!checkBlock(&s->mBlock)) valid = false;
	return valid;
}

bool SymbolCollectorTypeChecker::checkStatement(ast::CommandCall *s) {
	mLine = s->mLine;
	mFile = s->mFile;
	bool err;
	QList<ValueType*> params;
	for (QList<ast::Node*>::ConstIterator i = s->mParams.begin(); i != s->mParams.end(); i++) {
		ValueType *p = typeCheck(*i);
		if (p) {
			params.append(p);
		}
		else {
			err = true;
		}
	}
	if (err) return false;

	Symbol *sym = mScope->find(s->mName);
	if (!sym) {
		emit error(ErrorCodes::ecNotCommand, tr("Can't find symbol \"%1\"").arg(s->mName), mLine, mFile);
		return false;
	}
	if (sym->type() != Symbol::stFunctionOrCommand) {
		emit error(ErrorCodes::ecNotCommand, tr("Symbol \"%1\" is not a command").arg(s->mName), mLine, mFile);
		return false;
	}

	FunctionSymbol *funcSym = static_cast<FunctionSymbol*>(sym);
	Symbol::OverloadSearchError searchErr;
	Function *command = funcSym->findBestOverload(params, true, &searchErr);
	if (!command) {
		switch (searchErr) {
			case Symbol::oseCannotFindAny:
				emit error(ErrorCodes::ecCantFindCommand, tr("Can't find command \"%1\" with given parametres").arg(s->mName), mLine, mFile);
				return false;
			case Symbol::oseFoundMultipleOverloads:
				emit error(ErrorCodes::ecMultiplePossibleOverloads, tr("Can't choose between command \"%1\" overloads").arg(s->mName), mLine, mFile);
				return false;
			default:
				assert(0); //WTF?
				return false;
		}
	}
	return true;
}

bool SymbolCollectorTypeChecker::checkStatement(ast::FunctionCallOrArraySubscript *s) {
	return typeCheck(s) != 0;
}

bool SymbolCollectorTypeChecker::checkStatement(ast::Return *s) {
	mFile = s->mFile;
	mLine = s->mLine;
	if (mReturnValueType) { //Function
		if (!s->mValue) {
			emit error(ErrorCodes::ecExpectingValueAfterReturn, tr("Expecting value after Return"), mLine, mFile);
			return false;
		}
		ValueType *retVal = typeCheck(s->mValue);
		if (!retVal) return false;
		if (retVal->castCost(mReturnValueType) >= ValueType::maxCastCost) {
			emit error(ErrorCodes::ecInvalidReturn, tr("The type of the returned value doesn't match the return type of the function"), mLine, mFile);
			return false;
		}
		return true;
	}
	else { //Gosub return
		if (s->mValue) {
			emit error(ErrorCodes::ecGosubCannotReturnValue, tr("Gosub cannot return a value"), mLine, mFile);
			return false;
		}
		return true;
	}
}

bool SymbolCollectorTypeChecker::checkStatement(ast::Label *s) {
	mLine = s->mLine;
	mFile = s->mFile;
	Symbol *sym = mScope->find(s->mName);
	if (!sym) {
		LabelSymbol *label = new LabelSymbol(s->mName, mFile, mLine);
		mScope->addSymbol(label);
		return true;
	}
	if (sym->type() == Symbol::stLabel) {
		emit error(ErrorCodes::ecLabelAlreadyDefined, tr("Label already defined at line %1 in file %2").arg(QString::number(sym->line()), sym->file()->fileName()), mLine, mFile);
	}
	else {
		emit error(ErrorCodes::ecSymbolAlreadyDefined, tr("Symbol already defined at line %1 in file %2").arg(QString::number(sym->line()), sym->file()->fileName()), mLine, mFile);
	}

	return false;
}

ValueType *SymbolCollectorTypeChecker::checkTypePointerType(const QString &typeName) {
	Symbol *sym = mScope->find(typeName);
	if (!sym) {
		emit error(ErrorCodes::ecCantFindType, tr("Can't find type \"%1\"").arg(typeName), mLine, mFile);
		return 0;
	}
	if (sym->type() != Symbol::stType) {
		emit error(ErrorCodes::ecCantFindType, tr("\"%1\" is not a type").arg(typeName), mLine, mFile);
		return 0;
	}
	return static_cast<TypeSymbol*>(sym)->typePointerValueType();
}

bool SymbolCollectorTypeChecker::tryCastToBoolean(ValueType *t) {
	if (t->castCost(mRuntime->booleanValueType()) >= ValueType::maxCastCost) {
		emit error(ErrorCodes::ecNoCastFromTypePointer, tr("Required a value which can be casted to boolean"), mLine, mFile);
		return false;
	}
	return true;
}

ValueType *SymbolCollectorTypeChecker::checkVariable(const QString &name, ast::Variable::VarType type, const QString &typeName) {
	Symbol *sym = mScope->find(name);
	if (sym) {
		if (sym->type() == Symbol::stConstant) {
			ConstantSymbol *constant = static_cast<ConstantSymbol*>(sym);
			ValueType *valType = mRuntime->findValueType(constant->value().type());
			return valType;
		}
		else {
			if (sym->type() != Symbol::stVariable) {
				emit error(ErrorCodes::ecNotVariable, tr("\"%1\" is not a variable").arg(name), mLine, mFile);
				return 0;
			}
			VariableSymbol *var = static_cast<VariableSymbol*>(sym);
			if (type != ast::Variable::TypePtr) {
				if (type != ast::Variable::Default && valueTypeFromVarType(type) != var->valueType()->type()) {
					emit error(ErrorCodes::ecVarAlreadyDefinedWithAnotherType, tr("Variable already defined with another type"), mLine, mFile);
					return 0;
				}
			}
			else {
				ValueType *typePtr = checkTypePointerType(typeName);
				if (!typePtr) return 0;
				if (typePtr != var->valueType()) {
					emit error(ErrorCodes::ecVarAlreadyDefinedWithAnotherType, tr("Variable already defined with another type"), mLine, mFile);
					return 0;
				}
			}
			return var->valueType();
		}
	}
	else {
		if (mForceVariableDeclaration) {
			emit error(ErrorCodes::ecVariableNotDefined, tr("Variable \"%1\" is not defined").arg(name), mLine, mFile);
			return 0;
		}
		ValueType *vt;
		if(type == ast::Variable::TypePtr) { //Type pointer
			Symbol *typeSym = mScope->find(typeName);
			if (!typeSym) {
				emit error(ErrorCodes::ecCantFindType, tr("Can't find type with name \"%1\"").arg(typeName), mLine, mFile);
				return 0;
			}
			if (typeSym->type() != Symbol::stType) {
				emit error(ErrorCodes::ecCantFindType, tr("\"%1\" is not a type").arg(typeName), mLine, mFile);
				return 0;
			}
			vt = static_cast<TypeSymbol*>(typeSym)->typePointerValueType();
		}
		else {
			vt = mRuntime->findValueType(valueTypeFromVarType(type));
		}

		VariableSymbol *varSym = new VariableSymbol(name, vt, mFile, mLine);
		mScope->addSymbol(varSym);
		return vt;
	}
}

