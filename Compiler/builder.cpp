#include "builder.h"
#include "stringvaluetype.h"
#include "stringpool.h"
#include "intvaluetype.h"
#include "shortvaluetype.h"
#include "floatvaluetype.h"
#include "bytevaluetype.h"
#include "typepointervaluetype.h"
#include "booleanvaluetype.h"
#include "variablesymbol.h"
#include "arraysymbol.h"
#include "typesymbol.h"
#include "typevaluetype.h"
#include <QDebug>

Builder::Builder(llvm::LLVMContext &context) :
	mIRBuilder(context),
	mRuntime(0),
	mStringPool(0) {
}

void Builder::setRuntime(Runtime *r) {
	mRuntime = r;

	//double    @llvm.pow.f64(double %Val, double %Power)
	llvm::Type *params[2] = {mIRBuilder.getDoubleTy(), mIRBuilder.getDoubleTy()};
	llvm::FunctionType *funcTy = llvm::FunctionType::get(mIRBuilder.getDoubleTy(), params, false);
	mPowFF = llvm::cast<llvm::Function>(r->module()->getOrInsertFunction("llvm.pow.f64", funcTy));

	//double    @llvm.powi.f64(double %Val, i32 %power)
	params[1] = mIRBuilder.getInt32Ty();
	funcTy = llvm::FunctionType::get(mIRBuilder.getDoubleTy(), params, false);
	mPowFI = llvm::cast<llvm::Function>(r->module()->getOrInsertFunction("llvm.powi.f64", funcTy));

	assert(mPowFF && mPowFI);
}

void Builder::setStringPool(StringPool *s) {
	mStringPool = s;
}

void Builder::setInsertPoint(llvm::BasicBlock *basicBlock) {
	mIRBuilder.SetInsertPoint(basicBlock);
}

Value Builder::toInt(const Value &v) {
	if (v.valueType()->type() == ValueType::Integer) return v;
	if (v.isConstant()) {
		return Value(ConstantValue(v.constant().toInt()), mRuntime);
	}
	assert(v.value());
	switch (v.valueType()->type()) {
		case ValueType::Float: {
			llvm::Value *r = mIRBuilder.CreateFAdd(v.value(), llvm::ConstantFP::get(mIRBuilder.getFloatTy(), 0.5));
			return Value(mRuntime->intValueType(), mIRBuilder.CreateFPToSI(r,mIRBuilder.getInt32Ty()));
		}
		case ValueType::Boolean:
		case ValueType::Byte:
		case ValueType::Short: {
			llvm::Value *r = mIRBuilder.CreateCast(llvm::CastInst::ZExt, v.value(), mIRBuilder.getInt32Ty());
			return Value(mRuntime->intValueType(), r);
		}
		case ValueType::String: {
			llvm::Value *i = mRuntime->stringValueType()->stringToIntCast(&mIRBuilder, v.value());
			return Value(mRuntime->intValueType(), i);
		}
		default: break;
	}
	assert("Invalid value" && 0);
	return Value();
}

Value Builder::toFloat(const Value &v) {
	if (v.valueType()->type() == ValueType::Float) return v;
	if (v.isConstant()) {
		return Value(ConstantValue(v.constant().toFloat()), mRuntime);
	}
	switch(v.valueType()->type()) {
		case ValueType::Integer:
			return Value(mRuntime->floatValueType(),mIRBuilder.CreateSIToFP(v.value(), mIRBuilder.getFloatTy()));
		case ValueType::Boolean:
		case ValueType::Short:
		case ValueType::Byte:
			return Value(mRuntime->floatValueType(),mIRBuilder.CreateUIToFP(v.value(), mIRBuilder.getFloatTy()));
		case ValueType::String: {
			llvm::Value *val = mRuntime->stringValueType()->stringToFloatCast(&mIRBuilder, v.value());
			return Value(mRuntime->floatValueType(), val);
		}
		default: break;
	}
	assert("Invalid value" && 0);
	return Value();
}

Value Builder::toString(const Value &v) {
	if (v.valueType()->type() == ValueType::String) return v;
	if (v.isConstant()) {
		return Value(ConstantValue(v.constant().toString()), mRuntime);
	}
	switch (v.valueType()->type()) {
		case ValueType::Integer:
		case ValueType::Boolean:
		case ValueType::Short:
		case ValueType::Byte: {
			Value i = toInt(v);
			llvm::Value *val = mRuntime->stringValueType()->intToStringCast(&mIRBuilder, i.value());
			return Value(mRuntime->stringValueType(), val);
		}
		case ValueType::Float: {
			llvm::Value *val = mRuntime->stringValueType()->floatToStringCast(&mIRBuilder, v.value());
			return Value(mRuntime->stringValueType(), val);
		}
		default: break;
	}

	assert("Invalid value" && 0);
	return Value();
}

Value Builder::toShort(const Value &v) {
	if (v.valueType()->type() == ValueType::Short) return v;
	if (v.isConstant()) {
		return Value(ConstantValue(v.constant().toShort()), mRuntime);
	}
	assert(v.value());
	switch (v.valueType()->type()) {
		case ValueType::Float: {
			llvm::Value *r = mIRBuilder.CreateAdd(v.value(), llvm::ConstantFP::get(mIRBuilder.getFloatTy(), 0.5));
			return Value(mRuntime->intValueType(), mIRBuilder.CreateFPToSI(r,mIRBuilder.getInt16Ty()));
		}
		case ValueType::Boolean:
		case ValueType::Byte: {
			llvm::Value *r = mIRBuilder.CreateCast(llvm::CastInst::ZExt, v.value(), mIRBuilder.getInt16Ty());
			return Value(mRuntime->intValueType(), r);
		}
		case ValueType::Integer: {
			llvm::Value *r = mIRBuilder.CreateCast(llvm::CastInst::Trunc, v.value(), mIRBuilder.getInt16Ty());
			return Value(mRuntime->intValueType(), r);
		}

		case ValueType::String: {
			llvm::Value *i = mRuntime->stringValueType()->stringToIntCast(&mIRBuilder, v.value());
			return toShort(Value(mRuntime->intValueType(), i));
		}
		default: break;
	}
	assert("Invalid value" && 0);
	return Value();
}

Value Builder::toByte(const Value &v) {
	if (v.valueType()->type() == ValueType::Byte) return v;
	if (v.isConstant()) {
		return Value(ConstantValue(v.constant().toShort()), mRuntime);
	}
	assert(v.value());
	switch (v.valueType()->type()) {
		case ValueType::Float: {
			llvm::Value *r = mIRBuilder.CreateAdd(v.value(), llvm::ConstantFP::get(mIRBuilder.getFloatTy(), 0.5));
			return Value(mRuntime->intValueType(), mIRBuilder.CreateFPToSI(r,mIRBuilder.getInt16Ty()));
		}
		case ValueType::Boolean: {
			llvm::Value *r = mIRBuilder.CreateCast(llvm::CastInst::ZExt, v.value(), mIRBuilder.getInt16Ty());
			return Value(mRuntime->intValueType(), r);
		}
		case ValueType::Integer:
		case ValueType::Short: {
			llvm::Value *r = mIRBuilder.CreateCast(llvm::CastInst::Trunc, v.value(), mIRBuilder.getInt16Ty());
			return Value(mRuntime->intValueType(), r);
		}

		case ValueType::String: {
			llvm::Value *i = mRuntime->stringValueType()->stringToIntCast(&mIRBuilder, v.value());
			return toShort(Value(mRuntime->intValueType(), i));
		}
		default: break;
	}
	assert("Invalid value" && 0);
	return Value();
}

