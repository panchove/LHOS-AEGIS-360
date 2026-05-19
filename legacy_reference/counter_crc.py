import serial

# Function to calculate checksum (sum of ID, Command, and Length)
def calculate_checksum(id, command, length, data1=0, data2=0, data3=0):
    checksum = hex((id + command + length + data1 + data2 + data3) & 0xFF) # Sum the numeric values of ID, Command, and Length
    checksum = checksum[2:]  # Remove the '0x' prefix
    print(f"Checksum: {checksum}")
    return checksum.upper()


# Function to build the command frame
def build_command_frame(id, command, length, data=b''):
    stx = 0x02  # Start of text
    etx = 0x03  # End of text
    
    # Convert ID, command, length, and data to ASCII hex
    # id_ascii = '{:04}'.format(id)  # ID in ASCII (e.g., 0001 for front door)
    # command_ascii = '{:02}'.format(command)  # Command in ASCII
    # length_ascii = '{:02}'.format(length)  # Length in ASCII
    id_string = f'{id:04x}'  # ID in ASCII (e.g., 0001 for front door)
    command_string = f'{command:02x}'  # Command in ASCII
    length_string = f'{length:02x}'  # Length in ASCII
    id_bytes = id_string.encode('ascii')  # Convert ID to ASCII representation of hex
    command_bytes = command_string.encode('ascii')  # Convert command to ASCII representation of hex
    length_bytes = length_string.encode('ascii')  # Convert length to ASCII representation of hex
    
    # Build frame without CHK and ETX
    frame = bytearray([stx]) + id_bytes + command_bytes + length_bytes + data

    print(f"Frame without CHK and ETX: {frame}")
    
    # Calculate checksum (sum of ID, Command, and Length)
    chk = calculate_checksum(id, command, length)  # Sum the numeric values
    chk_ascii = str(chk).encode('ascii')  # Convert checksum to ASCII representation of decimal result
    
    # Append CHK and ETX to the frame
    frame += chk_ascii + bytearray([etx])
    
    return frame

# Function to send the command and receive the response
def send_command(serial_port, command_frame):
    serial_port.write(command_frame)
    response = serial_port.read_until(expected=bytes([0x03]))  # Read until ETX (0x03)
    return response

# Initialize serial communication (adjust the port and baudrate accordingly)
ser = serial.Serial(
    port='COM8',  # Replace with your serial port
    baudrate=9600,
    bytesize=serial.EIGHTBITS,
    stopbits=serial.STOPBITS_ONE,
    parity=serial.PARITY_NONE,
    timeout=1
)

# Example to query the front door processor (ID=0001)
id_front_door = 0x0001
command_query = 0x13
length_query = 0x00

# Build command frame for querying the front door
frame_query_front = build_command_frame(id_front_door, command_query, length_query)

# Print the constructed frame
print(f"Query front door frame: {frame_query_front}")

# Send command and receive response
response_query_front = send_command(ser, frame_query_front)

print(f"Response from front door: {response_query_front}")
# Exclude STX (\x02) and ETX (\x03)
data =response_query_front[1:-1]

# Split the byte array
id_ascii = data[0:4]  # ID is 4 ASCII bytes
command_ascii = data[4:6]  # Command is 2 ASCII bytes
resp_length_ascii = data[6:8]  # Response length is 2 ASCII bytes
passengers_in_ascii = data[8:16]  # Passengers in is 8 ASCII bytes
passengers_out_ascii = data[16:24]  # Passengers out is 8 ASCII bytes
data2_ascii = data[24:40]  # DATA2 is 16 ASCII bytes
chk_ascii = data[40:42]  # Checksum is 2 ASCII bytes

# Convert ASCII hex values to actual integers
id_value = int(id_ascii.decode('ascii'), 16)  # Convert ASCII hex to int
command_value = int(command_ascii.decode('ascii'), 16)
resp_length_value = int(resp_length_ascii.decode('ascii'), 16)
passengers_in_value = int(passengers_in_ascii.decode('ascii'), 16)
passengers_out_value = int(passengers_out_ascii.decode('ascii'), 16)
data2_value = int(data2_ascii.decode('ascii'), 16)
chk_value = int(chk_ascii.decode('ascii'), 16)
calc_chk = calculate_checksum(id_value, command_value, resp_length_value, passengers_in_value, passengers_out_value, data2_value)   # Calculate the checksum
hex_calc_chk = int(calc_chk, 16)    # Convert the calculated checksum to int
if chk_value == hex_calc_chk:
    print("Checksums match!")

#print int values
print(f"ID: {id_value}")
print(f"COMMAND: {command_value}")
print(f"RESP_LENGTH: {resp_length_value}")
print(f"PASSENGERS_IN: {passengers_in_value}")
print(f"PASSENGERS_OUT: {passengers_out_value}")
print(f"DATA2: {data2_value}")
print(f"CHK: {chk_value}")
print(f"Calc CHK: {hex_calc_chk}")


# Print the results
print(f"ID: 0x{id_value:04X}")
print(f"COMMAND: 0x{command_value:02X}")
print(f"RESP_LENGTH: 0x{resp_length_value:02X}")
print(f"PASSENGERS_IN: 0x{passengers_in_value:08X}")
print(f"PASSENGERS_OUT: 0x{passengers_out_value:08X}")
print(f"DATA2: 0x{data2_value:016X}")
print(f"CHK: 0x{chk_value:02X}")
print(f"Calc CHK: 0x{hex_calc_chk:02X}")

# Close the serial connection
ser.close()

