TARGET = KineticModelBuilder
TEMPLATE = app
QT += core widgets opengl xml concurrent svg

# MACX application bundle
macx: CONFIG += app_bundle

# C++11
CONFIG += c++11

# defines
debug: DEFINES += DEBUG
release: DEFINES += NDEBUG
release: DEFINES += EIGEN_NO_DEBUG

# arch
macx: DEFINES += MACX
linux: DEFINES += LINUX
win32: DEFINES += WIN32
win64: DEFINES += WIN64

# compiler flags
QMAKE_CFLAGS += -O3
QMAKE_CXXFLAGS += -march=native

# Qwt
# !!! On MacOSX you have to manually copy the qwt.framework to Libary/Frameworks folder
#     to avoid getting the "dyld: Library not loaded" error. No idea why this is necessary.
include($$(HOME)/qwt-6.1.2-qt-5.5.0/features/qwt.prf)
CONFIG += qwt

# Eigen
INCLUDEPATH += /usr/local/include/eigen3

# Gnu Scientific Library (GSL)
INCLUDEPATH += /usr/local/Cellar/gsl/1.16/include
LIBS += -L/usr/local/Cellar/gsl/1.16/lib -lgsl -lgslcblas

# source files
SOURCES += main.cpp

#HEADERS += exprtk.hpp

HEADERS += EigenLab.h

HEADERS += ObjectPropertyTreeSerializer.h
SOURCES += ObjectPropertyTreeSerializer.cpp

HEADERS += MarkovModel.h
SOURCES += MarkovModel.cpp
