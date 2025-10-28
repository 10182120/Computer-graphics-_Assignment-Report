#include <GL/freeglut.h>
#include <cmath>
#include <iostream>
using namespace std;

// =================== CONSTANTS ===================
const int WIN_SIZE = 700;
const int CENTER = WIN_SIZE / 2;
const int TOTAL_CIRCLES = 80;
const int START_RADIUS = 20;
const int END_RADIUS = 300;
const float MAX_POINT_SIZE = 6.0f;

// =================== HSV → RGB FUNCTION ===================
void hsvToRgb(float h, float s, float v, float &r, float &g, float &b) {
    if (s == 0.0f) { r = g = b = v; return; }

    h = fmod(h, 360.0f) / 60.0f;
    int i = int(floor(h));
    float f = h - i;
    float p = v * (1 - s);
    float q = v * (1 - s * f);
    float t = v * (1 - s * (1 - f));

    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }
}

// =================== COLOR GRADIENT ===================
void setCircleColor(int index) {
    float progress = float(index) / (TOTAL_CIRCLES - 1);
    float hue = 240.0f * (1.0f - progress);  // Blue → Red transition
    float sat = 1.0f;
    float val = 0.9f - 0.4f * progress;      // Slight dimming toward edge
    float r, g, b;
    hsvToRgb(hue, sat, val, r, g, b);
    glColor3f(r, g, b);
}

// =================== PIXEL DRAWING ===================
void putPixel(int x, int y, int cx, int cy, float size) {
    glPointSize(size);
    glBegin(GL_POINTS);
    glVertex2i(cx + x, cy + y);
    glEnd();
}

// =================== 8-WAY SYMMETRY ===================
void plotCirclePoints(int cx, int cy, int x, int y, float size) {
    putPixel(x, y, cx, cy, size);
    putPixel(-x, y, cx, cy, size);
    putPixel(x, -y, cx, cy, size);
    putPixel(-x, -y, cx, cy, size);
    putPixel(y, x, cx, cy, size);
    putPixel(-y, x, cx, cy, size);
    putPixel(y, -x, cx, cy, size);
    putPixel(-y, -x, cx, cy, size);
}

// =================== MIDPOINT CIRCLE ALGORITHM ===================
void drawCircleMidpoint(int cx, int cy, int radius, float thickness) {
    int x = 0;
    int y = radius;
    int p = 1 - radius;

    plotCirclePoints(cx, cy, x, y, thickness);

    while (x < y) {
        x++;
        if (p < 0) p += 2 * x + 1;
        else { y--; p += 2 * (x - y) + 1; }
        plotCirclePoints(cx, cy, x, y, thickness);
    }
}

// =================== DISPLAY ===================
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    int step = (END_RADIUS - START_RADIUS) / (TOTAL_CIRCLES - 1);

    for (int i = 0; i < TOTAL_CIRCLES; i++) {
        int r = START_RADIUS + i * step;
        float t = float(i) / (TOTAL_CIRCLES - 1);
        float thickness = 1.0f + t * (MAX_POINT_SIZE - 1.0f);
        setCircleColor(i);
        drawCircleMidpoint(CENTER, CENTER, r, thickness);
    }

    glFlush();
}

// =================== INIT ===================
void init() {
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_SIZE, 0, WIN_SIZE);

    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
}

// =================== MAIN ===================
int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WIN_SIZE, WIN_SIZE);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Gradient Concentric Circles - Version 2");

    init();
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}
