import asyncio
from bleak import BleakClient

# UUIDs for Nordic UART Service (NUS)
NUS_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  # RX characteristic
TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  # TX characteristic

# Replace with the MAC address of your BLE device
DEVICE_ADDRESS = "74:4D:BD:89:B2:1D"  # e.g., "AA:BB:CC:DD:EE:FF"

CHUNK_SIZE = 512  # Chunk size for splitting data into smaller packets 

# Global variables to store received data and chunk information
received_data = ""
expected_packet_size = 0
expected_chunks = 0
received_chunks = 0

def split_data_into_chunks(data, chunk_size):
    """Splits data into smaller chunks."""
    for i in range(0, len(data), chunk_size):
        yield data[i:i + chunk_size]

async def send_data(client, data):
    """Sends large data in chunks via BLE."""
    total_size = len(data)
    chunks = list(split_data_into_chunks(data, CHUNK_SIZE))
    chunk_count = len(chunks)

    # Prepare and send header
    header = f"##{total_size};{chunk_count}".encode('utf-8')
    print(f"Sending header: {header.decode('utf-8')}")
    await client.write_gatt_char(RX_CHAR_UUID, header)

    # Send each chunk sequentially
    for i, chunk in enumerate(chunks):
        print(f"Sending chunk {i + 1}/{chunk_count}: {chunk}")
        await client.write_gatt_char(RX_CHAR_UUID, chunk)
        # Optional: Add a small delay between chunks if necessary
        await asyncio.sleep(0.01)

# Callback function to handle notifications from the TX characteristic
def handle_tx_notification(sender, data):
    global received_data, expected_packet_size, expected_chunks, received_chunks

    # Convert the incoming data to a string
    message = data.decode('utf-8')

    # Check if this is a header message (e.g., starts with ## and ends with ##)
    if message.startswith("##"):
        # Parse the header: ##PACKET_SIZE;CHUNK_COUNTS##
        print(f"Header received: {message}")
        header = message[2:]  # Remove the ## delimiters
        packet_size_str, chunk_count_str = header.split(";")
        expected_packet_size = int(packet_size_str)
        expected_chunks = int(chunk_count_str)
        received_chunks = 0  # Reset the counter for received chunks
        received_data = ""  # Clear previous data
        print(f"Header received: Packet Size = {expected_packet_size}, Chunks = {expected_chunks}")
    
    else:
        # If it's not a header, it's a chunk of data
        received_data += message
        received_chunks += 1
        print(f"Received chunk {received_chunks}/{expected_chunks}")

        # Check if we have received all chunks
        if received_chunks == expected_chunks:
            print(f"Complete data received: {received_data}")
            print(f'Expected packet size: {expected_packet_size}, Received packet size: {len(received_data)}')


async def ble_client():
    try:
        async with BleakClient(DEVICE_ADDRESS) as client:
            if not client.is_connected:
                print("Failed to connect to the device.")
                return
            
            print("Connected to the BLE device.")

            # Start receiving notifications from TX characteristic
            await client.start_notify(TX_CHAR_UUID, handle_tx_notification)
            print("Started listening to notifications from TX characteristic.")

            # Example of sending a large packet
            # data = b"0" * 2048  # Simulating large data to check how the device handles it
            # await send_data(client, data)
            # data = b"<Ac>1;ping;;19EE;5DAA</Ac>"
            # await send_data(client, data)
            # data = b"<Ac>1;get_config;;56D6;688C</Ac>"
            # await send_data(client, data)
            # data = b"<Ac>1;get_info;;87B7;8976</Ac>"
            # await send_data(client, data)
            data = b"<Ac>1;get_msg;;E37A;5367</Ac>"
            await send_data(client, data)
            # data = b"<Ac>1;set_config;data.ble.en:true,ble.0.model:ELA_PUCK_RHT,ble.0.address:C4:5F:57:58:80:87,ble.1.model:ELA_PUCK_EDDYSTONE,ble.1.address:DE:11:86:45:8C:CF,ble.2.model:ELA_PUCK_DI,ble.2.address:F1:FB:3C:2C:8D:7E;A3BB;BBFE</Ac>"
            # await send_data(client, data)
            # data = b"<Ac>1;start_fota;;2959;BB70</Ac>"
            # await send_data(client, data)
            # data = b"<Ac>1;reboot;;16BE;A4CD</Ac>"
            # await send_data(client, data)

            # Keep the client running for 10 seconds to receive notifications
            await asyncio.sleep(10)

            # Stop notifications before disconnecting
            await client.stop_notify(TX_CHAR_UUID)
            print("Stopped notifications and disconnected from the device.")
    except Exception as e:
        print(f"An error occurred: {e}")

# Run the event loop
loop = asyncio.get_event_loop()
loop.run_until_complete(ble_client())