/*
 * Computacao Grafica
 * Paint2D
 * Autor: Ryan Ferreira de Sousa
 */

// Bibliotecas utilizadas pelo OpenGL
#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
    #include <OpenGL/glu.h>
#else
    #include <GL/glut.h>
    #include <GL/gl.h>
    #include <GL/glu.h>
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <stack>
#include <iostream>
#include "glut_text.h"
#include "matrix.hpp"

#define PB push_back
#define SQ(x) (x)*(x)

using namespace std;

#define ESC 27
#define UP 101
#define DOWN 103
#define LEFT 100
#define RIGHT 102

enum form_type { LIN = 1, TRI, REC, POL, CIR, FILL };

bool click1 = false;
int m_x, m_y, x_1, y_1, x_2, y_2;
int modo = LIN;
int width = 512, height = 512;

struct form {
    form_type type;
    vector<pair<int, int>> vertices;
    int radius = 0;

    form() : type(LIN) {}
    form(form_type _type) : type(_type) {}

    void addVertex(int x, int y) {
        vertices.PB({ x, y });
    }

    void addVertex(pair<int, int> vertex) {
        vertices.PB(vertex);
    }

    pair<double, double> getCentroid() {

        // a reta não possui área, o centroide é o ponto médio
        // entre os dois pontos que a descrevem
        if(type == LIN) {
            return { (vertices[0].first + vertices[1].first) / 2.0,
                (vertices[0].second + vertices[1].second) / 2.0};
        }

        double area = 0, soma_x = 0, soma_y = 0;

        auto n = vertices.size();

        // formula das areas de gauss
        for(int i = 0, j; i < n; i++) {
            j = (i + 1) % n;
            area += vertices[i].first * vertices[j].second - vertices[j].first * vertices[i].second;
        }

        for(int i = 0, j; i < n; i++) {
            j = (i + 1) % n;
            soma_x += (vertices[i].first + vertices[j].first) * (vertices[i].first * vertices[j].second - vertices[j].first * vertices[i].second);
            soma_y += (vertices[i].second + vertices[j].second) * (vertices[i].first * vertices[j].second - vertices[j].first * vertices[i].second);
        }

        return { soma_x / (3 * area), soma_y / (3 * area) };
    }

};

vector<form> forms;
form* openPolygon = nullptr;

void pushLine(int x1, int y1, int x2, int y2) {
    form line;
    line.addVertex({ x1, y1 });
    line.addVertex({ x2, y2 });
    forms.PB(line);
}

void pushRect(int x1, int y1, int x2, int y2) {
    form rect { REC };
    rect.addVertex({ x1, y1 });
    rect.addVertex({ x2, y1 });
    rect.addVertex({ x2, y2 });
    rect.addVertex({ x1, y2 });
    forms.PB(rect);
}

void pushPolygon(int x, int y) {
    form polygon { POL };
    polygon.addVertex(x, y);
    forms.PB(polygon);
    openPolygon = &forms.back();
}

void pushCircunference(int x, int y, int r) {
    form circunference { CIR };
    circunference.addVertex(x, y);
    circunference.radius = r;
    forms.PB(circunference);
}

void init(void);
void reshape(int w, int h);
void display(void);
void menu_popup(int value);
void keyboard(unsigned char key, int x, int y);
void keyboardSpecial(int key, int x, int y);
void mouse(int button, int state, int x, int y);
void mousePassiveMotion(int x, int y);
void drawPixel(int x, int y, bool keep);
void drawFormas();
void bresenham(int x1, int y1, int x2, int y2);
void bresenham(pair<int, int> p1, pair<int, int> p2);
void bresenhamCircunference(pair<int, int> mid, int r);
void bresenhamCircunference(int x, int y, int r);

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode (GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize (width, height);
    glutInitWindowPosition (100, 100);
    glutCreateWindow ("Computacao Grafica: Paint");
    init();
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(keyboardSpecial);
    glutMouseFunc(mouse);
    glutPassiveMotionFunc(mousePassiveMotion);
    glutDisplayFunc(display);
    
    glutCreateMenu(menu_popup);
    glutAddMenuEntry("Linha", LIN);
    glutAddMenuEntry("Retangulo", REC);
    glutAddMenuEntry("Triangulo", TRI);
    glutAddMenuEntry("Poligono", POL);
    glutAddMenuEntry("Circunferencia", CIR);
    glutAddMenuEntry("Preencher", FILL);
    glutAddMenuEntry("Limpar", -1);
    glutAddMenuEntry("Sair", 0);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    glutMainLoop();
    return EXIT_SUCCESS;
}

