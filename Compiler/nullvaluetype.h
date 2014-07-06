#ifndef NULLVALUETYPE_H
#define NULLVALUETYPE_H
#include "valuetype.h"

class NullValueType : public ValueType {
	public:
		NullValueType(Runtime *runtime);
		virtual QString name() const { return "Null"; }
		virtual CastCost castingCostToOtherValueType(const ValueType *to) const;
		virtual Value cast(Builder *builder, const Value &v) const;
		bool isTypePointer() const { return false; }
		bool isNumber() const{ return false; }
		llvm::Constant *defaultValue() const;
		bool isNamedValueType() const { return false; }
		int size() const;
		virtual Value generateOperation(Builder *builder, int opType, const Value &operand1, const Value &operand2, OperationFlags &operationFlags) const;
	private:
};

#endif // NULLVALUETYPE_H
