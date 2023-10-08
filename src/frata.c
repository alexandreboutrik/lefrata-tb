/*
 * Fork do LeFrata utilizando a Termbox ao invés da Ncurses
 * By Alexandre Boutrik (@alexandreboutrik)
 *
 */
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termbox.h>
#include <time.h>

#define		SCOREFILE			"scores.txt"
#define		FRATA_Y_INITIAL		2
#define		FRATA_X				16

#define		LEFT_ARROW		0x2190
#define		UP_ARROW		0x2191
#define		RIGHT_ARROW		0x2192
#define		DOWN_ARROW		0x2193

#define		COPYRIGHT		0x00A9
#define		DIAMOND			0x2666

#define		AC_ACUTE		0x00C1
#define		AC_TILDE		0x00C3
#define		C_CEDILLA		0x00C7
#define		EC_ACUTE		0x00C9
#define		OC_TILDE		0x00D5
#define		AS_GRAVE		0x00E0
#define		AS_ACUTE		0x00E1
#define		AS_TILDE		0x00E3
#define		S_CEDILLA		0x00E7
#define		ES_ACUTE		0x00E9
#define		ES_CIRCF		0x00EA
#define		IS_ACUTE		0x00ED
#define		OS_ACUTE		0x00F3
#define		OS_TILDE		0x00F5
#define		US_ACUTE		0x00FA

#define		i32			int_fast32_t
#define		i16			int_fast16_t

#define		u64			uint_fast64_t
#define		u32			uint_fast32_t
#define		u16			uint_fast16_t
#define		u8			uint_fast8_t

#define		ru16		register	uint_fast16_t
#define		ru8			register	uint_fast8_t
#define		cu16		const		uint_fast16_t
#define		cu8			const		uint_fast8_t

#define		ci32		const		int_fast32_t
#define		ci16		const		int_fast16_t

enum Scr {
	INITIAL = 0,
	LEVEL, PAUSE, GAMEOVER, SAVESCORE, RANKING, ABOUT
};

enum Abt {
	PAGE_1 = 0,
	PAGE_2
};

typedef struct ScEntry {		/* ScEntry = Score Entry */

	u64			 Score;
	struct tm	 Date;

} ScEntry;

typedef struct Score {

	FILE		*File;

	ScEntry		*Players;	/* lista de jogadores */
	u16			 np;		/* quantidade de entradas, de jogadores */

	ScEntry		 Sc_highest;/* maior score já atingido */
	ScEntry		 Sc_last;	/* último score salvo no arquivo */
	u64			 Sc_today;	/* maior score atingido hoje */

} Score;

typedef struct Hole {

	u8			 y;			/* a posição em y */
	u8			 x;			/* a posição em x */

} Hole;

typedef struct Van {

	u8		 	 y;			/* a posição em y */
	u8			 x;			/* a posição em x */

} Van;

typedef struct GameData {
	
	ScEntry		 Player;	/* jogador atual */
	Score		 Scores;	/* ranking de scores, contém lista dos jogadores */

	enum Scr	 Screen;	/* tela atual */
	enum Scr	 PrevScreen;/* tela anterior */
	enum Abt	 About;		/* define o número da página da tela About */

	Van			 Frata;		/* Le Frata */

	Hole		 Holes[25];	/* vetor de buracos */
	u16			 nb;		/* quantidade de buracos existentes */

	u8			 Refresh;	/* contador de atualização de tela */
	u8			 Damage;	/* variável auxiliar para o piscar quando leva dano */

	u8			 Life;		/* quantidade de vidas */
	u32			 Level;		/* nível atual */

	struct tb_event		Event;	/* termbox event */

} GameData;

/*
 * Lista de funções do código
 */

void		 InitScreen(void);
void		 Quit(GameData*);
void		 Error(GameData*, ci32, const char* Func);

void		 CheckWindowSize(GameData*);

void		 InitData(GameData*);
void		 FreeData(GameData*);

void		 HandleKey(GameData*);
void		 HandleMouse(GameData*);
void		 HandleInput(GameData*);

void		 GetCurrentDate(struct tm*);
bool		 CompareStruct_Date(struct tm*, struct tm*);

void		 CopyStruct_ScEntry(ScEntry*, ScEntry*);

u16			 CountEntries(FILE*);
void		 FillEntries(GameData*);

void		 SaveScorePlayer(FILE*, ScEntry);

void		 CreateIfDoesntExist(GameData*);
void		 OpenFile(GameData*);

void		 IsGameOver(GameData*, cu16);
void		 UpdateLevel(GameData*);

void		 MoveHoles(GameData*);
void		 CalculateColision(GameData*);
void		 GenerateNewHole(GameData*);

void		 PrintLine(cu8, u8, ci16, u16);
void		 PrintColumn(u8, cu8, ci16, u16);
void		 PrintHole(u8, cu8, ci16, cu16);
void		 PrintDiamond(u16);

void		 DrawStreetLane(void);
void		 DrawLifePoints(GameData*);
void		 DrawLevelIndicator(GameData*);

void		 DrawFrata(cu8, cu8);
void		 DrawHoles(GameData*);

void		 ChangeScreen(GameData*, enum Scr);

void		 DrawScr_Initial(void);
void		 DrawScr_Level(GameData*);
void		 DrawScr_Pause(void);
void		 DrawScr_GameOver(GameData*);

void		 DrawScr_Ranking(GameData*);
void		 DrawScr_AboutPage_1(void);
void		 DrawScr_AboutPage_2(void);
void		 DrawAbout(GameData*);

void		 ClearScreen(void);
void		 DrawScreen(GameData*);

void		 UpdatePosition(GameData*);

void InitScreen(void) {

	if (tb_init() != 0)
		err(errno, "tb_init");

	tb_enable_mouse();
	tb_hide_cursor();

}

void Quit(GameData* Game) {

	if (Game != NULL)
		FreeData(Game);

	tb_shutdown();

	exit(EXIT_SUCCESS);

}

void Error(GameData* Game, ci32 Errsv, const char* Func) {

	if (Func != NULL)
		fprintf(stderr, "%s: ", Func);

	if (Errsv != 0)
		fprintf(stderr, "%s", strerror(Errsv));

	fprintf(stderr, "\n");
	Quit(Game);
	
}

/*
 * Verifica se a tela tem tamanho suficiente para o jogo
 */
void CheckWindowSize(GameData* Game) {

	if (tb_width() < 103 || tb_height() < 23)
		Error(Game, 0, "tb_width() or tb_height()");

}

/*
 * Compara duas structs tm Date. Retorna 1 se forem iguais e 0 se forem diferentes
 */
bool CompareStruct_Date(struct tm* Date_1, struct tm* Date_2) {

	if (Date_1->tm_mday == Date_2->tm_mday)
		if (Date_1->tm_mon == Date_2->tm_mon)
			if (Date_1->tm_year == Date_2->tm_year)
				return true;

	return false;


}

void GetCurrentDate(struct tm* Date) {

	time_t CurrentTime;

	/* Lê a data do sistema e salva em CurrentTime */
	time(&CurrentTime);

	struct tm* tmp;

	/* Converte o conteúdo de time_t para struct tm */
	tmp				= localtime(&CurrentTime);

	Date->tm_mday	= tmp->tm_mday;

	/* tm_mon conta quantos meses se passaram desde Janeiro */
	Date->tm_mon	= tmp->tm_mon + 1;

	/* tm_year conta os anos passados desde 1900 */
	Date->tm_year	= tmp->tm_year + 1900;

}

