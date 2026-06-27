// WL_MAIN.C

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include "render.h"
#include "assets.h"
#include "WL_DEF.H"
#pragma hdrstop


/*
=============================================================================

						   WOLFENSTEIN 3-D

					  An Id Software production

						   by John Carmack

=============================================================================
*/

/*
=============================================================================

						 LOCAL CONSTANTS

=============================================================================
*/


#define FOCALLENGTH     (0x5700l)               // in global coordinates
#define VIEWGLOBAL      0x10000                 // globals visable flush to wall

#define VIEWWIDTH       256                     // size of view window
#define VIEWHEIGHT      144

/*
=============================================================================

						 GLOBAL VARIABLES

=============================================================================
*/

char            str[80],str2[20];
int				tedlevelnum;
boolean         tedlevel;
boolean         nospr;
int                     dirangle[9] = {0,ANGLES/8,2*ANGLES/8,3*ANGLES/8,4*ANGLES/8,
	5*ANGLES/8,6*ANGLES/8,7*ANGLES/8,ANGLES};

//
// proejection variables
//
fixed           focallength;
unsigned        screenofs;
int             viewwidth;
int             viewheight;
int             centerx;
int             shootdelta;                     // pixels away from centerx a target can be
fixed           scale,maxslope;
long            heightnumerator;
int                     minheightdiv;


void            Quit (char *error);

boolean         startgame,loadedgame,virtualreality;
int             mouseadjustment;

char	configname[13]="CONFIG.";


/*
=============================================================================

						 LOCAL VARIABLES

=============================================================================
*/


/*
====================
=
= ReadConfig
=
====================
*/

void ReadConfig(void)
{
	FILE *file_ptr = fopen(configname, "rb"); // read-only, binary

	if (file_ptr != NULL) { // valid config file
		fread(Scores, sizeof(HighScore) * MaxScores, 1, file_ptr);

		// legacy sound stuff.  keep it here so we can parse the file format, but we won't be using it
		SDMode          sd;
		SMMode          sm;
		SDSMode         sds;
		fread(&sd, sizeof(sd), 1, file_ptr); // sound mode
		fread(&sm, sizeof(sm), 1, file_ptr); // music mode
		fread(&sds, sizeof(sds), 1, file_ptr); // digi device

		fread(&mouseenabled, sizeof(mouseenabled), 1, file_ptr);
		fread(&joystickenabled, sizeof(joystickenabled), 1, file_ptr);
		fread(&joypadenabled, sizeof(joypadenabled), 1, file_ptr);
		fread(&joystickprogressive, sizeof(joystickprogressive), 1, file_ptr);
		fread(&joystickport, sizeof(joystickport), 1, file_ptr);

		fread(&dirscan, sizeof(dirscan), 1, file_ptr);
		fread(&buttonscan, sizeof(buttonscan), 1, file_ptr);
		fread(&buttonmouse, sizeof(buttonmouse), 1, file_ptr);
		fread(&buttonjoy, sizeof(buttonjoy), 1, file_ptr);

		fread(&viewsize, sizeof(viewsize), 1, file_ptr);
		fread(&mouseadjustment, sizeof(mouseadjustment), 1, file_ptr);

		fclose(file_ptr);

		if (!MousePresent)
			mouseenabled = false;
		if (!JoysPresent[joystickport])
			joystickenabled = false;

		MainMenu[6].active=1;
		MainItems.curpos=0;
	} else { // no config file, so select by hardware
		if (MousePresent)
			mouseenabled = true;

		joystickenabled = false;
		joypadenabled = false;
		joystickport = 0;
		joystickprogressive = false;

		viewsize = 15;
		mouseadjustment=5;
	}
}


/*
====================
=
= WriteConfig
=
====================
*/

