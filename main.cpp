#include <GL/freeglut.h> //OpenGL

#include <cmath> //Matematica

#include <cstdlib> //Padrão do c++

#include <algorithm> //mANIPULA DADOS

#include <cctype> //VERIFICA E MANIPULA CARACTERES

// ============================================================

// CONFIGURAÇÕES DA SALA

// ============================================================

const float SALA_ESQUERDA =-6.5f;

const float SALA_DIREITA  = 6.5f;

const float CENTRO_PORTA = -3.0f;

const float SALA_FRENTE   = -4.0f;

const float SALA_FUNDO    = 4.0f;

const float SALA_ALTURA   = 4.0f;

const float ESPESSURA_PAREDE = 0.20f;

const float PORTA_LARGURA = 1.8f;

const float PORTA_ALTURA  = 2.5f;

const float JANELA_LARGURA = 4.8f;

const float JANELA_ALTURA  = 1.4f;

const float JANELA_BASE    = 1.2f;

// ============================================================

// JOGADOR E CÂMERA

// ============================================================

float jogadorX = 0.0f;

float jogadorY = 1.7f;

float jogadorZ = 0.0f;

float yaw = 0.0f;

float pitch = 0.0f;

// Alterna entre projeção perspectiva (P) e ortogonal (O).
bool usarPerspectiva = true;

float velocidade = 0.06f;

float raioJogador = 0.25f;

bool teclas[256] = {false};

int larguraJanela = 1200;

int alturaJanela = 800;

int centroMouseX = larguraJanela / 2;

int centroMouseY = alturaJanela / 2;

// ============================================================

// FUNÇÕES AUXILIARES

// ============================================================

void definirCor(float r, float g, float b)

{

    glColor3f(r, g, b);

}

// ============================================================
// MATRIZES DE TRANSFORMAÇÃO E PROJEÇÃO
// As matrizes são armazenadas em ordem column-major, como a OpenGL espera.
// ============================================================ ela amtriz 4x4 Para representar transformações 3D

void aplicarTranslacao(float x, float y, float z)
{
    const GLfloat matriz[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        x,    y,    z,    1.0f // sao cordenadas homogeeas, ela add 4 componete ao ponto 3d o 1 mantem o homogena
    };

    glMultMatrixf(matriz); //Para multiplicar a matriz atual do OpenGL pela matriz de transformação que criamos.
}

void aplicarEscala(float x, float y, float z)
{
    const GLfloat matriz[16] = {
        x,    0.0f, 0.0f, 0.0f,
        0.0f, y,    0.0f, 0.0f,
        0.0f, 0.0f, z,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }; // caso tenha um valor negativo sera invertendo sua orientação

    glMultMatrixf(matriz);
}

// Rotação em torno de um eixo arbitrário, usando a fórmula de Rodrigues.
void aplicarRotacao(float anguloGraus, float eixoX, float eixoY, float eixoZ)
{
    const float comprimento = std::sqrt(
        eixoX * eixoX + eixoY * eixoY + eixoZ * eixoZ
    );

    if (comprimento == 0.0f)
        return;

    eixoX /= comprimento;
    eixoY /= comprimento;
    eixoZ /= comprimento;

    const float radianos = anguloGraus * 3.14159265358979323846f / 180.0f;
    const float c = std::cos(radianos);// esse abs Para calcular o comprimento do vetor, usando a raiz quadrada
    const float s = std::sin(radianos);
    const float t = 1.0f - c; // c representa o coseno do angulo, simplifica o calulo da matrix rotção, pq esse famula gira , Porque utiliza um vetor que define a direção do eixo de rotação
    const GLfloat matriz[16] = {
        t * eixoX * eixoX + c,
        t * eixoX * eixoY + s * eixoZ,
        t * eixoX * eixoZ - s * eixoY,
        0.0f,

        t * eixoX * eixoY - s * eixoZ,
        t * eixoY * eixoY + c,
        t * eixoY * eixoZ + s * eixoX,
        0.0f,

        t * eixoX * eixoZ + s * eixoY,
        t * eixoY * eixoZ - s * eixoX,
        t * eixoZ * eixoZ + c,
        0.0f,

        0.0f, 0.0f, 0.0f, 1.0f
    };

    glMultMatrixf(matriz);
}

void matrizPerspectiva(GLfloat matriz[16], float fov, float aspecto,
                       float perto, float longe)
{
    for (int i = 0; i < 16; ++i)
        matriz[i] = 0.0f;

    const float f = 1.0f / std::tan(//abertura da projeção perspectiva.
        fov * 3.14159265358979323846f / 360.0f // É o campo de visão
    );

    matriz[0] = f / aspecto;
    matriz[5] = f;//
    matriz[10] = (longe + perto) / (perto - longe);
    matriz[11] = -1.0f;//relação entre a profundidade
    matriz[14] = (2.0f * longe * perto) / (perto - longe); // e todos esses valores são para q tenham essa distoção de tamanhpo
}
// oq aconteceria se aumentasse o fov?A câmera passa a enxergar uma área maior, mas os objetos parecem menores e a imagem apresenta maior distorção nas bordas.
void matrizOrtogonal(GLfloat matriz[16], float esquerda, float direita,
                     float baixo, float cima, float perto, float longe)
{
    for (int i = 0; i < 16; ++i)
        matriz[i] = 0.0f;

    matriz[0] = 2.0f / (direita - esquerda);
    matriz[5] = 2.0f / (cima - baixo);
    matriz[10] = -2.0f / (longe - perto);
    matriz[12] = -(direita + esquerda) / (direita - esquerda);
    matriz[13] = -(cima + baixo) / (cima - baixo);
    matriz[14] = -(longe + perto) / (longe - perto);
    matriz[15] = 1.0f;
}//O que representam esquerda, direita, cima e baixo?
//Representam os limites da área visível da projeção. eaplicado em Em desenhos técnicos, plantas