void InitData(GameData *Game) {

	CheckWindowSize(NULL);

	GetCurrentDate(&(Game->Player.Date));

	/* Inicializa o score atual como 100 = Lv 1 e 0 buracos gerados */
	Game->Player.Score		= 100;

	/* Inicializa o maior score atingido hoje */
	Game->Scores.Sc_today	= Game->Player.Score;

	/* Inicializa o maior score já atingido como o jogador atual */
	CopyStruct_ScEntry(&(Game->Scores.Sc_highest), &(Game->Player));

	/* Screen padrão é a inicial */
	Game->Screen			= INITIAL;
	Game->PrevScreen		= INITIAL;
	Game->About				= 0;

	/* Coordenadas iniciais do Frata */
	Game->Frata.y			= FRATA_Y_INITIAL;
	Game->Frata.x			= 4;

	/* Não há buracos no começo do jogo */
	Game->nb				= 0;

	/* Contador começa em zero */
	Game->Refresh			= 0;
	Game->Damage			= 0;

	/* O jogador começa com 3 vidas */
	Game->Life				= 3;

	/* Começa no nível 1 */
	Game->Level				= 1;

	/* Seed para o rand() */
	srand((unsigned) time(NULL));

}

void FreeData(GameData* Game) {

	if (Game->Scores.Players != NULL)
		free(Game->Scores.Players);

	if (Game->Scores.File != NULL)
		fclose(Game->Scores.File);

}

void HandleKey(GameData* Game) {

	switch (Game->Event.key) {

		case TB_KEY_ESC:
		case TB_KEY_CTRL_C:
			Quit(Game);
			break;

		case TB_KEY_ARROW_DOWN:
			Game->Event.ch = 's';
			break;

		case TB_KEY_ARROW_UP:
			Game->Event.ch = 'w';
			break;

		case TB_KEY_CTRL_R:
			Game->Event.ch = 't';
			break;

	}

	switch (Game->Event.ch) {

		case 'q':
		case 'Q':
			Quit(Game);
			break;

		case 't':
		case 'T':
			InitData(Game);
			break;

		/*
		 * Tela inicial
		 */
		case 'i':
		case 'I':
			if (Game->Screen == GAMEOVER)
				InitData(Game);

			ChangeScreen(Game, INITIAL);
			break;

		/*
		 * Tela de ranking
		 */
		case 'r':
		case 'R':
			if (Game->Screen == RANKING)
				ChangeScreen(Game, Game->PrevScreen);
			else if (Game->Screen == INITIAL || Game->Screen == GAMEOVER)
				ChangeScreen(Game, RANKING);
			break;

		/*
		 * Pausa o jogo
		 */
		case 'p':
		case 'P':
			if (Game->Screen == PAUSE)
				ChangeScreen(Game, Game->PrevScreen);

			else if (Game->Screen == LEVEL)
				ChangeScreen(Game, PAUSE);

			break;

		/*
		 * Tela sobre
		 */
		case 'a':
		case 'A':
			if (Game->Screen == ABOUT)
				ChangeScreen(Game, Game->PrevScreen);

			else if (Game->Screen == LEVEL) {
				ChangeScreen(Game, ABOUT);
				Game->About = PAGE_2;
			}
			
			else {
				ChangeScreen(Game, ABOUT);
				Game->About = PAGE_1;
			}

			break;

		/*
		 * Página do about
		 */
		case '1':
			if (Game->Screen == ABOUT)
				Game->About = PAGE_1;
			break;

		case '2':
			if (Game->Screen == ABOUT)
				Game->About = PAGE_2;
			break;

		case 'l':
		case 'L':
			Game->Level = 5;
			break;

		case 's':
		case 'S':
			if (Game->Screen == INITIAL) {
				ChangeScreen(Game, LEVEL);
				break;
			}
			if (Game->Screen == GAMEOVER) {
				ChangeScreen(Game, SAVESCORE);
				break;
			}

			/* FALLTHROUGH */

		case 'w':
		case 'W':
			/* FALLTHROUGH */

		case TB_KEY_ARROW_UP:
		case TB_KEY_ARROW_DOWN:
			UpdatePosition(Game);
			break;

	}

}

/* TODO */
void HandleMouse(GameData* Game) {

		(void) Game;

		/*

		switch (Game->Event.key) {

			case TB_KEY_MOUSE_LEFT:

					(void)1;
					break;

		}

		*/

}

void HandleInput(GameData* Game) {

	switch (Game->Event.type) {

		case TB_EVENT_KEY:
			HandleKey(Game);
			break;

		case TB_EVENT_MOUSE:
			HandleMouse(Game);
			break;

		case TB_EVENT_RESIZE:
			tb_resize();
			CheckWindowSize(Game);
			break;

	}

}

/*
 * Copia duas structs ScEntry (Score Entry)
 */
void CopyStruct_ScEntry(ScEntry* Dst, ScEntry* Src) {

	Dst->Score			= Src->Score;
	Dst->Date.tm_mday	= Src->Date.tm_mday;
	Dst->Date.tm_mon	= Src->Date.tm_mon;
	Dst->Date.tm_year	= Src->Date.tm_year;

}

/*
 * Conta via leitura formatada quantas entradas existem no arquivo
 */
u16 CountEntries(FILE* File) {

	ru16 count = 0;
	ScEntry Entry;

	while ((fscanf(File, "%" SCNu64 " %d %d %d", &(Entry.Score), 
			&(Entry.Date.tm_mday), &(Entry.Date.tm_mon),
			&(Entry.Date.tm_year))) != EOF)
		count++;

	rewind(File);

	return count;

}

/*
 * Salva as entradas salvas do arquivo na struct Players
 * Também calcula os scores (Sc_highest, Sc_today e Sc_last)
 */
void FillEntries(GameData* Game) {
	
	ru16 i;
	for (i = 0; i < Game->Scores.np; i++) {

		fscanf(Game->Scores.File, "%" SCNu64 " %d %d %d", &(Game->Scores.Players[i].Score),
				&(Game->Scores.Players[i].Date.tm_mday),
				&(Game->Scores.Players[i].Date.tm_mon),
				&(Game->Scores.Players[i].Date.tm_year));

		/* Se for maior que o maior score de hoje e as datas forem as mesmas */
		if (Game->Scores.Players[i].Score > Game->Scores.Sc_today)
			if (CompareStruct_Date(&(Game->Scores.Players[i].Date), &(Game->Player.Date)))
					Game->Scores.Sc_today = Game->Scores.Players[i].Score;

		/* Se for maior que o maior score já atingido */
		if (Game->Scores.Players[i].Score > Game->Scores.Sc_highest.Score)
			CopyStruct_ScEntry(&(Game->Scores.Sc_highest), &(Game->Scores.Players[i]));

	}

	/* Salva o score da última entrada em Sc_last */
	CopyStruct_ScEntry(&(Game->Scores.Sc_last), &(Game->Scores.Players[i-1]));

}

/*
 * Salva o score do player no arquivo
 */
void SaveScorePlayer(FILE* File, ScEntry Player) {

	/* Move o ponteiro para o final do arquivo */
	fseek(File, 0, SEEK_END);

	fprintf(File, "\n%" PRIu64 " %d %d %d", Player.Score,
			Player.Date.tm_mday, Player.Date.tm_mon,
			Player.Date.tm_year);

}

/*
 * Verifica se existe o arquivo e cria ele caso não exista
 * Tentamos garantir que ele tenha sempre pelo menos uma entrada
 */
