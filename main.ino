/* Copyright (C) 2020 by JC Mariscal*/

/* This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

/*
Special thanks to:
.arduino15/packages/esp32/hardware/esp32/1.0.4/libraries/WebServer/examples/AdvancedWebServer/AdvancedWebServer.ino
.arduino15/packages/esp32/hardware/esp32/1.0.4/libraries/WebServer/examples/HelloServer/HelloServer.ino
.arduino15/packages/esp32/hardware/esp32/1.0.4/libraries/WebServer/examples/HttpBasicAuth/HttpBasicAuth.ino
 .arduino15/packages/esp32/hardware/esp32/1.0.4/libraries/WebServer/examples/SimpleAuthentification/SimpleAuthentification.ino
 .arduino15/packages/esp32/hardware/esp32/1.0.4/libraries/WebServer/examples/HelloServer/HelloServer.ino
https://lastminuteengineers.com/bme280-esp32-weather-station/
https://www.svgrepo.com/
https://anyconv.com/svg-to-html-converter/

/*
  Response codes:
    100: Continue
    101: Switching Protocols
    200: OK
    201: Created
    202: Accepted
    203: Non-Authoritative Information
    204: No Content
    205: Reset Content
    206: Partial Content
    300: Multiple Choices
    301: Moved Permanently
    302: Found
    303: See Other
    304: Not Modified
    305: Use Proxy
    307: Temporary Redirect
    400: Bad Request
    401: Unauthorized
    402: Payment Required
    403: Forbidden
    404: Not Found
    405: Method Not Allowed
    406: Not Acceptable
    407: Proxy Authentication Required
    408: Request Time-out
    409: Conflict
    410: Gone
    411: Length Required
    412: Precondition Failed
    413: Request Entity Too Large
    414: Request-URI Too Large
    415: Unsupported Media Type
    416: Requested range not satisfiable
    417: Expectation Failed
    500: Internal Server Error
    501: Not Implemented
    502: Bad Gateway
    503: Service Unavailable
    504: Gateway Time-out
    505: HTTP Version not supported
*/

// LIBRARIES
// =========
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <ESPmDNS.h>

// CONSTANTS DEFINITIONS
// =======================
// WiFi
const char *ssid = "wifiuser";
const char *pass = "pass";
WebServer server(80);

// PINS
// All pins A0 …​ A21 except A2, A3, A4 can be used for digital output.
const int TRIG_PIN = 12;        //gpio 12 A11
const int ECHO_PIN = 27;        // gpio 27 A10
const int LED_EMPTY_PIN = 21;   //21; // pin 21 original pin-220ohm-anode cathode-gnd white
const int LED_FULL_PIN = 4;     // //pin 4 original

// CALCULATION CONSTANTS
const int CM_CONST = 58;
const int IN_CONST = 148;

// 29000 us pulse = 500 cm
// Anything over 400 cm (23200 us pulse) is "out of range"
const unsigned int MAX_DISTANCE = 29000;
const int DELAY_TRIG_US = 70;
const int NUM_MEASUREMENTS = 50;

// TANK DIMENSIONS
int HEIGHT = 100;               // height of tank cm
int DIAM = 100;                 // diametre tank cm

// TEST
bool IS_TEST_LEDS = false;

/**
   @brief: Function initialises pin
   @param pin: Pin IO
   @param io: INPUT or OUTPUT
   @param low_high: digital write to high or low value
   @returns: void
 */
void init_pin(const int pin, int io, int low_high)
{
        pinMode(pin, io);
        digitalWrite(pin, low_high);
}

/**
   @brief: Calculate distance from ultrasonic sensor
   @param trigger_pin, echo_pin: Pin IO in the board
   @param cm_constant, in_constant: constant from datasheet to calculate distance (calculated from velocity of air)
   @param delay_trigger_us: how long to wait in microsends to trigger a wave
   @param ins_cm: true if return value set to cm, false if return value set to inches
   @returns: distance calculated in cm from sensor to object
 */
