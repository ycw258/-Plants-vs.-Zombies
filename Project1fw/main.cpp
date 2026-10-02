#include<stdio.h>
#include <graphics.h>
#include<time.h>
#include  "tools.h"
#include<mmsystem.h>//音乐库
#include<math.h>
#include "vector2.h"
#pragma comment(lib,"winmm.lib")

#define width 900
#define height 600
#define ZM_MAX_ACTIVE 30//可调整：同时存在的僵尸上限
#define ZM_WAVE_COUNT 3//可调整：僵尸总波数
#define ZM_SPECIAL_COUNT 4//可调整：新增特殊僵尸种类数
#define ZM_MAX_STAND_FRAMES 16//站立动画最大帧数
#define ZM_MAX_WALK_FRAMES 23//行走动画最大帧数
#define ZM_MAX_EAT_FRAMES 21//吃植物动画最大帧数
#define CHOMPER_ATTACK_FRAMES 9//大嘴花吞食动画帧数
#define CHOMPER_ATTACK_HOLD 2//可调整：吞食动画每帧持续次数
#define CHOMPER_DIGEST_FRAMES 6//可调整：消化动画总帧数
#define CHOMPER_DIGEST_HOLD 6//可调整：消化动画每帧持续次数
#define CHOMPER_ATTACK_TIME (CHOMPER_ATTACK_FRAMES * CHOMPER_ATTACK_HOLD)
#define CHOMPER_EAT_TIME (CHOMPER_ATTACK_TIME + CHOMPER_DIGEST_FRAMES * CHOMPER_DIGEST_HOLD)
#define REPEATER_FRAMES 15//可调整：双发射手动画帧数
#define THREEPEATER_FRAMES 16//可调整：三发射手动画帧数
#define BULLET_MAX 120//可调整：屏幕内子弹最大数量
#define WALLNUT_NORMAL_FRAMES 16//坚果正常动画帧数
#define WALLNUT_CRACKED1_FRAMES 11//坚果损坏动画帧数
#define WALLNUT_CRACKED2_FRAMES 15//坚果严重损坏动画帧数
#define WALLNUT_CRACKED1_TIME 70//可调整：切换到轻度损坏的计时
#define WALLNUT_CRACKED2_TIME 140//可调整：切换到严重损坏的计时
#define WALLNUT_DEAD_TIME 210//可调整：坚果被吃完的计时

enum { wan_d, xiang_rg, da_zui_hua, wall_nut, repeater, threepeater, zhi_count };
enum { ZM_NORMAL, ZM_BUCKETHEAD, ZM_CONEHEAD, ZM_FLAG, ZM_SCREENDOOR };//僵尸类型
enum { ZM_STATE_STATIC, ZM_STATE_STAND, ZM_STATE_WALK, ZM_STATE_EAT };//僵尸动画状态

enum{going,win,fall};
IMAGE imgwin;
IMAGE imgfall;

int killcount;
int zmcount;
int gamestatus;

IMAGE imgBg;
IMAGE imgBar;
IMAGE imgCards[zhi_count];
IMAGE* imgzw[zhi_count][20];
IMAGE imgChomperAttack[CHOMPER_ATTACK_FRAMES];//大嘴花吞食动画
IMAGE imgChomperDigest[CHOMPER_DIGEST_FRAMES];//大嘴花消化动画
IMAGE imgWallNutNormal[WALLNUT_NORMAL_FRAMES];//坚果正常状态
IMAGE imgWallNutCracked1[WALLNUT_CRACKED1_FRAMES];//坚果损坏状态
IMAGE imgWallNutCracked2[WALLNUT_CRACKED2_FRAMES];//坚果严重损坏状态
IMAGE imgRepeater[REPEATER_FRAMES];//双发射手动画
IMAGE imgThreepeater[THREEPEATER_FRAMES];//三发射手动画

int curx, cury;
int curzw;
struct zw {
    int type;//zw类型
    int frameindex;//zd动画
    bool catched;//僵尸捕获
    int deadtimer;//死亡计数器

    int timer;
    int eatTimer;//大嘴花吞食计时
    bool eating;//大嘴花是否正在吞食
    int wallnutState;//坚果损坏状态
    int shotTimer;//双发射手第二发计时
    int x, y;
};
struct zw map[3][9];//植物种植

enum{SUNSHINE_down,SUNSHINE_ground,SUNSHINE__collect,SUNSHINE_repoduct};

struct sunshineball {
    int x, y;  //坐标
    int frameindex;  //图片帧序号
    int destY;//落点
    bool used;//是否使用
    int timer;
    float xoff;
    float yoff;

    float t;//曲线时间点
    vector2 p1, p2, p3, p4;
    vector2 pCur;//当前阳光位置
    float speed;
    int status;
};
struct sunshineball  balls[10];
IMAGE imgsunshineball[29];
int sunshine;

struct zm {//僵尸
    int x, y;
    int type;//僵尸类型
    int state;//动画状态
    int stateTimer;//状态切换计时
    int frameindex;
    bool used;
    int speed;
    int row;
    int blood;
    bool dead;
    bool eating;
};
struct zm zms[ZM_MAX_ACTIVE];
IMAGE imgZM[22];
IMAGE imgZMdead[20];
IMAGE imgZMeat[21];
IMAGE imgZMstand[11];
IMAGE imgZMSpecialStatic[ZM_SPECIAL_COUNT];//特殊僵尸静止图
IMAGE imgZMSpecialStand[ZM_SPECIAL_COUNT][ZM_MAX_STAND_FRAMES];//特殊僵尸站立动画
IMAGE imgZMSpecialWalk[ZM_SPECIAL_COUNT][ZM_MAX_WALK_FRAMES];//特殊僵尸行走动画
IMAGE imgZMSpecialEat[ZM_SPECIAL_COUNT][ZM_MAX_EAT_FRAMES];//特殊僵尸吃植物动画
const char* zmSpecialFolders[ZM_SPECIAL_COUNT] = {
    "BucketheadZombie", "ConeheadZombie", "FlagZombie", "ScreenDoorZombie"
};
const int zmSpecialStandFrames[ZM_SPECIAL_COUNT] = { 6, 8, 16, 8 };
const int zmSpecialWalkFrames[ZM_SPECIAL_COUNT] = { 15, 21, 12, 23 };
const int zmSpecialEatFrames[ZM_SPECIAL_COUNT] = { 11, 11, 11, 12 };
int waveTotal[ZM_WAVE_COUNT];//每波僵尸总数
int waveSpawned[ZM_WAVE_COUNT];//每波已生成数
int waveFlagSpawned[ZM_WAVE_COUNT];//每波是否已生成旗帜僵尸
int currentWave;//当前波次
int totalZombieCount;//全局僵尸总数
int zmSpawnTimer;//僵尸生成计时
int zmSpawnInterval;//僵尸生成间隔

