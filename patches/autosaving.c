#include "patches.h"
#include "play_patches.h"
#include "z64save.h"
#include "z64horse.h"
#include "overlays/gamestates/ovl_file_choose/z_file_select.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"
#include "overlays/actors/ovl_Obj_Warpstone/z_obj_warpstone.h"
#include "misc_funcs.h"

#define SAVE_TYPE_AUTOSAVE 2

u8 gCanPause;
s32 ShrinkWindow_Letterbox_GetSizeTarget(void);
void ShrinkWindow_Letterbox_SetSizeTarget(s32 target);

// @recomp Patched function to set a global variable if the player can pause
void KaleidoSetup_Update(PlayState* play) {
    Input* input = CONTROLLER1(&play->state);
    MessageContext* msgCtx = &play->msgCtx;
    Player* player = GET_PLAYER(play);
    PauseContext* pauseCtx = &play->pauseCtx;

    if (CHECK_BTN_ALL(input->cur.button, BTN_R)) {
        if (msgCtx && msgCtx) {}
    }

    if ((pauseCtx->state == PAUSE_STATE_OFF) && (pauseCtx->debugEditor == DEBUG_EDITOR_NONE) &&
        (play->gameOverCtx.state == GAMEOVER_INACTIVE)) {
        if ((play->transitionTrigger == TRANS_TRIGGER_OFF) && (play->transitionMode == TRANS_MODE_OFF)) {
            if ((gSaveContext.save.cutsceneIndex < 0xFFF0) && (gSaveContext.nextCutsceneIndex < 0xFFF0)) {
                if (!Play_InCsMode(play) || ((msgCtx->msgMode != MSGMODE_NONE) && (msgCtx->currentTextId == 0xFF))) {
                    if ((play->bButtonAmmoPlusOne < 2) && (gSaveContext.magicState != MAGIC_STATE_STEP_CAPACITY) &&
                        (gSaveContext.magicState != MAGIC_STATE_FILL)) {
                        if (!CHECK_EVENTINF(EVENTINF_17) && !(player->stateFlags1 & PLAYER_STATE1_20)) {
                            if (!(play->actorCtx.flags & ACTORCTX_FLAG_1) &&
                                !(play->actorCtx.flags & ACTORCTX_FLAG_PICTO_BOX_ON)) {
                                if (!play->actorCtx.unk268 == 0) {
                                    if (CHECK_BTN_ALL(input->press.button, BTN_START)) {
                                        gSaveContext.prevHudVisibility = gSaveContext.hudVisibility;
                                        pauseCtx->itemDescriptionOn = false;
                                        pauseCtx->state = PAUSE_STATE_OPENING_0;
                                        func_800F4A10(play);
                                        // Set next page mode to scroll left
                                        pauseCtx->nextPageMode = pauseCtx->pageIndex * 2 + 1;
                                        Audio_SetPauseState(true);
                                    }
                                    // @recomp Create a variable to check if the player can pause or not, which is used to determine when to autosave
                                    else {
                                        gCanPause = true;
                                    }
                                }
                                if (pauseCtx->state == PAUSE_STATE_OPENING_0) {
                                    GameState_SetFramerateDivisor(&play->state, 2);
                                    if (ShrinkWindow_Letterbox_GetSizeTarget() != 0) {
                                        ShrinkWindow_Letterbox_SetSizeTarget(0);
                                    }
                                    Audio_PlaySfx_PauseMenuOpenOrClose(SFX_PAUSE_MENU_OPEN);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void Sram_SyncWriteToFlash(SramContext* sramCtx, s32 curPage, s32 numPages);

void autosave_reset_timer();
void autosave_reset_timer_slow();

void do_autosave(SramContext* sramCtx) {
    s32 fileNum = gSaveContext.fileNum;

    gSaveContext.save.isOwlSave = SAVE_TYPE_AUTOSAVE;

    gSaveContext.save.saveInfo.checksum = 0;
    gSaveContext.save.saveInfo.checksum = Sram_CalcChecksum(&gSaveContext, offsetof(SaveContext, fileNum));

    // Copy the saved parts of the global save context into the sram saving buffer.
    Lib_MemCpy(sramCtx->saveBuf, &gSaveContext, offsetof(SaveContext, fileNum));
    // Synchronously save into the owl save slot and the backup owl save slot.
    Sram_SyncWriteToFlash(sramCtx, gFlashOwlSaveStartPages[fileNum * 2], gFlashOwlSaveNumPages[fileNum * 2]);
    Sram_SyncWriteToFlash(sramCtx, gFlashOwlSaveStartPages[fileNum * 2 + 1], gFlashOwlSaveNumPages[fileNum * 2 + 1]);

    gSaveContext.save.isOwlSave = false;
}

// @recomp Do not clear the save if the save was an autosave.
void func_80147314(SramContext* sramCtx, s32 fileNum) {
    s32 save_type = gSaveContext.save.isOwlSave;
    gSaveContext.save.isOwlSave = false;

    // @recomp Prevent owl save/autosave deletion if autosaving is enabled.
    if (!recomp_autosave_enabled()) {
        gSaveContext.save.saveInfo.playerData.newf[0] = '\0';
        gSaveContext.save.saveInfo.playerData.newf[1] = '\0';
        gSaveContext.save.saveInfo.playerData.newf[2] = '\0';
        gSaveContext.save.saveInfo.playerData.newf[3] = '\0';
        gSaveContext.save.saveInfo.playerData.newf[4] = '\0';
        gSaveContext.save.saveInfo.playerData.newf[5] = '\0';

        gSaveContext.save.saveInfo.checksum = 0;
        gSaveContext.save.saveInfo.checksum = Sram_CalcChecksum(&gSaveContext, offsetof(SaveContext, fileNum));

        Lib_MemCpy(sramCtx->saveBuf, &gSaveContext, offsetof(SaveContext, fileNum));
        Sram_SyncWriteToFlash(sramCtx, gFlashOwlSaveStartPages[fileNum * 2], gFlashOwlSaveNumPages[fileNum * 2]);
        //! Note: should be `gFlashOwlSaveNumPages[fileNum * 2 + 1]`?
        Sram_SyncWriteToFlash(sramCtx, gFlashOwlSaveStartPages[fileNum * 2 + 1], gFlashOwlSaveNumPages[fileNum * 2]);

        gSaveContext.save.isOwlSave = true;

        gSaveContext.save.saveInfo.playerData.newf[0] = 'Z';
        gSaveContext.save.saveInfo.playerData.newf[1] = 'E';
        gSaveContext.save.saveInfo.playerData.newf[2] = 'L';
        gSaveContext.save.saveInfo.playerData.newf[3] = 'D';
        gSaveContext.save.saveInfo.playerData.newf[4] = 'A';
        gSaveContext.save.saveInfo.playerData.newf[5] = '3';
    }
}

void delete_owl_save(SramContext* sramCtx, s32 fileNum) {
    gSaveContext.save.saveInfo.playerData.newf[0] = '\0';
    gSaveContext.save.saveInfo.playerData.newf[1] = '\0';
    gSaveContext.save.saveInfo.playerData.newf[2] = '\0';
    gSaveContext.save.saveInfo.playerData.newf[3] = '\0';
    gSaveContext.save.saveInfo.playerData.newf[4] = '\0';
    gSaveContext.save.saveInfo.playerData.newf[5] = '\0';

    gSaveContext.save.saveInfo.checksum = 0;
    gSaveContext.save.saveInfo.checksum = Sram_CalcChecksum(&gSaveContext, offsetof(SaveContext, fileNum));

    Lib_MemCpy(sramCtx->saveBuf, &gSaveContext, offsetof(SaveContext, fileNum));
    Sram_SyncWriteToFlash(sramCtx, gFlashOwlSaveStartPages[fileNum * 2], gFlashOwlSaveNumPages[fileNum * 2]);
    Sram_SyncWriteToFlash(sramCtx, gFlashOwlSaveStartPages[fileNum * 2 + 1], gFlashOwlSaveNumPages[fileNum * 2 + 1]);

    gSaveContext.save.saveInfo.playerData.newf[0] = 'Z';
    gSaveContext.save.saveInfo.playerData.newf[1] = 'E';
    gSaveContext.save.saveInfo.playerData.newf[2] = 'L';
    gSaveContext.save.saveInfo.playerData.newf[3] = 'D';
    gSaveContext.save.saveInfo.playerData.newf[4] = 'A';
    gSaveContext.save.saveInfo.playerData.newf[5] = '3';
}

// @recomp Patched to delete owl saves when making regular saves.
void func_8014546C(SramContext* sramCtx) {
    s32 i;

    if (gSaveContext.save.isOwlSave) {
        for (i = 0; i < ARRAY_COUNT(gSaveContext.cycleSceneFlags); i++) {
            gSaveContext.save.saveInfo.permanentSceneFlags[i].chest = gSaveContext.cycleSceneFlags[i].chest;
            gSaveContext.save.saveInfo.permanentSceneFlags[i].switch0 = gSaveContext.cycleSceneFlags[i].switch0;
            gSaveContext.save.saveInfo.permanentSceneFlags[i].switch1 = gSaveContext.cycleSceneFlags[i].switch1;
            gSaveContext.save.saveInfo.permanentSceneFlags[i].clearedRoom = gSaveContext.cycleSceneFlags[i].clearedRoom;
            gSaveContext.save.saveInfo.permanentSceneFlags[i].collectible = gSaveContext.cycleSceneFlags[i].collectible;
        }

        gSaveContext.save.saveInfo.checksum = 0;
        gSaveContext.save.saveInfo.checksum = Sram_CalcChecksum(&gSaveContext, offsetof(SaveContext, fileNum));

        Lib_MemCpy(sramCtx->saveBuf, &gSaveContext, offsetof(SaveContext, fileNum));
    } else {
        // @recomp Delete the owl save.
        delete_owl_save(sramCtx, gSaveContext.fileNum);
        // @recomp Reset the autosave timer.
        autosave_reset_timer();
        for (i = 0; i < ARRAY_COUNT(gSaveContext.cycleSceneFlags); i++) {
            gSaveContext.save.saveInfo.permanentSceneFlags[i].chest = gSaveContext.cycleSceneFlags[i].chest;
            gSaveContext.save.saveInfo.permanentSceneFlags[i].switch0 = gSaveContext.cycleSceneFlags[i].switch0;
            gSaveContext.save.saveInfo.permanentSceneFlags[i].switch1 = gSaveContext.cycleSceneFlags[i].switch1;
            gSaveContext.save.saveInfo.permanentSceneFlags[i].clearedRoom = gSaveContext.cycleSceneFlags[i].clearedRoom;
            gSaveContext.save.saveInfo.permanentSceneFlags[i].collectible = gSaveContext.cycleSceneFlags[i].collectible;
        }

        gSaveContext.save.saveInfo.checksum = 0;
        gSaveContext.save.saveInfo.checksum = Sram_CalcChecksum(&gSaveContext.save, sizeof(Save));

        if (gSaveContext.flashSaveAvailable) {
            Lib_MemCpy(sramCtx->saveBuf, &gSaveContext, sizeof(Save));
            Lib_MemCpy(&sramCtx->saveBuf[0x2000], &gSaveContext.save, sizeof(Save));
        }
    }
}

extern u16 D_801F6AF0;
extern u8 D_801F6AF2;

// @recomp Patched to call the new owl save deletion function.
void Sram_EraseSave(FileSelectState* fileSelect2, SramContext* sramCtx, s32 fileNum) {
    FileSelectState* fileSelect = fileSelect2;
    s32 pad;

    if (gSaveContext.flashSaveAvailable) {
        if (fileSelect->isOwlSave[fileNum + 2]) {
            // @recomp Call the new owl save deletion function.
            delete_owl_save(sramCtx, fileNum);
            fileSelect->isOwlSave[fileNum + 2] = false;
        }
        bzero(sramCtx->saveBuf, SAVE_BUFFER_SIZE);
        Lib_MemCpy(&gSaveContext, sramCtx->saveBuf, sizeof(Save));
    }

    gSaveContext.save.time = D_801F6AF0;
    gSaveContext.flashSaveAvailable = D_801F6AF2;
}

SaveContext prev_save_ctx;
int bcmp_recomp(void* __s1, void* __s2, int __n);

#define SAVE_COMPARE_MEMBER(ctx1, ctx2, member) \
    if ((ctx1)->member != ((ctx2)->member)) { \
        return true; \
    }

#define SAVE_COMPARE_MEMBER_ARRAY(ctx1, ctx2, member) \
    if (bcmp_recomp(&(ctx1)->member, &(ctx2)->member, sizeof((ctx1)->member))) { \
        recomp_printf(#member " differed\n"); \
        return true; \
    }

bool autosave_compare_saves(SaveContext* a, SaveContext* b) {
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.playerData.healthCapacity);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.playerData.isMagicAcquired);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.playerData.isDoubleMagicAcquired);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.playerData.doubleDefense);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.playerData.owlActivationFlags);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.inventory.items);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.inventory.upgrades);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.inventory.questItems);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.inventory.dungeonItems);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.inventory.dungeonKeys);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.inventory.strayFairies);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.permanentSceneFlags);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.dekuPlaygroundHighScores);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.scenesVisible);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.stolenItems);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.highScores);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.weekEventReg);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.regionsVisited);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.worldMapCloudVisibility);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.scarecrowSpawnSongSet);
    SAVE_COMPARE_MEMBER_ARRAY(a, b, save.saveInfo.scarecrowSpawnSong);
    // SAVE_COMPARE_MEMBER_ARRAY(a, b, eventInf);

    return false;
}

