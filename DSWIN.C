/*
*****************************************************************************
* PROGRAM: DeepSkies for Windows (16-bit)                                   *
* PURPOSE: electronic computational star and deep sky atlas                 *
* VERSION: 1.0                                                              *
*  AUTHOR: Rick Towns                                                       *
*    DATE: 1985-03-06                                                       *
* CHANGED: 2020-11-28 by RT                                                 *
*****************************************************************************
*/

/* REQUIRED LIBRARIES */
#include "windows.h"                    /* REQUIRED FOR WINDOWS PROGRAMS */
#include "dswin.h"                      /* SPECIFIC TO THIS PROGRAM */
#include "math.h"                       /* MATH ROUTINES */
#include "stdio.h"                      /* INPUT/OUTPUT ROUTINES */
#include "stdlib.h"                     /* STANDARD LIBRARY ROUTINES */
#include "string.h"                     /* STRING ROUTINES */

/* GLOBAL DATA STRUCTURES */
typedef struct {
	double RA;                          /* DISPLAY RA AT CENTRE */
	double Dec;                         /* DISPLAY DEC AT CENTRE */
	double FOV;                         /* DISPLAY FIELD OF VIEW */
	double Mag;                         /* LIMITING MAGNITUDE */
	double ROT;                         /* ROTATION OF DISPLAY */
	double MinRA;                       /* MIN RA FOR DISPLAY */
	double MinDec;                      /* MIN DEC FOR DISPLAY */
	double MaxRA;                       /* MAX RA FOR DISPLAY */
	double MaxDec;                      /* MAX DEC FOR DISPLAY */
	int MaxX;                           /* MAX X PIXELS ON DISPLAY */
	int MaxY;                           /* MAX Y PIXELS ON DISPLAY */
	double CRA;                         /* CALCULATED RA VALUE */
	double CDec;                        /* CALCULATED DEC VALUE */
	int CX;                             /* CALCULATED X VALUE */
	int CY;                             /* CALCULATED Y VALUE */
	long Stars;                         /* NUMBER OF STARS IN DATA FILE */
} DSCONTROL;

typedef struct {
	char Label[17];                     /* STAR NAME / LABEL */
	double RA;                          /* STAR RIGHT ASCENSION */
	double Dec;                         /* STAR DECLINATION */
	double Mag;                         /* STAR VISUAL MAGNITUDE */
	char Class[2];                      /* STAR CLASS IE: A2 */
	double PMRA;                        /* RA PROPER MOTION IN MAS/YR */
	double PMDec;                       /* DEC PROPER MOTION IN MAS/YR */
} STARDATA;

/* GLOBAL CONSTANTS */
double PI    = 3.141592653590;          /* REASONABLE VALUE FOR PI */
double TWOPI = 6.283185307180;          /* DOUBLE PI */
double PI2   = 1.570796326795;          /* HALF OF PI */
double RADS  = 0.017453292520;          /* RADIANS */
int streclen = 61;                      /* RECORD LENGTH OF STAR DATA */

/* GLOBAL VARIABLES */
static HANDLE hInst;                    /* CURRENT INSTANCE */
DSCONTROL DsC;                          /* PROGRAM CONTROL STRUCTURE */
char buffer[80];                        /* USED FOR RANDOM OUTPUT */
FILE *fp_stars;                         /* MAIN STAR DATABASE */
FILE *fp_idx_ra;                        /* INDEX BY RIGHT ASCENSION */
FILE *fp_idx_dec;                       /* INDEX BY DECLINATION */
FILE *fp_idx_mag;                       /* INDEX BY MAGNITUDE */
long floc;                              /* USED IN FILE PROCESSING */
STARDATA Star;                          /* STAR STRUCTURE */

/* FUNCTION DEFINITIONS */
long FAR PASCAL DSWndProc(HWND, unsigned, WORD, LONG);
void EQConvert(double, double);
double NormalizeZeroTwoPI(double);
void GetStar(long);
void rtrim(char*);
int matherr(struct exception*);

