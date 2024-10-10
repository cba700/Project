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
#define MAP_Y_MAX 47//�� ���μ��� ����
#define FLOOR_Y 38//�ٴ� ��ǥ��
#define FLOOR_Y_1 29//�ٴ� ��ǥ�� 
#define FLOOR_Y_2 20//�ٴ� ��ǥ�� 
#define FLOOR_Y_3 11//�ٴ� ��ǥ��
#define OBJECT_MAX 32//������Ʈ �ִ� ����
#define SPAWN_TIME 15000//���� ������ �ð� 15�ʸ��� ������
#define BASE_Y 39 //UI Y��ǥ ������ ����

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
    short coord[2], size[2];   //��ǥ���� ������ ��
    float accel[2], flyTime;   //���ӵ��� �߷� ����
    bool direction;   //true=right, false=left
    //stat
    char name[16];
    double time;   //�ð� = ����
    short power, weapon, item;

    //animation control
    short motion[4];//       motion[1]:attack���� bool�� ,  leg_motion, attack_motion(1, 2, 3), invincibility motion
    unsigned int tick[6];//  tick[0]: gen ����� ,tick[1]:leg motion ����� ,  tick[2]:attack motion �����       //gen_tick, leg_tick, atk_tick, dash_tick, invincibility tick//�� ��Ǹ��� �ð��� ��� �迭
    unsigned int item_tick[4]; // ������ �ð� ���ִ� tick�迭
    short jump_y_max, is_jumping = 2;
    short skill_set;//��ų ���� ����
}Character;

typedef struct _Object {   //enemies, projectiles, etc.
    short coord[2], size[2];//size[0]: 
    float accel[2], flyTime;//���ӵ��� �߷� ����
    bool direction;

    short kind;   //1~99: items, 100~199: enemies, 200~: projectiles
    double time;   //hp: this value is used randomly for item or particle object
    short attack;
    short damage;
    short originPos[2];
    short characterCoord[2];//���� ������ �÷��̾� ��ġ�� ����ϱ� ���� �ʵ�

    short motion[3];   //motion
    unsigned int tick[5];   //0: ��ü�� �ð�(ü��) ������ ƽ 1:
    short jump_y_max, is_jumping = 4;
}Object;

Character character = { {MAP_X_MAX / 2, MAP_Y_MAX / 2}, {3, 3}, {0, 0}, 0, 1, "", 600, 10, 0, 0, {0, 1, 0, 0}, {0, 0, 0, 0, 0} , {0,0,0,0} };

Object** objects;

int kill = 0;
int kill_goal = 0;
int enemy_count = 0;
int enemy_max = 0;
unsigned int tick = 0;//�ý��� ƽ(������ �ݺ�����, ���� �޼ҵ忡�� ������)
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

/*�������� ��������*/

bool isUnlocked = TRUE;
bool isVisible = FALSE; //
int isVisible_first = 0; //
short flash_arr[10][2] = { {15,7},{85,7},{133,7},{26,16},{110,16},{66,16},{72,16},{13,25},{82,25},{124,25} }; // �ǻ��� ��ǥ �̸� ������
short flash_1[2];// ���� �ǻ��� ��ġ
short flash_2[2];// ���� �ǻ��� ��ġ
short flash_3[2];// ���� �ǻ��� ��ġ
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

short bossGenCnt = 0;//ó�� ��ȯ�ø� ����

int snitch_next_x = 10;
int snitch_next_y = 10;
float snitch_accel_x = 0;
float snitch_accel_y = 0;
int snitch_hit_count = 0;
int snitch_hit_count_max = 1;
bool snitch_hit = FALSE;

int Mob2Bullet = 0;
int Mob3Bullet = 0;

//ü��, xũ��, yũ��, 
const short stat_enemy[4][7] =
{ {150, 3, 3, 0, 1000, 0, 0},//100: ������
 {300, 3, 4, 0, 0, 0, 2000},//101: �ð�Ű
 {300, 4, 3, 0, 0, 0, 0}, //102: �����
{300, 5, 3, 0, 0, 0, 0} };//103: ���Ÿ���

const char sprite_character[10] = " 0  | _^_";//ĳ���� ������
const char sprite_character_leg[2][3][4] =//ĳ���� �̵� �ִϸ��̼�
{ {"-^.", "_^\'", "_^."},
 {".^-", "\'^_", ".^_"} };

const char sprite_weapon[3][2][4] =//ĳ���� ���� ����
{ { "<==", "==>" } ,//Į
{ "==\\", "/==" },//��
{ "@==", "==@" }//�˹�
};

const char sprite_itemBox[11] = "| ? ||___|";//������ ���� �ڽ�
const char sprite_item[4][15] = { "---\\ // \\---","bbbbbbbbbbbb","cccccccccccc","dddddddddddd" }; //������ 1~4�� ����� �ӽ� ����

int have_item = -1; //������ ȹ�� ���� 
int use_item[4] = { 0,0,0,0 }; //�����۵� ��� ����
const char sprite_invenWeapon[16] = "|//|";

const char sprite_enemy1[10] = " @ (!)_^_";//���� ���� �����
const char sprite_enemy1_leg[2][3][4] =//ĳ���� �̵� �ִϸ��̼�
{ {"-^.", "_^\'", "_^."},
 {".^-", "\'^_", ".^_"} };

const char sprite_enemy2[15] = "---\\ // \\---";//�ð� Ű �����
const char sprite_enemy2L[13] = { "---    =--- " };//����� �����
const char sprite_enemy2R[13] = { " ---=    ---" };//����� �����
const char sprite_enemy2_bullet[2] = "*";//��2 ���Ÿ� ����ü
const char sprite_enemy3_bullet[2] = "o";//��3 ���Ÿ� ����ü
const char sprite_enemy3[16] = "+@@@+@o:o@ @#@ ";//���� ���� �����

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
            tick = GetTickCount();//GetTickCount�Լ��� ���ο����� ȣ��ȴ�.(��, �޼ҵ忡���� ȣ��ȵȴ�)

            UpdateGame();

            if (tick == 0 || kill >= kill_goal)
                break;
        }
    }
    ExitGame();
    return 0;
}



void StartGame() {//���� ���� �Լ� ���� ������ �� ��ġ�� ������ ������.
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

    /*���� ��������*/
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

    objects = (Object**)malloc(sizeof(Object*) * OBJECT_MAX);//����� ������Ʈ�� �̸� ����
    memset(objects, 0, sizeof(Object*) * OBJECT_MAX);//���� 0���� ����
}

