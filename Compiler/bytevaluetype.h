#ifndef BYTEVALUETYPE_H
#define BYTEVALUETYPE_H
#include "valuetype.h"
class ByteValueType : public ValueType {
	public:
		ByteValueType(Runtime *r, llvm::Module *mod);
		QString name() const {return "byte";}
		eType type() const{return Byte;}
		/** Calculates cost for casting given ValueType to this ValueType.
		  * If returned cost is over maxCastCost, cast cannot be done. */
		CastCostType castingCostToOtherValueType(ValueType *to) const;
		Value cast(Builder *builder, const Value &v) const;
		llvm::Constant *constant(quint8 i) const;
		llvm::Constant *defaultValue() const;

		bool isTypePointer() const{return false;}
		bool isNumber() const{return true;}
		int size() const { return 1; }
	private:
};

#endif // BYTEVALUETYPE_H
