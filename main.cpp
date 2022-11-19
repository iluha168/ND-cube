#include "OOPWinAPi/OOPWinAPI.h"
#include <thread>
#include <algorithm>
#include <cmath>
#include <map>
#include <vector>
#include <bitset>
#include <numeric>
#include <string>
#include <dwmapi.h>
#include "matrix.h"

#define DOT_SIZE 3.L
#define R 120.L
#define EDGE_SAMPLES 200.L
#define DIMENSIONS 4ull
#define SCALE (150.L*DIMENSIONS)

///degrees of freedom (basic rotation)
const size_t dof = DIMENSIONS*(DIMENSIONS-1ull)/2ull;
const size_t verticesAmount = std::pow(2ull, DIMENSIONS);
std::vector<Matrix<DIMENSIONS, DIMENSIONS>> rotMatrices1;
std::array<floating, dof> rotation;
typedef std::array<floating, DIMENSIONS> coords;

floating flerp(floating a, floating b, floating percent){
    return percent*(b-a)+a;
}
coords flerp(coords a, coords b, floating percent){
    coords ans;
    for(size_t dimension = 0; dimension < DIMENSIONS; dimension++)
        ans[dimension] = flerp(a[dimension], b[dimension], percent);
    return ans;
}
floating delerp(floating a, floating b, floating value){
    return (value-a)/(b-a);
}
floating rerange(floating a0, floating b0, floating a1, floating b1, floating value){
    return (value-a1)/(b1-a1)*(b0-a0)+a0;
}


class Point {
public:
    const coords pos;
    COLORREF color;
    Point(coords pos): pos(pos),
        color(Point::colorFromPos(pos)){
    }
    static COLORREF colorFromPos(coords& pos){
        return RGB(
            rerange(0,255, -R,R, pos[0]),
            rerange(0,255, -R,R, pos[1]),
            rerange(0,255, -R*(DIMENSIONS-2),R*(DIMENSIONS-2), std::accumulate(pos.begin()+2, pos.end(), 0))
        );
    }
    coords getRealPos(){
        coords realPos = pos;
        for(size_t i = 0; i < dof; i++){
            Matrix<DIMENSIONS, DIMENSIONS> rotM = rotationMatrixPart2(rotMatrices1[i], rotation[i]);
            realPos = realPos * rotM;
        }
        return realPos;
    }
};

std::vector<Point*> points;

RECT CreateRect(LONG x, LONG y, LONG w, LONG h){
    return {x, y, x+w, y+h};
}

class MainWindow : public OOPWinAPI::Window {
    void OnClose(){
        exit(0);
    }

    const HBRUSH backgroundColor = CreateSolidBrush(RGB(100, 100, 100));
    void OnRepaint(){
        PAINTSTRUCT pstruct;
        BeginPaint(hWnd, &pstruct);
        HDC dc = GetDC(hWnd);
        HDC memdc = CreateCompatibleDC(dc);
        //find window center
        const int w = this->width(), h = this->height();
        LONG centerX  = w/2, centerY = h/2;
        //
        HBITMAP membitmap = CreateCompatibleBitmap(dc, w, h);
        SelectObject(memdc, membitmap);
        FillRect(memdc, &pstruct.rcPaint, backgroundColor);
        //calculate 2D canvas pos
        std::vector<coords> posInND;
        posInND.reserve(verticesAmount);
        for(Point*& point : points)
            posInND.push_back(point->getRealPos());
        /* Paint here */
        for(size_t i = 0; i < verticesAmount; i++){
            Point*& A = points[i];
            for(size_t j = i+1; j<verticesAmount; j++){
                Point*& B = points[j];
                size_t sameAxesCount = 0;
                for(size_t dimension = 0; dimension < DIMENSIONS; dimension++){
                    if(A->pos[dimension] == B->pos[dimension])
                        sameAxesCount++;
                }
                if(sameAxesCount == DIMENSIONS-1){ //draw line between A and B
                    for(size_t sample = 0; sample <= EDGE_SAMPLES; sample++){
                        coords pos = flerp(A->pos, B->pos, sample/EDGE_SAMPLES);
                        const HBRUSH brush = CreateSolidBrush(Point::colorFromPos(pos));
                               pos = flerp(posInND[i], posInND[j], sample/EDGE_SAMPLES);
                        const floating scale = std::accumulate(pos.begin()+2, pos.end(), 0)/SCALE +1;
                        const RECT dotRect = CreateRect(
                            (LONG)pos[0]/scale + centerX,
                            (LONG)pos[1]/scale + centerY,
                            DOT_SIZE/scale, DOT_SIZE/scale
                        );
                        FillRect(memdc, &dotRect, brush);
                        DeleteObject(brush);
                    }
                }
            }
        }
        //draw rotations
        for(size_t i = 0; i < dof; i++){
            std::wstring txt = L" ["+std::to_wstring(i)+L"]: "+std::to_wstring(rotation[i]);
            RECT txtpos{0, static_cast<LONG>(i*18), 36, static_cast<LONG>(i*18+18)};
            DrawTextW(memdc, txt.c_str(), -1, &txtpos, DT_TOP|DT_LEFT|DT_NOCLIP|DT_SINGLELINE);
        }
        //end paint
        BitBlt(dc, 0, 0, w, h, memdc, 0, 0, SRCCOPY);
        DeleteObject(membitmap);
        DeleteDC(memdc);
        DeleteDC(dc);
        EndPaint(hWnd, &pstruct);
    }

    size_t rotPlane = 0;
    void OnMouseWheel(short&, short& d, short&, short&){
        rotation[rotPlane] += copysign(1.0, d) * 0.2L;
        this->redraw();
    }
    void OnMousePress(WPARAM&, short&, short&){
        rotPlane++;
        rotPlane %= dof;
    }
public:
    MainWindow() : OOPWinAPI::Window(
        L"Cube!", CW_USEDEFAULT,CW_USEDEFAULT,800,800,
        WS_OVERLAPPEDWINDOW
    ){}
    ~MainWindow(){
        DeleteBrush(backgroundColor);
    }
};
MainWindow window;

int init()
{
    /* Generate cube */
    //vertices
    for(unsigned i = 0; i < verticesAmount; i++){
        auto bits = std::bitset<CHAR_BIT * sizeof(unsigned)>(i);
        coords vertex;
        for(size_t i = 0; i < DIMENSIONS; i++)
            vertex[i] = (floating)(bits[i] ? R : -R);
        points.push_back(new Point(vertex));
    }
    /* Generate all possible combinations of rotation axes */
    std::string bitmask(DIMENSIONS-2, 1);
    bitmask.resize(DIMENSIONS, 0);
    do {
        Matrix<DIMENSIONS, DIMENSIONS-2> rotAxis;
        size_t col = 0;
        for (size_t i = 0; i < DIMENSIONS; i++)
        if (bitmask[i]){
            for(size_t row = 0; row < DIMENSIONS; row++)
                rotAxis.data[row][col] = row==i;
            col++;
        }
        rotMatrices1.push_back(rotationMatrixPart1(rotAxis));
    } while(std::prev_permutation(bitmask.begin(), bitmask.end()));
    //z-order all points
    if(DIMENSIONS > 2)
    std::sort(points.begin(), points.end(), [](Point*& a, Point*& b){
        return a->pos[2] > b->pos[2];
    });
    // gui loop
    window.show();
    /*
    std::thread([]{
        for(;;){
            DwmFlush();
            for(size_t i = 0; i < dof; i++)
                rotation[i] += (i+1)*0.0001L;
            window.redraw();
        }
    }).detach();
    */
    return 0;
}
