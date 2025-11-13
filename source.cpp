#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cmath>

using namespace std;
using namespace sf;

#define BOARD_SIZE 12
#define MAX_LEN 15        // giới hạn kí tự nhập khi lưu file
#define CONFIG_X_O_x 7.0f // căn chỉnh x của X,O
#define CONFIG_X_y 0.05f  // căn chỉnh y của X
#define CONFIG_O_y 0.05f  // căn chỉnh y của O

struct _POINT
{
    int x, y, c;
}; // x,y là chỉ số hàng, c=0: trống, -1:X, 1:O

_POINT _TABLE[BOARD_SIZE][BOARD_SIZE];
bool _TURN = true;          // true: X, false: O
int selRow = 0, selCol = 0; // chọn ô hiện tại (hàng, cột)

// GUI parameters
const int CELL = 50;                              // kích thước ô (pixels)
const int MARGIN = 30;                            // lề trái / trên
int WIN_W = MARGIN * 2 + CELL * BOARD_SIZE;       // ngang
int WIN_H = MARGIN * 2 + CELL * BOARD_SIZE + 180; // extra dưới để in thông báo (dọc)

Font gFont;         // 1 class trong sfml. Cho phép load font vào biến gfont.
Font gMenuFont;     // font cho menu chính
Clock glowEffClock; // bộ đếm thời gian cho hiệu ứng viền ô chọn
Clock menuGlowClock;

struct Wave
{ // hiệu ứng sóng năng lượng khi đánh
    Vector2f center;
    Color color;
    float elapsed;
    float duration;
    float maxRadius;
};
static vector<Wave> gWaves;

// trạng thái menu
enum GameState
{
    MENU,
    PLAYING,
    LOADGAME,
    SETTINGS
};
GameState gState = MENU;

// biến chọn menu
int menuSelection = 0;
int menSel = 0;

// ===== MODEL =====
void ResetData()
{
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            _TABLE[i][j].x = i;
            _TABLE[i][j].y = j;
            _TABLE[i][j].c = 0;
        }
    }
    _TURN = true;
    selRow = 0;
    selCol = 0;
}

bool isFull()
{
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            if (_TABLE[i][j].c == 0)
                return false; // trả về false nếu tìm thấy ô trống
        }
    }
    return true;
}

int TestBoard()
{
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            int c = _TABLE[i][j].c;
            if (c == 0)
                continue;
            int dx[4] = {1, 0, 1, 1};
            int dy[4] = {0, 1, 1, -1};
            for (int k = 0; k < 4; k++)
            {
                int cnt = 0;
                for (int t = 0; t < 5; t++)
                {
                    int x = i + dx[k] * t;
                    int y = j + dy[k] * t;
                    if (x < 0 || y < 0 || x >= BOARD_SIZE || y >= BOARD_SIZE)
                        break;
                    if (_TABLE[x][y].c == c)
                        cnt++;
                    else
                        break;
                }
                if (cnt == 5)
                    return c; //-1 hoặc 1
            }
        }
    if (isFull())
        return 0; // hòa
    return 2;     // chưa ai thắng
}

// đặt vị trí nếu trống
int CheckBoard(int row, int col)
{
    if (row < 0 || col < 0 || row >= BOARD_SIZE || col >= BOARD_SIZE)
        return 0;
    if (_TABLE[row][col].c == 0)
    {
        _TABLE[row][col].c = (_TURN ? -1 : 1);
        return _TABLE[row][col].c;
    }
    return 0;
}

// ===== UI helpers =====
Vector2f cellTopLeft(int row, int col) // Vector2f : vecto 2 chiều số thực
{
    return Vector2f((float)(MARGIN + col * CELL), (float)(MARGIN + row * CELL));
} // xác định vị trí góc trên trái của ô đang đứng

