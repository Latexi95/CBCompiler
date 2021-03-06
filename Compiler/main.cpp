#include <QCoreApplication>
#include <QStringList>
#include "lexer.h"
#include <QDebug>
#include <QTime>
#include "errorhandler.h"
#include "errorcodes.h"
#include "parser.h"
#include <iostream>
#include "codegenerator.h"
#include <QSettings>

int main(int argc, char *argv[]) {
	QCoreApplication a(argc, argv);

	/*QSettings test("test.ini", QSettings::IniFormat);
	test.beginGroup("compiler");
	test.setValue("test-value", "asd");
	test.setValue("other-value", "val");
	test.endGroup();
	test.beginGroup("opt");
	test.setValue("test-value2", "asd2");
	test.setValue("other-value2", "val2");
	test.endGroup();
	test.sync();
	return 0;*/

	QTime bigTimer;
	bigTimer.start();

	QStringList params = a.arguments();
	if (params.size() != 2) {
		qCritical() << "Expecting file to compile";
		return 0;
	}


	ErrorHandler errHandler;

	Settings settings;
	bool success = false;
	success = settings.loadDefaults();
	if (!success) {
		errHandler.error(ErrorCodes::ecSettingsLoadingFailed, errHandler.tr("Loading the default settings \"%1\" failed").arg(settings.loadPath()), CodePoint());
		return ErrorCodes::ecSettingsLoadingFailed;
	}


	Lexer lexer;
	QObject::connect(&lexer, &Lexer::error, &errHandler, &ErrorHandler::error);
	QObject::connect(&lexer, &Lexer::warning, &errHandler, &ErrorHandler::warning);
	QTime timer;
	timer.start();
	if (lexer.tokenizeFile(params[1], settings) == Lexer::Success) {
		qDebug() << "Lexical analysing took " << timer.elapsed() << "ms";
#ifdef DEBUG_OUTPUT
		lexer.writeTokensToFile("tokens.txt");
#endif
	}
	else {
		errHandler.error(ErrorCodes::ecLexicalAnalysingFailed, errHandler.tr("Lexical analysing failed"), CodePoint(lexer.files().first().first));
		return ErrorCodes::ecLexicalAnalysingFailed;
	}

	Parser parser;
	QObject::connect(&parser, &Parser::error, &errHandler, &ErrorHandler::error);
	QObject::connect(&parser, &Parser::warning, &errHandler, &ErrorHandler::warning);

	timer.start();
	ast::Program *program = parser.parse(lexer.tokens(), settings);
	qDebug() << "Parsing took " << timer.elapsed() << "ms";
	if (!parser.success()) {
		errHandler.error(ErrorCodes::ecParsingFailed, errHandler.tr("Parsing failed \"%1\"").arg(params[1]), CodePoint());
		return ErrorCodes::ecParsingFailed;
	}

#ifdef DEBUG_OUTPUT
	if (program) {
		QFile file("ast.txt");
		if (file.open(QFile::WriteOnly)) {
			QTextStream stream(&file);
			program->write(stream);
			file.close();
		}
	}
#endif



	CodeGenerator codeGenerator;
	//QObject::connect(&codeGenerator, &CodeGenerator::error, &errHandler, &ErrorHandler::error);
	//QObject::connect(&codeGenerator, &CodeGenerator::warning, &errHandler, &ErrorHandler::warning);
	QObject::connect(&codeGenerator, &CodeGenerator::error, &errHandler, &ErrorHandler::error);
	QObject::connect(&codeGenerator, &CodeGenerator::warning, &errHandler, &ErrorHandler::warning);

	timer.start();
	if (!codeGenerator.initialize(settings)) {
		return ErrorCodes::ecCodeGeneratorInitializationFailed;
	}


	qDebug() << "Code generator initialization took " << timer.elapsed() << "ms";
	timer.start();
	if (!codeGenerator.generate(program)) {
		errHandler.error(ErrorCodes::ecCodeGenerationFailed, errHandler.tr("Code generation failed"), CodePoint());
		return ErrorCodes::ecCodeGenerationFailed;
	}

	qDebug() << "Code generation took" << timer.elapsed() << "ms";
	qDebug() << "LLVM-IR generated";

	timer.start();
	codeGenerator.createExecutable(settings.defaultOutputFile());
	qDebug() << "Executable generation took " << timer.elapsed() << "ms";
	qDebug() << "The whole compilation took " << bigTimer.elapsed() << "ms";
	return 0;
}