void CreateIfDoesntExist(GameData* Game) {

	if ((Game->Scores.File = fopen(SCOREFILE, "r")) == NULL) {

		if ((Game->Scores.File = fopen(SCOREFILE, "w")) == NULL)
			Error(Game, errno, "fopen");

		SaveScorePlayer(Game->Scores.File, Game->Player);

	}

	fclose(Game->Scores.File);

}

/*
 * Abre o ponteiro arquivo e salva as entradas com FillEntries
 */
void OpenFile(GameData* Game) {

	ru16 n;

	CreateIfDoesntExist(Game);

	/* Abre o arquivo para leitura e escrita */
	if ((Game->Scores.File = fopen(SCOREFILE, "r+")) == NULL)
		Error(Game, errno, "fopen");

	/* Quantidade de entradas no arquivo */
	n = CountEntries(Game->Scores.File);

	/* Salva a quantidade em Scores.np */
	Game->Scores.np = n;

	/* alocação dinâmica da lista de jogadores */
	if ((Game->Scores.Players = (ScEntry *) malloc(n * sizeof(ScEntry))) == NULL)
		Error(Game, errno, "malloc");

	/* Lê o arquivo e salva cada jogador e seu score na struct Players */
	FillEntries(Game);

}

/*
 * Reduz a quantidade de vidas e verifica se é Game Over
 */
void IsGameOver(GameData* Game, cu16 i) {

	Game->Life--;

	if (Game->Life == 0)
		ChangeScreen(Game, GAMEOVER);

	/* Faz o Frata piscar por 50 ms */
	Game->Damage = 50;

	/* Evita a recolisão do buraco com o Le Frata */
	Game->Holes[i].x = 0;

}

/*
 * Atualiza o Game->Refresh e aumenta de nível caso necessário
 */
void UpdateLevel(GameData* Game) {

	/* Evita overflow no Refresh */
	if (Game->Level > 5)
		Game->Refresh = 100;
	else
		Game->Refresh += 20 * Game->Level;

	/* no nv 1
	 * refresh = 0
	 * depois de 10 ms = 20
	 * depois de 10 ms = 40
	 * depois de 10 ms = 60
	 * depois de 10 ms = 80
	 * depois de 10 ms = 100
	 * move os buracos para a esquerda
	 *
	 * no nv 2
	 * refresh = 0
	 * depois de 10 ms = 40
	 * depois de 10 ms = 80
	 * depois de 10 ms = 120
	 * move os buracos para a esquerda
	 * ...
	 */

	/* Quando forem gerados 25 buracos naquele nível, vá para o próximo nível */
	if (Game->nb == 25) {

		Game->nb = 0;
		Game->Level++;

	}

	if (Game->Damage > 0)
		Game->Damage--;

}

/*
 * Move os buracos para a esquerda (x--)
 */
void MoveHoles(GameData* Game) {

	ru16 i;

	/* Quando refresh for >= 100, movemos os buracos */
	if (Game->Refresh >= 100) {

		for (i = 0; i < Game->nb; i++)

			/* Só move caso o x for acima de 0 */
			if (Game->Holes[i].x > 0)
				Game->Holes[i].x--;

		Game->Refresh = 0;

	}

}

/*
 * Verifica se houve colisão
 */
void CalculateColision(GameData* Game) {

	ru16 i;

	for (i = 0; i < Game->nb; i++) {

		/* Verifica se os x e y do Frata e do Buraco[i] são iguais */
		if (Game->Holes[i].x == FRATA_X)
			if (Game->Holes[i].y == Game->Frata.y)
				IsGameOver(Game, i);

	}


}

/*
 * Gera novos buracos (obstáculos) no jogo para o Frata desviar
 */
void GenerateNewHole(GameData* Game) {

	bool existe = false;
	ru16 i;

	for (i = 0; i < Game->nb; i++) {

		/* Caso exista algum buraco com coordenada x maior que 75 */
		if (Game->Holes[i].x > 75)
			existe = true;

	}

	/* Só gera outro buraco caso não existirem buracos com x > 75 */
	if (existe != true) {

		Game->nb++;

		if (rand() % 2 == 0)
			Game->Holes[Game->nb % 25].y = 2;	/* Cria na primeira via */
		else
			Game->Holes[Game->nb % 25].y = 14;	/* Cria na segunda via */

		Game->Holes[Game->nb % 25].x = 95;

		/* Calcula o novo score com o buraco gerado */
		Game->Player.Score = Game->Level * 100 + Game->nb * 4;

	}

}

/*
 * Desenha uma linha de algo
 * n é a quantidade de espaços
 */
void PrintLine(cu8 y, u8 x, ci16 Color, u16 n) {

	for (; n > 0; n--)
		tb_string(x++, y, TB_WHITE, Color, " ");

}

/*
 * Desenha uma coluna de algo
 * n é a quantidade de espaços
 */
void PrintColumn(u8 y, cu8 x, ci16 Color, u16 n){

	for( ; n > 0; n--)
		tb_string(x, y++, TB_WHITE, Color, " ");

}

/*
 * Desenha um buraco na tela inicial de tamanho n
 */
void PrintHole(u8 y, cu8 x, ci16 Color, cu16 n){

	u8 w = (n / 2);

	for (; w > 0; y++, w--)
		PrintLine(y, x, Color, n);


}

/*
 * Desenha as faixas da rua
 */
void DrawStreetLane(void) {

	/* Faixa contínua de cima */
	PrintLine(1, 2, TB_WHITE, 100);

	/* Faixa do meio */
	PrintLine(12, 8, TB_YELLOW, 10);
	PrintLine(12, 28, TB_YELLOW, 10);
	PrintLine(12, 48, TB_YELLOW, 10);
	PrintLine(12, 68, TB_YELLOW, 10);
	PrintLine(12, 88, TB_YELLOW, 10);

	/* Faixa contínua de baixo */
	PrintLine(23, 2, TB_WHITE, 100);

}

/* Imprime uma quantidade i de Diamantes */
void PrintDiamond(u16 i) {

	for (; i > 0; i--)
		tb_char(77 + i, 21, TB_RED, TB_BLACK, DIAMOND);

}

/*
 * Desenha os pontos de vida restantes
 */
void DrawLifePoints(GameData* Game) {

	tb_string(70, 21, TB_WHITE, TB_BLACK, "VIDAS = ");

	PrintDiamond(Game->Life);

}

/*
 * Imprime o nível atual
 */
void DrawLevelIndicator(GameData* Game) {

	tb_stringf(87, 21, TB_WHITE, TB_BLACK, "LEVEL = %" PRIu32, Game->Level);

}

/*
 * Desenha o Frata na tela, na posição y e x
 */
void DrawFrata(cu8 y, cu8 x) {

	PrintLine(y + 2, x, TB_MAGENTA, 9);

	/* Vidros */
	PrintLine(y + 2, x + 9, TB_CYAN, 2);

	PrintLine(y + 3, x, TB_MAGENTA, 13);
	PrintLine(y + 3, x + 12, TB_YELLOW, 1);
	PrintLine(y + 4, x, TB_BLUE, 13);
	PrintLine(y + 5, x, TB_MAGENTA, 13);

	/* Rodas */
	PrintLine(y + 6, x + 1, TB_WHITE, 2);
	PrintLine(y + 6, x + 9, TB_WHITE, 2);

}

/*
 * Desenha os buracos
 */
void DrawHoles(GameData* Game) {

	ru16 i;

	for (i = 0; i < Game->nb; i++) {

		/*
		 * Os buracos devem possuir x > frente do Frata para serem desenhados
		 */
		if (Game->Holes[i].x > FRATA_X) {

			PrintLine(Game->Holes[i].y + 3, Game->Holes[i].x, TB_RED, 6);
			PrintLine(Game->Holes[i].y + 4, Game->Holes[i].x, TB_RED, 6);
			PrintLine(Game->Holes[i].y + 5, Game->Holes[i].x, TB_RED, 6);

		}

	}

}

