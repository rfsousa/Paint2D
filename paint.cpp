/*
 * Computacao Grafica
 * Paint2D
 * Autor: Prof. Laurindo de Sousa Britto Neto
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
#include <vector>
#include "glut_text.h"

#define PB push_back
#define SQ(x) (x)*(x)

using namespace std;

#define ESC 27

enum form_type { LIN = 1, TRI, RECT, POL, CIR };

bool click1 = false;
int m_x, m_y, x_1, y_1, x_2, y_2;
int modo = LIN;
int width = 512, height = 512;

struct form {
    form_type type = LIN;
    vector<pair<int, int>> vertices;

    void addVertex(int x, int y) {
        vertices.PB({ x, y });
    }

    void addVertex(pair<int, int> vertex) {
        vertices.PB(vertex);
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
void mouse(int button, int state, int x, int y);
void mousePassiveMotion(int x, int y);
void drawPixel(int x, int y);
void drawFormas();
void bresenham(int x1, int y1, int x2, int y2);
void bresenham(pair<int, int> p1, pair<int, int> p2);

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize (width, height);
    glutInitWindowPosition (100, 100);
    glutCreateWindow ("Computacao Grafica: Paint");
    init();
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutPassiveMotionFunc(mousePassiveMotion);
    glutDisplayFunc(display);
    
    glutCreateMenu(menu_popup);
    glutAddMenuEntry("Linha", LIN);
    glutAddMenuEntry("Retangulo", RECT);
    glutAddMenuEntry("Triangulo", TRI);
    glutAddMenuEntry("Poligono", POL);
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
    glOrtho (0, w, 0, h, -1 ,1);  

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

}

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f (0.0, 0.0, 0.0);
    drawFormas();
    draw_text_stroke(0, 0, "(" + to_string(m_x) + "," + to_string(m_y) + ")", 0.2);
    glutSwapBuffers();
}

void menu_popup(int value) {
    if (value == 0) exit(EXIT_SUCCESS);
    modo = value;
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case ESC: exit(EXIT_SUCCESS); break;
    }
}

/*
 * Controle dos botoes do mouse
 */
void mouse(int button, int state, int x, int y) {
    switch(button) {
        case GLUT_LEFT_BUTTON:
        if(state == GLUT_DOWN) {
            if(modo == POL) {
                if(openPolygon != nullptr) {

                    auto firstVertex = openPolygon->vertices[0];

                    x_1 = x;
                    y_1 = height - y - 1;

                    // se o novo vértice estiver na região de uma circunferência
                    // com centro no primeiro vértice, feche o polígono
                    if(SQ(firstVertex.first - x_1) + SQ(firstVertex.second - y_1) <= SQ(8)) {
                        openPolygon = nullptr;
                        click1 = false;
                    } else {
                        openPolygon->addVertex({
                            x_1, y_1
                        });
                    }
                } else {
                    click1 = true;
                    x_1 = x;
                    y_1 = height - y - 1;
                    pushPolygon(x_1, y_1);
                }
            } else if(click1) {
                x_2 = x;
                y_2 = height - y - 1;
                printf("Clique 2(%d, %d)\n",x_2,y_2);
                
                switch(modo) {
                    case LIN:
                    pushLine(x_1, y_1, x_2, y_2);
                    break;
                    case RECT:
                    pushRect(x_1, y_1, x_2, y_2);
                }

                click1 = false;
                glutPostRedisplay();
            } else {
                click1 = true;
                x_1 = x;
                y_1 = height - y - 1;
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
    m_x = x; m_y = height - y - 1;
    glutPostRedisplay();
}

/*
 * Funcao para desenhar apenas um pixel na tela
 */
void drawPixel(int x, int y) {
    glBegin(GL_POINTS); // Seleciona a primitiva GL_POINTS para desenhar
    glVertex2i(x, y);
    glEnd();  // indica o fim do ponto
}

/*
 *Funcao que desenha a lista de formas geometricas
 */
void drawFormas() {
    //Apos o primeiro clique, desenha a reta com a posicao atual do mouse
    if(click1) {
        switch(modo) {
            case RECT:
            bresenham(x_1, y_1, m_x, y_1);
            bresenham(m_x, y_1, m_x, m_y);
            bresenham(m_x, m_y, x_1, m_y);
            bresenham(x_1, m_y, x_1, y_1);
            break;
            default:
            bresenham(x_1, y_1, m_x, m_y);
        }
    }
    
    //Percorre a lista de formas geometricas para desenhar
    int bound = (openPolygon == nullptr ? forms.size() : forms.size() - 1);
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
            bresenham(v[i - 1], v[i]);
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

void bresenham(pair<int, int> p1, pair<int, int> p2) {
    bresenham(p1.first, p1.second, p2.first, p2.second);
}

void bresenham(int x1, int y1, int x2, int y2) {
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