void WriteConfig(void)
{
	FILE *file_ptr = fopen(configname, "wb"); // read-only, binary

	if (file_ptr != NULL) {
		fwrite(Scores, sizeof(HighScore), MaxScores, file_ptr);

		// write empty data in the shape of old sound data to keep the order of the file
		fwrite(&SoundMode, sizeof(SoundMode), 1, file_ptr);
		fwrite(&MusicMode, sizeof(MusicMode), 1, file_ptr);
		fwrite(&DigiMode, sizeof(DigiMode), 1, file_ptr);

		fwrite(&mouseenabled, sizeof(mouseenabled), 1, file_ptr);
		fwrite(&joystickenabled, sizeof(joystickenabled), 1, file_ptr);
		fwrite(&joypadenabled, sizeof(joypadenabled), 1, file_ptr);
		fwrite(&joystickprogressive, sizeof(joystickprogressive), 1, file_ptr);
		fwrite(&joystickport, sizeof(joystickport), 1, file_ptr);

		fwrite(&dirscan, sizeof(dirscan), 1, file_ptr);
		fwrite(&buttonscan, sizeof(buttonscan), 1, file_ptr);
		fwrite(&buttonmouse, sizeof(buttonmouse), 1, file_ptr);
		fwrite(&buttonjoy, sizeof(buttonjoy), 1, file_ptr);

		fwrite(&viewsize, sizeof(viewsize), 1, file_ptr);
		fwrite(&mouseadjustment, sizeof(mouseadjustment), 1, file_ptr);

		fclose(file_ptr);
	}
}


//===========================================================================

/*
=====================
=
= NewGame
=
= Set up new game to start from the beginning
=
=====================
*/

void NewGame (int difficulty,int episode)
{
	memset (&gamestate,0,sizeof(gamestate));
	gamestate.difficulty = difficulty;
	gamestate.weapon = gamestate.bestweapon
		= gamestate.chosenweapon = wp_pistol;
	gamestate.health = 100;
	gamestate.ammo = STARTAMMO;
	gamestate.lives = 3;
	gamestate.nextextra = EXTRAPOINTS;
	gamestate.episode=episode;

	startgame = true;
}

//===========================================================================

void DiskFlopAnim(int x,int y)
{
	static char which=0;
	if (!x && !y)
		return;
	R_DrawPic(x,y,C_DISKLOADING1PIC+which);
	which^=1;
}

// modernized
long DoChecksum(const void *source,size_t size,long checksum)
{
	unsigned i;

	for (i=0;i<size-1;i++)
	checksum += source[i]^source[i+1];

	return checksum;
}

// modernized
long fwrite_with_checksum( const void *ptr,
    size_t size,
    size_t count,
    FILE *stream,
	long checksum) {
	
	frwite(ptr, size, count, stream);
	return DoChecksum(ptr, size, checksum);
}

// modernized
long fread_with_checksum( const void *ptr,
    size_t size,
    size_t count,
    FILE *stream,
	long checksum) {
	
	fread(ptr, size, count, stream);
	return DoChecksum(ptr, size, checksum);
}


/*
==================
=
= SaveTheGame
=
==================
*/

boolean SaveTheGame(FILE *file_ptr, int x, int y)
{ 
	struct diskfree_t dfree;
	long avail,size,checksum;
	objtype *ob,nullobj;

	// TODO: check for free space?

	avail = (long)dfree.avail_clusters *
			dfree.bytes_per_sector *
			dfree.sectors_per_cluster;

	size = 0;
	for (ob = player; ob ; ob=ob->next)
	  size += sizeof(*ob);
	size += sizeof(nullobj);

	size += sizeof(gamestate) +
			sizeof(LRstruct)*8 +
			sizeof(tilemap) +
			sizeof(actorat) +
			sizeof(laststatobj) +
			sizeof(statobjlist) +
			sizeof(doorposition) +
			sizeof(pwallstate) +
			sizeof(pwallx) +
			sizeof(pwally) +
			sizeof(pwalldir) +
			sizeof(pwallpos);

	if (avail < size)
	{
	 Message(STR_NOSPACE1"\n"
			 STR_NOSPACE2);
	 return false;
	}

	checksum = 0;

	DiskFlopAnim(x,y);

	checksum = fwrite_with_checksum(&gamestate, sizeof(gamestate), 1, file_ptr, checksum);

	DiskFlopAnim(x,y);
#ifdef SPEAR
	checksum = fwrite_with_checksum(&LevelRatios[0], sizeof(LRstruct), 20, file_ptr, checksum);
#else
	checksum = fwrite_with_checksum(&LevelRatios[0], sizeof(LRstruct), 8, file_ptr, checksum);
#endif

	DiskFlopAnim(x,y);
	checksum = fwrite_with_checksum(tilemap, sizeof(tilemap), 1, file_ptr, checksum);
	DiskFlopAnim(x,y);
	
	checksum = fwrite_with_checksum(actorat, sizeof(actorat), 1, file_ptr, checksum);

	fwrite(areaconnect, sizeof(areaconnect), 1, file_ptr);
	fwrite(areabyplayer, sizeof(areabyplayer), 1, file_ptr);

	for (ob = player ; ob ; ob=ob->next)
	{
		DiskFlopAnim(x,y);
		frwite(ob, sizeof(*ob), 1, file_ptr);
	}
	nullobj.active = ac_badobject;          // end of file marker
	DiskFlopAnim(x,y);
	fwrite(&nullobj, sizeof(nullobj), 1, file_ptr);


	DiskFlopAnim(x,y);
	checksum = fwrite_with_checksum(&laststatobj, sizeof(laststatobj), 1, file_ptr, checksum);
	DiskFlopAnim(x,y);
	checksum = fwrite_with_checksum(&statobjlist, sizeof(statobjlist), 1, file_ptr, checksum);

	DiskFlopAnim(x,y);
	checksum = fwrite_with_checksum(&doorposition, sizeof(doorposition), 1, file_ptr, checksum);
	DiskFlopAnim(x,y);
	checksum = fwrite_with_checksum(&doorobjlist, sizeof(doorobjlist), 1, file_ptr, checksum);

	DiskFlopAnim(x,y);
	checksum = fwrite_with_checksum(&pwallstate, sizeof(pwallstate), 1, file_ptr, checksum);
	checksum = fwrite_with_checksum(&pwallx, sizeof(pwallx), 1, file_ptr, checksum);
	checksum = fwrite_with_checksum(&pwally, sizeof(pwally), 1, file_ptr, checksum);
	checksum = fwrite_with_checksum(&pwalldir, sizeof(pwalldir), 1, file_ptr, checksum);
	checksum = fwrite_with_checksum(&pwallpos, sizeof(pwallpos), 1, file_ptr, checksum);
	

	//
	// WRITE OUT CHECKSUM
	//
	fwrite(&checksum, sizeof(checksum), 1, file_ptr);

	return(true);
}