struct bullet {
    int x, y;
    float fy;//子弹纵向浮点位置
    float vy;//子弹纵向速度
    bool used;
    int row;
    int speed;
    int blast;//是否发生爆炸
    int frameindex;
};
struct bullet bullets[BULLET_MAX];
IMAGE imgBulletnormal;
IMAGE imgBulletblast[4];




bool fileexist(const char* name) {//测试是否存在照片
    FILE* fp = fopen(name, "r");
    if (fp == NULL) {
        return false;
    }
    else {
        fclose(fp);
        return true;
    }

}


// 功能：统一加载植物动画帧，并兼容从输出目录运行。
void loadplantimage(IMAGE* img, const char* folder, const char* prefix, int index) {//加载植物动画帧
    char path[128];
    sprintf_s(path, sizeof(path), "res/Plants/%s/%s_%d.png", folder, prefix, index + 1);
    if (!fileexist(path)) {//从输出目录运行时回退到解决方案根目录
        sprintf_s(path, sizeof(path), "../../res/Plants/%s/%s_%d.png", folder, prefix, index + 1);
    }
    loadimage(img, path);
}

// 功能：加载特殊僵尸的静止、站立、行走和吃植物帧。
void loadzombieimage(IMAGE* img, const char* folder, const char* prefix, int index) {//加载僵尸动画帧
    char path[160];
    sprintf_s(path, sizeof(path), "res/Zombies/%s/frames/%s_%d.png", folder, prefix, index + 1);
    if (!fileexist(path)) {//从输出目录运行时回退到解决方案根目录
        sprintf_s(path, sizeof(path), "../../res/Zombies/%s/frames/%s_%d.png", folder, prefix, index + 1);
    }
    loadimage(img, path);
}

// 功能：初始化三波僵尸的数量、生成计时和总生成量。
void initzmwaves() {//初始化三波僵尸
    totalZombieCount = 0;
    currentWave = 0;
    zmSpawnTimer = 0;
    zmSpawnInterval = 0;
    for (int i = 0; i < ZM_WAVE_COUNT; i++) {
waveTotal[i] = 10 + rand() % 11;//可调整：每波 10~20 只
        waveSpawned[i] = 0;
        waveFlagSpawned[i] = 0;
        totalZombieCount += waveTotal[i];
    }
}

void gameInit() {//初始化
    loadimage(&imgBg, "res/bg.jpg");
    loadimage(&imgBar, "res/bar5.png");

    memset(imgzw, 0, sizeof(imgzw));
    memset(map, 0, sizeof(map));

    killcount = 0;
    zmcount = 0;
    gamestatus = going;

    char name[64];
    for (int i = 0; i < zhi_count; i++) {//植物卡牌
        if (i == wall_nut) {//坚果卡片单独使用对应素材
            loadimage(&imgCards[i], "res/Cards/card_wallnut.png");
        }
        else if (i == repeater) {
            loadimage(&imgCards[i], "res/Cards/card_repeaterpea.png");
        }
        else if (i == threepeater) {
            loadimage(&imgCards[i], "res/Cards/card_threepeashooter.png");
        }
        else {
            sprintf_s(name, sizeof(name), "res/Cards/card_%d.png", i + 1);
            loadimage(&imgCards[i], name);
        }
        for (int j = 0; j < 20; j++) {//植物动画帧
            sprintf_s(name, sizeof(name), "res/zhiwu/%d/%d.png", i, j + 1);
            if (fileexist(name)) {///33//
                imgzw[i][j] = new IMAGE;
                loadimage(imgzw[i][j], name);

            }
            else break;
        }
    }
    for (int i = 0; i < WALLNUT_NORMAL_FRAMES; i++) {//坚果正常动画帧
        loadplantimage(&imgWallNutNormal[i], "WallNut", "normal", i);
    }
    for (int i = 0; i < WALLNUT_CRACKED1_FRAMES; i++) {//坚果损坏动画帧
        loadplantimage(&imgWallNutCracked1[i], "WallNut", "cracked1", i);
    }
    for (int i = 0; i < WALLNUT_CRACKED2_FRAMES; i++) {//坚果严重损坏动画帧
        loadplantimage(&imgWallNutCracked2[i], "WallNut", "cracked2", i);
    }
    for (int i = 0; i < REPEATER_FRAMES; i++) {//双发射手动画帧
        loadplantimage(&imgRepeater[i], "Repeater", "repeater", i);
    }
    for (int i = 0; i < THREEPEATER_FRAMES; i++) {//三发射手动画帧
        loadplantimage(&imgThreepeater[i], "Threepeater", "threepeater", i);
    }
    for (int i = 0; i < CHOMPER_ATTACK_FRAMES; i++) {//大嘴花吞食动画帧
        loadplantimage(&imgChomperAttack[i], "Chomper", "attack", i);
    }
    for (int i = 0; i < CHOMPER_DIGEST_FRAMES; i++) {//大嘴花消化动画帧
        loadplantimage(&imgChomperDigest[i], "Chomper", "digest", i);
    }

    curzw = 0;
    sunshine = 150;

    memset(balls, 0, sizeof(balls));
    for (int i = 0; i < 29; i++) {//阳光动画帧
        sprintf_s(name, sizeof(name), "res/sunshine/%d.png", i + 1);
        loadimage(&imgsunshineball[i], name);
    }
    srand(time(NULL));//配置随机种子


    initgraph(width, height, 1);//游戏窗口

    LOGFONT f;//设置字体
    gettextstyle(&f);
    f.lfHeight = 30;
    f.lfWeight = 15;
    strcpy(f.lfFaceName, "Segoe UI Black");
    f.lfQuality = ANTIALIASED_QUALITY;//抗锯齿
    settextstyle(&f);
    setbkmode(TRANSPARENT);
    settextcolor(BLACK);

    memset(zms, 0, sizeof(zms));
    initzmwaves();//初始化僵尸波次//僵尸数据
    for (int i = 0; i < 22; i++) {
        sprintf_s(name, sizeof(name), "res/zm/%d.png", i + 1);
        loadimage(&imgZM[i], name);
    }

    loadimage(&imgBulletnormal, "res/bullets/bullet_normal.png");
    memset(bullets, 0, sizeof(bullets));

    loadimage(&imgBulletblast[3], "res/bullets/bullet_blast.png");
    for (int i = 0; i < 3; i++) {
        float k = (i + 1) * 0.2;
        loadimage(&imgBulletblast[i], "res/bullets/bullet_blast.png",
            imgBulletblast[3].getwidth() * k,
            imgBulletblast[3].getheight() * k, true);

    }

    for (int i = 0; i < 20; i++) {
        sprintf_s(name, sizeof(name), "res/zm_dead/%d.png", i + 1);
        loadimage(&imgZMdead[i], name);
    }

    for (int i = 0; i < 21; i++) {
        sprintf_s(name, "res/zm_eat/%d.png", i + 1);
        loadimage(&imgZMeat[i], name);
    }

    for (int i = 0; i < 11; i++) {
        sprintf_s(name, sizeof(name), "res/zm_stand/%d.png", i + 1);
        loadimage(&imgZMstand[i], name);
    }

    for (int type = 0; type < ZM_SPECIAL_COUNT; type++) {//加载四种特殊僵尸
        loadzombieimage(&imgZMSpecialStatic[type], zmSpecialFolders[type], "static", 0);
        for (int i = 0; i < zmSpecialStandFrames[type]; i++) {
            loadzombieimage(&imgZMSpecialStand[type][i], zmSpecialFolders[type], "stand", i);
        }
        for (int i = 0; i < zmSpecialWalkFrames[type]; i++) {
            loadzombieimage(&imgZMSpecialWalk[type][i], zmSpecialFolders[type], "walk", i);
        }
        for (int i = 0; i < zmSpecialEatFrames[type]; i++) {
            loadzombieimage(&imgZMSpecialEat[type][i], zmSpecialFolders[type], "eat", i);
        }
    }


}

