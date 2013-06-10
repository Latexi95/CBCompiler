#include "lexer.h"
#include "errorcodes.h"
#include <QTextStream>
#include <QDir>
#include <QDebug>
Lexer::Lexer()
{
	mKeywords["not"] = Token::opNot;
	mKeywords["or"] = Token::opOr;
	mKeywords["xor"] = Token::opXor;
	mKeywords["mod"] = Token::opMod;
	mKeywords["shr"] = Token::opShr;
	mKeywords["shl"] = Token::opShl;
	mKeywords["sar"] = Token::opSar;
	mKeywords["and"] = Token::opAnd;

	mKeywords["if"] = Token::kIf;
	mKeywords["then"] = Token::kThen;
	mKeywords["elseif"] = Token::kElseIf;
	mKeywords["else"] = Token::kElse;
	mKeywords["endif"] = Token::kEndIf;
	mKeywords["select"] = Token::kSelect;
	mKeywords["case"] = Token::kCase;
	mKeywords["default"] = Token::kDefault;
	mKeywords["endselect"] = Token::kEndSelect;
	mKeywords["function"] = Token::kFunction;
	mKeywords["return"] = Token::kReturn;
	mKeywords["endfunction"] = Token::kEndFunction;
	mKeywords["type"] = Token::kType;
	mKeywords["field"] = Token::kField;
	mKeywords["endtype"] = Token::kEndType;
	mKeywords["new"] = Token::kNew;
	mKeywords["while"] = Token::kWhile;
	mKeywords["wend"] = Token::kWend;
	mKeywords["repeat"] = Token::kRepeat;
	mKeywords["forever"] = Token::kForever;
	mKeywords["until"] = Token::kUntil;
	mKeywords["goto"] = Token::kGoto;
	mKeywords["gosub"] = Token::kGosub;
	mKeywords["for"] = Token::kFor;
	mKeywords["to"] = Token::kTo;
	mKeywords["each"] = Token::kEach;
	mKeywords["step"] = Token::kStep;
	mKeywords["next"] = Token::kNext;
	mKeywords["dim"] = Token::kDim;
	mKeywords["redim"] = Token::kRedim;
	mKeywords["cleararray"] = Token::kClearArray;
	mKeywords["const"] = Token::kConst;
	mKeywords["global"] = Token::kGlobal;
	mKeywords["data"] = Token::kData;
	mKeywords["read"] = Token::kRead;
	mKeywords["restore"] = Token::kRestore;
	mKeywords["end"] = Token::kEnd;
	mKeywords["integer"] = Token::kInteger;
	mKeywords["float"] = Token::kFloat;
	mKeywords["string"] = Token::kString;
	mKeywords["short"] = Token::kShort;
	mKeywords["byte"] = Token::kByte;
	mKeywords["as"] = Token::kAs;
	mKeywords["exit"] = Token::kExit;

	mKeywords["include"] = Token::kInclude;
}

Lexer::ReturnState Lexer::tokenizeFile(const QString &file) {
	Lexer::ReturnState ret = tokenize(file);
	combineTokens();

	//Dirty trick, btu works
	mTokens.append(Token(Token::EOL, mFiles.first().second.end(), mFiles.first().second.end(), 0, mFiles.first().first));
	mTokens.append(Token(Token::EOL, mFiles.first().second.end(), mFiles.first().second.end(), 0, mFiles.first().first));
	mTokens.append(Token(Token::EndOfTokens, mFiles.first().second.end(), mFiles.first().second.end(), 0, mFiles.first().first));
	return ret;
}