Value Builder::toBoolean(const Value &v) {
	if (v.valueType()->type() == ValueType::Boolean) {
		return v;
	}
	if (v.isConstant()) {
		return Value(ConstantValue(v.constant().toBool()), mRuntime);
	}
	switch(v.valueType()->type()) {
		case ValueType::Integer:
			return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpNE(v.value(), mIRBuilder.getInt32(0)));
		case ValueType::Short:
			return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpNE(v.value(), mIRBuilder.getInt16(0)));
		case ValueType::Byte:
			return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpNE(v.value(), mIRBuilder.getInt8(0)));
		case ValueType::Float:
			return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpONE(v.value(), llvm::ConstantFP::get(mIRBuilder.getFloatTy(), 0.0)));
		case ValueType::String: {
			llvm::Value *val = mIRBuilder.CreateIsNotNull(v.value());

			return Value(mRuntime->booleanValueType(), val);
		}
		default: break;
	}
	assert("Invalid value" && 0);
	return Value();
}

Value Builder::toValueType(ValueType *to, const Value &v) {
	return to->cast(this, v);
}

llvm::Value *Builder::llvmValue(const Value &v) {
	if (v.isConstant()) {
		return llvmValue(v.constant());
	}

	assert(v.value());
	return v.value();
}

llvm::Value *Builder::llvmValue(int i) {
	return mIRBuilder.getInt32(i);
}

llvm::Value *Builder::llvmValue(uint16_t i) {
	return mIRBuilder.getInt16(i);
}

llvm::Value *Builder::llvmValue(uint8_t i) {
	return mIRBuilder.getInt8(i);
}

llvm::Value *Builder::llvmValue(float i) {
	return llvm::ConstantFP::get(mIRBuilder.getFloatTy(), (double)i);
}

llvm::Value *Builder::llvmValue(const QString &s) {
	if (s.isEmpty()) {
		return llvm::ConstantPointerNull::get((llvm::PointerType*)mRuntime->stringValueType()->llvmType());
	}
	return mStringPool->globalString(this, s).value();
}

llvm::Value *Builder::llvmValue(bool t) {
	return mIRBuilder.getInt1(t);
}

llvm::Value *Builder::llvmValue(const ConstantValue &v) {
	switch(v.type()) {
		case ValueType::Integer:
			return llvmValue(v.toInt());
		case ValueType::Float:
			return llvmValue(v.toFloat());
		case ValueType::Short:
			return llvmValue(v.toShort());
		case ValueType::Byte:
			return llvmValue(v.toByte());
		case ValueType::Boolean:
			return llvmValue(v.toBool());
		case ValueType::String: {
			return llvmValue(v.toString());
		case ValueType::TypePointerCommon:
			return mRuntime->typePointerCommonValueType()->defaultValue();
		}
		default: break;
	}
	assert("Invalid constant value" && 0);
	return 0;
}

llvm::Value *Builder::bitcast(llvm::Type *type, llvm::Value *val) {
	return mIRBuilder.CreateBitCast(val, type);
}


Value Builder::call(Function *func, QList<Value> &params) {
	Function::ParamList paramTypes = func->paramTypes();
	assert(func->requiredParams() >= params.size() && params.size() <= paramTypes.size());
	Function::ParamList::ConstIterator pi = paramTypes.begin();
	for (QList<Value>::Iterator i = params.begin(); i != params.end(); ++i) {
		//Cast to the right value type
		*i = (*pi)->cast(this, *i);
		//string destruction hack
		i->toLLVMValue(this);
		pi++;
	}
	Value ret = func->call(this, params);
	for (QList<Value>::ConstIterator i = params.begin(); i != params.end(); ++i) {
		destruct(*i);
	}
	return ret;
}

void Builder::branch(llvm::BasicBlock *dest) {
	mIRBuilder.CreateBr(dest);
}


void Builder::branch(const Value &cond, llvm::BasicBlock *ifTrue, llvm::BasicBlock *ifFalse) {
	mIRBuilder.CreateCondBr(llvmValue(toBoolean(cond)), ifTrue, ifFalse);
}

void Builder::returnValue(ValueType *retType, const Value &v) {
	mIRBuilder.CreateRet(llvmValue(retType->cast(this, v)));
}

void Builder::construct(VariableSymbol *var) {
	llvm::Value *allocaInst = mIRBuilder.CreateAlloca(var->valueType()->llvmType());
	var->setAlloca(allocaInst);
	mIRBuilder.CreateStore(var->valueType()->defaultValue(), allocaInst);
}

void Builder::store(llvm::Value *ptr, llvm::Value *val) {
	assert(ptr->getType() == val->getType()->getPointerTo());
	mIRBuilder.CreateStore(val, ptr);
}

void Builder::store(llvm::Value *ptr, const Value &v) {
	assert(ptr->getType() == v.valueType()->llvmType()->getPointerTo());
	if (v.valueType()->type() == ValueType::String) {
		mRuntime->stringValueType()->assignString(&mIRBuilder, ptr, llvmValue(v));
	}
	else {
		mIRBuilder.CreateStore(llvmValue(v), ptr);
	}
}

void Builder::store(VariableSymbol *var, const Value &v) {
	store(var, llvmValue(var->valueType()->cast(this, v)));
}

void Builder::store(VariableSymbol *var, llvm::Value *val) {
	assert(var->valueType()->llvmType() == val->getType());
	if (var->valueType()->type() == ValueType::String) {
		mRuntime->stringValueType()->assignString(&mIRBuilder, var->alloca_(), val);
	}
	else {
		store(var->alloca_(), val);
	}
}

Value Builder::load(const VariableSymbol *var) {
	llvm::Value *val = mIRBuilder.CreateLoad(var->alloca_(), false);
	if (var->valueType()->type() == ValueType::String) {
		mRuntime->stringValueType()->refString(&mIRBuilder, val);
	}
	return Value(var->valueType(), val);
}

void Builder::destruct(VariableSymbol *var) {
	if (var->valueType()->type() == ValueType::String) {
		llvm::Value *str = mIRBuilder.CreateLoad(var->alloca_(), false);
		mRuntime->stringValueType()->destructString(&mIRBuilder, str);
	}
}

void Builder::destruct(const Value &a) {
	if (!a.isValid()) return;
	if (a.isConstant() || a.valueType()->type() != ValueType::String) return;
	assert(a.value());
	mRuntime->stringValueType()->destructString(&mIRBuilder, a.value());
}

Value Builder::nullTypePointer() {
	return Value(ConstantValue(ValueType::TypePointerCommon), mRuntime);
}

void Builder::initilizeArray(ArraySymbol *array, const QList<Value> &dimSizes) {
	assert(array->dimensions() == dimSizes.size());

	llvm::Value *memSize = calculateArrayMemorySize(array, dimSizes);
	llvm::Value *mem = mIRBuilder.CreateBitCast(allocate(memSize), array->valueType()->llvmType()->getPointerTo());
	memSet(mem, memSize, llvmValue(uint8_t(0)));
	mIRBuilder.CreateStore(mem, array->globalArrayData());

	fillArrayIndexMultiplierArray(array, dimSizes);

}

void Builder::store(ArraySymbol *array, const QList<Value> &dims, const Value &val) {
	store(arrayElementPointer(array, dims), array->valueType()->cast(this, val));
}

void Builder::store(VariableSymbol *typePtrVar, const QString &fieldName, const Value &v) {
	assert(typePtrVar->valueType()->type() == ValueType::TypePointer);

	TypePointerValueType *typePointerValTy = static_cast<TypePointerValueType*>(typePtrVar->valueType());
	TypeSymbol *typeSymbol = typePointerValTy->typeSymbol();

	ValueType *fieldValueType = typeSymbol->field(fieldName).valueType();
	llvm::Value *fieldPointer = typePointerFieldPointer(typePtrVar, fieldName);
	store(fieldPointer, fieldValueType->cast(this, v));
}

Value Builder::load(ArraySymbol *array, const QList<Value> &dims) {
	return Value(array->valueType(), mIRBuilder.CreateLoad(arrayElementPointer(array, dims)));
}