//===========================================================================

/*
==================
=
= LoadTheGame
=
==================
*/

boolean LoadTheGame(File *file_ptr,int x,int y)
{
	long checksum,oldchecksum;
	objtype *ob,nullobj;

	checksum = 0;

	DiskFlopAnim(x,y);
	checksum = fread_with_checksum(&gamestate, sizeof(gamestate), 1, file_ptr, checksum);

	DiskFlopAnim(x,y);
#ifdef SPEAR
	checksum = fread_with_checksum(&LevelRatios[0], sizeof(LRstruct), 20, file_ptr, checksum);
#else
	checksum = fread_with_checksum(&LevelRatios[0], sizeof(LRstruct), 8, file_ptr, checksum);
#endif

	DiskFlopAnim(x,y);
	SetupGameLevel ();

	DiskFlopAnim(x,y);
	checksum = fread_with_checksum(tilemap, sizeof(tilemap), 1, file_ptr, checksum);
	DiskFlopAnim(x,y);
	checksum = fread_with_checksum(actorat, sizeof(actorat), 1, file_ptr, checksum);

	fread(areaconnect, sizeof(areaconnect), 1, file_ptr);
	fread(areabyplayer, sizeof(areabyplayer), 1, file_ptr);

	InitActorList ();
	DiskFlopAnim(x,y);
	fread(player, sizeof(*player), 1, file_ptr);

	while (1)
	{
	 	DiskFlopAnim(x,y);
		fread(&nullobj, sizeof(nullobj), 1, file_ptr);
		if (nullobj.active == ac_badobject)
			break;
		GetNewActor();
	 	// don't copy over the links
		memcpy (new,&nullobj,sizeof(nullobj)-4);
	}



	DiskFlopAnim(x,y);
	checksum = fread_with_checksum(&laststatobj, sizeof(laststatobj), 1, file_ptr, checksum);
	DiskFlopAnim(x,y);
	checksum = fread_with_checksum(statobjlist, sizeof(statobjlist), 1, file_ptr, checksum);

	DiskFlopAnim(x,y);
	checksum = fread_with_checksum(doorposition, sizeof(doorposition), 1, file_ptr, checksum);
	DiskFlopAnim(x,y);
	checksum = fread_with_checksum(doorobjlist, sizeof(doorobjlist), 1, file_ptr, checksum);

	DiskFlopAnim(x,y);
	checksum = fread_with_checksum(&pwallstate, sizeof(pwallstate), 1, file_ptr, checksum);
	checksum = fread_with_checksum(&pwallx, sizeof(pwallx), 1, file_ptr, checksum);
	checksum = fread_with_checksum(&pwally, sizeof(pwally), 1, file_ptr, checksum);
	checksum = fread_with_checksum(&pwalldir, sizeof(pwalldir), 1, file_ptr, checksum);
	checksum = fread_with_checksum(&pwallpos, sizeof(pwallpos), 1, file_ptr, checksum);

	fread(&oldchecksum, sizeof(oldchecksum), 1, file_ptr);

	if (oldchecksum != checksum)
	{
	 Message(STR_SAVECHT1"\n"
			 STR_SAVECHT2"\n"
			 STR_SAVECHT3"\n"
			 STR_SAVECHT4);

	 IN_ClearKeysDown();
	 IN_Ack();

	 gamestate.score = 0;
	 gamestate.lives = 1;
	 gamestate.weapon =
	   gamestate.chosenweapon =
	   gamestate.bestweapon = wp_pistol;
	 gamestate.ammo = 8;
	}

	return true;
}

