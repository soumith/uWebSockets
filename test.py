#!/usr/bin/env python

import asyncio
import websockets

async def hello():
    async with websockets.connect("ws://localhost:8001") as websocket:
        await websocket.send("Hello world!")
        while True:
            await websocket.send("Hello world!")
            packet = await websocket.recv()
            print(packet)

asyncio.run(hello())
