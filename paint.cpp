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
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <array>
#include <set>
#include <algorithm>
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
#define PLUS 65
#define MINUS 45
#define ENTER 13

enum form_type { LIN = 1, TRI, RECT, POL, CIR, FILL };

bool click1 = false;
int m_x, m_y, x_1, y_1, x_2, y_2;
int modo = LIN;
int width = 512, height = 512;
vector<vector<array<GLubyte, 3>>> aux(width, vector<array<GLubyte, 3>>(height, { 0xFF, 0xFF, 0xFF })),
                                  tmp(width, vector<array<GLubyte, 3>>(height, { 0xFF, 0xFF, 0xFF }));

struct form {
    form_type type;
    vector<pair<int, int>> vertices;

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
    form rect { RECT };
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
    openPolygon = &forms[forms.size() - 1];
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
void bresenham(int x1, int y1, int x2, int y2, bool keep);
void bresenham(pair<int, int> p1, pair<int, int> p2, bool keep);

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
    glutAddMenuEntry("Retangulo", RECT);
    glutAddMenuEntry("Triangulo", TRI);
    glutAddMenuEntry("Poligono", POL);
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
	
	// width = w;
	// height = h;
    glOrtho (0, w, h, 0, -1, 1);  

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

}

double sx = 1, sy = 1;

GLubyte r = 0, g = 0, b = 0;

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3ub(r, g, b);
    drawFormas();
    stringstream ss;
    ss << fixed << setprecision(2) << " Sx: " << sx << " Sy: " << sy;
    glColor3ub(r, g, b);
    draw_text_stroke(0, height, "(" + to_string(m_x) + "," + to_string(m_y) +
        ")" + ss.str(), 0.2);
    glutSwapBuffers();
}

vector<pair<int, int>> points;

bool canTransformLast = false;

void clearMatrices() {
    for(int i = 0; i < width; i++) {
        for(int j = 0; j < height; j++) {
            aux[i][j][0] = aux[i][j][1] = aux[i][j][2] = 0xFF;
            tmp[i][j][0] = tmp[i][j][1] = tmp[i][j][2] = 0xFF;
        }
    }
}

void menu_popup(int value) {
    if(value == 0) exit(EXIT_SUCCESS);
    if(value == -1) {
        forms.clear();
        points.clear();
        canTransformLast = false;
        clearMatrices();
        cout << "Limpando...\n";
    }
    else modo = value;
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
        cout << (int) r << endl;
        return;
        case 'r':
        r = max((int) r - 16, 0);
        cout << (int) r << endl;
        return;
    }

    if(!canTransformLast) return;
    form &lastForm = forms[forms.size() - 1];
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
    cout << key << " (Unicode)" << endl;
    if(forms.size() == 0) return;
    if(!canTransformLast) return;
    form &lastForm = forms[forms.size() - 1];

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

/*
 * Controle dos botoes do mouse
 */
