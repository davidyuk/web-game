#include "WebgameServer.hpp"
#include "Router.hpp"
#include "DBConnector.hpp"

#include "Poco/Net/HTTPServer.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/NetException.h"

using namespace std;

void RequestHandler::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) {
    response.setChunkedTransferEncoding(true);
    if (!Router::instance().handle(request, response)) {
        string URI = request.getURI();
        string extension = URI.substr(URI.find_last_of(".") + 1, URI.length());
        try {
            response.sendFile("web" + URI, "text/" + extension);
        }
        catch (const Poco::Exception& e) {
            std::ostream& st = response.send();
            st << e.displayText();
            st.flush();
        }
    }
}

Poco::Net::HTTPRequestHandler* RequestHandlerFactory::createRequestHandler(const Poco::Net::HTTPServerRequest& request) {
    WebgameServer& app = WebgameServer::instance();

    app.logger().information("Request from "
        + request.clientAddress().toString()
        + ": " + request.getMethod()
        + " " + request.getURI()
        + " " + request.getVersion());

#ifdef SHOW_REQUEST_INFORMATION
    for (auto it = request.begin(); it != request.end(); ++it) {
        app.logger().information(it->first + ": " + it->second);
    }
#endif

    return new RequestHandler;
}

WebgameServer::WebgameServer() : _helpRequested(false) {}

WebgameServer::~WebgameServer() {}

void WebgameServer::initialize(Poco::Util::Application& self) {
    loadConfiguration();

    DBConnection::instance().Connect(
        config().getString("database.hostaddr", "localhost"),
        config().getString("database.port", "5432"),
        config().getString("database.dbname", "web-game"),
        config().getString("database.user", "web-game"),
        config().getString("database.password", "web-game")
    );

    ServerApplication::initialize(self);
}

void WebgameServer::uninitialize() {
    ServerApplication::uninitialize();
}

void WebgameServer::defineOptions(Poco::Util::OptionSet& options) {
    ServerApplication::defineOptions(options);

    options.addOption(
        Poco::Util::Option("help", "h", "display help information on command line arguments")
        .required(false)
        .repeatable(false));
}

void WebgameServer::handleOption(const std::string& name, const std::string& value) {
    ServerApplication::handleOption(name, value);

    if (name == "help")
        _helpRequested = true;
}

void WebgameServer::displayHelp() {
    Poco::Util::HelpFormatter helpFormatter(options());
    helpFormatter.setCommand(commandName());
    helpFormatter.setUsage("OPTIONS");
    helpFormatter.setHeader("A sample HTTP server supporting the WebSocket protocol.");
    helpFormatter.format(std::cout);
}

int WebgameServer::main(const std::vector<std::string>& args) {
    if (_helpRequested) {
        displayHelp();
    }

    else {
        Poco::Net::SocketAddress addr(
            config().getString("application.hostaddr", "127.0.0.1"),
            config().getUInt("application.port", 1337)
        );
        Poco::Net::ServerSocket svs(addr);
        Poco::Net::HTTPServer srv(new RequestHandlerFactory, svs, new Poco::Net::HTTPServerParams);
        srv.start();
        logger().information("Web-game server listening on http://" + addr.toString());
        waitForTerminationRequest();
        srv.stop();
    }

    return EXIT_OK;
}
