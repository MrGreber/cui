# CUI — The C User Interface Framework ⚡️

> ⚠️ **Work in Progress** — This project is still in active development and far from finished. Expect breaking changes, missing features, and rough edges throughout. I am building this in my spare time so progress is steady but slow.

## Why CUI?
I started CUI as a project to sharpen my C programming skills and push them further. I am building this with performance and lightweight UI in mind — the kind of programs that don't shove unnecessary bloat down your throat like most applications do nowadays. My overall goal is to get as close as I can to an HTML/CSS design workflow in pure C. It might be farfetched, but I'd still like to try.

<sub>For any questionable error/debug messages from the shaders module talk to - [Sizer - Github](https://github.com/Sizerino)/[Sizer - Codeberg](https://codeberg.org/SizeR)</sub>

---

## 🚀 Features
- **Minimal & Fast** — Written purely in C with zero external dependencies (unless you count stb_image, which I'll eventually replace with something I wrote myself).
- **Flexible Components** — Button, canvas, edit, panel, captions, and many more I have yet to implement, plus the ability to define your own.
- **Cross-Platform Ready** — Currently building on Windows and planning Linux support in the future. Mac can go to hell.

---

## ⚠️ Known Limitations

- **Text Rendering** — Text rendering is not in its best spot right now. I recently added an optimization I am still actively working on to make it faster. On top of that, the font system currently only loads a single font type from a font atlas, which means it needs a lot more work before it is anywhere near where I want it to be.

---

## 📋 Example

Here is what a basic CUI application looks like right now:

```c
#include <frame.h>
#include <error.h>
#include <panel.h>
#include <app.h>
#include <edit.h>
#include <button.h>
#include <canvas.h>

#define WIDTH 800
#define HEIGHT 800

static bool __init(app_t* app) {
    frame_t* frame = Frame(new)(DARK_GRAY, WIDTH, HEIGHT, "Frame");
    if (!frame) return false;

    style_group_t group = {
        .normal = {
            .modes = EMBEDDED_POPUP | CAPTION,
            .count = 4,
            .init = true,
            .background = {
                .type = BG_TEST,
                .mask = MAGENTA
            },
            .border = {
                .color = BLACK,
                .thickness = 1,
                .radius = 0
            },
        },
        .hover = {
            .init = true,
            .background = {
                .mask = {200, 200, 200, 255}
            }
        }
    };
    panel_t* panel = Panel(new)(frame, &group, &(bounding_box){200, 100, 600, 600});
    group.normal.background.type = BG_COLOR;
    group.normal.background.color = WHITE;

    canvas_t* canvas = Canvas(new)(panel, 400, 600);
    Canvas(set_brush)(canvas, 0, RED, 4);
    Canvas(set_brush)(canvas, 1, TRANSP, 4);

    group.normal.background.color = WHITE;
    group.normal.modes = WRITABLE;
    edit_t* edit = Edit(new)(frame, &group, &(bounding_box){0, 0, 200, 100});
    Font(set)(edit->font, __DIR__"\\Resources\\vcr_osd_mono.fnt", BLUE, TRANSP);
    Edit(set_text)(edit, "shit", 4);

    group.normal.background.type = BG_COLOR;
    group.normal.background.color = WHITE;
    group.normal.background.mask = WHITE;
    button_t* button = Button(new)(frame, &group, &(bounding_box){0, 700, 100, 100});

    app->frame = frame;
    Component(print_node)(frame->header.components, 0);
    return true;
}

void test(void) {
    Error(init)(NULL);
    app_t* app = App(new)((app_init_t)__init, NULL, NULL);
    if (app) {
        App(start)(app);
        App(exit)(app);
    }
    Error(dump)();
}
```

## 🖼️ Preview

![CUI Preview](https://github.com/MrGreber/cui/blob/main/example.JPG)

## ⚙️ Getting Started

Clone the repo:
```bash
git clone https://github.com/MrGreber/cui.git
cd cui
```

Build with CMake:
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## 📜 License
This project is licensed under the **Apache License 2.0**. See the [LICENSE](LICENSE) file for details.

```
   Copyright 2026 MrGreber

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       https://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
```
