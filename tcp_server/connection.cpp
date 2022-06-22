//
// Created by YLB on 2022/6/6.
//

#include "connection.h"

#include <boost/asio/placeholders.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>

#include "singleton.h"

bool Connection::m_logError = true;

Connection::Connection(boost::asio::ip::tcp::socket* socket,
	boost::asio::io_service& io_service,
	ServicePort_ptr service_port)
	: m_socket(socket), m_readTimer(io_service), m_writeTimer(io_service), m_io_service(io_service), m_service_port(
	service_port)
{
	m_refCount = 0;
	m_protocol = NULL;
	m_pendingWrite = 0;
	m_pendingRead = 0;
	m_connectionState = CONNECTION_STATE_OPEN;
	m_receivedFirst = false;
	m_writeError = false;
	m_readError = false;
}

Connection::~Connection()
{
}

boost::asio::ip::tcp::socket& Connection::getHandle()
{
	return *m_socket;
}

void Connection::closeConnection()
{
	boost::recursive_mutex::scoped_lock lockClass(m_connectionLock);
	if (m_connectionState == CONNECTION_STATE_CLOSED || m_connectionState == CONNECTION_STATE_REQUEST_CLOSE)
		return;

	m_connectionState = CONNECTION_STATE_REQUEST_CLOSE;

	g_dispatcher.addTask(
		createTask(boost::bind(&Connection::closeConnectionTask, this)));
}

void Connection::closeConnectionTask()
{
	m_connectionLock.lock();
	if (m_connectionState != CONNECTION_STATE_REQUEST_CLOSE)
	{
		std::cout << "Error: [Connection::closeConnectionTask] m_connectionState = " << m_connectionState << std::endl;
		m_connectionLock.unlock();
		return;
	}

	if (m_protocol)
	{
		m_protocol->setConnection(Connection_ptr());
		m_protocol->releaseProtocol();
		m_protocol = NULL;
	}

	m_connectionState = CONNECTION_STATE_CLOSING;

	if (m_pendingWrite == 0 || m_writeError)
	{
		closeSocket();
		releaseConnection();
		m_connectionState = CONNECTION_STATE_CLOSED;
	}
	else
	{
		//will be closed by onWriteOperation/handleWriteTimeout/handleReadTimeout instead
	}

	m_connectionLock.unlock();
}

void Connection::closeSocket()
{
	m_connectionLock.lock();

	if (m_socket->is_open())
	{
		m_pendingRead = 0;
		m_pendingWrite = 0;

		try
		{
			boost::system::error_code error;
			m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
			if (error)
			{
				if (error == boost::asio::error::not_connected)
				{
					//Transport endpoint is not connected.
				}
				else
				{
					PRINT_ASIO_ERROR("Shutdown");
				}
			}
			m_socket->close(error);

			if (error)
			{
				PRINT_ASIO_ERROR("Close");
			}
		}
		catch (boost::system::system_error& e)
		{
			if (m_logError)
			{
				m_logError = false;
			}
		}
	}

	m_connectionLock.unlock();
}

void Connection::releaseConnection()
{
	if (m_refCount > 0)
	{
		//Reschedule it and try again.
		g_scheduler.addEvent(createSchedulerTask(SCHEDULER_MINTICKS,
			boost::bind(&Connection::releaseConnection, this)));
	}
	else
	{
		deleteConnectionTask();
	}
}

void Connection::onStopOperation()
{
	//io_service thread
	m_connectionLock.lock();
	m_readTimer.cancel();
	m_writeTimer.cancel();

	try
	{
		if (m_socket->is_open())
		{
			boost::system::error_code error;
			m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
			m_socket->close();
		}
	}
	catch (boost::system::system_error&)
	{
		//
	}

	delete m_socket;
	m_socket = nullptr;

	m_connectionLock.unlock();
	ConnectionManager::getInstance()->releaseConnection(shared_from_this());
}

void Connection::deleteConnectionTask()
{
	//dispather thread
	assert(m_refCount == 0);
	try
	{
		m_io_service.dispatch(boost::bind(&Connection::onStopOperation, this));
	}
	catch (boost::system::system_error& e)
	{
		if (m_logError)
		{
			m_logError = false;
		}
	}
}

void Connection::acceptConnection(Protocol* protocol)
{
	m_protocol = protocol;
	m_protocol->onConnect();

	acceptConnection();
}