Value Builder::load(VariableSymbol *typePtrVar, const QString &fieldName) {
	assert(typePtrVar->valueType()->type() == ValueType::TypePointer);

	TypePointerValueType *typePointerValTy = static_cast<TypePointerValueType*>(typePtrVar->valueType());
	TypeSymbol *typeSymbol = typePointerValTy->typeSymbol();

	ValueType *fieldValueType = typeSymbol->field(fieldName).valueType();
	llvm::Value *fieldPointer = typePointerFieldPointer(typePtrVar, fieldName);
	llvm::Value *val = mIRBuilder.CreateLoad(fieldPointer);
	if (fieldValueType->type() == ValueType::String) {
		mRuntime->stringValueType()->refString(&mIRBuilder, val);
	}
	return Value(fieldValueType, val);
}

llvm::Value *Builder::calculateArrayElementCount(const QList<Value> &dimSizes) {

	QList<Value>::ConstIterator i = dimSizes.begin();
	llvm::Value *result = llvmValue(toInt(*i));
	for (; i != dimSizes.end(); i++) {
		result = mIRBuilder.CreateMul(result, llvmValue(toInt(*i)));
	}
	return result;
}

llvm::Value *Builder::calculateArrayMemorySize(ArraySymbol *array, const QList<Value> &dimSizes) {
	llvm::Value *elements = calculateArrayElementCount(dimSizes);
	int sizeOfElement = array->valueType()->size();
	return mIRBuilder.CreateMul(elements, llvmValue(sizeOfElement));
}

llvm::Value *Builder::arrayElementPointer(ArraySymbol *array, const QList<Value> &index) {
	assert(array->dimensions() == index.size());
	if (array->dimensions() == 1) {
		return mIRBuilder.CreateGEP(mIRBuilder.CreateLoad(array->globalArrayData()), llvmValue(toInt(index.first())));
	}
	else { // array->dimensions() > 1
		QList<Value>::ConstIterator i = index.begin();
		llvm::Value *arrIndex =  mIRBuilder.CreateMul(llvmValue(toInt(*i)), arrayIndexMultiplier(array, 0));
		int multIndex = 1;
		i++;
		for (QList<Value>::ConstIterator end = --index.end(); i != end; ++i) {
			arrIndex = mIRBuilder.CreateAdd(arrIndex, mIRBuilder.CreateMul(llvmValue(toInt(*i)), arrayIndexMultiplier(array, multIndex)));
			multIndex++;
		}
		arrIndex = mIRBuilder.CreateAdd(arrIndex, llvmValue(toInt(*i)));
		return mIRBuilder.CreateGEP(mIRBuilder.CreateLoad(array->globalArrayData()), arrIndex);
	}
}

llvm::Value *Builder::arrayIndexMultiplier(ArraySymbol *array, int index) {
	assert(index >= 0 && index < array->dimensions() - 1);
	llvm::Value *gepParams[2] = { llvmValue(0), llvmValue(index) };
	return mIRBuilder.CreateLoad(mIRBuilder.CreateGEP(array->globalIndexMultiplierArray(), gepParams));
}

void Builder::fillArrayIndexMultiplierArray(ArraySymbol *array, const QList<Value> &dimSizes) {
	assert(array->dimensions() == dimSizes.size());

	if (array->dimensions() > 1) {
		QList<Value>::ConstIterator i = --dimSizes.end();
		llvm::Value *multiplier = llvmValue(toInt(*i));
		int arrIndex = array->dimensions() - 2;
		while(i != dimSizes.begin()) {
			llvm::Value *gepParams[2] = { llvmValue(0), llvmValue(arrIndex) };
			llvm::Value *pointerToArrayElement = mIRBuilder.CreateGEP(array->globalIndexMultiplierArray(), gepParams);
			mIRBuilder.CreateStore(multiplier, pointerToArrayElement);

			--i;
			if (i != dimSizes.begin()) {
				multiplier = mIRBuilder.CreateMul(multiplier, llvmValue(toInt(*i)));
				--arrIndex;
			}
		}
	}

}

llvm::Value *Builder::typePointerFieldPointer(VariableSymbol *typePtrVar, const QString &fieldName) {
	assert(typePtrVar->valueType()->type() == ValueType::TypePointer);

	TypePointerValueType *typePointerValTy = static_cast<TypePointerValueType*>(typePtrVar->valueType());
	TypeSymbol *typeSymbol = typePointerValTy->typeSymbol();
	int fieldIndex = typeSymbol->fieldIndex(fieldName);

	return mIRBuilder.CreateStructGEP(mIRBuilder.CreateLoad(typePtrVar->alloca_()), fieldIndex);
}

Value Builder::newTypeMember(TypeSymbol *type) {
	llvm::Value *typePtr = mIRBuilder.CreateCall(mRuntime->typeValueType()->newFunction(), type->globalTypeVariable());
	return Value(type->typePointerValueType(), bitcast(type->typePointerValueType()->llvmType(), typePtr));
}

Value Builder::firstTypeMember(TypeSymbol *type) {
	llvm::Value *typePtr = mIRBuilder.CreateCall(mRuntime->typeValueType()->firstFunction(), type->globalTypeVariable());
	return Value(type->typePointerValueType(), bitcast(type->typePointerValueType()->llvmType(), typePtr));
}

Value Builder::lastTypeMember(TypeSymbol *type) {
	llvm::Value *typePtr = mIRBuilder.CreateCall(mRuntime->typeValueType()->lastFunction(), type->globalTypeVariable());
	return Value(type->typePointerValueType(), bitcast(type->typePointerValueType()->llvmType(), typePtr));
}

Value Builder::afterTypeMember(const Value &ptr) {
	assert(ptr.valueType()->isTypePointer());
	llvm::Value *param = bitcast(mRuntime->typePointerCommonValueType()->llvmType(), llvmValue(ptr));
	llvm::Value *typePtr = mIRBuilder.CreateCall(mRuntime->typeValueType()->afterFunction(), param);
	return Value(ptr.valueType(), bitcast(ptr.valueType()->llvmType(), typePtr));
}

Value Builder::beforeTypeMember(const Value &ptr) {
	assert(ptr.valueType()->isTypePointer());
	llvm::Value *param = bitcast(mRuntime->typePointerCommonValueType()->llvmType(), llvmValue(ptr));
	llvm::Value *typePtr = mIRBuilder.CreateCall(mRuntime->typeValueType()->beforeFunction(), param);
	return Value(ptr.valueType(), bitcast(ptr.valueType()->llvmType(), typePtr));
}

Value Builder::typePointerNotNull(const Value &ptr) {
	assert(ptr.valueType()->isTypePointer());
	return Value(mRuntime->booleanValueType(), mIRBuilder.CreateIsNotNull(llvmValue(ptr)));
}

llvm::GlobalVariable *Builder::createGlobalVariable(ValueType *type, bool isConstant, llvm::GlobalValue::LinkageTypes linkage, llvm::Constant *initializer, const llvm::Twine &name) {
	return createGlobalVariable(type->llvmType(), isConstant, linkage, initializer, name);
}

llvm::GlobalVariable *Builder::createGlobalVariable(llvm::Type *type, bool isConstant, llvm::GlobalValue::LinkageTypes linkage, llvm::Constant *initializer, const llvm::Twine &name) {
	return new llvm::GlobalVariable(*mRuntime->module(),type, isConstant, linkage, initializer, name);
}

llvm::LLVMContext &Builder::context() {
	return mRuntime->module()->getContext();
}

Value Builder::not_(const Value &a) {
	if (a.isConstant()) {
		return Value(ConstantValue::not_(a.constant()), mRuntime);
	}

	return Value(mRuntime->booleanValueType(), mIRBuilder.CreateNot(llvmValue(toBoolean(a))));
}

