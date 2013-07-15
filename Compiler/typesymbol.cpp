#include "typesymbol.h"
#include "typepointervaluetype.h"
#include "llvm.h"
#include "runtime.h"
#include "builder.h"


TypeSymbol::TypeSymbol(const QString &name, QFile *file, int line):
	Symbol(name, file, line),
	mTypePointerValueType(0),
	mGlobalTypeVariable(0),
	mFirstFieldIndex(0),
	mMemberSize(0) {
}

bool TypeSymbol::addField(const TypeField &field) {
	if (mFieldSearch.contains(field.name())) {
		return false;
	}
	mFields.append(field);
	mFieldSearch.insert(field.name(), --mFields.end());
	return true;
}

bool TypeSymbol::hasField(const QString &name) {
	return mFieldSearch.contains(name);
}

const TypeField &TypeSymbol::field(const QString &name) const{
	QMap<QString, QList<TypeField>::Iterator>::ConstIterator i = mFieldSearch.find(name);
	assert(i != mFieldSearch.end());
	return *i.value();
}

void TypeSymbol::initializeType(Builder *b) {
	assert("TODO: Call CBF_CB_ConstructType" && 0);
}


TypeField::TypeField(const QString &name, ValueType *valueType, QFile *file, int line) :
	mName(name),
	mValueType(valueType),
	mLine(line),
	mFile(file){
}

QString TypeField::info() const {
	return QString("Field %1 %2").arg(mValueType->name(), mName);
}


void TypeSymbol::createTypePointerValueType(Builder *b) {
	mRuntime = b->runtime();
	createLLVMMemberType();
	mGlobalTypeVariable = b->createGlobalVariable(mRuntime->typeLLVMType(), false, llvm::GlobalValue::PrivateLinkage, 0);
}

void TypeSymbol::createLLVMMemberType() {
	std::vector<llvm::Type*> elements;

	//Copy header
	unsigned i;
	for (i = 0; i < mRuntime->typeMemberLLVMType()->getStructNumElements(); i++) {
		elements.push_back(mRuntime->typeMemberLLVMType()->getStructElementType(i));
	}
	mFirstFieldIndex = i;
	foreach(const TypeField &field, mFields) {
		elements.push_back(field.valueType()->llvmType());
	}

	mMemberType = llvm::StructType::get(mRuntime->module()->getContext(), elements);
	mMemberSize = mRuntime->dataLayout().getTypeAllocSize(mMemberType);
}


QString TypeSymbol::info() const {
	QString str("Type %1\n");
	str = str.arg(mName);
	for (QList<TypeField>::ConstIterator i = mFields.begin(); i != mFields.end(); i++) {
		str += "    " + i->info() + '\n';
	}
	return str;
}
