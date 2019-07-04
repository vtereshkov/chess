// Chess engine header by Vasiliy Tereshkov, 2019

#ifndef ChessEngineH
#define ChessEngineH
 


enum Type {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};
enum Color {BLACK, WHITE};

  

struct Piece
  {
  Type type;
  Color color;
  int x, y;
  bool alive;
  bool moved;         // needed for castling
  bool doubleMove;    // needed for en passant
  
  void set(Type type0, Color color0, int x0, int y0, bool alive0 = true, bool moved0 = false, bool doubleMove0 = false)
    {
    type = type0;  color = color0;  x = x0;  y = y0;  alive = alive0;  moved = moved0;  doubleMove = doubleMove0;
    }  
  };



typedef bool ValidityMap[8][8];
 
  
  
class Board
  {
  public:  
    Piece piece[2][16];
    bool checkmate[2];

    Board();
    void reset();
    Piece *pieceAt(int x, int y);
    void findValidMoves(Piece &p, ValidityMap &map, bool captureOnly = false);
    void makeMove(Piece &p, int x, int y);
    Board makeBestMove(Color color, int depth = 0,
                      int blackAdvantageMaxLowerBound = -10000000,
                      int blackAdvantageMinUpperBound =  10000000);
    void asString(char *s);
    
  private:    
    int maxDepth;
    bool usePruning;
    
    int getBlackAdvantage();  
    void findValidPawnMoves(Piece &p, ValidityMap &map, bool captureOnly);
    void findValidKnightMoves(Piece &p, ValidityMap &map);
    void findValidBishopRookQueenKingMoves(Piece &p, ValidityMap &map, bool captureOnly);
    void findValidCastlings(Piece &king, ValidityMap &map);
    void findValidEnPassant(Piece &pawn, ValidityMap &map);
    void findAttackMap(Color color, ValidityMap &map);

  };

#endif

