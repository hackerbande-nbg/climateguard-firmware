function decodeUplink(input) {
  var bytes = input.bytes;

  var temperature = (bytes[0] << 8) | bytes[1]; // Combine MSB and LSB for temperature
  var humidity = (bytes[2] << 8) | bytes[3];    // Combine MSB and LSB for humidity
  var pressure = (bytes[4] << 16) | (bytes[5] << 8) | bytes[6]; // Combine MSB, middle byte, and LSB for pressure
  var voltage = (bytes[7] << 8) | bytes[8]; // Combine MSB and LSB for voltage

  return {
    data: {
      temperature: temperature / 100,  // Convert to actual temperature (hundredths of degrees)
      humidity: humidity / 100,        // Convert to actual humidity (hundredths of percent)
      pressure: pressure / 100,        // Convert to actual pressure (hundredths of hPa)
      voltage: voltage / 100           // Convert to actual voltage (hundredths of V)
    }
  };
}