//===========================================================================

/*
==========================
=
= ShutdownId
=
= Shuts down all ID_?? managers
=
==========================
*/

void ShutdownId (void)
{
	US_Shutdown ();
	SD_Shutdown ();
	IN_Shutdown ();
}


//===========================================================================

/*
==================
=
= BuildTables
=
= Calculates:
=
= scale                 projection constant
= sintable/costable     overlapping fractional tables
=
==================
*/

const   float   radtoint = (float)FINEANGLES/2/PI;

void BuildTables (void)
{
  int           i;
  float         angle,anglestep;
  double        tang;
  fixed         value;


//
// calculate fine tangents
//

	for (i=0;i<FINEANGLES/8;i++)
	{
		tang = tan( (i+0.5)/radtoint);
		finetangent[i] = tang*TILEGLOBAL;
		finetangent[FINEANGLES/4-1-i] = 1/tang*TILEGLOBAL;
	}

//
// costable overlays sintable with a quarter phase shift
// ANGLES is assumed to be divisable by four
//
// The low word of the value is the fraction, the high bit is the sign bit,
// bits 16-30 should be 0
//

  angle = 0;
  anglestep = PI/2/ANGLEQUAD;
  for (i=0;i<=ANGLEQUAD;i++)
  {
	value=GLOBAL1*sin(angle);
	sintable[i]=
	  sintable[i+ANGLES]=
	  sintable[ANGLES/2-i] = value;
	sintable[ANGLES-i]=
	  sintable[ANGLES/2+i] = value | 0x80000000l;
	angle += anglestep;
  }

}

//===========================================================================


/*
====================
=
= CalcProjection
=
= Uses focallength
=
====================
*/

void CalcProjection (long focal)
{
	int             i;
	long            intang;
	float   angle;
	double  tang;
	double  planedist;
	double  globinhalf;
	int             halfview;
	double  halfangle,facedist;


	focallength = focal;
	facedist = focal+MINDIST;
	halfview = viewwidth/2;                                 // half view in pixels

//
// calculate scale value for vertical height calculations
// and sprite x calculations
//
	scale = halfview*facedist/(VIEWGLOBAL/2);

//
// divide heightnumerator by a posts distance to get the posts height for
// the heightbuffer.  The pixel height is height>>2
//
	heightnumerator = (TILEGLOBAL*scale)>>6;
	minheightdiv = heightnumerator/0x7fff +1;

//
// calculate the angle offset from view angle of each pixel's ray
//

	for (i=0;i<halfview;i++)
	{
	// start 1/2 pixel over, so viewangle bisects two middle pixels
		tang = (long)i*VIEWGLOBAL/viewwidth/facedist;
		angle = atan(tang);
		intang = angle*radtoint;
		pixelangle[halfview-1-i] = intang;
		pixelangle[halfview+i] = -intang;
	}

//
// if a point's abs(y/x) is greater than maxslope, the point is outside
// the view area
//
	maxslope = finetangent[pixelangle[0]];
	maxslope >>= 8;
}



//===========================================================================

/*
===================
=
= SetupWalls
=
= Map tile values to scaled pics
=
===================
*/

void SetupWalls (void)
{
	int     i;

	for (i=1;i<MAXWALLTILES;i++)
	{
		horizwall[i]=(i-1)*2;
		vertwall[i]=(i-1)*2+1;
	}
}

//===========================================================================

/*
==========================
=
= SignonScreen
=
==========================
*/