string InputDialog(RenderWindow &win, const String &prompt)
{
    // Hiển thị hộp nhập tên file (chuỗi), trả về tên (không kèm .txt)
    String input;
    bool done = false;
    Event event;
    Clock cursorClock;
    bool showCursor = true;
    while (!done)
    {
        while (win.pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                win.close();
                return "";
            }
            else if (event.type == Event::TextEntered)
            {
                Uint32 unicode = event.text.unicode;
                if (event.text.unicode == 13)
                    done = true;
                else if (event.text.unicode == 8)
                { // backspace
                    if (!input.isEmpty())
                        input.erase(input.getSize() - 1, 1);
                }
                else if (unicode >= 32 && unicode < 127)
                { // chặn một số ASCII đặc biệt
                    if (input.getSize() < MAX_LEN)
                    {
                        char ch = static_cast<char>(unicode);
                        const string forbidden = "\\/:*?\"<>|~`!@#$%^&*(){}[];',.+=";
                        if (forbidden.find(ch) == string::npos)
                            input += ch;
                    }
                }
            }
            else if (event.type == Event::KeyPressed)
            {
                if (event.key.code == Keyboard::Enter)
                    done = true;
                else if (event.key.code == Keyboard::Escape)
                {
                    input.clear();
                    done = true;
                }
            }
        }

        // blink cursor
        if (cursorClock.getElapsedTime().asMilliseconds() > 500)
        {
            showCursor = !showCursor;
            cursorClock.restart();
        }

        // vẽ dialog
        win.clear(Color::White);

        // bảng
        RectangleShape bg(Vector2f((float)WIN_W, (float)WIN_H));
        bg.setFillColor(Color::White);
        win.draw(bg);

        // vẽ lưới
        for (int i = 0; i <= BOARD_SIZE; i++)
        {
            Vertex line[] = {
                Vertex(Vector2f(MARGIN, MARGIN + i * CELL), Color::Black),
                Vertex(Vector2f(MARGIN + BOARD_SIZE * CELL, MARGIN + i * CELL), Color::Black)};
            win.draw(line, 2, Lines);
        }

        for (int j = 0; j <= BOARD_SIZE; j++)
        {
            Vertex line[] = {
                Vertex(Vector2f(MARGIN + j * CELL, MARGIN), Color::Black),
                Vertex(Vector2f(MARGIN + j * CELL, MARGIN + BOARD_SIZE * CELL), Color::Black)};
            win.draw(line, 2, Lines);
        }

        // draw marks
        for (int r = 0; r < BOARD_SIZE; r++)
        {
            for (int c = 0; c < BOARD_SIZE; c++)
            {
                if (_TABLE[r][c].c == -1)
                {
                    // X color blue
                    Text t("X", gFont, CELL * 2 / 3);
                    t.setFillColor(Color(0, 0, 180));
                    t.setPosition(cellTopLeft(r, c).x + CELL * 0.15f + CONFIG_X_O_x, cellTopLeft(r, c).y + CELL * 0.05f + CONFIG_X_y);
                    win.draw(t);
                }
                else if (_TABLE[r][c].c == 1)
                {
                    Text t("O", gFont, CELL * 2 / 3);
                    t.setFillColor(Color(180, 0, 0));
                    t.setPosition(cellTopLeft(r, c).x + CELL * 0.15f + CONFIG_X_O_x, cellTopLeft(r, c).y + CELL * 0.02f + CONFIG_O_y);
                    win.draw(t);
                }
            }
        }

        // dialog box
        RectangleShape dialog(Vector2f(600.f, 120.f));
        dialog.setFillColor(Color(230, 230, 230));
        dialog.setOutlineColor(Color::Black);
        dialog.setOutlineThickness(2.f);
        dialog.setPosition((WIN_W - dialog.getSize().x) / 2.f, WIN_H - dialog.getSize().y - 10.f);
        win.draw(dialog);

        Text p(prompt, gFont, 18);
        p.setFillColor(Color::Black);
        p.setPosition(dialog.getPosition() + Vector2f(12.f, 8.f));
        win.draw(p);

        Text typed(input + (showCursor ? "_" : ""), gFont, 20);
        typed.setFillColor(Color::Black);
        typed.setPosition(dialog.getPosition() + Vector2f(12.f, 40.f));
        win.draw(typed);

        Text hint(L"Enter = OK, Esc = Cancel, Backspace để xóa", gFont, 14);
        hint.setFillColor(Color::Black);
        hint.setPosition(dialog.getPosition() + Vector2f(12.f, 78.f));
        win.draw(hint);

        win.display();
    }
    return input.toAnsiString();
}

