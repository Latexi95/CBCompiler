#include "typepointervaluetype.h"
#include "typesymbol.h"
#include "value.h"
#include "runtime.h"
#include "builder.h"
#include "llvm.h"

TypePointerValueType::TypePointerValueType(Runtime *r, TypeSymbol *s):
	ValueType(r),
	mTypeSymbol(s){
	mType = 0;
}

QString TypePointerValueType::name() const {
	return mTypeSymbol->name();
}

ValueType::CastCost TypePointerValueType::castingCostToOtherValueType(ValueType *to) const {
	if (to == this) return ccNoCost;
	if (to == mRuntime->typePointerCommonValueType()) return ccCastToBigger;
	return ccNoCast;
}

Value TypePointerValueType::cast(Builder *builder, const Value &v) const {
	if (v.valueType() == this) return v;
	assert("Invalid cast" && 0);
	return Value();
}

llvm::Constant *TypePointerValueType::defaultValue() const {
	return llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(mType));
}

int TypePointerValueType::size() const {
	return mRuntime->dataLayout().getPointerSize();
}


ValueType::CastCost TypePointerCommonValueType::castingCostToOtherValueType(ValueType *to) const {
	if (to == this) return ccNoCost;
	return ccNoCast;
}

Value TypePointerCommonValueType::cast(Builder *builder, const Value &v) const {
	if (v.valueType() == this) {
		return v;
	}
	if (v.valueType()->isTypePointer()) {
		if (v.isConstant()) {
			return Value(const_cast<TypePointerCommonValueType*>(this), defaultValue());
		}
		return Value(const_cast<TypePointerCommonValueType*>(this), builder->bitcast(mType, v.value()));
	}
	assert("Invalid cast" && 0);
	return Value();
}


llvm::Constant *TypePointerCommonValueType::defaultValue() const {
	return llvm::ConstantPointerNull::get(static_cast<llvm::PointerType*>(mType));
}

int TypePointerCommonValueType::size() const {
	return mRuntime->dataLayout().getPointerSize();
}