Lexer::ReturnState Lexer::tokenize(const QString &file) {
	emit error(0, "sadas", 0,0);
	QFile *curFile = new QFile(file);
	if (!curFile->open(QFile::ReadOnly | QFile::Text)) {
		delete curFile;
		emit error(ErrorCodes::ecCantOpenFile, tr("Cannot open file %1").arg(file), 0, 0);
		return Error;
	}
	qDebug("File \"%s\" opened", qPrintable(file));
	QString oldPath = QDir::currentPath();
	QFileInfo fi(*curFile);
	bool success = QDir::setCurrent(fi.absolutePath());
	assert(success);

	QTextStream stream(curFile);
	QString code2 = stream.readAll().toLower();
	curFile->close();
	mFiles.append(QPair<QFile*, QString>(curFile, code2));
	const QString &code = mFiles.last().second;

	ReturnState state = Success;
	int line = 1;
	for (QString::const_iterator i = code.begin(); i != code.end();) {
		if (i->category() == QChar::Separator_Space || *i == char(9) /* horizontal tab */) { // Space
			i++;
			continue;
		}
		if (*i == '\'') { //Single line comment
			i++;
			if (i == code.end()) return state;
			readToEOL(i, code.end());
			if (i != code.end()) {
				addToken(Token(Token::EOL, i, ++i, line, curFile));
				line++;
			}
			continue;
		}
		if (*i == '/') {
			i++;
			if (i != code.end()) {
				if (*i == '/') {
					i++;
					readToEOL(i, code.end());
					addToken(Token(Token::EOL, i, i + 1, line, curFile));
					i++;
					line++;
					continue;
				}
			}
			i--;
			addToken(Token(Token::opDivide, i, i + 1, line, curFile));
			i++;
			continue;
		}
		if (*i == '\n') {
			addToken(Token(Token::EOL, i, i + 1, line, curFile));
			i++;
			line++;
			continue;
		}
		if (*i == ',') {
			addToken(Token(Token::Comma, i, i + 1, line, curFile));
			i++;
			continue;
		}
		if (*i == ':') {
			addToken(Token(Token::Colon, i, i + 1, line, curFile));
			i++;
			continue;
		}
		if (*i == '(') {
			addToken(Token(Token::LeftParenthese, i, i + 1, line, curFile));
			i++;
			continue;
		}
		if (*i == ')') {
			addToken(Token(Token::RightParenthese, i, i + 1, line, curFile));
			i++;
			continue;
		}
		if (*i == '"') {
			readString(i, code.end(), line, curFile);
			continue;
		}
		if (*i == '*') {
			addToken(Token(Token::opMultiply, i, i + 1, line, curFile));
			i++;
			continue;
		}
		if (*i == '+') {
			addToken(Token(Token::opPlus, i, i + 1, line, curFile));
			i++;
			continue;
		}
		if (*i == '^') {
			addToken(Token(Token::opPower, i, i + 1, line, curFile));
			i++;
			continue;
		}
		if (*i == '-') {
			addToken(Token(Token::opMinus, i, i + 1, line, curFile));
			i++;
			continue;
		}
		if (*i == '<') {
			QString::const_iterator start = i;
			i++;
			if (*i == '=') {
				addToken(Token(Token::opLessEqual, start, ++i, line, curFile));
				continue;
			}
			if (*i == '>') {
				addToken(Token(Token::opNotEqual, start, ++i, line, curFile));
				continue;
			}
			addToken(Token(Token::opLess, start, i, line, curFile));
			continue;
		}
		if (*i == '>') {
			QString::const_iterator start = i;
			i++;
			if (*i == '=') {
				addToken(Token(Token::opGreaterEqual, start, ++i, line, curFile));
				continue;
			}
			addToken(Token(Token::opGreater, start, i, line, curFile));
			continue;
		}
		if (*i == '=') {
			QString::const_iterator start = i;
			i++;
			if (i != code.end()) {
				if (*i == '>') {
					addToken(Token(Token::opGreaterEqual, start, ++i, line, curFile));
					continue;
				}
				if (*i == '<') {
					addToken(Token(Token::opLessEqual, start, ++i, line, curFile));
					continue;
				}
			}
			addToken(Token(Token::opEqual, start, i, line, curFile));
			continue;
		}
		if (*i == '\\') {
			addToken(Token(Token::opTypePtrField, i, i + 1, line, curFile));
			i++;
			continue;
		}
		if (*i == '.') {
			i++;
			if (i != code.end()) {
				if (i->isNumber()) { //Float
					readFloatDot(i, code.end(), line, curFile);
					continue;
				}
			}
			addToken(Token(Token::opTypePtrType, i - 1, i, line, curFile));
			continue;
		}
		if (*i == '$') {
			i++;
			readHex(i, code.end(), line, curFile);
			continue;
		}
		if (i->isDigit()) {
			readNum(i, code.end(), line, curFile);
			continue;
		}
		if (i->isLetter() || *i == '_') {
			ReturnState retState = readIdentifier(i, code.end(), line, curFile);
			if (retState == Error) return Error;
			if (retState == ErrorButContinue) state = ErrorButContinue;
			continue;
		}

		emit error(ErrorCodes::ecUnexpectedCharacter, tr("Unexpected character \"%1\" %2,%3").arg(QString(*i), QString::number(i->row()), QString::number(i->cell())), line, curFile);
		state = ErrorButContinue;
		++i;
	}

	QDir::setCurrent(oldPath);
	return state;
}

