/* We simply call the root header file "App.h", giving you uWS::App and uWS::SSLApp */
#include "App.h"
#include <time.h>
#include <iostream>
#include <thread>
#include <algorithm>
#include <mutex>
#include <queue>
#include <unordered_set>
#include <atomic>

#include <pybind11/pybind11.h>
namespace py = pybind11;

/* Helpers for this example */
#include "helpers/AsyncFileReader.h"
#include "helpers/AsyncFileStreamer.h"
#include "helpers/Middleware.h"

// run server in separate thread...DONE
// add mime types compatibility...DONE
// make http_server example server serve files...DONE
// move the code from http_server to broadcast...DONE


// have a outbound queue that's shared between threads...DONE
// have server read from queue every 1ms and if there is data, to broadcast the data out...DONE
// have the server receive `message` from websocket and put that in inbound queue...DONE

// Pybind the server start (with port, ssl options, other options like maxpayloadlength, backpressure, timeout, compression...DONE
// write a server stop and pybind it.......DONE


// function to push json onto outbound queue
// function to get json from inbound queue

// function to push name + numpy array to outbound queue

// json streaming of data, directly from Python


// does json encoding need it's own thread?


static std::mutex outboundMutex;
static std::queue<std::pair<std::string, std::string> > outbound;
static std::mutex inboundMutex;
static std::queue<std::pair<std::string, std::string> > inbound;
static std::thread *web_thread;
static std::atomic<bool> exit_flag(false);

static thread_local struct us_listen_socket_t *globalListenSocket;

struct PerSocketData {
  /* Fill with user data */
};

thread_local std::unordered_set<uWS::WebSocket<true, true, PerSocketData> * > clients;


void web_server_func(const int port=8000,
		     const std::string root=".",
		     const uWS::CompressOptions compression=uWS::DISABLED,
		     const uint32_t max_payload_length=16 * 1024 * 1024,
		     const uint16_t idle_timeout=16,
		     const uint32_t max_backpressure=1 * 1024 * 1024,
		     const bool close_on_backpressure_limit=false,
		     const bool reset_idle_timeout_on_send=false,
		     const bool send_pings_automatically=true) {
  /* Keep in mind that uWS::SSLApp({options}) is the same as uWS::App() when compiled without SSL support.
   * You may swap to using uWS:App() if you don't need SSL */

  // todo SSL options
  uWS::SSLApp app = uWS::SSLApp();

  app.ws<PerSocketData>("/*", {
      /* Settings */
      .compression = compression,  // uWS::SHARED_COMPRESSOR,
      .maxPayloadLength = max_payload_length,
      .idleTimeout = idle_timeout,
      .maxBackpressure = max_backpressure,
      .closeOnBackpressureLimit = close_on_backpressure_limit,
      .resetIdleTimeoutOnSend = reset_idle_timeout_on_send,
      .sendPingsAutomatically = send_pings_automatically,
      /* Handlers */
      .upgrade = nullptr,
      .open = [](auto *ws) {
	std::cout << "Added client " << ws << std::endl;
	clients.insert(ws);
      },
      .message = [](auto */*ws*/, std::string_view message, uWS::OpCode /*opCode*/) {
	std::lock_guard<std::mutex> lock(inboundMutex);
	inbound.push(std::make_pair("foo", std::string(message)));
      },
      .close = [](auto *ws, int /*code*/, std::string_view /*message*/) {
	std::cout << "Removed client " << ws << std::endl;
	clients.erase(ws);
      }
    });


  AsyncFileStreamer asyncFileStreamer(root);
  app.get("/*", [&asyncFileStreamer](auto *res, auto *req) {
    auto url = req->getUrl();
    if (url == "/") {
      url = "/index.html";
    }
    if (asyncFileStreamer.findFile(url)) {
      res->writeStatus(uWS::HTTP_200_OK);
      auto mime_type = getMimeType(url);
      res->writeHeader("Content-Type", mime_type);	    
      asyncFileStreamer.streamFile(res, url);
    } else {
      // file not found
      res->writeStatus("404 Not Found");
      // TODO: send 404.html maybe?
      res->close();
    }
  });

  app.listen(port, [port](auto *listen_socket) {
    if (listen_socket) {
      globalListenSocket = listen_socket;
      std::cout << "Listening on port " << port << std::endl;
    }
  });

  struct us_loop_t *loop = (struct us_loop_t *) uWS::Loop::get();
  struct us_timer_t *delayTimer = us_create_timer(loop, 0, 0);

  // TODO: switch to polling
  // run every 1ms
  us_timer_set(delayTimer, [](struct us_timer_t *t) {

    // Exit logic
    if (exit_flag) {
      for ( auto it = clients.begin(); it != clients.end(); ++it ) {
	auto client = *it;
	client->close();
      }      
      us_listen_socket_close(1, globalListenSocket);
      us_timer_close(t);
    }

    // push to clients
    std::lock_guard<std::mutex> lock(outboundMutex);
    if (!outbound.empty()) {
      std::pair<std::string, std::string> val = outbound.front();
      outbound.pop();
      // std::cout << std::get<0>(val) << " " << std::get<1>(val) << std::endl;
      for ( auto it = clients.begin(); it != clients.end(); ++it ) {
	auto client = *it;
	client->send(std::get<1>(val), uWS::OpCode::TEXT);
      }      
    }

  }, 1, 1);
    
  app.run();
  std::cout << "Server Stooped" << std::endl;
}

void server_start(int port, std::string root) {
  exit_flag = false;
  web_thread = new std::thread([port, root]{web_server_func(port, root);});

}

void server_stop() {
  exit_flag = true;
}

void publish(std::string name, std::string data) {
  std::pair<std::string, std::string> val = std::make_pair(name, data);
  std::lock_guard<std::mutex> lock(outboundMutex);
  outbound.push(val);
}

std::string tryget(std::string /*name*/) {
  std::lock_guard<std::mutex> lock(inboundMutex);
  if (!inbound.empty()) {
    std::pair<std::string, std::string> val = inbound.front();
    inbound.pop();
    return std::get<1>(val);
  }
  return "";
}

PYBIND11_MODULE(droidlet_webserver, m) {
  m.doc() = "droidlet web server";

  m.def("start", &server_start, "starts the server",
	py::arg("port") = 8000,
	py::arg("root") = ".");
  m.def("stop", &server_stop, "stops the server");
  m.def("publish", &publish, "publish data to subscribers", py::arg("name"), py::arg("data"));
  m.def("tryget", &tryget, "get data from a given named channel", py::arg("name"));
}
