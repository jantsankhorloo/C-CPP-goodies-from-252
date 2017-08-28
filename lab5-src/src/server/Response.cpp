#include <string>
#include <cstring>
#include <map>
#include <iostream>
#include "server/Response.hpp"
#include "server/TcpConnection.hpp"
#include "http/HttpStatus.hpp"
#include "error/ResponseError.hpp"
#include "error/TodoError.hpp"
#include "Config.hpp"

Response::Response(Config const& config, TcpConnection& conn) :
    m_config(config),
    m_conn(conn),
    m_headers_sent(false)
{
    // We want every response to have this header
    // It tells browsers that we want separate connections per request
    m_headers["Connection"] = "close";
}

void Response::send(void const* buf, size_t bufsize, bool raw)
{
    //throw TodoError("2", "You need to implement sending responses");
	send_headers();
	m_conn.putbuf(buf, bufsize);
}

void Response::send_headers()
//this method should create and send a string that includes all header information stored inside of m_headers (the format is specified inside the handout) puts()
{
    //throw TodoError("2", "You need to implement sending headers");
	std::string headers;
	for (auto const& x: m_headers) {
		headers = (x.first + ": " + x.second + "\r\n");
		
		//std::cout << headers << std::endl;
		m_conn.puts(headers);
	}
	m_conn.puts("\r\n");
}

void Response::set_header(std::string const& key, std::string const& value)
{
    //throw TodoError("2", "You need to implement controllers setting headers");
	//std::cout << key << " " << value <<std::endl;
	m_headers[key] = (value);
	//m_conn.puts(key + ": " + value + "\r\n");
}

void Response::set_status(HttpStatus const& status)
{
 
 m_status_text = status.to_string();
m_conn.puts("HTTP/1.0 " + m_status_text + "\r\n");
}
