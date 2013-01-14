#-------------------------------------------------
#
# Project created by QtCreator 2012-05-25T15:50:57
#
#-------------------------------------------------

QT       -= core gui

TARGET = Runtime
TEMPLATE = lib
CONFIG += staticlib
SOURCES += \
    system.cpp \
    cbstring.cpp \
    main.cpp \
    referencecounter.cpp

HEADERS += \
    cbstring.h \
    referencecounter.h

LLVM_FILES += atomic_operations.ll


QMAKE_CC = clang
QMAKE_CXX = clang++
QMAKE_CFLAGS = -emit-llvm
QMAKE_CXXFLAGS = -emit-llvm
QMAKE_LIB = llvm-link -o

QMAKE_RUN_CC		= $(CC) $(CCFLAGS) $(INCPATH) -c $src -o $obj
QMAKE_RUN_CC_IMP	= $(CC) $(CCFLAGS) $(INCPATH) -c $< -o $@
QMAKE_RUN_CXX		= $(CXX) $(CXXFLAGS) $(INCPATH) -c $src -o $obj
QMAKE_RUN_CXX_IMP	= $(CXX) $(CXXFLAGS) $(INCPATH) -c $< -o $@

QMAKE_EXT_OBJ           = .bc
QMAKE_EXT_RES           = _res.bc

QMAKE_PREFIX_STATICLIB  = lib
QMAKE_EXTENSION_STATICLIB = bc

llvm_compiler.output  = ${QMAKE_FILE_BASE}.bc
llvm_compiler.commands = llvm-as ${QMAKE_FILE_NAME} -o ${QMAKE_FILE_OUT}
llvm_compiler.input = LLVM_FILES
QMAKE_EXTRA_COMPILERS += llvm_compiler


INCLUDEPATH += "$$(BOOST_INCLUDE)"


QMAKE_CXXFLAGS += -std=c++0x

OTHER_FILES += \
    atomic_operations.ll
