# Electric Time: Building and Owning the Render Loop

Most small games get shipped faster in Unity or Unreal. I built **Electric Time** in raw OpenGL/GLUT because I wanted full control over frame execution, state transitions, and draw order.  
Not editor workflows. Not prefab tooling. The point was to own the rendering pipeline end-to-end.

This repo is a 2D real-time game loop implemented in `main.cpp` using FreeGLUT and fixed-function OpenGL. The core lifecycle is explicit:

1. **Initialization**  
   `main()` creates the window (`glutCreateWindow`), initializes projection state in `initia()`, builds background data in `initBackground()`, loads persisted score in `loadHighScore()`, and registers callbacks (`display`, `keyboard`, `specialKeys`, `GamePage`).
2. **Main Render Loop**  
   `GamePage(int)` runs every ~16 ms via `glutTimerFunc(16, GamePage, 0)`, updates simulation state, and requests redraw with `glutPostRedisplay()`.  
   `display()` is the render pass: clear, draw scene by `gameState`, and present with `glutSwapBuffers()`.
3. **Cleanup**  
   FreeGLUT owns process teardown after `glutMainLoop()`. Persistent gameplay state is explicitly flushed through `saveHighScore()`.

---

## State Management & the Render Loop

OpenGL is a state machine, and this code leans on that directly. The main render-state decisions are:

- **Projection state**: `initia()` sets orthographic projection once:
  - `glMatrixMode(GL_PROJECTION)`
  - `gluOrtho2D(screenLeft, screenRight, screenBottom, screenTop)`
- **Per-frame clear**: `display()` starts with `glClear(GL_COLOR_BUFFER_BIT)`.
- **Transform stack discipline**: character parts use `glPushMatrix()` / `glPopMatrix()` around local transforms (`glTranslatef`, `glRotatef`) to avoid contaminating global transforms.
- **Gameplay state isolation**: `gameState` gates rendering and input paths (`START`, `SELECT`, `PLAYING`, `GAME_OVER`, `PAUSED`), so paused/over screens don’t accidentally run simulation updates.

### About VAOs/VBOs in this repository

This codebase uses immediate mode (`glBegin/glEnd`) and CPU-side containers (`std::vector<Rain>`, `std::vector<Rocket>`, `std::vector<PowerUp>`), so there are **no VAO/VBO allocations to leak** in the current implementation.  
Object lifetime is managed with erase patterns like:

- `Rains.erase(it)`
- `Rockets.erase(it)`
- `powerUps.erase(remove_if(...), powerUps.end())`

In a modernized core-profile path, the equivalent leak-prevention rule would be: create VAO/VBO once at init, bind minimally per draw, and always delete in shutdown (`glDeleteVertexArrays`, `glDeleteBuffers`).

---

## The "Electric Time" Shader Logic

Current repo state: rendering is fixed-function (no GLSL files checked in), and timing is driven by `glutGet(GLUT_ELAPSED_TIME)` in gameplay systems (`GamePage`, invincibility flashing, timer UI).

If you run the same effect through a programmable pipeline, the CPU→GPU time flow is:

1. Read time on CPU (`glfwGetTime()` or equivalent)
2. Upload as uniform (`u_time`)
3. Use in fragment shader for animated color/phase effects

### C++ uniform upload snippet (programmable path)

```cpp
float t = static_cast<float>(glfwGetTime());
glUseProgram(programId);
GLint timeLoc = glGetUniformLocation(programId, "u_time");
glUniform1f(timeLoc, t);
```

### GLSL fragment shader snippet

```glsl
#version 330 core
in vec2 v_uv;
out vec4 FragColor;
uniform float u_time;

void main() {
    float pulse = 0.5 + 0.5 * sin(u_time * 2.0 + v_uv.x * 12.0);
    vec3 base = vec3(0.05, 0.35, 0.9);
    vec3 hot  = vec3(0.2, 0.95, 1.0);
    FragColor = vec4(mix(base, hot, pulse), 1.0);
}
```

---

## Matrix Transformations (MVP)

Electric Time currently uses fixed-function matrix stacks, but the math still maps cleanly to MVP.

- **Model**: per-object local transforms (`glTranslatef`, `glRotatef`) in `drawBMO()` and `drawREPO()` for limb animation.
- **View**: effectively identity in this 2D setup (camera is not independently transformed).
- **Projection**: orthographic volume from `gluOrtho2D(screenLeft, screenRight, screenBottom, screenTop)`.

Equivalent conceptual pipeline:

`clipPos = Projection * View * Model * localPos`

Because projection bounds are explicit (`screenLeft/right/top/bottom`), gameplay coordinates and render coordinates stay tightly coupled, which simplifies collision and HUD placement.

---

## Debugging "Black Screens"

The two fastest ways to get a black frame are bad state and silent shader failure.

### In this repository (fixed-function path)

What I check first:

1. `display()` is still registered and firing.
2. `glClear` + `glutSwapBuffers` are both reached.
3. Projection bounds (`gluOrtho2D`) still contain object coordinates.
4. Every `glBegin` has a matching `glEnd`.
5. Draw color/state changes are not unintentionally inherited.

### In shader-based builds

I always fail fast on compile/link status:

```cpp
GLint ok = 0;
glCompileShader(shader);
glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
if (!ok) {
    GLint logLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
    std::string log(logLen, '\0');
    glGetShaderInfoLog(shader, logLen, nullptr, log.data());
    std::cerr << "Shader compile failed:\n" << log << std::endl;
}
```

And in debug contexts, enable `glDebugMessageCallback` so API errors are surfaced immediately instead of manifesting as blank output later.

---

## Build Instructions

### CMake build (standard flow)

```bash
git clone https://github.com/0Maxbon0/Electric-Time-OpenGl.git
cd Electric-Time-OpenGl
cmake -S . -B build
cmake --build build --config Release
```

### Run

```bash
./build/Electric-Time-OpenGl
```

On Windows (Visual Studio generator), run the produced `.exe` from `build/Release/`.

---

## Project Notes

- Core source file: `main.cpp`
- Window/input/runtime loop: FreeGLUT
- Render style: fixed-function immediate mode
- Timing source in gameplay loop: `glutGet(GLUT_ELAPSED_TIME)`
- Persistence: high score file via `loadHighScore()` / `saveHighScore()`

This project intentionally keeps the loop explicit so rendering, simulation, and state transitions are inspectable without engine abstraction.