/*
 * Muda os enum de tela atual e tela anterior
 */
void ChangeScreen(GameData* Game, enum Scr NextScreen) {

	Game->PrevScreen	=	Game->Screen;
	Game->Screen		=	NextScreen;

}

/*
 * Desenha a tela inicial do jogo ou seja, o menu principal
 */
void DrawScr_Initial(void) {
 
	ru8 j;

	for( j = 20; j < 24; j++)
		PrintColumn(1, j, TB_CYAN, 9); // coluna de L

	for( j = 9; j < 11; j++)
		PrintLine(j, 20, TB_CYAN, 10); // risco baixo L

	for( j = 32; j < 35; j++)
		PrintColumn(5, j, TB_CYAN, 6); //  coluna de E

	PrintLine(10, 35, TB_CYAN, 4); // risco em baixo de E

	for( j = 5; j <7; j++)
		PrintLine(j, 35, TB_CYAN, 4); // risco em cima de E

	PrintLine(8, 35, TB_CYAN, 3); // risco no meio de E

	for( j = 47; j < 51; j++)
		PrintColumn(1, j, TB_CYAN, 10); // coluna de F

 	for( j = 1; j < 3; j++)
 		PrintLine(j, 47, TB_CYAN, 10); // risco cima de F

 	for( j = 5; j < 7; j++)
 		PrintLine(j, 47, TB_CYAN, 7); // risco meio de F

 	for( j = 57; j < 60; j++)
 		PrintColumn(5, j, TB_CYAN, 6); // coluna maior de R

 	for( j = 5; j <7; j++)
 		PrintLine(j, 60, TB_CYAN, 4); // risco de cima de R

 	for( j = 62; j < 65; j++)
 		PrintColumn(5, j, TB_CYAN, 3); // coluna menor de R

	PrintLine(8, 60, TB_CYAN, 5); // baixo - R

	PrintLine(9, 61, TB_CYAN, 2);
 	PrintLine(10, 63, TB_CYAN, 2);
 	PrintLine(11, 65, TB_CYAN, 2);

 	for( j = 69; j < 72; j++)
 		PrintColumn(5, j, TB_CYAN, 6); // esquerda - A1

 	for( j = 5; j <7; j++)
 		PrintLine(j, 70, TB_CYAN, 4); // cima - A1

 	for( j = 74; j < 77; j++)
		PrintColumn(5, j, TB_CYAN, 6); // direita - A1

 	PrintLine(8, 70, TB_CYAN, 5); // baixo - A1

 	for( j = 82; j < 85; j++)
 		PrintColumn(5, j, TB_CYAN, 6); // T

 	for( j = 5; j <7 ; j++)
 		PrintLine(j, 79, TB_CYAN, 10); // cima - T

 	for( j = 91; j < 94; j++)
 		PrintColumn(5, j, TB_CYAN, 6); // esquerda - A1

 	for( j = 5; j <7; j++)
 		PrintLine(j, 92, TB_CYAN, 4); // cima - A1

 	for( j = 96; j < 99; j++)
 		PrintColumn(5, j, TB_CYAN, 6); // direita - A1

 	PrintLine(8, 92, TB_CYAN, 5); // baixo - A1

	/* Desenha o Frata */
	DrawFrata(14, 85);


	/* Desenha os buracos da tela inicial */
	PrintHole( 9, 5, TB_RED, 8);
    PrintHole( 15, 8, TB_RED, 2);
	PrintHole( 16, 13, TB_RED, 4);
   	PrintHole( 18, 20, TB_RED, 8);

	tb_char(45, 15, TB_WHITE, TB_BLACK, DIAMOND);

	for (j=15; j<22 ; j+=2) {
		tb_char(45, j, TB_WHITE, TB_BLACK, DIAMOND);
		tb_char(61, j, TB_WHITE, TB_BLACK, DIAMOND);
	}
	
	/* Imprime o menu */
	tb_string(50, 15, TB_WHITE, TB_BLACK, "(S)TART");
	tb_string(50, 17, TB_WHITE, TB_BLACK, "(Q)UIT");
	tb_string(50, 19, TB_WHITE, TB_BLACK, "(A)BOUT");
	tb_string(49, 21, TB_WHITE, TB_BLACK, "(R)ANKING");

}

/*
 * Desenha o nível atual do jogo
 */
void DrawScr_Level(GameData* Game) {

	/* Aumenta o Game->Refresh e verifica se está na hora de aumentar o nível */
	UpdateLevel(Game);

	/* Move os buracos para a esquerda se Game->Refresh >= 100 */
	MoveHoles(Game);

	/* Calcula se houve alguma colisão */
	CalculateColision(Game);

	/* Desenha o LeFrata */
	if (Game->Damage % 5 == 0)
		DrawFrata(Game->Frata.y, Game->Frata.x);

	/* Desenha as vias */
	DrawStreetLane();

	/* Desenha os buracos */
	DrawHoles(Game);

	/* Desenha o indicador dos pontos de vida */
	DrawLifePoints(Game);

	/* Desenha o indicador de nível */
	DrawLevelIndicator(Game);

	/* Gera um novo buraco na tela */
	GenerateNewHole(Game);

}

/*
 * Desenha a tela de Pause
 */
void DrawScr_Pause(void) {

	ru8 x;

	PrintLine(4, 17, TB_YELLOW, 5);
	PrintLine(6, 17, TB_YELLOW, 5);

	for(x = 17; x < 19; x++)
		PrintColumn(4, x, TB_YELLOW, 5);
	for(x = 22; x < 24; x++)
		PrintColumn(4, x, TB_YELLOW, 3);

	//a
	PrintLine(4, 26, TB_YELLOW, 5);
	PrintLine(6, 26, TB_YELLOW, 5);

	for(x = 26; x < 28; x++)
		PrintColumn(4, x, TB_YELLOW, 5);
	for(x = 31; x < 33; x++)
		PrintColumn(4, x, TB_YELLOW, 5);

	//u
	for(x = 35; x < 37; x++)
		PrintColumn(4, x, TB_YELLOW, 5);
	for(x = 40; x < 42; x++)
		PrintColumn(4, x, TB_YELLOW, 5);

	PrintLine(8, 35, TB_YELLOW, 5);

	//s
	PrintLine(4, 44, TB_YELLOW, 6);
	PrintLine(6, 44, TB_YELLOW, 6);
	PrintLine(8, 44, TB_YELLOW, 6);

	for(x = 44; x < 46; x++)
		PrintColumn(4, x, TB_YELLOW, 3);
	for(x = 48; x < 50; x++)
		PrintColumn(6, x, TB_YELLOW, 3);

	//e
	for(x = 52; x < 54; x++)
		PrintColumn(4, x, TB_YELLOW, 5);

	PrintLine(4, 52, TB_YELLOW, 6);
	PrintLine(6, 52, TB_YELLOW, 6);
	PrintLine(8, 52, TB_YELLOW, 6);

	//botao de pause
	for(x = 29; x < 31; x++)
		PrintColumn(10, x, TB_YELLOW, 9);
	for(x = 44; x < 46; x++)
		PrintColumn(10, x, TB_YELLOW, 9);

	PrintLine(10, 30, TB_YELLOW, 14);
	PrintLine(18, 30, TB_YELLOW, 14);

	//setinha no meio
	for(x = 35; x < 37; x++)
		PrintColumn(12, x, TB_YELLOW, 5);
	for(x = 37; x < 39; x++)
		PrintColumn(13, x, TB_YELLOW, 3);
	for(x = 39; x < 41; x++)
		PrintColumn(14, x, TB_YELLOW, 1);

}