Gfx* Gfx_DrawRect_DropShadow(Gfx* gfx, s16 rectLeft, s16 rectTop, s16 rectWidth, s16 rectHeight, u16 dsdx, u16 dtdy,
                             s16 r, s16 g, s16 b, s16 a);

int autosave_icon_counter = 0;

#define AUTOSAVE_ICON_FADE_OUT_FRAMES 5
#define AUTOSAVE_ICON_FADE_IN_FRAMES 5
#define AUTOSAVE_ICON_SHOW_FRAMES 30
#define AUTOSAVE_ICON_TOTAL_FRAMES (AUTOSAVE_ICON_FADE_IN_FRAMES + AUTOSAVE_ICON_SHOW_FRAMES + AUTOSAVE_ICON_FADE_OUT_FRAMES)

#define AUTOSAVE_ICON_WIDTH 24
#define AUTOSAVE_ICON_HEIGHT 16
#define AUTOSAVE_ICON_DRAW_WIDTH (AUTOSAVE_ICON_WIDTH * 3 / 4)
#define AUTOSAVE_ICON_DRAW_HEIGHT (AUTOSAVE_ICON_HEIGHT * 3 / 4)
#define AUTOSAVE_ICON_X (SCREEN_WIDTH - AUTOSAVE_ICON_DRAW_WIDTH - 4)
#define AUTOSAVE_ICON_Y (SCREEN_HEIGHT - AUTOSAVE_ICON_DRAW_HEIGHT - 4)