int ShowMessageYesNo(RenderWindow &win, const String &msg)
{
    // Hiện hộp thoại với nội dung msg, chờ Y (tiếp tục) hoặc phím khác (thoát),
    // trả về 1 nếu Y, 0 nếu khác.
    bool waiting = true;
    Event event;
    while (waiting && win.isOpen())
    {
        while (win.pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                win.close();
                return 0;
            }
            if (event.type == Event::KeyPressed)
            {
                if (event.key.code == Keyboard::Y)
                    return 1;
                else
                    return 0;
            }
        }
        // vẽ modal
        win.clear(Color::White);

        // vẽ lưới
        for (int i = 0; i <= BOARD_SIZE; i++)
        {
            Vertex line[] = {
                Vertex(Vector2f((float)(MARGIN), (float)(MARGIN + i * CELL)), Color::Black),
                Vertex(Vector2f((float)(MARGIN + BOARD_SIZE * CELL), (float)(MARGIN + i * CELL)), Color::Black)};
            win.draw(line, 2, Lines);
        }
        for (int j = 0; j <= BOARD_SIZE; j++)
        {
            Vertex line[] = {
                Vertex(Vector2f((float)(MARGIN + j * CELL), (float)(MARGIN)), Color::Black),
                Vertex(Vector2f((float)(MARGIN + j * CELL), (float)(MARGIN + BOARD_SIZE * CELL)), Color::Black)};
            win.draw(line, 2, Lines);
        }
        // marks
        for (int r = 0; r < BOARD_SIZE; r++)
        {
            for (int c = 0; c < BOARD_SIZE; c++)
            {
                if (_TABLE[r][c].c == -1)
                {
                    Text t("X", gFont, CELL * 2 / 3);
                    t.setFillColor(Color(0, 0, 180));
                    t.setPosition(cellTopLeft(r, c).x + CELL * 0.15f + CONFIG_X_O_x, cellTopLeft(r, c).y + CELL * 0.05f + CONFIG_X_y);
                    win.draw(t);
                }
                else if (_TABLE[r][c].c == 1)
                {
                    Text t("O", gFont, CELL * 2 / 3);
                    t.setFillColor(Color(180, 0, 0));
                    t.setPosition(cellTopLeft(r, c).x + CELL * 0.15f + CONFIG_X_O_x, cellTopLeft(r, c).y + CELL * 0.02f + CONFIG_O_y);
                    win.draw(t);
                }
            }
        }

        // dialog
        RectangleShape dialog(Vector2f(520.f, 100.f));
        dialog.setFillColor(Color(240, 240, 240));
        dialog.setOutlineColor(Color::Black);
        dialog.setOutlineThickness(2.f);
        dialog.setPosition((WIN_W - dialog.getSize().x) / 2.f, (WIN_H - dialog.getSize().y) / 2.f);
        win.draw(dialog);

        Text tmsg(msg, gFont, 20);
        tmsg.setFillColor(Color::Black);
        tmsg.setPosition(dialog.getPosition() + Vector2f(12.f, 18.f));
        win.draw(tmsg);

        Text th(L"Nhấn 'Y' để tiếp tục, phím khác để thoát.", gFont, 16);
        th.setFillColor(Color::Black);
        th.setPosition(dialog.getPosition() + Vector2f(12.f, 56.f));
        win.draw(th);

        win.display();
    }
    return 0;
}