void init(void) {
    glClearColor(1.0, 1.0, 1.0, 1.0);
}

void reshape(int w, int h) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glViewport(0, 0, w, h);
	
	width = w;
	height = h;
    glOrtho (0, w, h, 0, -1, 1);  

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

}

GLubyte r = 0, g = 0, b = 0;

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3ub(r, g, b);
    drawFormas();
    glColor3ub(r, g, b);
    draw_text_stroke(0, height, "(" + to_string(m_x) + "," + to_string(m_y) +
        ")", 0.2);
    glutSwapBuffers();
}

vector<pair<pair<int, int>, GLubyte*>> points;

bool canTransformLast = false;

void menu_popup(int value) {
    if(value == 0) exit(EXIT_SUCCESS);
    if(value == -1) {
        forms.clear();
        points.clear();
        cout << "Limpando...\n";
    }
    else modo = value;
    canTransformLast = false;
}

void transform(form &f, vector<Matrix<double>> transforms) {
    for(int a = 0; a < transforms.size(); a++)
            for(auto &i: f.vertices) {
                auto aux = transforms[a] * i;
                i = { round(aux.first), round(aux.second) };
            }
}

void keyboard(unsigned char key, int x, int y) {
    // cout << (int) key << " (ASCII)"<< endl;
    if(key == ESC) exit(EXIT_SUCCESS);


    switch(key) {
        case 'R':
        r = min((int) r + 16, 255);
        return;
        case 'r':
        r = max((int) r - 16, 0);
        return;
        case 'G':
        g = min((int) g + 16, 255);
        return;
        case 'g':
        g = max((int) g - 16, 0);
        return;
        case 'B':
        b = min((int) b + 16, 255);
        return;
        case 'b':
        b = max((int) b - 16, 0);
        return;
    }

    if(!canTransformLast) return;
    form &lastForm = forms.back();
    if(lastForm.type == CIR) return;
    auto centroid = lastForm.getCentroid();

    // inicialmente, imaginou-se criar uma classe de matrizes que
    // suporte a operação de multiplicação para gerar uma matriz
    // de transformação final
    vector<Matrix<double>> m = {
        Matrix<double>::translationMatrix(-centroid.first, -centroid.second),
        Matrix<double>(),
        Matrix<double>::translationMatrix(centroid.first, centroid.second)
    };

    switch(key) {
        case 'O':
        m[1] = Matrix<double>::scaleMatrix(1.1, 1);
        transform(lastForm, m);
        break;

        case 'o':
        m[1] = Matrix<double>::scaleMatrix(0.9, 1);
        transform(lastForm, m);
        break;

        case 'P':
        m[1] = Matrix<double>::scaleMatrix(1, 1.1);
        transform(lastForm, m);
        break;

        case 'p':
        m[1] = Matrix<double>::scaleMatrix(1, 0.9);
        transform(lastForm, m);
        break;

        case 'Z':
        m[1] = Matrix<double>::rotationMatrix(M_PI / 16);
        transform(lastForm, m);
        break;

        case 'z':
        m[1] = Matrix<double>::rotationMatrix(-M_PI / 16);
        transform(lastForm, m);
        break;

        case 'X':
        m[1] = Matrix<double>::shearMatrix(0.1, 0);
        transform(lastForm, m);
        break;

        case 'x':
        m[1] = Matrix<double>::shearMatrix(-0.1, 0);
        transform(lastForm, m);
        break;

        case 'Y':
        m[1] = Matrix<double>::shearMatrix(0, 0.1);
        transform(lastForm, m);
        break;

        case 'y':
        m[1] = Matrix<double>::shearMatrix(0, -0.1);
        transform(lastForm, m);
        break;

        case 'M':
        m[1] = Matrix<double>::scaleMatrix(-1, 1);
        transform(lastForm, m);
        break;

        case 'm':
        m[1] = Matrix<double>::scaleMatrix(1, -1);
        transform(lastForm, m);
        break;
    }
    glutPostRedisplay();
}

