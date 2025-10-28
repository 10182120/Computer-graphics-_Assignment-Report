#include <GL/freeglut.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <sstream>
using namespace std;

// =================== WINDOW SETTINGS ===================
const int WIN_WIDTH = 1200;
const int WIN_HEIGHT = 800;
const int HEADER_H = 50;

const float LOGICAL_XRANGE = 600.0f;
const float LOGICAL_YRANGE = 350.0f;

enum Mode { SET_WINDOW = 1, DRAW_LINES, SHOW_CLIPPED };

// =================== STRUCTURES ===================
struct Point { float x, y; };
struct Segment { Point a, b; };

// =================== GLOBALS ===================
Mode mode = SET_WINDOW;
vector<Segment> allLines;
vector<Point> clippedPts;

float x_min = 0, y_min = 0, x_max = 0, y_max = 0;
int clickStage = 0;
bool firstClick = true;
Point startPt;

float originX, originY, scaleX, scaleY;

// =================== HELPER FUNCTIONS ===================
Point toScreen(float lx, float ly) {
    return { originX + lx * scaleX, originY + ly * scaleY };
}

Point toLogical(float sx, float sy) {
    return { (sx - originX) / scaleX, (sy - originY) / scaleY };
}

void drawText(float x, float y, const string &txt, void *font, float r=0, float g=0, float b=0) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (char c : txt) glutBitmapCharacter(font, c);
}

// =================== LIANG–BARSKY ALGORITHM ===================
bool liangBarsky(float x0, float y0, float x1, float y1,
                 float &cx0, float &cy0, float &cx1, float &cy1) {
    float dx = x1 - x0;
    float dy = y1 - y0;
    float p[4] = { -dx, dx, -dy, dy };
    float q[4] = { x0 - x_min, x_max - x0, y0 - y_min, y_max - y0 };

    float t0 = 0.0f, t1 = 1.0f;

    for (int i = 0; i < 4; i++) {
        if (fabs(p[i]) < 1e-6) {
            if (q[i] < 0) return false; // Parallel and outside
        } else {
            float t = q[i] / p[i];
            if (p[i] < 0) t0 = max(t0, t);
            else t1 = min(t1, t);
        }
    }

    if (t0 > t1) return false;

    cx0 = x0 + t0 * dx;
    cy0 = y0 + t0 * dy;
    cx1 = x0 + t1 * dx;
    cy1 = y0 + t1 * dy;
    return true;
}

// =================== DRAWING HELPERS ===================
void drawAxes() {
    glColor3f(0.8f, 0.8f, 0.8f);
    glBegin(GL_LINES);
    glVertex2f(0, originY);
    glVertex2f(WIN_WIDTH, originY);
    glVertex2f(originX, 0);
    glVertex2f(originX, WIN_HEIGHT - HEADER_H);
    glEnd();
}

void drawClipWindow() {
    Point p1 = toScreen(x_min, y_min);
    Point p2 = toScreen(x_max, y_min);
    Point p3 = toScreen(x_max, y_max);
    Point p4 = toScreen(x_min, y_max);

    glColor3f(0, 0, 0);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(p1.x, p1.y);
    glVertex2f(p2.x, p2.y);
    glVertex2f(p3.x, p3.y);
    glVertex2f(p4.x, p4.y);
    glEnd();
}

void drawHeader(const string &title, const string &hint, float r, float g, float b) {
    glColor3f(0.95f, 0.95f, 0.95f);
    glBegin(GL_QUADS);
    glVertex2f(0, WIN_HEIGHT - HEADER_H);
    glVertex2f(WIN_WIDTH, WIN_HEIGHT - HEADER_H);
    glVertex2f(WIN_WIDTH, WIN_HEIGHT);
    glVertex2f(0, WIN_HEIGHT);
    glEnd();

    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_LINES);
    glVertex2f(0, WIN_HEIGHT - HEADER_H);
    glVertex2f(WIN_WIDTH, WIN_HEIGHT - HEADER_H);
    glEnd();

    drawText(15, WIN_HEIGHT - 25, title, GLUT_BITMAP_HELVETICA_18, r, g, b);
    drawText(15, WIN_HEIGHT - 45, hint, GLUT_BITMAP_HELVETICA_12, 0.1, 0.1, 0.1);
}

// =================== DISPLAY MODES ===================
void renderSetWindow() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawHeader("STEP 1: Define Clipping Window",
               "Click two opposite corners of the clipping rectangle", 0, 0.6f, 0);
    drawAxes();

    if (clickStage == 1) {
        Point temp = toScreen(x_min, y_min);
        glColor3f(1, 0, 0);
        glPointSize(6);
        glBegin(GL_POINTS);
        glVertex2f(temp.x, temp.y);
        glEnd();
    }

    glFlush();
}

