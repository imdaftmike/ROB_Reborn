# Very quick/rough code to puppeteer ROB using a controller with crosshair to help align the projector

from __future__ import division
import pygame
import time
import Adafruit_PCA9685
import smbus
import os

# i2c setup for the arduino controlling ROBs original motors
bus = smbus.SMBus(3)
address = 0x04

def writeNumber(value):
	bus.write_byte(address, value)
	return -1

def readNumber():
	number = bus.read_byte(address)
	return number

# map function for analog sticks	
def mapFromTo(x,a,b,c,d):
   y=(x-a)/(b-a)*(d-c)+c
   return y
   
# Define some colors
BLACK    = (   0,   0,   0)
WHITE    = ( 255, 255, 255)
   
pygame.init()
 
# Set the width and height of the screen [width,height]
size = [854, 480]
screen = pygame.display.set_mode(size)
pygame.mouse.set_visible(False)
  
# Draw crosshair
screen.fill(BLACK)
pygame.draw.rect(screen, WHITE, (0,0,854,480), 5)
pygame.draw.line(screen, BLACK, (75,0), (779,0), 5)
pygame.draw.line(screen, BLACK, (75,479), (779,479), 5)
pygame.draw.line(screen, BLACK, (0,75), (0,405), 5)
pygame.draw.line(screen, BLACK, (853,75), (853,405), 5)
pygame.draw.line(screen, WHITE, (250,240), (604,240), 5)	# horizontal line
pygame.draw.line(screen, WHITE, (427,100), (427,380), 5)	# vertical line

# update the screen
pygame.display.flip()


# Initialise the PCA9685 using alternate i2c pins:
pwm = Adafruit_PCA9685.PCA9685(address=0x40, busnum=3)

# Get the head tilt position from file
settings = open("/home/pi/mike/rob_settings.txt", "r")
head_tilt_pos = int(settings.read(3))

# Turn ROBs led on
pwm.set_pwm(4, 0, 4000)

# Configure min and max servo pulse lengths (out of 4096)
head_pan_min = 210
head_pan_max = 540
pwm.set_pwm(0, 0, 375)
time.sleep(0.5)

head_tilt_min = 460
head_tilt_max = 510
pwm.set_pwm(1, 0, head_tilt_pos)
time.sleep(0.5)

body_pan_min = 230
body_pan_max = 600
pwm.set_pwm(3, 0, 415)
time.sleep(0.5)

# Variables for head/body control
head_freeze = False
body_follow = False
		

# Helper function to make setting a servo pulse width simpler.
def set_servo_pulse(channel, pulse):
    pulse_length = 1000000    # 1,000,000 us per second
    pulse_length //= 60       # 60 Hz
    print('{0}us per period'.format(pulse_length))
    pulse_length //= 4096     # 12 bits of resolution
    print('{0}us per bit'.format(pulse_length))
    pulse *= 1000
    pulse //= pulse_length
    pwm.set_pwm(channel, 0, pulse)

# Set frequency to 60hz, good for servos.
pwm.set_pwm_freq(60)

#Loop until the user presses start+select.
done = False

# Used to manage how fast the screen updates
clock = pygame.time.Clock()

# Initialize the joysticks
pygame.joystick.init()

# -------- Main Program Loop -----------
while done==False:
	try:
		# EVENT PROCESSING STEP
		for event in pygame.event.get(): # User did something
			if event.type == pygame.QUIT: # If user clicked close
				done=True # Flag that we are done so we exit this loop
				 
		# Get count of joysticks
		joystick_count = pygame.joystick.get_count()
	   
		# For each joystick:
		for i in range(joystick_count):
			joystick = pygame.joystick.Joystick(i)
			joystick.init()
	
				
			
		# Commands to send to ROBs original motors
		if joystick.get_button(1) == 1:		# a button
			writeNumber(1)					# 'open'
			time.sleep(0.25)
			
		if joystick.get_button(0) == 1:		# b button
			writeNumber(2)					# 'close'
			time.sleep(0.25)
			
		if joystick.get_button(13) == 1:	# dpad up
			writeNumber(3)					# 'up'
			time.sleep(0.25)
			
		if joystick.get_button(14) == 1:	# dpad down
			writeNumber(4)					# 'down'
			time.sleep(0.25)
			
		if joystick.get_button(10) == 1:	# home button
			writeNumber(5)					# 'home'
			time.sleep(0.25)
		


		
		# Tilt ROBs head with the shoulder buttons
		if joystick.get_button(4) == 1:		# l button
			head_tilt_pos += 3
			pwm.set_pwm(1, 0, head_tilt_pos)
			time.sleep(0.05)
		
		if joystick.get_button(5) == 1:		# r button
			head_tilt_pos -= 3
			pwm.set_pwm(1, 0, head_tilt_pos)
			time.sleep(0.05)
		

		# Move ROBs neck/body by mapping analog stick values
		# get thumbstick positions
		x_axis = joystick.get_axis(0)
		y_axis = joystick.get_axis(1)
		x_axis2 = joystick.get_axis(2)
		
		# map the thumbsticks values to servo positions
		head_pan_servo = int(mapFromTo(x_axis,-1,1,head_pan_max,head_pan_min))
		body_pan_servo = int(mapFromTo(x_axis2,-1,1,body_pan_min,body_pan_max))			
		
		# update the servos
		if head_freeze == 0:
			pwm.set_pwm(0, 0, head_pan_servo)
		if head_freeze == 0 and body_follow == 1:
			body_pan_servo = int(mapFromTo(x_axis,-1,1,body_pan_min,body_pan_max))
			pwm.set_pwm(3, 0, body_pan_servo)
		if body_follow == 0:
			pwm.set_pwm(3, 0, body_pan_servo)
		
		
		# freeze the head position by clicking the right trigger
		if joystick.get_button(7):
			head_freeze = not head_freeze
			time.sleep(0.25)
			
		# toggle body following the head by clicking the right stick
		if joystick.get_button(12):
			body_follow = not body_follow
			time.sleep(0.25)

			
		# print some values to see what's going on
		#print "head:", head_pan_servo, head_freeze, "body:", body_pan_servo, body_follow

		
		# Exit by pressing start+select together
		if joystick.get_button(8) == 1 and joystick.get_button(9) == 1:
			save = open("/home/pi/mike/rob_settings.txt", "w")
			save.write(str(head_tilt_pos))
			os.system("/home/pi/mike/PiProjectorControl.sh enable")
			done=True	
		
		
		
		screen.fill(BLACK)
		pygame.draw.rect(screen, WHITE, (0,0,854,480), 5)

		pygame.draw.line(screen, BLACK, (75,0), (779,0), 5)
		pygame.draw.line(screen, BLACK, (75,479), (779,479), 5)
		pygame.draw.line(screen, BLACK, (0,75), (0,405), 5)
		pygame.draw.line(screen, BLACK, (853,75), (853,405), 5)

		pygame.draw.line(screen, WHITE, (250,240), (604,240), 5)	# horizontal line
		pygame.draw.line(screen, WHITE, (427,100), (427,380), 5)	# vertical line

	
		# update the screen
		pygame.display.flip()

		# 30 fps
		clock.tick(30)
		
	except IOError:
		print "Error with i2c device"
	

   
# Close the window and quit.
# If you forget this line, the program will 'hang'
# on exit if running from IDLE.
pygame.quit ()
