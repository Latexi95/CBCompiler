#ifndef TYPESYMBOL_H
#define TYPESYMBOL_H
#include "symbol.h"
#include <QMap>
#include <QList>
#include "value.h"

namespace llvm{
	class StructType;
	class Type;
	class Value;
	class Module;
	class GlobalVariable;
}
class Builder;
class TypePointerValueType;
class ValueType;
class Runtime;
class TypeField {
	public:
		TypeField(const QString &name, ValueType *valueType, QFile *file, int line);
		QString name()const{return mName;}
		ValueType *valueType()const{return mValueType;}
		QString info() const;
	private:
		QString mName;
		ValueType *mValueType;
		int mLine;
		QFile *mFile;
};

class TypeSymbol : public Symbol {
	public:
		TypeSymbol(const QString &name, Runtime *r, QFile *file, int line);
		Type type()const{return stType;}
		QString info() const;
		bool addField(const TypeField &field);
		bool hasField(const QString &name);
		const TypeField &field(const QString &name) const;
		int fieldIndex(const QString &name) const;
		llvm::StructType *llvmMemberType() const {return mMemberType;}

		void initializeType(Builder *b);
		void createOpaqueTypes(Builder *b);
		void createTypePointerValueType(Builder *b);
		TypePointerValueType *typePointerValueType()const{return mTypePointerValueType;}
		llvm::GlobalVariable *globalTypeVariable() { return mGlobalTypeVariable; }

		Value typeValue();
	private:
		void createLLVMMemberType();
		QList<TypeField> mFields;
		QMap<QString, QList<TypeField>::Iterator> mFieldSearch;
		Runtime *mRuntime;
		llvm::StructType *mMemberType;
		llvm::GlobalVariable *mGlobalTypeVariable;
		TypePointerValueType *mTypePointerValueType;
		unsigned mFirstFieldIndex;
		int mMemberSize;
};

#endif // TYPESYMBOL_H
