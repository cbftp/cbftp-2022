#include "loginscreen.h"

#include <cstdlib>

#include "../termint.h"
#include "../ui.h"
#include "../chardraw.h"

#ifndef BOXTAG
#define BOXTAG ""
#endif

#ifndef VERSION
#define VERSION "unknown"
#endif

#ifndef BUILDTIME
#define BUILDTIME "unknown"
#endif

LoginScreen::LoginScreen(Ui * ui) {
  this->ui = ui;
}

void LoginScreen::initialize(unsigned int row, unsigned int col) {
  passfield = TextInputField(25, 32, true);
  attempt = false;
  drawword = BOXTAG;
  drawx = 0;
  drawy = 0;
  srand(time(NULL));
  init(row, col);
}

void LoginScreen::redraw() {
  ui->erase();
  randomizeDrawLocation();
  background.clear();
  for (unsigned int i = 0; i < row; i++) {
    background.push_back(std::vector<int>());
    background[i].resize(col);
  }
  pass_row = row-2;
  pass_col = col-27;
  ui->showCursor();
  std::string svnstring = " cbftp version tag: " + std::string(VERSION) + " ";
  std::string compilestring = " Compiled: " + std::string(BUILDTIME) + " ";
  int boxchar = 0;
  for(unsigned int i = 1; i < row; i++) {
    for(unsigned int j = 0; j < col; j++) {
      if(i == 1) boxchar = (i+j)%2==0 ? BOX_HLINE_BOT : BOX_HLINE;
      else if (i == row-1) {
        if (j < col-29) boxchar = (i+j)%2==0 ? BOX_HLINE : BOX_HLINE_TOP;
        else if (j == col-29) boxchar = BOX_CORNER_BR;
        else continue;
      }
      else if ((i == row-2 || i == row-3) && j >= col-29) {
        if (j == col-29) boxchar = (i+j)%2==0 ? BOX_VLINE : BOX_VLINE_L;
        else continue;
      }
      else if (i == row-4 && j >= col-29) {
        if (j == col-29) boxchar = (i+j)%2==0 ? BOX_CORNER_TL : BOX_CROSS;
        else boxchar = (i+j)%2==0 ? BOX_HLINE : BOX_HLINE_TOP;
      }
      else boxchar = (i+j)%2==0 ? BOX_CORNER_TL : BOX_CORNER_BR;
      if (boxchar) {
        ui->printChar(i, j, boxchar);
        background[i][j] = boxchar;
      }
    }
  }
  ui->printStr(0, 3, svnstring);
  ui->printStr(0, col - compilestring.length() - 3, compilestring);
  update();

}

void LoginScreen::update() {
  std::string passtext = "AES passphrase required:";
  if (attempt) {
    passtext = "Invalid key, try again: ";
    ui->showCursor();
  }
  int currdrawx = drawx;
  ui->printStr(pass_row-1, pass_col, passtext);
  ui->printStr(pass_row, pass_col, passfield.getVisualText());
  ui->moveCursor(pass_row, pass_col + passfield.getVisualCursorPosition());
  for (unsigned int drawchar = 0; drawchar < drawword.length(); drawchar++) {
    bool show = passfield.getText().length() > drawchar &&
        passfield.getText().length() - drawchar < drawword.length() + 1;
    for (int i = 0; i < CHARDRAW_SIZE; i++) {
      std::string draw = CharDraw::getCharLine(drawword[drawchar], i);
      for (unsigned int j = 0; j < draw.length(); j++) {
        int bgchar = background[drawy + i][currdrawx + j];
        int c = show ? CharDraw::getMixedChar(bgchar, draw[j]) : bgchar;
        if (c) {
          ui->printChar(drawy + i, currdrawx + j, c);
        }
      }
    }
    currdrawx = currdrawx + CHARDRAW_SIZE;
  }
  if (passfield.getText().length() == drawword.length() * 2 || !passfield.getText().length()) {
    randomizeDrawLocation();
  }
}

bool LoginScreen::keyPressed(unsigned int ch) {
  if (ch >= 32 && ch <= 126) {
    passfield.addchar(ch);
  }
  else {
    switch(ch) {
      case 8:
      case 127:
      case KEY_BACKSPACE:
        passfield.erase();
        break;
      case KEY_HOME:
        passfield.moveCursorHome();
        break;
      case KEY_END:
        passfield.moveCursorEnd();
        break;
      case KEY_LEFT:
        passfield.moveCursorLeft();
        break;
      case KEY_RIGHT:
        passfield.moveCursorRight();
        break;
      case KEY_DC:
        if (passfield.moveCursorRight()) {
          passfield.erase();
        }
        break;
      case KEY_ENTER:
      case 10:
      case 13:
        ui->hideCursor();
        attempt = true;
        ui->key(passfield.getText());
        passfield.clear();
        break;
    }
  }
  ui->update();
  return true;
}

void LoginScreen::randomizeDrawLocation() {
  int ymin = 2;
  int ymax = row - 2 - CHARDRAW_SIZE - ymin;
  int xmin = 1;
  int xmax = col - 2 - CHARDRAW_SIZE * drawword.length() - xmin;
  drawy = rand() % ymax + ymin;
  drawx = rand() % xmax + xmin;
}