/*
*****************************************************************************
* FUNCTION: DSInit()                                                        *
*  PURPOSE: initializes window data and registers window class              *
* REQUIRES: instance handle                                                 *
*  RETURNS: boolean (TRUE | FALSE)                                          *
*  COMMENT: returns TRUE if window class is successfully registered         *
*****************************************************************************
*/
BOOL DSInit(hInstance)
HANDLE hInstance;
{
	NPWNDCLASS  pDSClass;               /* POINTER FOR CLASS */

	/* ALLOCATE RAM FOR CLASS STRUCTURE */
	pDSClass = (NPWNDCLASS)LocalAlloc(LPTR, sizeof(WNDCLASS));

	/* POPULATE CLASS */
	pDSClass->hCursor        = LoadCursor(NULL, IDC_ARROW);
	pDSClass->hIcon          = LoadIcon(hInstance, "DSICON");
	pDSClass->lpszMenuName   = "DSMENU";
	pDSClass->lpszClassName  = "DSCLASS";
	pDSClass->hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
	pDSClass->hInstance      = hInstance;
	pDSClass->style          = CS_HREDRAW | CS_VREDRAW;
	pDSClass->lpfnWndProc    = DSWndProc;

	/* TRY TO REGISTER THE CLASS */
	if (!RegisterClass((LPWNDCLASS)pDSClass)) {
		/* FAILED? RETURN FALSE */
		return FALSE;
	}

	/* FREE MEMORY FROM CLASS INITIALIZATION */
	LocalFree((HANDLE)pDSClass);

	/* SUCCESS! RETURN TRUE */
	return TRUE;
}

/*
*****************************************************************************
* FUNCTION: WinMain()                                                       *
*  PURPOSE: creates main window, processes message loop until WM_QUIT       *
* REQUIRES: instance handle, previous handle, command line, show command    *
*  RETURNS: integer                                                         *
*  COMMENT: returns the wParam of the message structure as the exit code    *
*****************************************************************************
*/
int PASCAL WinMain(hInstance, hPrevInstance, lpszCmdLine, cmdShow)
HANDLE hInstance, hPrevInstance;
LPSTR lpszCmdLine;
int cmdShow;
{
	MSG   msg;                          /* MESSAGE STRUCTURE */
	HDC   hDC;                          /* DEVICE CONTEXT */
	HWND  hWnd;                         /* WINDOW HANDLE */
	HMENU hMenu;                        /* MENU HANDLE */

	if (!hPrevInstance) {               /* CHECK FOR OTHER INSTANCES */
		if (!DSInit(hInstance)) {       /* INITIALIZE SHARED THINGS */
			return FALSE;               /* EXIT IF UNABLE TO INITIALIZE */
		}
	}

	hWnd = CreateWindow(
		"DSCLASS",                      /* REGISTERED CLASS NAME */
		"DeepSkies",                    /* WINDOW TITLE TEXT */
		WS_TILEDWINDOW,                 /* WINDOW STYLE */
		0,                              /*  X - HORIZONTAL POSITION */
		0,                              /*  Y - VERTICAL POSITION */
		0,                              /* CX - WIDTH */
		0,                              /* CY - HEIGHT */
		(HWND)NULL,                     /* NO PARENT */
		(HMENU)NULL,                    /* USE CLASS MENU */
		(HANDLE)hInstance,              /* HANDLE TO WINDOW INSTANCE */
		(LPSTR)NULL                     /* NO PARAMS TO PASS ON */
		);

	/* SAVE INSTANCE HANDLE IN STATIC VARIABLE FOR REUSE */
	hInst = hInstance;

	/* MAKE WINDOW VISIBLE AND UPDATE CLIENT AREA */
	ShowWindow(hWnd, cmdShow);
	UpdateWindow(hWnd);

	/* ACQUIRE AND DISPATCH MESSAGE UNTIL WM_QUIT IS RECEIVED */
	while (GetMessage((LPMSG)&msg, NULL, 0, 0)) {
		TranslateMessage((LPMSG)&msg);  /* TRANSLATES VIRTUAL KEY CODES */
		DispatchMessage((LPMSG)&msg);   /* DISPATCHES MESSAGE TO WINDOW */
	}

	/* RETURNS VALUE FROM POSTQUITMESSAGE */
	return (int)msg.wParam;
}

