#include <string>
#include <fstream>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include <climits>
#include <cstdlib>

#include "Config.hpp"
#include "Utils.hpp"
#include "controller/Controller.hpp"
#include "controller/SendFileController.hpp"
#include "http/HttpStatus.hpp"
#include "error/ControllerError.hpp"
#include "error/TodoError.hpp"

SendFileController::SendFileController(Config const& config) :
    Controller(config)
{

}

void SendFileController::run(Request const& req, Response& res) const
{
    std::string open;
    if (!resolve_requested_path(req.get_path(), m_config.static_dir, open)) {
    	return Controller::send_error_response(res, HttpStatus::NotFound, req.get_path() + "is not defined\n");
    }
    std::fstream stream(open, std::fstream::binary | std::fstream::in);
    if (!stream.good())  {
    	return Controller::send_error_response(res, HttpStatus::NotFound, req.get_path() + "is not defined\n");
    }

    size_t len = get_content_length(stream);
    char * buf = new char[len + 1];
    buf[len] = '\0';
    stream.read(buf, len);
    stream.close();

    res.set_header("Content-Length", std::to_string(len));
    res.set_header("Content-Type", get_content_type(open));
    res.set_status(HttpStatus::Ok);

    res.send(buf, len);
    delete[] buf;
	//throw TodoError("4", "You need to implement SendFileController");
}

int SendFileController::get_content_length(std::fstream& fs) const
{
    //throw TodoError("4", "You need to implement getting the length of a file");
	//std::ifstream is(fs, std::ifstream::binary);
	int len = 0;
	if (fs) {
		fs.seekg(0, fs.end);
		len = fs.tellg();
		fs.seekg(0, fs.beg);
	}
	return len;
}

// unfortunately, the only semi-reliable way to get MIME types of files is
// using shell commands xdg-mime and file
// as such, we subshell out to `xdg-mime query filetype <filename>`
std::string SendFileController::get_content_type(std::string const& filename) const
{
    int child_stdout[2];
    if (pipe(child_stdout) == -1)
    {
        throw ControllerError("Could not create pipe to communicate with `xdg-mime`");
    }

    d_printf("Getting content type. child_stdout = {%d, %d}", child_stdout[0], child_stdout[1]);

    pid_t child = fork();
    if (child == -1)
    {
        throw ControllerError("Could not create child process to find MIME type");
    }
    if (child == 0)
    {
        if (dup2(child_stdout[1], STDOUT_FILENO) == -1)
        {
            exit(1);
        }

        execlp("xdg-mime", "xdg-mime", "query", "filetype", filename.c_str(), 0);
        exit(1);
    }

    if (close(child_stdout[1]) == -1)
    {
        throw ControllerError("Could not close pipe input");
    }

    int status;
    if (waitpid(child, &status, 0) == -1)
    {
        throw ControllerError("Error while waiting for finding MIME type to finish");
    }

    if ((WIFEXITED(status) && WEXITSTATUS(status) != 0) || WIFSIGNALED(status))
    {
        throw ControllerError("`xdg-mime` exited with errors");
    }

    size_t const max_content_length = 50;
    char buf[max_content_length];
    memset(buf, 0, max_content_length);

    if (read(child_stdout[0], buf, max_content_length - 1) == -1)
    {
        throw ControllerError("Could not read output of `xdg-mime`");
    }

    if (close(child_stdout[0]) == -1)
    {
        throw ControllerError("Could not close pipe output");
    }

    // remove the trailing newline before returning
    std::string content_type(buf);
    content_type = content_type.substr(0, content_type.size() - 1);

    d_printf("Got content type of %s for %s", content_type.c_str(), filename.c_str());

    return content_type;
}
