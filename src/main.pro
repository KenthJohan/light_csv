TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
#https://stackoverflow.com/questions/3348711/add-a-define-to-qmake-with-a-value
DEFINES += _WIN32_WINNT=_WIN32_WINNT_WIN10
DEFINES += GIT_VERSION=$$system(git describe --always)
DEFINES += BUILD_DATE='"$(shell date)"'

QMAKE_CFLAGS += -Wno-unused-function
QMAKE_CFLAGS += -Wno-missing-braces
QMAKE_CFLAGS += -Wno-unused-parameter
QMAKE_CFLAGS += -Wno-missing-field-initializers

SOURCES += main.c

INCLUDEPATH += C:/msys64/mingw64/include

LIBS += -liupcontrols -liupcd -liup -lcd -lim -lfreetype -lkernel32 -luser32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lcomctl32
