/*
 * NetMain.cpp
 *
 *  Created on: Apr 03, 2016
 *      Author: z.j
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <string>

#include "NetLogger.h"
#include "ReactorThread.h"
#include "EpollSocketSet.h"
#include "Socket.h"
#include "DataBuffer.h"
#include "SctpSocket.h"
#include "Util.h"
#include "TcpServerSocketEventHandlerTest.h"
#include "TcpServer.h"
#include "Worker.h"
#include "Reactor.h"

using namespace std;
using namespace net;
using namespace cm;

void showUsage() {
    cout << "Usage: " << endl;
    cout << "net [Test Number]" << endl;
    cout << "  1 : Test ReactorThread" << endl;
    cout << "  2 : Test EpollSocketSet" << endl;
    cout << "  3 : Test Socket" << endl;
#ifdef SCTP_SUPPORT
    cout << "  4 : Test SctpSocket" << endl;
#endif
    cout << "  5 : Test TcpServer" << endl;
}

void testReactorThread(string ip, short port);
void testEpollSocketSet(string ip);
void testSocket(string ip);
void testSctpSocket(string ip);
void testTcpServer(string ip, short port);
int main(int argc, char* argv[]) {

    if (argc < 2) {
        showUsage();
        return 0;
    }

    string testNumber(argv[1]);
    cout << "testNumber = " << testNumber << endl;

    // NETLogger::initConsoleLog();
    // NETLogger::setLogLevel(cm::INFO);
    string ip("127.0.0.1");
    if (argc > 2) {
        ip = argv[2];
    }

    short port = 8080;
    if (argc > 3) {
        port = Util::s2i(argv[3]);
    }

    if (testNumber.compare("1") == 0) {
        testReactorThread(ip, port);
    } else if (testNumber.compare("2") == 0) {
        testEpollSocketSet(ip);
    } else if (testNumber.compare("3") == 0) {
        testSocket(ip);
    } else if (testNumber.compare("4") == 0) {
        testSctpSocket(ip);
    } else if (testNumber.compare("5") == 0) {
        testTcpServer(ip, port);
    } else {
        showUsage();
    }

    cout << "exit main " << getpid() << endl;
    return 0;
}

// ---------------------------------------------
void testReactorThread(string ip, short port) {
    ReactorThread* reactorThread = new ReactorThread();
    reactorThread->start();

    cm::Thread::sleep(1000);

    Socket* socket = new Socket(ip, port);
    socket->bind();
    socket->listen();  
    socket->makeNonBlocking();

    TcpServerSocketEventHandlerTest* tcpServerHandler = new TcpServerSocketEventHandlerTest(reactorThread);
    reactorThread->registerInputHandler(socket, tcpServerHandler);

    reactorThread->wait();

    delete reactorThread;
}

// ---------------------------------------------
void testTcpServer(string ip, short port) {
    // only create some reactor and worker threads for test
    // no need to call below 2 initialize function if you want to 
    // use default configurations
    Worker::initialize(2);
    Reactor::initialize(2);
    
    TcpServer* tcpServer = new TcpServer(0, port);
    tcpServer->listen();

    while (tcpServer->isRunning()) {
        sleep(10);
    }
}

// ---------------------------------------------
void testEpollSocketSet(string ip) {
    Socket* socket = new Socket(ip, 8080);
    socket->bind();
    socket->listen();  
    socket->makeNonBlocking();

    EpollSocketSet epollSocketSet;
    epollSocketSet.registerInputHandler(socket, 0);

    EpollSocketSet::EpollSocket* epollSocket;
    cout << "start to poll the socket" << endl;
    while (true) {
        epollSocket = epollSocketSet.poll(0);

        if (epollSocket != 0 && (epollSocket->events & EPOLLIN)) {
            if (epollSocket->socket->getSocket() == socket->getSocket()) {
                Socket::InetAddressPort remoteAddrAndPort;
                int newSocketFd = -1;

                int result = epollSocket->socket->accept(newSocketFd, remoteAddrAndPort);
                assert(result != SKT_ERR);
                if (result == SKT_SUCC) {
                    Socket* newSocket = new Socket(newSocketFd);
                    newSocket->makeNonBlocking();
                    epollSocketSet.registerInputHandler(newSocket, 0);
                }
            } else {
                DataBuffer* recvBuf = new DataBuffer();
                int numOfBytesRecved = 0;

                int result = epollSocket->socket->recv(recvBuf->getEndOfDataPointer(), recvBuf->getSize() - recvBuf->getLength(), numOfBytesRecved);
                assert(result != SKT_ERR);
                if (result == SKT_SUCC) {
                    if (numOfBytesRecved == 0) {
                        cout << "socket is disconnected by peer: " << epollSocket->socket->getSocket() << endl;
                        // close socket in server side
                        epollSocketSet.removeInputHandler(epollSocket->socket);
                        epollSocket->socket->close();
                        delete epollSocket->socket;
                    } else {
                        recvBuf->increaseDataLength(numOfBytesRecved);
                        cout << "receive " << numOfBytesRecved << " bytes data from socket: " << recvBuf->getData() << endl;
                        cout << "buffer length: " << recvBuf->getLength() << endl;

                        char respData[] = "TCP server response";
                        int numOfBytesSent;
                        assert(SKT_SUCC == epollSocket->socket->send(respData, strlen(respData), numOfBytesSent));
                        assert(numOfBytesSent == strlen(respData));
                        cout << "send " << numOfBytesSent << " bytes data to socket: " << respData << endl;
                    }
                    recvBuf->reset();
                } else {
                    cout << "no data, continue to read from socket: " << epollSocket->socket->getSocket() << endl;
                }
            }
        } else {
            cout << "no available read events" << endl;
            cm::Thread::sleep(1000);
        }
    }
}

// ---------------------------------------------
void testSocket(string ip) {
    Socket* socket = new Socket(ip, 8080);
    socket->bind();
    socket->listen();

    Socket::InetAddressPort remoteAddrAndPort;
    int newSocketFd = -1;

    // Test non-blocking socket
    socket->makeNonBlocking();
    while (true) {
        bool result = socket->accept(newSocketFd, remoteAddrAndPort);
        if (result) {
            Socket* newSocket = new Socket(newSocketFd);
            newSocket->makeNonBlocking();
            DataBuffer* recvBuf = new DataBuffer();

            int numOfBytesRecved = 0;

            while (true) {
                int result = newSocket->recv(recvBuf->getEndOfDataPointer(), recvBuf->getSize() - recvBuf->getLength(), numOfBytesRecved);
                assert(result != SKT_ERR);
                if (result == SKT_SUCC) {
                    if (numOfBytesRecved == 0) {
                        newSocket->close();
                        delete newSocket;
                        newSocket = 0;
                        cout << "socket is disconnected by peer: " << endl;
                        break;
                    }

                    recvBuf->increaseDataLength(numOfBytesRecved);
                    cout << "receive " << numOfBytesRecved << " bytes data from socket: " << recvBuf->getData() << endl;
                    cout << "buffer length: " << recvBuf->getLength() << endl;

                    char respData[] = "TCP server response";
                    int numOfBytesSent;
                    assert(SKT_SUCC == newSocket->send(respData, strlen(respData), numOfBytesSent));
                    assert(numOfBytesSent == strlen(respData));
                    cout << "send " << numOfBytesSent << " bytes data to socket: " << respData << endl;

                    recvBuf->reset();
                } else {
                    cm::Thread::sleep(1000);
                }
            }

            // stop the non-blocking test after one client connected complete
            break;
        } else {
            cm::Thread::sleep(1000);
        }
    }

    // Test blocking socket
    socket->makeBlocking();
    bool result = socket->accept(newSocketFd, remoteAddrAndPort);
    assert(result);
    Socket* newSocket = new Socket(newSocketFd);
    DataBuffer* recvBuf = new DataBuffer();

    int numOfBytesRecved = 0;

    while (true) {
        int result = newSocket->read(recvBuf->getEndOfDataPointer(), recvBuf->getSize() - recvBuf->getLength(), numOfBytesRecved);
        assert(result != SKT_ERR);
        if (result == SKT_SUCC) {
            if (numOfBytesRecved == 0) {
                newSocket->close();
                delete newSocket;
                newSocket = 0;
                cout << "socket is disconnected by peer: " << endl;
                break;
            }

            recvBuf->increaseDataLength(numOfBytesRecved);
            cout << "receive " << numOfBytesRecved << " bytes data from socket: " << recvBuf->getData() << endl;
            cout << "buffer length: " << recvBuf->getLength() << endl;

            char respData[] = "TCP server response";
            int numOfBytesSent;
            assert(SKT_SUCC == newSocket->send(respData, strlen(respData), numOfBytesSent));
            assert(numOfBytesSent == strlen(respData));
            cout << "send " << numOfBytesSent << " bytes data to socket: " << respData << endl;

            recvBuf->reset();
        } else {
            cm::Thread::sleep(1);
        }
    }
}

// --------------------------------------------------
void testSctpSocket(string ip) {
#ifdef SCTP_SUPPORT
    SctpSocket* socket = new SctpSocket(ip, 62324);
    socket->bind();
    socket->listen(5);

    Socket::InetAddressPort remoteAddrAndPort;
    int newSocketFd = -1;
    bool result = socket->accept(newSocketFd, remoteAddrAndPort);
    assert(result);
    SctpSocket* newSocket = new SctpSocket(newSocketFd);
    DataBuffer* recvBuf = new DataBuffer();
    int numOfBytesRecved = 0;

    while (true) {
        int result = newSocket->recv(recvBuf->getEndOfDataPointer(), recvBuf->getSize() - recvBuf->getLength(), numOfBytesRecved);
        assert(result != SKT_ERR);
        if (result == SKT_SUCC) {
            if (numOfBytesRecved == 0) {
                newSocket->close();
                delete newSocket;
                newSocket = 0;
                cout << "socket is disconnected by peer: " << endl;
                break;
            }

            recvBuf->increaseDataLength(numOfBytesRecved);
            cout << "receive " << numOfBytesRecved << " bytes data from socket: " << recvBuf->getData() << endl;
            cout << "buffer length: " << recvBuf->getLength() << endl;

            recvBuf->reset();
        } else {
            cm::Thread::sleep(1);
        }
    }   
#endif 
}