/*
*****************************************************************************
* FUNCTION: DSWndProc()                                                     *
*  PURPOSE: process messages                                                *
* REQUIRES: window handle, messsage, word parameter, long parameter         *
*  RETURNS: long integer                                                    *
*  COMMENT: process messages and if required, returns a long zero           *
*****************************************************************************
*/
long FAR PASCAL DSWndProc(hWnd, message, wParam, lParam)
HWND hWnd;
unsigned message;
WORD wParam;
LONG lParam;
{

	RECT r;                             /* USED FOR SCREEN SIZE */
	HDC hDC;                            /* DEVICE CONTEXT */
	HBRUSH hBrush, hOldBrush;           /* USED FOR PAINTING */
	HPEN hPen, hOldPen;                 /* USED FOR DRAWING */
	HFONT hFont, hOldFont;              /* USED FOR LABELS */
	LOGFONT lf;                         /* USED FOR FONTS */
	PAINTSTRUCT ps;                     /* PAINT STRUCTURE */
	int X, Y;                           /* USED FOR DRAWING PIXELS */
	long i;                             /* USED FOR LOOPING */
	double sz;                          /* USED FOR STAR SIZE */

	switch (message) {

		/* COMMAND FROM MENU */
		case WM_COMMAND: {

			switch(wParam) {

				/* FILE -> EXIT */
				case MENU_EXIT: {
					DestroyWindow(hWnd);
					break;
				}

			}
			break;

		}

		case WM_CREATE: {

			/* INITIALIZE APPLICATION */
			DsC.RA = 300.0;
			DsC.Dec = 40.0;
			DsC.FOV = 60.0;
			DsC.Mag =  6.5;
			DsC.ROT =  0.0;

			break;
		}

		/* PAINT THE WINDOW */
		case WM_PAINT: {

			/* GET SCREEN SIZE */
			GetClientRect(hWnd, &r);
			DsC.MaxX = r.right;
			DsC.MaxY = r.bottom;

			/* ACTUALLY PAINT SOME STUFF */
			hDC = BeginPaint(hWnd, &ps);

			/* SET SCREEN MODES */
			SetBkMode(hDC, OPAQUE);        /* OPAQUE OR TRANSPARENT */
			SetROP2(hDC, R2_COPYPEN);

			/* SELECT FONT, BRUSH AND PEN */
			lf.lfEscapement = 0;
			lf.lfOrientation = 0;
			lf.lfWeight = 0;
			lf.lfItalic = 0;
			lf.lfUnderline = 0;
			lf.lfStrikeOut = 0;
			lf.lfCharSet = OEM_CHARSET;
			lf.lfOutPrecision = 0;
			lf.lfClipPrecision = 0;
			lf.lfQuality = 0;
			lf.lfPitchAndFamily = 0;
			lf.lfHeight = 11;
			lf.lfWidth = 0;
			strcpy(lf.lfFaceName, "Helv");

			hFont = CreateFontIndirect(&lf);
			hOldFont = SelectObject(hDC, (HANDLE)hFont);

			hPen = CreatePen(PS_SOLID, 1, 0x00FFFFFF);
			hOldPen = SelectObject(hDC, (HANDLE)hPen);
			SetTextColor(hDC, 0x00FFFFFF);

			hBrush = CreateSolidBrush(0x0FFFFFF);
			hOldBrush = SelectObject(hDC, (HANDLE)hBrush);

			/* PROCESS STARS.DAT FILE */

			if ((fp_stars = fopen("STARS.DAT","r")) == NULL) {
				MessageBox(hWnd, "Unable to open data file!","ERROR", MB_OK);
			} else {
				/* CHECK DATAFILE INTEGRITY */
				fseek(fp_stars, 0L, SEEK_END);
				floc = ftell(fp_stars);
				if(floc % streclen != 0) {
					MessageBox(hWnd, "DATA FILE ERROR!", "ERROR", MB_OK);
				} else {
					/* DATA FILE OK - YOU MAY PROCEED! */
					DsC.Stars = floc / streclen - 5;

					for(i = 1; i <= DsC.Stars; i++) {

						GetStar(i);
						/* rtrim(Star.Label); */

						if(Star.Mag <= DsC.Mag) {

							EQConvert(Star.RA, Star.Dec);

							if(DsC.Mag <= 15.0) {
								sz = DsC.Mag - Star.Mag + 0.50;
							} else {
								sz = 15 - Star.Mag + 0.50;
							}

							if(sz < 1.0) {
								sz = 1.0;
							}

							if(sz > 8.0) {
								sz = 8.0;
							}

							SetBkMode(hDC, OPAQUE);

							if(sz == 1.0) {
								SetPixel(hDC, DsC.CX, DsC.CY, 0x00FFFFFF);
							} else {
								Ellipse(hDC, DsC.CX - (int)sz, DsC.CY - (int)sz, DsC.CX + (int)sz, DsC.CY + (int)sz);
							}

							if(Star.Mag < DsC.Mag - 3.0) {
								SetBkMode(hDC, TRANSPARENT);
								sprintf(buffer, "%s", Star.Label);
								TextOut(hDC, DsC.CX + (int)sz, DsC.CY - (int)sz, buffer, strlen(buffer));
							}
						}
					}
				}

				fclose(fp_stars);
			}

			/* DISCARD OBJECTS */
			SelectObject(hDC, (HANDLE)hOldPen);
			SelectObject(hDC, (HANDLE)hOldBrush);
			SelectObject(hDC, (HANDLE)hOldFont);

			DeleteObject(hPen);
			DeleteObject(hBrush);
			DeleteObject(hFont);
			DeleteObject(hOldPen);
			DeleteObject(hOldBrush);
			DeleteObject(hOldFont);

			EndPaint(hWnd, &ps);

			break;
		}

		/* WINDOW BEING DESTROYED */
		case WM_DESTROY: {

			/* PERFORM CLEAN UP HERE */
			/* IE: DELETE OBJECTS */
			fcloseall();

			PostQuitMessage(0);
			break;
		}

		/* PASS MESSAGE ON TO WINDOWS IF UNPROCESSED */
		default: {
			return DefWindowProc(hWnd, message, wParam, lParam);
			break;
		}

	}

	return(0L);

}