void Lexer::addToken(const Token &tok) {
	//tok.print();
	mTokens.append(tok);
}


Lexer::ReturnState Lexer::readToEOL(QString::const_iterator &i, const QString::const_iterator &end) {
	while (i != end) {
		if (*i == '\n') {
			return Success;
		}
		i++;
	}
	return Success;
}

Lexer::ReturnState Lexer::readToRemEnd(QString::const_iterator &i, const QString::const_iterator &end, int &line, QFile *file) {
	const char * const endRem = "remend";
	int foundIndex = 0;
	while (i != end) {
		if (*i == endRem[foundIndex]) {
			foundIndex++;
			if (foundIndex == 6) {
				i++;
				return Success;
			}
		}
		else {
			foundIndex = 0;
		}
		if (*i == '\n') {
			line++;
		}
		i++;
	}
	emit warning(ErrorCodes::ecExpectingRemEndBeforeEOF, tr("Expecting RemEnd before end of file"), line, file);
	return Lexer::Error;
}

Lexer::ReturnState Lexer::readFloatDot(QString::const_iterator &i, const QString::const_iterator &end, int line, QFile * file) {
	QString::const_iterator begin = i;
	while (i != end) {
		if (!(i->isDigit())) { //Not a number
			break;
		}
		i++;
	}
	if (*i == 'e') {
		i++;
		if (*i == '-' || *i == '+') {
			i++;
		}
		while (i != end) {
			if (!(i->isDigit())) { //Not a number
				break;
			}
			i++;
		}
	}
	addToken(Token(Token::FloatLeadingDot, begin, i, line, file));
	return Success;
}

Lexer::ReturnState Lexer::readNum(QString::const_iterator &i, const QString::const_iterator &end, int line, QFile *file)
{
	QString::const_iterator begin = i;
	while (i != end) {
		if (!(i->isDigit())) { //Not a number
			break;
		}
		i++;
	}
	if (*i == '.') { //Float
		i++;
		while (i != end) {
			if (!(i->isDigit())) { //Not a number
				break;
			}
			i++;
		}
		if (*i == 'e') {
			i++;
			if (*i == '-' || *i == '+') {
				i++;
			}
			while (i != end) {
				if (!(i->isDigit())) { //Not a number
					break;
				}
				i++;
			}
		}
		addToken(Token(Token::Float, begin, i, line, file));
		return Success;
	}

	addToken(Token(Token::Integer, begin, i, line, file));
	return Success;
}


Lexer::ReturnState Lexer::readHex(QString::const_iterator &i, const QString::const_iterator &end, int line, QFile * file)
{
	QString::const_iterator begin = i;
	while (i != end) {
		if (!(i->isDigit() || (*i >= QChar('a') && *i <= QChar('f')))) { //Not hex
			addToken(Token(Token::IntegerHex, begin, i, line, file));
			return Success;
		}
		i++;
	}
	return Success;
}

Lexer::ReturnState Lexer::readString(QString::const_iterator &i, const QString::const_iterator &end, int &line, QFile *file)
{
	++i;
	QString::const_iterator begin = i;
	while (i != end) {
		if (*i == '"') {
			addToken(Token(Token::String, begin, i, line, file));
			i++;
			return Success;
		}
		if (*i == '\n') {
			line++;
		}
		i++;
	}
	error(ErrorCodes::ecExpectingEndOfString, tr("Expecting '\"' before end of file"), line, file);
	return ErrorButContinue;
}