void configurarProjecao()
{
    const float aspecto = static_cast<float>(larguraJanela) /
                          static_cast<float>(alturaJanela);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    GLfloat matriz[16];

    if (usarPerspectiva)
    {
        matrizPerspectiva(matriz, 70.0f, aspecto, 0.1f, 200.0f);
    }
    else
    {
        const float metadeAltura = 6.0f;
        const float metadeLargura = metadeAltura * aspecto;

        matrizOrtogonal(
            matriz,
            -metadeLargura, metadeLargura,
            -metadeAltura, metadeAltura,
            0.1f, 200.0f
        );
    }

    glMultMatrixf(matriz);
    glMatrixMode(GL_MODELVIEW);
}

void bloco(

    float x,

    float y,

    float z,

    float largura,

    float altura,

    float profundidade,

    float r,

    float g,

    float b

)//Desenha um objeto 3D utilizando um cubo como forma básica, aplicando translação, escala e cor.

{

    glPushMatrix(); // Salva a matriz de transformação atual.

    aplicarTranslacao(x, y, z); //Move o objeto nas três direções:

    aplicarEscala(largura, altura, profundidade);//Altera o tamanho do objeto

    definirCor(r, g, b);

    glutSolidCube(1.0);//Desenha um cubo sólido.

    glPopMatrix();//Restaura a matriz

}

// ============================================================

// PISO INTERNO

// ============================================================

void desenharChao()

{

    glDisable(GL_LIGHTING);//Desativa a iluminação do OpenGL.

    glColor3f(0.68f, 0.68f, 0.68f);

    glBegin(GL_QUADS);//nicia a construção de uma superfície utilizando quadriláteros.

    glVertex3f(SALA_ESQUERDA, 0.0f, SALA_FRENTE);//Define um vértice

    glVertex3f(SALA_DIREITA, 0.0f, SALA_FRENTE);

    glVertex3f(SALA_DIREITA, 0.0f, SALA_FUNDO);

    glVertex3f(SALA_ESQUERDA, 0.0f, SALA_FUNDO);

    glEnd();//Finaliza a construção

    glEnable(GL_LIGHTING);

}

// ============================================================
// RASTERIZAÇÃO: ALGORITMO DE BRESENHAM
// Os pontos calculados são desenhados no piso (plano XZ).
// ============================================================

void desenharLinhaBresenham(float xInicial, float zInicial,
                            float xFinal, float zFinal)
{
    const int escalaGrade = 100; // 100 células por unidade do ambiente.

    int x0 = static_cast<int>(std::round(xInicial * escalaGrade));
    int z0 = static_cast<int>(std::round(zInicial * escalaGrade));
    const int x1 = static_cast<int>(std::round(xFinal * escalaGrade));
    const int z1 = static_cast<int>(std::round(zFinal * escalaGrade));

    const int dx = std::abs(x1 - x0);
    const int passoX = (x0 < x1) ? 1 : -1;
    const int dz = -std::abs(z1 - z0);
    const int passoZ = (z0 < z1) ? 1 : -1;

    int erro = dx + dz;

    glDisable(GL_LIGHTING);
    glColor3f(0.95f, 0.75f, 0.10f);
    glPointSize(3.0f);
    glBegin(GL_POINTS);

    while (true) // Bresenham utiliza um erro acumulado para escolher o próximo ponto da grade que melhor aproxima a linha ideal.
    {
        glVertex3f(
            static_cast<float>(x0) / escalaGrade,
            0.025f,
            static_cast<float>(z0) / escalaGrade
        );

        if (x0 == x1 && z0 == z1)
            break;

        const int dobroErro = 2 * erro;

        if (dobroErro >= dz)
        {
            erro += dz;
            x0 += passoX;
        }

        if (dobroErro <= dx)
        {
            erro += dx;
            z0 += passoZ;
        }
    }

    glEnd();
    glEnable(GL_LIGHTING);
}

void desenharDetalheRasterizado()
{
    // Linha de referência no piso, desenhada pelo algoritmo de Bresenham.
    desenharLinhaBresenham(-1.5f, -3.0f, 1.5f, -3.0f);
}

// ============================================================

// PISO EXTERNO

// ============================================================

void desenharExterior()

{

    glDisable(GL_LIGHTING);

    glColor3f(0.48f, 0.48f, 0.48f);

    glBegin(GL_QUADS);

    glVertex3f(-15.0f, -0.02f, -18.0f);

    glVertex3f(15.0f, -0.02f, -18.0f);

    glVertex3f(15.0f, -0.02f, SALA_FRENTE);

    glVertex3f(-15.0f, -0.02f, SALA_FRENTE);

    glEnd();

    glEnable(GL_LIGHTING);

}

// ============================================================

// PAREDE FRONTAL COM ENTRADA

// ============================================================

void desenharParedeFrontal()

{

    float metadePorta = PORTA_LARGURA / 2.0f;

    // Parede esquerda

    float larguraEsquerda = (CENTRO_PORTA - metadePorta) - SALA_ESQUERDA;

    bloco(

        (SALA_ESQUERDA + CENTRO_PORTA - metadePorta)  / 2.0f,

        SALA_ALTURA / 2.0f,

        SALA_FRENTE,

        larguraEsquerda,

        SALA_ALTURA,

        ESPESSURA_PAREDE,

        0.88f, 0.88f, 0.84f

    );

    // Parede direita

    float larguraDireita = SALA_DIREITA - (CENTRO_PORTA + metadePorta) ;

    bloco(

        (CENTRO_PORTA+ metadePorta + SALA_DIREITA) / 2.0f,

        SALA_ALTURA / 2.0f,

        SALA_FRENTE,

        larguraDireita,

        SALA_ALTURA,

        ESPESSURA_PAREDE,

        0.88f, 0.88f, 0.84f

    );

    // Parte superior da entrada

    bloco(

        CENTRO_PORTA,

        (SALA_ALTURA + PORTA_ALTURA) / 2.0f,

        SALA_FRENTE,

        PORTA_LARGURA,

        SALA_ALTURA - PORTA_ALTURA,

        ESPESSURA_PAREDE,

        0.88f, 0.88f, 0.84f

    );

}

