TEMPLATE = app
CONFIG += console c++20
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    Messages/Packet.cpp \
    Messages/RoutingMessage.cpp \
    Network.cpp \
    NetworkController.cpp \
    Nodes/AbstractNode.cpp \
    Nodes/Host.cpp \
    Nodes/Router.cpp \
#    graphical-representation-of-graphs-in-cpp-master/unweighted.cpp \
#    graphical-representation-of-graphs-in-cpp-master/weighted.cpp \
    TCPConnection.cpp \
    main.cpp

HEADERS += \
    Messages/AbstractNetMessage.h \
    Messages/Packet.h \
    Messages/RoutingMessage.h \
    Network.h \
    NetworkController.h \
    Nodes/AbstractNode.hpp \
    Nodes/Host.hpp \
    Nodes/Router.hpp \
    TCPConnection.h \
    concurrentqueue-master/blockingconcurrentqueue.h \
    concurrentqueue-master/c_api/concurrentqueue.h \
    concurrentqueue-master/concurrentqueue.h \
    concurrentqueue-master/internal/concurrentqueue_internal_debug.h \
    concurrentqueue-master/lightweightsemaphore.h \

DISTFILES += \
    concurrentqueue-master/CMakeLists.txt \
    concurrentqueue-master/LICENSE.md \
    concurrentqueue-master/README.md \
#    graphical-representation-of-graphs-in-cpp-master/README.md \
#    graphical-representation-of-graphs-in-cpp-master/assets/random_weighted_undirected_graph.png
