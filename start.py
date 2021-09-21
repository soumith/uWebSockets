import droidlet_webserver as web
import time

for i in range(10):
    web.start(8001)
    print("started")
    time.sleep(10)
    web.stop()
    print("stopped")
    time.sleep(10)