void desenharParedeFrontal1()

{

    float metadePorta = PORTA_LARGURA / 2.0f;

    // Parede esquerda

    float larguraEsquerda = (CENTRO_PORTA - metadePorta) - SALA_ESQUERDA;

    float deslocamentoZ = -7.0f;

    bloco(

        (SALA_ESQUERDA + CENTRO_PORTA - metadePorta)  / 2.0f,

        SALA_ALTURA / 2.0f,

        SALA_FRENTE + deslocamentoZ,

        larguraEsquerda,

        SALA_ALTURA,

        ESPESSURA_PAREDE,

        0.88f, 0.88f, 0.84f

    );

    // Parede direita

    float larguraDireita = SALA_DIREITA - (CENTRO_PORTA + metadePorta) ;

    bloco(

        (CENTRO_PORTA+ metadePorta + SALA_DIREITA) / 2.0f,

        SALA_ALTURA / 2.0f,

        SALA_FRENTE + deslocamentoZ,

        larguraDireita,

        SALA_ALTURA,

        ESPESSURA_PAREDE,

        0.88f, 0.88f, 0.84f

    );

    // Parte superior da entrada

    bloco(

        CENTRO_PORTA,

        (SALA_ALTURA + PORTA_ALTURA) / 2.0f,

        SALA_FRENTE + deslocamentoZ,

        PORTA_LARGURA,

        SALA_ALTURA - PORTA_ALTURA,

        ESPESSURA_PAREDE,

        0.88f, 0.88f, 0.84f

    );

}

void desenharParedeFrontal3()

{

    float metadePorta = PORTA_LARGURA / 2.0f;

    // Parede esquerda

    float larguraEsquerda = (CENTRO_PORTA - metadePorta) - SALA_ESQUERDA;

    float deslocamentoZ = -7.0f;

    bloco(

        -8.0f,

        SALA_ALTURA / 2.0f,

        SALA_FRENTE + deslocamentoZ,

        larguraEsquerda,

        SALA_ALTURA,

        ESPESSURA_PAREDE,

        0.88f, 0.88f, 0.84f

    );

    // Parede direita

    float larguraDireita = SALA_DIREITA - (CENTRO_PORTA + metadePorta) ;

    bloco(

        -13.0f,

        SALA_ALTURA / 2.0f,

        SALA_FRENTE + deslocamentoZ,

        larguraDireita - 4.0f,

        SALA_ALTURA,

        ESPESSURA_PAREDE,

        0.88f, 0.88f, 0.84f

    );

    // Parte superior da entrada

    bloco(

        -10.0f,

        (SALA_ALTURA + PORTA_ALTURA) / 2.0f,

        SALA_FRENTE + deslocamentoZ,

        PORTA_LARGURA,

        SALA_ALTURA - PORTA_ALTURA,

        ESPESSURA_PAREDE,

        0.88f, 0.88f, 0.84f

    );

}

void desenharParedeFrontal4()

{

    float metadePorta = PORTA_LARGURA / 2.0f;

    // Parede esquerda

    float larguraEsquerda = (CENTRO_PORTA - metadePorta) - SALA_ESQUERDA;

    bloco(

        -7.0f,

        SALA_ALTURA / 2.0f,

        SALA_FRENTE ,

        larguraEsquerda,

        SALA_ALTURA,

        ESPESSURA_PAREDE,

        0.88f, 0.88f, 0.84f

    );

    // Parede direita

    float larguraDireita = SALA_DIREITA - (CENTRO_PORTA + metadePorta) ;

    bloco(

        -12.0f,

        SALA_ALTURA / 2.0f,

        SALA_FRENTE ,

        larguraDireita - 4.0f,

        SALA_ALTURA,

        ESPESSURA_PAREDE,

        0.88f, 0.88f, 0.84f

    );

    // Parte superior da entrada

    bloco(

        -9.0f,

        (SALA_ALTURA + PORTA_ALTURA) / 2.0f,

        SALA_FRENTE ,

        PORTA_LARGURA,

        SALA_ALTURA - PORTA_ALTURA,

        ESPESSURA_PAREDE,

        0.88f, 0.88f, 0.84f

    );

}

// ============================================================

// PAREDES LATERAIS

// ============================================================

void desenharParedeEsquerda()

{

    bloco(

        SALA_ESQUERDA,

        SALA_ALTURA / 2.0f,

        (SALA_FRENTE + SALA_FUNDO) / 2.0f,

        ESPESSURA_PAREDE,

        SALA_ALTURA,

        SALA_FUNDO - SALA_FRENTE,

        0.82f, 0.82f, 0.79f

    );

}

void desenharParedeEsquerda1()

{

    float deslocamentoZ = -7.0f;

    bloco(

        SALA_DIREITA,

        SALA_ALTURA / 2.0f,

        (SALA_FRENTE + SALA_FUNDO) / 2.0f + deslocamentoZ,

        ESPESSURA_PAREDE,

        SALA_ALTURA,

        SALA_FUNDO - SALA_FRENTE,

        0.82f, 0.82f, 0.79f

    );

}

void desenharParedeEsquerda2()

