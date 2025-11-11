#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <fstream>
#include <iostream>
#include <string> 
#include <vector>
#include <cctype>  

using namespace std;
using namespace sf;


#define BOARD_SIZE 12

struct _POINT { int x, y, c; }; // x,y là chỉ số hàng, c=0: trống, -1:X, 1:O

_POINT  _TABLE[BOARD_SIZE][BOARD_SIZE];
bool _TURN = true;       // true: X, false: O
int selRow = 0, selCol = 0; // chọn ô hiện tại (hàng, cột)

// GUI parameters
const int CELL = 50;       // kích thước ô (pixels)
const int MARGIN = 30;     // lề trái / trên
int WIN_W = MARGIN * 2 + CELL * BOARD_SIZE;
int WIN_H = MARGIN * 2 + CELL * BOARD_SIZE + 120; // extra dưới để in thông báo

Font gFont;

// ===== MODEL =====
void ResetData() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            _TABLE[i][j].x = i; 
            _TABLE[i][j].y = j; 
            _TABLE[i][j].c = 0;
        }
    }
    _TURN = true;
    selRow = 0; selCol = 0;
}

bool isFull() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (_TABLE[i][j].c == 0) return false;
        }
    }
    return true;
}

int TestBoard() {
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++) {
            int c = _TABLE[i][j].c;
            if (c == 0) continue;
            int dx[4] = { 1, 0, 1, 1 };
            int dy[4] = { 0, 1, 1, -1 };
            for (int k = 0; k < 4; k++) {
                int cnt = 0;
                for (int t = 0; t < 5; t++) {
                    int x = i + dx[k] * t;
                    int y = j + dy[k] * t;
                    if (x < 0 || y < 0 || x >= BOARD_SIZE || y >= BOARD_SIZE) break;
                    if (_TABLE[x][y].c == c) cnt++;
                    else break;
                }
                if (cnt == 5) return c;
            }
        }
    if (isFull()) return 0; // hòa
	return 2; // chưa ai thắng
}

// đặt vị trí nếu trống
int CheckBoard(int row, int col) {
    if (row < 0 || col < 0 || row >= BOARD_SIZE || col >= BOARD_SIZE) return 0;
    if (_TABLE[row][col].c == 0) {
        _TABLE[row][col].c = (_TURN ? -1 : 1);
        return _TABLE[row][col].c;
    }
    return 0;
}

// ===== UI helpers =====
Vector2f cellTopLeft(int row, int col) {
    return Vector2f((float)(MARGIN + col * CELL), (float)(MARGIN + row * CELL));
}

