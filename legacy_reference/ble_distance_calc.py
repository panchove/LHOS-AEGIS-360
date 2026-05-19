import math

def calculate_distance(rssi, tx_power, n=2.0):
    """
    Estimate the distance between a BLE device and a BLE client.
    
    :param rssi: Received Signal Strength Indicator (dBm)
    :param tx_power: Transmit Power at 1 meter (dBm)
    :param n: Path Loss Exponent (default: 2.0 for free space)
    :return: Estimated distance in meters
    """
    distance = 10 ** ((tx_power - rssi) / (10 * n))
    return round(distance, 2)

# Example values
rssi_value = -70   # Measured RSSI
tx_power_value = 0  # Tx power at 1m (usually provided by the beacon)

# Calculate distances for different environments
print(f"Distance (Free Space, n=2.0): {calculate_distance(rssi_value, tx_power_value, 2.0)} m")
print(f"Distance (Office, n=2.7): {calculate_distance(rssi_value, tx_power_value, 2.7)} m")
print(f"Distance (Indoor, n=3.5): {calculate_distance(rssi_value, tx_power_value, 3.5)} m")
print(f"Distance (Dense, n=4.0): {calculate_distance(rssi_value, tx_power_value, 4.0)} m")
