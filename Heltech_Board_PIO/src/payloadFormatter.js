function decodeUplink(input) {
  var bytes = input.bytes;

  var bme280_temperature = (bytes[0] << 8) | bytes[1]; // Combine MSB and LSB for BME280 temperature
  var bme280_humidity = (bytes[2] << 8) | bytes[3];    // Combine MSB and LSB for BME280 humidity
  var bme280_pressure = (bytes[4] << 16) | (bytes[5] << 8) | bytes[6]; // Combine MSB, middle byte, and LSB for BME280 pressure

  var dht22_temperature = (bytes[7] << 8) | bytes[8];  // Combine MSB and LSB for DHT22 temperature
  var dht22_humidity = (bytes[9] << 8) | bytes[10];    // Combine MSB and LSB for DHT22 humidity

  return {
    data: {
      bme280_temperature: bme280_temperature / 100,  // Convert to actual temperature (assuming it's in hundredths of degrees)
      bme280_humidity: bme280_humidity / 100,        // Convert to actual humidity (assuming it's in hundredths of percent)
      bme280_pressure: bme280_pressure / 100,        // Convert to actual pressure (assuming it's in hundredths of hPa)
      dht22_temperature: dht22_temperature / 100,    // Convert to actual temperature (assuming it's in hundredths of degrees)
      dht22_humidity: dht22_humidity / 100           // Convert to actual humidity (assuming it's in hundredths of percent)
    }
  };
}