float cal_distance(const int trigger_pin = TRIG_PIN, const int echo_pin = ECHO_PIN, const int cm_constant = CM_CONST, const int in_constant = IN_CONST, const int delay_trigger_us = DELAY_TRIG_US, bool is_cm = true)
{
        const int delay_init = 5;
        unsigned long t_1;
        unsigned long t_2;
        unsigned long pw;       //pulse width

        // Hold the trigger pin high for at least 10 us
        digitalWrite(trigger_pin, LOW);
        delayMicroseconds(delay_init);
        digitalWrite(trigger_pin, HIGH);
        delayMicroseconds(delay_trigger_us);
        digitalWrite(trigger_pin, LOW);

        // wait until echo pin changes state (pulse on echo pin)
        while (digitalRead(echo_pin) == 0) ;

        // calculate how long the echo pin is held high
        // Note: the micros() counter will overflow after ~70 min
        t_1 = micros();
        while (digitalRead(echo_pin) == 1) ;
        t_2 = micros();
        pw = t_2 - t_1;
        if (pw > MAX_DISTANCE) {
                return (float)1000;
        }
        // calculate distance using constants from the datasheet
        return is_cm ? (pw / cm_constant) : (pw / in_constant);
}

void test_led(bool enable, int delay_ms)
{
        if (enable) {
                while (1) {
                        Serial.println("testing LEDs");
                        digitalWrite(LED_EMPTY_PIN, HIGH);
                        delay(1000);
                        digitalWrite(LED_EMPTY_PIN, LOW);
                        delay(3000);
                        digitalWrite(LED_FULL_PIN, HIGH);
                        delay(1000);
                        digitalWrite(LED_FULL_PIN, LOW);
                        delay(3000);
                }
        } else {
                return;
        }
}

/**
 * @brief measure average distance (function has side effects and depends on global variables)
 *
 * @return float
 */
float measure_average_distance(int num_measurements = NUM_MEASUREMENTS)
{
        float dist_cm;
        int i = NUM_MEASUREMENTS;
        float distance_sum = 0;
        while (i--) {
                //distance_sum += cal_distance();
                distance_sum += cal_distance(TRIG_PIN, ECHO_PIN, CM_CONST, IN_CONST, 20, true);
        }

        dist_cm = distance_sum / NUM_MEASUREMENTS;

        return dist_cm;
}

/**
 * @brief handle for index webpage
 *
 */