void ShowMessageOK(RenderWindow &win, const String &msg)
{
    // Hiển thị hộp thoại đơn, chờ 1 phím bất kỳ hoặc click
    bool waiting = true;
    Event event;
    while (waiting && win.isOpen())
    {
        while (win.pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                win.close();
                return;
            }
            if (event.type == Event::KeyPressed || event.type == Event::MouseButtonPressed)
                return;
        }
        // vẽ modal (giống trên)
        win.clear(Color::White);
        // vẽ lưới
        for (int i = 0; i <= BOARD_SIZE; i++)
        {
            Vertex line[] = {
                Vertex(Vector2f((float)(MARGIN), (float)(MARGIN + i * CELL)), Color::Black),
                Vertex(Vector2f((float)(MARGIN + BOARD_SIZE * CELL), (float)(MARGIN + i * CELL)), Color::Black)};
            win.draw(line, 2, Lines);
        }
        for (int j = 0; j <= BOARD_SIZE; j++)
        {
            Vertex line[] = {
                Vertex(Vector2f((float)(MARGIN + j * CELL), (float)(MARGIN)), Color::Black),
                Vertex(Vector2f((float)(MARGIN + j * CELL), (float)(MARGIN + BOARD_SIZE * CELL)), Color::Black)};
            win.draw(line, 2, Lines);
        }
        // marks
        for (int r = 0; r < BOARD_SIZE; r++)
        {
            for (int c = 0; c < BOARD_SIZE; c++)
            {
                if (_TABLE[r][c].c == -1)
                {
                    Text t("X", gFont, CELL * 2 / 3);
                    t.setFillColor(Color(0, 0, 180));
                    t.setPosition(cellTopLeft(r, c).x + CELL * 0.15f + CONFIG_X_O_x, cellTopLeft(r, c).y + CELL * 0.05f + CONFIG_X_y);
                    win.draw(t);
                }
                else if (_TABLE[r][c].c == 1)
                {
                    Text t("O", gFont, CELL * 2 / 3);
                    t.setFillColor(Color(180, 0, 0));
                    t.setPosition(cellTopLeft(r, c).x + CELL * 0.15f + CONFIG_X_O_x, cellTopLeft(r, c).y + CELL * 0.02f + CONFIG_O_y);
                    win.draw(t);
                }
            }
        }

        RectangleShape dialog(Vector2f(520.f, 80.f));
        dialog.setFillColor(Color(240, 240, 240));
        dialog.setOutlineColor(Color::Black);
        dialog.setOutlineThickness(2.f);
        dialog.setPosition((WIN_W - dialog.getSize().x) / 2.f, (WIN_H - dialog.getSize().y) / 2.f);
        win.draw(dialog);

        Text tmsg(msg, gFont, 20);
        tmsg.setFillColor(Color::Black);
        tmsg.setPosition(dialog.getPosition() + Vector2f(12.f, 12.f));
        win.draw(tmsg);

        Text th(L"Nhấn phím bất kì để tiếp tục.", gFont, 14);
        th.setFillColor(Color::Black);
        th.setPosition(dialog.getPosition() + Vector2f(12.f, 44.f));
        win.draw(th);

        win.display();
    }
}

// ===== SAVE / LOAD =====
void SaveGame(RenderWindow &win)
{
    string name = InputDialog(win, L"Nhập tên file để lưu (không kèm .txt):");
    if (name.empty())
    {
        ShowMessageOK(win, L"Hủy lưu.");
        return;
    }
    ofstream f(name + ".txt");
    if (!f.is_open())
    {
        ShowMessageOK(win, L"Không thể mở file để ghi!");
        return;
    }
    f << _TURN << "\n";
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
            f << _TABLE[i][j].c << " ";
        f << "\n";
    }
    f.close();
    ShowMessageOK(win, L"Đã lưu " + (String)name + L".txt");
}

void LoadGame(RenderWindow &win)
{
    string name = InputDialog(win, L"Nhập tên file để tải (không kèm .txt):");
    if (name.empty())
    {
        ShowMessageOK(win, L"Hủy tải.");
        return;
    }
    ifstream f(name + ".txt");
    if (!f.is_open())
    {
        ShowMessageOK(win, L"Không mở được file!");
        return;
    }
    f >> _TURN;
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            f >> _TABLE[i][j].c;
    f.close();
    ShowMessageOK(win, L"Đã tải file " + (String)name + L".txt");
}

void UpdateWaves(float dt)
{
    for (auto &w : gWaves)
        w.elapsed += dt;
}

