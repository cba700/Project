#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <time.h>
#include <windows.h>
#include<conio.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#define FALSE 0
#define TRUE 1
#define MAP_X_MAX 140
#define MAP_Y_MAX 47//맵 가로세로 직경
#define FLOOR_Y 38//바닥 좌표값
#define FLOOR_Y_1 29//바닥 좌표값 
#define FLOOR_Y_2 20//바닥 좌표값 
#define FLOOR_Y_3 11//바닥 좌표값
#define OBJECT_MAX 32//오브젝트 최대 개수
#define SPAWN_TIME 15000//몬스터 리스폰 시간 15초마다 리스폰
#define BASE_Y 39 //UI Y좌표 설정을 위함

int stage_num = 0;      

enum ColorType {
    BLACK,     //0
    darkBLUE,   //1
    DarkGreen,   //2
    darkSkyBlue,    //3
    DarkRed,     //4
    DarkPurple,   //5
    DarkYellow,   //6
    GRAY,      //7
    DarkGray,   //8
    BLUE,      //9
    GREEN,      //10
    SkyBlue,   //11
    RED,      //12
    PURPLE,      //13
    YELLOW,      //14
    WHITE   //15
} COLOR;

typedef struct _Character {
    short coord[2], size[2];   //좌표값과 사이즈 값
    float accel[2], flyTime;   //가속도와 중력 구현
    bool direction;   //true=right, false=left
    //stat
    char name[16];
    double time;   //시간 = 생명
    short power, weapon, item;

    //animation control
    short motion[4];//       motion[1]:attack여부 bool값 ,  leg_motion, attack_motion(1, 2, 3), invincibility motion
    unsigned int tick[6];//  tick[0]: gen 제어용 ,tick[1]:leg motion 제어용 ,  tick[2]:attack motion 제어용       //gen_tick, leg_tick, atk_tick, dash_tick, invincibility tick//각 모션마다 시간을 재는 배열
    unsigned int item_tick[4]; // 아이템 시간 재주는 tick배열
    short jump_y_max, is_jumping = 2;
    short skill_set;//스킬 변신 변수
}Character;

typedef struct _Object {   //enemies, projectiles, etc.
    short coord[2], size[2];//size[0]: 
    float accel[2], flyTime;//가속도와 중력 구현
    bool direction;

    short kind;   //1~99: items, 100~199: enemies, 200~: projectiles
    double time;   //hp: this value is used randomly for item or particle object
    short attack;
    short damage;
    short originPos[2];
    short characterCoord[2];//공격 시점의 플레이어 위치를 기억하기 위한 필드

    short motion[3];   //motion
    unsigned int tick[5];   //0: 객체의 시간(체력) 관리용 틱 1:
    short jump_y_max, is_jumping = 4;
}Object;

Character character = { {MAP_X_MAX / 2, MAP_Y_MAX / 2}, {3, 3}, {0, 0}, 0, 1, "", 600, 10, 0, 0, {0, 1, 0, 0}, {0, 0, 0, 0, 0} , {0,0,0,0} };

Object** objects;

int kill = 0;
int kill_goal = 0;
int enemy_count = 0;
int enemy_max = 0;
unsigned int tick = 0;//시스템 틱(메인의 반복문과, 각종 메소드에서 참조함)
unsigned int spon_tick = 0;
char sprite_floor[3][MAP_X_MAX];
int floor_coord[3];
char mapData[MAP_X_MAX * MAP_Y_MAX + 1];
char floorData[MAP_X_MAX * MAP_Y_MAX + 1];
int is_key_up_pressed = 0;

bool existTimeKey = 0;
bool mob2_switch = FALSE;
bool mob3_switch = FALSE;
bool isPoisoned = FALSE;

/*보스관련 전역변수*/

bool isUnlocked = TRUE;
bool isVisible = FALSE; //
int isVisible_first = 0; //
short flash_arr[10][2] = { {15,7},{85,7},{133,7},{26,16},{110,16},{66,16},{72,16},{13,25},{82,25},{124,25} }; // 뽀샤시 좌표 미리 만들어둠
short flash_1[2];// 보스 뽀샤시 위치
short flash_2[2];// 보스 뽀샤시 위치
short flash_3[2];// 보스 뽀샤시 위치
int flash_index1 = -1, flash_index2 = -1, flash_index3 = -1;
short flash_animation = 0;

int flash_cnt1 = 0, flash_cnt2 = 0, flash_cnt3 = 0;//
short flash_size[2] = { 3,3 };//
unsigned int flash_tick;//
unsigned int bossAttackTermTick;
unsigned int summons_tick[2];//
unsigned int raser_tick[2];//
unsigned int raserPos_x[2];
unsigned int raserPos_y[2];
unsigned int first_attack = 0;

short bossGenCnt = 0;//처음 소환시만 참고

int snitch_next_x = 10;
int snitch_next_y = 10;
float snitch_accel_x = 0;
float snitch_accel_y = 0;
int snitch_hit_count = 0;
int snitch_hit_count_max = 1;
bool snitch_hit = FALSE;

int Mob2Bullet = 0;
int Mob3Bullet = 0;

//체력, x크기, y크기, 
const short stat_enemy[4][7] =
{ {150, 3, 3, 0, 1000, 0, 0},//100: 근접몹
 {300, 3, 4, 0, 0, 0, 2000},//101: 시간키
 {300, 4, 3, 0, 0, 0, 0}, //102: 기관총
{300, 5, 3, 0, 0, 0, 0} };//103: 원거리몹

const char sprite_character[10] = " 0  | _^_";//캐릭터 디자인
const char sprite_character_leg[2][3][4] =//캐릭터 이동 애니메이션
{ {"-^.", "_^\'", "_^."},
 {".^-", "\'^_", ".^_"} };

const char sprite_weapon[3][2][4] =//캐릭터 무기 종류
{ { "<==", "==>" } ,//칼
{ "==\\", "/==" },//총
{ "@==", "==@" }//넉백
};

const char sprite_itemBox[11] = "| ? ||___|";//아이템 랜덤 박스
const char sprite_item[4][15] = { "---\\ // \\---","bbbbbbbbbbbb","cccccccccccc","dddddddddddd" }; //아이템 1~4번 생김새 임시 버전

int have_item = -1; //아이템 획득 감지 
int use_item[4] = { 0,0,0,0 }; //아이템들 사용 감지
const char sprite_invenWeapon[16] = "|//|";

const char sprite_enemy1[10] = " @ (!)_^_";//근접 몬스터 생김새
const char sprite_enemy1_leg[2][3][4] =//캐릭터 이동 애니메이션
{ {"-^.", "_^\'", "_^."},
 {".^-", "\'^_", ".^_"} };

const char sprite_enemy2[15] = "---\\ // \\---";//시간 키 생김새
const char sprite_enemy2L[13] = { "---    =--- " };//기관총 생김새
const char sprite_enemy2R[13] = { " ---=    ---" };//기관총 생김새
const char sprite_enemy2_bullet[2] = "*";//몹2 원거리 투사체
const char sprite_enemy3_bullet[2] = "o";//몹3 원거리 투사체
const char sprite_enemy3[16] = "+@@@+@o:o@ @#@ ";//유령 몬스터 생김새

const char sprite_boss_on[71] = "  ------    \\    /   __\\  /__ ////||/\\\\\\// /  \\ \\\\/ /    \\ \\  ------  ";
const char sprite_boss_off[71] = "  ------    \\    /   __\\  /__ ////||/\\\\\\// /  \\ \\\\/ /    \\ \\  ------  ";

const char sprite_boss_flash[4][15] = { "      `'.';`..","      .'`.;`.;","      `'.;`:`.", "\\ // \\" }; //

const char skill_image[2][4][33] =
{ {"   /\\      ||      ||    ==O    ", "  = o    = (|)  =  _^_          ", "   ^      / \\    /   \\  <-----> ", "++++++++++++++++++++++++        "},
{"        /===< * ||              ", "    --- --/    |--\\    |    --- ", "   ^      / \\    /   \\  <-----> ", "      @     / /  __/_/__  /_/   "} };
int time_sword = 0;
int use_time_sword = 0;
short time_sword_goal = 3;

int bossSkillTick = 0;
int bossSkillIsRazer = FALSE;
int bossSkillIsDash = FALSE;
int isStunned = FALSE;
int stunTick = 0;

/*UI*/
void DrawSprite(short x, short y, short size_x, short size_y, const char spr[]);   //draw sprite of size_x, size_y at x, y coordinates
void FillMap(char str[], char str_s, int max_value);   //array initialization
void EditMap(short x, short y, char str);   // edit x, y coordinate mapdata
void DrawBox(short x, short y, short size_x, short size_y);   //draw box of size_x, size_y at x, y coordinates
void DrawNumber(short x, short y, int num);   //draw numbers at x, y coordinates (align left)
void ControlUI();
void TextColor(int colorNum);
void GameIntro();
void GameManual();
void GameStory();


/*SYSTEM*/
void StartGame();   //=initialize
int NumLen(int num);   //return length of number
void UpdateGame();
void ExitGame();
void SetConsole();
bool CollisionCheck(short coord1[], short coord2[], short size1[], short size2[]);   //check collision
void SetCurrentCursorPos(int x, int y);
void ControlGravity(short coord[], short size[], short* is_jumping, short* jump_y_max, int index);
void WIN();
void LOSE();
void CLEAR();
void FindBossFlash();//


/*NPC*/
void ControlObject();
void ControlEnemy(int index);
void ControlTimeKey(int index);
void MoveControl(short coord[], float accel[], short size[], float* flyTime);   // motion control
void CreateObject(short x, short y, short kind);
void RemoveObject(int index);
bool EnemyPositionX(short x, short size_x);   //direction the enemy looks at the character
bool EnemyPositionY(short y, short size_y);
short Distance(short  x1, short  y1, short x2, short  y2);
void ControlMob2(int index);
void ControlMob2Bullet(int index);
void ControlMob3(int index);
void ControlMob3Bullet(int index);

void ControlBoss(int index);//
void BossAttackRazer(int index);//
void BossAttackSummons(int index);//

void BossDash(int index);


/*PC*/
void ControlCharacter();
void ControlItem(int index);
void ControlBullet(int index);
void ControlBomb(int index);
void ControlSmog(int index);

int main() {
    kill_goal = 5;
    enemy_max = 4;

    start:
    SetConsole();

    GameIntro();
    GameStory();
    StartGame();

    while (TRUE) {
        snitch_hit_count_max = stage_num + 1;

        if (tick + 30 < GetTickCount()) {
            tick = GetTickCount();//GetTickCount함수는 메인에서만 호출된다.(즉, 메소드에서는 호출안된다)

            UpdateGame();

            if (tick == 0 || kill >= kill_goal)
                break;
        }
    }
    ExitGame();
    return 0;
}