void handle_root()
{
        float dist_cm = 0;
        dist_cm = measure_average_distance();
        //do
        //{
        //  dist_cm = measure_average_distance();
        //} while (dist_cm >= 1000);

        if (dist_cm > 1000) {
                Serial.println("Out of range");
        } else {
                Serial.println(dist_cm);
                Serial.println(" cm \t");
        }

        //if (dist_cm >= (HEIGHT - (HEIGHT*0.125)) && dist_cm <= HEIGHT) {
        if (dist_cm >= (HEIGHT - (HEIGHT * 0.125))) {
                Serial.println("tank empty");
                digitalWrite(LED_EMPTY_PIN, HIGH);
                digitalWrite(LED_FULL_PIN, LOW);
                //digitalWrite(LED_EMPTY_PIN, LOW);
        } else if (dist_cm >= 0 && dist_cm <= (HEIGHT * 0.125)) {
                Serial.println("tank full");
                digitalWrite(LED_FULL_PIN, HIGH);
                digitalWrite(LED_EMPTY_PIN, LOW);
                //digitalWrite(LED_FULL_PIN, LOW);
        } else {
                Serial.println("tank neither full nor empty");
                digitalWrite(LED_FULL_PIN, LOW);
                digitalWrite(LED_EMPTY_PIN, LOW);
        }
        //delay(5000);

        int sec = millis() / 1000;
        int min = sec / 60;
        int hr = min / 60;

        /*
           char temp_buffer[10000];
           snprintf(temp_buffer, 2000,

           "<!DOCTYPE html>\
           <html>\
           <head>\
           <meta http-equiv='refresh' content='5'/>\
           <title>Tank Water Level</title>\
           <style>\
           body { background-color: ##FFFFFF; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\
           </style>\
           </head>\
           <body>\
           <h1>&nbsp;Practical Electronics Project</h1>\
           <hr />\
           <h2>Water Tank(live measurements)</h2>\
           <p>Uptime: %02d:%02d:%02d</p>\
           <p> tank height: %d cm </p>\
           <p> tank fill level (cm): %f</p>\
           <p> tank fill volume (cm3): %f</p>\
           <p><a href='/tank_measurements'>click here </a>to change tank dimensions.</p>\
           <img src=\"/test.svg\" />\
           </body>\
           </html>",
           hr,
           min % 60,
           sec % 60,
           HEIGHT,
           ((HEIGHT - dist_cm)< 0) ? HEIGHT : (HEIGHT - dist_cm),
           (PI * (DIAM/2)*(DIAM/2) * HEIGHT)
           );
           server.send(200, "text/html", temp_buffer);
         */

        // generated with: $ awk '{print$0"\\"}' index.html > index_for_c.html
        String temp_buffer = "<!DOCTYPE html>";
        temp_buffer += "<html>";
        temp_buffer += "<head>";
        temp_buffer += "<title>Water tank level watcher</title>";
        temp_buffer += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
        temp_buffer += "<link href='https://fonts.googleapis.com/css?family=Open+Sans:300,400,600' rel='stylesheet'>";
        temp_buffer += "<style>";
        temp_buffer += "html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px auto; text-align: center;color: #444444;}";
        temp_buffer += "body{margin: 0px;} ";
        temp_buffer += "h1 {margin: 50px auto 30px;} ";
        temp_buffer += ".side-by-side{display: table-cell;vertical-align: middle;position: relative;}";
        temp_buffer += ".text{font-weight: 600;font-size: 19px;width: 200px;}";
        temp_buffer += ".reading{font-weight: 300;font-size: 50px;padding-right: 25px;}";
        temp_buffer += ".uptime .reading{color: #F29C1F;}";
        temp_buffer += ".height .reading{color: ##444444;}";
        temp_buffer += ".fill_level .reading{color: ##444444;}";
        temp_buffer += ".volume .reading{color: ##444444;}";
        temp_buffer += ".superscript{font-size: 17px;font-weight: 600;position: absolute;top: 10px;}";
        temp_buffer += ".data{padding: 10px;}";
        temp_buffer += ".container{display: table;margin: 0 auto;}";
        temp_buffer += ".icon{width:65px}";
        temp_buffer += "</style>";
        temp_buffer += "<script>\n";
        temp_buffer += "setInterval(loadDoc,3000);\n";
        temp_buffer += "function loadDoc() {\n";
        temp_buffer += "var xhttp = new XMLHttpRequest();\n";
        temp_buffer += "xhttp.onreadystatechange = function() {\n";
        temp_buffer += "if (this.readyState == 4 && this.status == 200) {\n";
        temp_buffer += "document.body.innerHTML =this.responseText}\n";
        temp_buffer += "};\n";
        temp_buffer += "xhttp.open(\"GET\", \"/\", true);\n";
        temp_buffer += "xhttp.send();\n";
        temp_buffer += "}\n";
        temp_buffer += "</script>\n";
        temp_buffer += "</head>";
        temp_buffer += "<body>";
        temp_buffer += "<h1>Water tank (live measurements)</h1>";
        temp_buffer += "<div class='container'>";
        temp_buffer += "<div class='data uptime'>";
        temp_buffer += "<div class='side-by-side icon'>";
        // clock vector: https://www.svgrepo.com/svg/11916/clock
        temp_buffer += "<svg version='1.1' id='Layer_1' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' x='0px' y='0px'\
	 viewBox='0 0 512 512' style='enable-background:new 0 0 512 512;' xml:space='preserve'>\
<circle style='fill:#EDEDED;' cx='256' cy='256' r='245.801'/>\
<circle style='fill:#FFBC52;' cx='256' cy='256' r='197.283'/>\
<circle style='fill:#EDEDED;' cx='256' cy='256' r='23.717'/>\
<path d='M437.02,74.98C388.667,26.628,324.38,0,256,0S123.333,26.628,74.98,74.98S0,187.62,0,256s26.628,132.667,74.98,181.02\
	S187.62,512,256,512s132.667-26.628,181.02-74.98S512,324.38,512,256S485.372,123.333,437.02,74.98z M256,491.602\
	c-129.911,0-235.602-105.69-235.602-235.602S126.089,20.398,256,20.398S491.602,126.089,491.602,256S385.911,491.602,256,491.602z'\
	/>\
<path d='M256,48.514C141.591,48.514,48.514,141.591,48.514,256S141.591,463.486,256,463.486S463.486,370.409,463.486,256\
	S370.409,48.514,256,48.514z M256,443.088c-103.161,0-187.088-83.927-187.088-187.088S152.839,68.912,256,68.912\
	S443.088,152.839,443.088,256S359.161,443.088,256,443.088z'/>\
<path d='M149.627,135.204c-3.983-3.982-10.441-3.982-14.425,0c-3.983,3.983-3.983,10.441,0,14.425l2.287,2.287\
	c1.992,1.991,4.602,2.987,7.212,2.987s5.221-0.996,7.212-2.987c3.983-3.983,3.983-10.441,0-14.425L149.627,135.204z'/>\
<path d='M98.601,245.801h-3.234c-5.633,0-10.199,4.566-10.199,10.199c0,5.633,4.566,10.199,10.199,10.199h3.234\
	c5.633,0,10.199-4.566,10.199-10.199S104.234,245.801,98.601,245.801z'/>\
<path d='M137.49,360.086l-2.287,2.287c-3.983,3.983-3.983,10.441,0,14.425c1.992,1.991,4.602,2.987,7.212,2.987\
	s5.221-0.996,7.212-2.987l2.287-2.287c3.983-3.983,3.983-10.441,0-14.425C147.931,356.103,141.473,356.103,137.49,360.086z'/>\
<path d='M256,403.2c-5.633,0-10.199,4.566-10.199,10.199v3.234c0,5.633,4.566,10.199,10.199,10.199\
	c5.633,0,10.199-4.566,10.199-10.199v-3.234C266.199,407.766,261.633,403.2,256,403.2z'/>\
<path d='M374.51,360.086c-3.983-3.982-10.441-3.982-14.425,0c-3.983,3.983-3.983,10.441,0,14.425l2.287,2.287\
	c1.992,1.991,4.602,2.987,7.212,2.987c2.61,0,5.221-0.996,7.212-2.987c3.983-3.983,3.983-10.441,0-14.425L374.51,360.086z'/>\
<path d='M416.633,245.801h-3.234c-5.633,0-10.199,4.566-10.199,10.199c0,5.633,4.566,10.199,10.199,10.199h3.234\
	c5.633,0,10.199-4.566,10.199-10.199S422.266,245.801,416.633,245.801z'/>\
<path d='M362.373,135.204l-2.287,2.287c-3.983,3.983-3.983,10.441,0,14.425c1.992,1.991,4.602,2.987,7.212,2.987\
	s5.221-0.996,7.212-2.987l2.287-2.287c3.983-3.983,3.983-10.441,0-14.425C372.814,131.221,366.355,131.221,362.373,135.204z'/>\
<path d='M256,110.151c5.633,0,10.199-4.566,10.199-10.199v-4.586c0-5.633-4.566-10.199-10.199-10.199s-10.199,4.566-10.199,10.199\
	v4.586C245.801,105.585,250.367,110.151,256,110.151z'/>\
<path d='M346.559,245.801h-58.211c-3.322-10.512-11.636-18.826-22.148-22.148V133.61c0-5.633-4.566-10.199-10.199-10.199\
	s-10.199,4.566-10.199,10.199v90.044c-13.733,4.34-23.717,17.198-23.717,32.347c0,18.701,15.215,33.916,33.916,33.916\
	c15.149,0,28.007-9.985,32.347-23.717h58.211c5.633,0,10.199-4.566,10.199-10.199S352.192,245.801,346.559,245.801z M256,269.518\
	c-7.455,0-13.518-6.064-13.518-13.518c0-7.454,6.064-13.518,13.518-13.518c7.454,0,13.518,6.064,13.518,13.518\
	C269.518,263.454,263.455,269.518,256,269.518z'/>\
</svg>";
        temp_buffer += "</div>";
        temp_buffer += "<div class='side-by-side text'>Uptime</div>";
        temp_buffer += "<div class='side-by-side reading'>";
        temp_buffer += (int)hr;
        temp_buffer += "h:";
        temp_buffer += (int)min % 60;
        temp_buffer += "m:";
        temp_buffer += (int)sec % 60;
        temp_buffer += "s";
        temp_buffer += "<span class='superscript'></span></div>";
        temp_buffer += "</div>";
        temp_buffer += "<div class='data height'>";
        temp_buffer += "<div class='side-by-side icon'>";
        // height vector: https://www.svgrepo.com/svg/42372/ruler
        temp_buffer += "<svg version='1.1' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 340 340' xmlns:xlink='http://www.w3.org/1999/xlink' enable-background='new 0 0 340 340'>\
  <path d='M92.5,340h155V0h-155V340z M112.5,20h115v60H180v20h47.5v60H160v20h67.5v60H180v20h47.5v60h-115V20z'/>\
</svg>";
        temp_buffer += "</div>";
        temp_buffer += "<div class='side-by-side text'>Tank height: </div>";
        temp_buffer += "<div class='side-by-side reading'>";
        temp_buffer += (int)HEIGHT;
        temp_buffer += "<span class='superscript'>cm</span></div>";
        temp_buffer += "</div>";
        temp_buffer += "<div class='data fill_level'>";
        temp_buffer += "<div class='side-by-side icon'>";
        temp_buffer +=
            "<svg enable-background='new 0 0 40.542 40.541'height=40.541px id=Layer_1 version=1.1 viewBox='0 0 40.542 40.541'width=40.542px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M34.313,20.271c0-0.552,0.447-1,1-1h5.178c-0.236-4.841-2.163-9.228-5.214-12.593l-3.425,3.424";
        temp_buffer += "c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293c-0.391-0.391-0.391-1.023,0-1.414l3.425-3.424";
        temp_buffer += "c-3.375-3.059-7.776-4.987-12.634-5.215c0.015,0.067,0.041,0.13,0.041,0.202v4.687c0,0.552-0.447,1-1,1s-1-0.448-1-1V0.25";
        temp_buffer += "c0-0.071,0.026-0.134,0.041-0.202C14.39,0.279,9.936,2.256,6.544,5.385l3.576,3.577c0.391,0.391,0.391,1.024,0,1.414";
        temp_buffer += "c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293L5.142,6.812c-2.98,3.348-4.858,7.682-5.092,12.459h4.804";
        temp_buffer += "c0.552,0,1,0.448,1,1s-0.448,1-1,1H0.05c0.525,10.728,9.362,19.271,20.22,19.271c10.857,0,19.696-8.543,20.22-19.271h-5.178";
        temp_buffer += "C34.76,21.271,34.313,20.823,34.313,20.271z M23.084,22.037c-0.559,1.561-2.274,2.372-3.833,1.814";
        temp_buffer += "c-1.561-0.557-2.373-2.272-1.815-3.833c0.372-1.041,1.263-1.737,2.277-1.928L25.2,7.202L22.497,19.05";
        temp_buffer += "C23.196,19.843,23.464,20.973,23.084,22.037z'fill=#26B999 /></g></svg>";
        temp_buffer += "</div>";
        temp_buffer += "<div class='side-by-side text'>Tank fill level (cm): </div>";
        temp_buffer += "<div class='side-by-side reading'>";
        temp_buffer += (float)((HEIGHT - dist_cm) < 0) ? HEIGHT : (HEIGHT - dist_cm);
        temp_buffer += "<span class='superscript'>cm</span></div>";
        temp_buffer += "</div>";
        temp_buffer += "<div class='data volume'>";
        temp_buffer += "<div class='side-by-side icon'>";
        //volume svg
        temp_buffer += "<svg version='1.1' id='Layer_1' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' x='0px' y='0px'\
	 viewBox='0 0 483.956 483.956' style='enable-background:new 0 0 483.956 483.956;' xml:space='preserve'>\
			<rect x='87.322' y='107.261' width='149.667' height='15'/>\
			<rect x='117.255' y='147.173' width='119.733' height='15'/>\
			<rect x='177.122' y='187.084' width='59.867' height='15'/>\
			<path d='M460.292,276.884h-36.28c9.225-5.21,17.808-11.714,25.519-19.425c22.199-22.2,34.425-51.616,34.425-82.831\
				c0-64.655-52.601-117.256-117.256-117.256H199.589V27.44h-74.867v29.933h-7.467c-31.215,0-60.631,12.226-82.831,34.425\
				C12.226,113.997,0,143.413,0,174.628c0,43.847,24.196,82.143,59.933,102.255H23.664c-10.646,0-18.675,6.227-18.675,14.484v20.954\
				c0,8.257,8.029,14.484,18.675,14.484h36.203v114.711H17.478v15h449v-15h-42.389v-114.71h36.203\
				c10.646,0,18.675-6.227,18.675-14.484v-20.954C478.967,283.111,470.938,276.884,460.292,276.884z M354.244,72.372H366.7\
				c56.384,0,102.256,45.872,102.256,102.256c0,27.208-10.666,52.857-30.032,72.224c-19.365,19.366-45.016,30.031-72.224,30.031\
				h-12.456V72.372z M284.4,72.372h14.933v92.278h15V72.372h24.911v204.511H284.4V72.372z M139.722,42.44h44.867v14.933h-44.867\
				V42.44z M15,174.628c0-27.208,10.666-52.858,30.031-72.224c19.366-19.366,45.016-30.032,72.224-30.032H269.4v204.511H117.255\
				C60.872,276.884,15,231.012,15,174.628z M89.8,441.517H74.867V326.806H89.8V441.517z M379.155,441.517H104.8V326.806h274.355\
				V441.517z M409.089,441.517h-14.934V326.806h14.934V441.517z M460.292,311.806H23.664c-1.71,0-2.987-0.4-3.675-0.745v-18.432\
				c0.688-0.345,1.965-0.745,3.675-0.745h436.628c1.71,0,2.986,0.4,3.675,0.745v18.432h0\
				C463.279,311.406,462.002,311.806,460.292,311.806z'/>\
        </svg>";
        temp_buffer += "</div>";
        temp_buffer += "<div class='side-by-side text'>Tank volume (cm3): </div>";
        temp_buffer += "<div class='side-by-side reading'>";
        temp_buffer += (float)(PI * (DIAM / 2) * (DIAM / 2) * HEIGHT) / 1000;
        temp_buffer += "<span class='superscript'>m3</span></div>";
        temp_buffer += "</div>";
        temp_buffer += "</div>";
        temp_buffer += "</body>";
        temp_buffer += "</html>";
        server.send(200, "text/html", temp_buffer);
}