// ===== DRAW =====
void DrawGrid(RenderWindow &win)
{
    for (int i = 0; i <= BOARD_SIZE; i++)
    {
        Vertex line[] = {
            Vertex(Vector2f((float)MARGIN, (float)(MARGIN + i * CELL)), Color::Black),
            Vertex(Vector2f((float)(MARGIN + BOARD_SIZE * CELL), (float)(MARGIN + i * CELL)), Color::Black)};
        win.draw(line, 2, Lines);
    }
    for (int j = 0; j <= BOARD_SIZE; j++)
    {
        Vertex line[] = {
            Vertex(Vector2f((float)(MARGIN + j * CELL), (float)MARGIN), Color::Black),
            Vertex(Vector2f((float)(MARGIN + j * CELL), (float)(MARGIN + BOARD_SIZE * CELL)), Color::Black)};
        win.draw(line, 2, Lines);
    }
}

void DrawMarks(RenderWindow &win)
{
    for (int r = 0; r < BOARD_SIZE; r++)
    {
        for (int c = 0; c < BOARD_SIZE; c++)
        {
            int mark = _TABLE[r][c].c;
            if (mark == 0)
                continue;

            Text t((mark == 1) ? "O" : "X", gFont, CELL * 2 / 3);
            t.setFillColor(mark == 1 ? Color(180, 0, 0) : Color(0, 0, 180));
            Vector2f pos = cellTopLeft(r, c);
            float offsetY = (mark == 1 ? 0.02f : 0.05f);
            t.setPosition(pos.x + CELL * 0.15f + CONFIG_X_O_x, pos.y + CELL * offsetY + (mark == 1 ? CONFIG_O_y : CONFIG_X_y));
            win.draw(t);
        }
    }
}

void DrawHighlightGlow(RenderWindow &win)
{
    float outlineThick = 3.f;
    float dotThick = 3.f;
    Vector2f posOutline = cellTopLeft(selRow, selCol) + Vector2f(1.f, 1.f);
    float sizeOutline = CELL - 2.f;
    float size = sizeOutline;
    float perimeter = 4 * size;
    float time = glowEffClock.getElapsedTime().asSeconds();
    float speed = 100.f;

    // Viền mờ hơn dot glow
    RectangleShape outline(Vector2f(sizeOutline, sizeOutline));
    outline.setPosition(posOutline);
    outline.setFillColor(Color::Transparent);
    outline.setOutlineColor(Color(0, 180, 160, 150));
    outline.setOutlineThickness(outlineThick);
    win.draw(outline);

    // --- Dot glow chạy quanh ---
    auto getPos = [&](float t) -> Vector2f
    {
        if (t < size)
            return posOutline + Vector2f(t - dotThick / 2.f, -dotThick / 2.f);
        else if (t < 2 * size)
            return posOutline + Vector2f(size - dotThick / 2.f, t - size - dotThick / 2.f);
        else if (t < 3 * size)
            return posOutline + Vector2f(size - (t - 2 * size) - dotThick / 2.f, size - dotThick / 2.f);
        else
            return posOutline + Vector2f(-dotThick / 2.f, size - (t - 3 * size) - dotThick / 2.f);
    };

    float travel1 = fmod(time * speed, perimeter);
    float travel2 = fmod(travel1 + perimeter / 2.f, perimeter);
    float segment = 30.f;
    int steps = 20;

    for (int k = 0; k < 2; ++k)
    {
        float travel = (k == 0 ? travel1 : travel2);
        for (int i = 0; i < steps; ++i)
        {
            float ti = fmod(travel + segment * i / steps, perimeter);
            RectangleShape dot(Vector2f(dotThick, dotThick));
            dot.setFillColor(Color(0, 255, 180, 250));
            dot.setPosition(getPos(ti));
            win.draw(dot);
        }
    }

    // --- Glow quanh viền ---
    float glowTime = glowEffClock.getElapsedTime().asSeconds();
    float glowRadius = 2.f + 1.5f * sin(glowTime * 3.f);
    float glowAlpha = 120 + 80 * abs(sin(glowTime * 3.f));
    RectangleShape glow(Vector2f(CELL + glowRadius * 2.f, CELL + glowRadius * 2.f));
    glow.setPosition(posOutline.x - glowRadius, posOutline.y - glowRadius);
    glow.setFillColor(Color::Transparent);
    glow.setOutlineColor(Color(0, 255, 180, (Uint8)glowAlpha));
    glow.setOutlineThickness(1.5f);
    win.draw(glow);
}

