#include "application_ui.h"
#include "SDL2_gfxPrimitives.h"
#include <vector>
#include <list>
#include <map>
#include <queue>
#include <algorithm>
using namespace std;

#define EPSILON 0.0001f

struct Coords
{
    int x, y;

    bool operator==(const Coords& other)
    {
        return x == other.x and y == other.y;
    }
};

struct Segment
{
    Coords p1, p2;
};

struct Triangle
{
    Coords p1, p2, p3;
    bool complet=false;
};

struct Application
{
    int width, height;
    Coords focus{100, 100};

    std::vector<Coords> points;
    std::vector<Triangle> triangles;
};

bool compareCoords(Coords point1, Coords point2)
{
    if (point1.y == point2.y)
        return point1.x < point2.x;
    return point1.y < point2.y;
}

void drawPoints(SDL_Renderer *renderer, const std::vector<Coords> &points)
{
    for (std::size_t i = 0; i < points.size(); i++)
    {
        filledCircleRGBA(renderer, points[i].x, points[i].y, 3, 240, 240, 23, SDL_ALPHA_OPAQUE);
    }
}

void drawSegments(SDL_Renderer *renderer, const std::vector<Segment> &segments)
{
    for (std::size_t i = 0; i < segments.size(); i++)
    {
        lineRGBA(
            renderer,
            segments[i].p1.x, segments[i].p1.y,
            segments[i].p2.x, segments[i].p2.y,
            240, 240, 20, SDL_ALPHA_OPAQUE);
    }
}

void drawTriangles(SDL_Renderer *renderer, const std::vector<Triangle> &triangles)
{
    for (std::size_t i = 0; i < triangles.size(); i++)
    {
        const Triangle& t = triangles[i];
        trigonRGBA(
            renderer,
            t.p1.x, t.p1.y,
            t.p2.x, t.p2.y,
            t.p3.x, t.p3.y,
            0, 240, 160, SDL_ALPHA_OPAQUE
        );
    }
}

void draw(SDL_Renderer *renderer, const Application &app)
{
    /* Remplissez cette fonction pour faire l'affichage du jeu */
    int width, height;
    SDL_GetRendererOutputSize(renderer, &width, &height);

    drawPoints(renderer, app.points);
    drawTriangles(renderer, app.triangles);
}

/*
   Détermine si un point se trouve dans un cercle définit par trois points
   Retourne, par les paramètres, le centre et le rayon
*/
bool CircumCircle(
    float pX, float pY,
    float x1, float y1, float x2, float y2, float x3, float y3,
    float *xc, float *yc, float *rsqr
)
{
    float m1, m2, mx1, mx2, my1, my2;
    float dx, dy, drsqr;
    float fabsy1y2 = fabs(y1 - y2);
    float fabsy2y3 = fabs(y2 - y3);

    /* Check for coincident points */
    if (fabsy1y2 < EPSILON && fabsy2y3 < EPSILON)
        return (false);

    if (fabsy1y2 < EPSILON)
    {
        m2 = -(x3 - x2) / (y3 - y2);
        mx2 = (x2 + x3) / 2.0;
        my2 = (y2 + y3) / 2.0;
        *xc = (x2 + x1) / 2.0;
        *yc = m2 * (*xc - mx2) + my2;
    }
    else if (fabsy2y3 < EPSILON)
    {
        m1 = -(x2 - x1) / (y2 - y1);
        mx1 = (x1 + x2) / 2.0;
        my1 = (y1 + y2) / 2.0;
        *xc = (x3 + x2) / 2.0;
        *yc = m1 * (*xc - mx1) + my1;
    }
    else
    {
        m1 = -(x2 - x1) / (y2 - y1);
        m2 = -(x3 - x2) / (y3 - y2);
        mx1 = (x1 + x2) / 2.0;
        mx2 = (x2 + x3) / 2.0;
        my1 = (y1 + y2) / 2.0;
        my2 = (y2 + y3) / 2.0;
        *xc = (m1 * mx1 - m2 * mx2 + my2 - my1) / (m1 - m2);
        if (fabsy1y2 > fabsy2y3)
        {
            *yc = m1 * (*xc - mx1) + my1;
        }
        else
        {
            *yc = m2 * (*xc - mx2) + my2;
        }
    }

    dx = x2 - *xc;
    dy = y2 - *yc;
    *rsqr = dx * dx + dy * dy;

    dx = pX - *xc;
    dy = pY - *yc;
    drsqr = dx * dx + dy * dy;

    return ((drsqr - *rsqr) <= EPSILON ? true : false);
}