void handle_not_found()
{
        String message = "File Not Found\n\n";
        message += "URI: ";
        message += server.uri();
        message += "\nMethod: ";
        message += (server.method() == HTTP_GET) ? "GET" : "POST";
        message += "\nArguments: ";
        message += server.args();
        message += "\n";

        for (uint8_t i = 0; i < server.args(); i++) {
                message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
        }

        server.send(404, "text/plain", message);
}

//login page, also called for disconnect
//From: .arduino15/packages/esp32/hardware/esp32/1.0.4/libraries/WebServer/examples/SimpleAuthentification/SimpleAuthentification.ino
void handleLogin()
{
        String msg;
        if (server.hasHeader("Cookie")) {
                Serial.print("Found cookie: ");
                String cookie = server.header("Cookie");
                Serial.println(cookie);
        }
        if (server.hasArg("DISCONNECT")) {
                Serial.println("Disconnection");
                server.sendHeader("Location", "/login");
                server.sendHeader("Cache-Control", "no-cache");
                server.sendHeader("Set-Cookie", "ESPSESSIONID=0");
                server.send(301);
                return;
        }
        if (server.hasArg("USERNAME") && server.hasArg("PASSWORD")) {
                if (server.arg("USERNAME") == "admin" && server.arg("PASSWORD") == "admin") {
                        server.sendHeader("Location", "/");
                        server.sendHeader("Cache-Control", "no-cache");
                        server.sendHeader("Set-Cookie", "ESPSESSIONID=1");
                        server.send(301);
                        Serial.println("Log in Successful");
                        return;
                }
                msg = "Wrong username/password! try again.";
                Serial.println("Log in Failed");
        }
        String content = "<html><body><form action='/login' method='POST'>To log in, please use : admin/admin<br>";
        content += "User:<input type='text' name='USERNAME' placeholder='user name'><br>";
        content += "Password:<input type='password' name='PASSWORD' placeholder='password'><br>";
        content += "<input type='submit' name='SUBMIT' value='Submit'></form>" + msg + "<br>";
        content += "You also can go <a href='/inline'>here</a></body></html>";
        server.send(200, "text/html", content);
}