void DrawEnergyWaves(RenderWindow &win)
{
    for (int i = (int)gWaves.size() - 1; i >= 0; --i)
    {
        Wave &w = gWaves[i];
        float t = w.elapsed / w.duration;
        if (t >= 1.f)
        {
            gWaves.erase(gWaves.begin() + i);
            continue;
        }

        float ease = 1.f - powf(1.f - t, 3.f);
        float radius = ease * w.maxRadius;

        CircleShape big(radius);
        big.setOrigin(radius, radius);
        big.setPosition(w.center);
        Uint8 alphaFill = (Uint8)(80 * (1.f - t));
        big.setFillColor(Color(w.color.r, w.color.g, w.color.b, alphaFill));
        win.draw(big);

        float band = CELL * 0.5f;
        CircleShape ring(radius);
        ring.setOrigin(radius, radius);
        ring.setPosition(w.center);
        Uint8 alphaRing = (Uint8)(200 * (1.f - t));
        ring.setFillColor(Color::Transparent);
        ring.setOutlineColor(Color(w.color.r, w.color.g, w.color.b, alphaRing));
        ring.setOutlineThickness(4.f * (1.f - t));
        win.draw(ring);

        float lower = radius - band, upper = radius;
        for (int r = 0; r < BOARD_SIZE; ++r)
            for (int c = 0; c < BOARD_SIZE; ++c)
            {
                Vector2f cellCenter = cellTopLeft(r, c) + Vector2f(CELL / 2.f, CELL / 2.f);
                float d = hypot(cellCenter.x - w.center.x, cellCenter.y - w.center.y);
                if (d >= lower && d <= upper)
                {
                    float local = 1.f - fabsf((d - lower) / band);
                    Uint8 a = (Uint8)(160 * local * (1.f - t));
                    RectangleShape rect(Vector2f(CELL, CELL));
                    rect.setPosition(cellTopLeft(r, c));
                    rect.setFillColor(Color(w.color.r, w.color.g, w.color.b, a));
                    win.draw(rect);
                }
            }
    }
}

void DrawUI(RenderWindow &win)
{
    Text info(L"WASD: Di chuyển    Enter: Đánh    L: Lưu    T: Tải    Esc: Thoát", gFont, 22);
    info.setFillColor(Color(50, 50, 200));
    info.setPosition(30.f, (float)(MARGIN + BOARD_SIZE * CELL + 10));

    Text infoGlow = info;
    infoGlow.setFillColor(Color(0, 200, 255, 80));
    for (int dx = -2; dx <= 2; dx++)
        for (int dy = -2; dy <= 2; dy++)
        {
            infoGlow.setPosition(info.getPosition() + Vector2f((float)dx, (float)dy));
            win.draw(infoGlow);
        }
    win.draw(info);

    Text turnText(String(L"Lượt: ") + (_TURN ? L"X" : L"O"), gFont, 22);
    turnText.setFillColor(Color(200, 50, 50));
    turnText.setPosition((float)(WIN_W - 100), (float)(MARGIN + BOARD_SIZE * CELL + 10));

    Text turnGlow = turnText;
    turnGlow.setFillColor(Color(255, 180, 180, 100));
    for (int dx = -2; dx <= 2; dx++)
        for (int dy = -2; dy <= 2; dy++)
        {
            turnGlow.setPosition(turnText.getPosition() + Vector2f((float)dx, (float)dy));
            win.draw(turnGlow);
        }
    win.draw(turnText);
}

void DrawAll(RenderWindow &win)
{
    win.clear(Color::White);

    DrawGrid(win);
    DrawMarks(win);
    DrawHighlightGlow(win);
    DrawEnergyWaves(win);
    DrawUI(win);

    win.display();
}

