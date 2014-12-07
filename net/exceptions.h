#ifndef BRICKS_NET_EXCEPTIONS_H
#define BRICKS_NET_EXCEPTIONS_H

#include <exception>

struct NetworkException : std::exception {};

struct SocketException : NetworkException {};

struct SocketCreateException : SocketException {};
struct SocketBindException : SocketException {};
struct SocketListenException : SocketException {};
struct SocketAcceptException : SocketException {};
struct SocketFcntlException : SocketException {};
struct SocketReadException : SocketException {};
struct SocketWriteException : SocketException {};
struct SocketCouldNotWriteEverythingException : SocketWriteException {};

struct HTTPException : NetworkException {};
struct HTTPConnectionClosedByPeerBeforeHeadersWereSentInException : HTTPException {};
struct HTTPNoBodyProvidedException : HTTPException {};
struct HTTPAttemptedToRespondTwiceException : HTTPException {};

#endif  // BRICKS_NET_EXCEPTIONS_H