void UpdateGame() {//��� ����Ǵ� �Լ�,(1) �� ��� control�Լ��� ȣ��, (2) �� ����, (3)�� �׸���
    ControlUI();   //update mapData(UI)

    ControlObject();   //update mapData(enemy, projecticles, etc...)
    ControlCharacter();   //update mapData(character)

    /*generate Enemy*/
    if (enemy_count < enemy_max && stage_num != 3) {                       //1. ���� ��(100)
        CreateObject(rand() % 90, 35 - 10 * (rand() % 2), 100);
        enemy_count++;
    }
    if (existTimeKey == FALSE && stage_num != 3) {                         //2. �ð��� ����(101)
        int x = rand() % 90, y = 35;
        CreateObject(x, y, 202);
        existTimeKey = TRUE;//�ð��� ���� ���� FALSE�� ����
    }
    if (mob2_switch == FALSE &&stage_num == 1) {                          //3. �����(102)
        CreateObject(0, 17, 102);//L                 
        CreateObject(MAP_X_MAX - 4, 17, 102);//R
        mob2_switch = TRUE;
    }
    if (mob2_switch == FALSE && stage_num == 2) {                          //3. �����(102)
        CreateObject(0, 17, 102);//L                 
        CreateObject(MAP_X_MAX - 4, 17, 102);//R
        CreateObject(35, FLOOR_Y - 2, 102);//�Ʒ�
      
        mob2_switch = TRUE;
    }
    if (enemy_count < enemy_max && mob3_switch == FALSE && stage_num>1 && stage_num != 3) {//4. ���Ÿ� ��(103)    //�ϴ� �ѹ��� ���� �������� �� �Ѹ��� ���̸� ���̻� �������� ����
        int x = rand() % 90, y = 35;
        CreateObject(x, y, 103);
        mob3_switch = TRUE;
    }

    if (stage_num == 3 && isUnlocked == TRUE && bossGenCnt == 0) {                          //4. ����(104)
        CreateObject(50, 50, 203);//           
        bossGenCnt++;
    }

    if (character.tick[0] + 1000 < tick) {//decrease character time
        character.tick[0] = tick;
        character.time -= 1;
    }

    SetCurrentCursorPos(0, 0);
    if (character.tick[4] % 20 >= 10) {//������ �Ծ��� ��
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

void ExitGame() {//���� ���� �Լ�
    for (int i = 0; i < OBJECT_MAX; i++) {//�迭�� ���鼭 freeó��
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

void ControlUI() {//UI�׸���
    memcpy(mapData, floorData, 1 + sizeof(char) * MAP_X_MAX * MAP_Y_MAX);

    DrawBox(1, BASE_Y, 96, 9);//ȭ�� Ʋ �׸���
    DrawBox(1, BASE_Y, MAP_X_MAX, 9);//ȭ�� Ʋ �׸���

    DrawSprite(8, BASE_Y + 2, 5, 1, (char*)"NAME:\"");   //�̸� �׸���
    DrawSprite(13, BASE_Y + 2, strlen(character.name), 1, character.name);

    DrawSprite(8, BASE_Y + 5, 6, 1, (char*)"TIME:");   //�ð� �׸���
    DrawNumber(13, BASE_Y + 5, character.time / 60);
    DrawSprite(15, BASE_Y + 5, 1, 1, (char*)":");
    DrawNumber(16, BASE_Y + 5, (int)character.time % 60);
    /*
    DrawSprite(8, BASE_Y + 6, 6, 1, (char*)"KILL:"); //KILL �׸���
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
            if (have_item >= 0 && have_item < 4) // ������ �Ծ����� 
                DrawSprite(x, BASE_Y + 2, 3, 4, sprite_item[have_item]);
            else if (have_item == -1) // ������ �ȸԾ����� ������ ǥ��
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
            if (i < snitch_hit_count) DrawSprite(137, BASE_Y + 3 + i, 2, 1, (char*)"��");
            else DrawSprite(137, BASE_Y + 3 + i, 2, 1, (char*)"��");

        }
    }


}

void ControlCharacter() {//ĳ���� ���� �Լ�

    ControlGravity(character.coord, character.size, &character.is_jumping, &character.jump_y_max, 0);

    bool move = FALSE, attack = FALSE;//FALSE�� ��� �ش� ���°� �ƴ�
    int x = character.coord[0], y = character.coord[1];//��ǥ�� ����

    use_time_sword = 0;

    if (character.time < 1) {//ü���� ������ ���� ���� ����
        tick = 0;
        LOSE();
        return;
    }
    if (character.tick[4] > 0) {
        character.tick[4]--;
    }

    ///*���� �Է� �� attack �ѱ��*/

    // ��ų T ���
    if (GetAsyncKeyState(0x54) & 0x8000) {

        if (have_item == 0) { //1�� ������ �̸�
            use_item[have_item] = 1;
            character.item_tick[0] = GetTickCount(); //���ǹ� ���� �Ϲ�ȭ ��Ű�� ������ item.tick[have_item] ���� �ϸ� ĳ���Ͱ� ������ ���� ��� ������
            have_item = -1;
            character.item = 0;
        }

        else if (have_item == 1) { //2�� ������ ���
            use_item[have_item] = 1;
            have_item = -1;
            character.item = 0;
        }
        else if (have_item == 2) { //3�� ������ ���
            use_item[have_item] = 1;
            have_item = -1;
            character.item = 0;
        }
        else if (have_item == 3) { //4�� ������ ���
            use_item[have_item] = 1;
            have_item = -1;
            character.item = 0;
        }
    }


    //1�� ������ ��� ����
    if (tick > character.item_tick[0] + 5000) {
        use_item[0] = 0; //1�������� -> �ε���0  = �ð����� ������
    }//5�ʵ��� �ð� ����

    /*attack motion ����*/
    if (character.motion[1]) {// ���� �ٱ��� ���1�� �����ν� 150 ���� ���ݸ�� ���� ����

        if (tick > character.tick[2] + 100) {   //ƽ�� 150 �������� ƽ ����, ���[2] �÷���
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

    if (GetAsyncKeyState(0x45) & 1)//���� ���
        character.skill_set = (character.skill_set == 0) ? 1 : 0;

    if (GetAsyncKeyState(0x51) & 0x8000 && character.skill_set == 0) {//Q��� Į ���
        character.weapon = 0;
        attack = TRUE;
        character.motion[1] = TRUE;
    }

    if (GetAsyncKeyState(0x51) & 0x8000 && character.tick[5] + 600 <= tick && character.skill_set == 1) {//Q��� �� ���
        character.weapon = 1;
        attack = TRUE;
        character.motion[1] = TRUE;
        character.tick[5] = tick;
        CreateObject(character.coord[0], character.coord[1], 200);
    }

    if (GetAsyncKeyState(0x57) & 0x8000 && character.tick[3] + 1200 <= tick && character.skill_set == 0) {//W��� �뽬 ���
        character.accel[0] = character.direction * 6 - 3;
        character.tick[3] = tick;
    }

    if (GetAsyncKeyState(0x57) & 0x8000 && character.skill_set == 1) {//W��� �˹�
        character.weapon = 2;
        attack = TRUE;
        character.motion[1] = TRUE;
    }
    if (GetAsyncKeyState(0x52) & 0x8000 && time_sword == 1) {//R��� �ð��� �� ���
        character.weapon = 0;
        use_time_sword = 1;
        attack = TRUE;
        character.motion[1] = TRUE;
    }
    //���� ������ ���� ���� �׽�Ʈ�ÿ��� ����� ��
    if (GetAsyncKeyState(0x5A) & 0x8000) {//Z��� ��ź ���
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

    /*leg motion ����*/
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


    if (character.tick[4] % 4 == 0) {//�������� ��� ���ؼ�(��ҿ��� �׳� ���ư���, ������ ���� �ÿ��� ���ڰŸ�)
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
        DrawSprite(x + 1, y - 3, 3, 1, "[A]");// . ��ܿ� A�� ����ϱ�

        if (GetAsyncKeyState(0x41) & 0x8000) {// .A ������ ������ �Ծ���
            character.item = objects[index]->kind;
            have_item = objects[index]->kind; // .������  ���� �������� ������
            RemoveObject(index);//�ش� ������ �����
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

    DrawSprite(x, y, 5, 2, sprite_itemBox);// ������ ��� ȭ�鿡 �׷��ֱ�
}

void CreateObject(short x, short y, short kind) {//������Ʈ ���� �Լ�
    int index = 0;
    Object* obj = 0;

    while (TRUE) {//�� ������Ʈ �迭�� ã�� ������ �ݺ�
        if (!objects[index])
            break;
        if (index == OBJECT_MAX)
            return;
        index++;
    }

    obj = (Object*)malloc(sizeof(Object));//���� �� object�� �����Ҵ�
    objects[index] = obj;
    memset(obj, 0, sizeof(Object));//������Ʈ�� �����ϰ� �迭�� ����

    obj->kind = kind;
    obj->coord[0] = x; obj->coord[1] = y;
    obj->tick[0] = 0;//�� ���� �´� ���� ����

    if (kind == 198) {//��3 �Ѿ�
        obj->time = rand();
        obj->size[0] = 1;
        obj->size[1] = 1;
        obj->tick[1] = GetTickCount();//
        obj->tick[2] = GetTickCount();//���� Ÿ�̹� ���� ƽ
        obj->damage = 10;
        obj->motion[0] = 0;
        obj->motion[1] = 0;
        obj->originPos[0] = x;//���� ��� ��3 ��ġ
        obj->originPos[1] = y;
        obj->characterCoord[0] = character.coord[0];//���� ��� �÷��̾� ��ġ
        obj->characterCoord[1] = character.coord[1];
    }

    if (kind == 199) {//��2 �Ѿ�
        obj->time = rand();
        obj->size[0] = 1;
        obj->size[1] = 1;
        obj->tick[1] = GetTickCount();//
        obj->tick[2] = GetTickCount();//���� Ÿ�̹� ���� ƽ
        obj->damage = 10;
        obj->motion[0] = 0;
        obj->motion[1] = 0;
        obj->originPos[0] = x;//���� ��� ��2 ��ġ
        obj->originPos[1] = y;
        obj->characterCoord[0] = character.coord[0];//���� ��� �÷��̾� ��ġ
        obj->characterCoord[1] = character.coord[1];
    }

    if (kind == 200) {//�Ѿ�
        obj->time = rand();
        obj->coord[0] = character.coord[0] - 4 + 10 * character.direction;
        obj->coord[1] = character.coord[1] - 1;
        obj->size[0] = 1;
        obj->size[1] = 1;
        obj->tick[1] = GetTickCount();
        obj->direction = character.direction;
    }

    if (kind == 201) {//��ź
        obj->time = rand();
        obj->coord[0] = character.coord[0] - 4 + 10 * character.direction;
        obj->coord[1] = character.coord[1] - 1;
        obj->size[0] = 1;
        obj->size[1] = 1;
        obj->tick[1] = GetTickCount();
        obj->direction = character.direction;
    }

    if ((kind < 100 || kind > 199) && kind != 203) {//�����̳� �������� ���
        obj->time = rand();
        obj->tick[1] = 0;
        obj->tick[2] = 0;
        obj->tick[3] = 0;
        obj->is_jumping = 4;
    }

    if (kind > 99 && kind < 198) {//������ ���
        obj->time = stat_enemy[kind - 100][0];
        obj->size[0] = stat_enemy[kind - 100][1];
        obj->size[1] = stat_enemy[kind - 100][2];
        obj->tick[1] = stat_enemy[kind - 100][3];
        obj->tick[2] = stat_enemy[kind - 100][4];
        obj->tick[3] = stat_enemy[kind - 100][5];
        obj->tick[4] = stat_enemy[kind - 100][6];
        obj->motion[0] = 0;
    }
    if (kind == 202) {//����ġ�� ���
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
    if (kind == 203) {//������ ���
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

void ControlObject() {//������Ʈ ������ ���� Ȯ��
    for (int i = 0; i < OBJECT_MAX; i++) {
        if (objects[i]) {
            if (objects[i]->kind < 100)//kind�� 99������ ControlItem�Լ� ����
                ControlItem(i);
            else if (objects[i]->kind < 198)//kind�� 100���� 199������ ControlEnemy�Լ� ����
                ControlEnemy(i);
            else if (objects[i]->kind == 199)//199: mob2���Ÿ� ����
                ControlMob2Bullet(i);
            else if (objects[i]->kind == 198)//198: mob3���Ÿ� ����
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
    short x = objects[index]->coord[0], y = objects[index]->coord[1];//���� ��ǥ
    short at_coord[2] = { character.coord[0] - 5 + 8 * character.direction, character.coord[1] };//ĳ���� ��ǥ
    short at_size[2] = { 5, 3 };//���� ������
    short item_code = rand() % 100;
    short attack = FALSE;

    /*0.�� ��� ó��*/
    if (snitch_hit_count >= snitch_hit_count_max) {
        WIN();
        return;
    }
    else if (objects[index]->time < 1) LOSE();

    /*1.�� �ð� -1�� ó��*/
    if (objects[index]->tick[0] + 1000 < tick && use_item[0] != 1) { //������1�� ��� �����ľ�
        objects[index]->tick[0] = tick; //������ ���� tick
        objects[index]->time -= 1.0;
    }


    /*2.���� ���ݿ� �¾��� ��� ó��*/
    if (character.motion[2] == 1 && use_time_sword == 1 && CollisionCheck(objects[index]->coord, at_coord, objects[index]->size, at_size)) {
        objects[index]->tick[0] = tick;
        character.time -= character.power;//�߰���
        snitch_hit_count++;
        snitch_hit = TRUE;// �´� ���� ǥ������

        //����ġ �ڷ���Ʈ
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
    if (objects[index]->tick[4] + 5000 < tick) {//���� ��ǥ�� �Ѿ��      //tick[4]: ������ǥ ������ ���� ƽ



        objects[index]->coord[0] = snitch_next_x;
        objects[index]->coord[1] = snitch_next_y;
        objects[index]->accel[0] = (float)snitch_next_x;
        objects[index]->accel[1] = (float)snitch_next_y;



        objects[index]->tick[4] = tick;
        snitch_next_x = rand() % (MAP_X_MAX - objects[index]->size[0]) + objects[index]->size[0];  //�� ��ǥ ����
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
    /*�ð��� ���� Ÿ��Ű�� �ٸ� ��ġ�� �̵��ϴ� ���: �ش� ��ǥ�� �̵�, 5�� �ȿ� ���־����*/

    objects[index]->coord[0] = (int)objects[index]->accel[0];
    objects[index]->coord[1] = (int)objects[index]->accel[1];

    DrawSprite(objects[index]->coord[0] + 1, objects[index]->coord[1] - 2, objects[index]->size[0], objects[index]->size[1], sprite_enemy2);
    /*���� �ð� ���(�Ӹ� ��)*/
    DrawNumber(objects[index]->coord[0] + objects[index]->size[0] / 2 - NumLen(objects[index]->time) / 2 + 1, objects[index]->coord[1] - 3, (int)objects[index]->time);
}

void ControlMob1(int index) {
    short x = objects[index]->coord[0], y = objects[index]->coord[1];//���� ��ǥ
    short at_coord[2] = { character.coord[0] - 5 + 8 * character.direction, character.coord[1] };//ĳ���� ��ǥ
    short at_size[2] = { 5, 3 };//���� ������
    short item_code = rand() % 100;
    short attack = FALSE;

    /* ���ݰ˻� */
    if (abs(objects[index]->coord[0] - at_coord[0]) < 13) attack = TRUE;

    /*  ���� ��� ��:
    (1) �¿츦 �����Ÿ�
    (2) PC���� �ɾ  */
    if (attack == FALSE && use_item[0] != 1) { // ������ 1�� üũ

        /*(1) �¿츦 �����Ÿ�*/
        if (abs(objects[index]->coord[0] - at_coord[0]) > 40) {

            /*���⼳��*/
            srand(time(NULL) * (index + 1));
            int moveOrStop = rand() % 3;
            if (moveOrStop == 1) {
                objects[index]->direction = TRUE;

                if (tick > objects[index]->tick[4] + 170) {
                    objects[index]->tick[4] = tick;
                    if (objects[index]->coord[0] < MAP_X_MAX) { objects[index]->coord[0]++; }
                }
                /*leg motion*/
                if (tick > objects[index]->tick[3] + 170) {   //leg tick, �ٸ� ������ �ӵ�   
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
                if (tick > objects[index]->tick[3] + 170) {   //leg tick, �ٸ� ������ �ӵ�   
                    objects[index]->tick[3] = tick;
                    objects[index]->motion[0]++;
                    if (objects[index]->motion[0] > 3)
                        objects[index]->motion[0] = 1;
                }
            }
            //else {} //�ƹ��͵� ����          
        }

        /*(3) PC�� �����*/
        else if (abs(objects[index]->coord[0] - at_coord[0]) > 12 && abs(objects[index]->coord[0] - at_coord[0]) < 41) {

            /*���⼳��*/
            if (!EnemyPositionX(x, objects[index]->size[0])) objects[index]->direction = TRUE;
            else  objects[index]->direction = FALSE;

            /*��ǥ �̵� ����*/
            if (!EnemyPositionX(x, objects[index]->size[0])) {     //�̵��ӵ�
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
            /*leg motion ����*/
            if (tick > objects[index]->tick[3] + 170) {   //leg tick, �ٸ� ������ �ӵ�   
                objects[index]->tick[3] = tick;
                objects[index]->motion[0]++;

                if (objects[index]->motion[0] > 3)
                    objects[index]->motion[0] = 1;
            }
        }
    }

    /*(4)���� ���� ��: ���� ��ġ��*/
    if (attack == TRUE && use_item[0] != 1 && character.coord[1] == objects[index]->coord[1]) { //attack == TRUE, ������ 1�� üũ       
        objects[index]->motion[0] = 0;

        if (objects[index]->tick[1] + objects[index]->tick[2] < tick) {//���� ����� �ð� + ���� �뽬 �� ������ ���� ����
            objects[index]->tick[1] = tick;
            objects[index]->tick[2] = 1000 + rand() % 1000;// �뽬�ϴ� �ð� ������ ����
            //objects[index]->accel[1] = rand() / (float)RAND_MAX / 2 - 1.2;//����

            if (!EnemyPositionX(x, objects[index]->size[0]))
                objects[index]->accel[0] = 2.4 - rand() / (float)RAND_MAX;
            else
                objects[index]->accel[0] = rand() / (float)RAND_MAX - 2.4;
        }
    }

    /*���� ĳ������ �浹 �� ������ ó��*/
    if (character.tick[4] == 0 && CollisionCheck(objects[index]->coord, character.coord, objects[index]->size, character.size)) {//���� ó�� �Լ�
        character.tick[4] = 80;//�������·� ����, 80���� ƽ���� ����(�� 3��) ������ �� ����
        character.time -= 10;
    }

    MoveControl(objects[index]->coord, objects[index]->accel, objects[index]->size, &objects[index]->flyTime);//���� ������
    DrawSprite(x + 1, y - 1, objects[index]->size[0], objects[index]->size[1], sprite_enemy1);//������ ���� �׸���

    if (objects[index]->motion[0] > 0)
        DrawSprite(x + 1, y + 1, 3, 1, sprite_enemy1_leg[objects[index]->direction][objects[index]->motion[0] - 1]);   //draw leg motion
}

void ControlMob2(int index) {//
    int x = objects[index]->coord[0];
    int y = objects[index]->coord[1];
    short at_coord[2] = { character.coord[0] - 5 + 8 * character.direction, character.coord[1] };//ĳ���� ��ǥ

    /*��, �� ����� ���*/
    if (x < MAP_X_MAX / 2)
        DrawSprite(x + 1, y - 1, objects[index]->size[0], objects[index]->size[1], sprite_enemy2L);//���ʿ� ��ġ�� ��2
    else

        DrawSprite(x + 1, y - 1, objects[index]->size[0], objects[index]->size[1], sprite_enemy2R);//������

    /*����*/
    if (objects[index]->tick[4] + 4000 < tick) {//tick[3]���� ����ü�� ����� ����
        objects[index]->tick[4] = tick;

        /*���ݽ����� �÷��̾��� ��ġ�� ����Ѵ�(CreateObject).*/
        /*���� ����ü �����: ȭ�� �߾��� �������� �ٸ� ��ġ �ʱ�ȭ*/
        if (x < MAP_X_MAX / 2) {
            CreateObject(objects[index]->coord[0] + 4, objects[index]->coord[1], 199); //���Ÿ� �� ����ü ����, ControlObject���� ȣ�� //���� �� 2
            Mob2Bullet++;
        }
        else {
            CreateObject(objects[index]->coord[0] - 4, objects[index]->coord[1], 199); //���Ÿ� �� ����ü ����, ControlObject���� ȣ�� //������ �� 2
            Mob2Bullet++;
        }

    }

}

void ControlMob2Bullet(int index) {
    int x = objects[index]->originPos[0];
    int y = objects[index]->originPos[1];
    short at_coord[2] = { objects[index]->coord[0],objects[index]->coord[1] };
    short pc_coord[2] = { character.coord[0] - 5 + 8 * character.direction, character.coord[1] };//ĳ���� ��ǥ
    short at_size[2] = { 1, 1 };//���� ������(���� �� ��ư� �ϱ� ���� 2 X 2)
    short pc_size[2] = { 3, 3 };//�÷��̾� ������

    if (objects[index]->tick[2] + 2000 < tick) {
        RemoveObject(index);
        Mob2Bullet--;
        return;
    }
    if (objects[index]->tick[1] + 50 < tick) {//tick[1]: 
        objects[index]->tick[1] = tick;

        objects[index]->coord[0] = (((objects[index]->motion[0]) * (objects[index]->characterCoord[0]) + (30 - objects[index]->motion[0]) * x)) / 30; // objects[index]->motion[0] : 10- objects[index]->motion[0] ������
        objects[index]->coord[1] = ((objects[index]->motion[1] * (objects[index]->characterCoord[1]) + (30 - objects[index]->motion[1]) * y)) / 30; // objects[index]->motion[0] : 10- objects[index]->motion[0] ������

        objects[index]->motion[0]++;                //*motion�� �������� i���ҷ� �����
        objects[index]->motion[1]++;
        if (objects[index]->motion[0] == 31) objects[index]->motion[0] = 0;
        if (objects[index]->motion[1] == 31) objects[index]->motion[1] = 0;
    }

    /*���� ��� ��� ����*/
    if (objects[index]->coord[0] < 0 || objects[index]->coord[0] > MAP_X_MAX || objects[index]->coord[1] < 5 || objects[index]->coord[1] > FLOOR_Y) {
        RemoveObject(index);
        Mob2Bullet--;
        return;//��ü�� ��������Ƿ� �޼ҵ� ������Ѿ���
    }
    if (character.tick[4] == 0 && CollisionCheck(at_coord, pc_coord, at_size, pc_size)) {//�浹 �� 
        character.tick[4] = 80;//�������·� ����, 80���� ƽ���� ����(�� 3��) ������ �� ����
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
    short at_coord[2] = { character.coord[0] - 5 + 8 * character.direction, character.coord[1] };//ĳ���� ��ǥ
    short  dist = 0;
    bool relativePosWithPC_X = EnemyPositionX(x, 5);
    bool relativePosWithPC_Y = EnemyPositionY(y, 3);
    //������Ʈ�� direction�ʵ带 mob3������ ���� ���� �ȿ� �ִ��� ���θ� ������ �뵵�� ����Ѵ�.

    /*1.������*/

    /*1-1. ���� ��ġ üũ, �̵����� ����*/
    if (objects[index]->tick[1] + 2000 < tick) {//���� ���� ���� ������
        objects[index]->tick[1] = tick;
        dist = Distance(x, y, character.coord[0], character.coord[1]);

        if (dist < 25)  objects[index]->direction = TRUE;
        else objects[index]->direction = FALSE;
    }

    if (objects[index]->tick[2] + 100 < tick) {
        objects[index]->tick[2] = tick;

        //relativePosWithPC_X == FALSE: ���� PC�� ���ʿ� ����
        //relativePosWithPC_Y == FALSE: ���� PC�� �Ʒ��ʿ� ����

        /*1-2. �÷��̾� ������ �ٰ�����*/
        if (objects[index]->direction == FALSE) {

            if (relativePosWithPC_X == TRUE && relativePosWithPC_Y == TRUE) {//(1) pc���� 1��и� ->3��и� �������� �̵�
                if (objects[index]->motion[0] % 6 == 2) objects[index]->coord[1]++;// x���� 1ĭ, y����1ĭ, x���� 1ĭ, x���� 1ĭ
                else if (objects[index]->motion[0] % 4 == 4 || objects[index]->motion[0] % 4 == 5) {
                    objects[index]->coord[0] += (rand() % 3 - 1);
                    objects[index]->coord[1] += (rand() % 3 - 1);
                    objects[index]->motion[0]++;
                }
                else objects[index]->coord[0]--;
                objects[index]->motion[0]++;
            }
            else if (relativePosWithPC_X == TRUE && relativePosWithPC_Y == FALSE) {//(2) pc���� 2��и� -> 4��и� �������� �̵�
                if (objects[index]->motion[0] % 6 == 2) objects[index]->coord[1]--;// x���� 1ĭ, y����1ĭ, x���� 1ĭ, x���� 1ĭ
                else if (objects[index]->motion[0] % 4 == 4 || objects[index]->motion[0] % 4 == 5) {
                    objects[index]->coord[0] += (rand() % 3 - 1);
                    objects[index]->coord[1] += (rand() % 3 - 1);
                    objects[index]->motion[0]++;
                }
                else objects[index]->coord[0]--;
                objects[index]->motion[0]++;
            }
            else if (relativePosWithPC_X == FALSE && relativePosWithPC_Y == FALSE) {//(3) pc���� 3��и� -> 1��и� �������� �̵� 
                if (objects[index]->motion[0] % 6 == 2) objects[index]->coord[1]--;// x���� 1ĭ, y����1ĭ, x���� 1ĭ, x���� 1ĭ
                else if (objects[index]->motion[0] % 4 == 4 || objects[index]->motion[0] % 4 == 5) {
                    objects[index]->coord[0] += (rand() % 3 - 1);
                    objects[index]->coord[1] += (rand() % 3 - 1);
                    objects[index]->motion[0]++;
                }
                else objects[index]->coord[0]++;//�̰Ÿ� ����
                objects[index]->motion[0]++;
            }
            else if (relativePosWithPC_X == FALSE && relativePosWithPC_Y == TRUE) {//(4) pc���� 4��и� -> 2��и� �������� �̵�
                if (objects[index]->motion[0] % 6 == 2) objects[index]->coord[1]++;// x���� 1ĭ, y����1ĭ, x���� 1ĭ, x���� 1ĭ
                else if (objects[index]->motion[0] % 4 == 4 || objects[index]->motion[0] % 4 == 5) {
                    objects[index]->coord[0] += (rand() % 3 - 1);
                    objects[index]->coord[1] += (rand() % 3 - 1);
                    objects[index]->motion[0]++;
                }
                else objects[index]->coord[0]++;
                objects[index]->motion[0]++;
            }

        }
        /*1-2. �÷��̾�Լ� �־�����*/
        else {
            if (relativePosWithPC_X == TRUE && relativePosWithPC_Y == TRUE) {//(1) pc���� 1��и� ->3��и� �������� �̵�
                if (objects[index]->motion[0] % 6 == 2) objects[index]->coord[1]--;// x���� 1ĭ, y����1ĭ, x���� 1ĭ, x���� 1ĭ
                else if (objects[index]->motion[0] % 4 == 4 || objects[index]->motion[0] % 4 == 5) {
                    objects[index]->coord[0] += (rand() % 3 - 1);
                    objects[index]->coord[1] += (rand() % 3 - 1);
                    objects[index]->motion[0]++;
                }
                else objects[index]->coord[0]++;
                objects[index]->motion[0]++;
            }
            else if (relativePosWithPC_X == TRUE && relativePosWithPC_Y == FALSE) {//(2) pc���� 2��и� -> 4��и� �������� �̵�
                if (objects[index]->motion[0] % 6 == 2) objects[index]->coord[1]++;// x���� 1ĭ, y����1ĭ, x���� 1ĭ, x���� 1ĭ
                else if (objects[index]->motion[0] % 4 == 4 || objects[index]->motion[0] % 4 == 5) {
                    objects[index]->coord[0] += (rand() % 3 - 1);
                    objects[index]->coord[1] += (rand() % 3 - 1);
                    objects[index]->motion[0]++;
                }
                else objects[index]->coord[0]++;
                objects[index]->motion[0]++;
            }
            else if (relativePosWithPC_X == FALSE && relativePosWithPC_Y == FALSE) {//(3) pc���� 3��и� -> 1��и� �������� �̵� 
                if (objects[index]->motion[0] % 6 == 2) objects[index]->coord[1]++;// x���� 1ĭ, y����1ĭ, x���� 1ĭ, x���� 1ĭ
                else if (objects[index]->motion[0] % 4 == 4 || objects[index]->motion[0] % 4 == 5) {
                    objects[index]->coord[0] += (rand() % 3 - 1);
                    objects[index]->coord[1] += (rand() % 3 - 1);
                    objects[index]->motion[0]++;
                }
                else objects[index]->coord[0]--;
                objects[index]->motion[0]++;
            }
            else if (relativePosWithPC_X == FALSE && relativePosWithPC_Y == TRUE) {//(4) pc���� 4��и� -> 2��и� �������� �̵�          
                if (objects[index]->motion[0] % 6 == 2) objects[index]->coord[1]--;// x���� 1ĭ, y����1ĭ, x���� 1ĭ, x���� 1ĭ
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
    /*�� ���ѵα�*/
    if (objects[index]->coord[0] + objects[index]->size[0] >= MAP_X_MAX) objects[index]->coord[0]--;
    if (objects[index]->coord[0] - objects[index]->size[0] < 0) objects[index]->coord[0]++;
    if (objects[index]->coord[1] + objects[index]->size[1] >= BASE_Y) objects[index]->coord[1]--;
    if (objects[index]->coord[1] - objects[index]->size[1] < 0) objects[index]->coord[1]++;
    MoveControl(objects[index]->coord, objects[index]->accel, objects[index]->size, &objects[index]->flyTime);//���� ������
    DrawSprite(x + 1, y - 1, objects[index]->size[0], objects[index]->size[1], sprite_enemy3);

    /*2.����*/
    if (objects[index]->tick[3] + 2000 < tick) {//���� ���� ���� ������
        objects[index]->tick[3] = tick;
        dist = Distance(x, y, character.coord[0], character.coord[1]);

        if (dist < 23)  objects[index]->attack = TRUE;
        else objects[index]->attack = FALSE;

        if (objects[index]->attack == TRUE) {
            CreateObject(objects[index]->coord[0] + 4, objects[index]->coord[1], 198); //���Ÿ� �� ����ü ����, ControlObject���� ȣ�� //���� �� 2
            Mob3Bullet++;
        }
    }
    /*���� ���� �� ��Ʈ�� ����: 5�� 5�� ������ �ش�. */
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
    //short pc_coord[2] = { character.coord[0] - 5 + 8 * character.direction, character.coord[1] };//ĳ���� ��ǥ
    short pc_coord[2] = { character.coord[0], character.coord[1] };//ĳ���� ��ǥ
    short at_size[2] = { 3, 3 };//���� ������(���� �� �ߵǰ� �ϱ� ���� 3 X 3)
    short pc_size[2] = { 3, 3 };//�÷��̾� ������

    if (objects[index]->tick[2] + 500 < tick) {
        RemoveObject(index);
        Mob3Bullet--;
        return;
    }
    if (objects[index]->tick[1] + 30 < tick) {//tick[1]: 
        objects[index]->tick[1] = tick;

        objects[index]->coord[0] = (((objects[index]->motion[0]) * (objects[index]->characterCoord[0]) + (15 - objects[index]->motion[0]) * x)) / 15; // objects[index]->motion[0] : 10- objects[index]->motion[0] ������
        objects[index]->coord[1] = ((objects[index]->motion[1] * (objects[index]->characterCoord[1]) + (15 - objects[index]->motion[1]) * y)) / 15; // objects[index]->motion[0] : 10- objects[index]->motion[0] ������

        objects[index]->motion[0]++;                //*motion�� �������� i���ҷ� �����
        objects[index]->motion[1]++;
        if (objects[index]->motion[0] == 16) objects[index]->motion[0] = 0;
        if (objects[index]->motion[1] == 16) objects[index]->motion[1] = 0;
    }

    /*���� ��� ��� ����*/
    if (objects[index]->coord[0] < 0 || objects[index]->coord[0] > MAP_X_MAX || objects[index]->coord[1] < 5 || objects[index]->coord[1] > FLOOR_Y) {
        RemoveObject(index);
        Mob3Bullet--;
        return;//��ü�� ��������Ƿ� �޼ҵ� ������Ѿ���
    }

    /*��Ʈ�� ó��*/
    if (character.tick[4] == 0 && CollisionCheck(at_coord, pc_coord, at_size, pc_size)) {//�浹 �� 
        character.tick[4] = 201;//�������·� ����, 200���� ƽ���� ����(�� 6��) �ٸ� ������ �� ���� // �Ϲݰ���(80��ƽ)���� ��� �ش�.
        isPoisoned = TRUE;
        RemoveObject(index);
        Mob3Bullet--;
        return;
    }
    /*���ݿ� ���� ��� Bullet ��ü�� �����ǹǷ� ��Ʈ�� �����ϴ� �ڵ�� �� ���Ŀ� �ۼ��ϸ� ������ �ȵȴ�.*/
    /*ControlMob3�� �ۼ�*/

    DrawSprite(objects[index]->coord[0], objects[index]->coord[1], 1, 1, "o");
}

void ControlEnemy(int index) {//���� ó�� �Լ�
    if (objects[index]->kind == 100) ControlGravity(objects[index]->coord, objects[index]->size, &objects[index]->is_jumping, &objects[index]->jump_y_max, index);
    short x = objects[index]->coord[0], y = objects[index]->coord[1];//���� ��ǥ
    short at_coord[2] = { character.coord[0] - 5 + 8 * character.direction, character.coord[1] };//ĳ���� ��ǥ
    short at_size[2] = { 5, 3 };//���� ������
    short item_code = rand() % 100;
    short attack = FALSE;

    /*0.�� ��� ó��*/
    if (objects[index]->time < 1) {//���� ��� ó��
        if (item_code >= 84) //������ ��� Ȯ�� ���� ���ڸ� ������ Ȯ���� �޶���
            CreateObject(x + objects[index]->size[0] / 2 - 1, y - 1, 0); //������ 1�� �������� ���

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

    /*1.�� �ð� -1�� ó��*/
    if (objects[index]->tick[0] + 1000 < tick && use_item[0] != 1) { //������1�� ��� �����ľ�
        objects[index]->tick[0] = tick; //������ ���� tick
        objects[index]->time -= 1.0;
    }

    /*���� �ð� ���(�Ӹ� ��)*/
    DrawNumber(x + objects[index]->size[0] / 2 - NumLen(objects[index]->time) / 2 + 1, y - 2, (int)objects[index]->time);

    /*2.���� ���ݿ� �¾��� ��� ó��*/
    if (character.motion[2] == 1 && use_time_sword == 0 && CollisionCheck(objects[index]->coord, at_coord, objects[index]->size, at_size)) {
        objects[index]->tick[0] = tick;
        objects[index]->time -= character.power;
        character.time += character.power;//�߰���
        //objects[index]->accel[1] = -0.55; //�ǰݽ� �밢������ �ڷ� ��

        if (character.motion[3] == 3) {//�ָ԰��ݿ� �¾��� ���
            objects[index]->time -= character.power;//�ǰ� �� ü�� ����
            character.time += character.power;//�߰���
        }

        if (character.weapon == 2) {//�˹� ���� ���ӵ�
            if (!EnemyPositionX(x, objects[index]->size[0]))
                objects[index]->accel[0] = -5;
            else
                objects[index]->accel[0] = 5;
        }
        else {//�Ϲ� ���� �ǰ� ���ӵ�
            if (!EnemyPositionX(x, objects[index]->size[0]))
                objects[index]->accel[0] = -0.75;
            else
                objects[index]->accel[0] = 0.75;
        }
    }
    /////////////////////*     �� ������ ���� ���� ����      */////////////////////
    switch (objects[index]->kind) {

    case 100:  //������ ��
        ControlMob1(index);
        break;
    case 102:  //��ġ�� ��
        ControlMob2(index);
        break;
    case 103:  //������ ��
        ControlMob3(index);
        break;
    }


}

void FindBossFlash() { // �ǻ���3 ������ �� �־����� �Ǵ�
    int flash_sum = 0;

    flash_1[0] = flash_arr[flash_index1][0], flash_1[1] = flash_arr[flash_index1][1];
    flash_2[0] = flash_arr[flash_index2][0], flash_2[1] = flash_arr[flash_index2][1];
    flash_3[0] = flash_arr[flash_index3][0], flash_3[1] = flash_arr[flash_index3][1];


    /*�ǻ��� ���: 3������ �ִϸ��̼� ȿ�� �ֱ�*/
    if (flash_tick + 700 < tick) { flash_animation++; flash_tick = tick; }

    /*�ǻ��� 1*/
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
    /*�ǻ��� 2*/
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
    /*�ǻ��� 3*/
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
        DrawSprite(flash_1[0], flash_1[1] - 1, 3, 1, "[A]");// . ��ܿ� A�� ����ϱ�

        if (GetAsyncKeyState(0x41) & 0x8000) {//  A ������ �ð��� ���� ���� ����

            DrawSprite(flash_1[0], flash_1[1], 3, 4, sprite_enemy2);
            flash_cnt1 = 3;
        }

    }
    else if (CollisionCheck(flash_2, character.coord, flash_size, character.size)) {
        DrawSprite(flash_2[0], flash_2[1] - 1, 3, 1, "[A]");// . ��ܿ� A�� ����ϱ�

        if (GetAsyncKeyState(0x41) & 0x8000) {//  A ������ �ð��� ���� ���� ����

            DrawSprite(flash_2[0], flash_2[1], 3, 4, sprite_enemy2);
            flash_cnt2 = 3;
        }
    }
    else if (CollisionCheck(flash_3, character.coord, flash_size, character.size)) {
        DrawSprite(flash_3[0], flash_3[1] - 1, 3, 1, "[A]");// . ��ܿ� A�� ����ϱ�

        if (GetAsyncKeyState(0x41) & 0x8000) {//  A ������ �ð��� ���� ���� ����

            DrawSprite(flash_3[0], flash_3[1], 3, 4, sprite_enemy2);
            flash_cnt3 = 3;
        }
    }

    flash_sum = flash_cnt1 + flash_cnt2 + flash_cnt3; //3���� �������� ���ĺ�

    if (flash_sum == 9) {
        isVisible = TRUE;
        isVisible_first++;
        Sleep(1000); //�ణ ��ĩ�ϴ� ������ ����
        flash_tick = GetTickCount();
    }


}

void ControlBoss(int index) { // �κкκ� ����
    short x = objects[index]->coord[0];
    short y = objects[index]->coord[1];
    short at_coord[2] = { character.coord[0] - 5 + 8 * character.direction, character.coord[1] };//ĳ���� ��ǥ
    short at_size[2] = { 5, 3 };//���� ������

    if (objects[index]->time <= 0) {
        Sleep(2000);
        CLEAR();
    }

    //���� �ð� �߰�
    if (objects[index]->tick[0] + 1000 < tick && use_item[0] != 1) { //������1�� ��� �����ľ�
        objects[index]->tick[0] = tick; //������ ���� tick
        objects[index]->time += 1.0;
    }

    if ((flash_index1 == flash_index2) && (flash_index1 == flash_index3) && (flash_index3 == flash_index2)) {//
        while (1) { //�ǻ��� �� ��ǥ�� �ٸ��� �����
            flash_index1 = rand() % 10;
            flash_index2 = rand() % 10;
            flash_index3 = rand() % 10;
            if ((flash_index1 != flash_index2) && (flash_index1 != flash_index3) && (flash_index3 != flash_index2))break;
        }
    }

    /*0.���*/

    if (isVisible == TRUE) { // isvisible�� true ���� ������ ����
        if (flash_tick + 20000 <= GetTickCount()) { //20�� ���� ���� ���� ���
            isVisible = FALSE;
            flash_cnt1 = 0, flash_cnt2 = 0, flash_cnt3 = 0;
            flash_index1 = -1, flash_index2 = -1, flash_index3 = -1;
        }

        TextColor(WHITE);//
        //DrawSprite(x+1, y-5, objects[index]->size[0], objects[index]->size[1], sprite_boss_off);

        /*1.������*/


        /*
        if (objects[index]->coord[0] == 20 && objects[index]->coord[1] == 10) { //�ʱ�ȭ
            objects[index]->coord[0] = 10;
            objects[index]->coord[1] = 10;
            objects[index]->accel[0] = 10.0;
            objects[index]->accel[1] = 10.0;
        }
        */

        if (objects[index]->tick[4] + 5000 < tick) {//���� ��ǥ�� �Ѿ��      //tick[4]: ������ǥ ������ ���� ƽ
            bossSkillIsDash = FALSE;


            objects[index]->coord[0] = snitch_next_x;
            objects[index]->coord[1] = snitch_next_y;
            objects[index]->accel[0] = (float)snitch_next_x;
            objects[index]->accel[1] = (float)snitch_next_y;



            objects[index]->tick[4] = tick;
            snitch_next_x = rand() % (MAP_X_MAX - objects[index]->size[0]) + objects[index]->size[0];  //�� ��ǥ ����
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

        /*�ð��� ���� Ÿ��Ű�� �ٸ� ��ġ�� �̵��ϴ� ���: �ش� ��ǥ�� �̵�, 5�� �ȿ� ���־����*/
        objects[index]->coord[0] = (int)objects[index]->accel[0];
        objects[index]->coord[1] = (int)objects[index]->accel[1];

        /*���� �ð� ���(�Ӹ� ��)*/
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


        /*2.����*/

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


        /*3.�ǰ� �� + ���ó��*/

        if (character.motion[2] == 1 && use_time_sword == 0 && CollisionCheck(objects[index]->coord, at_coord, objects[index]->size, at_size)) { //�Ϲ� ����
            objects[index]->tick[0] = tick;
            objects[index]->time -= 100;
            character.time += 100;//�߰���
            //objects[index]->accel[1] = -0.55; //�ǰݽ� �밢������ �ڷ� ��

            if (character.motion[3] == 3) {//�ָ԰��ݿ� �¾��� ���
                objects[index]->time -= 100;//�ǰ� �� ü�� ����
                character.time += 100;//�߰���
            }

            //����ġ �ڷ���Ʈ
            objects[index]->coord[0] = rand() % (MAP_X_MAX - objects[index]->size[0]) + objects[index]->size[0];
            objects[index]->coord[1] = rand() % (BASE_Y - objects[index]->size[1]) + objects[index]->size[1];
            objects[index]->tick[4] -= 5000;

        }

        if (character.motion[2] == 1 && use_time_sword == 1 && CollisionCheck(objects[index]->coord, at_coord, objects[index]->size, at_size)) { //�ð��� �� ����
            objects[index]->tick[0] = tick;
            objects[index]->time -= 1000;
            character.time += 100;//�߰���

            //����ġ �ڷ���Ʈ
            objects[index]->coord[0] = rand() % (MAP_X_MAX - objects[index]->size[0]) + objects[index]->size[0];
            objects[index]->coord[1] = rand() % (BASE_Y - objects[index]->size[1]) + objects[index]->size[1];
            objects[index]->tick[4] -= 5000;

            snitch_hit_count++;
            snitch_hit = TRUE;// �´� ���� ǥ������
            time_sword = 0;

        }

    }
    else {// ���� �Ұ� ���

        TextColor(YELLOW);
        FindBossFlash();

        if (isVisible_first >= 1) { // ���� �������� ó���� ������ �������� ������ �յն��ٴϰ� �ؾ���
            DrawSprite(x + 1, y - 5, objects[index]->size[0], objects[index]->size[1], sprite_boss_on);

            /*1.������*/

            /*
            if (objects[index]->coord[0] == 20 && objects[index]->coord[1] == 10) { //�ʱ�ȭ
                objects[index]->coord[0] = 10;
                objects[index]->coord[1] = 10;
                objects[index]->accel[0] = 10.0;
                objects[index]->accel[1] = 10.0;
            }
            */


            if (objects[index]->tick[4] + 5000 < tick) {//���� ��ǥ�� �Ѿ��      //tick[4]: ������ǥ ������ ���� ƽ
                bossSkillIsDash = FALSE;

                /*
                objects[index]->coord[0] = snitch_next_x;
                objects[index]->coord[1] = snitch_next_y;
                objects[index]->accel[0] = (float)snitch_next_x;
                objects[index]->accel[1] = (float)snitch_next_y;
                */


                objects[index]->tick[4] = tick;
                snitch_next_x = rand() % (MAP_X_MAX - objects[index]->size[0]) + objects[index]->size[0];  //�� ��ǥ ����
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

            /*�ð��� ���� Ÿ��Ű�� �ٸ� ��ġ�� �̵��ϴ� ���: �ش� ��ǥ�� �̵�, 5�� �ȿ� ���־����*/
            objects[index]->coord[0] = (int)objects[index]->accel[0];
            objects[index]->coord[1] = (int)objects[index]->accel[1];

            /*���� �ð� ���(�Ӹ� ��)*/
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


            /*2.����*/

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


            /*3.�ǰ� �� + ���ó��*/
        }

    }


}

void BossAttackRazer(int index) {
    if (first_attack == 0 && bossSkillIsRazer == FALSE) return;
    int r_x[5][2] = { {0, 28}, {29, 56},{57, 84},{85,112},{113,140} };
    int r_y[3][2] = { {0, 12}, {13,25}, {26,37} };
    if (raser_tick[0] <= GetTickCount() && first_attack == 0) {//ó�� ��ų �������� Ȯ��
        raser_tick[0] = GetTickCount();
        first_attack = 1;
    }

    if (raser_tick[0] + 700 >= GetTickCount() && first_attack == 1) {//���ۺ��� 0.7�ʱ��� ��ǥ�� ����
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
        //0.7~2�ʻ��� ��� ����

        DrawSprite(raserPos_x[0], 0, 1, 38, "**************************************");
        DrawSprite(raserPos_x[1], 0, 1, 38, "**************************************");


        DrawSprite(0, raserPos_y[0], 140, 1, "********************************************************************************************************************************************");
        DrawSprite(0, raserPos_y[1], 140, 1, "********************************************************************************************************************************************");
        for (int i = 0; i < MAP_X_MAX * MAP_Y_MAX; i++) { //�� �����
            if (floorData[i] == '=') mapData[i] = '=';
        }

    }
    if (raser_tick[0] + 5000 >= GetTickCount() && raser_tick[0] + 2000 <= GetTickCount() && first_attack == 1) {
        //2~5�ʱ��� ����

        //������ ���
        for (int i = raserPos_x[0]; i <= raserPos_x[1]; i++) {
            DrawSprite(i, 0, 1, 38, "**************************************");
        }
        for (int i = raserPos_y[0]; i <= raserPos_y[1]; i++) {
            DrawSprite(0, i, 140, 1, "********************************************************************************************************************************************");
        }
        for (int i = 0; i < MAP_X_MAX * MAP_Y_MAX; i++) { //�� �����
            if (floorData[i] == '=') mapData[i] = '=';
        }

        //�ǰ� �������� Ȯ��
        if (raserPos_x[0] - 1 <= character.coord[0] && character.coord[0] <= raserPos_x[1] - 1
            || raserPos_y[0] <= character.coord[1] && character.coord[1] <= raserPos_y[1]) {

            if (character.tick[4] == 0) {
                character.time -= 1;// �ӽ�
                character.tick[4] = 25;//���� 1�ʸ� �ֱ�
            }

        }

    }
    if (raser_tick[0] + 5000 <= GetTickCount()) {//5�� ������ ��ų�� �� ����� �� �ֵ��� ���� �ʱ�ȭ
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
    snitch_next_x = character.coord[0];  //�� ��ǥ ����
    snitch_next_y = character.coord[1];

    snitch_accel_x = (snitch_next_x - objects[index]->coord[0]) / 32.0;
    snitch_accel_y = (snitch_next_y - objects[index]->coord[1]) / 32.0;
    objects[index]->motion[0] = 0;
}


void RemoveObject(int index) {//������Ʈ free
    free(objects[index]);
    objects[index] = 0;
}

void ControlBullet(int index) {//�Ѿ� ó��
    short at_coord[2] = { objects[index]->coord[0]/* - 5 + 8 * character.direction*/, objects[index]->coord[1] };//�Ѿ� ��ǥ
    short at_size[2] = { 1, 1 };//���� ������

    DrawSprite(objects[index]->coord[0], objects[index]->coord[1] + 1, 1, 1, "*");

    for (int enemy_index = 0; enemy_index < OBJECT_MAX; enemy_index++) {
        if (objects[enemy_index]) {
            if (objects[enemy_index]->kind > 99 && objects[enemy_index]->kind < 200) {
                bool check = CollisionCheck(objects[enemy_index]->coord, at_coord, objects[enemy_index]->size, at_size);
                if (check) {
                    objects[enemy_index]->tick[0] = tick;
                    objects[enemy_index]->time -= 20;
                    character.time += 20;//�߰���

                    if (!EnemyPositionX(objects[enemy_index]->coord[0], objects[enemy_index]->size[0]))//�Ѿ� �ǰݽ� �ڷ� �и�
                        objects[enemy_index]->accel[0] = -1.5;
                    else
                        objects[enemy_index]->accel[0] = 1.5;

                    RemoveObject(index);
                    MoveControl(objects[enemy_index]->coord, objects[enemy_index]->accel, objects[enemy_index]->size, &objects[enemy_index]->flyTime);//���� ������
                    return;
                }
            }

            if (objects[enemy_index]->kind == 203 && isVisible == TRUE) {
                bool check = CollisionCheck(objects[enemy_index]->coord, at_coord, objects[enemy_index]->size, at_size);
                if (check) {
                    objects[enemy_index]->tick[0] = tick;
                    objects[enemy_index]->time -= 100;
                    character.time += 100;//�߰���

                    RemoveObject(index);

                    //����ġ �ڷ���Ʈ
                    objects[enemy_index]->coord[0] = rand() % (MAP_X_MAX - objects[enemy_index]->size[0]) + objects[enemy_index]->size[0];
                    objects[enemy_index]->coord[1] = rand() % (BASE_Y - objects[enemy_index]->size[1]) + objects[enemy_index]->size[1];
                    objects[enemy_index]->tick[4] -= 5000;

                    return;
                }
            }

            /*
            if (objects[enemy_index]->kind == 202) { //����ġ�� �浹�� ���
                bool check = CollisionCheck(objects[enemy_index]->coord, at_coord, objects[enemy_index]->size, at_size);
                if (check) {
                    //����ġ �ڷ���Ʈ
                    objects[enemy_index]->coord[0] = rand() % (MAP_X_MAX - objects[enemy_index]->size[0]) + objects[enemy_index]->size[0];
                    objects[enemy_index]->coord[1] = rand() % (BASE_Y - objects[enemy_index]->size[1]) + objects[enemy_index]->size[1];
                    objects[enemy_index]->tick[4] -= 5000;

                    snitch_hit_count++;
                    objects[enemy_index]->tick[0] = tick;
                    character.time -= character.power;//�߰���
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

void ControlBomb(int index) {//��ź ó��
    short at_coord[2] = { objects[index]->coord[0]/* - 5 + 8 * character.direction*/, objects[index]->coord[1] };//�Ѿ� ��ǥ
    short at_size[2] = { 1, 1 };//���� ������

    DrawSprite(objects[index]->coord[0], objects[index]->coord[1] + 1, 1, 1, "@");

    for (int enemy_index = 0; enemy_index < OBJECT_MAX; enemy_index++) {
        if (objects[enemy_index]) {
            if (objects[enemy_index]->kind > 99 && objects[enemy_index]->kind < 200) {
                bool check = CollisionCheck(objects[enemy_index]->coord, at_coord, objects[enemy_index]->size, at_size);
                if (check) {
                    objects[enemy_index]->tick[0] = tick;
                    objects[enemy_index]->time -= character.power;
                    character.time += character.power;//�߰���

                    if (!EnemyPositionX(objects[enemy_index]->coord[0], objects[enemy_index]->size[0]))//�Ѿ� �ǰݽ� �ڷ� �и�
                        objects[enemy_index]->accel[0] = -1.5;
                    else
                        objects[enemy_index]->accel[0] = 1.5;

                    ControlSmog(index);
                    RemoveObject(index);
                    MoveControl(objects[enemy_index]->coord, objects[enemy_index]->accel, objects[enemy_index]->size, &objects[enemy_index]->flyTime);//���� ������
                    return;
                }
            }
            if (objects[enemy_index]->kind == 202) { //����ġ�� �浹�� ���
                bool check = CollisionCheck(objects[enemy_index]->coord, at_coord, objects[enemy_index]->size, at_size);
                if (check) {
                    objects[enemy_index]->coord[0] = rand() % (MAP_X_MAX - objects[enemy_index]->size[0]) + objects[enemy_index]->size[0];
                    objects[enemy_index]->coord[1] = rand() % (BASE_Y - objects[enemy_index]->size[1]) + objects[enemy_index]->size[1];
                    objects[enemy_index]->tick[4] -= 5000;

                    snitch_hit_count++;
                    objects[enemy_index]->tick[0] = tick;
                    character.time -= character.power;//�߰���

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
                    character.time += 100;//�߰���

                    RemoveObject(index);

                    //����ġ �ڷ���Ʈ
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

void ControlSmog(int index) {//��ź ó��
    objects[index]->coord[0] -= 4;
    objects[index]->coord[1] -= 1;
    objects[index]->size[0] = 10;
    objects[index]->size[1] = 3;
    objects[index]->tick[1] = GetTickCount();
    short at_coord[2] = { objects[index]->coord[0], objects[index]->coord[1] };//�Ѿ� ��ǥ
    short at_size[2] = { objects[index]->size[0],objects[index]->size[1] };//���� ������

    DrawSprite(at_coord[0], at_coord[1], at_size[0], at_size[1], " ;       ;        ; ;    ; ; ;");

    for (int enemy_index = 0; enemy_index < OBJECT_MAX; enemy_index++) {
        if (objects[enemy_index]) {
            if (objects[enemy_index]->kind > 99 && objects[enemy_index]->kind < 200) {
                bool check = CollisionCheck(objects[enemy_index]->coord, at_coord, objects[enemy_index]->size, at_size);
                if (check) {
                    objects[enemy_index]->tick[0] = tick;
                    objects[enemy_index]->time -= character.power;
                    character.time += character.power;//�߰���

                    if (!EnemyPositionX(objects[enemy_index]->coord[0], objects[enemy_index]->size[0]))//�Ѿ� �ǰݽ� �ڷ� �и�
                        objects[enemy_index]->accel[0] = -0.5;
                    else
                        objects[enemy_index]->accel[0] = 0.5;

                    if (GetTickCount() >= objects[index]->tick[1] + 1000) {
                        RemoveObject(index);
                        return;
                    }
                    MoveControl(objects[enemy_index]->coord, objects[enemy_index]->accel, objects[enemy_index]->size, &objects[enemy_index]->flyTime);//���� ������
                }
            }
        }
    }
}

void MoveControl(short coord[], float accel[], short size[], float* flyTime) {//�ε巯�� ������ ���� �Լ�
    float x_value = accel[0];//���ڷ� ���� x ���Ӹ�ŭ ��ġ�� ������

    /*�� �Ѱ� ����*/
    if (x_value != 0) {
        if (coord[0] + x_value < 1)
            x_value = 1 - coord[0];
        if (coord[0] + size[0] + x_value > MAP_X_MAX)
            x_value = MAP_X_MAX - size[0] - coord[0];
    }

    /*�ӵ��ݿ�*/
    coord[0] += floor(x_value + 0.5);

    /*����->0���� ���Ž�Ű��*/
    if (accel[0] > 0) accel[0] -= 0.2; if (accel[0] < 0) accel[0] += 0.2;

}

bool CollisionCheck(short coord1[], short coord2[], short size1[], short size2[]) {//2���� ��ü�� ũ�⸦ �ް� �浹�ϴ��� �˻��ϱ�
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

bool EnemyPositionX(short x, short size_x) {//���Ͱ� ĳ������ ��ġ�� ����
    if (character.coord[0] + 1 <= x + floor(size_x / 2 + 0.5))//pc ���� �����̸� TRUE����
        return TRUE;
    else
        return FALSE;//�����̸� FALSE����
}

bool EnemyPositionY(short y, short size_y) {//���Ͱ� ĳ������ ��ġ�� ����
    if (character.coord[1] + 1 >= y + floor(size_y / 2 + 0.5))//pv ���� �����̸� TRUE����
        return TRUE;
    else
        return FALSE;//�Ʒ����̸� FALSE����
}

void DrawBox(short x, short y, short size_x, short size_y) {//���� �׸���
    //EditMap(x, y, ''); EditMap(x + size_x - 1, y, '');
    //EditMap(x, y + size_y - 1, ''); EditMap(x + size_x - 1, y + size_y - 1, '');

    for (int i = 1; i < size_x - 1; i++) {
        EditMap(x + i, y, '-'); EditMap(x + i, y + size_y - 1, '-');
    }
    for (int i = 1; i < size_y - 1; i++) {
        EditMap(x, y + i, '|'); EditMap(x + size_x - 1, y + i, '|');
    }
}

void DrawNumber(short x, short y, int num) {//���� ���
    int tmp = num, len = NumLen(tmp), cnt = len;
    char* str = (char*)malloc(len);

    do {
        cnt--;
        str[cnt] = (char)(tmp % 10 + 48);
        tmp /= 10;
    } while (tmp != 0);

    DrawSprite(x, y, len, 1, str);
}

void DrawSprite(short x, short y, short size_x, short size_y, const char spr[]) {//���� ���
    for (int i = 0; i < size_y; i++) {
        for (int n = 0; n < size_x; n++)
            EditMap(x + n, y + i, spr[i * size_x + n]);
    }
}

void FillMap(char str[], char str_s, int max_value) {//Ư�� ���ڷ� �� ä���
    for (int i = 0; i < max_value; i++)
        str[i] = str_s;
}

void EditMap(short x, short y, char str) {//Ư�� ���ڷ� �� ����
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
    
    if (stage_num == 1) { // 1-> 2�������� ���� �ƾ�
        PlaySound(TEXT("win.wav"), NULL, SND_ASYNC | SND_SYNC);
        char print_temp1[1000];
        FILE* rfp1;
        rfp1 = fopen("stage2.txt", "rt");

        if (rfp1 == NULL) {
            printf("����");
            return;
        }

        while (fgets(print_temp1, 999, rfp1) != NULL) {
            printf(print_temp1);
        }
        puts("");
        fclose(rfp1);

        SetCurrentCursorPos(52, 23);
        printf("TIP : ������ ���ƿ��� ������ �����϶�!!!");


        Sleep(2000);
    }
    else if (stage_num == 2) {// 2-> 3�������� ���� �ƾ�
        PlaySound(TEXT("win.wav"), NULL, SND_ASYNC | SND_SYNC);
        char print_temp2[1000];
        FILE* rfp2;
        rfp2 = fopen("stage3.txt", "rt");

        if (rfp2 == NULL) {
            printf("����");
            return;
        }

        while (fgets(print_temp2, 999, rfp2) != NULL) {
            printf(print_temp2);
        }
        puts("");
        fclose(rfp2);

        SetCurrentCursorPos(48, 23);
        printf("TIP 1 : ������ ��Ʈ �������� �ִ� ���͸� ���� �϶�!!!");
        SetCurrentCursorPos(48, 25);
        printf("TIP 2 : �շ��ִ� ���ٴ����� ������ �ʰ� ���� �϶�!!!");

        Sleep(2000);
    }
    else if (stage_num == 3) {// 3-> ������������ ���� �ƾ�
        PlaySound(TEXT("win.wav"), NULL, SND_ASYNC | SND_SYNC);
        char print_temp3[1000];
        FILE* rfp3;
        rfp3 = fopen("bossstage.txt", "rt");

        if (rfp3 == NULL) {
            printf("����");
            return;
        }

        while (fgets(print_temp3, 999, rfp3) != NULL) {
            printf(print_temp3);
        }
        puts("");
        fclose(rfp3);

        SetCurrentCursorPos(36, 23);
        printf("TIP 1 : ������ ����� ���� 3���� \"������ �ð��� ����\"�� ���� �־��!!!");
        SetCurrentCursorPos(36, 25);
        printf("TIP 2 : ���������� \"������ �ð��� ����\"�� ���� ������ ������ �����϶�!!!");
        SetCurrentCursorPos(36, 27);
        printf("TIP 3 : \"������ �ð��� ����\"�� ���� ���� ������ ������ ������ �� ����!!!");

        Sleep(2000);
    }
    else if (stage_num == 4) {

        exit(0);
    }


    SetCurrentCursorPos(58, 38);
    printf("(S�� ������ ���� STAGE...)");

    while (1) {
        int flag = 0;
        for (int i = 0; i < 20; i++) {
            if (_kbhit() != 0) {

                int key = _getch();
                if (key == 115)flag = 1; //s������ ��ŵ
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
        printf("����");
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
        printf("����");
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
    printf("���丮 �� S Ű�� ������ ��ü ��ŵ�˴ϴ�.");
    Sleep(2000);
    system("cls");
    int cur_x = 58, cur_y = 18;

    if (GetKeyState(0x53) & 0x0001) return;
    SetCurrentCursorPos(cur_x + 8, cur_y);
    printf("2XXX ��..."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;
    SetCurrentCursorPos(cur_x, cur_y);
    printf("���� 1833415���� �����̴�..."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;
    SetCurrentCursorPos(cur_x + 4, cur_y);
    printf("������ �����ؾ� �ϳ�..."); Sleep(2500);
    if (GetKeyState(0x53) & 0x0001) return;

    system("cls");
    cur_y = 20;
    SetCurrentCursorPos(cur_x + 6, cur_y);
    printf("���� �װ� �ʹ�..."); Sleep(2500);
    if (GetKeyState(0x53) & 0x0001) return;

    system("cls");
    cur_y = 20;
    TextColor(DarkRed);
    SetCurrentCursorPos(cur_x + 2, cur_y);
    printf("\"���� �� ��̰� �غ��Ŷ�\""); Sleep(2500);

    system("cls");
    TextColor(WHITE);
    if (GetKeyState(0x53) & 0x0001) return;

    cur_y = 20;
    SetCurrentCursorPos(cur_x + 4, cur_y);
    printf("���������� ���� X��..."); Sleep(2500);
    if (GetKeyState(0x53) & 0x0001) return;

    system("cls");


    cur_y = 20;
    TextColor(DarkYellow);
    SetCurrentCursorPos(cur_x + 4, cur_y);
    printf("??? : �����ٱ�??"); Sleep(2500); cur_y += 2;
    TextColor(WHITE);
    if (GetKeyState(0x53) & 0x0001) return;

    system("cls");
    cur_y = 20;
    SetCurrentCursorPos(cur_x + 4, cur_y);
    printf("����...??"); Sleep(1700); cur_y += 2;
    SetCurrentCursorPos(cur_x + 4, cur_y);
    printf("������!?!?!?!?!"); Sleep(2500); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    system("cls");
    cur_y = 18;
    TextColor(DarkYellow);
    SetCurrentCursorPos(cur_x, cur_y);
    printf("??? : ������."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    SetCurrentCursorPos(cur_x, cur_y);
    printf("??? : ���� �ʸ� �����ַ��� ����ϱ�."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    SetCurrentCursorPos(cur_x, cur_y);
    printf("??? : �׷� �����δ� ���� �̱��� ����."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    SetCurrentCursorPos(cur_x, cur_y);
    printf("??? : ���� �� ��������."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    SetCurrentCursorPos(cur_x, cur_y);
    printf("??? : ���� ��� ���."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    char print_temp1[1000];
    FILE* rfp1;
    rfp1 = fopen("ascii-art (2).txt", "rt");

    if (rfp1 == NULL) {
        printf("����");
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
        printf("����");
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
    printf("��...��!!!"); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;
    SetCurrentCursorPos(cur_x, cur_y);
    printf("(�¸��� ���� ���� �����ε�...??)"); Sleep(2500); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;


    system("cls");
    TextColor(DarkYellow);
    cur_y = 18;
    SetCurrentCursorPos(cur_x - 8, cur_y);
    printf("??? : ��. ���� �ʴ� ������ �ð��� ���� �� �����ž�."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    SetCurrentCursorPos(cur_x - 8, cur_y);
    printf("??? : �׸���, ���� \"�ð��� ��\"�� �ٰ�."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    SetCurrentCursorPos(cur_x - 8, cur_y);
    printf("??? : � ���� \"������ �ð��� ����\"�� ���� ��."); Sleep(1700); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    SetCurrentCursorPos(cur_x - 8, cur_y);
    printf("??? : \"������ �ð�\"�� �����ļ� �̼����� ����."); Sleep(2500); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    system("cls");
    TextColor(WHITE);
    cur_y = 20;
    SetCurrentCursorPos(cur_x + 8, cur_y);
    printf("����..."); Sleep(2500); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    system("cls");
    TextColor(DarkYellow);
    cur_y = 20;
    SetCurrentCursorPos(cur_x + 6, cur_y);
    printf("??? : ������ ���..."); Sleep(2500); cur_y += 2;
    if (GetKeyState(0x53) & 0x0001) return;

    system("cls");
    cur_y = 20;
    TextColor(YELLOW);
    SetCurrentCursorPos(cur_x + 6, cur_y);
    printf("\"������ �� �ڽ��̿�.\""); Sleep(2500); cur_y += 2;

    TextColor(WHITE);

}

void GameIntro() {
    //SetConsole();
    PlaySound(TEXT("menu.wav"), NULL, SND_ASYNC | SND_SYNC);
    char print_temp[1000];
    FILE* rfp;
    rfp = fopen("ticktockintime.txt", "rt");

    if (rfp == NULL) {
        printf("����");
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
    int temp = 1; //1 ���ӽ��� 2 ���Ӽ��� 3 ������

    while (1)
    {
        SetCurrentCursorPos(110, 18);
        printf("���ӽ���");
        SetCurrentCursorPos(110, 22);
        printf("���Ӽ���");
        SetCurrentCursorPos(110, 26);
        printf("������");

        SetCurrentCursorPos(cur_x, cur_y);
        printf("��");

        if (_kbhit()) {
            key = _getch();
            if (key == 80) { //�Ʒ�Ű
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

            if (key == 72) { //��Ű
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

            if (key == 13 && temp == 1) { //���� ������ ���ӽ���
                Beep(530, 200);
                break;
            }
            else if (key == 13 && temp == 2) { //���� ������ ���� ����
                Beep(530, 200);
                GameManual();
                cur_y -= 4;
                temp = 1;

                char print_temp[1000];
                FILE* rfp;
                rfp = fopen("ticktockintime.txt", "rt");

                if (rfp == NULL) {
                    printf("����");
                    return;
                }
                SetCurrentCursorPos(50, 50);
                while (fgets(print_temp, 99, rfp) != NULL) {
                    printf(print_temp);
                }
                puts("");
                fclose(rfp);
            }
            else if (key == 13 && temp == 3) { //���� ������ ������
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

    //system("cls"); �ѹ��� ����� �� �׷��� ������ �� �̻�


    char print_temp1[1000];
    FILE* rfp1;
    rfp1 = fopen("gamemanual.txt", "rt");

    if (rfp1 == NULL) {
        printf("����");
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
    printf("1. ĳ������ HP�� TIME . ��, TIME�� 0�� �Ǹ� ĳ���ʹ� ���!");
    cur_y += 3;

    SetCurrentCursorPos(cur_x, cur_y);
    printf("2. ���͸� �����ϸ� ������ TIME�� ���� �� �ִ�!");
    cur_y += 3;

    SetCurrentCursorPos(cur_x, cur_y);
    printf("3. ���͸� óġ�ϸ� �����Ǵ� \"�ð��� ��\"�� ����Ͽ� �ð��� ������ Ÿ�� �ؾ� �Ѵ�!");
    cur_y += 3;


    SetCurrentCursorPos(cur_x, cur_y);
    printf("4. ������������ \"������ �ð��� ����\"�� ���� Ƚ�� �����Ͽ� ���� ���������� �Ѿ��! ");
    cur_y += 3;

    SetCurrentCursorPos(cur_x, cur_y);
    printf("5. 1,2,3 ���������� ������ ���� \"������ �ð�\"�� Ŭ���� �϶�!");
    cur_y += 3;


    TextColor(RED);
    SetCurrentCursorPos(cur_x, cur_y);
    printf("Tip : ���͸� ���� �����Ͽ� TIME�� ����� ��� ���� ���������� ���°� ���� ����..??!!");
    cur_y += 5;

    TextColor(WHITE);
    SetCurrentCursorPos(cur_x, cur_y);
    printf("(s ������ SKIP)");
    cur_y += 3;

    int flag;

    while (1) {
        flag = 0;
        for (int i = 0; i < 20; i++) {
            if (_kbhit() != 0) {

                key = _getch();
                if (key == 115)flag = 1; //s������ ��ŵ
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

    //system("cls"); //ȭ�� �����~

    //����Ű ����
    char print_temp2[1000];
    FILE* rfp2;
    rfp2 = fopen("gamemanual.txt", "rt");

    if (rfp2 == NULL) {
        printf("����");
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
    printf("1. ����Ű :  �� , �� , �� , ���");
    cur_y += 3;
    SetCurrentCursorPos(cur_x, cur_y);
    printf("2. ��ųŰ : Q , W ");
    cur_y += 3;
    SetCurrentCursorPos(cur_x, cur_y);
    printf("3. ����Ű : E");
    cur_y += 3;

    TextColor(RED);
    SetCurrentCursorPos(cur_x, cur_y);
    printf("Tip : ����Ű E�� �� ����Ͽ� �������� ��ų Ȱ���� �� �� �ִ�!");
    cur_y += 3;
    TextColor(WHITE);

    SetCurrentCursorPos(cur_x, cur_y);
    printf("4. �ð��� �� : R");
    cur_y += 3;

    TextColor(RED);
    SetCurrentCursorPos(cur_x, cur_y);
    printf("Tip : ���͸� óġ�Ͽ� ������ \"�ð��� ��\"�� \"������ �ð��� ����\"���� ��� ����!");
    cur_y += 3;
    TextColor(WHITE);

    SetCurrentCursorPos(cur_x, cur_y);
    printf("5. ������ ���Ű : T");
    cur_y += 3;
    TextColor(RED);
    SetCurrentCursorPos(cur_x, cur_y);
    printf("Tip : ���͸� ��ġ��� ���� Ȯ���� ������ �ڽ��� ���� �� �ִ�!");
    TextColor(WHITE);
    cur_y += 3;

    SetCurrentCursorPos(cur_x, cur_y);
    printf("(s ������ SKIP)");

    while (1) {
        flag = 0;
        for (int i = 0; i < 20; i++) {
            if (_kbhit() != 0) {

                key = _getch();
                if (key == 115)flag = 1; //s������ ��ŵ
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
        printf("����");
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
    printf("�������� ���� \"�ð��� ����\"�� ���� �� ������ óġ�Ͽ� ������ Ŭ���� �϶�!");
    TextColor(WHITE);

    cur_y += 5;
    SetCurrentCursorPos(cur_x, cur_y);
    printf("(s ������ ������)");

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



























