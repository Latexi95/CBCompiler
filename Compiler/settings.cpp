#include "settings.h"
#include <QSettings>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

Settings::Settings() :
	mFVD(false) {
}

bool Settings::loadDefaults() {


#ifdef _WIN32
	mLoadPath = QCoreApplication::applicationDirPath() + "\\settings_win.ini";
#else
	mLoadPath = QCoreApplication::applicationDirPath() + "/settings_linux.ini";
#endif


	QSettings settings(mLoadPath, QSettings::IniFormat);
	QVariant var = settings.value("compiler/force-variable-declaration");
	if (var.isNull() || !var.canConvert(QMetaType::Bool)) return false;
	mFVD = var.toBool();

	var = settings.value("compiler/default-output-file");
	if (var.isNull() ||  !var.canConvert(QMetaType::QString)) return false;
	mDefaultOutput = var.toString();

	var = settings.value("opt/call");
	if (var.isNull() ||  !var.canConvert(QMetaType::QString)) return false;
	mOpt = var.toString();

	var = settings.value("opt/default-flags");
	if (var.isNull() ||  !var.canConvert(QMetaType::QString)) return false;
	mOptFlags = var.toString();

	var = settings.value("llc/call");
	if (var.isNull() ||  !var.canConvert(QMetaType::QString)) return false;
	mLLC = var.toString();

	var = settings.value("llc/default-flags");
	if (var.isNull() ||  !var.canConvert(QMetaType::QString)) return false;
	mLLCFlags = var.toString();

	var = settings.value("linker/call");
	if (var.isNull() ||  !var.canConvert(QMetaType::QString)) return false;
	mLinker = var.toString();

	var = settings.value("linker/default-flags");
	if (var.isNull() ||  !var.canConvert(QMetaType::QString)) return false;
	mLinkerFlags = var.toString();

	return true;
}

bool Settings::callOpt(const QString &inputFile, const QString &outputFile) const {
	QString p = QDir::currentPath();
	QDir::setCurrent(QCoreApplication::applicationDirPath());
	int ret = QProcess::execute(mOpt.arg(mOptFlags, inputFile, outputFile));
	QDir::setCurrent(p);
	return ret == 0;
}

bool Settings::callLLC(const QString &inputFile, const QString &outputFile) const {
	QString p = QDir::currentPath();
	QDir::setCurrent(QCoreApplication::applicationDirPath());
	int ret = QProcess::execute(mLLC.arg(mLLCFlags, inputFile, outputFile));
	QDir::setCurrent(p);
	return ret == 0;
}

bool Settings::callLinker(const QString &inputFile, const QString &outputFile) const {
	QString p = QDir::currentPath();
	QDir::setCurrent(QCoreApplication::applicationDirPath());
	int ret = QProcess::execute(mLinker.arg(mLinkerFlags, inputFile, "\"" + p + "/" + outputFile + "\""));
	QDir::setCurrent(p);
	return ret == 0;
}
