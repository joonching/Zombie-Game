# Zombie-Game
Created a zombie game on my Cerebot MX7 cK board using the PIC32 peripheral  and PmodACL 3-axis accelerometer

Used SPI to communicate with the accelerometer.
To play this game you need Cerebot MX7 cK board and a PmodACL 3-axis accelerometer and a PmodOLED to connect in the ports.

LAB 4: Documentation

OLED display:
Top right corner = score (will reset to 0 if score exceeds 1000 but final score still displays 
full score + the thousands.)
"H" = represents the hearts you've collected
"T" = Teleport feature (maxed at 3)
Human = looks like human
Zombie = glyph that looks like pacman
heart = looks like a heart

Sequence of play:
Splash -> difficulty screen -> choose player -> loading screen -> playing field (if obtain heart -> power mode)
-> (if eaten) game over


Basic instructions:
The game is a zombie game where you run around for as long as you can. The longer you live, the higher
your score. There will be hearts that show itself throughout the game. Collecting the hearts (based on
difficulty) will take you to a "power mode" where you can pick up as many hearts as you can to get a
higher score. The power mode will only last around 3 seconds and the accelerometer is really sensitive.

To choose player: tilt the ADXL left to go left and right to go right.

To move the human: tilt the ADXL in the -x position to go vertically down and the +x position to go up on the led
		   To go right tilt the adxl in the -y position and to go left tilt the adxl in the +y orientation.
		   The higher/lower the tilt the faster the human will move.
To Teleport: press BTN3 at any time during the game to teleport to the bottom right side of the LED however you
	     can only use the button 3 times. 

EXTRA:
Plug&play
	- If nothing is plugged in it'll just show score board
LED Animations
The glyphs:
	- The zombie/pacman will change its head orientation depending on what direction its going
	- The human will change its orientation depending on the direction it moves
The difficulty levels (the speed of zombie is proportional to the difficulty)
	- 12 & 13 are regular modes (1 zombie)
	- 14 MED difficulty (2 zombie)
	- 15 HARD difficulty (3 zombie)
Choosing players
	- MU = Mudabir
	- GO = Gondhalekar
	- PA = Professor Patterson
The hearts:
	- During gameplay there will be hearts that spawn and disappear. If you obtain a heart then your score will
	  multiply by 1.5. If you obtain a specified number of hearts depending on difficulty (REG = 5, MED = 3, HARD = 2)
	  then you will enter a power mode (described below)
Power Mode:
	- If you obtain the specified number of hearts depending on difficulty you will enter a powermode in which
	  for 3~ seconds there are no zombies and a bunch of hearts. It's a chance to get a lot of points. After
	  3~ seconds the screen will return to the original state of the playing field.
Score Board:
	- There are 3 scores allowed (PA, MU, GO). Each one will get updated if you play that character. Only if 
	  your score is higher than the previous score will it update.