INCBIN(autosave_icon, "autosave.rgba32.bin");

Gfx* GfxEx_DrawRect_DropShadow(Gfx* gfx, s16 rectLeft, s16 rectTop, s16 rectWidth, s16 rectHeight, u16 dsdx, u16 dtdy,
                             s16 r, s16 g, s16 b, s16 a, u16 origin) {
    s16 dropShadowAlpha = a;

    if (a > 100) {
        dropShadowAlpha = 100;
    }

    gDPPipeSync(gfx++);
    gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, dropShadowAlpha);
    gEXTextureRectangle(gfx++, origin, origin, (rectLeft + 2) * 4, (rectTop + 2) * 4, (rectLeft + rectWidth + 2) * 4,
                        (rectTop + rectHeight + 2) * 4, G_TX_RENDERTILE, 0, 0, dsdx, dtdy);

    gDPPipeSync(gfx++);
    gDPSetPrimColor(gfx++, 0, 0, r, g, b, a);

    gEXTextureRectangle(gfx++, origin, origin,  rectLeft * 4, rectTop * 4, (rectLeft + rectWidth) * 4, (rectTop + rectHeight) * 4,
                        G_TX_RENDERTILE, 0, 0, dsdx, dtdy);

    return gfx;
}

bool autosave_was_ready = false;