{

    float deslocamentoZ = -7.0f;

    bloco(

        20.0f,

        SALA_ALTURA / 2.0f,

        (SALA_FRENTE + SALA_FUNDO) / 2.0f + deslocamentoZ,

        ESPESSURA_PAREDE,

        SALA_ALTURA,

        18.0f - SALA_FRENTE,

        0.82f, 0.82f, 0.79f

    );

}

void desenharParedeDireita()

{

    bloco(

        SALA_DIREITA,

        SALA_ALTURA / 2.0f,

        (SALA_FRENTE + SALA_FUNDO) / 2.0f,

        ESPESSURA_PAREDE,

        SALA_ALTURA,

        SALA_FUNDO - SALA_FRENTE,

        0.82f, 0.82f, 0.79f

    );

}

void desenharParedeDireita1()

{

    bloco(

        SALA_DIREITA + -13.1f,

        SALA_ALTURA / 2.0f,

        -15.0f,

        ESPESSURA_PAREDE,

        SALA_ALTURA,

        SALA_FUNDO - SALA_FRENTE,

        0.82f, 0.82f, 0.79f

    );

}

// ============================================================

// PAREDE DO FUNDO

// ============================================================

void desenharParedeFundo()

{

    float centrosJanelas[] = {-3.0f, 3.0f};

    float metadeJanela = JANELA_LARGURA / 2.0f;

    float fimJanela = JANELA_BASE + JANELA_ALTURA;

    float limiteAnterior = SALA_ESQUERDA;

    for (float centro : centrosJanelas)

    {

        float inicio = centro - metadeJanela;

        float fim = centro + metadeJanela;

        // Parte lateral da parede

        float larguraSegmento = inicio - limiteAnterior;

        if (larguraSegmento > 0.0f)

        {

            bloco(

                (limiteAnterior + inicio) / 2.0f,

                SALA_ALTURA / 2.0f,

                SALA_FUNDO,

                larguraSegmento,

                SALA_ALTURA,

                ESPESSURA_PAREDE,

                0.88f, 0.88f, 0.84f

            );

        }

        // Parte inferior da janela

        bloco(

            centro,

            JANELA_BASE / 2.0f,

            SALA_FUNDO,

            JANELA_LARGURA,

            JANELA_BASE,

            ESPESSURA_PAREDE,

            0.88f, 0.88f, 0.84f

        );

        // Parte superior da janela

        bloco(

            centro,

            (fimJanela + SALA_ALTURA) / 2.0f,

            SALA_FUNDO,

            JANELA_LARGURA,

            SALA_ALTURA - fimJanela,

            ESPESSURA_PAREDE,

            0.88f, 0.88f, 0.84f

        );

        limiteAnterior = fim;

    }

    // Último segmento

    if (limiteAnterior < SALA_DIREITA)

    {

        bloco(

            (limiteAnterior + SALA_DIREITA) / 2.0f,

            SALA_ALTURA / 2.0f,

            SALA_FUNDO,

            SALA_DIREITA - limiteAnterior,

            SALA_ALTURA,

            ESPESSURA_PAREDE,

            0.88f, 0.88f, 0.84f

        );

    }

}

// ============================================================

// JANELAS

// ============================================================

void desenharJanelas()

{

    float centros[] = {-3.0f, 3.0f};

    for (float centro : centros)

    {

        float metade = JANELA_LARGURA / 2.0f;

        float x1 = centro - metade;

        float x2 = centro + metade;

        float y1 = JANELA_BASE;

        float y2 = JANELA_BASE + JANELA_ALTURA;

        float z = SALA_FUNDO - ESPESSURA_PAREDE / 2.0f - 0.01f;

        // Vidro

        glDisable(GL_LIGHTING);

        glColor3f(0.30f, 0.70f, 0.90f);

        glBegin(GL_QUADS);

        glVertex3f(x1, y1, z);

        glVertex3f(x2, y1, z);

        glVertex3f(x2, y2, z);

        glVertex3f(x1, y2, z);

        glEnd();

        glEnable(GL_LIGHTING);

        float espessura = 0.08f;

        // Moldura inferior

        bloco(

            centro, y1, z,

            JANELA_LARGURA, espessura, 0.12f,

            0.25f, 0.25f, 0.25f

        );

        // Moldura superior

        bloco(

            centro, y2, z,

            JANELA_LARGURA, espessura, 0.12f,

            0.25f, 0.25f, 0.25f

        );

        // Moldura esquerda

        bloco(

            x1, (y1 + y2) / 2.0f, z,

            espessura, JANELA_ALTURA, 0.12f,

            0.25f, 0.25f, 0.25f

        );

        // Moldura direita

        bloco(

            x2, (y1 + y2) / 2.0f, z,

            espessura, JANELA_ALTURA, 0.12f,

            0.25f, 0.25f, 0.25f

        );

        // Divisão central

        bloco(

            centro, (y1 + y2) / 2.0f, z,

            0.05f, JANELA_ALTURA, 0.13f,

            0.25f, 0.25f, 0.25f

        );

    }

}

// ============================================================

// ARMÁRIO

// ============================================================

void desenharArmario(

    float x,

    float z,

    float largura,

    float altura,

    float profundidade,

    float rotacao

)

