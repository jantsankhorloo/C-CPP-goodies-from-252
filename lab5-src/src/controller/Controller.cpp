#include <string>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <limits.h>

#include "Config.hpp"
#include "controller/Controller.hpp"
#include "server/Request.hpp"
#include "server/Response.hpp"
#include "server/TcpConnection.hpp"
#include "http/HttpStatus.hpp"
#include "error/TodoError.hpp"
#include "Utils.hpp"

Controller::Controller(Config const& config) : m_config(config)
{
    
}

Controller::~Controller()
{
    
}

void Controller::send_error_response(Response& res, HttpStatus const& status, std::string response_text)
{
    try
    {
        res.set_status(status);
        res.set_header("Content-Type", "text/plain");
        res.set_header("Content-Length", std::to_string(response_text.size()));
        res.send(response_text.c_str(), response_text.size());
    }
    catch (std::runtime_error const& e)
    {
        d_warn("Could not write error response");
        d_warnf("Error: %s", e.what());
    }
}

void Controller::send_error_response(Config const& config, TcpConnection* conn, HttpStatus const& status, std::string response_text)
{
    try
    {
        Response res(config, *conn);
        res.set_status(status);
        res.set_header("Content-Type", "text/plain");
        res.set_header("Content-Length", std::to_string(response_text.size()));
        res.send(response_text.c_str(), response_text.size());
    }
    catch (std::runtime_error const& e)
    {
        d_warn("Could not write error response");
        d_warnf("Error: %s", e.what());
    }
}

bool Controller::resolve_requested_path(std::string const& requested, std::string const& basedir, std::string& resolved) const noexcept
{
    char resolved_path[PATH_MAX];
    std::string add_route = (requested == std::string("/")) ? requested + "index.html" : requested;
    if (realpath(basedir.c_str(), resolved_path) == NULL) {
    	d_errorf("Error here is: \"%s\"", basedir.c_str());
	return false;
    }
    std::string resolved_base(resolved_path);
    std::string unresolved_full_path(resolved_base + add_route);

    if (realpath(unresolved_full_path.c_str(), resolved_path) == NULL) {
    	d_warnf("another error: %s", requested.c_str());
    	return false;
    }
    std::string got_it(resolved_path);

    if (got_it.compare(0, resolved_base.size(), resolved_base)) {
    	d_warnf("EEror: %s", got_it.c_str());
	return false;
    }

    resolved = got_it;
    return true;
}