// 功能：根据僵尸类型和动画状态返回当前帧数。
int getzmframecount(struct zm* z) {//获取当前僵尸动画帧数
    if (z->dead) {
        return 20;
    }
    if (z->type == ZM_NORMAL) {
        if (z->state == ZM_STATE_STATIC) return 1;
        if (z->state == ZM_STATE_STAND) return 11;
        if (z->state == ZM_STATE_EAT) return 21;
        return 22;
    }

    int index = z->type - 1;
    if (z->state == ZM_STATE_STATIC) return 1;
    if (z->state == ZM_STATE_STAND) return zmSpecialStandFrames[index];
    if (z->state == ZM_STATE_EAT) return zmSpecialEatFrames[index];
    return zmSpecialWalkFrames[index];
}

// 功能：为普通和四种特殊僵尸选择正确的动画帧。
IMAGE* getzmimage(struct zm* z) {//获取当前僵尸帧
    if (z->dead) {
        return &imgZMdead[z->frameindex];
    }
    if (z->type == ZM_NORMAL) {
        if (z->state == ZM_STATE_STATIC) return &imgZM[0];
        if (z->state == ZM_STATE_STAND) return &imgZMstand[z->frameindex];
        if (z->state == ZM_STATE_EAT) return &imgZMeat[z->frameindex];
        return &imgZM[z->frameindex];
    }

    int index = z->type - 1;
    if (z->state == ZM_STATE_STATIC) return &imgZMSpecialStatic[index];
    if (z->state == ZM_STATE_STAND) return &imgZMSpecialStand[index][z->frameindex];
    if (z->state == ZM_STATE_EAT) return &imgZMSpecialEat[index][z->frameindex];
    return &imgZMSpecialWalk[index][z->frameindex];
}

// 功能：按僵尸类型和状态绘制游戏中的僵尸。
void drawzm() {
    int zmcount = sizeof(zms) / sizeof(zms[0]);
    for (int i = 0; i < zmcount; i++) {
        if (zms[i].used) {
            IMAGE* img = getzmimage(&zms[i]);
            putimagePNG(zms[i].x, zms[i].y - img->getheight(), img);
        }
    }
}
void drawsunshine() {
    int ballmax = sizeof(balls) / sizeof(balls[0]);//阳光动画
    for (int i = 0; i < ballmax; i++) {
       // if (balls[i].used || balls[i].xoff) {
        if(balls[i].used){
            IMAGE* img = &imgsunshineball[balls[i].frameindex];
            //putimagePNG(balls[i].x, balls[i].y, img);
            putimagePNG(balls[i].pCur.x, balls[i].pCur.y, img);
        }
    }
    char scoretext[8];//阳光数量
    sprintf_s(scoretext, sizeof(scoretext), "%d", sunshine);
    outtextxy(278, 67, scoretext);
}

void drawCards() {
    for (int i = 0; i < zhi_count; i++) {//卡牌
        int x = 338 + i * 65;
        int y = 6;
        putimagePNG(x, y, &imgCards[i]);
    }
}

// 功能：统一计算植物图片宽度，供子弹出口和碰撞逻辑使用。
int getplantwidth(int type) {//获取植物图片宽度
    if (type == repeater + 1) return imgRepeater[0].getwidth();
    if (type == threepeater + 1) return imgThreepeater[0].getwidth();
    if (type == wall_nut + 1) return imgWallNutNormal[0].getwidth();
    return imgzw[type - 1][0]->getwidth();
}

// 功能：统一计算植物图片高度，用于草坪基线对齐。
int getplantheight(int type) {//获取植物图片高度
    if (type == repeater + 1) return imgRepeater[0].getheight();
    if (type == threepeater + 1) return imgThreepeater[0].getheight();
    if (type == wall_nut + 1) return imgWallNutNormal[0].getheight();
    return imgzw[type - 1][0]->getheight();
}

// 功能：获取拖拽植物时显示的首帧图片。
IMAGE* getdragzhiwuimage(int type) {//获取拖拽植物首帧
    if (type == wall_nut + 1) return &imgWallNutNormal[0];
    if (type == repeater + 1) return &imgRepeater[0];
    if (type == threepeater + 1) return &imgThreepeater[0];
    return imgzw[type - 1][0];
}