/**
 * @brief Handle /tank_measurements page
 *
 */
void handle_tank_variables()
{
        String msg;
        if (server.hasHeader("Cookie")) {
                Serial.print("Found cookie: ");
                String cookie = server.header("Cookie");
                Serial.println(cookie);
        }
        if (server.hasArg("DISCONNECT")) {
                Serial.println("Disconnection");
                server.sendHeader("Location", "/login");
                server.sendHeader("Cache-Control", "no-cache");
                server.sendHeader("Set-Cookie", "ESPSESSIONID=0");
                server.send(301);
                return;
        }
        if (server.hasArg("HEIGHT") && server.hasArg("DIAMETER")) {
                if (server.arg("HEIGHT").toInt() <= 400 && server.arg("DIAMETER").toInt() <= 400) {
                        // update global variables
                        HEIGHT = server.arg("HEIGHT").toInt();
                        DIAM = server.arg("HEIGHT").toInt();

                        server.sendHeader("Location", "/");
                        server.sendHeader("Cache-Control", "no-cache");
                        server.sendHeader("Set-Cookie", "ESPSESSIONID=1");
                        server.send(301);
                        Serial.println("Modified H/D Successful");
                        return;
                }
                msg = "Please input maximum height/diameter of 400cm";
                Serial.println("Height input parameter failed");
        }
        String content = "<html><body><form action='/tank_measurements' method='POST'>Please insert the height and diameter of the tank:<br>";
        content += "Height:<input type='number' name='HEIGHT' placeholder='height in cm'><br>";
        content += "Diameter:<input type='number' name='DIAMETER' placeholder='diameter in cm'><br>";
        content += "<input type='submit' name='SUBMIT' value='Submit'></form>" + msg + "<br>";
        // content += "You also can go <a href='/inline'>here</a></body></html>";
        server.send(200, "text/html", content);
}

void setup()
{
        // initialise ultrasonic pins
        init_pin(TRIG_PIN, OUTPUT, LOW);
        init_pin(ECHO_PIN, INPUT, LOW);

        // initialise LEDs
        init_pin(LED_EMPTY_PIN, OUTPUT, HIGH);
        init_pin(LED_FULL_PIN, OUTPUT, HIGH);
        test_led(IS_TEST_LEDS, 50);

        Serial.begin(9600);
        Serial.println("Connecting: ");
        Serial.println(ssid);

        WiFi.begin(ssid, pass);
        while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                Serial.print(".");
        }
        Serial.print("IP is: ");
        Serial.println(WiFi.localIP());
        if (MDNS.begin("esp32")) {
                Serial.println("MDNS responder started");
        }
        server.on("/", handle_root);
        server.on("/login", handleLogin);
        server.on("/tank_measurements", handle_tank_variables);
        server.onNotFound(handle_not_found);
        server.begin();
        Serial.println("server starting");

}

void loop()
{
        server.handleClient();
}
