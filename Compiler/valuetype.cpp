#include "valuetype.h"
#include "llvm.h"
#include "value.h"
#include "abstractsyntaxtree.h"
#include "builder.h"
#include "floatvaluetype.h"
#include "booleanvaluetype.h"
#include "nullvaluetype.h"

ValueType::ValueType(Runtime *r):
	mType(0),
	mRuntime(r)
{
}

ValueType::ValueType(Runtime *r, llvm::Type *type) :
	mType(type),
	mRuntime(r) {
}

bool ValueType::canBeCastedToValueType(ValueType *to) const {
	return castingCostToOtherValueType(to) != ccNoCast;
}

Value ValueType::generateOperation(Builder *, int , const Value &, const Value &, OperationFlags &operationFlags) const {
	operationFlags = OperationFlag::NoSuchOperation;
	return Value();
}

Value ValueType::generateOperation(Builder *, int , const Value &, OperationFlags &operationFlags) const {
	operationFlags = OperationFlag::NoSuchOperation;
	return Value();
}

void ValueType::generateDestructor(Builder *, const Value &) {

}

Value ValueType::generateLoad(Builder *builder, const Value &var) const {
	assert(var.isReference());
	return Value(var.valueType(), builder->irBuilder().CreateLoad(var.value()), false);
}

llvm::LLVMContext &ValueType::context() {
	assert(mType);
	return mType->getContext();
}

OperationFlags ValueType::castCostOperationFlags(ValueType::CastCost cc) {
	switch (cc) {
		case ccNoCast:
			return OperationFlag::OperandBCantBeCastedToA;
		case ccCastToString:
			return OperationFlag::CastToString; break;
		case ccCastFromString:
			return OperationFlag::CastFromString; break;
		/*case ccCastToSmaller:
			return OperationFlag::MayLosePrecision; break;*/
		default:
			return OperationFlag::NoFlags;
	}
}

ValueType::CastCost ValueType::castToSameType(Builder *builder, Value &a, Value &b) {
	if (a.valueType() == b.valueType()) return ccNoCost;

	if (a.valueType() == builder->runtime()->nullValueType()) {
		a = builder->defaultValue(b.valueType());
	}
	if (b.valueType() == builder->runtime()->nullValueType()) {
		b = builder->defaultValue(a.valueType());
	}

	Value ao = a;
	Value bo = b;

	if (!a.valueType()->isBasicType() && !b.valueType()->isBasicType()) {
		CastCost cc = b.valueType()->castingCostToOtherValueType(a.valueType());
		if (cc != ccNoCast) {
			b = a.valueType()->cast(builder, b);
			builder->destruct(bo);
			return cc;
		}

		cc = a.valueType()->castingCostToOtherValueType(b.valueType());
		if (cc != ccNoCast) {
			a = b.valueType()->cast(builder, a);
			builder->destruct(ao);
			return cc;
		}

		return ccNoCast;
	}

	if ((int)b.valueType()->basicType() > (int)a.valueType()->basicType()) { // Cast a -> b
		CastCost cc = a.valueType()->castingCostToOtherValueType(b.valueType());
		if (cc != ccNoCast) {
			a = b.valueType()->cast(builder, a);
			builder->destruct(ao);
		}
		return cc;
	}
	else { // Cast b -> a
		CastCost cc = b.valueType()->castingCostToOtherValueType(a.valueType());
		if (cc != ccNoCast) {
			b = a.valueType()->cast(builder, b);
			builder->destruct(bo);
		}
		return cc;
	}
}