void StartGame() {//게임 시작 함수 게임 시작전 맵 배치와 설저을 만진다.
    TextColor(WHITE);
    //SetConsole();
    srand((unsigned int)time(NULL));

    //printf("Enter your name: ");
    //scanf("%[^\n]s", character.name);
    strcpy(character.name, "SEJONG");

    FillMap(floorData, ' ', MAP_X_MAX * MAP_Y_MAX);
    if (stage_num == 0) {
        PlaySound(TEXT("1.wav"), NULL, SND_ASYNC | SND_SYNC);
        for (int i = 0; i < MAP_X_MAX; i++) {
            floorData[(FLOOR_Y - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 15; i < MAP_X_MAX / 5 * 2 + 10; i++) {
            floorData[(FLOOR_Y_1 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = MAP_X_MAX / 5 * 3 - 10; i < MAP_X_MAX - 15; i++) {
            floorData[(FLOOR_Y_1 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 25; i < MAP_X_MAX / 5 * 2 + 5; i++) {
            floorData[(FLOOR_Y_2 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = MAP_X_MAX / 5 * 3 - 5; i < MAP_X_MAX - 25; i++) {
            floorData[(FLOOR_Y_2 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 35; i < MAP_X_MAX / 5 * 2; i++) {
            floorData[(FLOOR_Y_3 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = MAP_X_MAX / 5 * 3; i < MAP_X_MAX - 35; i++) {
            floorData[(FLOOR_Y_3 - 1) * MAP_X_MAX + i] = '=';
        }

    }

    else if (stage_num == 1) {
        PlaySound(TEXT("2.wav"), NULL, SND_ASYNC | SND_SYNC);
        for (int i = 0; i < MAP_X_MAX; i++) {
            floorData[(FLOOR_Y - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 0; i < MAP_X_MAX / 7 * 2; i++) {
            floorData[(FLOOR_Y_1 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 75; i < MAP_X_MAX / 5 * 4; i++) {
            floorData[(FLOOR_Y_1 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = MAP_X_MAX / 15 * 14; i < MAP_X_MAX; i++) {
            floorData[(FLOOR_Y_1 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 10; i < MAP_X_MAX / 5; i++) {
            floorData[(FLOOR_Y_2 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 50; i < MAP_X_MAX / 3 * 2; i++) {
            floorData[(FLOOR_Y_2 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 110; i < 125; i++) {
            floorData[(FLOOR_Y_2 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 20; i < MAP_X_MAX / 3 + 5; i++) {
            floorData[(FLOOR_Y_3 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 90; i < MAP_X_MAX / 3 * 2 + 15; i++) {
            floorData[(FLOOR_Y_3 - 1) * MAP_X_MAX + i] = '=';
        }
    }

    else if (stage_num == 2) {
        PlaySound(TEXT("3.wav"), NULL, SND_ASYNC | SND_SYNC);
        for (int i = 0; i < MAP_X_MAX / 3 * 2 - 4; i++) {
            floorData[(FLOOR_Y - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = MAP_X_MAX / 3 * 2 + 4; i < MAP_X_MAX; i++) {
            floorData[(FLOOR_Y - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 10; i < MAP_X_MAX / 7 * 2 - 10; i++) {
            floorData[(FLOOR_Y_1 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 54; i < 62; i++) {
            floorData[(FLOOR_Y_1 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 80; i < MAP_X_MAX / 5 * 4 - 5; i++) {
            floorData[(FLOOR_Y_1 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = MAP_X_MAX / 15 * 14 - 10; i < MAP_X_MAX - 15; i++) {
            floorData[(FLOOR_Y_1 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 20; i < 30; i++) {
            floorData[(FLOOR_Y_2 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 60; i < 75; i++) {
            floorData[(FLOOR_Y_2 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 105; i < 115; i++) {
            floorData[(FLOOR_Y_2 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 0; i < 20; i++) {
            floorData[(FLOOR_Y_3 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 80; i < 90; i++) {
            floorData[(FLOOR_Y_3 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = MAP_X_MAX - 20; i < MAP_X_MAX; i++) {
            floorData[(FLOOR_Y_3 - 1) * MAP_X_MAX + i] = '=';
        }
    }

    /*보스 스테이지*/
    else if (stage_num == 3) {
        PlaySound(TEXT("4.wav"), NULL, SND_ASYNC | SND_SYNC);
        for (int i = 30; i < 45; i++) {
            floorData[(FLOOR_Y - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 65; i < 81; i++) {
            floorData[(FLOOR_Y - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 10; i < 30; i++) {
            floorData[(FLOOR_Y_1 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 54; i < 62; i++) {
            floorData[(FLOOR_Y_1 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 80; i < 108; i++) {
            floorData[(FLOOR_Y_1 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 120; i < 132; i++) {
            floorData[(FLOOR_Y_1 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 20; i < 30; i++) {
            floorData[(FLOOR_Y_2 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 42; i < 48; i++) {
            floorData[(FLOOR_Y_2 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 60; i < 75; i++) {
            floorData[(FLOOR_Y_2 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 105; i < 115; i++) {
            floorData[(FLOOR_Y_2 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 0; i < 20; i++) {
            floorData[(FLOOR_Y_3 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 32; i < 45; i++) {
            floorData[(FLOOR_Y_3 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 80; i < 90; i++) {
            floorData[(FLOOR_Y_3 - 1) * MAP_X_MAX + i] = '=';
        }
        for (int i = 120; i < 140; i++) {
            floorData[(FLOOR_Y_3 - 1) * MAP_X_MAX + i] = '=';
        }

        character.coord[0] = 100;
        character.coord[1] = 15;
        enemy_max = 0;
    }

    objects = (Object**)malloc(sizeof(Object*) * OBJECT_MAX);//사용할 오브젝트를 미리 선언
    memset(objects, 0, sizeof(Object*) * OBJECT_MAX);//전부 0으로 대입
}

void UpdateGame() {//계속 실행되는 함수,(1) 각 모듈 control함수의 호출, (2) 몸 생성, (3)맵 그리기
    ControlUI();   //update mapData(UI)

    ControlObject();   //update mapData(enemy, projecticles, etc...)
    ControlCharacter();   //update mapData(character)

    /*generate Enemy*/
    if (enemy_count < enemy_max && stage_num != 3) {                       //1. 근접 몹(100)
        CreateObject(rand() % 90, 35 - 10 * (rand() % 2), 100);
        enemy_count++;
    }
    if (existTimeKey == FALSE && stage_num != 3) {                         //2. 시간의 조각(101)
        int x = rand() % 90, y = 35;
        CreateObject(x, y, 202);
        existTimeKey = TRUE;//시간의 조각 깨면 FALSE로 변경
    }
    if (mob2_switch == FALSE &&stage_num == 1) {                          //3. 기관총(102)
        CreateObject(0, 17, 102);//L                 
        CreateObject(MAP_X_MAX - 4, 17, 102);//R
        mob2_switch = TRUE;
    }
    if (mob2_switch == FALSE && stage_num == 2) {                          //3. 기관총(102)
        CreateObject(0, 17, 102);//L                 
        CreateObject(MAP_X_MAX - 4, 17, 102);//R
        CreateObject(35, FLOOR_Y - 2, 102);//아래
      
        mob2_switch = TRUE;
    }
    if (enemy_count < enemy_max && mob3_switch == FALSE && stage_num>1 && stage_num != 3) {//4. 원거리 몹(103)    //일단 한번만 생성 스테이지 당 한마리 죽이면 더이상 생성되지 않음
        int x = rand() % 90, y = 35;
        CreateObject(x, y, 103);
        mob3_switch = TRUE;
    }

    if (stage_num == 3 && isUnlocked == TRUE && bossGenCnt == 0) {                          //4. 보스(104)
        CreateObject(50, 50, 203);//           
        bossGenCnt++;
    }

    if (character.tick[0] + 1000 < tick) {//decrease character time
        character.tick[0] = tick;
        character.time -= 1;
    }

    SetCurrentCursorPos(0, 0);
    if (character.tick[4] % 20 >= 10) {//데미지 입었을 시
        char* temp;
        temp = (char*)malloc(sizeof(char) * (MAP_X_MAX * MAP_Y_MAX + 1));
        strcpy(temp, mapData);

        temp[MAP_X_MAX * FLOOR_Y] = '\0';
        fputs(temp, stdout);
        printf(" ");
        SetCurrentCursorPos(0, FLOOR_Y);
        if (isPoisoned == FALSE) TextColor(RED);
        else  TextColor(PURPLE);

        fputs(mapData + MAP_X_MAX * FLOOR_Y, stdout);
        TextColor(WHITE);
        free(temp);
    }
    else {
        if (snitch_hit == TRUE) {
            TextColor(YELLOW);
            fputs(mapData, stdout);
            Sleep(1000);
            snitch_hit = FALSE;
            TextColor(WHITE);
        }
        else fputs(mapData, stdout);
    }

}

void ExitGame() {//게임 종료 함수
    for (int i = 0; i < OBJECT_MAX; i++) {//배열을 돌면서 free처리
        if (objects[i])
            free(objects[i]);
    }
    free(objects);
}

void SetConsole() {
    char command[256] = { '\0', };
    sprintf(command, "mode con: lines=%d cols=%d", MAP_Y_MAX, MAP_X_MAX);
    system(command);
    system("title TICKTOCK in TIME ");

    HANDLE hConsole;
    CONSOLE_CURSOR_INFO ConsoleCursor;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    ConsoleCursor.bVisible = 0;
    ConsoleCursor.dwSize = 1;
    SetConsoleCursorInfo(hConsole, &ConsoleCursor);
}

void ControlUI() {//UI그리기
    memcpy(mapData, floorData, 1 + sizeof(char) * MAP_X_MAX * MAP_Y_MAX);

    DrawBox(1, BASE_Y, 96, 9);//화면 틀 그리기
    DrawBox(1, BASE_Y, MAP_X_MAX, 9);//화면 틀 그리기

    DrawSprite(8, BASE_Y + 2, 5, 1, (char*)"NAME:\"");   //이름 그리기
    DrawSprite(13, BASE_Y + 2, strlen(character.name), 1, character.name);

    DrawSprite(8, BASE_Y + 5, 6, 1, (char*)"TIME:");   //시간 그리기
    DrawNumber(13, BASE_Y + 5, character.time / 60);
    DrawSprite(15, BASE_Y + 5, 1, 1, (char*)":");
    DrawNumber(16, BASE_Y + 5, (int)character.time % 60);
    /*
    DrawSprite(8, BASE_Y + 6, 6, 1, (char*)"KILL:"); //KILL 그리기
    DrawNumber(13, BASE_Y + 6, kill);
    DrawSprite(15, BASE_Y + 6, 1, 1, (char*)"/");
    DrawNumber(16, BASE_Y + 6, kill_goal);
    */

    for (int x = 32; x <= 84; x += 4) {
        DrawBox(x, BASE_Y + 1, 10, 6);
        x += 4;
        if (x == 36) DrawSprite(x, BASE_Y + 7, 1, 1, (char*)"Q");
        else if (x == 48) DrawSprite(x, BASE_Y + 7, 1, 1, (char*)"W");
        else if (x == 60) DrawSprite(x, BASE_Y + 7, 1, 1, (char*)"E");
        else if (x == 72) DrawSprite(x, BASE_Y + 7, 1, 1, (char*)"R");
        else if (x == 84) {
            if (have_item >= 0 && have_item < 4) // 아이템 먹었으면 
                DrawSprite(x, BASE_Y + 2, 3, 4, sprite_item[have_item]);
            else if (have_item == -1) // 아이템 안먹었으면 기존의 표시
                DrawSprite(x, BASE_Y + 3, 2, 2, sprite_invenWeapon);
            DrawSprite(x, BASE_Y + 7, 1, 1, (char*)"T");
        }
        x += 4;
    }

    DrawSprite(33, BASE_Y + 2, 8, 4, skill_image[character.skill_set][0]);
    DrawSprite(45, BASE_Y + 2, 8, 4, skill_image[character.skill_set][1]);
    DrawSprite(57, BASE_Y + 2, 8, 4, skill_image[character.skill_set][2]);
    if (time_sword == 0) {
        DrawSprite(69, BASE_Y + 2, 8, 4, skill_image[time_sword][3]);
        DrawNumber(70, BASE_Y + 5, kill);
        DrawSprite(72, BASE_Y + 5, 1, 1, (char*)"/");
        DrawNumber(74, BASE_Y + 5, time_sword_goal);
    }
    else {
        DrawSprite(69, BASE_Y + 2, 8, 4, skill_image[time_sword][3]);
    }
    DrawSprite(111, BASE_Y + 5, 15, 1, (char*)"T I C K T O C K");
    DrawSprite(111, BASE_Y + 6, 15, 1, (char*)"      I N      ");
    DrawSprite(111, BASE_Y + 7, 15, 1, (char*)"    T I M E    ");
    DrawSprite(117, BASE_Y + 1, 3, 4, sprite_item[0]);

    //TP (TIME piece)
    if (stage_num != 3) {
        DrawSprite(137, BASE_Y + 1, 2, 1, (char*)"TP");
        for (int i = 0; i < snitch_hit_count_max; i++) {
            if (i < snitch_hit_count) DrawSprite(137, BASE_Y + 3 + i, 2, 1, (char*)"●");
            else DrawSprite(137, BASE_Y + 3 + i, 2, 1, (char*)"○");

        }
    }


}

void ControlCharacter() {//캐릭터 조종 함수

    ControlGravity(character.coord, character.size, &character.is_jumping, &character.jump_y_max, 0);

    bool move = FALSE, attack = FALSE;//FALSE일 경우 해당 상태가 아님
    int x = character.coord[0], y = character.coord[1];//좌표값 대입

    use_time_sword = 0;

    if (character.time < 1) {//체력이 없으면 게임 종료 설정
        tick = 0;
        LOSE();
        return;
    }
    if (character.tick[4] > 0) {
        character.tick[4]--;
    }

    ///*공격 입력 시 attack 켜기기*/

    // 스킬 T 사용
    if (GetAsyncKeyState(0x54) & 0x8000) {

        if (have_item == 0) { //1번 아이템 이면
            use_item[have_item] = 1;
            character.item_tick[0] = GetTickCount(); //조건문 없이 일반화 시키고 싶은데 item.tick[have_item] 으로 하면 캐릭터가 아이템 사용시 계속 깜박임
            have_item = -1;
            character.item = 0;
        }

        else if (have_item == 1) { //2번 아이템 사용
            use_item[have_item] = 1;
            have_item = -1;
            character.item = 0;
        }
        else if (have_item == 2) { //3번 아이템 사용
            use_item[have_item] = 1;
            have_item = -1;
            character.item = 0;
        }
        else if (have_item == 3) { //4번 아이템 사용
            use_item[have_item] = 1;
            have_item = -1;
            character.item = 0;
        }
    }


    //1번 아이템 사용 관련
    if (tick > character.item_tick[0] + 5000) {
        use_item[0] = 0; //1번아이템 -> 인덱스0  = 시간정지 아이템
    }//5초동안 시간 정지

    /*attack motion 관리*/
    if (character.motion[1]) {// 가장 바깥에 모션1을 둠으로써 150 간격 공격모션 제어 가능

        if (tick > character.tick[2] + 100) {   //틱이 150 벌어지면 틱 갱신, 모션[2] 올려줌
            character.tick[2] = tick;
            character.motion[2]++;
        }

        if (character.motion[2] > 3) {
            if (attack) {
                character.motion[2] = 1;
                character.motion[3]++;
            }
            else {
                character.motion[1] = FALSE;
                character.motion[2] = 0;
                character.motion[3] = 1;
            }

            if (character.motion[3] > 3)
                character.motion[3] = 1;
        }
    }

    if (GetAsyncKeyState(VK_LEFT) & 0x8000 && x > 1 && isStunned == FALSE) {   //move left 
        if (character.accel[0] > -1)
            character.accel[0] = -1;
        character.direction = FALSE;
        move = TRUE;
        for (int i = 0; i < character.size[1]; i++) {
            if (floorData[MAP_X_MAX * (character.coord[1] - i) + character.coord[0] - 1] == '=') character.accel[0] = 0;
        }
    }

    if (GetAsyncKeyState(VK_RIGHT) & 0x8000 && x < MAP_X_MAX - 2 && isStunned == FALSE) {   //move right 
        if (character.accel[0] < 1)
            character.accel[0] = 1;
        character.direction = TRUE;
        move = TRUE;
        for (int i = 0; i < character.size[1]; i++) {
            if (floorData[MAP_X_MAX * (character.coord[1] - i) + character.coord[0] + character.size[0]] == '=') character.accel[0] = 0;
        }
    }

    if (GetAsyncKeyState(0x45) & 1)//변신 사용
        character.skill_set = (character.skill_set == 0) ? 1 : 0;

    if (GetAsyncKeyState(0x51) & 0x8000 && character.skill_set == 0) {//Q사용 칼 사용
        character.weapon = 0;
        attack = TRUE;
        character.motion[1] = TRUE;
    }

    if (GetAsyncKeyState(0x51) & 0x8000 && character.tick[5] + 600 <= tick && character.skill_set == 1) {//Q사용 총 사용
        character.weapon = 1;
        attack = TRUE;
        character.motion[1] = TRUE;
        character.tick[5] = tick;
        CreateObject(character.coord[0], character.coord[1], 200);
    }

    if (GetAsyncKeyState(0x57) & 0x8000 && character.tick[3] + 1200 <= tick && character.skill_set == 0) {//W사용 대쉬 사용
        character.accel[0] = character.direction * 6 - 3;
        character.tick[3] = tick;
    }

    if (GetAsyncKeyState(0x57) & 0x8000 && character.skill_set == 1) {//W사용 넉백
        character.weapon = 2;
        attack = TRUE;
        character.motion[1] = TRUE;
    }
    if (GetAsyncKeyState(0x52) & 0x8000 && time_sword == 1) {//R사용 시간의 검 사용
        character.weapon = 0;
        use_time_sword = 1;
        attack = TRUE;
        character.motion[1] = TRUE;
    }
    //최종 버전시 삭제 예정 테스트시에만 사용할 것
    if (GetAsyncKeyState(0x5A) & 0x8000) {//Z사용 폭탄 사용
        attack = TRUE;
        character.motion[1] = TRUE;
        character.tick[5] = tick;
        CreateObject(character.coord[0], character.coord[1], 201);
    }
    if (GetAsyncKeyState(VK_UP) & 0x8000 && (character.is_jumping == 0 || character.is_jumping == 2) && is_key_up_pressed == 0 && isStunned == FALSE) {
        character.is_jumping += 1;
        if (character.is_jumping == 1) character.jump_y_max = character.coord[1] - 7;
        else character.jump_y_max = character.coord[1] - 5;
        is_key_up_pressed = 1;
    }

    if ((GetAsyncKeyState(VK_UP) & 0x8000) == 0) {
        is_key_up_pressed = 0;
    }

    /*leg motion 관리*/
    if (tick > character.tick[1] + 90) {   //leg tick   
        character.tick[1] = tick;

        if (move == TRUE)
            character.motion[0]++;
        else
            character.motion[0] = 0;

        if (character.motion[0] > 3)
            character.motion[0] = 1;
    }

    MoveControl(character.coord, character.accel, character.size, &character.flyTime);   // control character movement


    if (character.tick[4] % 4 == 0) {//무적상태 출력 위해서(평소에는 그냥 돌아가고, 데미지 있을 시에는 깜박거림)
        DrawSprite(x + 1, y - 1, character.size[0], character.size[1], sprite_character);   //draw character sprite

        if (character.direction) {
            EditMap(x + 1, y, '(');
        }
        else {
            EditMap(x + 3, y, ')');
        }

        if (character.accel[0] > 1)
            DrawSprite(x - 1, y - 1, 1, 3, "===");

        if (character.accel[0] < -1)
            DrawSprite(x + 5, y - 1, 1, 3, "===");
        if (character.motion[1]) {   //draw attack motion
            if (character.weapon == 0 || character.weapon == 2) {
                if (character.motion[2] == 2) {
                    DrawSprite(x - 5 + 10 * character.direction + 1, y, 3, 1, sprite_weapon[character.weapon][character.direction]);
                }
                else {
                    DrawSprite(x - 3 + 6 * character.direction + 1, y, 3, 1, sprite_weapon[character.weapon][character.direction]);
                }
            }
            else if (character.weapon == 1) {
                if (character.motion[2] == 2 && (character.weapon == 1)) {
                    DrawSprite(x - 3 + 6 * character.direction + 1, y - 1, 3, 1, sprite_weapon[character.weapon][character.direction]);
                }
                else {
                    DrawSprite(x - 3 + 6 * character.direction + 1, y, 3, 1, sprite_weapon[character.weapon][character.direction]);
                }
            }
        }
        else {
            EditMap(x + character.direction * 2 + 1, y, 'o');

            if (character.motion[0] == 3)
                EditMap(x + 2, y, 'l');
        }

        if (character.motion[0] > 0)
            DrawSprite(x + 1, y + 1, 3, 1, sprite_character_leg[character.direction][character.motion[0] - 1]);   //draw leg motion


        if (isStunned == TRUE) {
            if (tick % 2 == 0) {
                DrawSprite(x, y - 1, 1, 3, "<><");
                DrawSprite(x + 4, y - 1, 1, 3, "><>");
            }
            else {
                DrawSprite(x, y - 1, 1, 3, "><>");
                DrawSprite(x + 4, y - 1, 1, 3, "<><");
            }
        }


    }
}

void ControlGravity(short coord[], short size[], short* is_jumping, short* jump_y_max, int index) {
    if (coord[1] <= *jump_y_max) *is_jumping += 1;
    *is_jumping %= 5;

    if (coord[0] == character.coord[0] && coord[1] == character.coord[1]) {
        if (coord[1] > FLOOR_Y) LOSE();
    }

    else {
        if (floorData[MAP_X_MAX * (coord[1] + 1) + coord[0] + size[0]] != '=') coord[0] -= 1;
        if (floorData[MAP_X_MAX * (coord[1] + 1) + coord[0] - 1] != '=') coord[0] += 1;
        if (coord[1] > FLOOR_Y) objects[index]->time = 0;
    }

    int check_bottom = size[0];

    for (int i = 0; i < size[0]; i++) if (*is_jumping % 2 == 0 && (floorData[MAP_X_MAX * (coord[1] + 1) + coord[0] + i]) != '=') check_bottom -= 1;
    if (check_bottom == 0) {
        if (*is_jumping == 0) *is_jumping = 4;
        coord[1] += 1.1;
    }


    for (int i = 0; i < size[0]; i++) {
        if (*is_jumping % 2 != 0) {
            if (floorData[MAP_X_MAX * (coord[1] - size[1]) + coord[0] + i] == '=') *is_jumping = 4;
            if (coord[1] - size[1] + 1 <= 0) {
                coord[1] += 1;
                *is_jumping = 4;
            }
        }
        else {
            if (floorData[MAP_X_MAX * (coord[1] + 1) + coord[0] + i] == '=') *is_jumping = 0;
        }
    }

    if (*is_jumping % 2 != 0) {
        coord[1] -= 0.9;
    }
    if (coord[1] - 2 <= 0) *is_jumping += 1;

}

void ControlItem(int index) {
    short x = objects[index]->coord[0], y = objects[index]->coord[1];
    short item_coord[2] = { x, y };
    short item_size[2] = { 5, 2 };
    objects[index]->size[0] = item_size[0];
    objects[index]->size[1] = item_size[1];

    if (objects[index]->tick[1] < tick) {
        objects[index]->tick[1] = tick * 2;
        objects[index]->accel[0] = 1 - 2 * objects[index]->time / (float)RAND_MAX;
        objects[index]->accel[1] = -2 * objects[index]->time / (float)RAND_MAX;
    }

    if (CollisionCheck(item_coord, character.coord, item_size, character.size)) {
        DrawSprite(x + 1, y - 3, 3, 1, "[A]");// . 상단에 A를 출력하기

        if (GetAsyncKeyState(0x41) & 0x8000) {// .A 누르면 아이템 먹어짐
            character.item = objects[index]->kind;
            have_item = objects[index]->kind; // .아이템  먹음 아이템은 랜덤임
            RemoveObject(index);//해당 아이템 지우기
            return;
        }
    }



    if (objects[index]->is_jumping == 4) {
        objects[index]->coord[1] -= 1;
        objects[index]->is_jumping = 1;
        objects[index]->jump_y_max = objects[index]->coord[1] - 3;
    }
    ControlGravity(objects[index]->coord, objects[index]->size, &objects[index]->is_jumping, &objects[index]->jump_y_max, index);
    MoveControl(objects[index]->coord, objects[index]->accel, objects[index]->size, &objects[index]->flyTime);

    DrawSprite(x, y, 5, 2, sprite_itemBox);// 아이템 계속 화면에 그려주기
}

void CreateObject(short x, short y, short kind) {//오브젝트 생성 함수
    int index = 0;
    Object* obj = 0;

    while (TRUE) {//빈 오브젝트 배열을 찾을 때까지 반복
        if (!objects[index])
            break;
        if (index == OBJECT_MAX)
            return;
        index++;
    }

    obj = (Object*)malloc(sizeof(Object));//현재 빈 object에 동적할당
    objects[index] = obj;
    memset(obj, 0, sizeof(Object));//오브젝트를 선언하고 배열에 대입

    obj->kind = kind;
    obj->coord[0] = x; obj->coord[1] = y;
    obj->tick[0] = 0;//각 값에 맞는 값을 대입

    if (kind == 198) {//몹3 총알
        obj->time = rand();
        obj->size[0] = 1;
        obj->size[1] = 1;
        obj->tick[1] = GetTickCount();//
        obj->tick[2] = GetTickCount();//제거 타이밍 잡기용 틱
        obj->damage = 10;
        obj->motion[0] = 0;
        obj->motion[1] = 0;
        obj->originPos[0] = x;//생성 당시 몹3 위치
        obj->originPos[1] = y;
        obj->characterCoord[0] = character.coord[0];//생성 당시 플레이어 위치
        obj->characterCoord[1] = character.coord[1];
    }

    if (kind == 199) {//몹2 총알
        obj->time = rand();
        obj->size[0] = 1;
        obj->size[1] = 1;
        obj->tick[1] = GetTickCount();//
        obj->tick[2] = GetTickCount();//제거 타이밍 잡기용 틱
        obj->damage = 10;
        obj->motion[0] = 0;
        obj->motion[1] = 0;
        obj->originPos[0] = x;//생성 당시 몹2 위치
        obj->originPos[1] = y;
        obj->characterCoord[0] = character.coord[0];//생성 당시 플레이어 위치
        obj->characterCoord[1] = character.coord[1];
    }

    if (kind == 200) {//총알
        obj->time = rand();
        obj->coord[0] = character.coord[0] - 4 + 10 * character.direction;
        obj->coord[1] = character.coord[1] - 1;
        obj->size[0] = 1;
        obj->size[1] = 1;
        obj->tick[1] = GetTickCount();
        obj->direction = character.direction;
    }

    if (kind == 201) {//폭탄
        obj->time = rand();
        obj->coord[0] = character.coord[0] - 4 + 10 * character.direction;
        obj->coord[1] = character.coord[1] - 1;
        obj->size[0] = 1;
        obj->size[1] = 1;
        obj->tick[1] = GetTickCount();
        obj->direction = character.direction;
    }

    if ((kind < 100 || kind > 199) && kind != 203) {//동전이나 아이템일 경우
        obj->time = rand();
        obj->tick[1] = 0;
        obj->tick[2] = 0;
        obj->tick[3] = 0;
        obj->is_jumping = 4;
    }

    if (kind > 99 && kind < 198) {//몬스터일 경우
        obj->time = stat_enemy[kind - 100][0];
        obj->size[0] = stat_enemy[kind - 100][1];
        obj->size[1] = stat_enemy[kind - 100][2];
        obj->tick[1] = stat_enemy[kind - 100][3];
        obj->tick[2] = stat_enemy[kind - 100][4];
        obj->tick[3] = stat_enemy[kind - 100][5];
        obj->tick[4] = stat_enemy[kind - 100][6];
        obj->motion[0] = 0;
    }
    if (kind == 202) {//스니치일 경우
        obj->coord[0] = 10;
        obj->coord[1] = 10;
        obj->accel[0] = 10.0;
        obj->accel[1] = 10.0;
        obj->time = 660;
        obj->size[0] = 3;
        obj->size[1] = 4;
        obj->tick[1] = 0;
        obj->tick[2] = 0;
        obj->tick[3] = 0;
        obj->tick[4] = 3000;
        obj->motion[0] = 0;
    }
    if (kind == 203) {//보스일 경우
        obj->coord[0] = 20;
        obj->coord[1] = 10;
        obj->time = 2000;
        obj->size[0] = 10;
        obj->size[1] = 7;
        obj->tick[0] = 0;
        obj->tick[1] = 0;
        obj->tick[2] = 0;
        obj->tick[3] = 0;
        obj->tick[4] = 10000;
    }
}

void ControlObject() {//오브젝트 종류에 따른 확인
    for (int i = 0; i < OBJECT_MAX; i++) {
        if (objects[i]) {
            if (objects[i]->kind < 100)//kind가 99까지는 ControlItem함수 실행
                ControlItem(i);
            else if (objects[i]->kind < 198)//kind가 100부터 199까지는 ControlEnemy함수 실행
                ControlEnemy(i);
            else if (objects[i]->kind == 199)//199: mob2원거리 공격
                ControlMob2Bullet(i);
            else if (objects[i]->kind == 198)//198: mob3원거리 공격
                ControlMob3Bullet(i);
            else if (objects[i]->kind == 200)
                ControlBullet(i);
            else if (objects[i]->kind == 201)
                ControlBomb(i);
            else if (objects[i]->kind == 202)
                ControlTimeKey(i);
            else if (objects[i]->kind == 203)
                ControlBoss(i);
        }
    }
}

void ControlTimeKey(int index) {
    short x = objects[index]->coord[0], y = objects[index]->coord[1];//몬스터 좌표
    short at_coord[2] = { character.coord[0] - 5 + 8 * character.direction, character.coord[1] };//캐릭터 좌표
    short at_size[2] = { 5, 3 };//공격 사이즈
    short item_code = rand() % 100;
    short attack = FALSE;

    /*0.몹 사망 처리*/
    if (snitch_hit_count >= snitch_hit_count_max) {
        WIN();
        return;
    }
    else if (objects[index]->time < 1) LOSE();

    /*1.몹 시간 -1초 처리*/
    if (objects[index]->tick[0] + 1000 < tick && use_item[0] != 1) { //아이템1번 사용 여부파악
        objects[index]->tick[0] = tick; //메인의 시점 tick
        objects[index]->time -= 1.0;
    }


    /*2.몹이 공격에 맞았을 경우 처리*/
    if (character.motion[2] == 1 && use_time_sword == 1 && CollisionCheck(objects[index]->coord, at_coord, objects[index]->size, at_size)) {
        objects[index]->tick[0] = tick;
        character.time -= character.power;//추가함
        snitch_hit_count++;
        snitch_hit = TRUE;// 맞는 순간 표현위해

        //스니치 텔레포트
        objects[index]->coord[0] = rand() % (MAP_X_MAX - objects[index]->size[0]) + objects[index]->size[0];
        objects[index]->coord[1] = rand() % (BASE_Y - objects[index]->size[1]) + objects[index]->size[1];
        objects[index]->tick[4] -= 5000;

        time_sword = 0;
    }


    if (objects[index]->coord[0] == 0 && objects[index]->coord[1] == 0) {
        objects[index]->coord[0] = 10;
        objects[index]->coord[1] = 10;
        objects[index]->accel[0] = 10.0;
        objects[index]->accel[1] = 10.0;
    }
    if (objects[index]->tick[4] + 5000 < tick) {//다음 좌표로 넘어간다      //tick[4]: 다음자표 설정을 위한 틱



        objects[index]->coord[0] = snitch_next_x;
        objects[index]->coord[1] = snitch_next_y;
        objects[index]->accel[0] = (float)snitch_next_x;
        objects[index]->accel[1] = (float)snitch_next_y;



        objects[index]->tick[4] = tick;
        snitch_next_x = rand() % (MAP_X_MAX - objects[index]->size[0]) + objects[index]->size[0];  //새 좌표 받음
        snitch_next_y = rand() % (BASE_Y - objects[index]->size[1]) + objects[index]->size[1];
        if (snitch_next_x < 10) snitch_next_x += 10;
        if (snitch_next_y < 10) snitch_next_y += 10;
        snitch_accel_x = (snitch_next_x - objects[index]->coord[0]) / 160.0;
        snitch_accel_y = (snitch_next_y - objects[index]->coord[1]) / 160.0;
        objects[index]->motion[0] = 0;
    }
    else {
        objects[index]->accel[0] += (float)snitch_accel_x;
        objects[index]->accel[1] += (float)snitch_accel_y;
    }
    /*시간이 지나 타임키가 다른 위치로 이동하는 경우: 해당 좌표로 이동, 5초 안에 가있어야함*/

    objects[index]->coord[0] = (int)objects[index]->accel[0];
    objects[index]->coord[1] = (int)objects[index]->accel[1];

    DrawSprite(objects[index]->coord[0] + 1, objects[index]->coord[1] - 2, objects[index]->size[0], objects[index]->size[1], sprite_enemy2);
    /*몹의 시간 출력(머리 위)*/
    DrawNumber(objects[index]->coord[0] + objects[index]->size[0] / 2 - NumLen(objects[index]->time) / 2 + 1, objects[index]->coord[1] - 3, (int)objects[index]->time);
}

void ControlMob1(int index) {
    short x = objects[index]->coord[0], y = objects[index]->coord[1];//몬스터 좌표
    short at_coord[2] = { character.coord[0] - 5 + 8 * character.direction, character.coord[1] };//캐릭터 좌표
    short at_size[2] = { 5, 3 };//공격 사이즈
    short item_code = rand() % 100;
    short attack = FALSE;

    /* 공격검사 */
    if (abs(objects[index]->coord[0] - at_coord[0]) < 13) attack = TRUE;

    /*  몹의 평상 시:
    (1) 좌우를 서성거림
    (2) PC향해 걸어감  */
    if (attack == FALSE && use_item[0] != 1) { // 아이템 1번 체크

        /*(1) 좌우를 서성거림*/
        if (abs(objects[index]->coord[0] - at_coord[0]) > 40) {

            /*방향설정*/
            srand(time(NULL) * (index + 1));
            int moveOrStop = rand() % 3;
            if (moveOrStop == 1) {
                objects[index]->direction = TRUE;

                if (tick > objects[index]->tick[4] + 170) {
                    objects[index]->tick[4] = tick;
                    if (objects[index]->coord[0] < MAP_X_MAX) { objects[index]->coord[0]++; }
                }
                /*leg motion*/
                if (tick > objects[index]->tick[3] + 170) {   //leg tick, 다리 움직임 속도   
                    objects[index]->tick[3] = tick;
                    objects[index]->motion[0]++;
                    if (objects[index]->motion[0] > 3)
                        objects[index]->motion[0] = 1;
                }
            }

            else if (moveOrStop == 0) {
                objects[index]->direction = FALSE;
                if (tick > objects[index]->tick[4] + 170) {
                    objects[index]->tick[4] = tick;
                    if (objects[index]->coord[0] > 0) objects[index]->coord[0]--;
                }
                /*leg motion*/
                if (tick > objects[index]->tick[3] + 170) {   //leg tick, 다리 움직임 속도   
                    objects[index]->tick[3] = tick;
                    objects[index]->motion[0]++;
                    if (objects[index]->motion[0] > 3)
                        objects[index]->motion[0] = 1;
                }
            }
            //else {} //아무것도 안함          
        }

        /*(3) PC를 따라옴*/
        else if (abs(objects[index]->coord[0] - at_coord[0]) > 12 && abs(objects[index]->coord[0] - at_coord[0]) < 41) {

            /*방향설정*/
            if (!EnemyPositionX(x, objects[index]->size[0])) objects[index]->direction = TRUE;
            else  objects[index]->direction = FALSE;

            /*좌표 이동 관리*/
            if (!EnemyPositionX(x, objects[index]->size[0])) {     //이동속도
                if (tick > objects[index]->tick[4] + 170) {
                    objects[index]->tick[4] = tick;
                    objects[index]->coord[0]++;
                }
            }
            else {
                if (tick > objects[index]->tick[4] + 170) {
                    objects[index]->tick[4] = tick;
                    objects[index]->coord[0]--;
                }
            }
            /*leg motion 관리*/
            if (tick > objects[index]->tick[3] + 170) {   //leg tick, 다리 움직임 속도   
                objects[index]->tick[3] = tick;
                objects[index]->motion[0]++;

                if (objects[index]->motion[0] > 3)
                    objects[index]->motion[0] = 1;
            }
        }
    }

    /*(4)몹의 공격 시: 몸통 박치기*/
    if (attack == TRUE && use_item[0] != 1 && character.coord[1] == objects[index]->coord[1]) { //attack == TRUE, 아이템 1번 체크       
        objects[index]->motion[0] = 0;

        if (objects[index]->tick[1] + objects[index]->tick[2] < tick) {//이전 실행시 시간 + 이전 대쉬 시 결정된 랜덤 간격
            objects[index]->tick[1] = tick;
            objects[index]->tick[2] = 1000 + rand() % 1000;// 대쉬하는 시간 간격을 조절
            //objects[index]->accel[1] = rand() / (float)RAND_MAX / 2 - 1.2;//점프

            if (!EnemyPositionX(x, objects[index]->size[0]))
                objects[index]->accel[0] = 2.4 - rand() / (float)RAND_MAX;
            else
                objects[index]->accel[0] = rand() / (float)RAND_MAX - 2.4;
        }
    }

    /*몹과 캐릭터의 충돌 시 데미지 처리*/
    if (character.tick[4] == 0 && CollisionCheck(objects[index]->coord, character.coord, objects[index]->size, character.size)) {//무적 처리 함수
        character.tick[4] = 80;//무적상태로 돌입, 80번의 틱동안 유지(약 3초) 데미지 안 입음
        character.time -= 10;
    }

    MoveControl(objects[index]->coord, objects[index]->accel, objects[index]->size, &objects[index]->flyTime);//몬스터 움직임
    DrawSprite(x + 1, y - 1, objects[index]->size[0], objects[index]->size[1], sprite_enemy1);//근접형 몬스터 그리기

    if (objects[index]->motion[0] > 0)
        DrawSprite(x + 1, y + 1, 3, 1, sprite_enemy1_leg[objects[index]->direction][objects[index]->motion[0] - 1]);   //draw leg motion
}

void ControlMob2(int index) {//
    int x = objects[index]->coord[0];
    int y = objects[index]->coord[1];
    short at_coord[2] = { character.coord[0] - 5 + 8 * character.direction, character.coord[1] };//캐릭터 좌표

    /*좌, 우 생김새 출력*/
    if (x < MAP_X_MAX / 2)
        DrawSprite(x + 1, y - 1, objects[index]->size[0], objects[index]->size[1], sprite_enemy2L);//왼쪽에 설치된 몹2
    else

        DrawSprite(x + 1, y - 1, objects[index]->size[0], objects[index]->size[1], sprite_enemy2R);//오른쪽

    /*공격*/
    if (objects[index]->tick[4] + 4000 < tick) {//tick[3]공격 투사체를 만드는 간격
        objects[index]->tick[4] = tick;

        /*공격시점의 플레이어의 위치를 기억한다(CreateObject).*/
        /*공격 투사체 만들기: 화면 중앙을 기준으로 다른 위치 초기화*/
        if (x < MAP_X_MAX / 2) {
            CreateObject(objects[index]->coord[0] + 4, objects[index]->coord[1], 199); //원거리 몹 투사체 관리, ControlObject에서 호출 //왼쪽 몹 2
            Mob2Bullet++;
        }
        else {
            CreateObject(objects[index]->coord[0] - 4, objects[index]->coord[1], 199); //원거리 몹 투사체 관리, ControlObject에서 호출 //오른쪽 몹 2
            Mob2Bullet++;
        }

    }

}

void ControlMob2Bullet(int index) {
    int x = objects[index]->originPos[0];
    int y = objects[index]->originPos[1];
    short at_coord[2] = { objects[index]->coord[0],objects[index]->coord[1] };
    short pc_coord[2] = { character.coord[0] - 5 + 8 * character.direction, character.coord[1] };//캐릭터 좌표
    short at_size[2] = { 1, 1 };//공격 사이즈(판정 더 어렵게 하기 위해 2 X 2)
    short pc_size[2] = { 3, 3 };//플레이어 사이즈

    if (objects[index]->tick[2] + 2000 < tick) {
        RemoveObject(index);
        Mob2Bullet--;
        return;
    }
    if (objects[index]->tick[1] + 50 < tick) {//tick[1]: 
        objects[index]->tick[1] = tick;

        objects[index]->coord[0] = (((objects[index]->motion[0]) * (objects[index]->characterCoord[0]) + (30 - objects[index]->motion[0]) * x)) / 30; // objects[index]->motion[0] : 10- objects[index]->motion[0] 내분점
        objects[index]->coord[1] = ((objects[index]->motion[1] * (objects[index]->characterCoord[1]) + (30 - objects[index]->motion[1]) * y)) / 30; // objects[index]->motion[0] : 10- objects[index]->motion[0] 내분점

        objects[index]->motion[0]++;                //*motion을 내분점의 i역할로 사용함
        objects[index]->motion[1]++;
        if (objects[index]->motion[0] == 31) objects[index]->motion[0] = 0;
        if (objects[index]->motion[1] == 31) objects[index]->motion[1] = 0;
    }

    /*맵을 벗어날 경우 제거*/
    if (objects[index]->coord[0] < 0 || objects[index]->coord[0] > MAP_X_MAX || objects[index]->coord[1] < 5 || objects[index]->coord[1] > FLOOR_Y) {
        RemoveObject(index);
        Mob2Bullet--;
        return;//객체가 사라졌으므로 메소드 종료시켜야함
    }
    if (character.tick[4] == 0 && CollisionCheck(at_coord, pc_coord, at_size, pc_size)) {//충돌 시 
        character.tick[4] = 80;//무적상태로 돌입, 80번의 틱동안 유지(약 3초) 데미지 안 입음
        character.time -= 25;
        RemoveObject(index);
        Mob2Bullet--;
        return;
    }
    DrawSprite(objects[index]->coord[0], objects[index]->coord[1], 1, 1, "*");
}

void ControlMob3(int index) {//
    short x = objects[index]->coord[0];
    short y = objects[index]->coord[1];
    short at_coord[2] = { character.coord[0] - 5 + 8 * character.direction, character.coord[1] };//캐릭터 좌표
    short  dist = 0;
    bool relativePosWithPC_X = EnemyPositionX(x, 5);
    bool relativePosWithPC_Y = EnemyPositionY(y, 3);
    //오브젝트의 direction필드를 mob3에서만 추적 범위 안에 있는지 여부를 따지는 용도로 사용한다.

    /*1.움직임*/

    /*1-1. 현재 위치 체크, 이동방향 설정*/
    if (objects[index]->tick[1] + 2000 < tick) {//추적 범위 내에 있으면
        objects[index]->tick[1] = tick;
        dist = Distance(x, y, character.coord[0], character.coord[1]);

        if (dist < 25)  objects[index]->direction = TRUE;
        else objects[index]->direction = FALSE;
    }

    if (objects[index]->tick[2] + 100 < tick) {
        objects[index]->tick[2] = tick;

        //relativePosWithPC_X == FALSE: 몹이 PC의 왼쪽에 있음
        //relativePosWithPC_Y == FALSE: 몹이 PC의 아래쪽에 있음

        /*1-2. 플레이어 쪽으로 다가가기*/
        if (objects[index]->direction == FALSE) {

            if (relativePosWithPC_X == TRUE && relativePosWithPC_Y == TRUE) {//(1) pc기준 1사분면 ->3사분면 방향으로 이동
                if (objects[index]->motion[0] % 6 == 2) objects[index]->coord[1]++;// x방향 1칸, y방향1칸, x방향 1칸, x방향 1칸
                else if (objects[index]->motion[0] % 4 == 4 || objects[index]->motion[0] % 4 == 5) {
                    objects[index]->coord[0] += (rand() % 3 - 1);
                    objects[index]->coord[1] += (rand() % 3 - 1);
                    objects[index]->motion[0]++;
                }
                else objects[index]->coord[0]--;
                objects[index]->motion[0]++;
            }
            else if (relativePosWithPC_X == TRUE && relativePosWithPC_Y == FALSE) {//(2) pc기준 2사분면 -> 4사분면 방향으로 이동
                if (objects[index]->motion[0] % 6 == 2) objects[index]->coord[1]--;// x방향 1칸, y방향1칸, x방향 1칸, x방향 1칸
                else if (objects[index]->motion[0] % 4 == 4 || objects[index]->motion[0] % 4 == 5) {
                    objects[index]->coord[0] += (rand() % 3 - 1);
                    objects[index]->coord[1] += (rand() % 3 - 1);
                    objects[index]->motion[0]++;
                }
                else objects[index]->coord[0]--;
                objects[index]->motion[0]++;
            }
            else if (relativePosWithPC_X == FALSE && relativePosWithPC_Y == FALSE) {//(3) pc기준 3사분면 -> 1사분면 방향으로 이동 
                if (objects[index]->motion[0] % 6 == 2) objects[index]->coord[1]--;// x방향 1칸, y방향1칸, x방향 1칸, x방향 1칸
                else if (objects[index]->motion[0] % 4 == 4 || objects[index]->motion[0] % 4 == 5) {
                    objects[index]->coord[0] += (rand() % 3 - 1);
                    objects[index]->coord[1] += (rand() % 3 - 1);
                    objects[index]->motion[0]++;
                }
                else objects[index]->coord[0]++;//이거만 맞음
                objects[index]->motion[0]++;
            }
            else if (relativePosWithPC_X == FALSE && relativePosWithPC_Y == TRUE) {//(4) pc기준 4사분면 -> 2사분면 방향으로 이동
                if (objects[index]->motion[0] % 6 == 2) objects[index]->coord[1]++;// x방향 1칸, y방향1칸, x방향 1칸, x방향 1칸
                else if (objects[index]->motion[0] % 4 == 4 || objects[index]->motion[0] % 4 == 5) {
                    objects[index]->coord[0] += (rand() % 3 - 1);
                    objects[index]->coord[1] += (rand() % 3 - 1);
                    objects[index]->motion[0]++;
                }
                else objects[index]->coord[0]++;
                objects[index]->motion[0]++;
            }

        }
        /*1-2. 플레이어에게서 멀어지기*/
        else {
            if (relativePosWithPC_X == TRUE && relativePosWithPC_Y == TRUE) {//(1) pc기준 1사분면 ->3사분면 방향으로 이동
                if (objects[index]->motion[0] % 6 == 2) objects[index]->coord[1]--;// x방향 1칸, y방향1칸, x방향 1칸, x방향 1칸
                else if (objects[index]->motion[0] % 4 == 4 || objects[index]->motion[0] % 4 == 5) {
                    objects[index]->coord[0] += (rand() % 3 - 1);
                    objects[index]->coord[1] += (rand() % 3 - 1);
                    objects[index]->motion[0]++;
                }
                else objects[index]->coord[0]++;
                objects[index]->motion[0]++;
            }
            else if (relativePosWithPC_X == TRUE && relativePosWithPC_Y == FALSE) {//(2) pc기준 2사분면 -> 4사분면 방향으로 이동
                if (objects[index]->motion[0] % 6 == 2) objects[index]->coord[1]++;// x방향 1칸, y방향1칸, x방향 1칸, x방향 1칸
                else if (objects[index]->motion[0] % 4 == 4 || objects[index]->motion[0] % 4 == 5) {
                    objects[index]->coord[0] += (rand() % 3 - 1);
                    objects[index]->coord[1] += (rand() % 3 - 1);
                    objects[index]->motion[0]++;
                }
                else objects[index]->coord[0]++;
                objects[index]->motion[0]++;
            }
            else if (relativePosWithPC_X == FALSE && relativePosWithPC_Y == FALSE) {//(3) pc기준 3사분면 -> 1사분면 방향으로 이동 
                if (objects[index]->motion[0] % 6 == 2) objects[index]->coord[1]++;// x방향 1칸, y방향1칸, x방향 1칸, x방향 1칸
                else if (objects[index]->motion[0] % 4 == 4 || objects[index]->motion[0] % 4 == 5) {
                    objects[index]->coord[0] += (rand() % 3 - 1);
                    objects[index]->coord[1] += (rand() % 3 - 1);
                    objects[index]->motion[0]++;
                }
                else objects[index]->coord[0]--;
                objects[index]->motion[0]++;
            }
            else if (relativePosWithPC_X == FALSE && relativePosWithPC_Y == TRUE) {//(4) pc기준 4사분면 -> 2사분면 방향으로 이동          
                if (objects[index]->motion[0] % 6 == 2) objects[index]->coord[1]--;// x방향 1칸, y방향1칸, x방향 1칸, x방향 1칸
                else if (objects[index]->motion[0] % 4 == 4 || objects[index]->motion[0] % 4 == 5) {
                    objects[index]->coord[0] += (rand() % 3 - 1);
                    objects[index]->coord[1] += (rand() % 3 - 1);
                    objects[index]->motion[0]++;
                }
                else objects[index]->coord[0]--;
                objects[index]->motion[0]++;
            }
        }

    }
    /*맵 제한두기*/
    if (objects[index]->coord[0] + objects[index]->size[0] >= MAP_X_MAX) objects[index]->coord[0]--;
    if (objects[index]->coord[0] - objects[index]->size[0] < 0) objects[index]->coord[0]++;
    if (objects[index]->coord[1] + objects[index]->size[1] >= BASE_Y) objects[index]->coord[1]--;
    if (objects[index]->coord[1] - objects[index]->size[1] < 0) objects[index]->coord[1]++;
    MoveControl(objects[index]->coord, objects[index]->accel, objects[index]->size, &objects[index]->flyTime);//몬스터 움직임
    DrawSprite(x + 1, y - 1, objects[index]->size[0], objects[index]->size[1], sprite_enemy3);

    /*2.공격*/
    if (objects[index]->tick[3] + 2000 < tick) {//추적 범위 내에 있으면
        objects[index]->tick[3] = tick;
        dist = Distance(x, y, character.coord[0], character.coord[1]);

        if (dist < 23)  objects[index]->attack = TRUE;
        else objects[index]->attack = FALSE;

        if (objects[index]->attack == TRUE) {
            CreateObject(objects[index]->coord[0] + 4, objects[index]->coord[1], 198); //원거리 몹 투사체 관리, ControlObject에서 호출 //왼쪽 몹 2
            Mob3Bullet++;
        }
    }
    /*공격 맞을 시 도트딜 구현: 5씩 5번 데미지 준다. */
    if (character.tick[4] > 0 && character.tick[4] % 40 == 0) {
        character.time -= 5;
    }
    if (character.tick[4] == 0) {
        isPoisoned = FALSE;
    }

}

void ControlMob3Bullet(int index) {
    int x = objects[index]->originPos[0];
    int y = objects[index]->originPos[1];
    short at_coord[2] = { objects[index]->coord[0],objects[index]->coord[1] };
    //short pc_coord[2] = { character.coord[0] - 5 + 8 * character.direction, character.coord[1] };//캐릭터 좌표
    short pc_coord[2] = { character.coord[0], character.coord[1] };//캐릭터 좌표
    short at_size[2] = { 3, 3 };//공격 사이즈(판정 더 잘되게 하기 위해 3 X 3)
    short pc_size[2] = { 3, 3 };//플레이어 사이즈

    if (objects[index]->tick[2] + 500 < tick) {
        RemoveObject(index);
        Mob3Bullet--;
        return;
    }
    if (objects[index]->tick[1] + 30 < tick) {//tick[1]: 
        objects[index]->tick[1] = tick;

        objects[index]->coord[0] = (((objects[index]->motion[0]) * (objects[index]->characterCoord[0]) + (15 - objects[index]->motion[0]) * x)) / 15; // objects[index]->motion[0] : 10- objects[index]->motion[0] 내분점
        objects[index]->coord[1] = ((objects[index]->motion[1] * (objects[index]->characterCoord[1]) + (15 - objects[index]->motion[1]) * y)) / 15; // objects[index]->motion[0] : 10- objects[index]->motion[0] 내분점

        objects[index]->motion[0]++;                //*motion을 내분점의 i역할로 사용함
        objects[index]->motion[1]++;
        if (objects[index]->motion[0] == 16) objects[index]->motion[0] = 0;
        if (objects[index]->motion[1] == 16) objects[index]->motion[1] = 0;
    }

    /*맵을 벗어날 경우 제거*/
    if (objects[index]->coord[0] < 0 || objects[index]->coord[0] > MAP_X_MAX || objects[index]->coord[1] < 5 || objects[index]->coord[1] > FLOOR_Y) {
        RemoveObject(index);
        Mob3Bullet--;
        return;//객체가 사라졌으므로 메소드 종료시켜야함
    }

    /*도트딜 처리*/
    if (character.tick[4] == 0 && CollisionCheck(at_coord, pc_coord, at_size, pc_size)) {//충돌 시 
        character.tick[4] = 201;//무적상태로 돌입, 200번의 틱동안 유지(약 6초) 다른 데미지 안 입음 // 일반공격(80번틱)보다 길게 준다.
        isPoisoned = TRUE;
        RemoveObject(index);
        Mob3Bullet--;
        return;
    }
    /*공격에 맞을 경우 Bullet 객체는 삭제되므로 도트딜 적용하는 코드는 이 이후에 작성하면 수행이 안된다.*/
    /*ControlMob3에 작성*/

    DrawSprite(objects[index]->coord[0], objects[index]->coord[1], 1, 1, "o");
}

void ControlEnemy(int index) {//몬스터 처리 함수
    if (objects[index]->kind == 100) ControlGravity(objects[index]->coord, objects[index]->size, &objects[index]->is_jumping, &objects[index]->jump_y_max, index);
    short x = objects[index]->coord[0], y = objects[index]->coord[1];//몬스터 좌표
    short at_coord[2] = { character.coord[0] - 5 + 8 * character.direction, character.coord[1] };//캐릭터 좌표
    short at_size[2] = { 5, 3 };//공격 사이즈
    short item_code = rand() % 100;
    short attack = FALSE;

    /*0.몹 사망 처리*/
    if (objects[index]->time < 1) {//몬스터 사망 처리
        if (item_code >= 84) //아이템 드랍 확률 관련 숫자를 조정시 확률이 달라짐
            CreateObject(x + objects[index]->size[0] / 2 - 1, y - 1, 0); //아이템 1번 랜덤으로 드롭

        if (time_sword == 0)
            kill += 1;

        if (kill == time_sword_goal) {
            kill = 0;
            time_sword = 1;
        }
        enemy_count -= 1;

        RemoveObject(index);
        return;
    }

    /*1.몹 시간 -1초 처리*/
    if (objects[index]->tick[0] + 1000 < tick && use_item[0] != 1) { //아이템1번 사용 여부파악
        objects[index]->tick[0] = tick; //메인의 시점 tick
        objects[index]->time -= 1.0;
    }

    /*몹의 시간 출력(머리 위)*/
    DrawNumber(x + objects[index]->size[0] / 2 - NumLen(objects[index]->time) / 2 + 1, y - 2, (int)objects[index]->time);

    /*2.몹이 공격에 맞았을 경우 처리*/
    if (character.motion[2] == 1 && use_time_sword == 0 && CollisionCheck(objects[index]->coord, at_coord, objects[index]->size, at_size)) {
        objects[index]->tick[0] = tick;
        objects[index]->time -= character.power;
        character.time += character.power;//추가함
        //objects[index]->accel[1] = -0.55; //피격시 대각선으로 뒤로 감

        if (character.motion[3] == 3) {//주먹공격에 맞았을 경우
            objects[index]->time -= character.power;//피격 시 체력 감소
            character.time += character.power;//추가함
        }

        if (character.weapon == 2) {//넉백 공격 가속도
            if (!EnemyPositionX(x, objects[index]->size[0]))
                objects[index]->accel[0] = -5;
            else
                objects[index]->accel[0] = 5;
        }
        else {//일반 공격 피격 가속도
            if (!EnemyPositionX(x, objects[index]->size[0]))
                objects[index]->accel[0] = -0.75;
            else
                objects[index]->accel[0] = 0.75;
        }
    }
    /////////////////////*     몹 유형에 따른 동작 지정      */////////////////////
    switch (objects[index]->kind) {

    case 100:  //근접형 몹
        ControlMob1(index);
        break;
    case 102:  //설치형 몹
        ControlMob2(index);
        break;
    case 103:  //추정형 몹
        ControlMob3(index);
        break;
    }


}

void FindBossFlash() { // 뽀샤시3 군데에 찔러 넣었는지 판단
    int flash_sum = 0;

    flash_1[0] = flash_arr[flash_index1][0], flash_1[1] = flash_arr[flash_index1][1];
    flash_2[0] = flash_arr[flash_index2][0], flash_2[1] = flash_arr[flash_index2][1];
    flash_3[0] = flash_arr[flash_index3][0], flash_3[1] = flash_arr[flash_index3][1];


    /*뽀샤시 출력: 3컷으로 애니메이션 효과 주기*/
    if (flash_tick + 700 < tick) { flash_animation++; flash_tick = tick; }

    /*뽀샤시 1*/
    if (flash_cnt1 == 3) { DrawSprite(flash_1[0], flash_1[1], 3, 4, sprite_boss_flash[3]); }
    else {
        switch (flash_animation % 3) {
        case 0:
            DrawSprite(flash_1[0], flash_1[1], 3, 4, sprite_boss_flash[0]); break;
        case 1:
            DrawSprite(flash_1[0], flash_1[1], 3, 4, sprite_boss_flash[1]); break;
        case 2:
            DrawSprite(flash_1[0], flash_1[1], 3, 4, sprite_boss_flash[2]); break;
        }
    }
    /*뽀샤시 2*/
    if (flash_cnt2 == 3) { DrawSprite(flash_2[0], flash_2[1], 3, 4, sprite_boss_flash[3]); }
    else {
        switch (flash_animation % 3) {
        case 0:
            DrawSprite(flash_2[0], flash_2[1], 3, 4, sprite_boss_flash[0]); break;
        case 1:
            DrawSprite(flash_2[0], flash_2[1], 3, 4, sprite_boss_flash[1]); break;
        case 2:
            DrawSprite(flash_2[0], flash_2[1], 3, 4, sprite_boss_flash[2]); break;
        }
    }
    /*뽀샤시 3*/
    if (flash_cnt3 == 3) { DrawSprite(flash_3[0], flash_3[1], 3, 4, sprite_boss_flash[3]); }
    else {
        switch (flash_animation % 3) {
        case 0:
            DrawSprite(flash_3[0], flash_3[1], 3, 4, sprite_boss_flash[0]); break;
        case 1:
            DrawSprite(flash_3[0], flash_3[1], 3, 4, sprite_boss_flash[1]); break;
        case 2:
            DrawSprite(flash_3[0], flash_3[1], 3, 4, sprite_boss_flash[2]); break;
        }
    }


    if (CollisionCheck(flash_1, character.coord, flash_size, character.size)) {
        DrawSprite(flash_1[0], flash_1[1] - 1, 3, 1, "[A]");// . 상단에 A를 출력하기

        if (GetAsyncKeyState(0x41) & 0x8000) {//  A 누르면 시간의 조각 갖다 박음

            DrawSprite(flash_1[0], flash_1[1], 3, 4, sprite_enemy2);
            flash_cnt1 = 3;
        }

    }
    else if (CollisionCheck(flash_2, character.coord, flash_size, character.size)) {
        DrawSprite(flash_2[0], flash_2[1] - 1, 3, 1, "[A]");// . 상단에 A를 출력하기

        if (GetAsyncKeyState(0x41) & 0x8000) {//  A 누르면 시간의 조각 갖다 박음

            DrawSprite(flash_2[0], flash_2[1], 3, 4, sprite_enemy2);
            flash_cnt2 = 3;
        }
    }
    else if (CollisionCheck(flash_3, character.coord, flash_size, character.size)) {
        DrawSprite(flash_3[0], flash_3[1] - 1, 3, 1, "[A]");// . 상단에 A를 출력하기

        if (GetAsyncKeyState(0x41) & 0x8000) {//  A 누르면 시간의 조각 갖다 박음

            DrawSprite(flash_3[0], flash_3[1], 3, 4, sprite_enemy2);
            flash_cnt3 = 3;
        }
    }

    flash_sum = flash_cnt1 + flash_cnt2 + flash_cnt3; //3개가 박혔는지 합쳐봄

    if (flash_sum == 9) {
        isVisible = TRUE;
        isVisible_first++;
        Sleep(1000); //약간 멈칫하는 느낌을 줬음
        flash_tick = GetTickCount();
    }


}

void ControlBoss(int index) { // 부분부분 수정
    short x = objects[index]->coord[0];
    short y = objects[index]->coord[1];
    short at_coord[2] = { character.coord[0] - 5 + 8 * character.direction, character.coord[1] };//캐릭터 좌표
    short at_size[2] = { 5, 3 };//공격 사이즈

    if (objects[index]->time <= 0) {
        Sleep(2000);
        CLEAR();
    }

    //보스 시간 추가
    if (objects[index]->tick[0] + 1000 < tick && use_item[0] != 1) { //아이템1번 사용 여부파악
        objects[index]->tick[0] = tick; //메인의 시점 tick
        objects[index]->time += 1.0;
    }

    if ((flash_index1 == flash_index2) && (flash_index1 == flash_index3) && (flash_index3 == flash_index2)) {//
        while (1) { //뽀샤시 세 좌표가 다르게 만들기
            flash_index1 = rand() % 10;
            flash_index2 = rand() % 10;
            flash_index3 = rand() % 10;
            if ((flash_index1 != flash_index2) && (flash_index1 != flash_index3) && (flash_index3 != flash_index2))break;
        }
    }

    /*0.출력*/

    if (isVisible == TRUE) { // isvisible이 true 여야 보스가 등장
        if (flash_tick + 20000 <= GetTickCount()) { //20초 동안 공격 가능 모드
            isVisible = FALSE;
            flash_cnt1 = 0, flash_cnt2 = 0, flash_cnt3 = 0;
            flash_index1 = -1, flash_index2 = -1, flash_index3 = -1;
        }

        TextColor(WHITE);//
        //DrawSprite(x+1, y-5, objects[index]->size[0], objects[index]->size[1], sprite_boss_off);

        /*1.움직임*/


        /*
        if (objects[index]->coord[0] == 20 && objects[index]->coord[1] == 10) { //초기화
            objects[index]->coord[0] = 10;
            objects[index]->coord[1] = 10;
            objects[index]->accel[0] = 10.0;
            objects[index]->accel[1] = 10.0;
        }
        */

        if (objects[index]->tick[4] + 5000 < tick) {//다음 좌표로 넘어간다      //tick[4]: 다음자표 설정을 위한 틱
            bossSkillIsDash = FALSE;


            objects[index]->coord[0] = snitch_next_x;
            objects[index]->coord[1] = snitch_next_y;
            objects[index]->accel[0] = (float)snitch_next_x;
            objects[index]->accel[1] = (float)snitch_next_y;



            objects[index]->tick[4] = tick;
            snitch_next_x = rand() % (MAP_X_MAX - objects[index]->size[0]) + objects[index]->size[0];  //새 좌표 받음
            snitch_next_y = rand() % (BASE_Y - 10 - objects[index]->size[1]) + objects[index]->size[1];
            if (snitch_next_x < 10) snitch_next_x += 10;
            if (snitch_next_y < 10) snitch_next_y += 10;
            snitch_accel_x = (snitch_next_x - objects[index]->coord[0]) / 160.0;
            snitch_accel_y = (snitch_next_y - objects[index]->coord[1]) / 160.0;
            objects[index]->motion[0] = 0;
        }
        else {
            objects[index]->accel[0] += (float)snitch_accel_x;
            objects[index]->accel[1] += (float)snitch_accel_y;
        }

        /*시간이 지나 타임키가 다른 위치로 이동하는 경우: 해당 좌표로 이동, 5초 안에 가있어야함*/
        objects[index]->coord[0] = (int)objects[index]->accel[0];
        objects[index]->coord[1] = (int)objects[index]->accel[1];

        /*몹의 시간 출력(머리 위)*/
        DrawNumber(objects[index]->coord[0] + objects[index]->size[0] / 2 - NumLen(objects[index]->time) / 2 + 1, objects[index]->coord[1] - 7, (int)objects[index]->time);


        if (bossSkillTick + 4000 < tick) {
            bossSkillTick = tick;
            int skillNum = rand() % 3;
            if (skillNum == 0) bossSkillIsRazer = TRUE;
            else {
                bossSkillIsRazer = FALSE;
                if (skillNum == 1) BossAttackSummons(index);
                else if (skillNum == 2) BossDash(index);
            }
        }


        /*2.공격*/

        //if (rand() % 2 == 0)
        //    BossAttackSummons(index);
        //else
        BossAttackRazer(index);

        if (bossSkillIsDash == TRUE && CollisionCheck(objects[index]->coord, character.coord, objects[index]->size, character.size)) {
            stunTick = tick;
            isStunned = TRUE;
        }
        if (stunTick + 2000 < tick) isStunned = FALSE;



        DrawSprite(x + 1, y - 5, objects[index]->size[0], objects[index]->size[1], sprite_boss_off);


        if (bossSkillIsDash == TRUE) {
            if (tick % 8 < 2) for (int i = -2; i < 7; i++) DrawSprite(x, y - i, 1, 1, (char*)"!");
            else if (tick % 8 < 4) for (int i = 1; i < 11; i++) DrawSprite(x + i, y + 2, 1, 1, (char*)"!");
            else if (tick % 8 < 6) for (int i = -2; i < 7; i++) DrawSprite(x + 11, y - i, 1, 1, (char*)"!");
            else for (int i = 1; i < 11; i++) DrawSprite(x + i, y - 6, 1, 1, (char*)"!");
            /*
            for (int i = -2; i < 7; i++) {
                DrawSprite(x, y - i, 1, 1, (char*)"!");
                DrawSprite(x + 11, y - i, 1, 1, (char*)"!");
            }
            for (int i = 1; i < 11; i++) {
                DrawSprite(x+i, y+2, 1, 1, (char*)"!");
                DrawSprite(x +i, y - 6, 1, 1, (char*)"!");
            }
            */

            //DrawSprite(x, y, 1, 1, (char*)"!");
        }


        /*3.피격 시 + 사망처리*/

        if (character.motion[2] == 1 && use_time_sword == 0 && CollisionCheck(objects[index]->coord, at_coord, objects[index]->size, at_size)) { //일반 공격
            objects[index]->tick[0] = tick;
            objects[index]->time -= 100;
            character.time += 100;//추가함
            //objects[index]->accel[1] = -0.55; //피격시 대각선으로 뒤로 감

            if (character.motion[3] == 3) {//주먹공격에 맞았을 경우
                objects[index]->time -= 100;//피격 시 체력 감소
                character.time += 100;//추가함
            }

            //스니치 텔레포트
            objects[index]->coord[0] = rand() % (MAP_X_MAX - objects[index]->size[0]) + objects[index]->size[0];
            objects[index]->coord[1] = rand() % (BASE_Y - objects[index]->size[1]) + objects[index]->size[1];
            objects[index]->tick[4] -= 5000;

        }

        if (character.motion[2] == 1 && use_time_sword == 1 && CollisionCheck(objects[index]->coord, at_coord, objects[index]->size, at_size)) { //시간의 검 공격
            objects[index]->tick[0] = tick;
            objects[index]->time -= 1000;
            character.time += 100;//추가함

            //스니치 텔레포트
            objects[index]->coord[0] = rand() % (MAP_X_MAX - objects[index]->size[0]) + objects[index]->size[0];
            objects[index]->coord[1] = rand() % (BASE_Y - objects[index]->size[1]) + objects[index]->size[1];
            objects[index]->tick[4] -= 5000;

            snitch_hit_count++;
            snitch_hit = TRUE;// 맞는 순간 표현위해
            time_sword = 0;

        }

    }
    else {// 공격 불가 모드

        TextColor(YELLOW);
        FindBossFlash();

        if (isVisible_first >= 1) { // 보스 스테이지 처음을 제외한 나머지는 보스가 둥둥떠다니게 해야함
            DrawSprite(x + 1, y - 5, objects[index]->size[0], objects[index]->size[1], sprite_boss_on);

            /*1.움직임*/

            /*
            if (objects[index]->coord[0] == 20 && objects[index]->coord[1] == 10) { //초기화
                objects[index]->coord[0] = 10;
                objects[index]->coord[1] = 10;
                objects[index]->accel[0] = 10.0;
                objects[index]->accel[1] = 10.0;
            }
            */


            if (objects[index]->tick[4] + 5000 < tick) {//다음 좌표로 넘어간다      //tick[4]: 다음자표 설정을 위한 틱
                bossSkillIsDash = FALSE;

                /*
                objects[index]->coord[0] = snitch_next_x;
                objects[index]->coord[1] = snitch_next_y;
                objects[index]->accel[0] = (float)snitch_next_x;
                objects[index]->accel[1] = (float)snitch_next_y;
                */


                objects[index]->tick[4] = tick;
                snitch_next_x = rand() % (MAP_X_MAX - objects[index]->size[0]) + objects[index]->size[0];  //새 좌표 받음
                snitch_next_y = rand() % (BASE_Y - 10 - objects[index]->size[1]) + objects[index]->size[1];
                if (snitch_next_x < 10) snitch_next_x += 10;
                if (snitch_next_y < 10) snitch_next_y += 10;
                snitch_accel_x = (snitch_next_x - objects[index]->coord[0]) / 160.0;
                snitch_accel_y = (snitch_next_y - objects[index]->coord[1]) / 160.0;
                objects[index]->motion[0] = 0;
            }
            else {
                objects[index]->accel[0] += (float)snitch_accel_x;
                objects[index]->accel[1] += (float)snitch_accel_y;
            }

            /*시간이 지나 타임키가 다른 위치로 이동하는 경우: 해당 좌표로 이동, 5초 안에 가있어야함*/
            objects[index]->coord[0] = (int)objects[index]->accel[0];
            objects[index]->coord[1] = (int)objects[index]->accel[1];

            /*몹의 시간 출력(머리 위)*/
            DrawNumber(objects[index]->coord[0] + objects[index]->size[0] / 2 - NumLen(objects[index]->time) / 2 + 1, objects[index]->coord[1] - 7, (int)objects[index]->time);


            if (bossSkillTick + 4000 < tick) {
                bossSkillTick = tick;
                int skillNum = rand() % 3;
                if (skillNum == 0) bossSkillIsRazer = TRUE;
                else {
                    bossSkillIsRazer = FALSE;
                    if (skillNum == 1) BossAttackSummons(index);
                    else if (skillNum == 2) BossDash(index);
                }
            }


            /*2.공격*/

            //if (rand() % 2 == 0)
            //    BossAttackSummons(index);
            //else
            BossAttackRazer(index);

            if (bossSkillIsDash == TRUE && CollisionCheck(objects[index]->coord, character.coord, objects[index]->size, character.size)) {
                stunTick = tick;
                isStunned = TRUE;
            }
            if (stunTick + 2000 < tick) isStunned = FALSE;




            if (bossSkillIsDash == TRUE) {
                if (tick % 8 < 2) for (int i = -2; i < 7; i++) DrawSprite(x, y - i, 1, 1, (char*)"!");
                else if (tick % 8 < 4) for (int i = 1; i < 11; i++) DrawSprite(x + i, y + 2, 1, 1, (char*)"!");
                else if (tick % 8 < 6) for (int i = -2; i < 7; i++) DrawSprite(x + 11, y - i, 1, 1, (char*)"!");
                else for (int i = 1; i < 11; i++) DrawSprite(x + i, y - 6, 1, 1, (char*)"!");
                /*
                for (int i = -2; i < 7; i++) {
                    DrawSprite(x, y - i, 1, 1, (char*)"!");
                    DrawSprite(x + 11, y - i, 1, 1, (char*)"!");
                }
                for (int i = 1; i < 11; i++) {
                    DrawSprite(x+i, y+2, 1, 1, (char*)"!");
                    DrawSprite(x +i, y - 6, 1, 1, (char*)"!");
                }
                */

                //DrawSprite(x, y, 1, 1, (char*)"!");
            }


            /*3.피격 시 + 사망처리*/
        }

    }


}

void BossAttackRazer(int index) {
    if (first_attack == 0 && bossSkillIsRazer == FALSE) return;
    int r_x[5][2] = { {0, 28}, {29, 56},{57, 84},{85,112},{113,140} };
    int r_y[3][2] = { {0, 12}, {13,25}, {26,37} };
    if (raser_tick[0] <= GetTickCount() && first_attack == 0) {//처음 스킬 시전인지 확인
        raser_tick[0] = GetTickCount();
        first_attack = 1;
    }

    if (raser_tick[0] + 700 >= GetTickCount() && first_attack == 1) {//시작부터 0.7초까지 좌표를 구함
        for (int i = 0; i < 5; i++) {
            if (r_x[i][0] <= character.coord[0] && character.coord[0] <= r_x[i][1]) {
                raserPos_x[0] = r_x[i][0];
                raserPos_x[1] = r_x[i][1];
            }
        }
        for (int i = 0; i < 3; i++) {
            if (r_y[i][0] <= character.coord[1] && character.coord[1] <= r_y[i][1]) {
                raserPos_y[0] = r_y[i][0];
                raserPos_y[1] = r_y[i][1];
            }
        }
    }
    if (raser_tick[0] + 2000 >= GetTickCount() && raser_tick[0] + 700 <= GetTickCount() && first_attack == 1) {
        //0.7~2초사이 경고선 띄우기

        DrawSprite(raserPos_x[0], 0, 1, 38, "**************************************");
        DrawSprite(raserPos_x[1], 0, 1, 38, "**************************************");


        DrawSprite(0, raserPos_y[0], 140, 1, "********************************************************************************************************************************************");
        DrawSprite(0, raserPos_y[1], 140, 1, "********************************************************************************************************************************************");
        for (int i = 0; i < MAP_X_MAX * MAP_Y_MAX; i++) { //맵 덮어쓰기
            if (floorData[i] == '=') mapData[i] = '=';
        }

    }
    if (raser_tick[0] + 5000 >= GetTickCount() && raser_tick[0] + 2000 <= GetTickCount() && first_attack == 1) {
        //2~5초까지 공격

        //레이저 출력
        for (int i = raserPos_x[0]; i <= raserPos_x[1]; i++) {
            DrawSprite(i, 0, 1, 38, "**************************************");
        }
        for (int i = raserPos_y[0]; i <= raserPos_y[1]; i++) {
            DrawSprite(0, i, 140, 1, "********************************************************************************************************************************************");
        }
        for (int i = 0; i < MAP_X_MAX * MAP_Y_MAX; i++) { //맵 덮어쓰기
            if (floorData[i] == '=') mapData[i] = '=';
        }

        //피격 공격조건 확인
        if (raserPos_x[0] - 1 <= character.coord[0] && character.coord[0] <= raserPos_x[1] - 1
            || raserPos_y[0] <= character.coord[1] && character.coord[1] <= raserPos_y[1]) {

            if (character.tick[4] == 0) {
                character.time -= 1;// 임시
                character.tick[4] = 25;//무적 1초만 주기
            }

        }

    }
    if (raser_tick[0] + 5000 <= GetTickCount()) {//5초 지나면 스킬을 재 사용할 수 있도록 변수 초기화
        first_attack = 0;
    }

}

void BossAttackSummons(int index) {

    if (summons_tick[0] + 50000 < tick && first_attack == 0) {
        summons_tick[0] = tick;
        CreateObject(character.coord[0], 4, 100);
        CreateObject(character.coord[0], 4, 100);
        CreateObject(character.coord[0], character.coord[1] + 4, 103);
        first_attack = 1;
    }
}
//--------


void BossDash(int index) {
    bossSkillIsDash = TRUE;

    objects[index]->tick[4] = tick - 4000;
    snitch_next_x = character.coord[0];  //새 좌표 받음
    snitch_next_y = character.coord[1];

    snitch_accel_x = (snitch_next_x - objects[index]->coord[0]) / 32.0;
    snitch_accel_y = (snitch_next_y - objects[index]->coord[1]) / 32.0;
    objects[index]->motion[0] = 0;
}


void RemoveObject(int index) {//오브젝트 free
    free(objects[index]);
    objects[index] = 0;
}

void ControlBullet(int index) {//총알 처리
    short at_coord[2] = { objects[index]->coord[0]/* - 5 + 8 * character.direction*/, objects[index]->coord[1] };//총알 좌표
    short at_size[2] = { 1, 1 };//공격 사이즈

    DrawSprite(objects[index]->coord[0], objects[index]->coord[1] + 1, 1, 1, "*");

    for (int enemy_index = 0; enemy_index < OBJECT_MAX; enemy_index++) {
        if (objects[enemy_index]) {
            if (objects[enemy_index]->kind > 99 && objects[enemy_index]->kind < 200) {
                bool check = CollisionCheck(objects[enemy_index]->coord, at_coord, objects[enemy_index]->size, at_size);
                if (check) {
                    objects[enemy_index]->tick[0] = tick;
                    objects[enemy_index]->time -= 20;
                    character.time += 20;//추가함

                    if (!EnemyPositionX(objects[enemy_index]->coord[0], objects[enemy_index]->size[0]))//총알 피격시 뒤로 밀림
                        objects[enemy_index]->accel[0] = -1.5;
                    else
                        objects[enemy_index]->accel[0] = 1.5;

                    RemoveObject(index);
                    MoveControl(objects[enemy_index]->coord, objects[enemy_index]->accel, objects[enemy_index]->size, &objects[enemy_index]->flyTime);//몬스터 움직임
                    return;
                }
            }

            if (objects[enemy_index]->kind == 203 && isVisible == TRUE) {
                bool check = CollisionCheck(objects[enemy_index]->coord, at_coord, objects[enemy_index]->size, at_size);
                if (check) {
                    objects[enemy_index]->tick[0] = tick;
                    objects[enemy_index]->time -= 100;
                    character.time += 100;//추가함

                    RemoveObject(index);

                    //스니치 텔레포트
                    objects[enemy_index]->coord[0] = rand() % (MAP_X_MAX - objects[enemy_index]->size[0]) + objects[enemy_index]->size[0];
                    objects[enemy_index]->coord[1] = rand() % (BASE_Y - objects[enemy_index]->size[1]) + objects[enemy_index]->size[1];
                    objects[enemy_index]->tick[4] -= 5000;

                    return;
                }
            }

            /*
            if (objects[enemy_index]->kind == 202) { //스니치와 충돌한 경우
                bool check = CollisionCheck(objects[enemy_index]->coord, at_coord, objects[enemy_index]->size, at_size);
                if (check) {
                    //스니치 텔레포트
                    objects[enemy_index]->coord[0] = rand() % (MAP_X_MAX - objects[enemy_index]->size[0]) + objects[enemy_index]->size[0];
                    objects[enemy_index]->coord[1] = rand() % (BASE_Y - objects[enemy_index]->size[1]) + objects[enemy_index]->size[1];
                    objects[enemy_index]->tick[4] -= 5000;

                    snitch_hit_count++;
                    objects[enemy_index]->tick[0] = tick;
                    character.time -= character.power;//추가함
                    RemoveObject(index);
                    return;
                }
            }
            */
        }
    }
    objects[index]->coord[0] += objects[index]->direction ? 1 : -1;
    if (objects[index]->coord[0] <= 0 || objects[index]->coord[0] >= MAP_X_MAX)
        RemoveObject(index);
}

void ControlBomb(int index) {//폭탄 처리
    short at_coord[2] = { objects[index]->coord[0]/* - 5 + 8 * character.direction*/, objects[index]->coord[1] };//총알 좌표
    short at_size[2] = { 1, 1 };//공격 사이즈

    DrawSprite(objects[index]->coord[0], objects[index]->coord[1] + 1, 1, 1, "@");

    for (int enemy_index = 0; enemy_index < OBJECT_MAX; enemy_index++) {
        if (objects[enemy_index]) {
            if (objects[enemy_index]->kind > 99 && objects[enemy_index]->kind < 200) {
                bool check = CollisionCheck(objects[enemy_index]->coord, at_coord, objects[enemy_index]->size, at_size);
                if (check) {
                    objects[enemy_index]->tick[0] = tick;
                    objects[enemy_index]->time -= character.power;
                    character.time += character.power;//추가함

                    if (!EnemyPositionX(objects[enemy_index]->coord[0], objects[enemy_index]->size[0]))//총알 피격시 뒤로 밀림
                        objects[enemy_index]->accel[0] = -1.5;
                    else
                        objects[enemy_index]->accel[0] = 1.5;

                    ControlSmog(index);
                    RemoveObject(index);
                    MoveControl(objects[enemy_index]->coord, objects[enemy_index]->accel, objects[enemy_index]->size, &objects[enemy_index]->flyTime);//몬스터 움직임
                    return;
                }
            }
            if (objects[enemy_index]->kind == 202) { //스니치와 충돌한 경우
                bool check = CollisionCheck(objects[enemy_index]->coord, at_coord, objects[enemy_index]->size, at_size);
                if (check) {
                    objects[enemy_index]->coord[0] = rand() % (MAP_X_MAX - objects[enemy_index]->size[0]) + objects[enemy_index]->size[0];
                    objects[enemy_index]->coord[1] = rand() % (BASE_Y - objects[enemy_index]->size[1]) + objects[enemy_index]->size[1];
                    objects[enemy_index]->tick[4] -= 5000;

                    snitch_hit_count++;
                    objects[enemy_index]->tick[0] = tick;
                    character.time -= character.power;//추가함

                    ControlSmog(index);
                    RemoveObject(index);
                    return;
                }
            }
            if (objects[enemy_index]->kind == 203 && isVisible == TRUE) {
                bool check = CollisionCheck(objects[enemy_index]->coord, at_coord, objects[enemy_index]->size, at_size);
                if (check) {
                    objects[enemy_index]->tick[0] = tick;
                    objects[enemy_index]->time -= 100;
                    character.time += 100;//추가함

                    RemoveObject(index);

                    //스니치 텔레포트
                    objects[enemy_index]->coord[0] = rand() % (MAP_X_MAX - objects[enemy_index]->size[0]) + objects[enemy_index]->size[0];
                    objects[enemy_index]->coord[1] = rand() % (BASE_Y - objects[enemy_index]->size[1]) + objects[enemy_index]->size[1];
                    objects[enemy_index]->tick[4] -= 5000;

                    return;
                }
            }
        }
    }
    objects[index]->coord[0] += objects[index]->direction ? 1 : -1;
    if (objects[index]->coord[0] <= 0 || objects[index]->coord[0] >= MAP_X_MAX)
        RemoveObject(index);
}

void ControlSmog(int index) {//폭탄 처리
    objects[index]->coord[0] -= 4;
    objects[index]->coord[1] -= 1;
    objects[index]->size[0] = 10;
    objects[index]->size[1] = 3;
    objects[index]->tick[1] = GetTickCount();
    short at_coord[2] = { objects[index]->coord[0], objects[index]->coord[1] };//총알 좌표
    short at_size[2] = { objects[index]->size[0],objects[index]->size[1] };//공격 사이즈

    DrawSprite(at_coord[0], at_coord[1], at_size[0], at_size[1], " ;       ;        ; ;    ; ; ;");

    for (int enemy_index = 0; enemy_index < OBJECT_MAX; enemy_index++) {
        if (objects[enemy_index]) {
            if (objects[enemy_index]->kind > 99 && objects[enemy_index]->kind < 200) {
                bool check = CollisionCheck(objects[enemy_index]->coord, at_coord, objects[enemy_index]->size, at_size);
                if (check) {
                    objects[enemy_index]->tick[0] = tick;
                    objects[enemy_index]->time -= character.power;
                    character.time += character.power;//추가함

                    if (!EnemyPositionX(objects[enemy_index]->coord[0], objects[enemy_index]->size[0]))//총알 피격시 뒤로 밀림
                        objects[enemy_index]->accel[0] = -0.5;
                    else
                        objects[enemy_index]->accel[0] = 0.5;

                    if (GetTickCount() >= objects[index]->tick[1] + 1000) {
                        RemoveObject(index);
                        return;
                    }
                    MoveControl(objects[enemy_index]->coord, objects[enemy_index]->accel, objects[enemy_index]->size, &objects[enemy_index]->flyTime);//몬스터 움직임
                }
            }
        }
    }
}

void MoveControl(short coord[], float accel[], short size[], float* flyTime) {//부드러운 움직임 재현 함수
    float x_value = accel[0];//인자로 받은 x 가속만큼 위치에 더해줌

    /*벽 한계 제한*/
    if (x_value != 0) {
        if (coord[0] + x_value < 1)
            x_value = 1 - coord[0];
        if (coord[0] + size[0] + x_value > MAP_X_MAX)
            x_value = MAP_X_MAX - size[0] - coord[0];
    }

    /*속도반영*/
    coord[0] += floor(x_value + 0.5);

    /*변속->0으로 수렴시키기*/
    if (accel[0] > 0) accel[0] -= 0.2; if (accel[0] < 0) accel[0] += 0.2;

}

bool CollisionCheck(short coord1[], short coord2[], short size1[], short size2[]) {//2개의 객체와 크기를 받고 충돌하는지 검사하기
    for (int x1 = coord1[0]; x1 < coord1[0] + size1[0]; x1++)
        for (int x2 = coord2[0]; x2 < coord2[0] + size2[0]; x2++)
            for (int y1 = coord1[1]; y1 > coord1[1] - size1[1]; y1--)
                for (int y2 = coord2[1]; y2 > coord2[1] - size2[1]; y2--)
                    if (x1 == x2 && y1 == y2) return TRUE;
    return FALSE;
}

short Distance(short  x1, short  y1, short x2, short  y2) {
    short dx = x1 - x2;
    short dy = y1 - y2;
    return sqrt(dx * dx + dy * dy);
}

bool EnemyPositionX(short x, short size_x) {//몬스터가 캐릭터의 위치를 추적
    if (character.coord[0] + 1 <= x + floor(size_x / 2 + 0.5))//pc 기준 우측이면 TRUE리턴
        return TRUE;
    else
        return FALSE;//좌측이면 FALSE리턴
}

bool EnemyPositionY(short y, short size_y) {//몬스터가 캐릭터의 위치를 추적
    if (character.coord[1] + 1 >= y + floor(size_y / 2 + 0.5))//pv 기준 위쪽이면 TRUE리턴
        return TRUE;
    else
        return FALSE;//아래쪽이면 FALSE리턴
}

void DrawBox(short x, short y, short size_x, short size_y) {//상자 그리기
    //EditMap(x, y, ''); EditMap(x + size_x - 1, y, '');
    //EditMap(x, y + size_y - 1, ''); EditMap(x + size_x - 1, y + size_y - 1, '');

    for (int i = 1; i < size_x - 1; i++) {
        EditMap(x + i, y, '-'); EditMap(x + i, y + size_y - 1, '-');
    }
    for (int i = 1; i < size_y - 1; i++) {
        EditMap(x, y + i, '|'); EditMap(x + size_x - 1, y + i, '|');
    }
}

void DrawNumber(short x, short y, int num) {//숫자 출력
    int tmp = num, len = NumLen(tmp), cnt = len;
    char* str = (char*)malloc(len);

    do {
        cnt--;
        str[cnt] = (char)(tmp % 10 + 48);
        tmp /= 10;
    } while (tmp != 0);

    DrawSprite(x, y, len, 1, str);
}

void DrawSprite(short x, short y, short size_x, short size_y, const char spr[]) {//문자 출력
    for (int i = 0; i < size_y; i++) {
        for (int n = 0; n < size_x; n++)
            EditMap(x + n, y + i, spr[i * size_x + n]);
    }
}

void FillMap(char str[], char str_s, int max_value) {//특정 문자로 맵 채우기
    for (int i = 0; i < max_value; i++)
        str[i] = str_s;
}

void EditMap(short x, short y, char str) {//특정 문자로 맵 수정
    if (x > 0 && y > 0 && x - 1 < MAP_X_MAX && y - 1 < MAP_Y_MAX)
        mapData[(y - 1) * MAP_X_MAX + x - 1] = str;
}

int NumLen(int num) {
    int tmp = num, len = 0;

    if (num == 0) {
        return 1;
    }
    else {
        while (tmp != 0) {
            tmp /= 10;
            len++;
        }
    }

    return len;
}


void WIN() {
    system("cls");

    stage_num += 1;
    existTimeKey = FALSE;
    enemy_count = 0;
    snitch_hit_count = 0;
    snitch_hit_count_max++;
    mob2_switch = FALSE;
    
    if (stage_num == 1) { // 1-> 2스테이지 사이 컷씬
        PlaySound(TEXT("win.wav"), NULL, SND_ASYNC | SND_SYNC);
        char print_temp1[1000];
        FILE* rfp1;
        rfp1 = fopen("stage2.txt", "rt");

        if (rfp1 == NULL) {
            printf("오류");
            return;
        }

        while (fgets(print_temp1, 999, rfp1) != NULL) {
            printf(print_temp1);
        }
        puts("");
        fclose(rfp1);

        SetCurrentCursorPos(52, 23);
        printf("TIP : 나에게 날아오는 공격을 조심하라!!!");


        Sleep(2000);
    }
    else if (stage_num == 2) {// 2-> 3스테이지 사이 컷씬
        PlaySound(TEXT("win.wav"), NULL, SND_ASYNC | SND_SYNC);
        char print_temp2[1000];
        FILE* rfp2;
        rfp2 = fopen("stage3.txt", "rt");

        if (rfp2 == NULL) {
            printf("오류");
            return;
        }

        while (fgets(print_temp2, 999, rfp2) != NULL) {
            printf(print_temp2);
        }
        puts("");
        fclose(rfp2);

        SetCurrentCursorPos(48, 23);
        printf("TIP 1 : 나에게 도트 데미지를 주는 몬스터를 조심 하라!!!");
        SetCurrentCursorPos(48, 25);
        printf("TIP 2 : 뚫려있는 땅바닥으로 빠지지 않게 조심 하라!!!");

        Sleep(2000);
    }
    else if (stage_num == 3) {// 3-> 보스스테이지 사이 컷씬
        PlaySound(TEXT("win.wav"), NULL, SND_ASYNC | SND_SYNC);
        char print_temp3[1000];
        FILE* rfp3;
        rfp3 = fopen("bossstage.txt", "rt");

        if (rfp3 == NULL) {
            printf("오류");
            return;
        }

        while (fgets(print_temp3, 999, rfp3) != NULL) {
            printf(print_temp3);
        }
        puts("");
        fclose(rfp3);

        SetCurrentCursorPos(36, 23);
        printf("TIP 1 : 수상한 기운이 도는 3곳에 \"영겁의 시간의 조각\"을 끼워 넣어라!!!");
        SetCurrentCursorPos(36, 25);
        printf("TIP 2 : 지속적으로 \"영겁의 시간의 조각\"을 끼워 넣으며 보스를 공격하라!!!");
        SetCurrentCursorPos(36, 27);
        printf("TIP 3 : \"영겁의 시간의 조각\"을 끼워 넣지 않으면 보스를 공격할 수 없다!!!");

        Sleep(2000);
    }
    else if (stage_num == 4) {

        exit(0);
    }


    SetCurrentCursorPos(58, 38);
    printf("(S를 누르면 다음 STAGE...)");

    while (1) {
        int flag = 0;
        for (int i = 0; i < 20; i++) {
            if (_kbhit() != 0) {

                int key = _getch();
                if (key == 115)flag = 1; //s누르면 스킵
            }
        }
        if (flag == 1)break;
        Sleep(300);
    }



    for (int i = 0; i < OBJECT_MAX; i++) {
        objects[i] = NULL;
        free(objects[i]);
    }
    StartGame();
    UpdateGame();
}

void LOSE() {
    system("cls");
    PlaySound(TEXT("lose.wav"), NULL, SND_ASYNC | SND_SYNC);
    char print_temp3[1000];
    FILE* rfp3;
    rfp3 = fopen("gameover.txt", "rt");

    if (rfp3 == NULL) {
        printf("오류");
        return;
    }

    while (fgets(print_temp3, 999, rfp3) != NULL) {
        printf(print_temp3);
    }
    puts("");
    fclose(rfp3);

    getchar();
    getchar();
 
    exit(0);
}

void CLEAR() {
    system("cls");
    PlaySound(TEXT("clear.wav"), NULL, SND_ASYNC | SND_SYNC);
    char print_temp1[1000];
    FILE* rfp1;
    rfp1 = fopen("gameclear.txt", "rt");

    if (rfp1 == NULL) {
        printf("오류");
        return;
    }

    while (fgets(print_temp1, 999, rfp1) != NULL) {
        printf(print_temp1);
    }
    puts("");
    fclose(rfp1);

    getchar();
    getchar();
    exit(0);
}

void GameStory() {
    if (GetKeyState(0x53) & 0x8001) return;
    SetCurrentCursorPos(52, 20);
    printf("스토리 중 S 키를 누르면 전체 스킵됩니다.");
    Sleep(2000);
    system("cls");
    int cur_x = 58, cur_y = 18;

    if (GetKeyState(0x53) & 0x0001) return;
    SetCurrentCursorPos(cur_x + 8, cur_y);
    printf("2XXX 년..."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;
    SetCurrentCursorPos(cur_x, cur_y);
    printf("벌써 1833415번의 도전이다..."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;
    SetCurrentCursorPos(cur_x + 4, cur_y);
    printf("이제는 포기해야 하나..."); Sleep(2500);
    if (GetKeyState(0x53) & 0x0001) return;

    system("cls");
    cur_y = 20;
    SetCurrentCursorPos(cur_x + 6, cur_y);
    printf("차라리 죽고 싶다..."); Sleep(2500);
    if (GetKeyState(0x53) & 0x0001) return;

    system("cls");
    cur_y = 20;
    TextColor(DarkRed);
    SetCurrentCursorPos(cur_x + 2, cur_y);
    printf("\"나를 더 즐겁게 해보거라\""); Sleep(2500);

    system("cls");
    TextColor(WHITE);
    if (GetKeyState(0x53) & 0x0001) return;

    cur_y = 20;
    SetCurrentCursorPos(cur_x + 4, cur_y);
    printf("지긋지긋한 괴물 X끼..."); Sleep(2500);
    if (GetKeyState(0x53) & 0x0001) return;

    system("cls");


    cur_y = 20;
    TextColor(DarkYellow);
    SetCurrentCursorPos(cur_x + 4, cur_y);
    printf("??? : 도와줄까??"); Sleep(2500); cur_y += 2;
    TextColor(WHITE);
    if (GetKeyState(0x53) & 0x0001) return;

    system("cls");
    cur_y = 20;
    SetCurrentCursorPos(cur_x + 4, cur_y);
    printf("뭐야...??"); Sleep(1700); cur_y += 2;
    SetCurrentCursorPos(cur_x + 4, cur_y);
    printf("누구야!?!?!?!?!"); Sleep(2500); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    system("cls");
    cur_y = 18;
    TextColor(DarkYellow);
    SetCurrentCursorPos(cur_x, cur_y);
    printf("??? : 진정해."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    SetCurrentCursorPos(cur_x, cur_y);
    printf("??? : 그저 너를 도와주려는 존재니까."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    SetCurrentCursorPos(cur_x, cur_y);
    printf("??? : 그런 식으로는 절대 이기지 못해."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    SetCurrentCursorPos(cur_x, cur_y);
    printf("??? : 내가 좀 도와주지."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    SetCurrentCursorPos(cur_x, cur_y);
    printf("??? : 손을 잠깐 줘봐."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    char print_temp1[1000];
    FILE* rfp1;
    rfp1 = fopen("ascii-art (2).txt", "rt");

    if (rfp1 == NULL) {
        printf("오류");
        return;
    }

    while (fgets(print_temp1, 999, rfp1) != NULL) {
        printf(print_temp1);
    }
    puts("");
    fclose(rfp1);
    if (GetKeyState(0x53) & 0x0001) return;
    Sleep(1500);
    system("cls");
    Sleep(1000);
    if (GetKeyState(0x53) & 0x0001) return;

    char print_temp2[1000];
    FILE* rfp2;
    rfp2 = fopen("ascii-art (2).txt", "rt");

    if (rfp2 == NULL) {
        printf("오류");
        return;
    }

    while (fgets(print_temp2, 999, rfp2) != NULL) {
        printf(print_temp2);
    }
    puts("");
    fclose(rfp2);
    if (GetKeyState(0x53) & 0x0001) return;
    Sleep(1500);
    system("cls");
    if (GetKeyState(0x53) & 0x0001) return;

    TextColor(WHITE);
    cur_y = 20;
    SetCurrentCursorPos(cur_x + 10, cur_y);
    printf("어...어!!!"); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;
    SetCurrentCursorPos(cur_x, cur_y);
    printf("(온몸에 힘이 나는 느낌인데...??)"); Sleep(2500); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;


    system("cls");
    TextColor(DarkYellow);
    cur_y = 18;
    SetCurrentCursorPos(cur_x - 8, cur_y);
    printf("??? : 자. 이제 너는 적들의 시간을 뺏을 수 있을거야."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    SetCurrentCursorPos(cur_x - 8, cur_y);
    printf("??? : 그리고, 여기 \"시간의 검\"을 줄게."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    SetCurrentCursorPos(cur_x - 8, cur_y);
    printf("??? : 어서 가서 \"영겁의 시간의 조각\"을 모은 뒤."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    SetCurrentCursorPos(cur_x - 8, cur_y);
    printf("??? : \"영겁의 시간\"을 물리쳐서 이세상을 구해."); Sleep(2500); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    system("cls");
    TextColor(WHITE);
    cur_y = 20;
    SetCurrentCursorPos(cur_x + 8, cur_y);
    printf("고맙다..."); Sleep(2500); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    system("cls");
    TextColor(DarkYellow);
    cur_y = 20;
    SetCurrentCursorPos(cur_x + 6, cur_y);
    printf("??? : 건투를 빈다..."); Sleep(2500); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    system("cls");
    cur_y = 20;
    TextColor(YELLOW);
    SetCurrentCursorPos(cur_x + 6, cur_y);
    printf("\"과거의 나 자신이여.\""); Sleep(2500); cur_y += 2;

    TextColor(WHITE);

}

void GameIntro() {
    //SetConsole();
    PlaySound(TEXT("menu.wav"), NULL, SND_ASYNC | SND_SYNC);
    char print_temp[1000];
    FILE* rfp;
    rfp = fopen("ticktockintime.txt", "rt");

    if (rfp == NULL) {
        printf("오류");
        return;
    }
    SetCurrentCursorPos(50, 50);
    while (fgets(print_temp, 99, rfp) != NULL) {
        printf(print_temp);
    }
    puts("");
    fclose(rfp);



    int key;
    int cur_x, cur_y;
    cur_x = 108, cur_y = 18;
    int temp = 1; //1 게임시작 2 게임설명 3 나가기

    while (1)
    {
        SetCurrentCursorPos(110, 18);
        printf("게임시작");
        SetCurrentCursorPos(110, 22);
        printf("게임설명");
        SetCurrentCursorPos(110, 26);
        printf("나가기");

        SetCurrentCursorPos(cur_x, cur_y);
        printf("▶");

        if (_kbhit()) {
            key = _getch();
            if (key == 80) { //아래키
                if (temp < 3) {
                    SetCurrentCursorPos(cur_x, cur_y);
                    printf("  ");
                    cur_y += 4;
                    temp++;
                }
                else if (temp == 3) {
                    SetCurrentCursorPos(cur_x, cur_y);
                    printf("  ");
                    cur_y = 18;
                    temp = 1;
                }
                Beep(530, 200);
            }

            if (key == 72) { //위키
                if (temp == 1) {
                    SetCurrentCursorPos(cur_x, cur_y);
                    printf("  ");
                    cur_y = 26;
                    temp = 3;
                }
                else if (temp > 1) {
                    SetCurrentCursorPos(cur_x, cur_y);
                    printf("  ");
                    cur_y -= 4;
                    temp--;
                }
                Beep(530, 200);
            }

            if (key == 13 && temp == 1) { //엔터 누르면 게임시작
                Beep(530, 200);
                break;
            }
            else if (key == 13 && temp == 2) { //엔터 누르면 게임 설명
                Beep(530, 200);
                GameManual();
                cur_y -= 4;
                temp = 1;

                char print_temp[1000];
                FILE* rfp;
                rfp = fopen("ticktockintime.txt", "rt");

                if (rfp == NULL) {
                    printf("오류");
                    return;
                }
                SetCurrentCursorPos(50, 50);
                while (fgets(print_temp, 99, rfp) != NULL) {
                    printf(print_temp);
                }
                puts("");
                fclose(rfp);
            }
            else if (key == 13 && temp == 3) { //엔터 누르면 나가기
                Beep(530, 200);
                exit(0);
            }
        }

    }
    system("cls");
}

void GameManual() {
    int cur_x, cur_y;
    int key;

    for (int i = 0; i < MAP_Y_MAX; i++) {
        for (int j = 0; j < MAP_X_MAX; j++) {
            printf("  ");
        }
    }

    //system("cls"); 한번에 지우는 거 그러나 위에께 더 이쁨


    char print_temp1[1000];
    FILE* rfp1;
    rfp1 = fopen("gamemanual.txt", "rt");

    if (rfp1 == NULL) {
        printf("오류");
        return;
    }

    while (fgets(print_temp1, 99, rfp1) != NULL) {
        printf(print_temp1);
    }
    puts("");
    fclose(rfp1);


    TextColor(WHITE);
    cur_x = 20, cur_y = 20;
    SetCurrentCursorPos(cur_x, cur_y);
    printf("1. 캐릭터의 HP는 TIME . 즉, TIME이 0이 되면 캐릭터는 사망!");
    cur_y += 3;

    SetCurrentCursorPos(cur_x, cur_y);
    printf("2. 몬스터를 공격하면 몬스터의 TIME을 뺏을 수 있다!");
    cur_y += 3;

    SetCurrentCursorPos(cur_x, cur_y);
    printf("3. 몬스터를 처치하면 충전되는 \"시간의 검\"을 사용하여 시간의 조각을 타격 해야 한다!");
    cur_y += 3;


    SetCurrentCursorPos(cur_x, cur_y);
    printf("4. 스테이지별로 \"영겁의 시간의 조각\"을 일정 횟수 공격하여 다음 스테이지로 넘어가라! ");
    cur_y += 3;

    SetCurrentCursorPos(cur_x, cur_y);
    printf("5. 1,2,3 스테이지와 마지막 보스 \"영겁의 시간\"을 클리어 하라!");
    cur_y += 3;


    TextColor(RED);
    SetCurrentCursorPos(cur_x, cur_y);
    printf("Tip : 몬스터를 많이 공격하여 TIME을 충분히 얻고 다음 스테이지로 가는게 좋을 지도..??!!");
    cur_y += 5;

    TextColor(WHITE);
    SetCurrentCursorPos(cur_x, cur_y);
    printf("(s 누르면 SKIP)");
    cur_y += 3;

    int flag;

    while (1) {
        flag = 0;
        for (int i = 0; i < 20; i++) {
            if (_kbhit() != 0) {

                key = _getch();
                if (key == 115)flag = 1; //s누르면 스킵
            }
        }
        if (flag == 1)break;
        Sleep(300);
    }
    Beep(530, 200);
    for (int i = 0; i < MAP_Y_MAX; i++) {
        for (int j = 0; j < MAP_X_MAX; j++) {
            printf("  ");
        }
    }

    //system("cls"); //화면 지우고~

    //방향키 설명
    char print_temp2[1000];
    FILE* rfp2;
    rfp2 = fopen("gamemanual.txt", "rt");

    if (rfp2 == NULL) {
        printf("오류");
        return;
    }

    while (fgets(print_temp2, 99, rfp2) != NULL) {
        printf(print_temp2);
    }
    puts("");
    fclose(rfp2);

    cur_x = 20, cur_y = 20;
    SetCurrentCursorPos(cur_x, cur_y);

    TextColor(WHITE);
    printf("1. 방향키 :  ← , → , ↑ , ↑↑");
    cur_y += 3;
    SetCurrentCursorPos(cur_x, cur_y);
    printf("2. 스킬키 : Q , W ");
    cur_y += 3;
    SetCurrentCursorPos(cur_x, cur_y);
    printf("3. 변신키 : E");
    cur_y += 3;

    TextColor(RED);
    SetCurrentCursorPos(cur_x, cur_y);
    printf("Tip : 변신키 E을 잘 사용하여 전략적인 스킬 활용을 할 수 있다!");
    cur_y += 3;
    TextColor(WHITE);

    SetCurrentCursorPos(cur_x, cur_y);
    printf("4. 시간의 검 : R");
    cur_y += 3;

    TextColor(RED);
    SetCurrentCursorPos(cur_x, cur_y);
    printf("Tip : 몬스터를 처치하여 충전된 \"시간의 검\"은 \"영겁의 시간의 조각\"에만 사용 가능!");
    cur_y += 3;
    TextColor(WHITE);

    SetCurrentCursorPos(cur_x, cur_y);
    printf("5. 아이템 사용키 : T");
    cur_y += 3;
    TextColor(RED);
    SetCurrentCursorPos(cur_x, cur_y);
    printf("Tip : 몬스터를 해치우면 랜덤 확률로 아이템 박스를 얻을 수 있다!");
    TextColor(WHITE);
    cur_y += 3;

    SetCurrentCursorPos(cur_x, cur_y);
    printf("(s 누르면 SKIP)");

    while (1) {
        flag = 0;
        for (int i = 0; i < 20; i++) {
            if (_kbhit() != 0) {

                key = _getch();
                if (key == 115)flag = 1; //s누르면 스킵
            }
        }
        if (flag == 1)break;
        Sleep(300);
    }
    Beep(530, 200);
    char print_temp3[1000];
    FILE* rfp3;
    rfp3 = fopen("gamemanual.txt", "rt");

    if (rfp3 == NULL) {
        printf("오류");
        return;
    }

    while (fgets(print_temp3, 99, rfp3) != NULL) {
        printf(print_temp3);
    }
    puts("");
    fclose(rfp3);

    cur_x = 36, cur_y = 25;
    SetCurrentCursorPos(cur_x, cur_y);
    TextColor(RED);
    printf("스테이지 별로 \"시간의 조각\"을 모은 뒤 보스를 처치하여 게임을 클리어 하라!");
    TextColor(WHITE);

    cur_y += 5;
    SetCurrentCursorPos(cur_x, cur_y);
    printf("(s 누르면 나가기)");

    while (1) {
        flag = 0;
        for (int i = 0; i < 20; i++) {
            if (_kbhit() != 0) {

                key = _getch();
                if (key == 115)flag = 1;
            }
        }
        if (flag == 1)break;
        Sleep(300);
    }
    Beep(530, 200);
    system("cls");
    Sleep(300);
}

void SetCurrentCursorPos(int x, int y)
{
    COORD pos = { x, y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

void TextColor(int colorNum) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colorNum);
}



























