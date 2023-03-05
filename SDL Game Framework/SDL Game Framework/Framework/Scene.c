#include "stdafx.h"
#include "Scene.h"
#include "csv.h"
#include "Text.h"

#include "Framework.h"

Scene g_Scene;
CsvFile csvFile;


Text CsvText[512][512]; // 여기는 계속 내가 손으로 바꿔줘야함 //여기 크게 하니까 에러 안난다
Text text_buf[TEXT_MAX_LINE];

bool isCreated = false;
bool isActive = false;

static ESceneType s_nextScene = SCENE_NULL;

#pragma region TitleScene

#define SOLID 0
#define SHADED 1
#define BLENDED 2


typedef struct TitleSceneData
{
	Text	GuideLine[10];
	Text	TestText;
	Text	Credit;
	int32	FontSize;
	int32	RenderMode;
	SoundEffect Effect;
	float		Volume;
	Image	TestImage;
} TitleSceneData;

void init_title(void)
{

	g_Scene.Data = malloc(sizeof(TitleSceneData));
	memset(g_Scene.Data, 0, sizeof(TitleSceneData));
	// TITLE SCENE에 들어가는 모든 데이터 크기만큼 g_Scene.data 에 크기를 딱 만들어둠

	TitleSceneData* data = (TitleSceneData*)g_Scene.Data;
	// 이거 g_scene.data가 어딘가에 저장된 data를 가리킬건데 그 포인터의 주솟값을  data에 복붙한거다. 

	for (int r = 0; r < csvFile.RowCount; ++r)
	{
		for (int c = 0; c < csvFile.ColumnCount; ++c)
		{
			wchar_t* str = ParseToUnicode(csvFile.Items[r][c]);
			
			Text_CreateText(&CsvText[r][c], "font1.ttf", 16, str, wcslen(str));
			free(str);
		}
	}

	// 여기까지 csvtext 파일에 샥샥 내용이 다 들어감

	data->FontSize = 40;

	Text_CreateText(&data->TestText, "font1.ttf", data->FontSize, L"GAME START", 11);
	// 여기서는 데이터를 업데이트만 한거고 좌표는 RENDER에서 찍어줘야 한다. 
	Text_CreateText(&data->Credit, "font1.ttf", data->FontSize, L"CREDIT", 11);

	Image_LoadImage(&data->TestImage, "title.png");
	Audio_LoadSoundEffect(&data->Effect, "click1.mp3");


	data->RenderMode = SOLID;
}

void update_title(void)
{
	TitleSceneData* data = (TitleSceneData*)g_Scene.Data;

	if (Input_GetKeyDown(VK_UP))
	{
		Audio_PlaySoundEffect(&data->Effect, 1);
		if (isActive == false)
		{
			Text_SetFontStyle(&data->TestText, FS_BOLD);
			Text_SetFontStyle(&data->Credit, FS_NORMAL);
			isActive = true;
		}
		else
		{
			Text_SetFontStyle(&data->TestText, FS_NORMAL);
			Text_SetFontStyle(&data->Credit, FS_BOLD);
			isActive = false;
		}
	}

	if (Input_GetKeyDown(VK_DOWN))
	{
		Audio_PlaySoundEffect(&data->Effect, 1);
		if (isActive == false)
		{
			Text_SetFontStyle(&data->TestText, FS_BOLD);
			Text_SetFontStyle(&data->Credit, FS_NORMAL);
			isActive = true;
		}
		else
		{
			Text_SetFontStyle(&data->TestText, FS_NORMAL);
			Text_SetFontStyle(&data->Credit, FS_BOLD);
			isActive = false;
		}
	}

	if ((isActive) && Input_GetKeyDown(VK_RETURN))
	{
		Scene_SetNextScene(SCENE_MAIN);
	}

	if ((!isActive) && Input_GetKeyDown(VK_RETURN))
	{
		Scene_SetNextScene(SCENE_CREDIT);
	}

}