// 功能：根据植物类型和状态选择正确的动画帧。
IMAGE* getzhiwuimage(struct zw* zhiwu) {//获取当前植物帧
    if (zhiwu->type == repeater + 1) {
        return &imgRepeater[zhiwu->frameindex];
    }
    if (zhiwu->type == threepeater + 1) {
        return &imgThreepeater[zhiwu->frameindex];
    }
    if (zhiwu->type == wall_nut + 1) {//根据损坏状态选择坚果动画
        if (zhiwu->wallnutState == 2) {
            return &imgWallNutCracked2[zhiwu->frameindex];
        }
        if (zhiwu->wallnutState == 1) {
            return &imgWallNutCracked1[zhiwu->frameindex];
        }
        return &imgWallNutNormal[zhiwu->frameindex];
    }
    if (zhiwu->type == da_zui_hua + 1 && zhiwu->eating) {
        if (zhiwu->eatTimer <= CHOMPER_ATTACK_TIME) {
            return &imgChomperAttack[zhiwu->frameindex];
        }
        return &imgChomperDigest[zhiwu->frameindex];
    }
    return imgzw[zhiwu->type - 1][zhiwu->frameindex];
}

// 功能：绘制拖拽预览和地图上的所有植物。
void drawzhiwu() {
    if (curzw) {//拖动植物动画
        IMAGE* img = getdragzhiwuimage(curzw);
        putimagePNG(curx - img->getwidth() / 2, cury - img->getheight() / 2, img);
    }

    for (int i = 0; i < 3; i++) {//渲染植物
        for (int j = 0; j < 9; j++) {
            if (map[i][j].type > 0) {
                //int x = 256 + j * 81;
                //int y = 179 + i * 102 + 10;
                IMAGE* img = getzhiwuimage(&map[i][j]);
                putimagePNG(map[i][j].x, map[i][j].y, img);
            }
        }
    }

    if (curzw) {//渲染拖动植物卡牌
        IMAGE* img = getdragzhiwuimage(curzw);
        putimagePNG(curx - img->getwidth() / 2, cury - img->getheight() / 2, img);
    }
}

void drawbullet() {
    int bulletmax = sizeof(bullets) / sizeof(bullets[0]);
    for (int i = 0; i < bulletmax; i++) {
        if (bullets[i].used) {
            if (bullets[i].blast) {
                IMAGE* img = &imgBulletblast[bullets[i].frameindex];
                putimagePNG(bullets[i].x, bullets[i].y, img);
            }
            else {
                putimagePNG(bullets[i].x, bullets[i].y, &imgBulletnormal);
            }
        }
    }
}

void updatewindow() {
    BeginBatchDraw();


    putimagePNG(-112, 0, &imgBg);//背景
    putimagePNG(250, 0, &imgBar);

    drawCards();

    drawzhiwu();

    drawsunshine();

    drawzm();

    drawbullet();

 
    EndBatchDraw();
}

void collectsunshine(ExMessage* msg) {//阳光初始化
    int count = sizeof(balls) / sizeof(balls[0]);
    int w = imgsunshineball[0].getwidth();
    int h = imgsunshineball[0].getheight();
    for (int i = 0; i < count; i++) {
        if (balls[i].used) {//确定阳光位置
           // int x = balls[i].x;
           // int y = balls[i].y;
            int x = balls[i].pCur.x;
            int y = balls[i].pCur.y;
            if (msg->x > x && msg->x<x + w && msg->y>y && msg->y < y + h) {
                //balls[i].used = false;
                balls[i].status = SUNSHINE__collect;
                //sunshine += 25;
                
               // mciSendString("play res/sunshine.mp3",0,0,0);//阳光音乐
                PlaySound("res/sunshine.wav", NULL, SND_FILENAME | SND_ASYNC);
               
                //阳光的偏移量
              //  float destY = 0;
              // float destX = 262;
              // float angle = atan((y - destY) / (x - destX));
              //  balls[i].xoff = 4 * cos(angle);
              //balls[i].yoff = 4 * sin(angle);
                balls[i].p1 = balls[i].pCur;
                balls[i].p4 = vector2(262, 0);
                balls[i].t = 0;
                float distance = dis(balls[i].p1 - balls[i].p4);
                float off = 18;
                balls[i].speed = 1.0 / (distance / off);
                break;
            }
        }
    }
}

void useclick() {
    ExMessage msg;
    static int status = 0;
    peekmessage(&msg);
    if (msg.message == WM_LBUTTONDOWN) {//左键按下
        if (msg.x > 338 && msg.x < 338 + 65 * zhi_count && msg.y < 96) {
            int index = (msg.x - 338) / 65;
            //printf("%d\n", index);
            status = 1;
            curzw = index + 1;
        }
        else {
            collectsunshine(&msg);
        }
    }
    else if (msg.message == WM_MOUSEMOVE && status == 1) {
        curx = msg.x;
        cury = msg.y;


    }
    else if (msg.message == WM_LBUTTONUP && status == 1) {
        if (msg.x > 256 -112&& msg.y > 179 && msg.y < 489) {
            int row = (msg.y - 179) / 102;
            int col = (msg.x - 256+112) / 81;

            if (map[row][col].type == 0) {
                memset(&map[row][col], 0, sizeof(map[row][col]));//重置植物状态
                map[row][col].type = curzw;
                map[row][col].frameindex = 0;
                
                map[row][col].x = 256-112 + col* 81;
                map[row][col].y = 179 + row * 102 + 14;
                int baseHeight = imgzw[wan_d][0]->getheight();
                if (curzw == da_zui_hua + 1 || curzw == repeater + 1 ||
                    curzw == threepeater + 1) {//使图片底部对齐草坪基线
                    map[row][col].y -= getplantheight(curzw) - baseHeight;
                }
            }
        }
        //int row = (msg.y - 179) / 102;
        //int col = (msg.x - 256) / 81;
        //printf("%d %d\n", row, col);



        curzw = 0;
        status = 0;
    }


}

