# Voice-activated virtual controller for ROB to play Gyromite
import uinput
import time
import snowboydecoder
import sys
import signal
import Adafruit_PCA9685

interrupted = False

# Create a virutal gamepad with two buttons and an x/y axis
events = (
	uinput.BTN_0,
	uinput.BTN_1,
	uinput.ABS_X + (0, 255, 0, 0),
	uinput.ABS_Y + (0, 255, 0, 0),
	)

# RetroPie won't recognise a controller without at least an x/y axis
	
	
# Centre the analog stick
device = uinput.Device(events)
device.emit(uinput.ABS_X, 128)
device.emit(uinput.ABS_Y, 128)

# Initialise the PCA9685 using alternate i2c pins:
pwm = Adafruit_PCA9685.PCA9685(address=0x40, busnum=3)
pwm.set_pwm_freq(60)

# Turn ROBs led on
pwm.set_pwm(4, 0, 4000)



def rob_press_a():
	device.emit(uinput.BTN_0,1)
	flash_led()

def rob_press_b():
	device.emit(uinput.BTN_1,1)
	flash_led()

def rob_release_a():
	device.emit(uinput.BTN_0,0)
	flash_led()

def rob_release_b():
	device.emit(uinput.BTN_1,0)
	flash_led()

def flash_led():
	pwm.set_pwm(4, 0, 0)
	time.sleep(0.1)
	pwm.set_pwm(4, 0, 4000)
	time.sleep(0.1)
	pwm.set_pwm(4, 0, 0)
	time.sleep(0.1)
	pwm.set_pwm(4, 0, 4000)


def signal_handler(signal, frame):
    global interrupted
    interrupted = True


def interrupt_callback():
    global interrupted
    return interrupted

# These models correspond to our button commands "ROB press A" etc.
models = (["/home/pi/snowboy/resources/rob_press_a.pmdl", 
"/home/pi/snowboy/resources/rob_press_b.pmdl",
"/home/pi/snowboy/resources/rob_release_a.pmdl",
"/home/pi/snowboy/resources/rob_release_b.pmdl"])

# capture SIGINT signal, e.g., Ctrl+C
signal.signal(signal.SIGINT, signal_handler)

detector = snowboydecoder.HotwordDetector(models, sensitivity=0.5, audio_gain=1.75)
callbacks = [lambda: rob_press_a(),
             lambda: rob_press_b(),
			 lambda: rob_release_a(),
			 lambda: rob_release_b()]
print('Listening... Press Ctrl+C to exit')

# main loop
# make sure you have the same numbers of callbacks and models
detector.start(detected_callback=callbacks,
               interrupt_check=interrupt_callback,
               sleep_time=0.03)

detector.terminate()