void SignonScreen (void)                        // VGA version
{
	unsigned        segstart,seglength;

	if (!virtualreality)
	{
		VL_MungePic (&introscn,320,200);
		VL_MemToScreen (&introscn,320,200,0,0);
	}
}


/*
==========================
=
= FinishSignon
=
==========================
*/

void FinishSignon (void)
{

#ifndef SPEAR
	VW_Bar (0,189,300,11,peekb(0xa000,0));
	WindowX = 0;
	WindowW = 320;
	PrintY = 190;

	#ifndef JAPAN
	SETFONTCOLOR(14,4);

	#ifdef SPANISH
	US_CPrint ("Oprima una tecla");
	#else
	US_CPrint ("Press a key");
	#endif

	#endif

	if (!NoWait)
		IN_Ack ();

	#ifndef JAPAN
	VW_Bar (0,189,300,11,peekb(0xa000,0));

	PrintY = 190;
	SETFONTCOLOR(10,4);

	#ifdef SPANISH
	US_CPrint ("pensando...");
	#else
	US_CPrint ("Working...");
	#endif

	#endif

	SETFONTCOLOR(0,15);
#endif
}

//===========================================================================

/*
=================
=
= MS_CheckParm
=
=================
*/

boolean MS_CheckParm (char far *check)
{
	int             i;
	char    *parm;

	for (i = 1;i<_argc;i++)
	{
		parm = _argv[i];

		while ( !isalpha(*parm) )       // skip - / \ etc.. in front of parm
			if (!*parm++)
				break;                          // hit end of string without an alphanum

		if ( !_fstricmp(check,parm) )
			return true;
	}

	return false;
}

//===========================================================================

/*
=====================
=
= InitDigiMap
=
=====================
*/

static  int     wolfdigimap[] =
		{
			// These first sounds are in the upload version
#ifndef SPEAR
			HALTSND,                0,
			DOGBARKSND,             1,
			CLOSEDOORSND,           2,
			OPENDOORSND,            3,
			ATKMACHINEGUNSND,       4,
			ATKPISTOLSND,           5,
			ATKGATLINGSND,          6,
			SCHUTZADSND,            7,
			GUTENTAGSND,            8,
			MUTTISND,               9,
			BOSSFIRESND,            10,
			SSFIRESND,              11,
			DEATHSCREAM1SND,        12,
			DEATHSCREAM2SND,        13,
			DEATHSCREAM3SND,        13,
			TAKEDAMAGESND,          14,
			PUSHWALLSND,            15,

			LEBENSND,               20,
			NAZIFIRESND,            21,
			SLURPIESND,             22,

			YEAHSND,				32,

#ifndef UPLOAD
			// These are in all other episodes
			DOGDEATHSND,            16,
			AHHHGSND,               17,
			DIESND,                 18,
			EVASND,                 19,

			TOT_HUNDSND,            23,
			MEINGOTTSND,            24,
			SCHABBSHASND,           25,
			HITLERHASND,            26,
			SPIONSND,               27,
			NEINSOVASSND,           28,
			DOGATTACKSND,           29,
			LEVELDONESND,           30,
			MECHSTEPSND,			31,

			SCHEISTSND,				33,
			DEATHSCREAM4SND,		34,		// AIIEEE
			DEATHSCREAM5SND,		35,		// DEE-DEE
			DONNERSND,				36,		// EPISODE 4 BOSS DIE
			EINESND,				37,		// EPISODE 4 BOSS SIGHTING
			ERLAUBENSND,			38,		// EPISODE 6 BOSS SIGHTING
			DEATHSCREAM6SND,		39,		// FART
			DEATHSCREAM7SND,		40,		// GASP
			DEATHSCREAM8SND,		41,		// GUH-BOY!
			DEATHSCREAM9SND,		42,		// AH GEEZ!
			KEINSND,				43,		// EPISODE 5 BOSS SIGHTING
			MEINSND,				44,		// EPISODE 6 BOSS DIE
			ROSESND,				45,		// EPISODE 5 BOSS DIE

#endif
#else
//
// SPEAR OF DESTINY DIGISOUNDS
//
			HALTSND,                0,
			CLOSEDOORSND,           2,
			OPENDOORSND,            3,
			ATKMACHINEGUNSND,       4,
			ATKPISTOLSND,           5,
			ATKGATLINGSND,          6,
			SCHUTZADSND,            7,
			BOSSFIRESND,            8,
			SSFIRESND,              9,
			DEATHSCREAM1SND,        10,
			DEATHSCREAM2SND,        11,
			TAKEDAMAGESND,          12,
			PUSHWALLSND,            13,
			AHHHGSND,               15,
			LEBENSND,               16,
			NAZIFIRESND,            17,
			SLURPIESND,             18,
			LEVELDONESND,           22,
			DEATHSCREAM4SND,		23,		// AIIEEE
			DEATHSCREAM3SND,        23,		// DOUBLY-MAPPED!!!
			DEATHSCREAM5SND,		24,		// DEE-DEE
			DEATHSCREAM6SND,		25,		// FART
			DEATHSCREAM7SND,		26,		// GASP
			DEATHSCREAM8SND,		27,		// GUH-BOY!
			DEATHSCREAM9SND,		28,		// AH GEEZ!
			GETGATLINGSND,			38,		// Got Gat replacement

#ifndef SPEARDEMO
			DOGBARKSND,             1,
			DOGDEATHSND,            14,
			SPIONSND,               19,
			NEINSOVASSND,           20,
			DOGATTACKSND,           21,
			TRANSSIGHTSND,			29,		// Trans Sight
			TRANSDEATHSND,			30,		// Trans Death
			WILHELMSIGHTSND,		31,		// Wilhelm Sight
			WILHELMDEATHSND,		32,		// Wilhelm Death
			UBERDEATHSND,			33,		// Uber Death
			KNIGHTSIGHTSND,			34,		// Death Knight Sight
			KNIGHTDEATHSND,			35,		// Death Knight Death
			ANGELSIGHTSND,			36,		// Angel Sight
			ANGELDEATHSND,			37,		// Angel Death
			GETSPEARSND,			39,		// Got Spear replacement
#endif
#endif
			LASTSOUND
		};