void mouse(int button, int state, int x, int y) {
    switch(button) {
        case GLUT_LEFT_BUTTON:
        if(state == GLUT_DOWN) {
            if(modo == FILL) {
                canTransformLast = false;
                drawFormas(); // bug fix
                cout << x << " " << y << endl;
                auto initial = aux[x][y];
                stack<pair<int, int>> q;
                q.push({ x, y });
                vector<int> dx = { 0,  0, -1, 1},
                            dy = { 1, -1,  0, 0};
                vector<vector<bool>> visited(width, vector<bool>(height, false));
                vector<pair<int, int>> tr;
                while(!q.empty()) {
                    auto u = q.top(); q.pop();
                    visited[u.first][u.second] = true;

                    auto pixel = aux[u.first][u.second];

                    // o problema das cores pode ser resolvido adicionando comparações
                    // envolvendo a matriz auxiliar aqui
                    if((pixel[0] == initial[0] && pixel[1] == initial[1] && pixel[2] == initial[2])) {
                        points.PB({ u.first, u.second });
                        tr.PB({ u.first, u.second });
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
                for(auto p: tr) {
                    auto &pixel = aux[p.first][p.second];

                    pixel[0] = pixel[1] = pixel[2] = 0xFF;
                }
                glutPostRedisplay();
            } else if(modo == POL || modo == TRI) {
                if(openPolygon != nullptr) {

                    auto firstVertex = openPolygon->vertices[0];

                    x_1 = x;
                    y_1 = y;

                    // se o novo vértice estiver na região de uma circunferência
                    // com centro no primeiro vértice, feche o polígono
                    if(SQ(firstVertex.first - x_1) + SQ(firstVertex.second - y_1) <= SQ(10)) {
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
                    case RECT:
                    pushRect(x_1, y_1, x_2, y_2);
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

//        case GLUT_MIDDLE_BUTTON:
//            if (state == GLUT_DOWN) {
//                glutPostRedisplay();
//            }
//        break;
//
//        case GLUT_RIGHT_BUTTON:
//            if (state == GLUT_DOWN) {
//                glutPostRedisplay();
//            }
//        break;

    }
}

/*
 * Controle da posicao do cursor do mouse
 */
void mousePassiveMotion(int x, int y) {
    m_x = x; m_y = y;
    glutPostRedisplay();
}

/*
 * Funcao para desenhar apenas um pixel na tela
 */
void drawPixel(int x, int y, bool keep = true) {
    if(x < 0 || y < 0 || x >= width || y >= height) return;
    if(keep) {
        auto &pixel = aux[x][y];
        if(!(
            pixel[0] == pixel[1] && pixel [1] == pixel[2]
            && pixel[2] == 0xFF
            )) return;
        pixel[0] = r;
        pixel[1] = g;
        pixel[2] = b;
    } else {
        auto &pixel = tmp[x][y];
        pixel[0] = r;
        pixel[1] = g;
        pixel[2] = b;
    }
}

void bresenham(pair<int, int> p1, pair<int, int> p2, bool keep = true) {
    bresenham(p1.first, p1.second, p2.first, p2.second, keep);
}

void bresenham(int x1, int y1, int x2, int y2, bool keep = true) {
    // Decidi refatorar este código em relação ao código enviado como tarefa
    // Isso ocorreu após a leitura do livro "Computer Graphics with OpenGL".
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

    drawPixel(_x, _y, keep);

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

        drawPixel(_x, _y, keep);
    }
}

/*
 *Funcao que desenha a lista de formas geometricas
 */
void drawFormas() {
    //Apos o primeiro clique, desenha a reta com a posicao atual do mouse

    if(click1) {
        switch(modo) {
            case RECT:
            bresenham(x_1, y_1, m_x, y_1, false);
            bresenham(m_x, y_1, m_x, m_y, false);
            bresenham(m_x, m_y, x_1, m_y, false);
            bresenham(x_1, m_y, x_1, y_1, false);
            break;
            default:
            bresenham(x_1, y_1, m_x, m_y, false);
        }
    }

    glutPostRedisplay();

    for(auto i: points) {
        drawPixel(i.first, i.second);
    }
    
    //Percorre a lista de formas geometricas para desenhar
    int bound = (openPolygon == nullptr && !canTransformLast ? forms.size() : forms.size() - 1);
    form f;
    for(int i = 0; i < bound; i++) {
        f = forms[i];
        switch(f.type) {
            case LIN:
            bresenham(f.vertices[0], f.vertices[1]);
            break;
            default:
            auto &v = f.vertices;
            for(int i = 1; i < v.size(); i++) {
                bresenham(v[i - 1], v[i]);
            }
            bresenham(v[v.size() - 1], v[0]);
        }
    }

    if(openPolygon != nullptr) {
        f = forms[forms.size() - 1];
        auto &v = f.vertices;
        for(int i = 1; i < v.size(); i++) {
            bresenham(v[i - 1], v[i], false);
        }
    }

    for(int i = 0; i < width; i++) {
        for(int j = 0; j < height; j++) {
            glColor3ub(aux[i][j][0], aux[i][j][1], aux[i][j][2]);
            glBegin(GL_POINTS); // Seleciona a primitiva GL_POINTS para desenhar
            glVertex2i(i, j);
            glEnd();  // indica o fim do ponto
        }
    }

    if(canTransformLast) {
        auto f = forms[forms.size() - 1];
        switch(f.type) {
            case LIN:
            bresenham(f.vertices[0], f.vertices[1], false);
            break;
            default:
            auto &v = f.vertices;
            for(int i = 1; i < v.size(); i++) {
                bresenham(v[i - 1], v[i], false);
            }
            bresenham(v[v.size() - 1], v[0], false);
        }
    }

    for(int i = 0; i < width; i++) {
        for(int j = 0; j < height; j++) {
            auto pixel = tmp[i][j];
            tmp[i][j][0] = tmp[i][j][1] = tmp[i][j][2] = 0xFF;
            if(pixel[0] == pixel[1] && pixel[1] == pixel[2]
                && pixel[2] == 0xFF) continue;
            glColor3ub(pixel[0], pixel[1], pixel[2]);
            glBegin(GL_POINTS); // Seleciona a primitiva GL_POINTS para desenhar
            glVertex2i(i, j);
            glEnd();  // indica o fim do ponto
        }
    }
    // for(forward_list<forma>::iterator f = formas.begin(); f != formas.end(); f++){
    //     switch (f->tipo) {
    //         case LIN:
    //         int i = 0, x[2], y[2];
    //             //Percorre a lista de vertices da forma linha para desenhar
    //         for(forward_list<vertice>::iterator v = f->v.begin(); v != f->v.end(); v++, i++){
    //             x[i] = v->x;
    //             y[i] = v->y;
    //         }
    //             //Desenha o segmento de reta apos dois cliques
    //         bresenham(x[0], y[0], x[1], y[1]);
    //         break;
    //         case RET:

    //         break;
    //     }
    // }
}
