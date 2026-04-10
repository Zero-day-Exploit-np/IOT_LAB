from picamera2 import Picamera2
import time

camera = Picamera2()

# Configure camera
config = camera.create_still_configuration()
camera.configure(config)

camera.start()
time.sleep(2)  # Warm up the camera

camera.capture_file("photo.jpg")
print("Photo saved as photo.jpg")

camera.stop()