Value Builder::minus(const Value &a) {
	if (a.isConstant()) {
		return Value(ConstantValue::minus(a.constant()), mRuntime);
	}

	if (a.valueType()->type() == ValueType::Boolean) {
		return Value(mRuntime->intValueType(), mIRBuilder.CreateNeg(llvmValue(toInt(a))));
	}
	if (a.valueType()->type() == ValueType::Float) {
		return Value(a.valueType(), mIRBuilder.CreateFNeg(llvmValue(a)));
	}
	return Value(a.valueType(), mIRBuilder.CreateNeg(llvmValue(a)));
}

Value Builder::plus(const Value &a) {
	assert(0); return Value();
}

Value Builder::add(const Value &a, const Value &b) {
	if (a.isConstant() && b.isConstant()) {
		return Value(ConstantValue::add(a.constant(), b.constant()), mRuntime);
	}
	llvm::Value *result;
	switch(a.valueType()->type()) {
		case ValueType::Boolean:
		case ValueType::Integer:
			switch(b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Boolean:
					result = mIRBuilder.CreateAdd(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFAdd(llvmValue(toFloat(a)), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Short:
				case ValueType::Byte:
					result = mIRBuilder.CreateAdd(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				case ValueType::String: {
					Value as = toString(a);
					result = mRuntime->stringValueType()->stringAddition(&mIRBuilder, llvmValue(as), llvmValue(b));
					destruct(as);
					return Value(mRuntime->stringValueType(), result);
				}
				default: break;
			}
			break;
		case ValueType::Float:
			switch(b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Short:
				case ValueType::Byte:
				case ValueType::Boolean:
					result = mIRBuilder.CreateFAdd(llvmValue(a), llvmValue(toFloat(b)));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFAdd(llvmValue(a), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::String: {
					Value as = toString(a);
					result = mRuntime->stringValueType()->stringAddition(&mIRBuilder, llvmValue(as), llvmValue(b));
					destruct(as);
					return Value(mRuntime->stringValueType(), result);
				}
				default: break;
			}
			break;
		case ValueType::Short:
			switch(b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Boolean:
					result = mIRBuilder.CreateAdd(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFAdd(llvmValue(toFloat(a)), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Short:
				case ValueType::Byte:
					result = mIRBuilder.CreateAdd(llvmValue(a), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				case ValueType::String: {
					Value as = toString(a);
					result = mRuntime->stringValueType()->stringAddition(&mIRBuilder, llvmValue(as), llvmValue(b));
					destruct(as);
					return Value(mRuntime->stringValueType(), result);
				}
				default: break;
			}
			break;
		case ValueType::Byte:
			switch (b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Boolean:
					result = mIRBuilder.CreateAdd(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFAdd(llvmValue(toFloat(a)), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Short:
					result = mIRBuilder.CreateAdd(llvmValue(toShort(a)), llvmValue(b));
					return Value(mRuntime->intValueType(), result);
				case ValueType::Byte:
					result = mIRBuilder.CreateAdd(llvmValue(a), llvmValue(b));
					return Value(mRuntime->intValueType(), result);
				case ValueType::String: {
					Value as = toString(a);
					result = mRuntime->stringValueType()->stringAddition(&mIRBuilder, llvmValue(as), llvmValue(b));
					destruct(as);
					return Value(mRuntime->stringValueType(), result);
				}
				default: break;
			}
			break;
		case ValueType::String:
			switch (b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Boolean:
				case ValueType::Float:
				case ValueType::Short:
				case ValueType::Byte: {
					Value as = toString(b);
					result = mRuntime->stringValueType()->stringAddition(&mIRBuilder, llvmValue(a), llvmValue(as));
					destruct(as);
					return Value(mRuntime->stringValueType(), result);
				}
				case ValueType::String: {
					result = mRuntime->stringValueType()->stringAddition(&mIRBuilder, llvmValue(a), llvmValue(b));
					destruct(a);
					return Value(mRuntime->stringValueType(), result);
				}
				default: break;
			}
			break;
		default: break;
	}
	assert("Invalid value" && 0);
	return Value();
}

Value Builder::subtract(const Value &a, const Value &b) {
	if (a.isConstant() && b.isConstant()) {
		return Value(ConstantValue::subtract(a.constant(), b.constant()), mRuntime);
	}
	llvm::Value *result;
	switch(a.valueType()->type()) {
		case ValueType::Boolean:
		case ValueType::Integer:
			switch(b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Boolean:
					result = mIRBuilder.CreateSub(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFSub(llvmValue(toFloat(a)), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Short:
				case ValueType::Byte:
					result = mIRBuilder.CreateSub(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				default: break;
			}
			break;
		case ValueType::Float:
			switch(b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Short:
				case ValueType::Byte:
				case ValueType::Boolean:
					result = mIRBuilder.CreateFSub(llvmValue(a), llvmValue(toFloat(b)));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFSub(llvmValue(a), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				default: break;
			}
			break;
		case ValueType::Short:
			switch(b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Boolean:
					result = mIRBuilder.CreateSub(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFSub(llvmValue(toFloat(a)), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Short:
				case ValueType::Byte:
					result = mIRBuilder.CreateSub(llvmValue(a), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				default: break;
			}
			break;
		case ValueType::Byte:
			switch (b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Boolean:
					result = mIRBuilder.CreateSub(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFSub(llvmValue(toFloat(a)), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Short:
					result = mIRBuilder.CreateSub(llvmValue(toShort(a)), llvmValue(b));
					return Value(mRuntime->intValueType(), result);
				case ValueType::Byte:
					result = mIRBuilder.CreateSub(llvmValue(a), llvmValue(b));
					return Value(mRuntime->intValueType(), result);
				default: break;
			}
			break;
		default: break;
	}
	assert("Invalid value" && 0);
	return Value();
}

Value Builder::multiply(const Value &a, const Value &b) {
	if (a.isConstant() && b.isConstant()) {
		return Value(ConstantValue::multiply(a.constant(), b.constant()), mRuntime);
	}
	llvm::Value *result;
	switch(a.valueType()->type()) {
		case ValueType::Boolean:
		case ValueType::Integer:
			switch(b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Boolean:
					result = mIRBuilder.CreateMul(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFMul(llvmValue(toFloat(a)), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Short:
				case ValueType::Byte:
					result = mIRBuilder.CreateMul(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				default: break;
			}
			break;
		case ValueType::Float:
			switch(b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Short:
				case ValueType::Byte:
				case ValueType::Boolean:
					result = mIRBuilder.CreateFMul(llvmValue(a), llvmValue(toFloat(b)));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFMul(llvmValue(a), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				default: break;
			}
			break;
		case ValueType::Short:
			switch(b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Boolean:
					result = mIRBuilder.CreateMul(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFMul(llvmValue(toFloat(a)), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Short:
				case ValueType::Byte:
					result = mIRBuilder.CreateMul(llvmValue(a), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				default: break;
			}
			break;
		case ValueType::Byte:
			switch (b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Boolean:
					result = mIRBuilder.CreateMul(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFMul(llvmValue(toFloat(a)), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Short:
					result = mIRBuilder.CreateMul(llvmValue(toShort(a)), llvmValue(b));
					return Value(mRuntime->intValueType(), result);
				case ValueType::Byte:
					result = mIRBuilder.CreateMul(llvmValue(a), llvmValue(b));
					return Value(mRuntime->intValueType(), result);
				default: break;
			}
			break;
		default: break;
	}
	assert("Invalid value" && 0);
	return Value();
}

Value Builder::divide(const Value &a, const Value &b) {
	if (a.isConstant() && b.isConstant()) {
		return Value(ConstantValue::divide(a.constant(), b.constant()), mRuntime);
	}
	llvm::Value *result;
	switch(a.valueType()->type()) {
		case ValueType::Boolean:
		case ValueType::Integer:
			switch(b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Boolean:
					result = mIRBuilder.CreateSDiv(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFDiv(llvmValue(toFloat(a)), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Short:
				case ValueType::Byte:
					result = mIRBuilder.CreateUDiv(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				default: break;
			}
			break;
		case ValueType::Float:
			switch(b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Short:
				case ValueType::Byte:
				case ValueType::Boolean:
					result = mIRBuilder.CreateFDiv(llvmValue(a), llvmValue(toFloat(b)));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFDiv(llvmValue(a), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				default: break;
			}
			break;
		case ValueType::Short:
			switch(b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Boolean:
					result = mIRBuilder.CreateSDiv(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFDiv(llvmValue(toFloat(a)), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Short:
				case ValueType::Byte:
					result = mIRBuilder.CreateUDiv(llvmValue(a), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				default: break;
			}
			break;
		case ValueType::Byte:
			switch (b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Boolean:
					result = mIRBuilder.CreateSDiv(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFDiv(llvmValue(toFloat(a)), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Short:
				case ValueType::Byte:
					result = mIRBuilder.CreateUDiv(llvmValue(toShort(a)), llvmValue(b));
					return Value(mRuntime->intValueType(), result);
				default: break;
			}
			break;
		default: break;
	}
	assert("Invalid value" && 0);
	return Value();
}

Value Builder::mod(const Value &a, const Value &b){
	if (a.isConstant() && b.isConstant()) {
		return Value(ConstantValue::mod(a.constant(), b.constant()), mRuntime);
	}
	llvm::Value *result;
	switch(a.valueType()->type()) {
		case ValueType::Boolean:
		case ValueType::Integer:
			switch(b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Boolean:
					result = mIRBuilder.CreateSRem(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFRem(llvmValue(toFloat(a)), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Short:
				case ValueType::Byte:
					result = mIRBuilder.CreateURem(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				default: break;
			}
			break;
		case ValueType::Float:
			switch(b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Short:
				case ValueType::Byte:
				case ValueType::Boolean:
					result = mIRBuilder.CreateFRem(llvmValue(a), llvmValue(toFloat(b)));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFRem(llvmValue(a), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				default: break;
			}
			break;
		case ValueType::Short:
			switch(b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Boolean:
					result = mIRBuilder.CreateSRem(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFRem(llvmValue(toFloat(a)), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Short:
				case ValueType::Byte:
					result = mIRBuilder.CreateURem(llvmValue(a), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				default: break;
			}
			break;
		case ValueType::Byte:
			switch (b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Boolean:
					result = mIRBuilder.CreateSRem(llvmValue(toInt(a)), llvmValue(toInt(b)));
					return Value(mRuntime->intValueType(), result);
				case ValueType::Float:
					result = mIRBuilder.CreateFRem(llvmValue(toFloat(a)), llvmValue(b));
					return Value(mRuntime->floatValueType(), result);
				case ValueType::Short:
				case ValueType::Byte:
					result = mIRBuilder.CreateURem(llvmValue(toShort(a)), llvmValue(b));
					return Value(mRuntime->intValueType(), result);
				default: break;
			}
			break;
		default: break;
	}
	assert("Invalid value" && 0);
	return Value();
}

Value Builder::shl(const Value &a, const Value &b){
	if (a.isConstant() && b.isConstant()) {
		return Value(ConstantValue::shl(a.constant(),b.constant()), mRuntime);
	}
	//No strings or floats
	assert(a.valueType()->type() != ValueType::Float && a.valueType()->type() != ValueType::String);
	assert(b.valueType()->type() != ValueType::Float && b.valueType()->type() != ValueType::String);

	if (a.valueType()->type() == ValueType::Integer || b.valueType()->type() == ValueType::Integer || a.valueType()->type() == ValueType::Boolean || b.valueType()->type() == ValueType::Boolean) {
		return Value(mRuntime->intValueType(), mIRBuilder.CreateShl(llvmValue(toInt(a)), llvmValue(toInt(b))));
	}
	if (a.valueType()->type() == ValueType::Short || b.valueType()->type() == ValueType::Short) {
		return Value(mRuntime->shortValueType(), mIRBuilder.CreateShl(llvmValue(toShort(a)), llvmValue(toShort(b))));
	}

	return Value(mRuntime->byteValueType(), mIRBuilder.CreateShl(llvmValue(toByte(a)), llvmValue(toByte(b))));
}

Value Builder::shr(const Value &a, const Value &b){
	if (a.isConstant() && b.isConstant()) {
		return Value(ConstantValue::shl(a.constant(),b.constant()), mRuntime);
	}
	//No strings or floats
	assert(a.valueType()->type() != ValueType::Float && a.valueType()->type() != ValueType::String);
	assert(b.valueType()->type() != ValueType::Float && b.valueType()->type() != ValueType::String);

	if (a.valueType()->type() == ValueType::Integer || b.valueType()->type() == ValueType::Integer || a.valueType()->type() == ValueType::Boolean || b.valueType()->type() == ValueType::Boolean) {
		return Value(mRuntime->intValueType(), mIRBuilder.CreateLShr(llvmValue(toInt(a)), llvmValue(toInt(b))));
	}
	if (a.valueType()->type() == ValueType::Short || b.valueType()->type() == ValueType::Short) {
		return Value(mRuntime->shortValueType(), mIRBuilder.CreateLShr(llvmValue(toShort(a)), llvmValue(toShort(b))));
	}

	return Value(mRuntime->byteValueType(), mIRBuilder.CreateLShr(llvmValue(toByte(a)), llvmValue(toByte(b))));
}

Value Builder::sar(const Value &a, const Value &b){
	if (a.isConstant() && b.isConstant()) {
		return Value(ConstantValue::shl(a.constant(),b.constant()), mRuntime);
	}
	//No strings or floats
	assert(a.valueType()->type() != ValueType::Float && a.valueType()->type() != ValueType::String);
	assert(b.valueType()->type() != ValueType::Float && b.valueType()->type() != ValueType::String);

	if (a.valueType()->type() == ValueType::Integer || b.valueType()->type() == ValueType::Integer || a.valueType()->type() == ValueType::Boolean || b.valueType()->type() == ValueType::Boolean) {
		return Value(mRuntime->intValueType(), mIRBuilder.CreateAShr(llvmValue(toInt(a)), llvmValue(toInt(b))));
	}
	if (a.valueType()->type() == ValueType::Short || b.valueType()->type() == ValueType::Short) {
		return Value(mRuntime->shortValueType(), mIRBuilder.CreateAShr(llvmValue(toShort(a)), llvmValue(toShort(b))));
	}

	return Value(mRuntime->byteValueType(), mIRBuilder.CreateAShr(llvmValue(toByte(a)), llvmValue(toByte(b))));
}

Value Builder::and_(const Value &a, const Value &b) {
	if (a.isConstant() && b.isConstant()) {
		return Value(ConstantValue::and_(a.constant(),b.constant()), mRuntime);
	}
	return Value(mRuntime->booleanValueType(), mIRBuilder.CreateAnd(llvmValue(toBoolean(a)), llvmValue(toBoolean(b))));
}

Value Builder::or_(const Value &a, const Value &b) {
	if (a.isConstant() && b.isConstant()) {
		return Value(ConstantValue::or_(a.constant(),b.constant()), mRuntime);
	}
	return Value(mRuntime->booleanValueType(), mIRBuilder.CreateOr(llvmValue(toBoolean(a)), llvmValue(toBoolean(b))));
}

Value Builder::xor_(const Value &a, const Value &b) {
	if (a.isConstant() && b.isConstant()) {
		return Value(ConstantValue::xor_(a.constant(),b.constant()), mRuntime);
	}
	return Value(mRuntime->booleanValueType(), mIRBuilder.CreateXor(llvmValue(toBoolean(a)), llvmValue(toBoolean(b))));
}

Value Builder::power(const Value &a, const Value &b) {
	if (a.isConstant() && b.isConstant()) {
		return Value(ConstantValue::power(a.constant(), b.constant()), mRuntime);
	}
	if (a.valueType()->type() == ValueType::Float || b.valueType()->type() == ValueType::Float) {
		llvm::Value *af = llvmValue(toFloat(a));
		llvm::Value *bf = llvmValue(toFloat(b));
		llvm::Value *ad = mIRBuilder.CreateFPExt(af, mIRBuilder.getDoubleTy());
		llvm::Value *bd = mIRBuilder.CreateFPExt(bf, mIRBuilder.getDoubleTy());
		llvm::Value *retD = mIRBuilder.CreateCall2(mPowFF, ad, bd);
		return Value(mRuntime->floatValueType(), mIRBuilder.CreateFPTrunc(retD, mIRBuilder.getFloatTy()));
	}
	llvm::Value *af = llvmValue(toFloat(a));
	llvm::Value *ad = mIRBuilder.CreateFPExt(af, mIRBuilder.getDoubleTy());
	llvm::Value *retD = mIRBuilder.CreateCall2(mPowFI, ad, llvmValue(toInt(b)));
	llvm::Value *greaterCond = mIRBuilder.CreateFCmpOGT(retD, llvm::ConstantFP::get( mIRBuilder.getDoubleTy(), (double)INT_MAX));
	llvm::Value *lessCond = mIRBuilder.CreateFCmpOLT(retD, llvm::ConstantFP::get( mIRBuilder.getDoubleTy(), (double)-INT_MAX));
	llvm::Value *cond = mIRBuilder.CreateOr(greaterCond, lessCond);
	llvm::Value *ret = mIRBuilder.CreateSelect(cond, llvmValue(-INT_MAX), mIRBuilder.CreateFPToSI(retD, mIRBuilder.getInt32Ty()));
	return Value(mRuntime->intValueType(), ret);
}

Value Builder::less(const Value &a, const Value &b) {
	if (a.isConstant() && b.isConstant()) {
		return Value(ConstantValue::lessEqual(a.constant(), b.constant()), mRuntime);
	}
	if (a.valueType()->type() == ValueType::String) {
		//TODO: finish this
		assert(0);
		return Value();
	}
	if (b.valueType()->type() == ValueType::String) {
		//TODO: finish this
		assert(0);
		return Value();
	}
	switch (a.valueType()->type()) {
		case ValueType::Boolean:
		case ValueType::Integer:
			switch (b.valueType()->type()) {
				case ValueType::Boolean:
				case ValueType::Integer:
				case ValueType::Short:
				case ValueType::Byte:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpSLT(llvmValue(toInt(a)), llvmValue(toInt(b))));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOLT(llvmValue(toFloat(a)), llvmValue(toFloat(b))));
				default: break;
			}
			break;
		case ValueType::Short:
			switch (b.valueType()->type()) {
				case ValueType::Boolean:
				case ValueType::Integer:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpSLT(llvmValue(toInt(a)), llvmValue(toInt(b))));
				case ValueType::Short:
				case ValueType::Byte:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpULT(llvmValue(toShort(a)), llvmValue(toShort(b))));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOLT(llvmValue(toFloat(a)), llvmValue(toFloat(b))));
				default: break;
			}
			break;
		case ValueType::Byte:
			switch (b.valueType()->type()) {
				case ValueType::Boolean:
				case ValueType::Integer:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpSLT(llvmValue(toInt(a)), llvmValue(toInt(b))));
				case ValueType::Short:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpULT(llvmValue(toShort(a)), llvmValue(toShort(b))));
				case ValueType::Byte:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpULT(llvmValue(a), llvmValue(b)));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOLT(llvmValue(toFloat(a)), llvmValue(toFloat(b))));
				default: break;
			}
			break;
		case ValueType::Float:
			return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOLT(llvmValue(toFloat(a)), llvmValue(toFloat(b))));
		default: break;
	}

	assert("Invalid value" && 0);
	return Value();
}

Value Builder::lessEqual(const Value &a, const Value &b) {
	if (a.isConstant() && b.isConstant()) {
		return Value(ConstantValue::lessEqual(a.constant(), b.constant()), mRuntime);
	}
	if (a.valueType()->type() == ValueType::String) {
		//TODO: finish this
		assert(0);
		return Value();
	}
	if (b.valueType()->type() == ValueType::String) {
		//TODO: finish this
		assert(0);
		return Value();
	}
	switch (a.valueType()->type()) {
		case ValueType::Boolean:
		case ValueType::Integer:
			switch (b.valueType()->type()) {
				case ValueType::Boolean:
				case ValueType::Integer:
				case ValueType::Short:
				case ValueType::Byte:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpSLE(llvmValue(toInt(a)), llvmValue(toInt(b))));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOLE(llvmValue(toFloat(a)), llvmValue(toFloat(b))));
				default: break;
			}
			break;
		case ValueType::Short:
			switch (b.valueType()->type()) {
				case ValueType::Boolean:
				case ValueType::Integer:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpSLE(llvmValue(toInt(a)), llvmValue(toInt(b))));
				case ValueType::Short:
				case ValueType::Byte:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpULE(llvmValue(toShort(a)), llvmValue(toShort(b))));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOLE(llvmValue(toFloat(a)), llvmValue(toFloat(b))));
				default: break;
			}
			break;
		case ValueType::Byte:
			switch (b.valueType()->type()) {
				case ValueType::Boolean:
				case ValueType::Integer:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpSLE(llvmValue(toInt(a)), llvmValue(toInt(b))));
				case ValueType::Short:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpULE(llvmValue(toShort(a)), llvmValue(toShort(b))));
				case ValueType::Byte:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpULE(llvmValue(a), llvmValue(b)));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOLE(llvmValue(toFloat(a)), llvmValue(toFloat(b))));
				default: break;
			}
			break;
		case ValueType::Float:
			return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOLE(llvmValue(toFloat(a)), llvmValue(toFloat(b))));
		default: break;
	}

	assert("Invalid value" && 0); return Value();
}

Value Builder::greater(const Value &a, const Value &b) {
	if (a.isConstant() && b.isConstant()) {
		return Value(ConstantValue::lessEqual(a.constant(), b.constant()), mRuntime);
	}
	if (a.valueType()->type() == ValueType::String) {
		//TODO: finish this
		assert(0);
		return Value();
	}
	if (b.valueType()->type() == ValueType::String) {
		//TODO: finish this
		assert(0);
		return Value();
	}
	switch (a.valueType()->type()) {
		case ValueType::Boolean:
		case ValueType::Integer:
			switch (b.valueType()->type()) {
				case ValueType::Boolean:
				case ValueType::Integer:
				case ValueType::Short:
				case ValueType::Byte:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpSGT(llvmValue(toInt(a)), llvmValue(toInt(b))));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOGT(llvmValue(toFloat(a)), llvmValue(toFloat(b))));
				default: break;
			}
			break;
		case ValueType::Short:
			switch (b.valueType()->type()) {
				case ValueType::Boolean:
				case ValueType::Integer:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpSGT(llvmValue(toInt(a)), llvmValue(toInt(b))));
				case ValueType::Short:
				case ValueType::Byte:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpUGT(llvmValue(toShort(a)), llvmValue(toShort(b))));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOGT(llvmValue(toFloat(a)), llvmValue(toFloat(b))));
				default: break;
			}
			break;
		case ValueType::Byte:
			switch (b.valueType()->type()) {
				case ValueType::Boolean:
				case ValueType::Integer:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpSGT(llvmValue(toInt(a)), llvmValue(toInt(b))));
				case ValueType::Short:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpUGT(llvmValue(toShort(a)), llvmValue(toShort(b))));
				case ValueType::Byte:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpUGT(llvmValue(a), llvmValue(b)));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOGT(llvmValue(toFloat(a)), llvmValue(toFloat(b))));
				default: break;
			}
			break;
		case ValueType::Float:
			return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOGT(llvmValue(toFloat(a)), llvmValue(toFloat(b))));
		default: break;
	}

	assert("Invalid value" && 0);
	return Value();
}

Value Builder::greaterEqual(const Value &a, const Value &b) {
	if (a.isConstant() && b.isConstant()) {
		return Value(ConstantValue::lessEqual(a.constant(), b.constant()), mRuntime);
	}
	if (a.valueType()->type() == ValueType::String) {
		//TODO: finish this
		assert(0);
		return Value();
	}
	if (b.valueType()->type() == ValueType::String) {
		//TODO: finish this
		assert(0);
		return Value();
	}
	switch (a.valueType()->type()) {
		case ValueType::Boolean:
		case ValueType::Integer:
			switch (b.valueType()->type()) {
				case ValueType::Boolean:
				case ValueType::Integer:
				case ValueType::Short:
				case ValueType::Byte:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpSGE(llvmValue(toInt(a)), llvmValue(toInt(b))));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOGE(llvmValue(toFloat(a)), llvmValue(toFloat(b))));
				default: break;
			}
			break;
		case ValueType::Short:
			switch (b.valueType()->type()) {
				case ValueType::Boolean:
				case ValueType::Integer:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpSGE(llvmValue(toInt(a)), llvmValue(toInt(b))));
				case ValueType::Short:
				case ValueType::Byte:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpUGE(llvmValue(toShort(a)), llvmValue(toShort(b))));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOGE(llvmValue(toFloat(a)), llvmValue(toFloat(b))));
				default: break;
			}
			break;
		case ValueType::Byte:
			switch (b.valueType()->type()) {
				case ValueType::Boolean:
				case ValueType::Integer:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpSGE(llvmValue(toInt(a)), llvmValue(toInt(b))));
				case ValueType::Short:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpUGE(llvmValue(toShort(a)), llvmValue(toShort(b))));
				case ValueType::Byte:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpUGE(llvmValue(a), llvmValue(b)));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOGE(llvmValue(toFloat(a)), llvmValue(toFloat(b))));
				default: break;
			}
			break;
		case ValueType::Float:
			return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOGE(llvmValue(toFloat(a)), llvmValue(toFloat(b))));
		default: break;
	}

	assert(0); return Value();
}

Value Builder::equal(const Value &a, const Value &b) {
	if (a.isConstant() && b.isConstant()) {
		return Value(ConstantValue::equal(a.constant(), b.constant()), mRuntime);
	}
	switch (a.valueType()->type()) {
		case ValueType::Integer:
			switch (b.valueType()->type()) {
				case ValueType::Short:
				case ValueType::Byte:
				case ValueType::Integer:
				case ValueType::Boolean:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpEQ(llvmValue(a), llvmValue(toInt(b))));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOEQ(llvmValue(toFloat(a)), llvmValue(b)));
				case ValueType::String: {
					Value as = toString(a);
					llvm::Value *ret = mRuntime->stringValueType()->stringEquality(&mIRBuilder, llvmValue(as), llvmValue(b));
					destruct(b);
					destruct(as);
					return Value(mRuntime->booleanValueType(), ret);
				}
				default: break;
			}
			break;
		case ValueType::Float:
			switch (b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Float:
				case ValueType::Short:
				case ValueType::Byte:
				case ValueType::Boolean:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOEQ(llvmValue(a), llvmValue(toFloat(b))));
				case ValueType::String: {
					Value as = toString(a);
					llvm::Value *ret = mRuntime->stringValueType()->stringEquality(&mIRBuilder, llvmValue(as), llvmValue(b));
					destruct(b);
					destruct(as);
					return Value(mRuntime->booleanValueType(), ret);
				}
				default: break;
			}
			break;
		case ValueType::Boolean:
			switch (b.valueType()->type()) {
				case ValueType::Boolean:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpEQ(llvmValue(a), llvmValue(b)));
				case ValueType::Byte:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpEQ(llvmValue(toByte(a)), llvmValue(b)));
				case ValueType::Short:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpEQ(llvmValue(toShort(a)), llvmValue(b)));
				case ValueType::Integer:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpEQ(llvmValue(toInt(a)), llvmValue(b)));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpEQ(llvmValue(toFloat(a)), llvmValue(b)));
				case ValueType::String: {
					Value as = toString(a);
					llvm::Value *ret = mRuntime->stringValueType()->stringEquality(&mIRBuilder, llvmValue(as), llvmValue(b));
					destruct(as);
					destruct(b);
					return Value(mRuntime->booleanValueType(), ret);
				}
				default: break;
			}
			break;
		case ValueType::Short:
			switch (b.valueType()->type()) {
				case ValueType::Boolean:
				case ValueType::Short:
				case ValueType::Byte:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpEQ(llvmValue(a), llvmValue(toShort(b))));
				case ValueType::Integer:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpEQ(llvmValue(toInt(a)), llvmValue(b)));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOEQ(llvmValue(toFloat(a)), llvmValue(b)));
				case ValueType::String: {
					Value as = toString(a);
					llvm::Value *ret = mRuntime->stringValueType()->stringEquality(&mIRBuilder, llvmValue(as), llvmValue(b));
					destruct(as);
					destruct(b);
					return Value(mRuntime->booleanValueType(), ret);
				}
				default: break;
			}
			break;
		case ValueType::Byte:
			switch (b.valueType()->type()) {
				case ValueType::Boolean:
				case ValueType::Byte:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpEQ(llvmValue(a), llvmValue(toByte(b))));
				case ValueType::Short:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpEQ(llvmValue(toShort(a)), llvmValue(b)));
				case ValueType::Integer:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpEQ(llvmValue(toInt(a)), llvmValue(b)));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpOEQ(llvmValue(toFloat(a)), llvmValue(b)));
				case ValueType::String: {
					Value as = toString(a);
					llvm::Value *ret = mRuntime->stringValueType()->stringEquality(&mIRBuilder, llvmValue(as), llvmValue(b));
					destruct(as);
					destruct(b);
					return Value(mRuntime->booleanValueType(), ret);
				}
				default: break;
			}
			break;
		case ValueType::String: {
			if (b.valueType()->type() == ValueType::String) {
				llvm::Value *ret = mRuntime->stringValueType()->stringEquality(&mIRBuilder, llvmValue(a), llvmValue(b));
				return Value(mRuntime->booleanValueType(), ret);
			}
			Value bs = toString(b);
			llvm::Value *ret = mRuntime->stringValueType()->stringEquality(&mIRBuilder, llvmValue(a), llvmValue(bs));
			destruct(bs);
			destruct(a);
			return Value(mRuntime->booleanValueType(), ret);
		}
		case ValueType::TypePointer:
			if (b.valueType()->type() == ValueType::TypePointer || b.valueType()->type() == ValueType::TypePointerCommon) {
				llvm::Value *ap = bitcast(llvm::Type::getInt8PtrTy(context()), llvmValue(a));
				llvm::Value *bp = bitcast(llvm::Type::getInt8PtrTy(context()), llvmValue(b));
				return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpEQ(ap, bp));
			}
			break;
		case ValueType::TypePointerCommon:
			if (b.valueType()->type() == ValueType::TypePointer || b.valueType()->type() == ValueType::TypePointerCommon) {
				llvm::Value *ap = bitcast(llvm::Type::getInt8PtrTy(context()), llvmValue(a));
				llvm::Value *bp = bitcast(llvm::Type::getInt8PtrTy(context()), llvmValue(b));
				return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpEQ(ap, bp));
			}
			break;
		default: break;
	}

	assert("Invalid equality operation" && 0); return Value();
}

