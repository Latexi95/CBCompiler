#ifndef CLASSVALUETYPE_H
#define CLASSVALUETYPE_H
#include "valuetype.h"
#include "codepoint.h"
#include <map>
#include <vector>

class StructField {
	public:
		StructField(const std::string &name, ValueType *valueType, const CodePoint &cp);
		std::string name()const{return mName;}
		ValueType *valueType()const{return mValueType;}
		std::string info() const;
		int line() const { return mCodePoint.line(); }
		boost::string_ref file() const { return mCodePoint.file(); }
		int column() const { return mCodePoint.column(); }
		const CodePoint &codePoint() const { return mCodePoint; }
	private:
		std::string mName;
		ValueType *mValueType;
		CodePoint mCodePoint;
};


class StructValueType : public ValueType {
	public:
		StructValueType(const std::string &name, const CodePoint &cp, Runtime *runtime);
		StructValueType(const std::string &name, const CodePoint &cp, const std::vector<StructField> &fields, Runtime *runtime);
		~StructValueType();
		bool containsValueType(const ValueType *valueType) const;
		bool containsItself() const;

		virtual std::string name() const;
		virtual CastCost castingCostToOtherValueType(const ValueType *to) const;
		virtual Value cast(Builder *builder, const Value &v) const;
		bool isTypePointer() const{return false;}
		bool isNumber() const{return false;}
		bool isStruct() const { return true; }
		llvm::Constant *defaultValue() const;
		int size() const;
		void setFields(const std::vector<StructField> &fields);
		const CodePoint &codePoint() const { return mCodePoint; }
		bool generateLLVMType();
		bool isGenerated() const;

		bool isNamedValueType() const { return true; }
		virtual Value generateOperation(Builder *builder, int opType, const Value &operand1, const Value &operand2, OperationFlags &operationFlags) const;
		void generateDestructor(Builder *builder, const Value &v);
		Value generateLoad(Builder *builder, const Value &var) const;
		virtual Value member(Builder *builder, const Value &a, const std::string &memberName) const;
		virtual ValueType *memberType(const std::string &memberName) const;
		Value field(Builder *builder, const Value &v, int fieldIndex) const;

		llvm::StructType *structType() const { return mStructType; }
	protected:
		std::string mName;
		CodePoint mCodePoint;
		llvm::StructType *mStructType;
		std::vector<StructField> mFields;
		std::map<std::string, std::vector<StructField>::const_iterator> mFieldSearch;
};

#endif // CLASSVALUETYPE_H
