// Chess engine by Vasiliy Tereshkov, 2019

#include <stdio.h>
#include <mem.h>
#include <assert.h>
#include "ChessEngine.h"



Board::Board(): maxDepth(5), usePruning(true)
  {
  reset();
  }



void Board::reset()
  {
  for (int i = 0; i < 8; i++)
    {
    piece[BLACK][i].set(PAWN, BLACK, i, 6);
    piece[WHITE][i].set(PAWN, WHITE, i, 1);
    }
      
  for (int i = 0; i < 2; i++)
    {
    piece[BLACK][8 + i].set(KNIGHT, BLACK, 1 + 5 * i, 7);
    piece[WHITE][8 + i].set(KNIGHT, WHITE, 1 + 5 * i, 0);

    piece[BLACK][10 + i].set(BISHOP, BLACK, 2 + 3 * i, 7);
    piece[WHITE][10 + i].set(BISHOP, WHITE, 2 + 3 * i, 0);

    piece[BLACK][12 + i].set(ROOK, BLACK, 7 * i, 7);
    piece[WHITE][12 + i].set(ROOK, WHITE, 7 * i, 0);
    }

  piece[BLACK][14].set(QUEEN, BLACK, 3, 7);
  piece[WHITE][14].set(QUEEN, WHITE, 3, 0);

  piece[BLACK][15].set(KING, BLACK, 4, 7);
  piece[WHITE][15].set(KING, WHITE, 4, 0);

  checkmate[BLACK] = checkmate[WHITE] = false;
  }



int Board::getBlackAdvantage()
  {
  int pieceValue[] = {1, 3, 3, 5, 9, 100};
  int value[] = {0, 0};    

  for (int clr = BLACK; clr <= WHITE; clr++)
    {
    if (checkmate[clr])
      value[clr] = -1000000;
    else
      {            
      for (int i = 0; i < 16; i++)
        {
        Piece &p = piece[clr][i];
        if (p.alive)
          {
          int positionBonus = (clr == WHITE) ? p.y : (7 - p.y);
          value[clr] += 100 * pieceValue[p.type] + positionBonus;
          }
        }  
      }    
    }

  return value[BLACK] - value[WHITE];
  }



Piece *Board::pieceAt(int x, int y)
  {
  for (int clr = BLACK; clr <= WHITE; clr++)
    for (int i = 0; i < 16; i++)
      if (piece[clr][i].alive && piece[clr][i].x == x && piece[clr][i].y == y)
        return &piece[clr][i];

  return NULL;
  }



void Board::findValidPawnMoves(Piece &p, ValidityMap &map, bool captureOnly)
  {
  const int step = (p.color == WHITE) ? 1 : -1;

  // no capture
  int xNew = p.x, yNew = p.y + step;
  if (yNew >= 0 && yNew < 8 && !pieceAt(xNew, yNew) && !captureOnly)
    {
    map[xNew][yNew] = true;

    // first move
    yNew = p.y + 2 * step;
    if (!p.moved && !pieceAt(xNew, yNew))
      map[xNew][yNew] = true;
    }

  // capture
  const int delta[2][2] = {{1, step}, {-1, step}};

  for (int i = 0; i < 2; i++)
    {
    xNew = p.x + delta[i][0]; yNew = p.y + delta[i][1];

    if (xNew >= 0 && xNew < 8 && yNew >= 0 && yNew < 8)
      {
      Piece *enemy = pieceAt(xNew, yNew);
      if (enemy && enemy->alive && enemy->color != p.color)
        map[xNew][yNew] = true;
      }
    }
    
  findValidEnPassant(p, map);  
  }



void Board::findValidKnightMoves(Piece &p, ValidityMap &map)
  {
  const int delta[8][2] = {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};

  for (int i = 0; i < 8; i++)
    {
    const int xNew = p.x + delta[i][0], yNew = p.y + delta[i][1];

    if (xNew >= 0 && xNew < 8 && yNew >= 0 && yNew < 8)
      {
      Piece *enemy = pieceAt(xNew, yNew);
      if (!enemy || (enemy->alive && enemy->color != p.color))
        map[xNew][yNew] = true;
      }
    }
  }



