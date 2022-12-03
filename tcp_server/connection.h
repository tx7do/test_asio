//
// Created by YLB on 2022/6/6.
//

#ifndef CONNECTION_H
#define CONNECTION_H

#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/thread/recursive_mutex.hpp>

class Connection;
class ServiceBase;
class ServicePort;

typedef boost::shared_ptr<Connection> Connection_ptr;
typedef boost::shared_ptr<ServiceBase> Service_ptr;
typedef boost::shared_ptr<ServicePort> ServicePort_ptr;

class Connection : public boost::enable_shared_from_this<Connection>, boost::noncopyable
{
	friend class ConnectionManager;

public:
	Connection(boost::asio::ip::tcp::socket* socket,
		boost::asio::io_context& ioc,
		ServicePort_ptr service_port);
	~Connection();

	enum { write_timeout = 30 };
	enum { read_timeout = 30 };

	enum ConnectionState_t {
		CONNECTION_STATE_OPEN = 0,
		CONNECTION_STATE_REQUEST_CLOSE = 1,
		CONNECTION_STATE_CLOSING = 2,
		CONNECTION_STATE_CLOSED = 3
	};

	boost::asio::ip::tcp::socket& getHandle();

	void closeConnection();
	// Used by protocols that require server to send first
	void acceptConnection(Protocol* protocol);
	void acceptConnection();

	bool send(OutputMessage_ptr msg);

	uint32_t getIP() const;

	uint32_t addRef();
	uint32_t unRef();

private:
	void parseHeader(const boost::system::error_code& error);
	void parsePacket(const boost::system::error_code& error);

	void onWriteOperation(OutputMessage_ptr msg, const boost::system::error_code& error);

	void onStopOperation();
	void handleReadError(const boost::system::error_code& error);
	void handleWriteError(const boost::system::error_code& error);

	static void handleReadTimeout(boost::weak_ptr<Connection> weak_conn, const boost::system::error_code& error);
	static void handleWriteTimeout(boost::weak_ptr<Connection> weak_conn, const boost::system::error_code& error);

	void closeConnectionTask();
	void deleteConnectionTask();
	void releaseConnection();
	void closeSocket();
	void onReadTimeout();
	void onWriteTimeout();

	void internalSend(OutputMessage_ptr msg);

	NetworkMessage m_msg;
	boost::asio::ip::tcp::socket* m_socket;
	boost::asio::deadline_timer m_readTimer;
	boost::asio::deadline_timer m_writeTimer;
	boost::asio::io_context& m_io_context;
	ServicePort_ptr m_service_port;
	bool m_receivedFirst;
	bool m_writeError;
	bool m_readError;

	int32_t m_pendingWrite;
	int32_t m_pendingRead;
	ConnectionState_t m_connectionState;
	uint32_t m_refCount;
	static bool m_logError;
	boost::recursive_mutex m_connectionLock;

	Protocol* m_protocol;
};

class ConnectionManager
{
public:
	static ConnectionManager* getInstance();

	Connection_ptr createConnection(boost::asio::ip::tcp::socket* socket,
		boost::asio::io_context& io_context, ServicePort_ptr servicers);
	void releaseConnection(Connection_ptr connection);
	void closeAll();

protected:
	std::list<Connection_ptr> m_connections;
	boost::recursive_mutex m_connectionManagerLock;
};


#endif //CONNECTION_H
