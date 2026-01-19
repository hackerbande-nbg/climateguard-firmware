function decodeUplink(input) {
  var bytes = input.bytes;

  var version = bytes[0]; // First byte is version
  var temperature = (bytes[1] << 8) | bytes[2]; // Combine MSB and LSB for temperature
  var humidity = (bytes[3] << 8) | bytes[4];    // Combine MSB and LSB for humidity
  var pressure = (bytes[5] << 16) | (bytes[6] << 8) | bytes[7]; // Combine MSB, middle byte, and LSB for pressure
  var voltage = (bytes[8] << 8) | bytes[9]; // Combine MSB and LSB for voltage

  return {
    data: {
      version: version,
      temperature: temperature / 100,  // Convert to actual temperature (hundredths of degrees)
      humidity: humidity / 100,        // Convert to actual humidity (hundredths of percent)
      pressure: pressure / 100,        // Convert to actual pressure (hundredths of hPa)
      voltage: voltage / 100           // Convert to actual voltage (hundredths of V)
    }
  };
}
