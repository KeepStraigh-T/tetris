#include <locale.h>
#include <ncurses.h>
// #include <string.h>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <random>

std::vector<std::string> tetromino(7);
const int nTetrominoSize{ 4 };

int nFieldWidth{ 14 };
int nFieldHeight{ 18 };
unsigned char* pField{ nullptr };

// returs index in rotated tetramino grid
int Rotate(int py, int px, int r)
{
	switch (r % 4)
	{
		case 0:
			return py * nTetrominoSize + px; // 0 degrees
		case 1:
			return 12 + py - (px * nTetrominoSize); // 90 degrees
		case 2:
			return 15 - (py * nTetrominoSize) - px; // 180 degrees
		case 3:
			return 3 - py + (px * nTetrominoSize); // 270 degrees
	}

	return 0;
}

// collision detection
bool DoesPieceFit(int nTetromino, int nRotation, int posY, int posX)
{
	for (int py{ 0 }; py < nTetrominoSize; ++py)
		for (int px{ 0 }; px < nTetrominoSize; ++px)
		{
			// Get index into piece
			int pi{ Rotate(py, px, nRotation) };

			int fi{ (posY + py) * nFieldWidth + (posX + px) };

			if (posY + py >= 0 && posY + py < nFieldHeight)
				if (posX + px >= 0 && posX + px < nFieldWidth)
					if (tetromino[nTetromino][pi] == 'X' && pField[fi] != 0)
						return false;
		}

	return true;
}

