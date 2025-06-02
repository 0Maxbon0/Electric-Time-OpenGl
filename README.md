# Electric Time

**Electric Time** is a 2D side-scrolling survival game built using C++ and OpenGL (GLUT). The player chooses between two quirky characters—**BMO** and **REPO**—and must run continuously to survive falling rain and incoming rockets while collecting power-ups to restore health.

## 🎮 Features

* Character selection: BMO or REPO with unique designs.
* Dynamic sky themes: Night, day, and sunset cycles.
* Parallax background and animated scenery.
* Obstacles: Rain and rockets.
* Power-ups: Health recovery items.
* Visual health system using a battery-style indicator.
* Persistent high scores (AppData support on Windows).
* Game states: Start, character selection, playing, paused, and game over.
* Invincibility frames and movement animation.

## 🛠️ Requirements

* C++ compiler
* OpenGL and GLUT libraries
* Windows or Linux (AppData save only on Windows)

## 🚀 Building and Running

### Linux/macOS:

```bash
g++ main.cpp -o ElectricTime -lGL -lGLU -lglut
./ElectricTime
```

### Windows (MinGW):

```bash
g++ main.cpp -o ElectricTime -lglu32 -lopengl32 -lfreeglut
ElectricTime.exe
```

## 🕹️ Controls

* `Enter` — Start the game
* `1` / `2` — Select character
* `Arrow keys` — Move and jump
* `R` — Restart after Game Over
* `Esc` — Pause/Resume game

## 📁 File Structure

* `main.cpp` — Complete source code with rendering, game logic, character management, and I/O.
* `README.md` — This file.
* `Electric Time Game.mp4` — Gameplay demo video.
* `bin/`, `obj/` — Build output directories.
* `game`, `game.depend`, `game.layout` — Project/IDE files.

## 📽️ Gameplay Demo

You can view a short gameplay demo here:
[Electric Time Game Demo](./Electric%20Time%20Game.mp4)

## 📄 .gitignore Suggestion

To avoid committing unnecessary files, include the following in a `.gitignore` file:

```gitignore
bin/
obj/
*.depend
*.layout
*.mp4
```

## 👥 Authors

* **Maxim Mamdouh Salib**
* **Abdallah Salah Elsaid**

---

Enjoy dodging rockets and dancing in the rain—good luck surviving **Electric Time**!