Lexer::ReturnState Lexer::readIdentifier(QString::const_iterator &i, const QString::const_iterator &end, int &line, QFile *file)
{
	QString::const_iterator begin = i;
	QString name;
	name += *i;
	i++;
	while (i != end) {
		if (!i->isLetterOrNumber() && *i != '_') {
			break;
		}
		name += *i;
		i++;
	}
	if (name == "remstart") {
		return readToRemEnd(i, end, line, file);
	}
	QMap<QString, Token::Type>::ConstIterator keyIt = mKeywords.find(name);
	if (keyIt != mKeywords.end()) {
		if (keyIt.value() == Token::kInclude) {
			QString includeFile;
			while (i != end) {
				if (*i == '"') {
					i++;
					while (i != end) {
						if (*i == '"') {
							i++;
							ReturnState state =  tokenize(includeFile);
							return (state == Success) ? Success : ErrorButContinue;
						}
						if (*i == '\n') { //Only for correct line number in error message
							line++;
						}
						includeFile += *i;
						i++;
					}
					emit error(ErrorCodes::ecExpectingEndOfString, tr("Expecting '\"' before end of file"), line, file);
					return ErrorButContinue;
				}
				if (i->category() != QChar::Separator_Space) {
					emit error(ErrorCodes::ecExpectingString, tr("Expecting \" after Include"), line, file);
					return Error;
				}
				i++;
			}
		}
		else {
			addToken(Token(keyIt.value(),begin, i, line, file));
			return Success;
		}
	}
	else {
		addToken(Token(Token::Identifier, begin, i, line, file));
	}

	if (i != end) {
		if (*i == '%') {
			addToken(Token(Token::IntegerTypeMark, i, i + 1, line, file));
			i++;
			return Success;
		}
		if (*i == '#') {
			addToken(Token(Token::FloatTypeMark, i, i + 1, line, file));
			i++;
			return Success;
		}
		if (*i == '$') {
			addToken(Token(Token::StringTypeMark, i, i + 1, line, file));
			i++;
			return Success;
		}
	}
	return Success;
}


void Lexer::combineTokens() {
	QList<Token>::Iterator i = mTokens.begin();
	QList<Token>::Iterator last;
	while (i != mTokens.end()) {
		if (i->mType == Token::kEnd) {
			last = i;
			i++;
			if (i != mTokens.end()) {
				if (i->mType == Token::kFunction) {
					int line = i->mLine;
					QFile *file = i->mFile;
					QString::ConstIterator begin = last->mBegin;
					QString::ConstIterator end = i->mEnd;
					i++;
					i = mTokens.erase(last, i);
					i = mTokens.insert(i, Token(Token::kEndFunction, begin, end, line, file));
					i++;
					continue;
				}
				if (i->mType == Token::kIf) {
					int line = i->mLine;
					QFile *file = i->mFile;
					QString::ConstIterator begin = last->mBegin;
					QString::ConstIterator end = i->mEnd;
					i++;
					i = mTokens.erase(last, i);
					i = mTokens.insert(i, Token(Token::kEndIf, begin, end, line, file));
					i++;
					continue;
				}
				if (i->mType == Token::kSelect) {
					int line = i->mLine;
					QFile *file = i->mFile;
					QString::ConstIterator begin = last->mBegin;
					QString::ConstIterator end = i->mEnd;
					i++;
					i = mTokens.erase(last, i);
					i = mTokens.insert(i, Token(Token::kEndSelect, begin, end, line, file));
					i++;
					continue;
				}
				if (i->mType == Token::kType) {
					int line = i->mLine;
					QFile *file = i->mFile;
					QString::ConstIterator begin = last->mBegin;
					QString::ConstIterator end = i->mEnd;
					i++;
					i = mTokens.erase(last, i);
					i = mTokens.insert(i, Token(Token::kEndType, begin, end, line, file));
					i++;
					continue;
				}
				continue;
			}
			else {
				return;
			}
		}
		if (i->mType == Token::EOL) {
			i++;
			if (i == mTokens.end()) return;
			if (i->mType == Token::Identifier) {
				last = i;
				i++;
				if (i == mTokens.end()) return;
				if (i->mType == Token::Colon) {
					int line = i->mLine;
					QFile *file = i->mFile;
					QString::ConstIterator begin = last->mBegin;
					QString::ConstIterator end = last->mEnd;
					i++;
					i = mTokens.erase(last, i);
					i = mTokens.insert(i, Token(Token::Label, begin, end, line, file));
					i++;
					continue;
				}
			}
			continue;
		}
		i++;

	}
}


void Lexer::printTokens() {
	for (QList<Token>::const_iterator i = mTokens.begin(); i != mTokens.end(); i++) {
		i->print();
	}
}

void Lexer::writeTokensToFile(const QString &fileName)
{
	QFile file(fileName);
	if (!file.open(QFile::WriteOnly | QFile::Text)) {
		emit error(ErrorCodes::ecCantOpenFile, tr("Cannot open file %1").arg(fileName), 0, 0);
		return;
	}
	QTextStream out(&file);
	out << mTokens.size() << " tokens\n\n";
	for (QList<Token>::const_iterator i = mTokens.begin(); i != mTokens.end(); i++) {
		out << i->info() << '\n';
	}
}