void Connection::acceptConnection()
{
	try
	{
		++m_pendingRead;
		m_readTimer.expires_from_now(boost::posix_time::seconds(Connection::read_timeout));
		m_readTimer.async_wait(boost::bind(&Connection::handleReadTimeout,
			boost::weak_ptr<Connection>(shared_from_this()),
			boost::asio::placeholders::error));

		// Read size of the first packet
		boost::asio::async_read(getHandle(),
			boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::header_length),
			boost::bind(&Connection::parseHeader, shared_from_this(), boost::asio::placeholders::error));
	}
	catch (boost::system::system_error& e)
	{
		if (m_logError)
		{
			m_logError = false;
			closeConnection();
		}
	}
}

void Connection::parseHeader(const boost::system::error_code& error)
{
	m_connectionLock.lock();
	m_readTimer.cancel();

	int32_t size = m_msg.decodeHeader();
	if (error || size <= 0 || size >= NETWORKMESSAGE_MAXSIZE - 16)
	{
		handleReadError(error);
	}

	if (m_connectionState != CONNECTION_STATE_OPEN || m_readError)
	{
		closeConnection();
		m_connectionLock.unlock();
		return;
	}

	--m_pendingRead;

	try
	{
		++m_pendingRead;
		m_readTimer.expires_from_now(boost::posix_time::seconds(Connection::read_timeout));
		m_readTimer
			.async_wait(boost::bind(&Connection::handleReadTimeout, boost::weak_ptr<Connection>(shared_from_this()),
				boost::asio::placeholders::error));

		// Read packet content
		m_msg.setMessageLength(size + NetworkMessage::header_length);
		boost::asio::async_read(getHandle(), boost::asio::buffer(m_msg.getBodyBuffer(), size),
			boost::bind(&Connection::parsePacket, shared_from_this(), boost::asio::placeholders::error));
	}
	catch (boost::system::system_error& e)
	{
		if (m_logError)
		{
			m_logError = false;
			closeConnection();
		}
	}

	m_connectionLock.unlock();
}

void Connection::parsePacket(const boost::system::error_code& error)
{
	m_connectionLock.lock();
	m_readTimer.cancel();

	if (error)
	{
		handleReadError(error);
	}

	if (m_connectionState != CONNECTION_STATE_OPEN || m_readError)
	{
		closeConnection();
		m_connectionLock.unlock();
		return;
	}

	--m_pendingRead;

	//Check packet checksum
	uint32_t recvChecksum = m_msg.PeekU32();
	uint32_t checksum = 0;
	int32_t len = m_msg.getMessageLength() - m_msg.getReadPos() - 4;
	if (len > 0)
	{
		checksum = adlerChecksum((uint8_t*)(m_msg.getBuffer() + m_msg.getReadPos() + 4), len);
	}

	if (recvChecksum == checksum)
		// remove the checksum
		m_msg.GetU32();

	if (!m_receivedFirst)
	{
		m_receivedFirst = true;
		// First message received
		if (!m_protocol)
		{ // Game protocol has already been created at this point
			m_protocol = m_service_port->make_protocol(recvChecksum == checksum, m_msg);
			if (!m_protocol)
			{
				closeConnection();
				m_connectionLock.unlock();
				return;
			}
			m_protocol->setConnection(shared_from_this());
		}
		else
		{
			// Skip protocol ID
			m_msg.GetByte();
		}
		m_protocol->onRecvFirstMessage(m_msg);
	}
	else
	{
		// Send the packet to the current protocol
		m_protocol->onRecvMessage(m_msg);
	}

	try
	{
		++m_pendingRead;
		m_readTimer.expires_from_now(boost::posix_time::seconds(Connection::read_timeout));
		m_readTimer
			.async_wait(boost::bind(&Connection::handleReadTimeout, boost::weak_ptr<Connection>(shared_from_this()),
				boost::asio::placeholders::error));

		// Wait to the next packet
		boost::asio::async_read(getHandle(),
			boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::header_length),
			boost::bind(&Connection::parseHeader, shared_from_this(), boost::asio::placeholders::error));
	}
	catch (boost::system::system_error& e)
	{
		if (m_logError)
		{
			m_logError = false;
			closeConnection();
		}
	}

	m_connectionLock.unlock();
}

bool Connection::send(OutputMessage_ptr msg)
{
	m_connectionLock.lock();
	if (m_connectionState != CONNECTION_STATE_OPEN || m_writeError)
	{
		m_connectionLock.unlock();
		return false;
	}

	if (m_pendingWrite == 0)
	{
		msg->getProtocol()->onSendMessage(msg);
		internalSend(msg);
	}
	else
	{
		OutputMessagePool* outputPool = OutputMessagePool::getInstance();
		outputPool->addToAutoSend(msg);
	}

	m_connectionLock.unlock();
	return true;
}