{

    glPushMatrix();

    aplicarTranslacao(x, altura / 2.0f, z);

    aplicarRotacao(rotacao, 0.0f, 1.0f, 0.0f);

    // Estrutura

    bloco(

        0.0f, 0.0f, 0.0f,

largura, altura, profundidade,

        0.0f, 0.0f, 0.0f

    );

    // Acabamento frontal

    bloco(

        0.0f, 0.0f, -profundidade / 2.0f - 0.015f,

largura - 0.08f,

altura - 0.08f,

        0.025f,

        0.68f, 0.43f, 0.25f

    );

    // Porta esquerda

    bloco(

        -largura / 4.0f,

        0.0f,

        -profundidade / 2.0f - 0.045f,

largura / 2.0f - 0.025f,

altura - 0.08f,

        0.06f,

        0.72f, 0.48f, 0.28f

    );

    // Porta direita

    bloco(

largura / 4.0f,

        0.0f,

        -profundidade / 2.0f - 0.045f,

largura / 2.0f - 0.025f,

altura - 0.08f,

        0.06f,

        0.72f, 0.48f, 0.28f

    );

    // Puxador esquerdo

    bloco(

        -0.07f, 0.0f,

        -profundidade / 2.0f - 0.09f,

        0.035f, 0.18f, 0.04f,

        0.15f, 0.15f, 0.15f

    );

    // Puxador direito

    bloco(

        0.07f, 0.0f,

        -profundidade / 2.0f - 0.09f,

        0.035f, 0.18f, 0.04f,

        0.15f, 0.15f, 0.15f

    );

    glPopMatrix();

}

// ============================================================

// MESA

// ============================================================

void desenharMesa0()

{

    float x = 3.30f;

    float z = -1.4f;

    float largura = 5.0f;

    float profundidade = 1.2f;

    // Salva a matriz atual

    glPushMatrix();

    // Posiciona o centro da mesa

   aplicarTranslacao(x, 0.0f, z);

    // Rotaciona 90 graus no eixo Y

    aplicarRotacao(90.0f, 0.0f, 1.0f, 0.0f);

    float alturaMesa = 0.80f;

    float alturaTampo = 0.12f;

    // Tampo

    bloco(

        0.0f, alturaMesa, 0.0f,

        largura, alturaTampo, profundidade,

        0.85f, 0.78f, 0.70f

    );

    float deslocamentoX = largura / 2.0f - 0.08f;

    float deslocamentoZ = profundidade / 2.0f - 0.08f;

    float posicoesX[] = {

        - deslocamentoX,

        + deslocamentoX

    };

    float posicoesZ[] = {

        - deslocamentoZ,

        + deslocamentoZ

    };

    for (float px : posicoesX)

    {

        for (float pz : posicoesZ)

        {

            bloco(

                px, alturaMesa / 2.0f, pz,

                0.10f, alturaMesa, 0.10f,

                0.30f, 0.18f, 0.10f

            );

        }

    }

    glPopMatrix();

}

void desenharMesa()

{

    float x =-0.30f;

    float z = -1.4f;

    float largura = 5.0f;

    float profundidade = 1.2f;

    // Salva a matriz atual

    glPushMatrix();

    // Posiciona o centro da mesa

   aplicarTranslacao(x, 0.0f, z);

    // Rotaciona 90 graus no eixo Y

    aplicarRotacao(90.0f, 0.0f, 1.0f, 0.0f);

    float alturaMesa = 0.80f;

    float alturaTampo = 0.12f;

    // Tampo

    bloco(

        0.0f, alturaMesa, 0.0f,

        largura, alturaTampo, profundidade,

        0.48f, 0.28f, 0.15f

    );

    float deslocamentoX = largura / 2.0f - 0.08f;

    float deslocamentoZ = profundidade / 2.0f - 0.08f;

    float posicoesX[] = {

        - deslocamentoX,

        + deslocamentoX

    };

    float posicoesZ[] = {

        - deslocamentoZ,

        + deslocamentoZ

    };

    for (float px : posicoesX)

    {

        for (float pz : posicoesZ)

        {

            bloco(

                px, alturaMesa / 2.0f, pz,

                0.10f, alturaMesa, 0.10f,

                0.30f, 0.18f, 0.10f

            );

        }

    }

    glPopMatrix();

}

void desenharMesa2()

{

    float x =-0.30f;

    float z = -1.4f;

    float largura = 5.0f;

    float profundidade = 1.2f;

    // Salva a matriz atual

    glPushMatrix();

    // Posiciona o centro da mesa

   aplicarTranslacao(x, 0.0f, z);

    // Rotaciona 90 graus no eixo Y

    aplicarRotacao(90.0f, 0.0f, 1.0f, 0.0f);

    float alturaMesa = 1.60f;

    float alturaTampo = 0.12f;

    // Tampo

    bloco(

        0.0f, alturaMesa, 0.0f,

        largura, alturaTampo, profundidade,

        0.48f, 0.28f, 0.15f

    );

    float deslocamentoX = largura / 2.0f - 0.08f;

    float deslocamentoZ = profundidade / 2.0f - 0.08f;

    float posicoesX[] = {

        - deslocamentoX,

        + deslocamentoX

    };

    float posicoesZ[] = {

        - deslocamentoZ,

        + deslocamentoZ

    };

    for (float px : posicoesX)

    {

        for (float pz : posicoesZ)

        {

            bloco(

                px, alturaMesa / 2.0f, pz,

                0.10f, alturaMesa, 0.10f,

                0.30f, 0.18f, 0.10f

            );

        }

    }

    glPopMatrix();

}

void desenharMesa3()

{

    float x =-0.30f;

    float z = -1.4f;

    float largura = 5.0f;

    float profundidade = 1.2f;

    // Salva a matriz atual

    glPushMatrix();

    // Posiciona o centro da mesa

   aplicarTranslacao(x, 0.0f, z);

    // Rotaciona 90 graus no eixo Y

    aplicarRotacao(90.0f, 0.0f, 1.0f, 0.0f);

    float alturaMesa = 2.40f;

    float alturaTampo = 0.12f;

    // Tampo

    bloco(

        0.0f, alturaMesa, 0.0f,

        largura, alturaTampo, profundidade,

        0.48f, 0.28f, 0.15f

    );

    float deslocamentoX = largura / 2.0f - 0.08f;

    float deslocamentoZ = profundidade / 2.0f - 0.08f;

    float posicoesX[] = {

        - deslocamentoX,

        + deslocamentoX

    };

    float posicoesZ[] = {

        - deslocamentoZ,

        + deslocamentoZ

    };

    for (float px : posicoesX)

    {

        for (float pz : posicoesZ)

        {

            bloco(

                px, alturaMesa / 2.0f, pz,

                0.10f, alturaMesa, 0.10f,

                0.30f, 0.18f, 0.10f

            );

        }

    }

    glPopMatrix();

}