void InitDigiMap (void)
{
	int                     *map;

	for (map = wolfdigimap;*map != LASTSOUND;map += 2)
		DigiMap[map[0]] = map[1];


}


#ifndef SPEAR
CP_iteminfo	MusicItems={CTL_X,CTL_Y,6,0,32};
CP_itemtype far MusicMenu[]=
	{
		{1,"Get Them!",0},
		{1,"Searching",0},
		{1,"P.O.W.",0},
		{1,"Suspense",0},
		{1,"War March",0},
		{1,"Around The Corner!",0},

		{1,"Nazi Anthem",0},
		{1,"Lurking...",0},
		{1,"Going After Hitler",0},
		{1,"Pounding Headache",0},
		{1,"Into the Dungeons",0},
		{1,"Ultimate Conquest",0},

		{1,"Kill the S.O.B.",0},
		{1,"The Nazi Rap",0},
		{1,"Twelfth Hour",0},
		{1,"Zero Hour",0},
		{1,"Ultimate Conquest",0},
		{1,"Wolfpack",0}
	};
#else
CP_iteminfo MusicItems={CTL_X,CTL_Y-20,9,0,32};
CP_itemtype far MusicMenu[]=
   {
		{1,"Funky Colonel Bill",0},
		{1,"Death To The Nazis",0},
		{1,"Tiptoeing Around",0},
		{1,"Is This THE END?",0},
		{1,"Evil Incarnate",0},
		{1,"Jazzin' Them Nazis",0},
		{1,"Puttin' It To The Enemy",0},
		{1,"The SS Gonna Get You",0},
		{1,"Towering Above",0}
	};
#endif

