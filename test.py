#!/usr/bin/env python

import asyncio
import websockets
import json

jdec = json.JSONDecoder()

async def hello():
    async with websockets.connect("ws://localhost:8001") as websocket:
        await websocket.send("Hello world!")
        while True:
            out = {}
            out['bar'] = 'Hello world!'
            json_val = json.dumps(out)
            await websocket.send(json_val)
            # await websocket.send("Hello world!")
            packet = await websocket.recv()
            print(jdec.decode(packet))

asyncio.run(hello())