Value ValueType::generateBasicTypeOperation(Builder *builder, int opType, const Value &operand1, const Value &operand2, OperationFlags &operationFlags) const {
	assert(operand1.valueType() == this);
	switch (opType) {
		case ast::ExpressionNode::opAssign: {
			if (!operand1.isReference()) {
				operationFlags = OperationFlag::ReferenceRequired;
				return Value();
			}
			CastCost cc = operand2.valueType()->castingCostToOtherValueType(this);
			operationFlags = castCostOperationFlags(cc);
			if (operationFlagsContainFatalFlags(operationFlags)) return Value();
			Value op2 = this->cast(builder, operand2);
			builder->store(operand1, op2);
			builder->destruct(op2);
			return operand1;
		}

		case ast::ExpressionNode::opEqual: {
			if (operand2.valueType()->isBasicType()) {
				Value op1 = operand1;
				Value op2 = operand2;
				CastCost cc = castToSameType(builder, op1, op2);
				operationFlags = castCostOperationFlags(cc);
				if (operationFlagsContainFatalFlags(operationFlags)) return Value();

				Value ret = builder->equal(op1, op2);
				builder->destruct(op1);
				builder->destruct(op2);
				return ret;
			}
			operationFlags = OperationFlag::NoSuchOperation;
			return Value();
		}
		case ast::ExpressionNode::opNotEqual: {
			if (operand2.valueType()->isBasicType()) {
				Value op1 = operand1;
				Value op2 = operand2;
				CastCost cc = castToSameType(builder, op1, op2);
				operationFlags = castCostOperationFlags(cc);
				if (operationFlagsContainFatalFlags(operationFlags)) return Value();

				Value ret = builder->notEqual(op1, op2);
				builder->destruct(op1);
				builder->destruct(op2);
				return ret;
			}
			operationFlags = OperationFlag::NoSuchOperation;
			return Value();
		}

		case ast::ExpressionNode::opGreater: {
			if (operand2.valueType()->isBasicType()) {
				Value op1 = operand1;
				Value op2 = operand2;
				CastCost cc = castToSameType(builder, op1, op2);
				operationFlags = castCostOperationFlags(cc);
				if (operationFlagsContainFatalFlags(operationFlags)) return Value();

				Value ret = builder->greater(op1, op2);
				builder->destruct(op1);
				builder->destruct(op2);
				return ret;
			}
			operationFlags = OperationFlag::NoSuchOperation;
			return Value();
		}
		case ast::ExpressionNode::opLess: {
			if (operand2.valueType()->isBasicType()) {
				Value op1 = operand1;
				Value op2 = operand2;
				CastCost cc = castToSameType(builder, op1, op2);
				operationFlags = castCostOperationFlags(cc);
				if (operationFlagsContainFatalFlags(operationFlags)) return Value();

				Value ret = builder->less(op1, op2);
				builder->destruct(op1);
				builder->destruct(op2);
				return ret;
			}
			operationFlags = OperationFlag::NoSuchOperation;
			return Value();
		}
		case ast::ExpressionNode::opGreaterEqual: {
			if (operand2.valueType()->isBasicType()) {
				Value op1 = operand1;
				Value op2 = operand2;
				CastCost cc = castToSameType(builder, op1, op2);
				operationFlags = castCostOperationFlags(cc);
				if (operationFlagsContainFatalFlags(operationFlags)) return Value();

				Value ret = builder->greaterEqual(op1, op2);
				builder->destruct(op1);
				builder->destruct(op2);
				return ret;
			}
			operationFlags = OperationFlag::NoSuchOperation;
			return Value();
		}
		case ast::ExpressionNode::opLessEqual:{
			if (operand2.valueType()->isBasicType()) {
				Value op1 = operand1;
				Value op2 = operand2;
				CastCost cc = castToSameType(builder, op1, op2);
				operationFlags = castCostOperationFlags(cc);
				if (operationFlagsContainFatalFlags(operationFlags)) return Value();

				Value ret = builder->lessEqual(op1, op2);
				builder->destruct(op1);
				builder->destruct(op2);
				return ret;
			}
			operationFlags = OperationFlag::NoSuchOperation;
			return Value();
		}

		case ast::ExpressionNode::opAdd: {
			if (operand2.valueType()->isBasicType()) {
				Value op1 = operand1;
				Value op2 = operand2;
				CastCost cc = castToSameType(builder, op1, op2);
				operationFlags = castCostOperationFlags(cc);
				if (operationFlagsContainFatalFlags(operationFlags)) return Value();

				Value ret = builder->add(op1, op2);
				builder->destruct(op1);
				builder->destruct(op2);
				return ret;
			}
			operationFlags = OperationFlag::NoSuchOperation;
			return Value();
		}
		case ast::ExpressionNode::opSubtract: {
			if (operand2.valueType()->isNumber()) {
				Value op1 = operand1;
				Value op2 = operand2;
				CastCost cc = castToSameType(builder, op1, op2);
				operationFlags = castCostOperationFlags(cc);
				if (operationFlagsContainFatalFlags(operationFlags)) return Value();

				Value ret = builder->subtract(op1, op2);
				builder->destruct(op1);
				builder->destruct(op2);
				return ret;
			}
			operationFlags = OperationFlag::NoSuchOperation;
			return Value();
		}
		case ast::ExpressionNode::opMultiply: {
			if (operand2.valueType()->isNumber()) {
				Value op1 = operand1;
				Value op2 = operand2;
				CastCost cc = castToSameType(builder, op1, op2);
				operationFlags = castCostOperationFlags(cc);
				if (operationFlagsContainFatalFlags(operationFlags)) return Value();

				Value ret = builder->multiply(op1, op2);
				builder->destruct(op1);
				builder->destruct(op2);
				return ret;
			}
			operationFlags = OperationFlag::NoSuchOperation;
			return Value();
		}
		case ast::ExpressionNode::opPower: {
			if (operand2.valueType()->isNumber()) {
				Value op1 = operand1;
				Value op2 = operand2;
				CastCost cc = castToSameType(builder, op1, op2);
				operationFlags = castCostOperationFlags(cc);
				if (operationFlagsContainFatalFlags(operationFlags)) return Value();

				Value ret = builder->power(op1, op2);
				builder->destruct(op1);
				builder->destruct(op2);
				return ret;
			}
			operationFlags = OperationFlag::NoSuchOperation;
			return Value();
		}

		case ast::ExpressionNode::opMod: {
			if (operand2.valueType()->isNumber()) {
				Value op1 = operand1;
				Value op2 = operand2;
				CastCost cc = castToSameType(builder, op1, op2);
				operationFlags = castCostOperationFlags(cc);
				if (operationFlagsContainFatalFlags(operationFlags)) return Value();

				Value ret = builder->mod(op1, op2);
				builder->destruct(op1);
				builder->destruct(op2);
				return ret;
			}
			operationFlags = OperationFlag::NoSuchOperation;
			return Value();
		}
		case ast::ExpressionNode::opShl: {
			if (operand2.valueType()->isNumber() && operand2.valueType() != mRuntime->floatValueType()) {
				Value op1 = operand1;
				Value op2 = operand2;
				CastCost cc = castToSameType(builder, op1, op2);
				operationFlags = castCostOperationFlags(cc);
				if (operationFlagsContainFatalFlags(operationFlags)) return Value();

				Value ret = builder->shl(op1, op2);
				builder->destruct(op1);
				builder->destruct(op2);
				return ret;
			}
			operationFlags = OperationFlag::NoSuchOperation;
			return Value();
		}
		case ast::ExpressionNode::opShr: {
			if (operand2.valueType()->isNumber() && operand2.valueType() != mRuntime->floatValueType()) {
				Value op1 = operand1;
				Value op2 = operand2;
				CastCost cc = castToSameType(builder, op1, op2);
				operationFlags = castCostOperationFlags(cc);
				if (operationFlagsContainFatalFlags(operationFlags)) return Value();

				Value ret = builder->shr(op1, op2);
				builder->destruct(op1);
				builder->destruct(op2);
				return ret;
			}
			operationFlags = OperationFlag::NoSuchOperation;
			return Value();
		}
		case ast::ExpressionNode::opSar: {
			if (operand2.valueType()->isNumber() && operand2.valueType() != mRuntime->floatValueType()) {
				Value op1 = operand1;
				Value op2 = operand2;
				CastCost cc = castToSameType(builder, op1, op2);
				operationFlags = castCostOperationFlags(cc);
				if (operationFlagsContainFatalFlags(operationFlags)) return Value();

				Value ret = builder->sar(op1, op2);
				builder->destruct(op1);
				builder->destruct(op2);
				return ret;
			}
			operationFlags = OperationFlag::NoSuchOperation;
			return Value();
		}
		case ast::ExpressionNode::opDivide: {
			if (operand2.valueType()->isNumber()) {
				Value op1 = operand1;
				Value op2 = operand2;
				CastCost cc = castToSameType(builder, op1, op2);
				operationFlags = castCostOperationFlags(cc);


				if (operationFlagsContainFatalFlags(operationFlags)) return Value();
				//Integer divided by zero
				if (operand1.valueType()->basicType() <= ValueType::Integer
						&& operand2.valueType()->basicType() <= ValueType::Integer
						&& operand2.isConstant()
						&& operand2.constant().toInt() == 0) {
					operationFlags |= OperationFlag::IntegerDividedByZero;
					return Value();
				}
				if (op1.isConstant() && op2.isConstant()) {
					ConstantValue constVal = ConstantValue::divide(op1.constant(), op2.constant(), operationFlags);
					if (operationFlagsContainFatalFlags(operationFlags)) return Value();

					return Value(constVal, mRuntime);
				}
				Value ret = builder->divide(op1, op2);
				builder->destruct(op1);
				builder->destruct(op2);
				return ret;
			}
			operationFlags = OperationFlag::NoSuchOperation;
			return Value();
		}
		case ast::ExpressionNode::opAnd: {
			CastCost cc1 = operand1.valueType()->castingCostToOtherValueType(mRuntime->booleanValueType());
			operationFlags = castCostOperationFlags(cc1);
			if (operationFlagsContainFatalFlags(operationFlags)) return Value();

			CastCost cc2 = operand2.valueType()->castingCostToOtherValueType(mRuntime->booleanValueType());
			operationFlags |= castCostOperationFlags(cc2);
			if (operationFlagsContainFatalFlags(operationFlags)) return Value();

			Value op1 = mRuntime->booleanValueType()->cast(builder, operand1);
			Value op2 = mRuntime->booleanValueType()->cast(builder, operand2);

			Value ret = builder->and_(op1, op2);
			builder->destruct(op1);
			builder->destruct(op2);
			return ret;
		}
		case ast::ExpressionNode::opOr: {
			CastCost cc1 = operand1.valueType()->castingCostToOtherValueType(mRuntime->booleanValueType());
			operationFlags = castCostOperationFlags(cc1);
			if (operationFlagsContainFatalFlags(operationFlags)) return Value();

			CastCost cc2 = operand2.valueType()->castingCostToOtherValueType(mRuntime->booleanValueType());
			operationFlags |= castCostOperationFlags(cc2);
			if (operationFlagsContainFatalFlags(operationFlags)) return Value();

			Value op1 = mRuntime->booleanValueType()->cast(builder, operand1);
			Value op2 = mRuntime->booleanValueType()->cast(builder, operand2);

			Value ret = builder->or_(op1, op2);
			builder->destruct(op1);
			builder->destruct(op2);
			return ret;
		}
		case ast::ExpressionNode::opXor: {
			CastCost cc1 = operand1.valueType()->castingCostToOtherValueType(mRuntime->booleanValueType());
			operationFlags = castCostOperationFlags(cc1);
			if (operationFlagsContainFatalFlags(operationFlags)) return Value();

			CastCost cc2 = operand2.valueType()->castingCostToOtherValueType(mRuntime->booleanValueType());
			operationFlags |= castCostOperationFlags(cc2);
			if (operationFlagsContainFatalFlags(operationFlags)) return Value();

			Value op1 = mRuntime->booleanValueType()->cast(builder, operand1);
			Value op2 = mRuntime->booleanValueType()->cast(builder, operand2);

			Value ret = builder->xor_(op1, op2);
			builder->destruct(op1);
			builder->destruct(op2);
			return ret;
		}
		case ast::ExpressionNode::opComma:
			builder->destruct(operand1);
			return operand2;
	}
	assert("Invalid ast::ExpressionNode::Op" && 0);
	return Value();
}

Value ValueType::generateBasicTypeOperation(Builder *builder, int opType, const Value &operand, OperationFlags &operationFlags) const {
	switch (opType) {
		case ast::Unary::opNegative:
			if (operand.valueType()->isNumber()) {
				Value ret = builder->minus(operand);
				builder->destruct(operand);
				return ret;
			}
			operationFlags = OperationFlag::NoSuchOperation;
			return Value();
		case ast::Unary::opNot: {
			CastCost cc = operand.valueType()->castingCostToOtherValueType(mRuntime->booleanValueType());
			operationFlags = castCostOperationFlags(cc);
			if (operationFlagsContainFatalFlags(operationFlags)) return Value();

			Value op = this->cast(builder, operand);
			Value ret = builder->not_(op);
			builder->destruct(op);
			return ret;
		}
		case ast::Unary::opPositive: {
			if (operand.valueType()->isNumber()) {
				Value ret = builder->plus(operand);
				builder->destruct(operand);
				return ret;
			}
			operationFlags = OperationFlag::NoSuchOperation;
			return Value();
		}
	}
	assert("Invalid ast::Unary::Op" && 0);
	return Value();
}