#ifndef SPEARDEMO
void DoJukebox(void)
{
	int which,lastsong=-1;
	unsigned start,songs[]=
		{
#ifndef SPEAR
			GETTHEM_MUS,
			SEARCHN_MUS,
			POW_MUS,
			SUSPENSE_MUS,
			WARMARCH_MUS,
			CORNER_MUS,

			NAZI_OMI_MUS,
			PREGNANT_MUS,
			GOINGAFT_MUS,
			HEADACHE_MUS,
			DUNGEON_MUS,
			ULTIMATE_MUS,

			INTROCW3_MUS,
			NAZI_RAP_MUS,
			TWELFTH_MUS,
			ZEROHOUR_MUS,
			ULTIMATE_MUS,
			PACMAN_MUS
#else
			XFUNKIE_MUS,             // 0
			XDEATH_MUS,              // 2
			XTIPTOE_MUS,             // 4
			XTHEEND_MUS,             // 7
			XEVIL_MUS,               // 17
			XJAZNAZI_MUS,            // 18
			XPUTIT_MUS,              // 21
			XGETYOU_MUS,             // 22
			XTOWER2_MUS              // 23
#endif
		};
	struct dostime_t time;



	IN_ClearKeysDown();
	if (!AdLibPresent && !SoundBlasterPresent)
		return;


	MenuFadeOut();

#ifndef SPEAR
#ifndef UPLOAD
	_dos_gettime(&time);
	start = (time.hsecond%3)*6;
#else
	start = 0;
#endif
#else
	start = 0;
#endif

	fontnumber=1;
	ClearMScreen ();
	R_DrawPic(112,184,C_MOUSELBACKPIC);
	DrawStripes (10);
	SETFONTCOLOR (TEXTCOLOR,BKGDCOLOR);

#ifndef SPEAR
	DrawWindow (CTL_X-2,CTL_Y-6,280,13*7,BKGDCOLOR);
#else
	DrawWindow (CTL_X-2,CTL_Y-26,280,13*10,BKGDCOLOR);
#endif

	DrawMenu (&MusicItems,&MusicMenu[start]);

	SETFONTCOLOR (READHCOLOR,BKGDCOLOR);
	PrintY=15;
	WindowX = 0;
	WindowY = 320;
	US_CPrint ("Robert's Jukebox");

	SETFONTCOLOR (TEXTCOLOR,BKGDCOLOR);
	MenuFadeIn();

	do
	{
		which = HandleMenu(&MusicItems,&MusicMenu[start],NULL);
		if (which>=0)
		{
			if (lastsong >= 0)
				MusicMenu[start+lastsong].active = 1;

			StartCPMusic(songs[start + which]);
			MusicMenu[start+which].active = 2;
			DrawMenu (&MusicItems,&MusicMenu[start]);
			lastsong = which;
		}
	} while(which>=0);

	MenuFadeOut();
	IN_ClearKeysDown();
}
#endif


/*
==========================
=
= InitGame
=
= Load a few things right away
=
==========================
*/

void InitGame (void)
{
	int                     i,x,y;
	unsigned        *blockstart;

	if (MS_CheckParm ("virtual"))
		virtualreality = true;
	else
		virtualreality = false;

	SignonScreen ();

	// initialize asset manager + load and cache
	// media assets for game
	AM_SetupAssets();

	IN_Startup ();
	SD_Startup ();
	US_Startup ();


//
// build some tables
//
	InitDigiMap ();

	for (i=0;i<MAPSIZE;i++)
	{
		nearmapylookup[i] = &tilemap[0][0]+MAPSIZE*i;
		farmapylookup[i] = i*64;
	}

	for (i=0;i<PORTTILESHIGH;i++)
		uwidthtable[i] = UPDATEWIDE*i;

	blockstart = &blockstarts[0];
	for (y=0;y<UPDATEHIGH;y++)
		for (x=0;x<UPDATEWIDE;x++)
			*blockstart++ = SCREENWIDTH*16*y+x*TILEWIDTH;

	updateptr = &update[0];

	bufferofs = 0;
	displayofs = 0;
	ReadConfig ();


//
// HOLDING DOWN 'M' KEY?
//
#ifndef SPEARDEMO
	if (Keyboard[sc_M])
	  DoJukebox();
	else
#endif
//
// draw intro screen stuff
//
	if (!virtualreality)
		IntroScreen ();

//
// load in and lock down some basic chunks
//
	BuildTables ();          // trig tables
	SetupWalls ();

#if 0
{
int temp,i;
temp = viewsize;
	profilehandle = open("SCALERS.TXT", O_CREAT | O_WRONLY | O_TEXT);
for (i=1;i<20;i++)
	NewViewSize(i);
viewsize = temp;
close(profilehandle);
}
#endif

	NewViewSize (viewsize);


//
// initialize variables
//
	if (!virtualreality)
		FinishSignon();

	displayofs = PAGE1START;
	bufferofs = PAGE2START;

	if (virtualreality)
	{
		NoWait = true;
		geninterrupt(0x60);
	}

	// init renderer
	// TODO: make sure this is in the right spot
	R_SetPalette(gamepal);

}

//===========================================================================

/*
==========================
=
= SetViewSize
=
==========================
*/