Value Builder::notEqual(const Value &a, const Value &b) {
	if (a.isConstant() && b.isConstant()) {
		return Value(ConstantValue::equal(a.constant(), b.constant()), mRuntime);
	}
	switch (a.valueType()->type()) {
		case ValueType::Integer:
			switch (b.valueType()->type()) {
				case ValueType::Short:
				case ValueType::Byte:
				case ValueType::Integer:
				case ValueType::Boolean:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpNE(llvmValue(a), llvmValue(toInt(b))));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpONE(llvmValue(toFloat(a)), llvmValue(b)));
				case ValueType::String: {
					//TODO: StringValueType::stringUnequality?
					Value as = toString(a);
					llvm::Value *ret = mRuntime->stringValueType()->stringEquality(&mIRBuilder, llvmValue(as), llvmValue(b));
					destruct(b);
					destruct(as);
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateNot(ret));
				}
				default: break;
			}
			break;
		case ValueType::Float:
			switch (b.valueType()->type()) {
				case ValueType::Integer:
				case ValueType::Float:
				case ValueType::Short:
				case ValueType::Byte:
				case ValueType::Boolean:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpONE(llvmValue(a), llvmValue(toInt(b))));
				case ValueType::String: {
					Value as = toString(a);
					llvm::Value *ret = mRuntime->stringValueType()->stringEquality(&mIRBuilder, llvmValue(as), llvmValue(b));
					destruct(b);
					destruct(as);
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateNot(ret));
				}
				default: break;
			}
			break;
		case ValueType::Boolean:
			switch (b.valueType()->type()) {
				case ValueType::Boolean:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpNE(llvmValue(a), llvmValue(b)));
				case ValueType::Byte:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpNE(llvmValue(toByte(a)), llvmValue(b)));
				case ValueType::Short:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpNE(llvmValue(toShort(a)), llvmValue(b)));
				case ValueType::Integer:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpNE(llvmValue(toInt(a)), llvmValue(b)));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpNE(llvmValue(toFloat(a)), llvmValue(b)));
				case ValueType::String: {
					Value as = toString(a);
					llvm::Value *ret = mRuntime->stringValueType()->stringEquality(&mIRBuilder, llvmValue(as), llvmValue(b));
					destruct(as);
					destruct(b);
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateNot(ret));
				}
				default: break;
			}
			break;
		case ValueType::Short:
			switch (b.valueType()->type()) {
				case ValueType::Boolean:
				case ValueType::Short:
				case ValueType::Byte:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpNE(llvmValue(a), llvmValue(toShort(b))));
				case ValueType::Integer:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpNE(llvmValue(toInt(a)), llvmValue(b)));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpONE(llvmValue(toFloat(a)), llvmValue(b)));
				case ValueType::String: {
					Value as = toString(a);
					llvm::Value *ret = mRuntime->stringValueType()->stringEquality(&mIRBuilder, llvmValue(as), llvmValue(b));
					destruct(as);
					destruct(b);
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateNot(ret));
				}
				default: break;
			}
			break;
		case ValueType::Byte:
			switch (b.valueType()->type()) {
				case ValueType::Boolean:
				case ValueType::Byte:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpNE(llvmValue(a), llvmValue(toByte(b))));
				case ValueType::Short:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpNE(llvmValue(toShort(a)), llvmValue(b)));
				case ValueType::Integer:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpNE(llvmValue(toInt(a)), llvmValue(b)));
				case ValueType::Float:
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateFCmpONE(llvmValue(toFloat(a)), llvmValue(b)));
				case ValueType::String: {
					Value as = toString(a);
					llvm::Value *ret = mRuntime->stringValueType()->stringEquality(&mIRBuilder, llvmValue(as), llvmValue(b));
					destruct(as);
					destruct(b);
					return Value(mRuntime->booleanValueType(), mIRBuilder.CreateNot(ret));
				}
				default: break;
			}
			break;
		case ValueType::String: {
			if (b.valueType()->type() == ValueType::String) {
				llvm::Value *ret = mRuntime->stringValueType()->stringEquality(&mIRBuilder, llvmValue(a), llvmValue(b));
				return Value(mRuntime->booleanValueType(), mIRBuilder.CreateNot(ret));
			}
			Value bs = toString(b);
			llvm::Value *ret = mRuntime->stringValueType()->stringEquality(&mIRBuilder, llvmValue(a), llvmValue(bs));
			destruct(bs);
			destruct(a);
			return Value(mRuntime->booleanValueType(), mIRBuilder.CreateNot(ret));
		}
		case ValueType::TypePointer:
			if (b.valueType()->type() == ValueType::TypePointer || b.valueType()->type() == ValueType::TypePointerCommon)
				return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpNE(llvmValue(a), llvmValue(toValueType(a.valueType(), b))));
			break;
		case ValueType::TypePointerCommon:
			if (b.valueType()->type() == ValueType::TypePointer || b.valueType()->type() == ValueType::TypePointerCommon)
				return Value(mRuntime->booleanValueType(), mIRBuilder.CreateICmpNE(llvmValue(toValueType(b.valueType(), a)), llvmValue(b)));
			break;
		default: break;
	}

	assert("Invalid operation" && 0); return Value();
}

