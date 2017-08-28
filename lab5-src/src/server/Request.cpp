#include <unordered_map>
#include <vector>
#include <string>
#include <cstring>
#include <string>
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <stdexcept>

#include "server/Request.hpp"
#include "http/HttpStatus.hpp"
#include "server/TcpConnection.hpp"
#include "Config.hpp"
#include "Utils.hpp"
#include "error/RequestError.hpp"
#include "error/ConnectionError.hpp"
#include "error/TodoError.hpp"
#include <boost/algorithm/string.hpp>


Request::Request(Config const& config, TcpConnection& conn) :
    m_config(config),
    m_conn(conn)
{
    std::string request_line = parse_raw_line();
    parse_method(request_line);
    parse_route(request_line);
    parse_version(request_line);

    // the previous three parse_* calls should consume the entire line
    if (!request_line.empty())
    {
        throw RequestError(HttpStatus::BadRequest, "Malformed request-line\n");
    }

    parse_headers();
    parse_body();
}

void Request::parse_method(std::string& raw_line)
{
    //throw TodoError("2", "You have to implement parsing methods");
	std::string GET = raw_line.substr(0,3);
	if (GET != "GET") {
		throw RequestError(HttpStatus::MethodNotAllowed, "405 Method Not Allowed\n");
	}//cut out GET for here, easy parsing for every methods
	//set m_conn method var to GET
	m_method = GET;
	int len = raw_line.length();
	raw_line = raw_line.substr(4, len);//shouldve started at 4, but overlook the white space with 5
	//white spaces should be omitted for values stored
}

void Request::parse_route(std::string& raw_line)
{
    char c = raw_line[0];
	if (c != '/') {
		throw RequestError(HttpStatus::BadRequest, "400 Bad Request\n");
	}//cut out route, what you just parsed
	//throw TodoError("2", "You have to implement parsing routes");
	std::string result;
	unsigned int i;	
	for (i = 1; i < raw_line.length() && raw_line[i] != ' '; i++) {
		result += raw_line[i];
	}
	m_path = raw_line[0] + result;
	raw_line = raw_line.substr(i + 1, raw_line.length());
}

void Request::parse_querystring(std::string query, std::unordered_map<std::string, std::string>& parsed)
{
    throw TodoError("6", "You have to implement parsing querystrings");
	
}

void Request::parse_version(std::string& raw_line)
{
    //throw TodoError("2", "You have to implement parsing HTTP version");
	//int len = raw_line.length();
	bool found = false;
	std::string ver0 = "HTTP/1.0";
	std::string ver1 = "HTTP/1.1";
	//for (int i = 0; i < (len - 7); i++) {
		//std::string comp = 	raw_line.substr(i, i + 7);	
		if (ver0.compare(raw_line) == 0 || ver1.compare(raw_line) == 0) {
			found = true; 
			m_version = raw_line;			
			//break;
		}
	//}
	if (found == false) {
		throw RequestError(HttpStatus::HttpVersionNotSupported, "505 HTTP Version Not Supported\n");
	}
	//int len = m_version.length();
	raw_line = "";
}

void Request::parse_headers()
{
    //throw TodoError("2", "You have to implement parsing headers");
	std::string line = parse_raw_line();
	std::vector<std::string> vec;
	while (line.length() != 0) {
		//line = parse_raw_line();
		boost::split(vec, line, boost::is_any_of(" "));
		m_headers[vec[0]] = vec[1]; 
		line = parse_raw_line();
	}
}

void Request::parse_body()
{
    if (m_method == "GET") return;

    throw TodoError("6", "You have to implement parsing request bodies");
}

std::string Request::parse_raw_line()
{
    //throw TodoError("2", "You need to implement line fetching");

	unsigned char * c = new unsigned char[1]();
	std::string result;
	m_conn.getc(c);
	for (int i = 0; i < m_max_buf; i++) {
		if (*c == '\r' || *c == '\n') {break;}
		result += *c;
		m_conn.getc(c);
	}
	delete c;
	return result;
}

void Request::print() const noexcept
{
    std::cout << m_method << ' ' << m_path << ' ' << m_version << std::endl;
#ifdef DEBUG    
    for (auto const& el : m_headers)
    {
        std::cout << el.first << ": " << el.second << std::endl;
    }

    for (auto const& el : m_query)
    {
        std::cerr << el.first << ": " << el.second << std::endl;
    }

    for (auto const& el : m_body_data)
    {
        std::cerr << el.first << ": " << el.second << std::endl;
    }
#endif	
}

bool Request::try_header(std::string const& key, std::string& value) const noexcept
{
    if (m_headers.find(key) == m_headers.end())
    {
        return false;
    }
    else
    {
        value = m_headers.at(key);
        return true;
    }
}

std::string const& Request::get_path() const noexcept
{
    return m_path;
}

std::string const& Request::get_method() const noexcept
{
    return m_method;
}

std::string const& Request::get_version() const noexcept
{
    return m_version;
}

std::unordered_map<std::string, std::string> const& Request::get_headers() const noexcept
{
    return m_headers;
}

std::unordered_map<std::string, std::string> const& Request::get_query() const noexcept
{
    return m_query;
}

std::unordered_map<std::string, std::string> const& Request::get_body() const noexcept
{
    return m_body_data;
}
