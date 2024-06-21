from Broker import Broker

import pygame
import json

colors = {
    "white": pygame.Color('white'),
    "red": pygame.Color('red'),
    "green": pygame.Color('green'),
    "blue": pygame.Color('blue'),
    "black": pygame.Color('black'),
    "unknown": pygame.Color('gray')
}

sizes = {
    "big": 40,
    "small": 20,
}

messages = []


# Define the message handler function
def messageHandler(client, userdata, message):
    global messages
    # This function will be called when a message is received
    # Implement your message handling logic here
    print(message.payload)
    d = json.loads(message.payload)
    scale = 3
    d["x"] *= scale
    d["y"] *= scale
    messages.append(d)


# Initialize Pygame
pygame.init()

# Instantiate the Broker class with the provided arguments
broker = Broker(
    "mqtt.ics.ele.tue.nl",  # MQTT broker host
    "Student37",  # Username
    "tohfe0aY",  # Password
    ["/pynqbridge/19/recv", "/pynqbridge/19/send",
     "/pynqbridge/57/recv", "/pynqbridge/57/send"],  # Topics
    messageHandler  # Message handler function
)

# Set up the display
width, height = 1200, 600
screen = pygame.display.set_mode((width, height))
pygame.display.set_caption("Venus UI")

# Set up the origin point
origin_x, origin_y = width // 2, height // 2
move_speed = 10

img = pygame.image.load("LEGEND_UI_VENUS_V2.png").convert()
img = pygame.transform.scale(img, (800, 600))
img = img.convert()


def draw_axes():
    # Draw x-axis
    pygame.draw.line(screen, (0, 0, 0), (0, origin_y), (width, origin_y), 2)

    # Draw y-axis
    pygame.draw.line(screen, (0, 0, 0), (origin_x, 0), (origin_x, height), 2)


def draw_cube(x, y, size=10, color=(255, 0, 0)):
    pygame.draw.rect(screen, color, pygame.Rect(
        x - size/2 + origin_x, -y - size/2 + origin_y, size, size))


def draw_hole(x, y, size=5, color=(0, 0, 0)):
    pygame.draw.circle(screen, color, (x + origin_x, -y + origin_y), size)


def draw_mountain(x, y, size=10, color=(255, 255, 255)):
    pygame.draw.circle(screen, color, (x + origin_x, -y + origin_y), size)


# Run the game loop
running = True
display_help = False
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

        if event.type == pygame.KEYDOWN:
            match event.key:
                case pygame.K_UP:
                    origin_y -= move_speed
                case pygame.K_DOWN:
                    origin_y += move_speed
                case pygame.K_LEFT:
                    origin_x -= move_speed
                case pygame.K_RIGHT:
                    origin_x += move_speed
                case pygame.K_h | pygame.K_SLASH:
                    display_help = not display_help
                case pygame.K_q:
                    running = False

    # Fill the screen with the background color
    screen.fill((128, 128, 128))

    # Draw the axes and the point
    # draw_axes()
    for message in messages:
        x = message["x"]
        y = message["y"]

        if message["type"] == "cube":
            color = colors[message["color"]]
            size = sizes[message["size"]]
            draw_cube(x, y, size=size, color=color)

        if message["type"] == "hole":
            draw_hole(x, y)

        if message["type"] == "mountain":
            draw_mountain(x, y)

    if display_help:
        screen.blit(img, (0, 0))

    # Update the display
    pygame.display.flip()

# Quit Pygame
pygame.quit()
