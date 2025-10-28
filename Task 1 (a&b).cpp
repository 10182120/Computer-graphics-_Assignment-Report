#include <GL/freeglut.h>
#include <iostream>
#include <string>
#include <cmath>
using namespace std;

// ==================== CONSTANTS ====================
const int WINDOW_SIZE = 700;
const int AXIS_STEP = 50;
const int CENTER = WINDOW_SIZE / 2;

const int MODE_INPUT = 1;
const int MODE_DRAW = 2;

// ==================== STATE VARIABLES ====================
int current_mode = MODE_INPUT;
int input_stage = 1;
string buffer;

string s_choice, s_x1, s_y1, s_x2, s_y2, s_width;
int choice = 0, x1_val = 0, y1_val = 0, x2_val = 0, y2_val = 0, width_val = 1;
bool ready_to_draw = false;

// ==================== UTILITY FUNCTIONS ====================
int screen_x(int x) { return x + CENTER; }
int screen_y(int y) { return y + CENTER; }

void put_pixel(int x, int y, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_POINTS);
    glVertex2i(screen_x(x), screen_y(y));
    glEnd();
}

void draw_text(float x, float y, float r, float g, float b, const string &text) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (char c : text)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}

int to_int(const string &s) {
    if (s.empty() || s == "-") return 0;
    try { return stoi(s); } catch (...) { return 0; }
}

// ==================== BRESENHAM'S ALGORITHM ====================
void bresenham_line(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        put_pixel(x1, y1, 0, 0, 0);
        if (x1 == x2 && y1 == y2) break;

        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

// ==================== THICK LINE HANDLING ====================
// 8-way symmetric thickness extension
void bresenham_thick_line(int x1, int y1, int x2, int y2, int thickness) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    float length = sqrt(dx * dx + dy * dy);
    float offset_x = -dy / length;
    float offset_y = dx / length;

    for (int i = -thickness / 2; i <= thickness / 2; i++) {
        int start_x = round(x1 + i * offset_x);
        int start_y = round(y1 + i * offset_y);
        int end_x = round(x2 + i * offset_x);
        int end_y = round(y2 + i * offset_y);
        bresenham_line(start_x, start_y, end_x, end_y);
    }
}

// ==================== DRAWING HELPERS ====================
void draw_axes() {
    glColor3f(0, 0, 1);
    glBegin(GL_LINES);
    glVertex2i(0, CENTER);
    glVertex2i(WINDOW_SIZE, CENTER);
    glVertex2i(CENTER, 0);
    glVertex2i(CENTER, WINDOW_SIZE);
    glEnd();

    for (int c = -CENTER + AXIS_STEP; c < CENTER; c += AXIS_STEP) {
        if (c == 0) continue;
        int sx = screen_x(c);
        int sy = screen_y(c);
        glBegin(GL_LINES);
        glVertex2i(sx, CENTER - 3);
        glVertex2i(sx, CENTER + 3);
        glVertex2i(CENTER - 3, sy);
        glVertex2i(CENTER + 3, sy);
        glEnd();
    }
}

// ==================== INPUT SCREEN ====================
void draw_input_screen() {
    glClearColor(0.9, 0.9, 0.9, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    string title = "Bresenham’s Line Drawing - Input Mode";
    draw_text(100, 620, 0, 0, 0, title);

    string prompt;
    switch (input_stage) {
        case 1: prompt = "Enter 1 for Normal Line or 2 for Thick Line: "; break;
        case 2: prompt = "Enter Start Point X1: "; break;
        case 3: prompt = "Enter Start Point Y1: "; break;
        case 4: prompt = "Enter End Point X2: "; break;
        case 5: prompt = "Enter End Point Y2: "; break;
        case 6: prompt = "Enter Line Thickness (>0): "; break;
    }

    draw_text(100, 500, 0.2, 0.2, 0.8, prompt + buffer + "_");
    glFlush();
}

// ==================== INPUT PROCESSING ====================
void handle_input() {
    if (buffer.empty() && input_stage != 1) return;

    switch (input_stage) {
        case 1:
            if (buffer == "1" || buffer == "2") { s_choice = buffer; input_stage = 2; }
            else cerr << "Invalid input! Enter 1 or 2.\n";
            break;
        case 2: s_x1 = buffer; input_stage = 3; break;
        case 3: s_y1 = buffer; input_stage = 4; break;
        case 4: s_x2 = buffer; input_stage = 5; break;
        case 5:
            s_y2 = buffer;
            if (s_choice == "1") { ready_to_draw = true; current_mode = MODE_DRAW; }
            else input_stage = 6;
            break;
        case 6: s_width = buffer; ready_to_draw = true; current_mode = MODE_DRAW; break;
    }

    buffer.clear();

    if (current_mode == MODE_DRAW) {
        choice = to_int(s_choice);
        x1_val = to_int(s_x1);
        y1_val = to_int(s_y1);
        x2_val = to_int(s_x2);
        y2_val = to_int(s_y2);
        width_val = (choice == 2) ? max(1, to_int(s_width)) : 1;

        cout << "Drawing Line: P1(" << x1_val << "," << y1_val << ") → P2(" << x2_val << "," << y2_val
             << "), Width = " << width_val << endl;
    }

    glutPostRedisplay();
}

// ==================== KEYBOARD ====================
void keyboard(unsigned char key, int, int) {
    if (current_mode != MODE_INPUT) return;

    if (key == 13) handle_input(); // Enter
    else if (key == 8 || key == 127) { // Backspace
        if (!buffer.empty()) buffer.pop_back();
    }
    else if ((key >= '0' && key <= '9') || key == '-') {
        if (key == '-' && !buffer.empty()) return;
        if (buffer.size() < 5) buffer += key;
    }

    glutPostRedisplay();
}

// ==================== DISPLAY ====================
void display() {
    if (current_mode == MODE_INPUT) {
        draw_input_screen();
    } else {
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        draw_axes();

        if (ready_to_draw) {
            if (choice == 1)
                bresenham_line(x1_val, y1_val, x2_val, y2_val);
            else
                bresenham_thick_line(x1_val, y1_val, x2_val, y2_val, width_val);
        }

        draw_text(10, 10, 0.5, 0, 0, "Line Drawn! Close the window to exit.");
        glFlush();
    }
}

// ==================== INIT & MAIN ====================
void initGL() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_SIZE, 0, WINDOW_SIZE);
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_SIZE, WINDOW_SIZE);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Bresenham’s Line Drawing - Version 2");

    initGL();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}
