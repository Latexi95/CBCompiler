#ifndef ERRORCODES_H
#define ERRORCODES_H

namespace ErrorCodes {
enum ErrorCode {
	//Settings
	ecSettingsLoadingFailed = 11,
	ecCantParseCustomDataTypeDefinitionFile,
	ecInvalidCustomDataTypeDefinitionFileFormat,

	//Lexer
	ecLexicalAnalysingFailed,
	ecCantOpenFile,
	ecExpectingEndOfString,
	ecExpectingRemEndBeforeEOF,
	ecExpectingString,
	ecUnexpectedCharacter,

	//Parser
	ecExpectingIdentifier,
	ecExpectingEndOfStatement,
	ecExpectingVarType,
	ecExpectingLeftParenthese,
	ecExpectingRightParenthese,
	ecExpectingLeftSquareBracket,
	ecExpectingRightSquareBracket,
	ecExpectingPrimaryExpression,
	ecExpectingEndOfRepeat,
	ecExpectingEndFunction,
	ecExpectingEndType,
	ecExpectingEndIf,
	ecExpectingWend,
	ecExpectingNext,
	ecExpectingTo,
	ecExpectingEach,
	ecExpectingTypePtr,
	ecExpectingVariable,
	ecExpectingAssignment,
	ecExpectingEndSelect,
	ecCantParseInteger,
	ecCantParseFloat,
	ecForAndNextDontMatch,
	ecTypesDontMatch,
	ecUnexpectedToken,
	ecMultipleSelectDefaultCases,
	ecExpectingLabel,

	ecFunctionReturnTypeDefinedTwice,
	ecFunctionReturnTypeRequired,

	ecVarAlreadyDefinedWithAnotherType,
	ecVariableTypeDefinedTwice,

	ecParsingFailed,

	//Runtime
	ecCantLoadRuntime,
	ecInvalidRuntime,
	ecPrefixReserved,
	ecCantLoadFunctionMapping,
	ecInvalidFunctionMappingFile,
	ecCantFindRuntimeFunction,
	ecCantFindCustomDataType,
	ecValueTypeDefinedMultipleTimes,

	//Code generation
	ecCodeGeneratorInitializationFailed,
	ecOnlyBasicValueTypeCanBeConstant,
	ecNotConstant,
	ecNotVariable,
	ecNotTypePointer,
	ecArraySubscriptNotInteger,
	ecConstantAlreadyDefined,
	ecSymbolAlreadyDefinedInRuntime,
	ecSymbolAlreadyDefined,
	ecCantFindType,
	ecCantFindTypeField,
	ecForEachInvalidContainer,
	ecForcingType,
	ecNotArrayOrFunction,
	ecInvalidArraySubscript,
	ecMathematicalOperationOperandTypeMismatch,
	ecCantCreateTypePointerLLVMStructType,
	ecVariableNotDefined,
	ecCantFindFunction,
	ecCantFindCommand,
	ecMultiplePossibleOverloads,
	ecNoCastFromTypePointer,
	ecNotNumber,
	ecInvalidAssignment,
	ecVariableAlreadyDefined,
	ecParametreSymbolAlreadyDefined,
	ecFunctionAlreadyDefined,
	ecSymbolAlreadyDefinedWithDifferentType,
	ecCantFindSymbol,
	ecForVariableContainerTypeMismatch,
	ecNotCommand,
	ecExpectingValueAfterReturn,
	ecInvalidReturn,
	ecGosubCannotReturnValue,
	ecLabelAlreadyDefined,
	ecInvalidExit,
	ecCantUseCommandInsideExpression,
	ecSymbolNotArray,
	ecSymbolNotArrayOrCommand,
	ecArrayDimensionCountDoesntMatch,
	ecExpectingNumberValue,
	ecDangerousFloatToIntCast,
	ecTypeCantHaveValueType,
	ecExpectingType,
	ecCastFunctionRequiresOneParameter,
	ecCantCastValue,

	ecCantWriteBitcodeFile,

	ecCodeGenerationFailed,

	ecTypeHasMultipleFieldsWithSameName,

	ecWTF

};
}
#endif // ERRORCODES_H
