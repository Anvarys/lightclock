import pcbnew
import math

board = pcbnew.GetBoard()
led = 1

center_x = pcbnew.FromMM(100)
center_y = pcbnew.FromMM(100)
radius = pcbnew.FromMM(62)

leds = []

for footprint in board.GetFootprints():
	if footprint.GetReference().upper().startswith("LED"):
		leds.append(footprint)

leds.sort(key=lambda x: int(x.GetReference()[3:]))

for i, fp in enumerate(leds):
	angle = (2 * math.pi * i) / len(leds) + math.pi/len(leds) - math.pi * 0.5
	x = center_x + radius * math.cos(angle)
	y = center_y + radius * math.sin(angle)

	fp.SetPosition(pcbnew.VECTOR2I(int(x), int(y)))
	
	dx = x - center_x
	dy = y - center_y
	
	rotation = math.degrees(angle)
	if rotation > 180:
		rotation = 360-rotation
	else:
		rotation = -rotation
	fp.SetOrientation(pcbnew.EDA_ANGLE(rotation, pcbnew.DEGREES_T))

pcbnew.Refresh()