/*
 * Desenha a tela de GameOver
 */
void DrawScr_GameOver(GameData* Game) {

	ru8 y, x;

	/*QUADRADOS ESQUERDA*/

	for (x=3; x < 9; x++)
		PrintColumn(4, x, TB_RED, 3);

	for (x=3; x < 9; x++)
		PrintColumn(8, x, TB_RED, 3);

	for (x=3; x < 9; x++)
		PrintColumn(12, x, TB_RED, 3);

	for (x=3; x < 9; x++)
		PrintColumn(16, x, TB_RED, 3);

	for (x=3; x < 9; x++)
		PrintColumn(20, x, TB_RED, 1);

	/*G*/

	for (y=4; y < 5; y++)
		PrintLine(y, 16, TB_RED, 5);

	for (x=15; x < 17; x++)
		PrintColumn(5, x, TB_RED, 5);

	for (y=10; y < 11; y++)
		PrintLine(y, 16, TB_RED, 4);

	for (x=19; x < 21; x++)
		PrintColumn(8, x, TB_RED, 2);

	for (y=7; y < 8; y++)
		PrintLine(y, 18, TB_RED, 3);

	for (x=17; x < 18; x++)
		PrintColumn(11, x, TB_RED, 3);

	for (x=18; x < 19; x++)
		PrintColumn(11, x, TB_RED, 1);

	/*A*/

	for (x=22; x < 24; x++)
		PrintColumn(4, x, TB_RED, 7);

	for (y=4; y < 5; y++)
		PrintLine(y, 24, TB_RED, 2);

	for (y=7; y < 8; y++)
		PrintLine(y, 24, TB_RED, 2);

	for (x=26; x < 28; x++)
		PrintColumn(4, x, TB_RED, 7);

	for (x=23; x < 24; x++)
		PrintColumn(11, x, TB_RED, 2);

	for (x=26; x < 27; x++)
		PrintColumn(11, x, TB_RED, 3);

	/*M*/

	for (x=29; x < 31; x++)
		PrintColumn(4, x, TB_RED, 7);

	for (y=4; y < 5; y++)
		PrintLine(y, 31, TB_RED, 1);

	for (x=32; x < 33; x++)
		PrintColumn(5, x, TB_RED, 2);

	for (y=4; y < 5; y++)
		PrintLine(y, 33, TB_RED, 1);

	for (x=34; x < 36; x++)
		PrintColumn(4, x, TB_RED, 7);

	for (x=29; x < 30; x++)
		PrintColumn(11, x, TB_RED, 3);

	for (x=34; x < 35; x++)
		PrintColumn(11, x, TB_RED, 1);

	/*E*/

	for (x=37; x < 39; x++)
		PrintColumn(4, x, TB_RED, 7);

	for (y=4; y < 5; y++)
		PrintLine(y, 39, TB_RED, 4);

	for (y=7; y < 8; y++)
		PrintLine(y, 39, TB_RED, 2);

	for (y=10; y < 11; y++)
		PrintLine(y, 39, TB_RED, 4);

	for (x=40; x < 41; x++)
		PrintColumn(11, x, TB_RED, 3);


	for (x=38; x < 39; x++)
		PrintColumn(11, x, TB_RED, 2);

	for (x=41; x < 42; x++)
		PrintColumn(11, x, TB_RED, 1);

	/*O*/

	for (x= 49; x < 51; x++)
		PrintColumn(5, x, TB_RED, 5);

	for (y=4; y < 5; y++)
		PrintLine(y, 50, TB_RED, 4);

	for (y=10; y < 11; y++)
		PrintLine(y, 50, TB_RED, 4);

	for (x= 53; x < 55; x++)
		PrintColumn(5, x, TB_RED, 5);

	for (x=51; x < 52; x++)
		PrintColumn(11, x, TB_RED, 2);

	for (x=52; x < 53; x++)
		PrintColumn(11, x, TB_RED, 1);

	/*V*/

	for (x= 56; x < 58; x++)
		PrintColumn(4, x, TB_RED, 5);

	for (x=58; x < 59; x++)
		PrintColumn(9, x, TB_RED, 2);

	for (x=60; x < 61; x++)
		PrintColumn(9, x, TB_RED, 2);

	for (x= 61; x < 63; x++)
		PrintColumn(4, x, TB_RED, 5);

	for (x=59; x < 60; x++)
		PrintColumn(11, x, TB_RED, 2);

	/*E*/

	for (x= 64; x < 66; x++)
		PrintColumn(4, x, TB_RED, 7);

	for (y=4; y < 5; y++)
		PrintLine(y, 66, TB_RED, 4);

	for (y=7; y < 8; y++)
		PrintLine(y, 66, TB_RED, 2);

	for (y=10; y < 11; y++)
		PrintLine(y, 66, TB_RED, 4);

	for (x=65; x < 66; x++)
		PrintColumn(11, x, TB_RED, 1);

	for (x=66; x < 67; x++)
		PrintColumn(11, x, TB_RED, 2);

	for (x=69; x < 70; x++)
		PrintColumn(11, x, TB_RED, 3);

	/*R*/

	for (x= 71; x < 73; x++)
		PrintColumn(4, x, TB_RED, 7);

	for (y=4; y < 5; y++)
		PrintLine(y, 73, TB_RED, 4);

	for (x= 75; x < 77; x++)
		PrintColumn(4, x, TB_RED, 3);

	for (y=7; y < 8; y++)
		PrintLine(y, 73, TB_RED, 4);

	for (y=8; y < 9; y++)
		PrintLine(y, 73, TB_RED, 1);

	for (y=9; y < 10; y++)
		PrintLine(y, 74, TB_RED, 1);

	for (x=75; x < 76; x++)
		PrintColumn(10, x, TB_RED, 3);

	/*QUADRADOS DIREITA*/

	for (x= 84; x < 90; x++)
		PrintColumn(4, x, TB_RED, 3);

	for (x= 84; x < 90; x++)
		PrintColumn(8, x, TB_RED, 3);

	for (x= 84; x < 90; x++)
		PrintColumn(12, x, TB_RED, 3);

	for (x= 84; x < 90; x++)
		PrintColumn(16, x, TB_RED, 3);

	for (x= 84; x < 90; x++)
		PrintColumn(20, x, TB_RED, 1);


	/*ENTRADAS MARGEM*/


	for (y = 16; y < 17; y++)
		PrintLine(y, 15, TB_WHITE, 62);

	for (x = 15; x < 16; x++)
		PrintColumn(16, x, TB_WHITE, 4);

	for (y = 20; y < 21; y++)
		PrintLine(y, 15, TB_WHITE, 62);

	for (x = 76; x < 77; x++)
		PrintColumn(16, x, TB_WHITE, 4);


	/* Imprime o Score na tela GameOver*/
	tb_stringf(22, 18, TB_WHITE, TB_BLUE, "SCORE %" PRIu64, Game->Player.Score);

	/* ENTRADAS */

	tb_string(35, 18, TB_WHITE, TB_RED, "(T)RY AGAIN");
	tb_string(48, 18, TB_WHITE, TB_RED, "(S)AVE SCORE");
	tb_string(62, 18, TB_WHITE, TB_RED, "(Q)UIT");

}

