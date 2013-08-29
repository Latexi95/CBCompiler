#include "variablesymbol.h"
#include "valuetype.h"

VariableSymbol::VariableSymbol(const QString &name, ValueType *t, const QString &f, int line):
	Symbol(name, f, line),
	mValueType(t),
	mAlloca(0) {
	assert(t);
}

QString VariableSymbol::info() const {
	return QString("Variable \"%1\" %2").arg(mName, mValueType->name());
}
