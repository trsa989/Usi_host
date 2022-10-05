/**
 * \file
 *
 * \brief Common Utils
 *
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

#ifndef _LED_H_

int set_led_brightness(const char *ledname, int brightness);
int set_led_heartbeat_invert(const char *ledname, int invert);
int set_led_timer_delays(const char *ledname, int delay_on, int delay_off);

#define LED_ON  1
#define LED_OFF 0

#define LED_HEARTBEAT_NOT_INVERTED 0
#define LED_HEARTBEAT_INVERTED 1

#define LED_RED   "red"
#define LED_GREEN "green"
#define LED_BLUE  "blue"

#define SET_LED(led,value) set_led_brightness(led, value);
#define SET_LED_TIMER(led,on,off) set_led_timer_delays(led,on,off);
#define SET_LED_HEARTBEAT(led,invert) set_led_heartbeat_invert(led,invert);

/* ON/OFF LED-RGB Status */
#define SET_LED_RED()       SET_LED(LED_RED,LED_ON)
#define SET_LED_GREEN()     SET_LED(LED_GREEN,LED_ON)
#define SET_LED_BLUE()      SET_LED(LED_BLUE,LED_ON)
#define SET_LED_RED_OFF()   SET_LED(LED_RED,LED_OFF)
#define SET_LED_GREEN_OFF() SET_LED(LED_GREEN,LED_OFF)
#define SET_LED_BLUE_OFF()  SET_LED(LED_BLUE,LED_OFF)
#define SET_RGB_OFF()      { SET_LED(LED_BLUE,LED_OFF) ; SET_LED(LED_GREEN,LED_OFF); SET_LED(LED_RED,LED_OFF); }
#define SET_RGB_RED()      { SET_LED(LED_BLUE,LED_OFF) ; SET_LED(LED_GREEN,LED_OFF); SET_LED(LED_RED,LED_ON); }
#define SET_RGB_GREEN()    { SET_LED(LED_BLUE,LED_OFF) ; SET_LED(LED_GREEN,LED_ON); SET_LED(LED_RED,LED_OFF); }
#define SET_RGB_BLUE()     { SET_LED(LED_BLUE,LED_ON) ; SET_LED(LED_GREEN,LED_OFF); SET_LED(LED_RED,LED_OFF); }
#define SET_RGB_YELLOW()   { SET_LED(LED_BLUE,LED_OFF) ; SET_LED(LED_GREEN,LED_ON); SET_LED(LED_RED,LED_ON); }
#define SET_RGB_VIOLET()   { SET_LED(LED_GREEN,LED_OFF) ; SET_LED(LED_BLUE,LED_ON); SET_LED(LED_RED,LED_ON); }
#define SET_RGB_BLUE_SKY() { SET_LED(LED_RED,LED_OFF) ; SET_LED(LED_BLUE,LED_ON); SET_LED(LED_GREEN,LED_ON); }
#define SET_RGB_WHITE()    { SET_LED(LED_GREEN,LED_ON) ; SET_LED(LED_BLUE,LED_ON); SET_LED(LED_RED,LED_ON);   }

/* Heartbeat LED-RGB Status */
#define SET_LED_RED_HEARTBEAT(invert)   SET_LED_HEARTBEAT(LED_RED,invert)
#define SET_LED_GREEN_HEARTBEAT(invert) SET_LED_HEARTBEAT(LED_GREEN,invert)
#define SET_LED_BLUE_HEARTBEAT(invert)  SET_LED_HEARTBEAT(LED_BLUE,invert)

/* Timer LED-RGB Status */
#define SET_LED_RED_TIMER(on,off)   SET_LED_TIMER(LED_RED,on,off)
#define SET_LED_GREEN_TIMER(on,off) SET_LED_TIMER(LED_GREEN,on,off)
#define SET_LED_BLUE_TIMER(on,off)  SET_LED_TIMER(LED_BLUE,on,off)

#endif