void render_title(void)
{
	TitleSceneData* data = (TitleSceneData*)g_Scene.Data;
	SDL_Color color = { .a = 255 };
	for (int r = 0; r < csvFile.RowCount; ++r)
		for (int c = 0; c < csvFile.ColumnCount; ++c)
		{
			Renderer_DrawTextSolid(&CsvText[r][c], 150 * r, 30 * c, color);
		}
	
	switch (data->RenderMode)
	{
	case SOLID:
	{
		SDL_Color color = { .a = 255 };
		Renderer_DrawImage(&data->TestImage, 0, 0);
		Renderer_DrawTextSolid(&data->TestText, 535, 465, color);
		Renderer_DrawTextSolid(&data->Credit, 580, 575, color);
	}
	break;
	}
}
void release_title(void)
{
	TitleSceneData* data = (TitleSceneData*)g_Scene.Data;

	for (int32 i = 0; i <10; ++i)
	{
		Text_FreeText(&data->GuideLine[i]);
	}
	Text_FreeText(&data->TestText);

	SafeFree(g_Scene.Data);
}
#pragma endregion

#pragma region MainScene

#define GUIDELINE_COUNT 8

typedef struct MainSceneData
{
	CsvFile		csvFile;
	Text		GuideLine[GUIDELINE_COUNT];
	Music		BGM;
	float		Volume;
	SoundEffect Effect;
	Image		BackGround;
	float		Speed;
	int32		X;
	int32		Y;
	int32		Alpha;
	bool		PageType;
	Image		character1;
	Image		character2;
	Image		character3;
	int32		POS;

} MainSceneData;


void logOnFinished(void)
{
	LogInfo("You can show this log on stopped the music");
}

void log2OnFinished(int32 channel)
{
	LogInfo("You can show this log on stopped the effect");
}

static int32 i = 1;
char* _text;
char* effect_buf;
char* _background;
char* _BGM;
char* _character1;
char* _character2;
char* _character3;
char* _choiceType;
char* _choiceTextOne; // 선택지 1번의 지문
char* _choiceTextTwo; // 선택지 2번의 지문
int32 choiceIndex1 = 0; // 선택지 1을 선택했을때 가야하는 씬 번호 저장
int32 choiceIndex2 = 0; // 선택지 2를 선택했을때 가야하는 씬 번호
int prev_scene;
int current_scene;
int next_scene;
Image choicebox;
Image textbox;


void init_main(void)
{
	if (!isCreated)
	{
		CreateCsvFile(&csvFile, "test.csv");  // 희희 
		isCreated = true;
	} //초기 CSV 파일 만들어 주는거 

	g_Scene.Data = malloc(sizeof(MainSceneData));
	memset(g_Scene.Data, 0, sizeof(MainSceneData));
	
	MainSceneData* data = (MainSceneData*)g_Scene.Data;

	for (int r = 0; r < csvFile.RowCount; r++)
	{
		for (int c = 0; c < csvFile.ColumnCount; c++)
		{
			wchar_t* str = ParseToUnicode(csvFile.Items[r][c]);

			Text_CreateText(&CsvText[r][c], "Maplestory.ttf", 20, str, wcslen(str));
			free(str);
		}
	}

	Audio_LoadSoundEffect(&data->Effect, "click1.mp3");
	Audio_HookSoundEffectFinished(log2OnFinished);
	Image_LoadImage(&choicebox, "selectbox.png");		// select 박스 업데이트

}