/*
 * Desenha a tela com os rankings
 */
void DrawScr_Ranking(GameData* Game) {

	ru8 x;

    //tabela
	for (x = 17; x < 78; x++)
		PrintColumn(4, x, TB_CYAN, 7);

	for (x = 17; x < 19; x++)
		PrintColumn(11, x, TB_CYAN, 12);

	for (x = 76; x < 78; x++)
		PrintColumn(11, x, TB_CYAN, 12);

	for (x = 37; x < 39; x++)
		PrintColumn(11, x, TB_CYAN, 12);

	for (x = 56; x < 58; x++)
		PrintColumn(11, x, TB_CYAN, 12);


	PrintLine(22, 19, TB_CYAN, 57);
	PrintLine(14, 19, TB_CYAN, 57);
	PrintLine(18, 19, TB_CYAN, 57);

        //f
	PrintLine(5, 19, TB_WHITE, 4);
	PrintLine(7, 19, TB_WHITE, 3);
	PrintColumn(5, 19, TB_WHITE, 5);

	//r

	PrintColumn(5, 24, TB_WHITE, 5);
	PrintColumn(5, 27, TB_WHITE, 2);
	PrintLine(5, 24, TB_WHITE, 3);
	PrintLine(7, 24, TB_WHITE, 3);
	PrintColumn(8, 27, TB_WHITE, 2);

	//a

	PrintColumn(5, 29, TB_WHITE, 5);
	PrintColumn(5, 32, TB_WHITE, 5);
	PrintLine(5, 29, TB_WHITE, 3);
	PrintLine(7, 29, TB_WHITE, 3);

	//t

	PrintColumn(5, 36, TB_WHITE, 5);
	PrintLine(5, 34, TB_WHITE, 5);

	//a

	PrintColumn(5, 40, TB_WHITE, 5);
	PrintColumn(5, 43, TB_WHITE, 5);
	PrintLine(5, 40, TB_WHITE, 3);
	PrintLine(7, 40, TB_WHITE, 3);

	//s

	PrintLine(5, 49, TB_WHITE, 5);
	PrintLine(7, 49, TB_WHITE, 5);
	PrintLine(9, 49, TB_WHITE, 5);
	PrintColumn(5, 49, TB_WHITE, 2);
	PrintColumn(7, 53, TB_WHITE, 2);

	//c
	
	PrintColumn(5, 55, TB_WHITE, 5);
	PrintLine(5, 55, TB_WHITE, 5);
	PrintLine(9, 55, TB_WHITE, 5);

	//o

	PrintLine(5, 61, TB_WHITE, 5);
	PrintLine(9, 61, TB_WHITE, 5);
	PrintColumn(5, 61, TB_WHITE, 5);
	PrintColumn(5, 65, TB_WHITE, 5);

	//r
	PrintColumn(5, 67, TB_WHITE, 5);
	PrintColumn(5, 70, TB_WHITE, 2);
	PrintLine(5, 67, TB_WHITE, 3);
	PrintLine(7, 67, TB_WHITE, 3);
	PrintColumn(8, 70, TB_WHITE, 2);

	//e
	PrintColumn(5, 72, TB_WHITE, 5);
	PrintLine(5, 72, TB_WHITE, 4);
	PrintLine(7, 72, TB_WHITE, 4);
	PrintLine(9, 72, TB_WHITE, 4);


	tb_string(22, 12, TB_WHITE, TB_BLACK, "HIGHEST EVER");
	tb_string(23, 16, TB_WHITE, TB_BLACK, "BEST TODAY");
	tb_string(23, 20, TB_WHITE, TB_BLACK, "LAST SCORE");

	tb_stringf(45, 12, TB_WHITE, TB_BLACK, "%" PRIu64, 
			Game->Scores.Sc_highest.Score);
	tb_stringf(45, 16, TB_WHITE, TB_BLACK, "%" PRIu64, 
			Game->Scores.Sc_today);
	tb_stringf(45, 20, TB_WHITE, TB_BLACK, "%" PRIu64, 
			Game->Scores.Sc_last.Score);

	tb_stringf(63, 12, TB_WHITE, TB_BLACK, "%d/%d/%d", 
			Game->Scores.Sc_highest.Date.tm_mday, 
			Game->Scores.Sc_highest.Date.tm_mon, 
			Game->Scores.Sc_highest.Date.tm_year);
	tb_stringf(63, 16, TB_WHITE, TB_BLACK, "%d/%d/%d", 
			Game->Player.Date.tm_mday, 
			Game->Player.Date.tm_mon, 
			Game->Player.Date.tm_year);
	tb_stringf(63, 20, TB_WHITE, TB_BLACK, "%d/%d/%d", 
			Game->Scores.Sc_last.Date.tm_mday, 
			Game->Scores.Sc_last.Date.tm_mon, 
			Game->Scores.Sc_last.Date.tm_year);

}

/*
 * Desenha a tela que diz "Score salvo"
 */
void DrawScr_SaveScore(GameData* Game) {

	ClearScreen();
    
	ru8 x;

    //s
	PrintLine(7, 22, TB_BLUE, 5);
	PrintLine(9, 22, TB_BLUE, 5);
	PrintLine(11, 22, TB_BLUE, 5);
	PrintColumn(7, 22, TB_BLUE, 2);
	PrintColumn(9, 26, TB_BLUE, 2);

	//c
	PrintColumn(7, 28, TB_BLUE, 5);
	PrintLine(7, 28, TB_BLUE, 5);
	PrintLine(11, 28, TB_BLUE, 5);

	//o
	PrintLine(7, 34, TB_BLUE, 5);
	PrintLine(11, 34, TB_BLUE, 5);
	PrintColumn(7, 34, TB_BLUE, 5);
	PrintColumn(7, 38, TB_BLUE, 5);

	//r
	PrintColumn(7, 40, TB_BLUE, 5);
	PrintColumn(7, 43, TB_BLUE, 2);
	PrintLine(7, 40, TB_BLUE, 3);
	PrintLine(9, 40, TB_BLUE, 3);
	PrintColumn(10, 43, TB_BLUE, 2);

	//e
	PrintColumn(7, 45, TB_BLUE, 5);
	PrintLine(7, 45, TB_BLUE, 4);
	PrintLine(9, 45, TB_BLUE, 4);
	PrintLine(11, 45, TB_BLUE, 4);

	//s
	PrintLine(7, 51, TB_BLUE, 5);
	PrintLine(9, 51, TB_BLUE, 5);
	PrintLine(11, 51, TB_BLUE, 5);
	PrintColumn(7, 51, TB_BLUE, 2);
	PrintColumn(9, 55, TB_BLUE, 2);

	//a
	PrintLine(7, 57, TB_BLUE, 5);
	PrintLine(9, 58, TB_BLUE, 3);
	PrintColumn(7, 57, TB_BLUE, 5);
	PrintColumn(7, 61, TB_BLUE, 5);

	//faz o l
	PrintColumn(7, 63, TB_BLUE, 5);
	PrintLine(11, 64, TB_BLUE, 4);

	//v
	PrintColumn(7, 69, TB_BLUE, 3);
	PrintColumn(7, 73, TB_BLUE, 3);
	PrintColumn(10, 70, TB_BLUE, 1);
	PrintColumn(11, 71, TB_BLUE, 1);
	PrintColumn(10, 72, TB_BLUE, 1);

	//o
	PrintLine(7, 76, TB_BLUE, 4);
	PrintLine(11, 76, TB_BLUE, 4);
	PrintColumn(7, 75, TB_BLUE, 5);
	PrintColumn(7, 79, TB_BLUE, 5);

	// !
	PrintColumn(7, 85, TB_BLUE, 3);
	PrintLine(11, 85, TB_BLUE, 1);

	tb_string(45, 14, TB_WHITE, TB_BLACK, "Clique para sair:");

	for (x = 49; x < 57; x++) {
		tb_char(x, 16, TB_BLACK, TB_WHITE, ' ');
		tb_char(x, 20, TB_BLACK, TB_WHITE, ' ');
	}

	tb_char(49, 17, TB_BLACK, TB_WHITE, ' ');
	tb_char(49, 19, TB_BLACK, TB_WHITE, ' ');

	for (x = 17; x < 20; x++) {
		tb_char(49, x, TB_BLACK, TB_WHITE, ' ');
		tb_char(56, x, TB_BLACK, TB_WHITE, ' ');
	}

	tb_string(51, 18, TB_WHITE, TB_BLUE, "(O)K");

	SaveScorePlayer(Game->Scores.File, Game->Player);

	while (tb_peek_event(&(Game->Event), 10) != 10) {

		switch (Game->Event.ch) {

			case 'o':
			case 'O':
			case 'q':
			case 'Q':
				Quit(Game);
				break;

		}

		tb_render();

	}

}


