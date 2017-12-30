# ESP_Telnet2Serial
Wifi Serial Bridge

Based on
https://blog.thesen.eu/telnet2serial-telnet-zu-rs232seriell-bruecke-mit-dem-esp8266-microcontroller/

Modified send/receive handling to suite the Arduino Sofware Serial library which can not send and receive simultanously.

Every single character received via Wifi is send to the serial port. ESP waits for the processing of this character.
Normally the character would be simply echoed, but any number of characters received before a 2ms timeout occures are
collected and then send back via Wifi. If the original character was a CR, the timeout is increased to 50ms. Because normally
more complex Arduino command processing starts when a complete line is entered.