void keyboardSpecial(int key, int x, int y) {
    // cout << key << " (Unicode)" << endl;
    if(forms.size() == 0) return;
    if(!canTransformLast) return;
    form &lastForm = forms.back();

    switch(key) {
        case UP:
        for(auto &i: lastForm.vertices) i.second -= 4;
        break;

        case DOWN:
        for(auto &i: lastForm.vertices) i.second += 4;
        break;

        case LEFT:
        for(auto &i: lastForm.vertices) i.first -= 4;
        break;

        case RIGHT:
        for(auto &i: lastForm.vertices) i.first += 4;
    }

    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    switch(button) {
        case GLUT_LEFT_BUTTON:
        if(state == GLUT_DOWN) {
            if(modo == FILL) {
                stack<pair<int, int>> q;
                q.push({ x, y });
                vector<int> dx = { 0,  0, -1, 1},
                            dy = { 1, -1,  0, 0};
                vector<vector<bool>> visited(width, vector<bool>(height, false));

				GLubyte initial[3];
				glReadPixels(x, height - y - 1, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, initial);

				GLubyte* data = new GLubyte[width * height * 3];
				glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);

                GLubyte *color = new GLubyte[3];
                color[0] = r;
                color[1] = g;
                color[2] = b;
                while(!q.empty()) {
                    auto u = q.top(); q.pop();
                    visited[u.first][u.second] = true;

					int index = ((height - u.second) * width + u.first - 1) * 3;

                    if(data[index] == initial[0] && data[index + 1] == initial[1] && data[index + 2] == initial[2]) {
                        points.PB({ { u.first, u.second }, color });
					}
                    else continue;

                    for(int i = 0; i < dx.size(); i++) {
                        int xx = u.first + dx[i], yy = u.second + dy[i];
                        if(xx < 512 && xx >= 0 && yy < 512 && yy >= 0) {
                            if(visited[xx][yy]) continue;
                            q.push({ xx, yy });
                        }
                    }
                }
                glutPostRedisplay();
            } else if(modo == POL || modo == TRI) {
                if(openPolygon != nullptr) {

                    auto firstVertex = openPolygon->vertices[0];

                    x_1 = x;
                    y_1 = y;

                    // se o novo vértice estiver na região de uma circunferência
                    // com centro no primeiro vértice, feche o polígono
                    if(hypot(firstVertex.first - x_1, firstVertex.second - y_1) <= 8) {
                        openPolygon = nullptr;
                        canTransformLast = true;
                        click1 = false;
                    } else {
                        openPolygon->addVertex({
                            x_1, y_1
                        });
                        if(modo == TRI) {
                            if(openPolygon->vertices.size() == 3) {
                                openPolygon = nullptr;
                                canTransformLast = true;
                                click1 = false;
                            }
                        }
                    }
                } else {
                    click1 = true;
                    canTransformLast = false;
                    x_1 = x;
                    y_1 = y;
                    pushPolygon(x_1, y_1);
                }
            } else if(click1) {
                x_2 = x;
                y_2 = y;
                printf("Clique 2(%d, %d)\n",x_2,y_2);
                
                switch(modo) {
                    case LIN:
                    pushLine(x_1, y_1, x_2, y_2);
                    break;
                    case REC:
                    pushRect(x_1, y_1, x_2, y_2);
                    break;
                    case CIR:
                    pushCircunference(x_1, y_1, 
                        hypot(x_1 - x_2, y_1 - y_2));
                }

                click1 = false;
                canTransformLast = true;
                glutPostRedisplay();
            } else {
                click1 = true;
                canTransformLast = false;
                x_1 = x;
                y_1 = y;
                printf("Clique 1(%d, %d)\n",x_1,y_1);
            }
        }
        break;
    }
}

void mousePassiveMotion(int x, int y) {
    m_x = x; m_y = y;
    glutPostRedisplay();
}

void drawPixel(int x, int y) {
	glBegin(GL_POINTS);
	glVertex2i(x, y);
	glEnd();
}

void bresenham(pair<int, int> p1, pair<int, int> p2) {
    bresenham(p1.first, p1.second, p2.first, p2.second);
}

