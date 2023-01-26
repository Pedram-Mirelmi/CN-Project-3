TEMPLATE = app
CONFIG += console c++20
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    DVAlgController.cpp \
    Messages/Packet.cpp \
    Messages/RoutingMessage.cpp \
    Nodes/AbstractNode.cpp \
    Nodes/Host.cpp \
    Nodes/Router.cpp \
    main.cpp

HEADERS += \
    DVAlgController.h \
    Messages/AbstractNetMessage.h \
    Messages/Packet.h \
    Messages/RoutingMessage.h \
    Network.h \
    Nodes/AbstractNode.hpp \
    Nodes/Host.hpp \
    Nodes/Router.hpp \
    concurrentqueue-master/blockingconcurrentqueue.h \
    concurrentqueue-master/c_api/concurrentqueue.h \
    concurrentqueue-master/concurrentqueue.h \
    concurrentqueue-master/internal/concurrentqueue_internal_debug.h \
    concurrentqueue-master/lightweightsemaphore.h \

DISTFILES += \
    concurrentqueue-master/CMakeLists.txt \
    concurrentqueue-master/LICENSE.md \
    concurrentqueue-master/README.md \