void createsunshine() {//阳光池
    static int count = 0;
    static int fre = 300;
    count++;
    if (count >= fre) {
        fre = 100 + rand() % 200;
        count = 0;

        int i;
        int ballmax = sizeof(balls) / sizeof(balls[0]);
        for (i = 0; i < ballmax && balls[i].used; i++);
        if (i >= ballmax)return;

        balls[i].used = true;
        balls[i].frameindex = 0;
        balls[i].timer = 0;


        balls[i].status = SUNSHINE_down;
        balls[i].t = 0;
        balls[i].p1 = vector2(260-112 + rand() % (900 - 260+112), 60);
        balls[i].p4 = vector2(balls[i].p1.x, 200 + (rand() % 4) * 90);
        int off = 2;
        float distance = balls[i].p4.y - balls[i].p1.y;
        balls[i].speed = 1.0 / (distance / off);
    }
     
    int ballmax = sizeof(balls) / sizeof(balls[0]);
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 9; j++) {
                if (map[i][j].type == xiang_rg + 1) {
                    map[i][j].timer++;
                    if (map[i][j].timer > 200) {
                        map[i][j].timer = 0;

                        int k;
                        for (k = 0; k < ballmax && balls[k].used; k++);
                        if (k >= ballmax)continue;

                            balls[k].used = true;
                            balls[k].p1 = vector2(map[i][j].x, map[i][j].y);
                            int w = (100 + rand() % 50)* (rand() % 2 ? 1 : -1);
                            balls[k].p4 = vector2(map[i][j].x + w,
                             map[i][j].y + imgzw[xiang_rg][0]->getheight()  -
                                imgsunshineball[0].getheight());
                            balls[k].p2 = vector2(balls[k].p1.x + w * 0.3, balls[k].p1.y - 100);
                            balls[k].p3 = vector2(balls[k].p1.x + w * 0.7, balls[k].p1.y - 100);
                            balls[k].status = SUNSHINE_repoduct;
                            balls[k].speed = 0.05;//20帧
                            balls[k].t = 0;

                        
                    
                }
            }
        }
    }
}

void updatesunshine() {
    int ballmax = sizeof(balls) / sizeof(balls[0]);
    for (int i = 0; i < ballmax; i++) {
        if (balls[i].used) {
            balls[i].frameindex = (balls[i].frameindex + 1) % 29;
            if (balls[i].status == SUNSHINE_down) {
                struct  sunshineball* sun = &balls[i];
                sun->t += sun->speed;
                sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1);
                if (sun->t >= 1) {
                    sun->status = SUNSHINE_ground;
                    sun->timer = 0;
                }
            }
            else if (balls[i].status == SUNSHINE_ground) {
                balls[i].timer++;
                if (balls[i].timer > 100) {
                    balls[i].used = false;
                    balls[i].timer = 0;
                }
            }
            else if (balls[i].status == SUNSHINE__collect) {
                struct  sunshineball* sun = &balls[i];
                sun->t += sun->speed;
                sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1);
                if (sun->t > 1) {
                    sun->used = false;
                    sunshine += 25;
                }
            }
            else if (balls[i].status == SUNSHINE_repoduct) {
                struct  sunshineball* sun = &balls[i];
                sun->t += sun->speed;
                sun->pCur = calcBezierPoint(sun->t, sun->p1, sun->p2, sun->p3, sun->p4);
                if (sun->t > 1) {
                    sun->status = SUNSHINE_ground;
                    sun->timer = 0;
                }

            }



            //balls[i].frameindex = (balls[i].frameindex + 1) % 29;
            //if (balls[i].timer == 0) {
            // balls[i].y += 2;
            //}

            //if (balls[i].y >= balls[i].destY) {
            // balls[i].timer++;
            //if (balls[i].timer > 100) {
            //  balls[i].used = false;
            //   }
           //}

      // else if (balls[i].xoff) {
       //    float destY = 0;
        //   float destX = 262;
          // float angle = atan((balls[i].y - destY) / (balls[i].x - destX));
      //     balls[i].xoff = 4 * cos(angle);
       //   balls[i].yoff = 4 * sin(angle);
        //   balls[i].x -= balls[i].xoff;
        //   balls[i].y -= balls[i].yoff;
        //   if (balls[i].y < 0 || balls[i].x < 262) {
         //      balls[i].xoff = 0;
        //       balls[i].yoff = 0;
           //    sunshine += 25;
         //  }
      // }
        }
    }
}

// 功能：按当前波次设定的权重随机选择僵尸类型。
int choosezombietype(int wave) {//随机选择僵尸类型
int specialChance = 30;//可调整：第一波特殊僵尸百分比
    if (wave == 1) {
specialChance = 55;//可调整：第二波特殊僵尸百分比
    }
    else if (wave >= 2) {
specialChance = 80;//可调整：最后一波特殊僵尸百分比
    }

    if (rand() % 100 < specialChance) {
        int specialTypes[3] = { ZM_BUCKETHEAD, ZM_CONEHEAD, ZM_SCREENDOOR };
        return specialTypes[rand() % 3];
    }
    return ZM_NORMAL;
}

// 功能：分配一个空闲槽位并初始化僵尸属性和生成位置。
bool spawnzombie(int type) {//生成一只指定类型僵尸
    int zmmax = sizeof(zms) / sizeof(zms[0]);
    int i;
    for (i = 0; i < zmmax && zms[i].used; i++);
    if (i >= zmmax) {
        return false;
    }

    memset(&zms[i], 0, sizeof(zms[i]));
    zms[i].used = true;
    zms[i].type = type;
    zms[i].state = ZM_STATE_STATIC;//先播放静止图片
zms[i].stateTimer = 10;//可调整：静止状态持续次数
    zms[i].row = rand() % 3;
    zms[i].y = 172 + (1 + zms[i].row) * 100;
zms[i].speed = 3;//可调整：僵尸移动速度
zms[i].blood = 150;//可调整：普通与旗帜僵尸血量
    if (type == ZM_BUCKETHEAD) {
zms[i].blood = 300;//可调整：铁桶僵尸血量
    }
    else if (type == ZM_CONEHEAD) {
zms[i].blood = 200;//可调整：路障僵尸血量
    }
    else if (type == ZM_SCREENDOOR) {
zms[i].blood = 250;//可调整：拿门僵尸血量
    }
    if (type == ZM_FLAG) {//旗帜僵尸生成在其他僵尸前面
zms[i].x = width - 160;//可调整：旗帜僵尸前置距离
    }
    else {
zms[i].x = width + 20 + rand() % 120;//可调整：普通僵尸在右侧的分散范围
    }
    zmcount++;
    return true;
}