void desenharMesa4()

{

    float x =-0.30f;

    float z = -1.4f;

    float largura = 5.0f;

    float profundidade = 1.2f;

    // Salva a matriz atual

    glPushMatrix();

    // Posiciona o centro da mesa

   aplicarTranslacao(x, 0.0f, z);

    // Rotaciona 90 graus no eixo Y

    aplicarRotacao(90.0f, 0.0f, 1.0f, 0.0f);

    float alturaMesa = 3.20f;

    float alturaTampo = 0.12f;

    // Tampo

    bloco(

        0.0f, alturaMesa, 0.0f,

        largura, alturaTampo, profundidade,

        0.48f, 0.28f, 0.15f

    );

    float deslocamentoX = largura / 2.0f - 0.08f;

    float deslocamentoZ = profundidade / 2.0f - 0.08f;

    float posicoesX[] = {

         - deslocamentoX,

         + deslocamentoX

    };

    float posicoesZ[] = {

         - deslocamentoZ,

         + deslocamentoZ

    };

    for (float px : posicoesX)

    {

        for (float pz : posicoesZ)

        {

            bloco(

                px, alturaMesa / 2.0f, pz,

                0.10f, alturaMesa, 0.10f,

                0.30f, 0.18f, 0.10f

            );

        }

    }

    glPopMatrix();

}

void desenharMesa5()

{

    float x =-1.45f;

    float z = -1.0f;

    float largura = 0.75f;

    float profundidade = 1.2f;

    // Salva a matriz atual

    glPushMatrix();

    // Posiciona o centro da mesa

   aplicarTranslacao(x, 0.0f, z);

    // Rotaciona 90 graus no eixo Y

    aplicarRotacao(-90.0f, 0.0f, 1.0f, 0.0f);

    float alturaMesa = 0.80f;

    float alturaTampo = 0.12f;

    // Tampo

    bloco(

        0.0f, alturaMesa, 0.0f,

        largura, alturaTampo, profundidade,

        0.48f, 0.28f, 0.15f

    );

    float deslocamentoX = largura / 2.0f - 0.08f;

    float deslocamentoZ = profundidade / 2.0f - 0.08f;

    float posicoesX[] = {

        - deslocamentoX,

        + deslocamentoX

    };

    float posicoesZ[] = {

        - deslocamentoZ,

        + deslocamentoZ

    };

    for (float px : posicoesX)

    {

        for (float pz : posicoesZ)

        {

            bloco(

                px, alturaMesa / 2.0f, pz,

                0.10f, alturaMesa, 0.10f,

                0.30f, 0.18f, 0.10f

            );

        }

    }

    glPopMatrix();

}

void desenharBancada()

{

    float x = SALA_ESQUERDA +0.70f;

    float z = 0.0f;

    float largura = 8.0f;

    float profundidade = 1.50f;

    float altura = 0.90f;

    float espessura = 0.08f;

    // Tampo da bancada

    bloco(

        x,

        altura - espessura / 2.0f,

        z,

        profundidade,

        espessura,

        largura,

        0.45f, 0.25f, 0.12f

    );

    // Posição das pernas

    float posicoesZ[] = {

        z - largura / 2.0f + 3.0f,

        z - largura / 2.0f + 0.30f,

        z - largura / 2.0f + 7.8f,

        z - largura / 2.0f + 5.80f

    };

    // Pernas da bancada

    for (float pz : posicoesZ)

    {

        bloco(

            x,

            (altura - espessura) / 2.0f,

            pz,

            1.50f,

            altura - espessura,

            0.44f,

            0.30f, 0.18f, 0.10f

        );

    }

}

// ============================================================

// MÓVEIS

// ============================================================

void desenharMoveis()

{

    desenharMesa0();

    desenharMesa();

    desenharMesa2();

    desenharMesa3();

    desenharMesa4();

    desenharMesa5();

    desenharArmario(

        -0.8f,

        1.5f,

        2.2f,

        2.2f,

        0.65f,

        -180.0f

    );

    desenharBancada();

}

// ============================================================

// SALA COMPLETA

// ============================================================

void desenharSala()

{

    desenharChao();
    desenharDetalheRasterizado();

    desenharParedeFrontal();

    desenharParedeFrontal1();

    desenharParedeFrontal3();

    desenharParedeFrontal4();

    desenharParedeEsquerda();

    desenharParedeEsquerda1();

    desenharParedeEsquerda2();

    desenharParedeDireita();

    desenharParedeDireita1();

    desenharParedeFundo();

    desenharJanelas();

    desenharMoveis();

    // Sem teto.

}

// ============================================================

// COLISÃO

// ============================================================

bool cruzouLinha(float anterior, float atual, float linha)

{

    return

        (anterior > linha && atual <= linha) ||

        (anterior < linha && atual >= linha);

}

bool podeMover(float novoX, float novoZ)