void Board::findValidBishopRookQueenKingMoves(Piece &p, ValidityMap &map, bool captureOnly)
  {
  const int delta[8][2] = {{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}};
  bool dirValid[8] = {true, true, true, true, true, true, true, true};

  if (p.type == BISHOP)
    dirValid[0] = dirValid[2] = dirValid[4] = dirValid[6] = false;
  else if (p.type == ROOK)
    dirValid[1] = dirValid[3] = dirValid[5] = dirValid[7] = false;

  const int distMax = (p.type == KING) ? 1 : 7;

  for (int dir = 0; dir < 8; dir++)
    if (dirValid[dir])
      for (int dist = 1; dist <= distMax; dist++)
        {
        const int xNew = p.x + dist * delta[dir][0], yNew = p.y + dist * delta[dir][1];

        if (xNew < 0 || xNew >= 8 || yNew < 0 || yNew >= 8)
          break;

        Piece *enemy = pieceAt(xNew, yNew);
        if (!enemy || (enemy->alive && enemy->color != p.color))
          map[xNew][yNew] = true;

        if (enemy && enemy->alive)
          break;
        }
  
  if (p.type == KING && !captureOnly)
    findValidCastlings(p, map);
  }
  
  
  
  
void Board::findValidCastlings(Piece &king, ValidityMap &map)
  {
  assert(king.type == KING && king.alive);
  
  if (!king.moved)
    {
    assert(king.x == 4 && ((king.color == WHITE && king.y == 0) || (king.color == BLACK && king.y == 7)));

    ValidityMap attackMap;
    Color enemyColor = (king.color == WHITE) ? BLACK : WHITE;
    findAttackMap(enemyColor, attackMap);
    
    // long castling
    Piece *rook = pieceAt(0, king.y);
    if (rook && rook->type == ROOK && rook->alive && rook->color == king.color && !rook->moved)
      if (!pieceAt(1, king.y) && !pieceAt(2, king.y) && !pieceAt(3, king.y))
        if (!attackMap[2][king.y] && !attackMap[3][king.y] && !attackMap[4][king.y])
          map[2][king.y] = true;  
      
    // short castling
    rook = pieceAt(7, king.y);
    if (rook && rook->type == ROOK && rook->alive && rook->color == king.color && !rook->moved)
      if (!pieceAt(5, king.y) && !pieceAt(6, king.y))
        if (!attackMap[4][king.y] && !attackMap[5][king.y] && !attackMap[6][king.y])
          map[6][king.y] = true;  
    }
  }  



void Board::findValidEnPassant(Piece &pawn, ValidityMap &map)
  {
  assert(pawn.type == PAWN && pawn.alive);

  Color enemyColor = (pawn.color == WHITE) ? BLACK : WHITE;
  const int step = (pawn.color == WHITE) ? 1 : -1;
  const int delta[2][2] = {{1, step}, {-1, step}};

  for (int i = 0; i < 2; i++)
    {
    int xNew = pawn.x + delta[i][0], yNew = pawn.y + delta[i][1];

    if (xNew >= 0 && xNew < 8 && yNew >= 0 && yNew < 8 && yNew - step >= 0 && yNew - step < 8)
      {
      Piece *enemy = pieceAt(xNew, yNew - step);
      if (enemy && enemy->alive && enemy->color != pawn.color && enemy->type == PAWN && enemy->doubleMove)
        map[xNew][yNew] = true;
      }
    }
  }  



void Board::findValidMoves(Piece &p, ValidityMap &map, bool captureOnly)
  {
  assert(p.alive);

  switch (p.type)
    {
    case PAWN:
      findValidPawnMoves(p, map, captureOnly); break;
    case KNIGHT:
      findValidKnightMoves(p, map); break;
    case BISHOP:
    case ROOK:
    case QUEEN:
    case KING:
      findValidBishopRookQueenKingMoves(p, map, captureOnly); break;
    default:
      assert(false);
    }
  }
  
  
  
void Board::findAttackMap(Color color, ValidityMap &map)
  {
  memset(&map, 0, sizeof(map));

  for (int i = 0; i < 16; i++)
    {
    Piece &p = piece[color][i];
    if (p.alive)
      findValidMoves(p, map, true);
    }  
  }   