/*
*****************************************************************************
* FUNCTION: EQConvert()                                                     *
*  PURPOSE: convert RA/DEC coordinates to X/Y pixel coordinates             *
* REQUIRES: double RA and double DEC                                        *
*  RETURNS: nothing                                                         *
*  COMMENT: populates global DsC structure CX and CY                        *
*****************************************************************************
*/
void EQConvert(RA, Dec)
double RA;
double Dec;
{

	/* LOCAL VARIABLES */
	double Az, Alt, TempVal, nz, az2, tX, tY, xyScale;
	double fRA, fDec, fFOV, fROT, p1, p2;

	double temp;

	/* POPULATE LOCAL VARIABLES */
	fRA  = DsC.RA  * RADS;
	fDec = DsC.Dec * RADS;
	fFOV = DsC.FOV * RADS;
	fROT = DsC.ROT * RADS;
	RA   = RA      * RADS;
	Dec  = Dec     * RADS;

	/* CONVERT RA TO AZIMUTH */
	p1 = sin(fRA - RA);
	p2 = cos(fRA - RA) * sin(fDec) - tan(Dec) * cos(fDec);

	/*
		NOTE ABOUT ATAN2:

		ATAN2(0,0) RETURNS 0
		ATAN2(0, -0) RETURNS PI
		ATAN2(-0, 0) RETURNS -0
		ATAN2(-0, -0) RETURNS -PI
	*/
	if (p1 == 0.0 && p2 == 0.0) {
		Az = 0.0;
	} else {
		Az = atan2(p1, p2);
	}
	TempVal = sin(fDec) * sin(Dec) + cos(fDec) * cos(Dec) * cos(fRA - RA);

	/* CONVERT DEC TO ALTITUDE */
	if (TempVal >= 1.0) {
		Alt = PI2;
	}
	else {
		Alt = asin(TempVal);
	}

	/* CONVERT ALT/AZ TO X/Y */
	nz = 1.0 - 2.0 * Alt / PI;
	az2 = Az - PI2 + fROT;
	tX = (nz * cos(az2)) * PI / fFOV;
	tY = -(nz * sin(az2)) * PI / fFOV;

	/* SCALE APPROPRIATELY FOR THE CURRENT DISPLAY */
	xyScale = (DsC.MaxX / fFOV) / (120.0 / DsC.FOV);

	/* SAVE RESULTS INTO DsC STRUCTURE */
	DsC.CX = (int)((DsC.MaxX / 2.0) + (tX * xyScale));
	DsC.CY = (int)((DsC.MaxY / 2.0) + (tY * xyScale));

}