{

    float raio = raioJogador; //epresenta o raio utilizado para calcular o espaço ocupado pelo jogador

    // --------------------------------------------------------

    // PAREDE FRONTAL

    // --------------------------------------------------------

    if (cruzouLinha(jogadorZ, novoZ, SALA_FRENTE))

    {

        float limitePorta = PORTA_LARGURA / 2.0f - raio;

        bool dentroDaPorta =

            std::abs(jogadorX  - CENTRO_PORTA) <= limitePorta && 

            std::abs(novoX  - CENTRO_PORTA) <= limitePorta;//que retorna o valor absoluto de um número. Calcula a distância entre o jogador e o centro da porta

        if (!dentroDaPorta) // AJUDA PARA COLISÃO JOSHUA

        {

            return false;

        }

    }

    // --------------------------------------------------------

    // PAREDE ESQUERDA

    // --------------------------------------------------------

    bool estaNaRegiaoLateral =

novoZ > SALA_FRENTE + raio &&

novoZ < SALA_FUNDO - raio;

    if (estaNaRegiaoLateral)

    {

        if (cruzouLinha(jogadorX, novoX, SALA_ESQUERDA))

        {

            return false;

        }

        if (novoX < SALA_ESQUERDA + raio &&

novoX > SALA_ESQUERDA - raio)

        {

            return false;

        }

        // Parede direita

        if (cruzouLinha(jogadorX, novoX, SALA_DIREITA))

        {

            return false;

        }

        if (novoX > SALA_DIREITA - raio &&

novoX < SALA_DIREITA + raio)

        {

            return false;

        }

    }

    // --------------------------------------------------------

    // PAREDE DO FUNDO

    // --------------------------------------------------------

    if (cruzouLinha(jogadorZ, novoZ, SALA_FUNDO))

    {

        return false;

    }

    return true;

}

// ============================================================

// TECLADO

// ============================================================

void teclaPressionada(unsigned char tecla, int, int)

{

    if (tecla == 27)

    {

        std::exit(0);

    }

    unsigned char normalizada =

        static_cast<unsigned char>(std::tolower(tecla)); // ESSE TOLOWER TRANSFOMAR MASUCULA EM MENISCULA

    // Tecla P: projeção perspectiva. Tecla O: projeção ortogonal.
    if (normalizada == 'p')
    {
        usarPerspectiva = true;
        configurarProjecao();
        glutPostRedisplay();
        return;
    }

    if (normalizada == 'o')
    {
        usarPerspectiva = false;
        configurarProjecao();
        glutPostRedisplay();
        return;
    }

    teclas[normalizada] = true;

}

void teclaSolta(unsigned char tecla, int, int)

{

    unsigned char normalizada =

        static_cast<unsigned char>(std::tolower(tecla)); //E um tipo de variável utilizado para armazenar um caractere

    teclas[normalizada] = false;

}

// ============================================================

// MOVIMENTAÇÃO CONTÍNUA

// ============================================================

void movimentar()

{

    float dx = 0.0f;

    float dz = 0.0f;

    float frenteX = std::sin(yaw);

    float frenteZ = -std::cos(yaw);

    float lateralX = std::cos(yaw);

    float lateralZ = std::sin(yaw);

    if (teclas['w'])

    {

        dx += frenteX;

        dz += frenteZ;

    }

    if (teclas['s'])

    {

        dx -= frenteX;

        dz -= frenteZ;

    }

    if (teclas['d'])

    {

        dx += lateralX;

        dz += lateralZ;

    }

    if (teclas['a'])

    {

        dx -= lateralX;

        dz -= lateralZ;

    }

    // Normaliza a velocidade para não andar mais rápido

    // na diagonal.

    float comprimento = std::sqrt(dx * dx + dz * dz);

    if (comprimento > 0.0f)

    {

        dx = (dx / comprimento) * velocidade;

        dz = (dz / comprimento) * velocidade;

        float novoX = jogadorX + dx;

        float novoZ = jogadorZ + dz;

        if (podeMover(novoX, novoZ))

        {

            jogadorX = novoX;

            jogadorZ = novoZ;

        }

        else

        {

            // Permite deslizar junto à parede.

            if (podeMover(jogadorX + dx, jogadorZ))

            {

                jogadorX += dx;

            }

            if (podeMover(jogadorX, jogadorZ + dz))

            {

                jogadorZ += dz;

            }

        }

        glutPostRedisplay();

    }

}

// ============================================================

// TEMPORIZADOR

// ============================================================

void atualizar(int)

{

    movimentar();

    glutTimerFunc(16, atualizar, 0);// TODA VEZ Q ELE SE MOVIMENTAR ELA VAI ATUALIZA OS QUADROS POR SEGUDOS E ESSE 16 E EQUIVALENTE A 62,5 QUADROS POR SEGUNDOS.

}

// ============================================================

// MOUSE

// ============================================================

void movimentarMouse(int x, int y)

{

    // Eventos gerados pelo próprio reposicionamento são ignorados.

    if (x == centroMouseX && y == centroMouseY) // VAI VERIFIC A POSIÇÃO

    {

        return;

    }

    int diferencaX = x - centroMouseX; // CALCULO  DE DESLOCAMNETO

    int diferencaY = y - centroMouseY;

    float sensibilidade = 0.003f; // COMO NOME JA DIZ SINSIBILIDADE DI MOUSE

    yaw += diferencaX * sensibilidade; // ROTAÇÃO DA CAMERA

    pitch -= diferencaY * sensibilidade;

    pitch = std::max(-1.2f, std::min(1.2f, pitch));//representa a rotação vertical da câmera.

    // Reposiciona o cursor no centro.

    glutWarpPointer(centroMouseX, centroMouseY);

    glutPostRedisplay();//E PARA JANELA SER REDESENHADA

}

// ============================================================

// REDIMENSIONAMENTO

// ============================================================

void redimensionar(int largura, int altura)// TAMANHO DA JANELA

{

    if (altura <= 0)

    {

altura = 1;

    }

    larguraJanela = largura;

    alturaJanela = altura;

    centroMouseX = largura / 2;

    centroMouseY = altura / 2;

    glViewport(0, 0, largura, altura);//Define a região da janela onde o OpenGL vai desenhar.

    configurarProjecao();

}