void Builder::memCopy(llvm::Value *src, llvm::Value *dest, llvm::Value *num, int align) {
	assert(src->getType()->isPointerTy());
	assert(dest->getType()->isPointerTy());
	assert(num->getType() == llvm::IntegerType::get(context(), 32));

	//Cast src and dest to i8* if they are not already
	if (src->getType() != llvm::IntegerType::get(context(), 8)->getPointerTo()) {
		src = mIRBuilder.CreateBitCast(src, llvm::IntegerType::get(context(), 8)->getPointerTo());
	}
	if (dest->getType() != llvm::IntegerType::get(context(), 8)->getPointerTo()) {
		dest = mIRBuilder.CreateBitCast(dest, llvm::IntegerType::get(context(), 8)->getPointerTo());
	}

	mIRBuilder.CreateMemCpy(dest, src, num, align);
}

void Builder::memSet(llvm::Value *ptr, llvm::Value *num, llvm::Value *value, int align) {
	assert(ptr->getType()->isPointerTy());
	assert(num->getType() == llvm::IntegerType::get(context(), 32));
	assert(value->getType() == llvm::IntegerType::get(context(), 8));

	//Cast src and dest to i8* if they are not already
	if (ptr->getType() != llvm::IntegerType::get(context(), 8)->getPointerTo()) {
		ptr = mIRBuilder.CreateBitCast(ptr, llvm::IntegerType::get(context(), 8)->getPointerTo());
	}

	mIRBuilder.CreateMemSet(ptr, value, num, align);
}

