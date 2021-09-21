import droidlet_webserver as web
import time

web.start(8001)

# for i in range(10):
#     print("started")
#     time.sleep(10)
#     web.stop()
#     print("stopped")
#     time.sleep(10)




# API

while True:
    data = "hi there"
    web.publish("foo", data)

    client_data = web.tryget("foo")
    if client_data != "":
        print(client_data)


"""
# name, data = web.tryget()

@web.subscribe("foo")
def message(name, data):
    print(
"""