void update_main(void) // 업데이트 할거임 초기값 i =1
{
	MainSceneData* data = (MainSceneData*)g_Scene.Data;

	int32 choiceNum = ParseToInt(csvFile.Items[i][1]);  // i =1
	_choiceType = choiceNum;

	if (_choiceType == 0) // 선택지 X
	{
		next_scene = ParseToInt(csvFile.Items[i][11]);	// i=1	
		_text = ParseToUnicode(csvFile.Items[i][10]); // i=1
		//effect_buf = ParseToUnicode(csvFile.Items[i][17]); // i=1

			if (Input_GetKeyDown(VK_SPACE))
			{

				_background = ParseToAscii(csvFile.Items[i][2]);
				Image_LoadImage(&data->BackGround, _background); // 백그라운드 업데이트
				Image_LoadImage(&textbox, "textbox.png");		// 텍스트 박스 업데이트

				// 오디오
				_BGM = ParseToAscii(csvFile.Items[i][3]);
				Audio_LoadMusic(&data->BGM, _BGM);
				Audio_HookMusicFinished(logOnFinished);
				Audio_PlayFadeIn(&data->BGM, INFINITY_LOOP, 3000);

				//Audio_LoadSoundEffect(&data->Effect,1);

				_character1 = ParseToAscii(csvFile.Items[i][4]);
				_character2 = ParseToAscii(csvFile.Items[i][6]);
				_character3 = ParseToAscii(csvFile.Items[i][8]);
				Image_LoadImage(&data->character1, _character1);
				Image_LoadImage(&data->character2, _character2);
				Image_LoadImage(&data->character3, _character3);

				// 글씨 출력하는 부분
				//_text = ParseToUnicode(csvFile.Items[i][10]);
				Text_CreateText(&data->GuideLine[0], "Maplestory.ttf", 30, _text, wcslen(_text));

				memset(text_buf, 0, sizeof(text_buf));

				//개행 문자 나누기 시작//
				const wchar_t* lineStart = _text;

				for (int32 line = 0; line < TEXT_MAX_LINE; ++line)
				{
					const wchar_t* lineEnd = lineStart;
					while (true)
					{
						if (L'\n' == *lineEnd || L'\0' == *lineEnd)
							break;
						++lineEnd;
					}
					int32 lineLength = lineEnd - lineStart;
					Text_CreateText(&text_buf[line], "font1.ttf", 25, lineStart, lineLength);
					if (L'\0' == *lineEnd)
						break;
					lineStart = lineEnd + 1;
				}
				//개행 문자 나누기 끝~!//

				i++;
			}	
		
		if (next_scene != 0) // 다음에 나오는 씬이 연속된 것이 아님
		{
			if (Input_GetKeyDown(VK_SPACE))
			{
				i = next_scene - 2;
			}
		}

		
	}

	if (_choiceType == 1) // 선택지 O
	{

			_choiceTextOne = ParseToUnicode(csvFile.Items[i][12]); // 선택지 1번 보이기
			Text_CreateText(&data->GuideLine[0], "font1.ttf", 30, _choiceTextOne, wcslen(_choiceTextOne));

			_choiceTextTwo = ParseToUnicode(csvFile.Items[i][14]); // 선택지 2번 보이기
			Text_CreateText(&data->GuideLine[0], "font1.ttf", 30, _choiceTextTwo, wcslen(_choiceTextTwo));


		if (Input_GetKeyDown(VK_UP))
		{
			data->POS = 0;
			Audio_PlaySoundEffect(&data->Effect, 1);
		}

		else if (Input_GetKeyDown(VK_DOWN))
		{
			data->POS = 1;
			Audio_PlaySoundEffect(&data->Effect, 1);
		}

		if (Input_GetKeyDown(VK_RETURN)) // 엔터가 눌리면
		{
			if (data->POS == 0) // 1번 선택지에서 엔터
			{
				choiceIndex1 = ParseToInt(csvFile.Items[i][13]); // 선택지 1의 인덱스 받아오기
				i = choiceIndex1 - 2; // 이 경우 배경은 35페이지 , 대사는 
		
			}
			else // 2번 선택지에서 엔터
			{
				choiceIndex2 = ParseToInt(csvFile.Items[i][15]); // 선택지 2의 인덱스 받아오기
				i = choiceIndex2 - 2;

			}
		}
	} // 선택지 있을때


} // 전체 update



