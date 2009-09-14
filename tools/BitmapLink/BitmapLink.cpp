// BitmapLink.cpp : Defines the entry point for the console application.
//

/**
 *  BitmapLink - программа для соединения последовательности
 *               bmp файлов в один для использования в анимации
 *  version 1.1  меньше размер кода. русский шрифт в консоли. обработка ошибок.
 *  version 2.0  поддержка формата tga.
**/


#include <windows.h>
#include <stdio.h>
#include <math.h>


int frameW = 0, frameH = 0;


struct tga_header
{
	char   IdLeight;	//   Длина текстовой информации после первого 
	char   ColorMap;	//   Идентификатор наличия цветовой карты - устарел 
	char   DataType;	//   Тип данных - запакованный или нет 
	char   ColorMapInfo[5];	//   Информация о цветовой карте - нужно пропустить эти 5 байт 
	short  x_origin;	//   Начало изображения по оси X 
	short  y_origin;	//   Начало изображения по оси Y 
	short  width;		//   Ширина изображения 
	short  height;		//   Высота изображения 
	char   BitPerPel;	//   Кол-во бит на пиксель - здесь только 24 или 32 
	char   Description;	//   Описание - пропускайте 
} header;

bool ReadBitmap(char* cFileName, RGBQUAD **pBits)
{
	HANDLE	hBMP_File = CreateFile(	cFileName, 
									GENERIC_READ, 
									FILE_SHARE_READ, 
									NULL, 
									OPEN_EXISTING, 
									FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,
									NULL);
	
	DWORD nBytesRead = 0;

	ReadFile(hBMP_File, &header, sizeof(header), &nBytesRead, NULL );
	
	frameW = header.width;
	frameH = header.height;
	
	if ( header.BitPerPel != 32 ) 
	{
		CloseHandle(hBMP_File);
		printf("only 32 bit tga are supported\n");
		return false;
	}
	
	*pBits = new RGBQUAD[ header.width * header.height ];	
	
	SetFilePointer(hBMP_File, header.IdLeight, NULL, FILE_CURRENT);
	ReadFile(hBMP_File, *pBits, header.width * header.height * sizeof(RGBQUAD), &nBytesRead, NULL );
	
	CloseHandle(hBMP_File);

	return true;
}

BOOL SelectFile (char *pFileName) 
{
	OPENFILENAME of = { sizeof(OPENFILENAME) };

	of.lpstrFilter = "Targa Image Files (*.tga)\0*.tga\0\0";
	of.lpstrDefExt = "tga";
	of.lpstrFile   = pFileName;
	of.nMaxFile    = 16384;
	of.lpstrTitle  = "Выбрать картинки";
	of.Flags       = OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_ALLOWMULTISELECT;
	
	return GetOpenFileName (&of);	
}


int main(int argc, char* argv[])
{
	//определяем список исходных файлов
	char pFileName[16384] = {0};
	if ( !SelectFile(pFileName) ) 
	{
		printf("canceled\n");
		return 0;
	}
	printf("%s\n", pFileName);

	//считаем количество файлов в списке
	int nFrames = 0;
	for(int n = 0; pFileName[n]; n++)
	{
		if (pFileName[n] == ' ') 
			nFrames++;
	}

	if (0 == nFrames)
	{
		printf("there are no files selected\n");
		return -1;
	}


	char *pNextFile = strstr(pFileName, " ");

	//определяем директорию
	char *dir = pFileName;
	*pNextFile = '\0';
	int res = SetCurrentDirectory(dir);
	CharToOem(dir, dir);
	printf("Current directory is '%s'\n", dir);


	// оптимальное кол-во кадров по X и по Y
	int n_frames_x = int( sqrt((double) nFrames) ) - 1;
	int n_frames_y;
	do {
		n_frames_y = nFrames / (++n_frames_x);
	} while (n_frames_x * n_frames_y != nFrames);


	//буфер финального изображения
	RGBQUAD *OutBits = NULL;

	//читаем данные
	for (int nCurFrame = 0; pNextFile; nCurFrame++)
	{
		///////////////////////////////////////////////////////
		//получаем очередное имя файла
		char *pCurFile = pNextFile + 1;
		pNextFile = strstr(pCurFile, " ");
		if (pNextFile) *pNextFile = '\0';

		//буфер кадра
		RGBQUAD *pFrameBits = NULL;

		//читаем в буфер картинку
		if (!ReadBitmap(pCurFile, &pFrameBits))
		{
			fprintf(stderr, "error reading '%s'\n", pCurFile);
			break;
		}
	
		if (NULL == OutBits)
			OutBits = new RGBQUAD[frameH*frameW*nFrames];

		//переписываем кадр в конечный буфер
		for(int y = 0; y < frameH; y++)
		for(int x = 0; x < frameW; x++)
		{
			int cur_frame_x = nCurFrame % n_frames_x;
			int cur_frame_y = nCurFrame / n_frames_x;

			OutBits[cur_frame_x * frameW + x + ((n_frames_y - cur_frame_y - 1) * frameH + y) * (frameW * n_frames_x)] = 
				pFrameBits[x + y * frameW];
		}

		delete[] pFrameBits;


		char str[MAX_PATH + 64];
		sprintf(str, "frame#%d = '%s'\n", nCurFrame, pCurFile);
		CharToOem(str, str);
		printf(str);
	}

	//////////////////////////////////////////////////////////////////////////
	//пишем полученное изображение в файл

	pFileName[0] = '\0';
	OPENFILENAME ofn = { sizeof(OPENFILENAME) };
	ofn.lpstrFilter = "Targa Image Files (*.tga)\0*.tga\0\0";
	ofn.lpstrDefExt = "tga";
	ofn.lpstrFile   = pFileName;
	ofn.nMaxFile    = MAX_PATH;
	ofn.lpstrTitle  = "Save as";
	ofn.Flags       = OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_OVERWRITEPROMPT;
	if(GetOpenFileName(&ofn))
	{
		HANDLE	hBMP_File = CreateFile(	pFileName, 
										GENERIC_WRITE, 
										FILE_SHARE_READ, 
										NULL, 
										CREATE_ALWAYS, 
										FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,
										NULL);
		DWORD nBytesWritten = 0;
		
		header.width  = frameW * n_frames_x;
		header.height = frameH * n_frames_y;
		header.IdLeight = 0;

		WriteFile(hBMP_File, &header, sizeof(header), &nBytesWritten, NULL );
		WriteFile(hBMP_File, OutBits, header.width*header.height*sizeof(RGBQUAD), &nBytesWritten, NULL );
	
		CloseHandle(hBMP_File);
	}
	delete[] OutBits;

	return 0;
}