void SpawnWave(int row, int col, Color color, float duration = 0.8f, float radiusScale = 0.6f)
{
    Wave w;
    // tâm wave = center ô
    w.center = cellTopLeft(row, col) + Vector2f(CELL / 2.f, CELL / 2.f);
    w.color = color;
    w.elapsed = 0.f;
    w.duration = duration;

    // radius đủ phủ toàn bộ màn hình
    w.maxRadius = sqrtf((float)(WIN_W * WIN_W + WIN_H * WIN_H)) * radiusScale;

    gWaves.push_back(w);
}

void DrawTextGlow(RenderWindow &win, Text &text, Color glowColor, float glowRadius = 6.f)
{
    // Lấy bounding box của chữ
    FloatRect bounds = text.getGlobalBounds();

    // Glow: vẽ nhiều Rectangle nhỏ xung quanh chữ
    int steps = 8;
    for (int dx = -steps; dx <= steps; dx += 2)
    {
        for (int dy = -steps; dy <= steps; dy += 2)
        {
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist <= glowRadius)
            {
                Text glow = text;
                Uint8 alpha = (Uint8)(200 * (1.f - dist / glowRadius)); // mờ dần ra ngoài
                glow.setFillColor(Color(glowColor.r, glowColor.g, glowColor.b, alpha));
                glow.setPosition(text.getPosition() + Vector2f((float)dx, (float)dy));
                win.draw(glow);
            }
        }
    }

    // Vẽ chữ chính ở trên
    win.draw(text);
}

void DrawMenu(RenderWindow &win)
{
    win.clear(Color::White);

    Text title(L"Caro Game", gMenuFont, 100);
    title.setFillColor(Color::Black);
    title.setPosition(WIN_W / 2 - 150, WIN_H / 4);
    DrawTextGlow(win, title, Color::Yellow, 10.f); // glow vàng

    Text startText(L"Start Game", gMenuFont, 32);
    Text loadText(L"Load Game", gMenuFont, 32);
    Text settingsText(L"Settings", gMenuFont, 32);
    Text exitText(L"Exit", gMenuFont, 32);
    startText.setFillColor(Color::Black);
    loadText.setFillColor(Color::Black);
    settingsText.setFillColor(Color::Black);
    exitText.setFillColor(Color::Black);
    startText.setPosition(WIN_W / 2 - 80, WIN_H / 2);
    loadText.setPosition(WIN_W / 2 - 80, WIN_H / 2 + 50);
    settingsText.setPosition(WIN_W / 2 - 80, WIN_H / 2 + 100);
    exitText.setPosition(WIN_W / 2 - 80, WIN_H / 2 + 150);

    // menu glow động
    float glowTime = menuGlowClock.getElapsedTime().asSeconds();
    float radius = 4.f + 3.f * abs(sin(glowTime * 3.f));

    if (menuSelection == 0)
    {
        startText.setScale(1.1f, 1.1f); // phóng to
        DrawTextGlow(win, startText, Color::Green, radius);
    }
    else
        win.draw(startText);

    if (menuSelection == 1)
    {
        loadText.setScale(1.1f, 1.1f); // phóng to
        DrawTextGlow(win, loadText, Color::Cyan, radius);
    }
    else
        win.draw(loadText);

    if (menuSelection == 2)
    {
        settingsText.setScale(1.1f, 1.1f); // phóng to
        DrawTextGlow(win, settingsText, Color::Cyan, radius);
    }
    else
        win.draw(settingsText);

    if (menuSelection == 3)
    {
        exitText.setScale(1.1f, 1.1f); // phóng to
        DrawTextGlow(win, exitText, Color::Red, radius);
    }
    else
        win.draw(exitText);

    win.display();
}

void HandleMenuInput(Event &ev)
{
    if (ev.type == Event::KeyPressed)
    {
        if (ev.key.code == Keyboard::Up || ev.key.code == Keyboard::W)
            menuSelection = abs((menSel-- - 1)) % 4;
        else if (ev.key.code == Keyboard::Down || ev.key.code == Keyboard::S)
            menuSelection = abs((menSel++ + 1)) % 4;
        else if (ev.key.code == Keyboard::Enter)
        {
            if (menuSelection == 0)
                gState = PLAYING; // bắt đầu game
            else if (menuSelection == 1)
                gState = LOADGAME; // load file save đã có
            else if (menuSelection == 2)
                gState = SETTINGS; // cài đặt (chưa làm)
            else
                exit(0); // thoát game
        }
        if (ev.key.code == Keyboard::Escape)
        {
            exit(0); // thoát game
        }
    }
}