s32 recomp_autosave_debug_enabled() {
    return 0;
}

void draw_autosave_icon(PlayState* play) {
    s32 alpha = 0;
    if (autosave_icon_counter > (AUTOSAVE_ICON_SHOW_FRAMES + AUTOSAVE_ICON_FADE_OUT_FRAMES)) {
        alpha = (255 * (AUTOSAVE_ICON_FADE_IN_FRAMES - (autosave_icon_counter - AUTOSAVE_ICON_SHOW_FRAMES - AUTOSAVE_ICON_FADE_OUT_FRAMES))) / AUTOSAVE_ICON_FADE_IN_FRAMES;
    }
    else if (autosave_icon_counter > AUTOSAVE_ICON_FADE_OUT_FRAMES) {
        alpha = 255;
    }
    else if (autosave_icon_counter > 0) {
        alpha = (255 * autosave_icon_counter) / AUTOSAVE_ICON_FADE_OUT_FRAMES;
    }

    if (autosave_icon_counter > 0) {
        autosave_icon_counter--;
    }

    OPEN_DISPS(play->state.gfxCtx);

    if (alpha != 0) {
        gEXForceUpscale2D(OVERLAY_DISP++, 1);
        gDPLoadTextureBlock(OVERLAY_DISP++, autosave_icon, G_IM_FMT_RGBA, G_IM_SIZ_32b, AUTOSAVE_ICON_WIDTH, AUTOSAVE_ICON_HEIGHT, 0,
            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

        gDPSetCombineMode(OVERLAY_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        OVERLAY_DISP = GfxEx_DrawRect_DropShadow(OVERLAY_DISP, AUTOSAVE_ICON_X - SCREEN_WIDTH, AUTOSAVE_ICON_Y, AUTOSAVE_ICON_DRAW_WIDTH, AUTOSAVE_ICON_DRAW_HEIGHT,
            (s32)(1024.0f * AUTOSAVE_ICON_WIDTH / AUTOSAVE_ICON_DRAW_WIDTH), (s32)(1024.0f * AUTOSAVE_ICON_HEIGHT / AUTOSAVE_ICON_DRAW_HEIGHT),
            255, 255, 255, alpha, G_EX_ORIGIN_RIGHT);
        gEXForceUpscale2D(OVERLAY_DISP++, 0);
    }

    if (recomp_autosave_debug_enabled() && autosave_was_ready) {
        gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 0, 0, 255);
        gDPSetCombineLERP(OVERLAY_DISP++, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE);
        gEXFillRectangle(OVERLAY_DISP++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, 0, SCREEN_HEIGHT - 10, 20, SCREEN_HEIGHT);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

void show_autosave_icon() {
    autosave_icon_counter = AUTOSAVE_ICON_TOTAL_FRAMES;
}

u32 recomp_autosave_interval() {
    return 2 * 60 * 1000;
}

#define MIN_FRAMES_SINCE_CHANGED 10
#define MIN_FRAMES_SINCE_READY 20
OSTime last_autosave_time = 0;
u32 extra_autosave_delay_milliseconds = 0;

bool reached_final_three_hours() {
    // Logic copied with modifications from Interface_DrawClock.
    if ((CURRENT_DAY >= 4) ||
        ((CURRENT_DAY == 3) && (((void)0, gSaveContext.save.time) >= CLOCK_TIME(3, 0)) && (((void)0, gSaveContext.save.time) < CLOCK_TIME(6, 0)))
    ) {
        return true;
    }
    return false;
}

void autosave_reset_timer() {
    last_autosave_time = osGetTime();
    extra_autosave_delay_milliseconds = 0;
}

void autosave_reset_timer_slow() {
    // Set the most recent autosave time in the future to give extra time before an autosave triggers.
    last_autosave_time = osGetTime();
    extra_autosave_delay_milliseconds = 2 * 60 * 1000;
}

void autosave_post_play_update(PlayState* play) {
    static int frames_since_save_changed = 0;
    static int frames_since_autosave_ready = 0;
    if (recomp_autosave_enabled()) {
        if (autosave_compare_saves(&gSaveContext, &prev_save_ctx)) {
            frames_since_save_changed = 0;
            Lib_MemCpy(&prev_save_ctx, &gSaveContext, offsetof(SaveContext, fileNum));
        }
        else {
            frames_since_save_changed++;
        }

        OSTime time_now = osGetTime();

        // Check the following conditions:
        // * The UI is in a normal state.
        // * Time is passing.
        // * No message is on screen.
        // * The game is not paused.
        // * No cutscene is running.
        // * The game is not in cutscene mode.
        // * The clock has not reached the final 3 hours.
        // * The player is allowed to pause.
        if (gSaveContext.hudVisibility == HUD_VISIBILITY_ALL &&
            R_TIME_SPEED != 0 &&
            !Environment_IsTimeStopped() &&
            play->msgCtx.msgMode == MSGMODE_NONE &&
            play->pauseCtx.state == PAUSE_STATE_OFF &&
            gSaveContext.save.cutsceneIndex < 0xFFF0 &&
            !Play_InCsMode(play) &&
            !reached_final_three_hours() &&
            gCanPause
        ) {
            frames_since_autosave_ready++;
            autosave_was_ready = true;
        }
        else {
            frames_since_autosave_ready = 0;
            autosave_was_ready = false;
        }

        // Check if the frame count thresholds for the save data remaining unchanged and autosaving being ready have both been met
        // and that enough time has passed since the previous autosave to create a new one.
        if (frames_since_save_changed >= MIN_FRAMES_SINCE_CHANGED &&
            frames_since_autosave_ready >= MIN_FRAMES_SINCE_READY &&
            time_now - last_autosave_time > (OS_USEC_TO_CYCLES(1000 * (recomp_autosave_interval() + extra_autosave_delay_milliseconds)))
        ) {
            do_autosave(&play->sramCtx);
            show_autosave_icon();
            autosave_reset_timer();
        }
    }
    else {
        // Update the last autosave time to the current time to prevent autosaving immediately if autosaves are turned back on.
        autosave_reset_timer();
    }
    gCanPause = false;
}

void autosave_init() {
    autosave_reset_timer_slow();
    Lib_MemCpy(&prev_save_ctx, &gSaveContext, offsetof(SaveContext, fileNum));
}

extern s32 gFlashSaveSizes[];
extern u16 D_801C6A58[];

#define CHECK_NEWF(newf)                                                                                 \
    ((newf)[0] != 'Z' || (newf)[1] != 'E' || (newf)[2] != 'L' || (newf)[3] != 'D' || (newf)[4] != 'A' || \
     (newf)[5] != '3')

typedef struct {
    /* 0x00 */ s16 csId;
    /* 0x02 */ s16 length;
    /* 0x04 */ s16 endCsId;
    /* 0x06 */ s16 subCamId;
    /* 0x08 */ Actor* targetActor;
    /* 0x0C */ s32 startMethod;
    /* 0x10 */ PlayState* play;
    /* 0x14 */ s16 retCamId;
    /* 0x16 */ s16 isCameraStored;
} CutsceneManager; // size = 0x18

extern CutsceneManager sCutsceneMgr;
extern ActorCutscene* sSceneCutsceneList;
extern s16 sSceneCutsceneCount;

bool skip_entry_cutscene = false;

// @recomp Patched to skip the entrance cutscene if the flag is enabled.
s16 CutsceneManager_FindEntranceCsId(void) {
    PlayState* play;
    s32 csId;

    for (csId = 0; csId < sSceneCutsceneCount; csId++) {
        //! FAKE:
        if ((sSceneCutsceneList[csId].scriptIndex != CS_SCRIPT_ID_NONE) &&
            (sSceneCutsceneList[csId].scriptIndex < (play = sCutsceneMgr.play)->csCtx.scriptListCount) &&
            (sCutsceneMgr.play->curSpawn ==
             sCutsceneMgr.play->csCtx.scriptList[sSceneCutsceneList[csId].scriptIndex].spawn)) {

            // @recomp Check if the entry cutscene should be skipped and do so.
            if (skip_entry_cutscene) {
                skip_entry_cutscene = false;
                return -1;
            }
            return csId;
        }
    }

    for (csId = 0; csId < sSceneCutsceneCount; csId++) {
        if ((sSceneCutsceneList[csId].customValue >= 100) &&
            (sSceneCutsceneList[csId].customValue == (sCutsceneMgr.play->curSpawn + 100))) {
            return csId;
        }
    }

    return -1;
}

s32 spawn_entrance_from_autosave_entrance(s16 autosave_entrance) {
    s32 scene_id = Entrance_GetSceneIdAbsolute(gSaveContext.save.entrance);
    recomp_printf("Loaded entrance: %d in scene: %d\n", autosave_entrance, scene_id);

    switch (scene_id) {
        default:
            return ENTRANCE(SOUTH_CLOCK_TOWN, 0);
        case SCENE_MITURIN: // Woodfall Temple
        case SCENE_MITURIN_BS: // Odolwa's Lair
            return ENTRANCE(WOODFALL_TEMPLE, 0);
        case SCENE_HAKUGIN: // Snowhead Temple
        case SCENE_HAKUGIN_BS: // Goht's Lair
            return ENTRANCE(SNOWHEAD_TEMPLE, 0);
        case SCENE_SEA: // Great Bay Temple
        case SCENE_SEA_BS: // Gyorg's Lair
            return ENTRANCE(GREAT_BAY_TEMPLE, 0);
        case SCENE_INISIE_N: // Stone Tower Temple
            return ENTRANCE(STONE_TOWER_TEMPLE, 0);
        case SCENE_INISIE_R: // Inverted Stone Tower Temple
        case SCENE_INISIE_BS: // Twinmold's Lair
            return ENTRANCE(STONE_TOWER_TEMPLE_INVERTED, 0);
    }
}

// @recomp Patched to change the entrance for autosaves and initialize autosaves.
void Sram_OpenSave(FileSelectState* fileSelect, SramContext* sramCtx) {
    s32 i;
    s32 pad;
    s32 phi_t1 = 0;
    s32 pad1;
    s32 fileNum;

    if (gSaveContext.flashSaveAvailable) {
        bzero(sramCtx->saveBuf, SAVE_BUFFER_SIZE);

        if (gSaveContext.fileNum == 0xFF) {
            SysFlashrom_ReadData(sramCtx->saveBuf, gFlashSaveStartPages[0], gFlashSaveNumPages[0]);
        } else if (fileSelect->isOwlSave[gSaveContext.fileNum + 2]) {
            phi_t1 = gSaveContext.fileNum + 2;
            phi_t1 *= 2;

            if (SysFlashrom_ReadData(sramCtx->saveBuf, gFlashSaveStartPages[phi_t1], gFlashSaveNumPages[phi_t1]) != 0) {
                SysFlashrom_ReadData(sramCtx->saveBuf, gFlashSaveStartPages[phi_t1 + 1],
                                     gFlashSaveNumPages[phi_t1 + 1]);
            }
        } else {
            phi_t1 = gSaveContext.fileNum;
            phi_t1 *= 2;

            if (SysFlashrom_ReadData(sramCtx->saveBuf, gFlashSaveStartPages[phi_t1], gFlashSaveNumPages[phi_t1]) != 0) {
                SysFlashrom_ReadData(sramCtx->saveBuf, gFlashSaveStartPages[phi_t1 + 1],
                                     gFlashSaveNumPages[phi_t1 + 1]);
            }
        }

        Lib_MemCpy(&gSaveContext, sramCtx->saveBuf, gFlashSaveSizes[phi_t1]);

        if (CHECK_NEWF(gSaveContext.save.saveInfo.playerData.newf)) {
            SysFlashrom_ReadData(sramCtx->saveBuf, gFlashSaveStartPages[phi_t1 + 1], gFlashSaveNumPages[phi_t1 + 1]);
            Lib_MemCpy(&gSaveContext, sramCtx->saveBuf, gFlashSaveSizes[phi_t1]);
        }
    }

    gSaveContext.save.saveInfo.playerData.magicLevel = 0;

    if (!gSaveContext.save.isOwlSave) {
        for (i = 0; i < ARRAY_COUNT(gSaveContext.eventInf); i++) {
            gSaveContext.eventInf[i] = 0;
        }

        for (i = 0; i < ARRAY_COUNT(gSaveContext.cycleSceneFlags); i++) {
            gSaveContext.cycleSceneFlags[i].chest = gSaveContext.save.saveInfo.permanentSceneFlags[i].chest;
            gSaveContext.cycleSceneFlags[i].switch0 = gSaveContext.save.saveInfo.permanentSceneFlags[i].switch0;
            gSaveContext.cycleSceneFlags[i].switch1 = gSaveContext.save.saveInfo.permanentSceneFlags[i].switch1;
            gSaveContext.cycleSceneFlags[i].clearedRoom = gSaveContext.save.saveInfo.permanentSceneFlags[i].clearedRoom;
            gSaveContext.cycleSceneFlags[i].collectible = gSaveContext.save.saveInfo.permanentSceneFlags[i].collectible;
        }

        for (i = 0; i < TIMER_ID_MAX; i++) {
            gSaveContext.timerStates[i] = TIMER_STATE_OFF;
            gSaveContext.timerCurTimes[i] = SECONDS_TO_TIMER(0);
            gSaveContext.timerTimeLimits[i] = SECONDS_TO_TIMER(0);
            gSaveContext.timerStartOsTimes[i] = 0;
            gSaveContext.timerStopTimes[i] = SECONDS_TO_TIMER(0);
            gSaveContext.timerPausedOsTimes[i] = 0;
        }

        if (gSaveContext.save.isFirstCycle) {
            gSaveContext.save.entrance = ENTRANCE(SOUTH_CLOCK_TOWN, 0);
            gSaveContext.save.day = 0;
            gSaveContext.save.time = CLOCK_TIME(6, 0) - 1;
        } else {
            gSaveContext.save.entrance = ENTRANCE(CUTSCENE, 0);
            gSaveContext.nextCutsceneIndex = 0;
            gSaveContext.save.playerForm = PLAYER_FORM_HUMAN;
        }
    }
    // @recomp Handle autosaves.
    else if (gSaveContext.save.isOwlSave == SAVE_TYPE_AUTOSAVE) {
        gSaveContext.save.entrance = spawn_entrance_from_autosave_entrance(gSaveContext.save.entrance);

        // Skip the turtle cutscene that happens when entering Great Bay Temple.
        if (gSaveContext.save.entrance == ENTRANCE(GREAT_BAY_TEMPLE, 0)) {
            skip_entry_cutscene = true;
        }

        for (i = 0; i < ARRAY_COUNT(gSaveContext.cycleSceneFlags); i++) {
            gSaveContext.cycleSceneFlags[i].chest = gSaveContext.save.saveInfo.permanentSceneFlags[i].chest;
            gSaveContext.cycleSceneFlags[i].switch0 = gSaveContext.save.saveInfo.permanentSceneFlags[i].switch0;
            gSaveContext.cycleSceneFlags[i].switch1 = gSaveContext.save.saveInfo.permanentSceneFlags[i].switch1;
            gSaveContext.cycleSceneFlags[i].clearedRoom = gSaveContext.save.saveInfo.permanentSceneFlags[i].clearedRoom;
            gSaveContext.cycleSceneFlags[i].collectible = gSaveContext.save.saveInfo.permanentSceneFlags[i].collectible;
        }

        if (gSaveContext.save.saveInfo.scarecrowSpawnSongSet) {
            Lib_MemCpy(gScarecrowSpawnSongPtr, gSaveContext.save.saveInfo.scarecrowSpawnSong,
                       sizeof(gSaveContext.save.saveInfo.scarecrowSpawnSong));

            for (i = 0; i != ARRAY_COUNT(gSaveContext.save.saveInfo.scarecrowSpawnSong); i++) {}
        }

        fileNum = gSaveContext.fileNum;
        func_80147314(sramCtx, fileNum);
    }
    else {
        gSaveContext.save.entrance = D_801C6A58[(void)0, gSaveContext.save.owlSaveLocation];
        if ((gSaveContext.save.entrance == ENTRANCE(SOUTHERN_SWAMP_POISONED, 10)) &&
            CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE)) {
            gSaveContext.save.entrance = ENTRANCE(SOUTHERN_SWAMP_CLEARED, 10);
        } else if ((gSaveContext.save.entrance == ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 8)) &&
                   CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE)) {
            gSaveContext.save.entrance = ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 8);
        }

        for (i = 0; i < ARRAY_COUNT(gSaveContext.cycleSceneFlags); i++) {
            gSaveContext.cycleSceneFlags[i].chest = gSaveContext.save.saveInfo.permanentSceneFlags[i].chest;
            gSaveContext.cycleSceneFlags[i].switch0 = gSaveContext.save.saveInfo.permanentSceneFlags[i].switch0;
            gSaveContext.cycleSceneFlags[i].switch1 = gSaveContext.save.saveInfo.permanentSceneFlags[i].switch1;
            gSaveContext.cycleSceneFlags[i].clearedRoom = gSaveContext.save.saveInfo.permanentSceneFlags[i].clearedRoom;
            gSaveContext.cycleSceneFlags[i].collectible = gSaveContext.save.saveInfo.permanentSceneFlags[i].collectible;
        }

        if (gSaveContext.save.saveInfo.scarecrowSpawnSongSet) {
            Lib_MemCpy(gScarecrowSpawnSongPtr, gSaveContext.save.saveInfo.scarecrowSpawnSong,
                       sizeof(gSaveContext.save.saveInfo.scarecrowSpawnSong));

            for (i = 0; i != ARRAY_COUNT(gSaveContext.save.saveInfo.scarecrowSpawnSong); i++) {}
        }

        fileNum = gSaveContext.fileNum;
        func_80147314(sramCtx, fileNum);
    }

    // @recomp Initialize the autosave state tracking.
    autosave_init();
}