llvm::Value *Builder::allocate(llvm::Value *size) {
	assert(size->getType() == mIRBuilder.getInt32Ty());
	return mIRBuilder.CreateCall(mRuntime->allocatorFunction(), size);
}

void Builder::free(llvm::Value *ptr) {
	assert(ptr->getType()->isPointerTy());
	ptr = pointerToBytePointer(ptr);
	mIRBuilder.CreateCall(mRuntime->freeFunction(), ptr);
}

llvm::Value *Builder::pointerToBytePointer(llvm::Value *ptr) {
	assert(ptr->getType()->isPointerTy());
	if (ptr->getType() != llvm::IntegerType::get(context(), 8)->getPointerTo()) {
		ptr = mIRBuilder.CreateBitCast(ptr, llvm::IntegerType::get(context(), 8)->getPointerTo());
	}
	return ptr;
}




//TODO: These dont seems to work? Why?
void Builder::pushInsertPoint() {
	llvm::IRBuilder<>::InsertPoint insertPoint = mIRBuilder.saveIP();
	mInsertPointStack.push(insertPoint);
}

void Builder::popInsertPoint() {
	llvm::IRBuilder<>::InsertPoint insertPoint = mInsertPointStack.pop();
	mIRBuilder.restoreIP(insertPoint);
}