void render_main(void)
{
	MainSceneData* data = (MainSceneData*)g_Scene.Data;

	Renderer_DrawImage(&data->BackGround, 0, 0); // 배경 출력
	
	Renderer_DrawImage(&data->character1, 0, 10); // char1 출력

	Renderer_DrawImage(&data->character2, 0, 0); // char2 출력

	Renderer_DrawImage(&data->character3, 0, 0); // char3 출력

	if (2 < i)
	{
		Renderer_DrawImage(&textbox, 0, 0); // 텍스트박스 출력
	}
	
	// 텍스트
	SDL_Color color = { .a = 255 };
	Renderer_DrawTextSolid(&text_buf[0], 100, 550, color);
	Renderer_DrawTextSolid(&text_buf[1], 100, 580, color);
	Renderer_DrawTextSolid(&text_buf[2], 100, 610, color);
	
	if (_choiceType == 1)
	{
		Renderer_DrawImage(&choicebox, 0, 50);
		Renderer_DrawImage(&choicebox, 0, 200);

		if (data->POS == 0)
		{
			color.a = 255;
			color.r = 255;
			Renderer_DrawTextSolid(&CsvText[i][12], 565, 225, color);
			color.a = 255;
			color.r = 0;
			Renderer_DrawTextSolid(&CsvText[i][14], 565, 375, color);
		}
		else if (data->POS == 1)
		{
			color.r = 0;
			color.a = 255;
			Renderer_DrawTextSolid(&CsvText[i][12], 565, 225, color);
			color.a = 255;
			color.r = 255;
			Renderer_DrawTextSolid(&CsvText[i][14], 565, 375, color);
		}
	}
}

void release_main(void)
{
	MainSceneData* data = (MainSceneData*)g_Scene.Data;

	for (int32 i = 0; i < GUIDELINE_COUNT; ++i)
	{
		Text_FreeText(&data->GuideLine);
	}

	Text_FreeText((&text_buf[0]));
	Text_FreeText((&text_buf[1]));
	Text_FreeText((&text_buf[2]));


	Audio_FreeMusic(&data->BGM);
	Image_FreeImage(&data->BackGround);
	Image_FreeImage(&data->character1);
	Image_FreeImage(&data->character2);
	Image_FreeImage(&data->character3);
	Audio_FreeSoundEffect(&data->Effect);

	SafeFree(g_Scene.Data);
}
#pragma endregion

#pragma region CreditScene

typedef struct CreditSceneData
{
	Image		BackGround;

} CreditSceneData;


void init_credit(void)
{
	g_Scene.Data = malloc(sizeof(CreditSceneData));
	memset(g_Scene.Data, 0, sizeof(CreditSceneData));

	CreditSceneData* data = (CreditSceneData*)g_Scene.Data;
	Image_LoadImage(&data->BackGround, "Credit.png");
}

void update_credit(void)
{

	CreditSceneData* data = (CreditSceneData*)g_Scene.Data;
	if (Input_GetKeyDown(VK_SPACE))
	{
		Scene_SetNextScene(SCENE_TITLE);
	}
}
void render_credit(void)
{
	CreditSceneData* data = (CreditSceneData*)g_Scene.Data;
	Renderer_DrawImage(&data->BackGround, 0, 0);
}
void release_credit(void)
{
	CreditSceneData* data = (CreditSceneData*)g_Scene.Data;

}


#pragma endregion // 완료. 더 건들 필요X

bool Scene_IsSetNextScene(void)
{
	if (SCENE_NULL == s_nextScene)
	{
		return false;
	}
	else
	{
		return true;
	}
}

void Scene_SetNextScene(ESceneType scene)
{
	assert(s_nextScene == SCENE_NULL);
	assert(SCENE_NULL < scene&& scene < SCENE_MAX);

	s_nextScene = scene;
}

void Scene_Change(void)
{
	assert(s_nextScene != SCENE_NULL);

	if (g_Scene.Release)
	{
		g_Scene.Release();
	}

	switch (s_nextScene)
	{
	case SCENE_TITLE:
		g_Scene.Init = init_title;
		g_Scene.Update = update_title;
		g_Scene.Render = render_title;
		g_Scene.Release = release_title;
		break;
	case SCENE_MAIN:
		g_Scene.Init = init_main;
		g_Scene.Update = update_main;
		g_Scene.Render = render_main;
		g_Scene.Release = release_main;
		break;
	case SCENE_CREDIT:
		g_Scene.Init = init_credit;
		g_Scene.Update = update_credit;
		g_Scene.Render = render_credit;
		g_Scene.Release = release_credit;
		break;
	}

	g_Scene.Init();

	s_nextScene = SCENE_NULL;
}