extern s32 Actor_ProcessTalkRequest(Actor* actor, GameState* gameState);

// @recomp Reset the autosave timer when the moon crashes.
void Sram_ResetSaveFromMoonCrash(SramContext* sramCtx) {
    s32 i;
    s32 cutsceneIndex = gSaveContext.save.cutsceneIndex;

    bzero(sramCtx->saveBuf, SAVE_BUFFER_SIZE);

    if (SysFlashrom_ReadData(sramCtx->saveBuf, gFlashSaveStartPages[gSaveContext.fileNum * 2],
                             gFlashSaveNumPages[gSaveContext.fileNum * 2]) != 0) {
        SysFlashrom_ReadData(sramCtx->saveBuf, gFlashSaveStartPages[gSaveContext.fileNum * 2 + 1],
                             gFlashSaveNumPages[gSaveContext.fileNum * 2 + 1]);
    }
    Lib_MemCpy(&gSaveContext.save, sramCtx->saveBuf, sizeof(Save));
    if (CHECK_NEWF(gSaveContext.save.saveInfo.playerData.newf)) {
        SysFlashrom_ReadData(sramCtx->saveBuf, gFlashSaveStartPages[gSaveContext.fileNum * 2 + 1],
                             gFlashSaveNumPages[gSaveContext.fileNum * 2 + 1]);
        Lib_MemCpy(&gSaveContext, sramCtx->saveBuf, sizeof(Save));
    }
    gSaveContext.save.cutsceneIndex = cutsceneIndex;

    for (i = 0; i < ARRAY_COUNT(gSaveContext.eventInf); i++) {
        gSaveContext.eventInf[i] = 0;
    }

    for (i = 0; i < ARRAY_COUNT(gSaveContext.cycleSceneFlags); i++) {
        gSaveContext.cycleSceneFlags[i].chest = gSaveContext.save.saveInfo.permanentSceneFlags[i].chest;
        gSaveContext.cycleSceneFlags[i].switch0 = gSaveContext.save.saveInfo.permanentSceneFlags[i].switch0;
        gSaveContext.cycleSceneFlags[i].switch1 = gSaveContext.save.saveInfo.permanentSceneFlags[i].switch1;
        gSaveContext.cycleSceneFlags[i].clearedRoom = gSaveContext.save.saveInfo.permanentSceneFlags[i].clearedRoom;
        gSaveContext.cycleSceneFlags[i].collectible = gSaveContext.save.saveInfo.permanentSceneFlags[i].collectible;
    }

    for (i = 0; i < TIMER_ID_MAX; i++) {
        gSaveContext.timerStates[i] = TIMER_STATE_OFF;
        gSaveContext.timerCurTimes[i] = SECONDS_TO_TIMER(0);
        gSaveContext.timerTimeLimits[i] = SECONDS_TO_TIMER(0);
        gSaveContext.timerStartOsTimes[i] = 0;
        gSaveContext.timerStopTimes[i] = SECONDS_TO_TIMER(0);
        gSaveContext.timerPausedOsTimes[i] = 0;
    }

    D_801BDAA0 = true;
    gHorseIsMounted = false;
    gSaveContext.powderKegTimer = 0;
    gSaveContext.unk_1014 = 0;
    gSaveContext.jinxTimer = 0;

    // @recomp Use the slow autosave timer to give the player extra time to respond to the moon crashing to decide if they want to reload their autosave.
    autosave_reset_timer_slow();
}


