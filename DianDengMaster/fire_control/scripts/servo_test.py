import sys
import argparse
import termios
import tty

import serial

STEP = 50
FLIP_X = False
FLIP_Y = True

SERVO_MIN = 500
SERVO_MAX = 2500


def main():
    parser = argparse.ArgumentParser(description="WASD servo test")
    parser.add_argument("--port", default="/dev/ttyUSB0")
    parser.add_argument("--step", type=int, default=STEP)
    args = parser.parse_args()

    ser = serial.Serial(args.port, 115200, timeout=0.1)
    print(f"connected: {args.port} @115200")

    flip_x = -1 if FLIP_X else 1
    flip_y = -1 if FLIP_Y else 1

    print("a/d: X left/right   w/s: Y up/down   q: quit")
    print("if direction is reversed, flip FLIP_X/FLIP_Y at top of this file")

    servo_x = 1500
    servo_y = 1500

    def send(dx, dy):
        nonlocal servo_x, servo_y
        servo_x = max(SERVO_MIN, min(SERVO_MAX, servo_x - dx))
        servo_y = max(SERVO_MIN, min(SERVO_MAX, servo_y + dy))
        ser.write(f"{dx},{dy}\n".encode())
        print(f"send {dx},{dy}  -> x={servo_x}, y={servo_y}")

    fd = sys.stdin.fileno()
    old = termios.tcgetattr(fd)
    tty.setcbreak(fd)

    try:
        while True:
            ch = sys.stdin.read(1).lower()
            if ch == "q":
                break
            elif ch == "a":
                send(-args.step * flip_x, 0)
            elif ch == "d":
                send(args.step * flip_x, 0)
            elif ch == "w":
                send(0, -args.step * flip_y)
            elif ch == "s":
                send(0, args.step * flip_y)
    except KeyboardInterrupt:
        pass
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old)
        ser.close()
        print("bye")


if __name__ == "__main__":
    main()