void bresenhamCircunference(pair<int, int> mid, int r) {
    bresenhamCircunference(mid.first, mid.second, r);
}

void bresenhamCircunference(int x1, int y1, int r) {
    int d = 1 - r, de = 3, dse = - 2 * r + 5, x = 0, y = r;
    
    drawPixel(x1, y + y1);
    drawPixel(x1, -y + y1);
    drawPixel(y + x1, y1);
    drawPixel(-y + x1, y1);

    while(y > x) {
        if(d < 0) {
            d += de;
            de += 2;
            dse += 2;
        } else {
            d += dse;
            de += 2;
            dse += 4;
            y--;
        }
        x++;
        drawPixel(x + x1, y + y1);
        drawPixel(-x + x1, y + y1);
        drawPixel(x + x1, -y + y1);
        drawPixel(-x + x1, -y + y1);

        drawPixel(y + x1, x + y1);
        drawPixel(-y + x1, x + y1);
        drawPixel(y + x1, -x + y1);
        drawPixel(-y + x1, -x + y1);
    }
}

void bresenham(int x1, int y1, int x2, int y2) {
    // Decidi refatorar este código em relação ao código enviado como tarefa
    int dx = x1 - x2, dy = y1 - y2;

    bool simetrico = false, declive = false;

    if(dy * dx < 0) {
        y1 = -y1;
        y2 = -y2;
        simetrico = true;
    }

    if(abs(dy) > abs(dx)) {
        swap(x1, y1);
        swap(x2, y2);
        swap(dx, dy);
        declive = true;
    }

    if(x1 > x2) {
        swap(x1, x2);
        swap(y1, y2);
        dx = -dx;
        dy = -dy;
    }

    dx = abs(dx);
    dy = abs(dy);
    int d = 2 * dy - dx;
    int incE = 2 * dy, incNE = 2 * (dy - dx);
    int x, y, xEnd;

    if(x1 > x2) {
        x = x2;
        y = y2;
        xEnd = x1;
    } else {
        x = x1;
        y = y1;
        xEnd = x2;
    }

    int _x = x, _y = y;
    if(declive) swap(_x, _y);
    if(simetrico) _y = -_y;

    drawPixel(_x, _y);

    while(x < xEnd) {
        x++;
        if(d < 0)
            d += incE;
        else {
            y++;
            d += incNE;
        }

        _x = x, _y = y;
        if(declive) swap(_x, _y);
        if(simetrico) _y = -_y;

        drawPixel(_x, _y);
    }
}

void drawFormas() {

    for(auto i: points) {
        GLubyte *color = i.second;
        glColor3ub(color[0], color[1], color[2]);
        drawPixel(i.first.first, i.first.second);
    }
    glColor3ub(0, 0, 0);

    if(click1) {
        switch(modo) {
            case REC:
            bresenham(x_1, y_1, m_x, y_1);
            bresenham(m_x, y_1, m_x, m_y);
            bresenham(m_x, m_y, x_1, m_y);
            bresenham(x_1, m_y, x_1, y_1);
            break;
            case CIR:
            bresenhamCircunference(x_1, y_1, 
                hypot(x_1 - m_x, y_1 - m_y));
            break;
            default:
            bresenham(x_1, y_1, m_x, m_y);
        }
    }

    glutPostRedisplay();

    //Percorre a lista de formas geometricas para desenhar
    int bound = (openPolygon == nullptr ? forms.size() : forms.size() - 1);
    form f;
    for(int i = 0; i < bound; i++) {
        f = forms[i];
        switch(f.type) {
            case LIN:
            bresenham(f.vertices[0], f.vertices[1]);
            break;
            case CIR:
            bresenhamCircunference(f.vertices[0], f.radius);
            break;
            default:
            auto &v = f.vertices;
            for(int i = 1; i < v.size(); i++) {
                bresenham(v[i - 1], v[i]);
            }
            bresenham(v.back(), v[0]);
        }
    }

    if(openPolygon != nullptr) {
        f = forms.back();
        auto &v = f.vertices;
        for(int i = 1; i < v.size(); i++) {
            bresenham(v[i - 1], v[i]);
        }
    }

}
