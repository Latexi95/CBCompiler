#ifndef TYPEPOINTERVALUETYPE_H
#define TYPEPOINTERVALUETYPE_H
#include "valuetype.h"
class TypeSymbol;
class TypePointerValueType : public ValueType {
	public:
		TypePointerValueType(Runtime *r, TypeSymbol *s);
		virtual std::string name()const;
		virtual CastCost castingCostToOtherValueType(const ValueType *to) const;
		virtual Value cast(Builder *builder, const Value &v) const;
		TypeSymbol *typeSymbol() const {return mTypeSymbol;}
		bool isTypePointer() const{return true;}
		bool isNumber() const{return false;}
		llvm::Constant *defaultValue() const;
		int size() const;
		void setLLVMType(llvm::Type *type) { mType = type; }
		bool isNamedValueType() const { return true; }
		virtual Value generateOperation(Builder *builder, int opType, const Value &operand1, const Value &operand2, OperationFlags &operationFlags) const;
		virtual Value member(Builder *builder, const Value &a, const std::string &memberName) const;
		virtual ValueType *memberType(const std::string &memberName) const;
	private:
		TypeSymbol *mTypeSymbol;
};

class TypePointerCommonValueType : public ValueType {
	public:
		TypePointerCommonValueType(Runtime *r, llvm::Type *type) : ValueType(r) { mType = type; }
		virtual std::string name() const { return "TypePointerCommon"; }
		virtual CastCost castingCostToOtherValueType(const ValueType *to) const;
		virtual Value cast(Builder *builder, const Value &v) const;
		TypeSymbol *typeSymbol() const {return mTypeSymbol;}
		bool isTypePointer() const{return true;}
		bool isNumber() const{return false;}
		llvm::Constant *defaultValue() const;
		bool isNamedValueType() const { return false; }
		int size() const;
		virtual Value generateOperation(Builder *builder, int opType, const Value &operand1, const Value &operand2, OperationFlags &operationFlags) const;
	private:
		TypeSymbol *mTypeSymbol;
};
#endif // TYPEPOINTERVALUETYPE_H