/*
 * Desenha a primeira página do About
 */
void DrawScr_AboutPage_1(void){

	ru8 i;

	/* Desenha ABOUT na tela */

	// Desenha A
	for( i = 29; i < 31; i++ )
		PrintColumn(1, i, TB_BLUE, 5);

	PrintLine(1, 31, TB_BLUE, 3);
	PrintLine(3, 31, TB_BLUE, 3);

	for( i = 34 ; i < 36; i++ )
		PrintColumn(1, i, TB_BLUE, 5);


	// Desenha B
	for( i = 38; i < 40; i++ )
		PrintColumn(1, i, TB_BLUE, 4);

	PrintLine(1, 39, TB_BLUE, 4);
	PrintLine(3, 39, TB_BLUE, 4);

	for( i = 42 ; i < 44; i++ )
		PrintColumn(1, i, TB_BLUE, 2);

	for( i = 43; i < 45; i++ )
		PrintColumn(3, i, TB_BLUE, 2);

	PrintLine(5, 38, TB_BLUE, 7);


	// Desenha O
	for( i = 47; i< 49; i++ )
		PrintColumn(1, i, TB_BLUE, 5);

	PrintLine(1, 49, TB_BLUE, 3);
	PrintLine(5, 49, TB_BLUE, 3);

	for( i = 52 ; i < 54; i++ )
		PrintColumn(1, i, TB_BLUE, 5);


	// Desenha U
	for( i = 56; i < 58; i++ )
		PrintColumn(1, i, TB_BLUE, 5);

	PrintLine(5, 58, TB_BLUE, 3);

	for( i = 61 ; i < 63; i++ )
		PrintColumn(1, i, TB_BLUE, 5);


	// Desenha T
	for( i = 67; i< 69; i++ )
		PrintColumn(1, i, TB_BLUE, 5);

	PrintLine(1, 65 , TB_BLUE, 6);


	/* Imprime o cabeçalho*/
	tb_stringf(2, 1, TB_WHITE, TB_BLACK, "P%lcGINA 1/2", AC_ACUTE);
	tb_string(85, 1, TB_WHITE, TB_BLACK, "Press (2) for ");
	tb_char(99, 1, TB_WHITE, TB_BLACK, RIGHT_ARROW);
	tb_char(9, 1, TB_WHITE, TB_CYAN, '1');


	/* Imprime o texto de conteztualização */
	tb_stringf(2, 8, TB_WHITE, TB_BLACK, "      Uma bela sexta-feira ensolarada! Diferentemente do dia anterior, h%lc poucas, mas brancas", AS_ACUTE);
	tb_stringf(2, 9, TB_WHITE, TB_BLACK, "nuvens no c%lcu de uma fresca manh%lc. Com %lcxito, a afamada van Frata cumprir%lc mais uma vez o seu mais", ES_ACUTE, AS_TILDE, ES_CIRCF, AS_ACUTE);
	tb_stringf(14, 10, TB_WHITE, TB_BLACK, "nobre objetivo: levar os estudantes e chegar ao seu destino %lc tempo...", AS_GRAVE);
	tb_stringf(2, 12, TB_WHITE, TB_BLACK, "    Ah n%lco! Sua rota di%lcria parece ter sido obtru%lcda por %lcrvores e pedras advindas da tempestade ", AS_TILDE, AS_ACUTE, IS_ACUTE, AS_ACUTE);
	tb_stringf(2, 13, TB_WHITE, TB_BLACK, "noite de quinta-feira. Sua %lcnica alternativa, ent%lco: fazer um desvio por uma estrada seriamente ", US_ACUTE, AS_TILDE);
	tb_stringf(15, 14, TB_WHITE, TB_BLACK, "esburacada. Frata est%lc atrasado dessa vez e, agora, precisar%lc se apressar.", AS_ACUTE, AS_ACUTE);

	tb_stringf(14, 17, TB_CYAN, TB_BLACK, "GUIE O FRATA AO LONGO DE SUA ROTA DESVIANDO DO M%lcXIMO DE BURACOS QUE PUDER", AC_ACUTE);
	tb_stringf(65, 20, TB_WHITE, TB_BLACK, "(Instru%lc%lces e informa%lc%lces p%lcgina 2)", S_CEDILLA, OS_TILDE, S_CEDILLA, OS_TILDE, AS_ACUTE);


	/* Imprime os diamantes */
	tb_char(10, 17, TB_CYAN, TB_BLACK, DIAMOND);
	tb_char(91, 17, TB_CYAN, TB_BLACK, DIAMOND);


	/* Imprime os quadrados vermelhos (buracos) na tela*/
	PrintHole(20, 2, TB_RED, 4);
	PrintHole(18, 5, TB_RED, 2);
	PrintHole(16, 2, TB_RED, 2);
	PrintHole(19, 9, TB_RED, 4);
	PrintHole(21, 15, TB_RED, 2);

}

/*
 * Desenha a segunda página do About
 */
