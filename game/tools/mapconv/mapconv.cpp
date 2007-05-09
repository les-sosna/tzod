// mapconv.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../Tank/SaveFile.h"


#define I_ERROR { printf("invalid input map\n"); fclose(in); continue; }
#define O_ERROR { printf("error writing output file\n"); fclose(in); continue; }

int _tmain(int argc, _TCHAR* argv[])
{
	WIN32_FIND_DATA wfd;
	HANDLE hSearch = FindFirstFile("*.*", &wfd);
	if( INVALID_HANDLE_VALUE == hSearch )
	{
		printf("no files found\n");
		return -1;
	}

    do
	{
		FILE *in = fopen(wfd.cFileName, "r");
		if( NULL == in )
			continue;


		int width  = 0;
		int height = 0;

		char object[1024] = {0};

		// проверяем заголовок файла
		fgets(object, 32, in);
		object[strlen(object) - 1] = 0;	// отрезать символ переноса строки
		if (0 != strcmp(object, "<tank map>"))
		{
			fclose(in);
			continue;
		}

		bool old = false;

		// запоминаем позицию в файле
		long fpos = ftell(in);

		// пытаемся получить размеры уровня
		try {
			char tmp[32] = {0};

			fgets(object, 32, in);
			object[strlen(object) - 1] = 0;	// отрезать символ переноса строки
			if( 2 != sscanf(object, "%s%d", tmp, &width) ) throw -1;
			if( 0 != strcmp(tmp, "width") ) throw -1;

			fgets(object, 32, in);
			object[strlen(object) - 1] = 0;	// отрезать символ переноса строки
			if( 2 != sscanf(object, "%s%d", tmp, &height) ) throw -1;
			if( 0 != strcmp(tmp, "height") ) throw -1;

			if( width == 0 || height == 0 )
				throw -1;
		}
		catch (int)
		{
			fseek(in, fpos, SEEK_SET);
			width  = 32;
			height = 24;
			old    = true;
			printf("%s:warning: old map format\n", wfd.cFileName);
		}


		char outfn[MAX_PATH];
		sprintf(outfn, "%s.map", wfd.cFileName);


		MapFile out;
		out.setMapAttribute("width",  width);
		out.setMapAttribute("height", height);
		out.setMapAttribute("type",  "deathmatch");

		if( !out.Open(outfn, true) )
		{
			printf("couldn't open output file '%s' for writing\n", outfn);
			fclose(in);
			continue;
		}

		if( old )
		{
			for (int x = 0; x < 32; x++)
			{
				out.BeginObject("wall_concrete");
				out.setObjectAttribute("x", (float) x * 32 + 16);
				out.setObjectAttribute("y", (float) -32 + 16);
				out.WriteCurrentObject();

				out.BeginObject("wall_concrete");
				out.setObjectAttribute("x", (float) x * 32 + 16);
				out.setObjectAttribute("y", (float) 32 * 24 + 16);
				out.WriteCurrentObject();
			}

			for (int y = -1; y < 24 + 1; y++)
			{
				out.BeginObject("wall_concrete");
				out.setObjectAttribute("x", (float) -32 + 16);
				out.setObjectAttribute("y", (float) y * 32 + 16);
				out.WriteCurrentObject();

				out.BeginObject("wall_concrete");
				out.setObjectAttribute("x", (float) 32 * 32 + 16);
				out.setObjectAttribute("y", (float) y * 32 + 16);
				out.WriteCurrentObject();
			}
		}

		while( 0 < fscanf(in, "%s", object) )
		{
			int x, y;
			float a;

			if (0 == strcmp(object, "WALL"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				x -= (x%32)-16;
				y -= (y%32)-16;
				out.BeginObject("wall_brick");
			}
			else if (0 == strcmp(object, "WALL_CONCRETE"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				x -= (x%32)-16;
				y -= (y%32)-16;
				out.BeginObject("wall_concrete");
			}
			else if (0 == strcmp(object, "WOOD"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				x -= (x%32)-16;
				y -= (y%32)-16;
				out.BeginObject("wood");
			}
			else if (0 == strcmp(object, "WATER"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				x -= (x%32)-16;
				y -= (y%32)-16;
				out.BeginObject("water");
			}
			else if (0 == strcmp(object, "TURET_ROCKET"))
			{
				if (3 != fscanf(in, "%d %d %f", &x, &y, &a)) I_ERROR;
				out.BeginObject("turret_rocket");
				out.setObjectAttribute("dir", a);
			}
			else if (0 == strcmp(object, "TURET_CANON"))
			{
				if (3 != fscanf(in, "%d %d %f", &x, &y, &a)) I_ERROR;
				out.BeginObject("turret_cannon");
				out.setObjectAttribute("dir", a);
			}
			else if (0 == strcmp(object, "TURET_SMART"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				out.BeginObject("turret_minigun");
			}
			else if (0 == strcmp(object, "TURET_GAUSS"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				out.BeginObject("turret_gauss");
			}
			else if (0 == strcmp(object, "HEALTH"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				out.BeginObject("pu_health");
			}
			else if (0 == strcmp(object, "INV"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				out.BeginObject("pu_shield");
			}
			else if (0 == strcmp(object, "SHOCK"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				out.BeginObject("pu_shock");
			}
			else if (0 == strcmp(object, "BUSTER") || 0 == strcmp(object, "BOOSTER"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				out.BeginObject("pu_booster");
			}
			else if (0 == strcmp(object, "MINE"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				out.BeginObject("pu_mine");
			}
			else if (0 == strcmp(object, "WEAP_PLAZMA"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				out.BeginObject("weap_plazma");
			}
			else if (0 == strcmp(object, "WEAP_AUTOCANNON"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				out.BeginObject("weap_autocannon");
			}
			else if (0 == strcmp(object, "WEAP_ROCKETLAUNCHER"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				out.BeginObject("weap_rockets");
			}
			else if (0 == strcmp(object, "WEAP_CANON"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				out.BeginObject("weap_cannon");
			}
			else if (0 == strcmp(object, "WEAP_GAUSS"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				out.BeginObject("weap_gauss");
			}
			else if (0 == strcmp(object, "WEAP_RAM"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				out.BeginObject("weap_ram");
			}
			else if (0 == strcmp(object, "WEAP_BFG"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				out.BeginObject("weap_bfg");
			}
			else if (0 == strcmp(object, "WEAP_RIPPER"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				out.BeginObject("weap_ripper");
			}
			else if (0 == strcmp(object, "WEAP_MINIGUN"))
			{
				if (2 != fscanf(in, "%d %d", &x, &y)) I_ERROR;
				out.BeginObject("weap_minigun");
			}
			else if (0 == strcmp(object, "SPAWNPOINT"))
			{
				if (3 != fscanf(in, "%d %d %f", &x, &y, &a)) I_ERROR;
				out.BeginObject("respawn_point");
				out.setObjectAttribute("dir", a);
			}
			else 
				I_ERROR;


			out.setObjectAttribute("x", (float) x);
			out.setObjectAttribute("y", (float) y);
			if( !out.WriteCurrentObject() ) O_ERROR;
		}

	} while( FindNextFile(hSearch, &wfd) );
	FindClose(hSearch);

	printf("done");


	return 0;
}