int MainMenu(int nScore, bool bGameOver = false)
{
	WINDOW* wMainMenu{ newwin(10, 50, (LINES - 10) / 2, (COLS - 50) / 2) };
	keypad(wMainMenu, true);
	box(wMainMenu, 0, 0);

	if (bGameOver)
	{
		mvwprintw(wMainMenu, 3, 20, "GAME OVER!");
		mvwprintw(wMainMenu, 4, 17, "Your score: %d", nScore);
	}
	wrefresh(wMainMenu);

	std::vector<std::string> choices = { " Start again ", " Quit " };

	int highlighted{ 0 };
	int choice{ 0 };
	while (true)
	{
		for (size_t i{ 0 }; i < choices.size(); ++i)
		{
			if (i == highlighted)
				wattron(wMainMenu, A_REVERSE);
			mvwaddstr(wMainMenu, 8, 2 + i * 40, choices[i].c_str());
			wattroff(wMainMenu, A_REVERSE);
		}

		choice = wgetch(wMainMenu);

		switch (choice)
		{
			case KEY_LEFT:
				if (highlighted > 0)
					--highlighted;
				break;
			case KEY_RIGHT:
				if (highlighted < choices.size() - 1)
					++highlighted;
				break;
			default:
				break;
		}

		// dont process ESC key if game is over
		if (choice == '\n' || (choice == 27 && !bGameOver))
			break;
	}

	wborder(wMainMenu, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
	werase(wMainMenu);
	wrefresh(wMainMenu);
	delwin(wMainMenu);

	if (choice == '\n')
		return highlighted;

	return -1;
}

void CreateAssets()
{
	tetromino[0].append("..X.");
	tetromino[0].append("..X.");
	tetromino[0].append("..X.");
	tetromino[0].append("..X.");

	tetromino[1].append("..X.");
	tetromino[1].append(".XX.");
	tetromino[1].append(".X..");
	tetromino[1].append("....");

	tetromino[2].append(".X..");
	tetromino[2].append(".XX.");
	tetromino[2].append("..X.");
	tetromino[2].append("....");

	tetromino[3].append("..X.");
	tetromino[3].append(".XX.");
	tetromino[3].append("..X.");
	tetromino[3].append("....");

	tetromino[4].append(".XX.");
	tetromino[4].append("..X.");
	tetromino[4].append("..X.");
	tetromino[4].append("....");

	tetromino[5].append(".X..");
	tetromino[5].append(".X..");
	tetromino[5].append(".XX.");
	tetromino[5].append("....");

	tetromino[6].append("....");
	tetromino[6].append(".XX.");
	tetromino[6].append(".XX.");
	tetromino[6].append("....");
}

int main()
{
	// init ncurses
	setlocale(LC_ALL, "");
	initscr();
	keypad(stdscr, true);
	raw();
	// nodelay(stdscr, true);
	timeout(0);
	noecho();
	curs_set(0);	 // disable cursor
	set_escdelay(0); // disable delay for escape sequences

	std::random_device rd;
	std::mt19937 gen{ rd() };
	std::uniform_int_distribution<int> dist(0, 1000);

	// Tetromino assets
	CreateAssets();

	// init Field
	pField = new unsigned char[nFieldWidth * nFieldHeight];
	for (int y{ 0 }; y < nFieldHeight; ++y)
		for (int x{ 0 }; x < nFieldWidth; ++x)
			pField[y * nFieldWidth + x] = (x == 0 || x == nFieldWidth - 1 || y == nFieldHeight - 1)
							  ? 9  // 9 - boarder
							  : 0; // 0 - inside area

	// Game logic stuff
	bool bGameOver{ false };

	int nCurrentPiece{ dist(gen) % 7 };
	int nCurrentRotation{ dist(gen) % 4 };
	int nCurrentY{ 0 };
	int nCurrentX{ (nFieldWidth - nTetrominoSize) / 2 };

	bool bRotateHold{ false };

	int nSpeed{ 60 }; // difficulty of the game
	int nSpeedCounter{ 0 };
	bool bForceDown{ false };
	int nPieceCount{ 0 };
	int nScores{ 0 };
	int lastTermHeight{ LINES }, lastTermWidth{ COLS };

	auto initGame = [&]()
	{
		bGameOver	 = false;
		nCurrentPiece	 = dist(gen) % 7;
		nCurrentRotation = dist(gen) % 4;
		nCurrentY	 = 0;
		nCurrentX	 = (nFieldWidth - nTetrominoSize) / 2;

		bRotateHold = false;

		nSpeed	       = 60;
		nSpeedCounter  = 0;
		bForceDown     = false;
		nPieceCount    = 0;
		nScores	       = 0;
		lastTermHeight = LINES;
		lastTermWidth  = COLS;

		// clear the field inside area
		for (int py{ 0 }; py < nFieldHeight - 1; ++py)
			for (int px{ 1 }; px < nFieldWidth - 1; ++px)
				pField[py * nFieldWidth + px] = 0;
		clear();
	};
	std::vector<int> vLine;

	// GAME LOOP
	while (!bGameOver)
	{
		// GAME TIMING ============================================================================

		std::this_thread::sleep_for(std::chrono::milliseconds(16));
		++nSpeedCounter;
		bForceDown = (nSpeedCounter == nSpeed);

		// INPUT ==================================================================================

		int c{ getch() };

		nCurrentX -=
		    (c == KEY_LEFT && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentY, nCurrentX - 1)) ? 1 : 0;
		nCurrentX +=
		    (c == KEY_RIGHT && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentY, nCurrentX + 1)) ? 1 : 0;
		nCurrentY +=
		    (c == KEY_DOWN && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentY + 1, nCurrentX)) ? 1 : 0;

		if (c == KEY_UP)
		{
			nCurrentRotation +=
			    (!bRotateHold && DoesPieceFit(nCurrentPiece, nCurrentRotation + 1, nCurrentY, nCurrentX))
				? 1
				: 0;
			bRotateHold = true;
		}
		else
			bRotateHold = false;

		if (c == 27)
		{
			int nSelectedOption{ MainMenu(nScores) };

			// Restart game
			if (nSelectedOption == 0)
			{
				initGame();
				continue;
			}
			// Quit
			if (nSelectedOption == 1)
				break;
		}

		// GAME LOGIC =============================================================================

		if (bForceDown) // Force current piece down
		{
			// Can current piece move down?
			if (DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentY + 1, nCurrentX))
				++nCurrentY; // it can, move down
			else
			{
				// write current piece into field
				for (int py{ 0 }; py < nTetrominoSize; ++py)
					for (int px{ 0 }; px < nTetrominoSize; ++px)
						if (tetromino[nCurrentPiece][Rotate(py, px, nCurrentRotation)] == 'X')
							pField[(nCurrentY + py) * nFieldWidth + (nCurrentX + px)] =
							    nCurrentPiece + 1;

				// Check have we got any lines
				for (int py{ 0 }; py < nTetrominoSize; ++py)
					if (nCurrentY + py < nFieldHeight - 1)
					{
						bool bLine{ true };
						for (int px{ 1 }; px < nFieldWidth - 1; ++px)
							if (!(bLine =
								  (pField[(nCurrentY + py) * nFieldWidth + px]) != 0))
								break;

						if (bLine)
						{
							for (int px{ 1 }; px < nFieldWidth - 1; ++px)
								pField[(nCurrentY + py) * nFieldWidth + px] =
								    8; // set all elements in row to "="
							vLine.push_back(nCurrentY + py);
						}
					}

				// Choose next piece
				nCurrentPiece	 = dist(gen) % 7;
				nCurrentRotation = dist(gen) % 4;
				nCurrentY	 = 0;
				nCurrentX	 = (nFieldWidth - nTetrominoSize) / 2;

				// if piece does not fit in entire field - game over
				bGameOver = !DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentY, nCurrentX);

				if (!bGameOver)
				{
					nScores += 25;
					if (!vLine.empty())
						nScores += (1 << vLine.size()) * 100;
				}

				// Change speed (difficulty)
				++nPieceCount;
				if (nPieceCount % 10 == 0 && nSpeed >= 10)
					nSpeed -= 5;
			}
			nSpeedCounter = 0;
		}

		// RENDER OUTPUT ==========================================================================

		int offsetFieldY{ (LINES - nFieldHeight) / 2 };
		int offsetFieldX{ (COLS - nFieldWidth) / 2 };

		// clear the screen when terminal dimensions change
		if (!(lastTermHeight == LINES && lastTermWidth == COLS))
		{
			lastTermHeight = LINES;
			lastTermWidth  = COLS;
			clear();
		}

		// Draw Field
		for (int y{ 0 }; y < nFieldHeight; ++y)
			for (int x{ 0 }; x < nFieldWidth; ++x)
				mvaddch(offsetFieldY + y, offsetFieldX + x, " ABCDEFG=#"[pField[y * nFieldWidth + x]]);

		// Draw Current Piece
		for (int py{ 0 }; py < nTetrominoSize; ++py)
			for (int px{ 0 }; px < nTetrominoSize; ++px)
				if (tetromino[nCurrentPiece][Rotate(py, px, nCurrentRotation)] == 'X')
					mvaddch(offsetFieldY + nCurrentY + py, offsetFieldX + nCurrentX + px,
						nCurrentPiece + 65);

		// Draw player's score
		mvprintw(offsetFieldY + nFieldHeight + 2, (COLS - 8) / 2, "Score: %d", nScores);

		// if 1 or more lines are full
		if (!vLine.empty())
		{
			refresh();
			std::this_thread::sleep_for(std::chrono::milliseconds(50));

			for (auto& v : vLine)
				for (int px{ 1 }; px < nFieldWidth - 1; ++px)
				{
					for (int py{ v }; py > 0; --py)
						pField[py * nFieldWidth + px] = pField[(py - 1) * nFieldWidth + px];
					pField[px] = 0;
				}

			vLine.clear();
		}

		// open MainMenu on game over
		if (bGameOver)
			if (MainMenu(nScores, bGameOver) == 0) // start again
				initGame();

		refresh();
	}

	delete[] pField;
	endwin();

	return 0;
}
