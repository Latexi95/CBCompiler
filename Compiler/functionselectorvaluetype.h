#ifndef FUNCTIONSELECTORVALUETYPE_H
#define FUNCTIONSELECTORVALUETYPE_H
#include "valuetype.h"

class FunctionSelectorValueType : public ValueType {
	public:
		FunctionSelectorValueType(Runtime *runtime, const QList<Function*> &functions);
		~FunctionSelectorValueType() {}

		QString name() const;
		virtual bool isNamedValueType() const { return false; }
		llvm::Type *llvmType() const {return mType;}
		bool isTypePointer() const { return false; }
		bool isNumber() const { return false; }
		virtual llvm::Constant *defaultValue() const;

		virtual int size() const;

		virtual CastCost castingCostToOtherValueType(ValueType *to);

		virtual Value cast(Builder *, const Value &v) const;

		virtual ValueType *operatorResultType(int opType, ValueType *operand1, ValueType *operand2, OperationFlags &operationFlags) const { return 0; }
		virtual ValueType *operatorResultType(int opType, ValueType *operand, OperationFlags &operationFlags) const { return 0; }
		bool hasOperator(int opType, ValueType *operand1, ValueType *operand2) const { return operatorResultType(opType, operand1, operand2) != 0; }
		bool hasOperator(int opType, ValueType *operand) const { return operatorResultType(opType, operand) != 0; }
		virtual Value generateOperation(int opType, const Value &operand1, const Value &operand2) const { return Value();}
		virtual Value generateOperation(int opType, const Value &operand) const { return Value();}

		virtual bool isCallable() const { return true; }


	protected:
		QList<Function*> mFunctions;
};

#endif // FUNCTIONSELECTORVALUETYPE_H