/*
*****************************************************************************
* FUNCTION: NormalizeZeroTwoPI()                                            *
*  PURPOSE: converts a given value to be between 0 and 2*PI                 *
* REQUIRES: double value                                                    *
*  RETURNS: double value normalized                                         *
*  COMMENT: no further comment                                              *
*****************************************************************************
*/
double NormalizeZeroTwoPI(value)
double value;
{

	double tmpVal;
	tmpVal = value;

	while (tmpVal >= TWOPI) {
		tmpVal = tmpVal - TWOPI;
	}

	while (tmpVal < 0.0) {
		tmpVal = tmpVal + TWOPI;
	}

	return(tmpVal);

}

/*
*****************************************************************************
* FUNCTION: rtrim()                                                         *
*  PURPOSE: trims spaces from end of string                                 *
* REQUIRES: pointer to string                                               *
*  RETURNS: nothing                                                         *
*  COMMENT: replaces char[32] with char[0] on right of string               *
*****************************************************************************
*/
void rtrim(target)
char *target;
{
	int i;

	for (i=strlen(target) - 1; i > 0; i--) {
		if(target[i] == ' ') {
			target[i] = '\0';
		} else {
			i = 0;
		}
	}
}

/*
*****************************************************************************
* FUNCTION: matherr()                                                       *
*  PURPOSE: handles errors generated by math.h routines                     *
* REQUIRES: math.h library                                                  *
*  RETURNS: nothing                                                         *
*  COMMENT: by doing nothing in this function, no math errors willd display *
*****************************************************************************
*/
int matherr(err)
struct exception *err;
{
	/* DO NOTHING! */
}

/*
*****************************************************************************
* FUNCTION: GetStar()                                                       *
*  PURPOSE: retrieves a specific star from the star data file               *
* REQUIRES: long value                                                      *
*  RETURNS: nothing                                                         *
*  COMMENT: puts star data into Star global variable                        *
*****************************************************************************
*/
void GetStar(index)
long index;
{
	long fspot;
	char SL[80];
	char S[80];
	int error = 0;

	fspot = (index + 4) * streclen;

	if(fseek(fp_stars, fspot, SEEK_SET) == 0) {
		/* FOUND STAR - LETS LOAD IT */
		if(fgets(SL, streclen, fp_stars) != NULL) {

			/* LABEL = CHAR 1 TO 17 */
			strncpy(Star.Label, SL, 17);
			Star.Label[16] = '\0';
			/* rtrim(&star.Label); */

			/* RA = CHAR 18 to 28 */
			strncpy(S, SL + 17, 11);
			S[11] = '\0';
			Star.RA = atof(S);

			/* DEC = CHAR 29 TO 40 */
			strncpy(S, SL + 28, 12);
			S[12] = '\0';
			Star.Dec = atof(S);

			/* MAG = CHAR 41 TO 45 */
			strncpy(S, SL + 40, 5);
			S[5] = '\0';
			Star.Mag = atof(S);

			/* CLASS = CHAR 46 TO 47 */
			strncpy(S, SL + 45, 3);
			S[2] = '\0';
			sprintf(Star.Class, "%s", S);

			/* PMRA = CHAR 48 TO 56 */
			strncpy(S, SL + 47, 9);
			S[9] = '\0';
			Star.PMRA = atof(S);

			/* PMDEC = CHAR 57 TO 65 */
			strncpy(S, SL + 56, 9);
			S[9] = '\0';
			Star.PMDec = atof(S);

		} else {
			error = 1;
		}

	} else {
		error = 1;
	}

	if (error == 1) {
		/*
		Star.Label =
		Star.RA =
		Star.Dec =
		Star.Mag =
		Star.Class =
		Star.PMRA =
		Star.PMDec =
		*/
	}

}