void construitVoronoi(Application &app)
{
    sort(app.points.begin(), app.points.end(), compareCoords);
    // Création d'un triangle très grand qui contient tous les points
    // Vider la liste existante de triangles
    app.triangles.clear();

    // Créer un trés grand triangle (-1000, -1000); (500, 3000); (1500, -1000)
    Triangle superTriangle={{-1000,-1000},{5000,3000},{1500,-1000}};
    app.triangles.push_back(superTriangle);

    // Pour chaque point
    for (auto& p : app.points) {
        // Créer une liste de segments
        vector<Segment> LS;

        // Pour chaque triangle déjà créé
        for (auto& t : app.triangles) {
            // Tester si le cercle circonscrit contient le point
            float xc, yc, rsqr;
            if (CircumCircle(p.x, p.y, t.p1.x, t.p1.y, t.p2.x, t.p2.y, t.p3.x, t.p3.y, &xc, &yc, &rsqr)) {
                // Récupérer les différents segments de ce triangle
                LS.push_back({t.p1, t.p2});
                LS.push_back({t.p2, t.p3});
                LS.push_back({t.p3, t.p1});
               
                // Enlever le triangle de la liste
                 t.complet=true;

                auto last = app.triangles.end() - 1;
                iter_swap(find(app.triangles.begin(), last, t), last);
                app.triangles.pop_back();
            }
        }

        // Pour chaque segment de la liste de segments
        for (auto& s1 : LS) {
            for (auto& s2 : LS) {
                // Si un segment partage ses points avec un autre segment, le virer
                if (s1.p1 == s2.p2 && s1.p2 == s2.p1) {
                   swap(s1,s2);
                   LS.pop_back();

                }
            }
            
        }
    }
    //on créé de nouveaux triangle 
    // app.triangles.push_back(Triangle(s1.p1, s1.p2, p));
    // // Enlever les triangles qui contiennent un sommet du superTriangle
    //    auto iter = app.triangles.begin();
    // while (iter != app.triangles.end()) {
    //     if (contientPoint(superTriangle, (*iter)[0]) || 
    //         contientPoint(superTriangle, (*iter)[1]) ||
    //         contientPoint(superTriangle, (*iter)[2])) {
    //         iter = app.triangles.erase(iter);
    //     }
    //     else {
    //         iter++;
    //     }
    // }

}    

bool handleEvent(Application &app)
{
    /* Remplissez cette fonction pour gérer les inputs utilisateurs */
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            return false;
        else if (e.type == SDL_WINDOWEVENT_RESIZED)
        {
            app.width = e.window.data1;
            app.height = e.window.data1;
        }
        else if (e.type == SDL_MOUSEWHEEL)
        {
        }
        else if (e.type == SDL_MOUSEBUTTONUP)
        {
            if (e.button.button == SDL_BUTTON_RIGHT)
            {
                app.focus.x = e.button.x;
                app.focus.y = e.button.y;
                app.points.clear();
            }
            else if (e.button.button == SDL_BUTTON_LEFT)
            {
                app.focus.y = 0;
                app.points.push_back(Coords{e.button.x, e.button.y});
                construitVoronoi(app);
            }
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    SDL_Window *gWindow;
    SDL_Renderer *renderer;
    Application app{720, 720, Coords{0, 0}};
    bool is_running = true;

    // Creation de la fenetre
    gWindow = init("Awesome Voronoi", 720, 720);

    if (!gWindow)
    {
        SDL_Log("Failed to initialize!\n");
        exit(1);
    }

    renderer = SDL_CreateRenderer(gWindow, -1, 0); // SDL_RENDERER_PRESENTVSYNC

    /*  GAME LOOP  */
    while (true)
    {
        // INPUTS
        is_running = handleEvent(app);
        if (!is_running)
            break;

        // EFFACAGE FRAME
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // DESSIN
        draw(renderer, app);

        // VALIDATION FRAME
        SDL_RenderPresent(renderer);

        // PAUSE en ms
        SDL_Delay(1000 / 30);
    }

    // Free resources and close SDL
    close(gWindow, renderer);

    return 0;
}