void DrawScr_AboutPage_2(void){

	/* Imprime os cabeçalho */
	tb_stringf(2, 1, TB_WHITE, TB_BLACK, "P%lcGINA 1/2", AC_ACUTE);
	tb_string(92, 1, TB_WHITE, TB_BLACK, "Press (1)");
	tb_char(89, 1, TB_WHITE, TB_BLACK, LEFT_ARROW);
	tb_char(11, 1, TB_WHITE, TB_CYAN, '2');

	/* Imprime os títulos das seções */
	tb_stringf(18, 3, TB_WHITE, TB_BLUE, "INSTRU%lc%lcES", C_CEDILLA, OC_TILDE);
	tb_stringf(54, 14, TB_WHITE, TB_BLUE, "CR%lcDITOS", EC_ACUTE);


	/* Imprime as linhas de divisão da tela */
	ru8 i;

	for (i = 3; i < 23; i++) {
		tb_char(50, i, TB_WHITE, TB_BLUE, ' ');
		tb_char(51, i, TB_WHITE, TB_BLUE, ' ');
	}
	for (i = 63; i < 105; i++)
		tb_char(i, 14, TB_WHITE, TB_BLUE, ' ');
    

    /* Imprime as instruções de movimentação */
    tb_char(10, 6, TB_WHITE, TB_BLACK, ',');
    tb_string(15, 6, TB_WHITE, TB_BLACK, "PARA MOVER-SE NO JOGO.");
	tb_char(8, 6, TB_RED, TB_BLACK, UP_ARROW);
	tb_char(8, 7, TB_RED, TB_BLACK, DOWN_ARROW);
	tb_char(12, 6, TB_YELLOW, TB_BLACK, 'W');
	tb_char(12, 7, TB_YELLOW, TB_BLACK, 'S');


   	/* Imprime as demais intruções */
   	tb_string(2, 9, TB_WHITE, TB_BLACK, "ABOUT:  pressione A a qualque momento.");
   	tb_string(2, 11, TB_WHITE, TB_BLACK, "PAUSE:  pressione P durante o jogo.");
   	tb_string(2, 13, TB_WHITE, TB_BLACK, "QUIT:  pressione Q a qualquer momento.");
   	tb_string(2, 15, TB_WHITE, TB_BLACK, "REINICIAR O JOGO:            ou    .");
   	tb_string(2, 17, TB_WHITE, TB_BLACK, "TELA INICIAL:  pressione I a qualquer momento.");
   	tb_string(2, 19, TB_WHITE, TB_BLACK, "RANKING: pressione R antes ou ao final do jogo.");
   	tb_string(2, 21, TB_WHITE, TB_BLACK, "SAVESCORE:  pressione S ao final do jogo.");


   	/* Imprime os caracteres de destaque das intruções */
	tb_char(20, 9, TB_BLUE, TB_BLACK, 'A');
	tb_char(20, 11, TB_BLUE, TB_BLACK, 'P');
	tb_char(19, 13, TB_BLUE, TB_BLACK, 'Q');
	tb_string(21, 15, TB_BLUE, TB_BLACK, "CTRL + R");
	tb_char(35, 15, TB_BLUE, TB_BLACK, 'T');
	tb_char(27, 17, TB_BLUE, TB_BLACK, 'I');
	tb_char(22, 19, TB_BLUE, TB_BLACK, 'R');
	tb_char(24, 21, TB_BLUE, TB_BLACK, 'S');


    /* Imprime os nomes em destaque de cada intruções */
	tb_string(2, 6, TB_CYAN, TB_BLACK, "USE:");
	tb_string(2, 9, TB_CYAN, TB_BLACK, "ABOUT:");
	tb_string(2, 11, TB_CYAN, TB_BLACK, "PAUSE:");
	tb_string(2, 13, TB_CYAN, TB_BLACK, "QUIT:");
	tb_string(2, 15, TB_CYAN, TB_BLACK, "REINICIAR O JOGO:");
	tb_string(2, 17, TB_CYAN, TB_BLACK, "TELA INICIAL:");
	tb_string(2, 19, TB_CYAN, TB_BLACK, "RANKING:");
	tb_string(2, 21, TB_CYAN, TB_BLACK, "SAVESCORE:");


    /* Imprime as informações do lado esquerdo */
   	tb_stringf(53, 3, TB_WHITE, TB_BLACK, "O GAMEOVER acontece ap%lcs 3 colis%lces com buracos.", OS_ACUTE, OS_TILDE);
   	tb_stringf(53, 5, TB_WHITE, TB_BLACK, "A velocidade m%lcxima %lc atingida ao alcan%lcar LEVEL 5.", AS_ACUTE, ES_ACUTE, S_CEDILLA);
   	tb_stringf(53, 7, TB_WHITE, TB_BLACK, "Caso seja desejado, pode-se acessar o hist%lcrico de", OS_ACUTE);
   	tb_stringf(53, 8, TB_WHITE, TB_BLACK, "scores: pesquise pelo arquivo 'score.txt' em sua");
   	tb_stringf(53, 9, TB_WHITE, TB_BLACK, "m%lcquina.", AS_ACUTE);
   	tb_stringf(53, 11, TB_RED, TB_BLACK, "ATEN%lc%lcO: as dimens%lces m%lcnimas da tela do terminal", C_CEDILLA, AC_TILDE, OS_TILDE, IS_ACUTE);
   	tb_string(53, 12, TB_RED, TB_BLACK, "devem ser 103 X 23.");


    /* Imprime os créditos */
   	tb_string(53, 16, TB_WHITE, TB_BLACK, "Criadores:");
   	tb_string(68, 16, TB_WHITE, TB_BLACK,"Alexandre Boutrik");
   	tb_string(68, 17, TB_WHITE, TB_BLACK, "Giulia Maria Rheinheimer");
   	tb_string(68, 18, TB_WHITE, TB_BLACK, "Larissa Machado");
   	tb_string(68, 19, TB_WHITE, TB_BLACK, "Maria Fernanda De Bastiani");
   	tb_stringf(68, 21, TB_WHITE, TB_BLACK, "2023 LeFrata%lc", COPYRIGHT);

    /* Imprime os quadrados vermelhos (buracos) na tela*/
   	PrintHole(21, 100, TB_RED, 4);
   	PrintHole(18, 97, TB_RED, 4);
   	PrintHole(16, 100, TB_RED, 2);
   	PrintHole(21, 95, TB_RED, 2);
   	PrintHole(22, 91, TB_RED, 2);

}

void DrawAbout(GameData* Game) {

    switch (Game->About){

        case PAGE_1:
            DrawScr_AboutPage_1();
            break;

        case PAGE_2:
            DrawScr_AboutPage_2();
			break;

     }

}

/*
 * Imprime plano de fundo preto no buffer inteiro
 * TODO: utilizar tb_clear_screen()
 */
void ClearScreen(void) {

	tb_clear_buffer();

	ru8 y;

	for (y = 0; y < 24; y++)
		tb_empty(1, y, TB_BLACK, 103);

}

/*
 * Desenha a tela especificada em Game->Screen
 */
void DrawScreen(GameData* Game) {

	switch (Game->Screen) {

		case LEVEL:
			DrawScr_Level(Game);
			break;

		case PAUSE:
			DrawScr_Pause();
			break;

		case GAMEOVER:
			DrawScr_GameOver(Game);
			break;

		case RANKING:
			DrawScr_Ranking(Game);
			break;

		case SAVESCORE:
			DrawScr_SaveScore(Game);
			break;

		case ABOUT:
			DrawAbout(Game);
			break;

		default:
			DrawScr_Initial();

	}

}

/*
 * Atualizar a posição do Input em x ou y de acordo com as KEY_(direção)
 * Os y possíveis são 2 na primeira via e 14 na segunda via
 */
void UpdatePosition(GameData* Game) {

	switch (Game->Event.ch) {

		case 'w':
		case 'W':
		case TB_KEY_ARROW_UP:
			if (Game->Frata.y > FRATA_Y_INITIAL)
				Game->Frata.y = FRATA_Y_INITIAL;
			break;

		case 's':
		case 'S':
		case TB_KEY_ARROW_DOWN:
			if (Game->Frata.y < 14)
				Game->Frata.y = 14;
			break;

	}

}

/*
 * Van Le Frata, o grande esquivador de buracos
 */
int main(int argc, const char* argv[]) {

	(void) argc; (void) argv;

	setlocale(LC_CTYPE, "");

	GameData Game;

	InitScreen();

	InitData(&Game);
	OpenFile(&Game);

	/* Atualiza a tela a cada 10 ms */
	while (tb_peek_event(&(Game.Event), 10) != -1) {

		/* Limpa a tela */
		ClearScreen();

		/* Recebe o input do usuário */
		HandleInput(&Game);

		/* Desenha alguma tela */
		DrawScreen(&Game);

		/* Renderização */
		tb_render();

	}

	Quit(&Game);

	return 0;

}