// 功能：控制三波僵尸的生成节奏、旗帜僵尸位置和波次切换。
void createzm() {
    if (currentWave >= ZM_WAVE_COUNT) {
        return;
    }
    if (waveSpawned[currentWave] >= waveTotal[currentWave]) {//等待本波清完
        if (zmcount == 0) {
            currentWave++;
            if (currentWave < ZM_WAVE_COUNT) {
                zmSpawnTimer = 0;
                zmSpawnInterval = 0;
            }
        }
        return;
    }
    if (zmcount >= ZM_MAX_ACTIVE) {
        return;
    }

    zmSpawnTimer++;
    if (zmSpawnTimer < zmSpawnInterval) {
        return;
    }
    zmSpawnTimer = 0;
zmSpawnInterval = 40 + rand() % 60;//可调整：生成间隔

    int type;
    if (waveFlagSpawned[currentWave] == 0) {//每波先生成一只旗帜僵尸
        type = ZM_FLAG;
        waveFlagSpawned[currentWave] = 1;
    }
    else {
        type = choosezombietype(currentWave);
    }
    if (spawnzombie(type)) {
        waveSpawned[currentWave]++;
    }
}
// 功能：更新僵尸移动、动画状态、死亡和胜利条件。
void updatezm() {
    int zmmax = sizeof(zms) / sizeof(zms[0]);
    static int count = 0;

    count++;
    if (count > 2) {
        count = 0;
        for (int i = 0; i < zmmax; i++) {
            if (zms[i].used && !zms[i].dead && !zms[i].eating &&
                zms[i].state == ZM_STATE_WALK) {//只有行走状态才移动
                zms[i].x -= zms[i].speed;
                if (zms[i].x < 56) {
                    gamestatus = fall;
                }
            }
        }
    }

    for (int i = 0; i < zmmax; i++) {
        if (!zms[i].used) {
            continue;
        }
        if (zms[i].dead) {
            zms[i].frameindex++;
            if (zms[i].frameindex >= 20) {
                zms[i].used = false;
                if (zmcount > 0) {
                    zmcount--;
                }
                killcount++;
                if (killcount >= totalZombieCount) {
                    gamestatus = win;
                }
            }
        }
        else if (zms[i].eating) {
            zms[i].state = ZM_STATE_EAT;
            int frameCount = getzmframecount(&zms[i]);
            zms[i].frameindex = (zms[i].frameindex + 1) % frameCount;
        }
        else if (zms[i].state == ZM_STATE_STATIC) {
            zms[i].frameindex = 0;
            zms[i].stateTimer--;
            if (zms[i].stateTimer <= 0) {//进入开局站立状态
                zms[i].state = ZM_STATE_STAND;
zms[i].stateTimer = 30;//可调整：站立状态持续次数
                zms[i].frameindex = 0;
            }
        }
        else if (zms[i].state == ZM_STATE_STAND) {
            int frameCount = getzmframecount(&zms[i]);
            zms[i].frameindex = (zms[i].frameindex + 1) % frameCount;
            zms[i].stateTimer--;
            if (zms[i].stateTimer <= 0) {//站立结束后开始行走
                zms[i].state = ZM_STATE_WALK;
                zms[i].stateTimer = 0;
                zms[i].frameindex = 0;
            }
        }
        else {
            zms[i].state = ZM_STATE_WALK;
            int frameCount = getzmframecount(&zms[i]);
            zms[i].frameindex = (zms[i].frameindex + 1) % frameCount;
        }
    }
}
// 功能：创建一颗豌豆并按目标行计算斜向弹道。
bool firebullet(int sourceRow, int targetRow, int startX, int startY) {//发射一颗豌豆
    int bulletmax = sizeof(bullets) / sizeof(bullets[0]);
    int k;
    for (k = 0; k < bulletmax && bullets[k].used; k++);
    if (k >= bulletmax) {
        return false;
    }

    bullets[k].used = true;
    bullets[k].row = targetRow;
    bullets[k].speed = 4;//可调整：豌豆水平速度
    bullets[k].blast = false;
    bullets[k].frameindex = 0;
    bullets[k].x = startX;
    bullets[k].y = startY;
    bullets[k].fy = (float)startY;

int targetY = 179 + targetRow * 102 + 19;//可调整：目标行子弹中心高度
    float distance = (float)(width - startX);
    if (distance < 1.0f) {
        distance = 1.0f;
    }
    bullets[k].vy = (targetY - startY) / distance * bullets[k].speed;//根据角度计算纵向速度
    return true;
}

// 功能：根据植物位置和图片宽度计算子弹发射点。
int getzhiwumuzzleX(int row, int col) {//获取植物口部发射位置
    return 256 - 112 + col * 81 + getplantwidth(map[row][col].type) - 10;
}

