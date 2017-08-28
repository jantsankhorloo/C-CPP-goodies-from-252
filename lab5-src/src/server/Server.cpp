#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include <stdexcept>
#include <signal.h>

#include "Utils.hpp"
#include "server/TcpConnection.hpp"
#include "server/Server.hpp"
#include "server/Request.hpp"
#include "server/Response.hpp"
#include "controller/Controller.hpp"
#include "controller/SendFileController.hpp"
#include "controller/TextController.hpp"
#include "controller/ExecScriptController.hpp"
#include "http/HttpStatus.hpp"
#include "error/RequestError.hpp"
#include "error/ResponseError.hpp"
#include "error/ControllerError.hpp"
#include "error/SocketError.hpp"
#include "error/ConnectionError.hpp"
#include "error/TodoError.hpp"

Server::Server(Config const& config) : m_config(config)
{
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_config.port);
	addr.sin_addr.s_addr = INADDR_ANY;//change to big endian
	m_master = socket(AF_INET, SOCK_STREAM, 0);
	if (m_master < 0) {
		perror("setsocket fail");
		//exit(1);
	}
	
	int enable = 1;
	int sock_opt = setsockopt(m_master, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	if (sock_opt < 0) {
		perror("setsocketopt fail");
		//exit(1);
	}	

	int bind_socket = bind(m_master, (struct sockaddr *) & addr, sizeof(addr));//initialize fields in addr
	if (bind_socket < 0) {
		perror("bind failed");
		//exit(1);
	}
	int listen_socket = listen(m_master, m_config.queue_length);//mconfig qlink
	if (listen_socket < 0) {
		perror("listen failed");
		//exit(1);
	}

    //throw TodoError("2", "Server constructor/connecting to a socket");
}

void Server::run_linear() const
{
    while (true)
    {
        std::string response;
        TcpConnection* conn = new TcpConnection(m_config, m_master);

        handle(conn);

        delete conn;
    }
}

void Server::run_thread_request() const
{
	while (1) {
		TcpConnection * conn = new TcpConnection(m_config, m_master);
		std::thread t([this, conn] () {
			handle(conn);
			delete conn;
		});
		t.detach();
		//delete conn;
	}
}

void Server::run_fork() const
{
	signal(SIGCHLD, SIG_IGN);
	while (1) {
		TcpConnection * conn = new TcpConnection(m_config, m_master);
		pid_t slave = fork();
		if (slave == 0) {
			signal(SIGCHLD, SIG_DFL);
			handle(conn);
			exit(0);		
		}
		delete conn;
	}
	//delete conn;
}

void Server::run_thread_pool() const
{
    throw TodoError("3", "You need to implement thread-pool mode");
	//pthread_t pool[m_config.threads + 1];
	//TcpConnection * conn = new TcpConnection(m_config, m_master);
	//for (int i = 0; i < m_config.threads + 1; i++) {
		//TcpConnection * conn = new TcpConnection(m_config, m_master);
		//pthread_create(&pool[i], NULL, handle, conn);
	//}
	//pthread_join(pool[0], NULL);
}

void Server::handle(TcpConnection* conn) const
{

    Controller const* controller = nullptr;

    try
    {
        // creating req will parse the incoming request
        Request req(m_config, *conn);

        // creating res as an empty response
        Response res(m_config, *conn);

        // Printing the request will be helpful to tell what our server is seeing
        req.print();

        std::string path = req.get_path();

        // This will route a request to the right controller
        // You only need to change this if you rename your controllers or add more routes
        if (path == "/hello-world")
        {
            controller = new TextController(m_config, "Hello world!\n");
        }
        else if (path.find("/script") == 0)
        {
            controller = new ExecScriptController(m_config, "/script");
        }
        else
        {
            controller = new SendFileController(m_config);
        }

        // Whatever controller we picked needs to be run with the given request and response
        controller->run(req, res);
    }
    catch (RequestError const& e)
    {
        d_warnf("Error parsing request: %s", e.what());
        
        Controller::send_error_response(m_config, conn, e.status, "Error while parsing request\n");
    }
    catch (ControllerError const& e)
    {
        d_warnf("Error while handling request: %s", e.what());

        Controller::send_error_response(m_config, conn, HttpStatus::InternalServerError, "Error while handling request\n");
    }
    catch (ResponseError const& e)
    {
        d_warnf("Error while creating response: %s", e.what());

        Controller::send_error_response(m_config, conn, HttpStatus::InternalServerError, "Error while handling response\n");
    }
    catch (ConnectionError const& e)
    {
        // Do not try to write a response when we catch a ConnectionError, because that will likely just throw
        d_errorf("Connection error: %s", e.what());
    }
    catch (TodoError const& e)
    {
        d_errorf("You tried to use unimplemented functionality: %s", e.what());
    }

    // Dont forget about freeing memory!
    delete controller;
}

Server::~Server() noexcept
{
    if (close(m_master) == -1)
    {
        d_error("Could not close master socket");
    }
}
