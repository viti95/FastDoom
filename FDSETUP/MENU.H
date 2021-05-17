//
// MENU.H
//

//
// Structures for menu
//
typedef struct item_s
{
	int	id;			 	// enum value for id (i.e., MODSKILL1)
	int	x;				 	// x-coord of item
	int	y;
	int	w;				 	// width of item (for inverse bar)
	int	left;				// item # if LEFT ARROW pressed (-1 = none)
	int	right;			// item # if RIGHT ARROW pressed (-1 = none)
	int	up;				// item # if UP ARROW pressed (0 = none)
	int	down;				// item # if DOWN ARROW pressed (0 = none)
} item_t;

typedef struct
{
	item_t	*items;		// * to items
	int	startitem;		// item to start on
	int	maxitems;		// # of items in menu
	char	invert;			// attribute for inversion
} menu_t;

void	SetupMenu(menu_t *menu);
int	GetMenuInput(void);
void	SetMark(item_t *item,int value);
void Sound(int freq, int dly);

extern short menukey;
