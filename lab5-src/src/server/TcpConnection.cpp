#include <string>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "Utils.hpp"
#include "Config.hpp"
#include "server/TcpConnection.hpp"
#include "error/ConnectionError.hpp"
#include "error/SocketError.hpp"
#include "error/TodoError.hpp"

TcpConnection::TcpConnection(Config const& config, int master_fd) :
    m_config(config),
    m_master(master_fd),
    m_shutdown(false)
{

		m_conn = accept(m_master, NULL, NULL);
		if (m_conn < 0) {
			
		}
    //throw TodoError("2", "You need to implement construction of TcpConnections");
}

TcpConnection::~TcpConnection() noexcept
{
    d_printf("Closing connection on %d", m_conn);

    if (close(m_conn) == -1) d_errorf("Could not close connection %d", m_conn);
}

void TcpConnection::shutdown()
{
    d_printf("Shutting down connection on %d", m_conn);
    
    if (::shutdown(m_conn, SHUT_RDWR) == -1) d_errorf("Could not shut down connection %d", m_conn);

    m_shutdown = true;
}

bool TcpConnection::getc(unsigned char* c)
{
	unsigned char p;
	int result = read(m_conn, &p, sizeof(char)); 
	
	if (result == 0) {
		throw ConnectionError("end of file");
	
	} else if (result < 0) {
		throw ConnectionError("read error");
		return false;	
	}
	*c = p;
	return true;
	
	//throw TodoError("2", "You have to implement reading from connections");
}

void TcpConnection::putc(unsigned char c)
{
	const void * ch = reinterpret_cast<const void *>(c);
	int n = write(m_conn, ch, sizeof(char));
	if (n < 0) {
		throw ConnectionError("connection error");		
	}    
	//throw TodoError("2", "You need to implement writing characters to connections");
}

void TcpConnection::puts(std::string const& str)
{
    const char *mychar = str.c_str();
	int len = strlen(mychar);
	int n = write(m_conn, mychar, len);
	if (n < 0) {
		throw ConnectionError("connection error");	
	}
	//delete mychar;
	//throw TodoError("2", "You need to implement writing strings to connections");
}

void TcpConnection::putbuf(void const* buf, size_t bufsize)
{
	int n = write(m_conn, buf, bufsize);
	if (n < 0) {
		throw ConnectionError("connection error");	
	}
    //throw TodoError("2", "You need to implement writing buffers to connections");
}
