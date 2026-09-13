#include <cstdint>
#include <locale.h>
#include <ncurses.h>
// #include <string.h>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <random>

#define COLOR_REDDISH 10

std::vector<std::string> tetromino(7);
const int nTetrominoSize{ 4 };
const int nFieldWidth{ 14 };
const int nFieldHeight{ 18 };
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
	std::random_device rd;
	std::mt19937 gen{ rd() };
	std::uniform_int_distribution<int> dist(0, 1000);

	// init ncurses
	setlocale(LC_ALL, "");
	initscr();
	raw();
	noecho();
	curs_set(0);	 // disable cursor
	set_escdelay(0); // disable delay for escape sequences

	WINDOW* wField{ newwin(nFieldHeight, (nFieldWidth - 1) * 2, (LINES - nFieldHeight) / 2,
			       (COLS - nFieldWidth * 2) / 2) };
	wborder(wField, 0, 0, ' ', 0, 0, 0, 0, 0);
	nodelay(wField, true);
	keypad(wField, true);

	start_color();
	init_color(COLOR_REDDISH, 1000, 0, 0);

	init_pair(1, COLOR_REDDISH, COLOR_REDDISH);
	init_pair(2, COLOR_GREEN, COLOR_GREEN);
	init_pair(3, COLOR_YELLOW, COLOR_YELLOW);
	init_pair(4, COLOR_BLUE, COLOR_BLUE);
	init_pair(5, COLOR_MAGENTA, COLOR_MAGENTA);
	init_pair(6, COLOR_CYAN, COLOR_CYAN);
	init_pair(7, COLOR_WHITE, COLOR_WHITE);
	init_pair(9, COLOR_WHITE, COLOR_BLACK);

	// Tetromino assets
	CreateAssets();

	// init Field
	pField = new unsigned char[nFieldWidth * nFieldHeight];
	for (int y{ 0 }; y < nFieldHeight; ++y)
		for (int x{ 0 }; x < nFieldWidth; ++x)
		{
			pField[y * nFieldWidth + x] = (x == 0 || x == nFieldWidth - 1 || y == nFieldHeight - 1)
							  ? 9  // 3 - boarder
							  : 0; // 0 - inside area
		}

	// Game logic stuff
	bool bGameOver{ false };

	int nCurrentPiece{ dist(gen) % 7 };
	int nCurrentRotation{ dist(gen) % 4 };
	int nCurrentY{ 0 };
	int nCurrentX{ (nFieldWidth - nTetrominoSize) / 2 };
	int nCurrentColor{ nCurrentPiece + 1 };

	bool bRotateHold{ false };

	std::vector<int> vLine;

	int nSpeed{ 120 }; // difficulty of the game
	int64_t nFrameCounter{ 0 };
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
		nCurrentColor	 = nCurrentPiece + 1;

		bRotateHold = false;

		nSpeed	       = 120;
		nFrameCounter  = 0;
		bForceDown     = false;
		nPieceCount    = 0;
		nScores	       = 0;
		lastTermHeight = LINES;
		lastTermWidth  = COLS;

		// clear the field inside area
		for (int py{ 0 }; py < nFieldHeight - 1; ++py)
			for (int px{ 1 }; px < nFieldWidth - 1; ++px)
				pField[py * nFieldWidth + px] = 0;
	};

	int fps{ 120 };
	auto frame_duration{ std::chrono::duration<double>(1.0 / fps) };

	// GAME LOOP
	while (!bGameOver)
	{

		// GAME TIMING ============================================================================

		auto frame_start{ std::chrono::steady_clock::now() };
		bForceDown = (nFrameCounter % nSpeed == 0);

		// INPUT ==================================================================================

		int key{};
		int c{ wgetch(wField) };

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

		if (c == 27) // ESC
		{
			int nSelectedOption{ MainMenu(nScores) };

			// Restart game
			if (nSelectedOption == 0)
			{
				initGame();
				clear();
				wclear(wField);
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
							    nCurrentPiece + 1; // 1 - ██

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
								    8; // cleared line -  ░░
							vLine.push_back(nCurrentY + py);
						}
					}

				// Choose next piece
				nCurrentPiece	 = dist(gen) % 7;
				nCurrentRotation = dist(gen) % 4;
				nCurrentY	 = 0;
				nCurrentX	 = (nFieldWidth - nTetrominoSize) / 2;
				nCurrentColor	 = nCurrentPiece + 1;

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
				if (nPieceCount % 10 == 0 && nSpeed > 20)
					nSpeed -= 10;
			}
		}

		// RENDER OUTPUT ==========================================================================

		// clear and move the window when terminal dimensions change
		if (!(lastTermHeight == LINES && lastTermWidth == COLS))
		{
			lastTermHeight = LINES;
			lastTermWidth  = COLS;
			clear();
			mvwin(wField, (LINES - nFieldHeight) / 2, (COLS - nFieldWidth * 2) / 2);
		}

		// Draw Field
		for (int y{ 0 }; y < nFieldHeight; ++y)
			for (int x{ 0 }; x < nFieldWidth; ++x)
			{
				unsigned char cell{ pField[y * nFieldWidth + x] };
				// -1 (+ decreased window width) to remove most left and most right empty colums
				if (cell == 0)
				{
					wattron(wField, COLOR_PAIR(9));
					mvwprintw(wField, y, x * 2 - 1, "░░");
					wattroff(wField, COLOR_PAIR(9));
				}
				if (cell >= 1 && cell <= 7)
				{
					wattron(wField, COLOR_PAIR(cell));
					mvwaddstr(wField, y, x * 2 - 1, "██");
					wattroff(wField, COLOR_PAIR(cell));
				}
				if (pField[y * nFieldWidth + x] == 8)
					mvwprintw(wField, y, x * 2 - 1, "░░");
			}

		// Draw Current Piece
		for (int py{ 0 }; py < nTetrominoSize; ++py)
			for (int px{ 0 }; px < nTetrominoSize; ++px)
				if (tetromino[nCurrentPiece][Rotate(py, px, nCurrentRotation)] == 'X')
				{
					wattron(wField, COLOR_PAIR(nCurrentColor));
					mvwprintw(wField, nCurrentY + py, (nCurrentX + px) * 2 - 1, "██");
					wattroff(wField, COLOR_PAIR(nCurrentColor));
				}

		// Draw player's score
		mvprintw((LINES - nFieldHeight) / 2 + nFieldHeight + 2, (COLS - 8) / 2, "Score: %d", nScores);
		refresh();

		// if 1 or more lines are full
		if (!vLine.empty())
		{
			// refresh();
			// std::this_thread::sleep_for(std::chrono::milliseconds(50));

			for (auto& v : vLine)
				for (int px{ 1 }; px < nFieldWidth - 1; ++px)
				{
					for (int py{ v }; py > 0; --py)
						pField[py * nFieldWidth + px] = pField[(py - 1) * nFieldWidth + px];
					pField[px] = 0;
				}

			vLine.clear();
		}

		wborder(wField, 0, 0, ' ', 0, 0, 0, 0, 0);
		wrefresh(wField);

		auto frame_end{ std::chrono::steady_clock::now() };
		auto elapsed{ frame_end - frame_start };
		if (elapsed < frame_duration)
			std::this_thread::sleep_for(frame_duration - elapsed);

		++nFrameCounter;

		// open MainMenu on game over
		if (bGameOver)
			if (MainMenu(nScores, bGameOver) == 0) // start again
			{
				initGame();
				clear();
			}
	}

	delete[] pField;
	endwin();

	return 0;
}