string InputDialog(RenderWindow& win, const String& prompt) {
    // Hiển thị hộp nhập tên file (chuỗi), trả về tên (không kèm .txt)
    string input;
    bool done = false;
    Event event;
    while (!done) {
        while (win.pollEvent(event)) {
            if (event.type == Event::Closed) {
                win.close();
                return "";
            }
            else if (event.type == Event::TextEntered) {
                if (event.text.unicode == 13) done = true;
                else if (event.text.unicode == 8) { // backspace
                    if (!input.empty()) input.pop_back();
                }
                else if (event.text.unicode < 128) {
                    char ch = (char)event.text.unicode;
                    // chấp nhận ký tự filename đơn giản
                    if (isprint(ch)) input.push_back(ch);
                }
            }
            else if (event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Enter) done = true;
                else if (event.key.code == Keyboard::Escape) { 
                    input = ""; 
                    done = true;
                }
            }
        }

        // vẽ dialog
        win.clear(Color::White);

        // bảng
        RectangleShape bg(Vector2f((float)WIN_W, (float)WIN_H));
        bg.setFillColor(Color::White);
        win.draw(bg);

        // draw grid lines
        for (int i = 0; i <= BOARD_SIZE; i++) {
            Vertex line[] = {
                Vertex(Vector2f(MARGIN, MARGIN + i * CELL), Color::Black),
                Vertex(Vector2f(MARGIN + BOARD_SIZE * CELL, MARGIN + i * CELL), Color::Black)
            };
            win.draw(line, 2, Lines);
        }

        for (int j = 0; j <= BOARD_SIZE; j++) {
            Vertex line[] = {
                Vertex(Vector2f(MARGIN + j * CELL, MARGIN), Color::Black),
                Vertex(Vector2f(MARGIN + j * CELL, MARGIN + BOARD_SIZE * CELL), Color::Black)
            };
            win.draw(line, 2, Lines);
        }

        // draw marks
        for (int r = 0; r < BOARD_SIZE; r++) {
            for (int c = 0; c < BOARD_SIZE; c++) {
                if (_TABLE[r][c].c == -1) {
                    // X color blue
                    Text t("X", gFont, CELL * 2 / 3);
                    t.setFillColor(Color(0, 0, 180));
                    t.setPosition(cellTopLeft(r, c).x + CELL * 0.3f, cellTopLeft(r, c).y + CELL * 0.05f);
                    win.draw(t);
                }
                else if (_TABLE[r][c].c == 1) {
                    Text t("O", gFont, CELL * 2 / 3);
                    t.setFillColor(Color(180, 0, 0));
                    t.setPosition(cellTopLeft(r, c).x + CELL * 0.3f, cellTopLeft(r, c).y + CELL * 0.02f);
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

        Text typed(input + "_", gFont, 20);
        typed.setFillColor(Color::Black);
        typed.setPosition(dialog.getPosition() + Vector2f(12.f, 40.f));
        win.draw(typed);

        Text hint(L"Enter = OK, Esc = Cancel, Backspace để xóa", gFont, 14);
        hint.setFillColor(Color::Black);
        hint.setPosition(dialog.getPosition() + Vector2f(12.f, 78.f));
        win.draw(hint);

        win.display();
    }
    return input;
}

int ShowMessageYesNo(RenderWindow& win, const String& msg) {
    // Hiện hộp thoại với nội dung msg, chờ Y (tiếp tục) hoặc phím khác (thoát),
    // trả về 1 nếu Y, 0 nếu khác.
    bool waiting = true;
    Event event;
    while (waiting && win.isOpen()) {
        while (win.pollEvent(event)) {
            if (event.type == Event::Closed) { 
                win.close(); 
                return 0; 
            }
            if (event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Y) return 1;
                else return 0;
            }
        }
        // vẽ modal
        win.clear(Color::White);

        // draw board snapshot
        // grid lines
        for (int i = 0; i <= BOARD_SIZE; i++) {
            Vertex line[] = {
                Vertex(Vector2f((float)(MARGIN), (float)(MARGIN + i * CELL)), Color::Black),
                Vertex(Vector2f((float)(MARGIN + BOARD_SIZE * CELL), (float)(MARGIN + i * CELL)), Color::Black)
			};
			win.draw(line, 2, Lines);
        }
        for (int j = 0; j <= BOARD_SIZE; j++) {
            Vertex line[] = {
                Vertex(Vector2f((float)(MARGIN + j * CELL), (float)(MARGIN)), Color::Black),
                Vertex(Vector2f((float)(MARGIN + j * CELL), (float)(MARGIN + BOARD_SIZE * CELL)), Color::Black)
            };
			win.draw(line, 2, Lines);
        }
        // marks
        for (int r = 0; r < BOARD_SIZE; r++) {
            for (int c = 0; c < BOARD_SIZE; c++) {
                if (_TABLE[r][c].c == -1) {
                    Text t("X", gFont, CELL * 2 / 3);
                    t.setFillColor(Color(0, 0, 180));
                    t.setPosition(cellTopLeft(r, c).x + CELL * 0.15f, cellTopLeft(r, c).y + CELL * 0.05f);
                    win.draw(t);
                }
                else if (_TABLE[r][c].c == 1) {
                    Text t("O", gFont, CELL * 2 / 3);
                    t.setFillColor(Color(180, 0, 0));
                    t.setPosition(cellTopLeft(r, c).x + CELL * 0.15f, cellTopLeft(r, c).y + CELL * 0.02f);
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

void ShowMessageOK(RenderWindow& win, const String& msg) {
    // Hiển thị hộp thoại đơn, chờ 1 phím bất kỳ hoặc click
    bool waiting = true;
    Event event;
    while (waiting && win.isOpen()) {
        while (win.pollEvent(event)) {
            if (event.type == Event::Closed) { 
                win.close(); 
                return; 
            }
            if (event.type == Event::KeyPressed || event.type == Event::MouseButtonPressed) return;
        }
        // vẽ modal (giống trên)
        win.clear(Color::White);
        // draw grid snapshot
        for (int i = 0; i <= BOARD_SIZE; i++) {
            Vertex line[] = {
                Vertex(Vector2f((float)(MARGIN), (float)(MARGIN + i * CELL)), Color::Black),
                Vertex(Vector2f((float)(MARGIN + BOARD_SIZE * CELL), (float)(MARGIN + i * CELL)), Color::Black)
			};
			win.draw(line, 2, Lines);
        }
        for (int j = 0; j <= BOARD_SIZE; j++) {
            Vertex line[] = {
                Vertex(Vector2f((float)(MARGIN + j * CELL), (float)(MARGIN)), Color::Black),
                Vertex(Vector2f((float)(MARGIN + j * CELL), (float)(MARGIN + BOARD_SIZE * CELL)), Color::Black)
			};
			win.draw(line, 2, Lines);
        }
        // marks
        for (int r = 0; r < BOARD_SIZE; r++) {
            for (int c = 0; c < BOARD_SIZE; c++) {
                if (_TABLE[r][c].c == -1) {
                    Text t("X", gFont, CELL * 2 / 3);
                    t.setFillColor(Color(0, 0, 180));
                    t.setPosition(cellTopLeft(r, c).x + CELL * 0.15f, cellTopLeft(r, c).y + CELL * 0.05f);
                    win.draw(t);
                }
                else if (_TABLE[r][c].c == 1) {
                    Text t("O", gFont, CELL * 2 / 3);
                    t.setFillColor(Color(180, 0, 0));
                    t.setPosition(cellTopLeft(r, c).x + CELL * 0.15f, cellTopLeft(r, c).y + CELL * 0.02f);
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
void SaveGame(RenderWindow& win) {
    string name = InputDialog(win, L"Nhập tên file để lưu (không kèm .txt):");
    if (name.empty()) { 
        ShowMessageOK(win, L"Hủy lưu."); 
        return; 
    }
    ofstream f(name + ".txt");
    if (!f.is_open()) { 
        ShowMessageOK(win, L"Không thể mở file để ghi!"); 
        return; 
    }
    f << _TURN << "\n";
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) f << _TABLE[i][j].c << " ";
        f << "\n";
    }
    f.close();
    ShowMessageOK(win, L"Đã lưu" + (String)name + L".txt");
}

void LoadGame(RenderWindow& win) {
    string name = InputDialog(win, L"Nhập tên file để tải (không kèm .txt):");
    if (name.empty()) { 
        ShowMessageOK(win, L"Hủy tải."); 
        return; 
    }
    ifstream f(name + ".txt");
    if (!f.is_open()) { 
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

// ===== DRAW =====
void DrawAll(RenderWindow& win) {
    win.clear(Color::White);

    // grid lines (nền trắng)
    // vertical & horizontal lines
    for (int i = 0; i <= BOARD_SIZE; i++) {
        // horizontal
        Vertex line[] = {
            Vertex(Vector2f((float)MARGIN, (float)(MARGIN + i * CELL)), Color::Black),
            Vertex(Vector2f((float)(MARGIN + BOARD_SIZE * CELL), (float)(MARGIN + i * CELL)), Color::Black)
        };
        win.draw(line, 2, Lines);
    }
    for (int j = 0; j <= BOARD_SIZE; j++) {
        Vertex line[] = {
            Vertex(Vector2f((float)(MARGIN + j * CELL), (float)MARGIN), Color::Black),
            Vertex(Vector2f((float)(MARGIN + j * CELL), (float)(MARGIN + BOARD_SIZE * CELL)), Color::Black)
        };
        win.draw(line, 2, Lines);
    }

    // draw marks
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (_TABLE[r][c].c == -1) {
                Text t("X", gFont, CELL * 2 / 3);
                t.setFillColor(Color(0, 0, 180)); // blue
                auto pos = cellTopLeft(r, c);
                t.setPosition(pos.x + CELL * 0.15f, pos.y + CELL * 0.05f);
                win.draw(t);
            }
            else if (_TABLE[r][c].c == 1) {
                Text t("O", gFont, CELL * 2 / 3);
                t.setFillColor(Color(180, 0, 0)); // red
                auto pos = cellTopLeft(r, c);
                t.setPosition(pos.x + CELL * 0.15f, pos.y + CELL * 0.02f);
                win.draw(t);
            }
        }
    }

    // highlight selected cell
    RectangleShape highlight(Vector2f((float)CELL - 2.f, (float)CELL - 2.f));
    highlight.setPosition(cellTopLeft(selRow, selCol) + Vector2f(1.f, 1.f));
    highlight.setFillColor(Color(0, 0, 0, 0));
    highlight.setOutlineColor(Color(50, 150, 50));
    highlight.setOutlineThickness(3.f);
    win.draw(highlight);

    // footer text (controls)
    Text info(L"WASD: Di chuyển    Enter: Đánh    L: Lưu    T: Tải    Esc: Thoát", gFont, 18);
    info.setFillColor(Color::Black);
    info.setPosition(12.f, (float)(MARGIN + BOARD_SIZE * CELL + 10));
    win.draw(info);

    // whose turn
    Text turnText(String(L"Lượt: ") + (_TURN ? L"X" : L"O"), gFont, 18);
    turnText.setFillColor(Color::Black);
    turnText.setPosition((float)(WIN_W - 140), (float)(MARGIN + BOARD_SIZE * CELL + 10));
    win.draw(turnText);

    win.display();
}

// ===== MAIN =====
int main() {
    // load font
    if (!gFont.loadFromFile("patrickHand.ttf")) {
        cerr << "Khong tim thay font 'patrickHand.ttf'. Vui long dat file patrickHand.ttf trong thu muc chay.\n";
        // ta vẫn thử dùng SFML default? (không có) -> thoát
        return -1;
    }

    ResetData();

    RenderWindow window(VideoMode((unsigned)WIN_W, (unsigned)WIN_H), "Caro-beta");
    window.setFramerateLimit(60);

    DrawAll(window);

    while (window.isOpen()) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) { window.close(); break; }
            if (event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Escape) {
                    // show confirm exit
                    int yn = ShowMessageYesNo(window, L"Bạn có muốn thoát game?");
                    if (!yn) window.close();
                    else { /* return to game */ }
                    DrawAll(window);
                }
                else if (event.key.code == Keyboard::A) {
                    if (selCol > 0) selCol--;
                    DrawAll(window);
                }
                else if (event.key.code == Keyboard::D) {
                    if (selCol < BOARD_SIZE - 1) selCol++;
                    DrawAll(window);
                }
                else if (event.key.code == Keyboard::W) {
                    if (selRow > 0) selRow--;
                    DrawAll(window);
                }
                else if (event.key.code == Keyboard::S) {
                    if (selRow < BOARD_SIZE - 1) selRow++;
                    DrawAll(window);
                }
                else if (event.key.code == Keyboard::L) {
                    SaveGame(window);
                    DrawAll(window);
                }
                else if (event.key.code == Keyboard::T) {
                    LoadGame(window);
                    DrawAll(window);
                }
                else if (event.key.code == Keyboard::Enter) {
                    int check = CheckBoard(selRow, selCol);
                    if (check == -1) {
                        // placed X
                    }
                    else if (check == 1) {
                        // placed O
                    }
                    else {
                        // invalid place -> show small message
                        ShowMessageOK(window, L"Ô đã được đặt ở đây (chỉ được đặt ô trống)");
                    }
                    int state = TestBoard();
                    if (state != 2) {
                        // someone won or draw
                        String msg;
                        if (state == -1) msg = L"Người chơi X đã thắng!";
                        else if (state == 1) msg = L"Người chơi O đã thắng!";
                        else msg = L"Hai ben hoa nhau!";
                        int cont = ShowMessageYesNo(window, msg + L"    Nhấn Y để chơi tiếp?");
                        if (!cont) {
                            ShowMessageOK(window, L"Cảm ơn đã chơi!");
                            window.close();
                            break;
                        }
                        else {
                            ResetData();
                            DrawAll(window);
                            continue;
                        }
                    }
                    // nếu hợp lệ thì đổi lượt
                    if (check != 0) _TURN = !_TURN;
                    DrawAll(window);
                }
            }
        }
        // no continuous update needed; draw already on interactions
    }

    return 0;
}