boolean SetViewSize (unsigned width, unsigned height)
{
	viewwidth = width&~15;                  // must be divisable by 16
	viewheight = height&~1;                 // must be even
	centerx = viewwidth/2-1;
	shootdelta = viewwidth/10;
	screenofs = ((200-STATUSLINES-viewheight)/2*SCREENWIDTH+(320-viewwidth)/8);

//
// calculate trace angles and projection constants
//
	CalcProjection (FOCALLENGTH);

//
// build all needed compiled scalers
//
	SetupScaling (viewwidth*1.5);
	return true;
}


void ShowViewSize (int width)
{
	int     oldwidth,oldheight;

	oldwidth = viewwidth;
	oldheight = viewheight;

	viewwidth = width*16;
	viewheight = width*16*HEIGHTRATIO;
	DrawPlayBorder ();

	viewheight = oldheight;
	viewwidth = oldwidth;
}


void NewViewSize (int width)
{
	viewsize = width;
	SetViewSize (width*16,width*16*HEIGHTRATIO);
}



//===========================================================================

/*
==========================
=
= Quit
=
==========================
*/

/// ✅
void Quit (char *error)
{
	ShutdownId ();

	AM_FreeAssets();
	R_Shutdown();

	exit(0);
}

//===========================================================================



/*
=====================
=
= DemoLoop
=
=====================
*/

static  char *ParmStrings[] = {"baby","easy","normal","hard",""};

void    DemoLoop (void)
{
	static int LastDemo;
	int     i,level;
	long nsize;
	void*	nullblock;

//
// check for launch from ted
//
	if (tedlevel)
	{
		NoWait = true;
		NewGame(1,0);

		for (i = 1;i < _argc;i++)
		{
			if ( (level = US_CheckParm(_argv[i],ParmStrings)) != -1)
			{
			 gamestate.difficulty=level;
			 break;
			}
		}

#ifndef SPEAR
		gamestate.episode = tedlevelnum/10;
		gamestate.mapon = tedlevelnum%10;
#else
		gamestate.episode = 0;
		gamestate.mapon = tedlevelnum;
#endif
		GameLoop();
		Quit (NULL);
	}


//
// main game cycle
//


#ifndef DEMOTEST

	#ifndef UPLOAD

		#ifndef GOODTIMES
		#ifndef SPEAR
		#ifndef JAPAN
		if (!NoWait)
			NonShareware();
		#endif
		#else

			#ifndef GOODTIMES
			#ifndef SPEARDEMO
			CopyProtection();
			#endif
			#endif

		#endif
		#endif
	#endif

	StartCPMusic(INTROSONG);

#ifndef JAPAN
	if (!NoWait)
		PG13 ();
#endif

#endif

	while (1)
	{
		while (!NoWait)
		{
//
// title page
//
#ifndef DEMOTEST

#ifdef SPEAR
			R_DrawPic (0,0,TITLE1PIC);

			R_DrawPic (0,80,TITLE2PIC);
			VL_FadeIn(0,255,AM_GetGraphicsAsset(TITLEPALETTE),30);
#else
			VW_FadeIn();
#endif
			if (IN_UserInput(TickBase*15))
				break;
			VW_FadeOut();
//
// credits page
//
			VW_FadeIn ();
			if (IN_UserInput(TickBase*10))
				break;
			VW_FadeOut ();
//
// high scores
//
			DrawHighScores ();
			VW_FadeIn ();

			if (IN_UserInput(TickBase*10))
				break;
#endif
//
// demo
//

			#ifndef SPEARDEMO
			PlayDemo (LastDemo++%4);
			#else
			PlayDemo (0);
			#endif

			if (playstate == ex_abort)
				break;
			StartCPMusic(INTROSONG);
		}

		VW_FadeOut ();

#ifndef SPEAR
		if (Keyboard[sc_Tab] && MS_CheckParm("goobers"))
#else
		if (Keyboard[sc_Tab] && MS_CheckParm("debugmode"))
#endif
			RecordDemo ();
		else
			US_ControlPanel (0);

		if (startgame || loadedgame)
		{
			GameLoop ();
			VW_FadeOut();
			StartCPMusic(INTROSONG);
		}
	}
}


//===========================================================================


/*
==========================
=
= main
=
==========================
*/

char    *nosprtxt[] = {"nospr",nil};

void main (void)
{
	CheckForEpisodes();

	if (R_Startup("Wolfenstein 3D", 3) != 0) {
		fprintf(stderr, "Error initializing renderer: %s\n", R_GetError()));
		return 1;
	}

	// do this in game init
	InitGame ();

	DemoLoop();

	Quit("Demo loop exited???");
}