// ===== MAIN =====
int main()
{
    // load font
    if (!gFont.loadFromFile("fonts/patrickHand.ttf"))
    {
        cerr << "Khong tim thay font 'patrickHand.ttf'. Vui long dat file patrickHand.ttf trong thu muc chay.\n";
        // ta vẫn thử dùng SFML default? (không có) -> thoát
        return -1;
    }
    if (!gMenuFont.loadFromFile("fonts/feotgp.ttf"))
    {
        cerr << "Khong tim thay font 'feotgp.ttf'. Vui long dat file feotgp.ttf trong thu muc chay.\n";
        return -1;
    }

    ResetData();

    Clock frameClock;
    RenderWindow window(VideoMode((unsigned)WIN_W, (unsigned)WIN_H), "Caro-beta", Style::Titlebar | Style::Close);
    window.setFramerateLimit(60);

    DrawAll(window);

    while (window.isOpen())
    {
        float dt = frameClock.restart().asSeconds();
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                window.close();
                break;
            }
            if (gState == MENU)
                HandleMenuInput(event);
            /*else if(gState == LOADGAME)
            {

            }*/
            else
            {
                if (event.type == Event::KeyPressed)
                {
                    if (event.key.code == Keyboard::Escape)
                    {
                        int yn = ShowMessageYesNo(window, L"Bạn có muốn thoát game?");
                        if (!yn)
                            window.close();
                        else
                        {
                        }
                        DrawAll(window);
                    }
                    else if (event.key.code == Keyboard::A)
                    {
                        if (selCol > 0)
                            selCol--;
                        DrawAll(window);
                    }
                    else if (event.key.code == Keyboard::D)
                    {
                        if (selCol < BOARD_SIZE - 1)
                            selCol++;
                        DrawAll(window);
                    }
                    else if (event.key.code == Keyboard::W)
                    {
                        if (selRow > 0)
                            selRow--;
                        DrawAll(window);
                    }
                    else if (event.key.code == Keyboard::S)
                    {
                        if (selRow < BOARD_SIZE - 1)
                            selRow++;
                        DrawAll(window);
                    }
                    else if (event.key.code == Keyboard::L)
                    {
                        SaveGame(window);
                        DrawAll(window);
                    }
                    else if (event.key.code == Keyboard::T)
                    {
                        LoadGame(window);
                        DrawAll(window);
                    }
                    else if (event.key.code == Keyboard::Enter)
                    {
                        int check = CheckBoard(selRow, selCol);
                        if (check == -1)
                        {
                            // placed X
                        }
                        else if (check == 1)
                        {
                            // placed O
                        }
                        else
                        {
                            ShowMessageOK(window, L"Ô đã được đặt ở đây (chỉ được đặt ô trống)");
                        }
                        int state = TestBoard();
                        if (state != 2)
                        {
                            String msg;
                            if (state == -1)
                                msg = L"Người chơi X đã thắng!";
                            else if (state == 1)
                                msg = L"Người chơi O đã thắng!";
                            else
                                msg = L"Hai bên hoà nhau!";
                            int cont = ShowMessageYesNo(window, msg + L"  Nhấn Y để chơi tiếp?");
                            if (!cont)
                            {
                                ShowMessageOK(window, L"Cảm ơn đã chơi!");
                                window.close();
                                break;
                            }
                            else
                            {
                                ResetData();
                                DrawAll(window);
                                continue;
                            }
                        }
                        // nếu hợp lệ thì đổi lượt
                        if (check != 0)
                        {
                            SpawnWave(selRow, selCol, check == -1 ? Color(0, 0, 255) : Color(255, 0, 0)); // sóng
                            _TURN = !_TURN;
                        }
                    }
                }
            }
        }
        if (gState == MENU)
        {
            float dt = frameClock.restart().asSeconds();
            DrawMenu(window);
        }
        else
        {
            UpdateWaves(dt);
            DrawAll(window);
        }
    }
    return 0;
}