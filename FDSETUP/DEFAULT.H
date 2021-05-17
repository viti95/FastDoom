//
// DEFAULT.H
//
typedef struct
{
	char	   *name;
	int		*location;
	int		defaultvalue;
} default_t;

void M_SaveDefaults (void);
int CheckParm(char *string);
int M_LoadDefaults (void);