void renderDrawLines() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawHeader("STEP 2: Draw Line Segments",
               "Click endpoints. Press ENTER to clip, R to reset.", 0.8f, 0.3f, 0);
    drawAxes();
    drawClipWindow();

    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_LINES);
    for (auto &seg : allLines) {
        Point s1 = toScreen(seg.a.x, seg.a.y);
        Point s2 = toScreen(seg.b.x, seg.b.y);
        glVertex2f(s1.x, s1.y);
        glVertex2f(s2.x, s2.y);
    }
    glEnd();

    glFlush();
}

void renderClipped() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawHeader("STEP 3: Liang–Barsky Clipping Result",
               "Red = Original, Green = Visible part, Blue = Intersections, R = Reset",
               0, 0.4f, 0);
    drawAxes();
    drawClipWindow();

    clippedPts.clear();

    for (auto &seg : allLines) {
        // Original (red)
        glColor3f(1, 0, 0);
        glBegin(GL_LINES);
        Point p1 = toScreen(seg.a.x, seg.a.y);
        Point p2 = toScreen(seg.b.x, seg.b.y);
        glVertex2f(p1.x, p1.y);
        glVertex2f(p2.x, p2.y);
        glEnd();

        float cx0, cy0, cx1, cy1;
        if (liangBarsky(seg.a.x, seg.a.y, seg.b.x, seg.b.y, cx0, cy0, cx1, cy1)) {
            Point c1 = toScreen(cx0, cy0);
            Point c2 = toScreen(cx1, cy1);

            glColor3f(0, 0.7f, 0);
            glLineWidth(3);
            glBegin(GL_LINES);
            glVertex2f(c1.x, c1.y);
            glVertex2f(c2.x, c2.y);
            glEnd();

            glColor3f(0, 0, 1);
            glPointSize(7);
            glBegin(GL_POINTS);
            glVertex2f(c1.x, c1.y);
            glVertex2f(c2.x, c2.y);
            glEnd();

            clippedPts.push_back({cx0, cy0});
            clippedPts.push_back({cx1, cy1});
        }
    }

    // Display clipped coordinates
    float y = WIN_HEIGHT - HEADER_H - 30;
    drawText(900, y + 10, "Visible Coordinates:", GLUT_BITMAP_HELVETICA_12);
    ostringstream ss;
    ss << fixed << setprecision(1);
    y -= 15;
    for (size_t i = 0; i < clippedPts.size(); i++) {
        ss.str("");
        ss << "P" << (i+1) << ": (" << clippedPts[i].x << ", " << clippedPts[i].y << ")";
        drawText(900, y, ss.str(), GLUT_BITMAP_HELVETICA_10);
        y -= 15;
        if (y < 50) break;
    }

    glFlush();
}

// =================== DISPLAY CALLBACK ===================
void display() {
    if (mode == SET_WINDOW) renderSetWindow();
    else if (mode == DRAW_LINES) renderDrawLines();
    else renderClipped();
}

// =================== INPUT HANDLERS ===================
void mouse(int button, int state, int x, int y) {
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) return;

    Point logical = toLogical(x, WIN_HEIGHT - y);

    if (mode == SET_WINDOW) {
        if (clickStage == 0) {
            x_min = logical.x; y_min = logical.y;
            clickStage = 1;
        } else {
            x_max = logical.x; y_max = logical.y;
            if (x_min > x_max) swap(x_min, x_max);
            if (y_min > y_max) swap(y_min, y_max);
            mode = DRAW_LINES;
            clickStage = 0;
        }
    } else if (mode == DRAW_LINES) {
        if (firstClick) {
            startPt = logical;
            firstClick = false;
        } else {
            allLines.push_back({startPt, logical});
            startPt = logical;
        }
    }

    glutPostRedisplay();
}

void keyboard(unsigned char key, int, int) {
    if (key == 13 && mode == DRAW_LINES) {  // Enter
        mode = SHOW_CLIPPED;
    } else if (key == 'r' || key == 'R') {  // Reset
        mode = SET_WINDOW;
        allLines.clear();
        clippedPts.clear();
        x_min = y_min = x_max = y_max = 0;
        clickStage = 0;
        firstClick = true;
    } else if (key == ' ') {
        firstClick = true;
    }
    glutPostRedisplay();
}

// =================== INIT ===================
void init() {
    glClearColor(1, 1, 1, 1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_WIDTH, 0, WIN_HEIGHT);

    originX = WIN_WIDTH * 0.35f;
    originY = (WIN_HEIGHT - HEADER_H) / 2.0f;
    scaleX = (WIN_WIDTH * 0.6f) / (2 * LOGICAL_XRANGE);
    scaleY = (WIN_HEIGHT - HEADER_H) / (2 * LOGICAL_YRANGE);
}

// =================== MAIN ===================
int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Liang–Barsky Line Clipping - Version 2");

    init();
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}