// 功能：处理单发、双发延迟和三发三道的开火逻辑。
void shoot() {
    int lines[3] = { 0 };
    int zmmax = sizeof(zms) / sizeof(zms[0]);

    for (int i = 0; i < zmmax; i++) {
        if (zms[i].used && !zms[i].dead) {
            lines[zms[i].row] = 1;
        }
    }

    for (int i = 0; i < 3; i++) {//处理双发射手的第二发
        for (int j = 0; j < 9; j++) {
            if (map[i][j].type != repeater + 1 || map[i][j].shotTimer <= 0) {
                continue;
            }
            map[i][j].shotTimer--;
            if (map[i][j].shotTimer == 0) {
                firebullet(i, i, getzhiwumuzzleX(i, j), 179 + i * 102 + 19);
            }
        }
    }

    static int shootCount = 0;
    shootCount++;
    if (shootCount <= 30) {
        return;
    }
    shootCount = 0;

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 9; j++) {
            int type = map[i][j].type;
            if (type == 0) {
                continue;
            }
            int shotX = getzhiwumuzzleX(i, j);
            int shotY = 179 + i * 102 + 19;

            if (type == wan_d + 1 && lines[i]) {
                firebullet(i, i, shotX, shotY);
            }
            else if (type == repeater + 1 && lines[i]) {
                if (firebullet(i, i, shotX, shotY)) {//第一发
map[i][j].shotTimer = 4;//可调整：双发射手第二发延迟
                }
            }
            else if (type == threepeater + 1) {
                bool danger = false;
                for (int offset = -1; offset <= 1; offset++) {
                    int targetRow = i + offset;
                    if (targetRow >= 0 && targetRow < 3 && lines[targetRow]) {
                        danger = true;
                    }
                }
                if (danger) {//上、中、下三条弹道
                    for (int offset = -1; offset <= 1; offset++) {
                        firebullet(i, i + offset, shotX, shotY);
                    }
                }
            }
        }
    }
}
// 功能：更新豌豆的水平位置、斜向高度和爆炸动画。
void updatebullets() {
    int countmax = sizeof(bullets) / sizeof(bullets[0]);
    for (int i = 0; i < countmax; i++) {
        if (bullets[i].used) {
            bullets[i].x += bullets[i].speed;
            bullets[i].fy += bullets[i].vy;
            bullets[i].y = (int)(bullets[i].fy + 0.5f);
            if (bullets[i].x > width) {
                bullets[i].used = false;
            }
            if (bullets[i].blast) {
                bullets[i].frameindex++;
                if (bullets[i].frameindex >= 4) {
                    bullets[i].used = false;
                }
            }
        }
    }
}
// 功能：检测豌豆与僵尸的碰撞并扣除僵尸血量。
void checkbulletZM() {
    int bcount = sizeof(bullets) / sizeof(bullets[0]);
    int zcount = sizeof(zms) / sizeof(zms[0]);
    for (int i = 0; i < bcount; i++) {
        if (bullets[i].used == false || bullets[i].blast)continue;
        for (int k = 0; k < zcount; k++) {
            if (zms[k].used == false)continue;
            int x1 = zms[k].x + 80;
            int x2 = zms[k].x + 110;
            int x = bullets[i].x;

            if (zms[k].dead == false &&
                bullets[i].row == zms[k].row && x > x1 && x < x2) {
                zms[k].blood -= 10;
                bullets[i].blast = true;
                bullets[i].speed = 0;
                bullets[i].vy = 0;

                if (zms[k].blood <= 0) {
                    zms[k].dead = true;
                    zms[k].speed = 0;
                    zms[k].frameindex = 0;
                }
                break;
            }
        }
    }
}

// 功能：判断僵尸是否进入大嘴花嘴部，并启动直接吞食流程。
bool chompZM(int row, int col, int zmindex) {//大嘴花吞食僵尸
    int mouthX1 = map[row][col].x + 45;
    int mouthX2 = map[row][col].x + imgzw[da_zui_hua][0]->getwidth() + 25;
    int zombieX = zms[zmindex].x + 80;

    if (map[row][col].eating ||
        zombieX <= mouthX1 || zombieX >= mouthX2) {
        return false;
    }

    map[row][col].eating = true;//进入吞食状态，期间不重复吞食
    map[row][col].eatTimer = 0;
    map[row][col].frameindex = 0;

    zms[zmindex].used = false;//僵尸被直接消除，不播放死亡动画
    if (zmcount > 0) {
        zmcount--;
    }
    killcount++;
    if (killcount >= totalZombieCount) {
        gamestatus = win;
    }
    return true;
}

void checkZM2zhiwu() {
    int zcount = sizeof(zms) / sizeof(zms[0]);
    for (int i = 0; i < zcount; i++) {
        if (zms[i].used == false || zms[i].dead)continue;

        int row = zms[i].row;
        for (int k = 0; k < 9; k++) {
            if (map[row][k].type == 0)continue;

            if (map[row][k].type == da_zui_hua + 1) {//大嘴花优先吞食前方僵尸
                chompZM(row, k, i);
                continue;
            }

            int zhiwuX = 256-112 + k * 81;
            int x1 = zhiwuX + 10;
            int x2 = zhiwuX + 60;
            int x3 = zms[i].x + 80;
            if (x3 > x1 && x3 < x2) {
                if (map[row][k].catched) {
                    //zms[i].frameindex++;
                    map[row][k].deadtimer++;
                    int destroyTime = 70;//普通植物的耐久时间
                    if (map[row][k].type == wall_nut + 1) {//坚果按三个状态增加耐久
                        destroyTime = WALLNUT_DEAD_TIME;
                    }
                   if(map[row][k].deadtimer>destroyTime){
                        map[row][k].deadtimer = 0;
                        map[row][k].type = 0;
                        zms[i].eating = false;
                        zms[i].state = ZM_STATE_WALK;//植物被吃完后恢复行走
                        zms[i].stateTimer = 0;
                        zms[i].frameindex = 0;
zms[i].speed = 3;//可调整：僵尸移动速度
                    }
                }
                else {
                    map[row][k].catched = true;
                    map[row][k].deadtimer = 0;
                    zms[i].eating = true;
                    zms[i].state = ZM_STATE_EAT;//切换到吃植物动画
                    zms[i].speed = 0;
                    zms[i].frameindex = 0;
                }
            }

                

        }
    }
}

void collisioncheck() {
    checkbulletZM();//子弹和僵尸
    checkZM2zhiwu();//植物和僵尸
}

// 功能：更新大嘴花、坚果、双发射手和三发射手的动画状态。
void updatezhiwu() {
    static int count = 0;
    if (++count < 2)return;
    count = 0;

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 9; j++) {
            if (map[i][j].type > 0) {
                if (map[i][j].type == wall_nut + 1) {//坚果三种状态动画
                    int state = 0;
                    if (map[i][j].deadtimer > WALLNUT_CRACKED2_TIME) {
                        state = 2;
                    }
                    else if (map[i][j].deadtimer > WALLNUT_CRACKED1_TIME) {
                        state = 1;
                    }
                    if (state != map[i][j].wallnutState) {//切换状态时从头播放
                        map[i][j].wallnutState = state;
                        map[i][j].frameindex = 0;
                    }
                    int frameCount = WALLNUT_NORMAL_FRAMES;
                    if (state == 1) {
                        frameCount = WALLNUT_CRACKED1_FRAMES;
                    }
                    else if (state == 2) {
                        frameCount = WALLNUT_CRACKED2_FRAMES;
                    }
                    map[i][j].frameindex =
                        (map[i][j].frameindex + 1) % frameCount;
                }
                else if (map[i][j].type == da_zui_hua + 1 &&
                    map[i][j].eating) {//大嘴花吞食与消化动画
                    map[i][j].eatTimer++;
                    if (map[i][j].eatTimer <= CHOMPER_ATTACK_TIME) {
                        map[i][j].frameindex =
                            (map[i][j].eatTimer - 1) / CHOMPER_ATTACK_HOLD;
                    }
                    else if (map[i][j].eatTimer <= CHOMPER_EAT_TIME) {
                        map[i][j].frameindex =
                            (map[i][j].eatTimer - CHOMPER_ATTACK_TIME - 1) /
                            CHOMPER_DIGEST_HOLD;
                    }
                    else {//消化结束，恢复普通动画
                        map[i][j].eating = false;
                        map[i][j].eatTimer = 0;
                        map[i][j].frameindex = 0;
                    }
                }
                else if (map[i][j].type == repeater + 1) {//双发射手动画
                    map[i][j].frameindex = (map[i][j].frameindex + 1) % REPEATER_FRAMES;
                }
                else if (map[i][j].type == threepeater + 1) {//三发射手动画
                    map[i][j].frameindex = (map[i][j].frameindex + 1) % THREEPEATER_FRAMES;
                }
                else {
                    map[i][j].frameindex++;
                    int zwtype = map[i][j].type - 1;
                    int index = map[i][j].frameindex;
                    if (imgzw[zwtype][index] == NULL) {
                        map[i][j].frameindex = 0;
                    }
                }
            }
        }
    }
}

