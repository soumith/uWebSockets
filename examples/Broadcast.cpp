/* We simply call the root header file "App.h", giving you uWS::App and uWS::SSLApp */
#include "App.h"
#include <time.h>
#include <iostream>
#include <thread>
#include <algorithm>
#include <mutex>

/* Helpers for this example */
#include "helpers/AsyncFileReader.h"
#include "helpers/AsyncFileStreamer.h"
#include "helpers/Middleware.h"

// run server in separate thread...DONE
// add mime types compatibility...DONE
// make http_server example server serve files...DONE
// move the code from http_server to broadcast...DONE


// have a outbound queue that's shared between threads
// have server read from queue every 1ms and if there is data, to broadcast the data out

// have the server receive `message` from websocket and put that in inbound queue

thread_local uWS::SSLApp *globalApp;
const int SSL = 1;
std::mutex queueMutex;


void web_server_func() {
  /* ws->getUserData returns one of these */
  struct PerSocketData {
    /* Fill with user data */
  };

  /* Keep in mind that uWS::SSLApp({options}) is the same as uWS::App() when compiled without SSL support.
   * You may swap to using uWS:App() if you don't need SSL */
  uWS::SSLApp app = uWS::SSLApp({
      /* There are example certificates in uWebSockets.js repo */
      .key_file_name = "../misc/key.pem",
      .cert_file_name = "../misc/cert.pem",
      .passphrase = "1234"
    });



  app.ws<PerSocketData>("/*", {
      /* Settings */
      .compression = uWS::SHARED_COMPRESSOR,
      .maxPayloadLength = 16 * 1024 * 1024,
      .idleTimeout = 16,
      .maxBackpressure = 1 * 1024 * 1024,
      .closeOnBackpressureLimit = false,
      .resetIdleTimeoutOnSend = false,
      .sendPingsAutomatically = true,
      /* Handlers */
      .upgrade = nullptr,
      .open = [](auto *ws) {
	/* Open event here, you may access ws->getUserData() which points to a PerSocketData struct */
	ws->subscribe("broadcast");
      },
      .message = [](auto */*ws*/, std::string_view /*message*/, uWS::OpCode /*opCode*/) {

      },
      .drain = [](auto */*ws*/) {
	/* Check ws->getBufferedAmount() here */
      },
      .close = [](auto */*ws*/, int /*code*/, std::string_view /*message*/) {
	/* You may access ws->getUserData() here */
      }
    });


  AsyncFileStreamer asyncFileStreamer(".");
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

  app.listen(9001, [](auto *listen_socket) {
    if (listen_socket) {
      std::cout << "Listening on port " << 9001 << std::endl;
    }
  });

  struct us_loop_t *loop = (struct us_loop_t *) uWS::Loop::get();
  struct us_timer_t *delayTimer = us_create_timer(loop, 0, 0);

  // broadcast the unix time as millis every 8 millis
  us_timer_set(delayTimer, [](struct us_timer_t */*t*/) {

    struct timespec ts;
    timespec_get(&ts, TIME_UTC);

    int64_t millis = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

    std::cout << "Broadcasting timestamp: " << millis << std::endl;

    globalApp->publish("broadcast", std::string_view((char *) &millis, sizeof(millis)), uWS::OpCode::BINARY, false);

  }, 1, 1);

    
  globalApp = &app;
  app.run();
}

int main() {

  std::thread *web_thread = new std::thread(web_server_func);
  web_thread->join();

}