// @recomp If autosave is enabled, skip the part of the owl statue dialog that talks about the file being deleted on load, since it's not true.
void ObjWarpstone_Update(Actor* thisx, PlayState* play) {
    ObjWarpstone* this = (ObjWarpstone*)thisx;
    s32 pad;

    if (this->isTalking) {
        if (Actor_TextboxIsClosing(&this->dyna.actor, play)) {
            this->isTalking = false;
        } else if ((Message_GetState(&play->msgCtx) == TEXT_STATE_CHOICE) && Message_ShouldAdvance(play)) {
            if (play->msgCtx.choiceIndex != 0) {
                Audio_PlaySfx_MessageDecide();
                play->msgCtx.msgMode = MSGMODE_OWL_SAVE_0;
                play->msgCtx.unk120D6 = 0;
                play->msgCtx.unk120D4 = 0;
                gSaveContext.save.owlSaveLocation = OBJ_WARPSTONE_GET_ID(&this->dyna.actor);
            } else {
                Message_CloseTextbox(play);
            }
        }
    } else if (Actor_ProcessTalkRequest(&this->dyna.actor, &play->state)) {
        this->isTalking = true;
    } else if (!this->actionFunc(this, play)) {
        Actor_OfferTalkNearColChkInfoCylinder(&this->dyna.actor, play);
    }

    // @recomp Skip the text talking about the save being deleted on load, if autosave is enabled.
    if (recomp_autosave_enabled()) {
        if (this->isTalking && play->msgCtx.currentTextId == 0xC01 && play->msgCtx.msgBufPos == 269) {
            play->msgCtx.msgBufPos = 530;
        }
    }

    Collider_ResetCylinderAC(play, &this->collider.base);
    Collider_UpdateCylinder(&this->dyna.actor, &this->collider);
    CollisionCheck_SetOC(play, &play->colChkCtx, &this->collider.base);
    CollisionCheck_SetAC(play, &play->colChkCtx, &this->collider.base);
}