void updategame() {
    static int count = 0;

    updatezhiwu();
    createsunshine();
    updatesunshine();

    createzm();
    updatezm();

    shoot();
    updatebullets();

    collisioncheck();//碰撞检测
}

void startUI() {//ui界面
    IMAGE imgBg, imgMenu1, imgMenu2;
    loadimage(&imgBg, "res/menu.png");
    loadimage(&imgMenu1, "res/menu1.png");
    loadimage(&imgMenu2, "res/menu2.png");
    int flag = 0;

    while (1) {
        BeginBatchDraw();
        putimagePNG(0, 0, &imgBg);
        putimagePNG(474, 75, flag ? &imgMenu2 : &imgMenu1);

        ExMessage msg;
        if (peekmessage(&msg)) {
            if (msg.message == WM_LBUTTONDOWN
                && msg.x > 474 && msg.x < 474 + 331
                && msg.y>75 && msg.y < 75 + 140) {
                flag = 1;
           
            }
            else if (msg.message == WM_LBUTTONUP) {
                if (flag == 1) {
                    EndBatchDraw();
                    break;


                }
            }
        }
        EndBatchDraw();
    }



}

// 功能：播放开场镜头移动，并混合显示所有僵尸种类的站立动画。
void viewScence() {
    int xmin = width - imgBg.getwidth();
    vector2 points[9] = {
        {550,80},{530,160},{630,170},{530,200},{515,270},{565,370},{605,310},{705,280},{690,340} };
    struct zm sceneZms[9];
    for (int i = 0; i < 9; i++) {//开场远景混合显示所有僵尸种类
        memset(&sceneZms[i], 0, sizeof(sceneZms[i]));
        sceneZms[i].type = i % 5;
        sceneZms[i].state = ZM_STATE_STAND;
        sceneZms[i].frameindex = rand() % getzmframecount(&sceneZms[i]);
    }

    int count = 0;

    for (int x = 0; x >= xmin; x -= 2) {
        BeginBatchDraw();
        putimage(x, 0, &imgBg);
        count++;
        for (int k = 0; k < 9; k++) {
            IMAGE* img = getzmimage(&sceneZms[k]);
            putimagePNG(points[k].x - xmin + x, points[k].y, img);
            if (count >= 10) {
                sceneZms[k].frameindex =
                    (sceneZms[k].frameindex + 1) % getzmframecount(&sceneZms[k]);
            }
        }
        if (count >= 10) count = 0;
        EndBatchDraw();
        Sleep(5);
    }
    for (int i = 0; i < 100; i++) {
        BeginBatchDraw();
        putimage(xmin, 0, &imgBg);
        for (int k = 0; k < 9; k++) {
            IMAGE* img = getzmimage(&sceneZms[k]);
            putimagePNG(points[k].x, points[k].y, img);
            sceneZms[k].frameindex =
                (sceneZms[k].frameindex + 1) % getzmframecount(&sceneZms[k]);
        }
        EndBatchDraw();
        Sleep(30);
    }
    for (int x = xmin; x <= -112; x += 2) {
        BeginBatchDraw();
        putimage(x, 0, &imgBg);
        count++;
        for (int k = 0; k < 9; k++) {
            IMAGE* img = getzmimage(&sceneZms[k]);
            putimagePNG(points[k].x - xmin + x, points[k].y, img);
            if (count >= 10) {
                sceneZms[k].frameindex =
                    (sceneZms[k].frameindex + 1) % getzmframecount(&sceneZms[k]);
            }
        }
        if (count >= 10) count = 0;
        EndBatchDraw();
    }
}
// 功能：播放植物卡片栏从上方缓慢下降的过场动画。
void bardown() {
    int heigh = imgBar.getheight();
    for (int y = -heigh;y <= 0; y++) {
        BeginBatchDraw();

        putimagePNG(-112, 0, &imgBg);//保持背景，避免左侧出现空植物栏
        putimagePNG(250, y, &imgBar);//植物栏从上方缓缓下降

        for (int i = 0; i < zhi_count; i++) {
            int x = 338 + i * 65;
       
            putimage(x, y+6, &imgCards[i]);

        }

        EndBatchDraw();
        Sleep(10);
    }
}

bool checkover() {
    bool ret = false;
    if (gamestatus == win) {
   
        loadimage(&imgwin, "res/win2.png");
        cleardevice();
        putimagePNG(0, 0, &imgwin);
        Sleep(2000);
        ret = true;

    }
    else if (gamestatus == fall) {
        loadimage(&imgfall,"res/fail2.png");
        cleardevice();
        putimagePNG(0, 0, &imgfall);
        Sleep(2000);
        ret = true;
    }
    return ret;
}


int main(void) {
    gameInit();

    startUI();

    viewScence();



    bardown();

    int timer = 0;
    bool flag = true;
    while (1) {
        useclick();
        timer += getDelay();
        if (timer > 30) {
            flag = true;
            timer = 0;
        }
        if (flag) {
            flag = false;
            updatewindow();
            updategame();
            if (checkover())break;
        }


    }

    system("pause");
    return 0;
}