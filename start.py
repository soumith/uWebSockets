import droidlet_webserver as web
import time
import numpy as np

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
    data = [1, 2, 3, 4, 5, 'hi there', np.ones((5,))]    
    web.publish("foo", data)

    client_data = web.tryget("foo")
    if client_data is not None:
        print(client_data)


"""
# name, data = web.tryget()

@web.subscribe("foo")
def message(name, data):
    print(
"""
