#include "constantsymbol.h"

ConstantSymbol::ConstantSymbol(const QString &symbolName, ValueType valueType, const CodePoint &cp):
	Symbol(symbolName, cp),
	mValue(val),
	mValueType(valueType) {

}

QString ConstantSymbol::info() const {
	QString str("Constant %1 = %2 %3");

	str = str.arg(mName, mValue.typeName(), mValue.valueInfo());
	return str;
}