void Board::makeMove(Piece &p, int x, int y)
  {
  // pawn's double move
  for (int i = 0; i < 16; i++)
    piece[p.color][i].doubleMove = false;
    
  if (p.type == PAWN && (y == p.y + 2 || y == p.y - 2))
    p.doubleMove = true;
    
  // en passant
  if (p.type == PAWN && (x == p.x + 1 || x == p.x - 1) && !pieceAt(x, y))
    {
    const int step = (p.color == WHITE) ? 1 : -1;    
    Piece *enemy = pieceAt(x, y - step);
    assert(enemy && enemy->alive && enemy->color != p.color && enemy->type == PAWN && enemy->doubleMove);
    enemy->alive = false;
    }  
    
  // castling
  if (p.type == KING)
    if (x == p.x - 2)       // long
      {
      Piece *rook = pieceAt(0, p.y);
      assert(rook != NULL);
      makeMove(*rook, 3, p.y);
      }
    else if (x == p.x + 2)  // short
      {
      Piece *rook = pieceAt(7, p.y);
      assert(rook != NULL);
      makeMove(*rook, 5, p.y);
      }   
  
  // conventional move    
  Piece *enemy = pieceAt(x, y);

  if (enemy)
    {
    assert(enemy->color != p.color);
    enemy->alive = false;
    if (enemy->type == KING)
      checkmate[enemy->color] = true;
    }

  p.x = x;
  p.y = y;
  p.moved = true;

  // promotion
  if (p.type == PAWN)
    if ((p.color == BLACK && p.y == 0) || (p.color == WHITE && p.y == 7))
      p.type = QUEEN;           
  }



Board Board::makeBestMove(Color color, int depth, int blackAdvantageMaxLowerBound, int blackAdvantageMinUpperBound)
  {
  if (depth >= maxDepth || checkmate[color])
    return *this;

  Board bestBoard;  
  int bestAdvantage = (color == BLACK) ? -10000000 : 10000000;
  int bestX = 0, bestY = 0;
  Piece *bestPiece = NULL;
  bool stop = false;

  for (int i = 0; i < 16 && !stop; i++)
    {
    Piece &p = piece[color][i];
    if (p.alive)
      {
      ValidityMap map;
      memset(&map, 0, sizeof(map));
      findValidMoves(p, map);
      
      for (int x = 0; x < 8 && !stop; x++)
        for (int y = 0; y < 8 && !stop; y++)
          if (map[x][y])
            {
            Board boardCopy(*this);
            Piece &pCopy = boardCopy.piece[color][i];
            Color enemyColor = (color == WHITE) ? BLACK : WHITE;
            
            boardCopy.makeMove(pCopy, x, y);
            
            if (boardCopy.checkmate[enemyColor])
              {
              makeMove(p, x, y);
              return *this;
              }

            // predict enemy's move
            boardCopy = boardCopy.makeBestMove(enemyColor, depth + 1, blackAdvantageMaxLowerBound, blackAdvantageMinUpperBound);

            int blackAdvantage = boardCopy.getBlackAdvantage();

            if ((color == BLACK && blackAdvantage > bestAdvantage) ||
                (color == WHITE && blackAdvantage < bestAdvantage) || !bestPiece)
              {
              bestBoard = boardCopy;
              bestAdvantage = blackAdvantage;
              bestX = x;
              bestY = y;
              bestPiece = &p;
              }
              
            // alpha-beta pruning
            if (usePruning)
              {
              if (color == BLACK && bestAdvantage > blackAdvantageMaxLowerBound)
                blackAdvantageMaxLowerBound = bestAdvantage;

              if (color == WHITE && bestAdvantage < blackAdvantageMinUpperBound)
                blackAdvantageMinUpperBound = bestAdvantage;

              if (blackAdvantageMaxLowerBound >= blackAdvantageMinUpperBound)
                stop = true;
              }
                           
            } // if (map...)
            
      } // if (p.alive)      
    } // for (int i...)

  assert(bestPiece != NULL);
  makeMove(*bestPiece, bestX, bestY);
  
  return bestBoard;
  }



void Board::asString(char *s)
  {
  const char letter[2][6] = {{'p', 'n', 'b', 'r', 'q', 'k'},
                             {'P', 'N', 'B', 'R', 'Q', 'K'}};
  int pos = 0;                           

  for (int y = 7; y >= 0; y--)
    {
    for (int x = 0; x <= 7; x++)
      {
      char c = '*';
      Piece *p = pieceAt(x, y);
      if (p && p->alive)
        c = letter[p->color][p->type];
      s[pos++] = c;
      }
    s[pos++] = 0x0D;  s[pos++] = 0x0A;
    }
    
  s[pos++] = 0;  
  }