void Connection::internalSend(OutputMessage_ptr msg)
{
	TRACK_MESSAGE(msg);

	try
	{
		++m_pendingWrite;
		m_writeTimer.expires_from_now(boost::posix_time::seconds(Connection::write_timeout));
		m_writeTimer
			.async_wait(boost::bind(&Connection::handleWriteTimeout, boost::weak_ptr<Connection>(shared_from_this()),
				boost::asio::placeholders::error));

		boost::asio::async_write(getHandle(),
			boost::asio::buffer(msg->getOutputBuffer(), msg->getMessageLength()),
			boost::bind(&Connection::onWriteOperation, shared_from_this(), msg, boost::asio::placeholders::error));
	}
	catch (boost::system::system_error& e)
	{
		if (m_logError)
		{
			m_logError = false;
		}
	}
}

uint32_t Connection::getIP() const
{
	//Ip is expressed in network byte order
	boost::system::error_code error;
	const boost::asio::ip::tcp::endpoint endpoint = m_socket->remote_endpoint(error);
	if (!error)
	{
		return htonl(endpoint.address().to_v4().to_ulong());
	}
	else
	{
		return 0;
	}
}

uint32_t Connection::addRef()
{
	return ++m_refCount;
}

uint32_t Connection::unRef()
{
	return --m_refCount;
}

void Connection::onWriteOperation(OutputMessage_ptr msg, const boost::system::error_code& error)
{
	m_connectionLock.lock();
	m_writeTimer.cancel();

	TRACK_MESSAGE(msg);
	msg.reset();

	if (error)
	{
		handleWriteError(error);
	}

	if (m_connectionState != CONNECTION_STATE_OPEN || m_writeError)
	{
		closeSocket();
		closeConnection();
		m_connectionLock.unlock();
		return;
	}

	--m_pendingWrite;
	m_connectionLock.unlock();
}

void Connection::handleReadError(const boost::system::error_code& error)
{
	boost::recursive_mutex::scoped_lock lockClass(m_connectionLock);

	if (error == boost::asio::error::operation_aborted)
	{
		//Operation aborted because connection will be closed
		//Do NOT call closeConnection() from here
	}
	else if (error == boost::asio::error::eof)
	{
		//No more to read
		closeConnection();
	}
	else if (error == boost::asio::error::connection_reset ||
		error == boost::asio::error::connection_aborted)
	{
		//Connection closed remotely
		closeConnection();
	}
	else
	{
		closeConnection();
	}
	m_readError = true;
}

void Connection::onReadTimeout()
{
	boost::recursive_mutex::scoped_lock lockClass(m_connectionLock);

	if (m_pendingRead > 0 || m_readError)
	{
		closeSocket();
		closeConnection();
	}
}

void Connection::onWriteTimeout()
{
	boost::recursive_mutex::scoped_lock lockClass(m_connectionLock);

	if (m_pendingWrite > 0 || m_writeError)
	{
		closeSocket();
		closeConnection();
	}
}

void Connection::handleReadTimeout(boost::weak_ptr<Connection> weak_conn, const boost::system::error_code& error)
{
	if (error != boost::asio::error::operation_aborted)
	{
		if (weak_conn.expired())
		{
			return;
		}

		if (boost::shared_ptr<Connection> connection = weak_conn.lock())
		{
			connection->onReadTimeout();
		}
	}
}

void Connection::handleWriteError(const boost::system::error_code& error)
{
	boost::recursive_mutex::scoped_lock lockClass(m_connectionLock);

	if (error == boost::asio::error::operation_aborted)
	{
		//Operation aborted because connection will be closed
		//Do NOT call closeConnection() from here
	}
	else if (error == boost::asio::error::eof)
	{
		//No more to read
		closeConnection();
	}
	else if (error == boost::asio::error::connection_reset ||
		error == boost::asio::error::connection_aborted)
	{
		//Connection closed remotely
		closeConnection();
	}
	else
	{
		closeConnection();
	}
	m_writeError = true;
}

void Connection::handleWriteTimeout(boost::weak_ptr<Connection> weak_conn, const boost::system::error_code& error)
{
	if (error != boost::asio::error::operation_aborted)
	{
		if (weak_conn.expired())
		{
			return;
		}

		if (boost::shared_ptr<Connection> connection = weak_conn.lock())
		{
			connection->onWriteTimeout();
		}
	}
}