// ============================================================

// ILUMINAÇÃO

// ============================================================

void configurarIluminacao()

{

    glEnable(GL_LIGHTING);

    glEnable(GL_LIGHT0);

    GLfloat posicao[] = {

        0.0f, 3.8f, 0.0f, 1.0f

    }; // X = 0.Y = 3,8.Z = 0.W = 1. O último valor indica que é uma luz posicionada no espaço, e não uma luz direcional

    GLfloat ambiente[] = {

        0.45f, 0.45f, 0.45f, 1.0f

    };

    GLfloat difusa[] = {
        0.85f, 0.85f, 0.85f, 1.0f
    };

    GLfloat especularLuz[] = {
        1.0f, 1.0f, 1.0f, 1.0f
    };

    GLfloat especularMaterial[] = {
        0.75f, 0.75f, 0.75f, 1.0f
    };

//Ambiente: iluminação geral do ambiente.
//Difusa: iluminação que depende do ângulo entre a luz e a superfície.
//Especular: brilho refletido pela superfície.
//Phong é um modelo de iluminação, enquanto Gouraud é uma técnica de tonalização.
    glLightfv(GL_LIGHT0, GL_POSITION, posicao);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambiente);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, difusa);
    glLightfv(GL_LIGHT0, GL_SPECULAR, especularLuz);

    // Componente especular e brilho do modelo de iluminação Phong.
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, especularMaterial);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 48.0f);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

    glEnable(GL_COLOR_MATERIAL);//afetando os OBJETOS

    glColorMaterial(

        GL_FRONT_AND_BACK,

        GL_AMBIENT_AND_DIFFUSE

    );//afetando os componentes ambiente PELA FRENTE E ATRAS

    glEnable(GL_NORMALIZE); //Normaliza os vetores normais das superfícies.seu projeto utiliza o modelo de iluminação Phong do pipeline fixo do OpenGL com interpolação Gouraud.

}

// ============================================================

// DESENHO PRINCIPAL

// ============================================================

void desenhar()

{

    glClear(

        GL_COLOR_BUFFER_BIT |

        GL_DEPTH_BUFFER_BIT

    );//VAI LIMPAR OS BUFFER ANTES DE DESENHAR UM NOVO QUADRO

    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();

    float direcaoX = std::sin(yaw) * std::cos(pitch); // AQUI E DIREÇÃO DA CAMERA ONDE ESTA OLHADO

    float direcaoY = std::sin(pitch);

    float direcaoZ = -std::cos(yaw) * std::cos(pitch);

    gluLookAt(

        jogadorX,

        jogadorY,

        jogadorZ,

        jogadorX + direcaoX,

        jogadorY + direcaoY,

        jogadorZ + direcaoZ,

        0.0f, 1.0f, 0.0f

    );//AQUI Define a posição e a orientação da câmera.

    configurarIluminacao();

    desenharExterior();

    desenharSala();

    glutSwapBuffers();//TROCA OS BUFFERS

}

// ============================================================

// INICIALIZAÇÃO

// ============================================================

void inicializar()

{

    glClearColor(

        0.45f,

        0.60f,

        0.75f,

        1.0f

    );

    glEnable(GL_DEPTH_TEST); // Z-Buffer: determina quais superfícies estão visíveis.

    glDisable(GL_CULL_FACE);

    configurarProjecao();

    configurarIluminacao();

    glShadeModel(GL_SMOOTH); // Tonalização Gouraud: interpola as cores entre os vértices.

}

// ============================================================

// MAIN

// ============================================================

int main(int argc, char** argv)

{

    glutInit(&argc, argv);//  AQUI INICIALIZA O openGL

    glutInitDisplayMode(

        GLUT_DOUBLE |

        GLUT_RGB |

        GLUT_DEPTH

    );// DOIS BUFF DE IMAGENS, RGB, HABILITA BUFFER DE PROFUNDIDADE

    glutInitWindowSize(

        larguraJanela,

        alturaJanela

    );// DEFINI O TAMANHO DA JANELA

    glutCreateWindow(

        "Laboratorio 3D - OpenGL"

    ); // CRI JANELA COM TITULO

    inicializar();

    glutDisplayFunc(desenhar);

    glutReshapeFunc(redimensionar);

    // Teclado contínuo

    glutKeyboardFunc(teclaPressionada);

    glutKeyboardUpFunc(teclaSolta);

    // Mouse

    glutPassiveMotionFunc(movimentarMouse);

    // Esconde o cursor

    glutSetCursor(GLUT_CURSOR_NONE);

    // Centraliza inicialmente

    glutWarpPointer(

        centroMouseX,

        centroMouseY

    );

    // Atualização contínua

    glutTimerFunc(16, atualizar, 0);

    glutMainLoop();// VAI UM LOOP

    return 0;

}

//O projeto foi desenvolvido utilizando C++ e OpenGL com a biblioteca FreeGLUT. O ambiente é construído por meio de primitivas geométricas, principalmente cubos, que são modificados utilizando operações de translação, rotação e escala.

//Para a visualização, foi implementada uma câmera em primeira pessoa. Sua orientação é controlada pelo mouse por meio dos ângulos Yaw e Pitch, que são convertidos em um vetor de direção utilizando funções trigonométricas.

// movimentação é controlada pelo teclado, com armazenamento do estado das teclas e atualização periódica por temporizador.

//Também foi implementado um sistema de detecção de colisões para controlar a movimentação do jogador próximo às paredes e portas.

//Por fim, a cena utiliza projeção perspectiva, iluminação e renderização com buffer duplo para apresentar o laboratório tridimensional na tela.